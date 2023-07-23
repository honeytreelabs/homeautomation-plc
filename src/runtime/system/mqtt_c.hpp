#pragma once

#include <mqtt.hpp>

#include <MQTTAsync.h>
#include <spdlog/spdlog.h>

#include <mutex>
#include <optional>
#include <stdexcept>

namespace HomeAutomation {
namespace IO {
namespace MQTT {

using Subscriptions = std::vector<SubscribedTopic>;

static void onConnect_wrapper(void *context, MQTTAsync_successData *response);
static void onConnectFailure_wrapper(void *context,
                                     MQTTAsync_failureData *response);
static void onConnected_wrapper(void *context, char *cause);
static void connlost_wrapper(void *context, char *cause);
static int msgarrvd_wrapper(void *context, char *topicName, int topicLen,
                            MQTTAsync_message *message);

class ClientPahoC : public RawClient {
public:
  ClientPahoC(std::string const &address, std::string const &clientID) {
    int rc = MQTTAsync_create(&client, address.c_str(), clientID.c_str(),
                              MQTTCLIENT_PERSISTENCE_NONE, nullptr);
    if (rc != MQTTASYNC_SUCCESS) {
      throw std::runtime_error("Could not create MQTT client.");
    }
  }

  ~ClientPahoC() {
    std::lock_guard lock{mutex};
    MQTTAsync_destroy(&client);
  }

  bool connect(ConnectOptions const &options) override {
    using namespace std::chrono_literals;

    std::lock_guard lock{mutex};
    if (connecting || connected) {
      return true;
    }
    connecting = true;
    if (!running) {
      auto rc = MQTTAsync_setCallbacks(client, this, connlost_wrapper,
                                       msgarrvd_wrapper /* message arrived */,
                                       nullptr /* delivery complete */);
      if (rc != MQTTASYNC_SUCCESS) {
        spdlog::error("Could not register MQTT callbacks: {}",
                      MQTTAsync_strerror(rc));
        // return false;
      }
      rc = MQTTAsync_setConnected(client, this, onConnected_wrapper);
      if (rc != MQTTASYNC_SUCCESS) {
        spdlog::error("Could not register MQTT connected callback: {}",
                      MQTTAsync_strerror(rc));
        // return false;
      }
    }

    MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
    conn_opts.onSuccess = onConnect_wrapper;
    conn_opts.onFailure = onConnectFailure_wrapper;
    conn_opts.context = this;
    conn_opts.connectTimeout = 1;
    conn_opts.automaticReconnect =
        0; // we do it ourselves because then we have more control over it
    if (options.user && options.pass) {
      conn_opts.username = options.user.value().c_str();
      conn_opts.password = options.pass.value().c_str();
    }
    return MQTTAsync_connect(client, &conn_opts) == MQTTASYNC_SUCCESS;
  }

  bool is_connected() const override { return connected; }

  bool publish(Message const &message) override {
    send_msgs.put(message); // optimize: std::move
    return true;
  }

  bool subscribe(SubscribedTopic const &topic) override {
    {
      std::lock_guard lock{subs_mutex};
      subscriptions.push_back(topic);
    }
    return subscribe_raw(topic);
  }

  OptionalMessage receive() override { return recv_msgs.get(); }

  void set_on_resubscribe(SubscriptionCb cb) override { subs_cb = cb; }

  void disconnect() override {
    using namespace std::chrono_literals;

    running = false;
    while (connecting) {
      std::this_thread::sleep_for(100ms);
    }
    if (send_worker.joinable()) {
      send_worker.join();
    }
    subscriptions.clear();
    MQTTAsync_disconnectOptions disc_opts =
        MQTTAsync_disconnectOptions_initializer;
    disc_opts.timeout = 2000 /* milliseconds */;
    std::lock_guard lock{mutex};
    MQTTAsync_disconnect(client, &disc_opts);
    connected = false;
  }

  // MQTT C API callbacks

  void on_connect(MQTTAsync_successData *response) { (void)response; }

  void on_connected(char *cause) {
    (void)cause;

    spdlog::info("Connected to MQTT broker.");

    connected = true;
    running = true;
    connecting = false;
    if (!send_worker.joinable()) {
      send_worker = std::thread(&ClientPahoC::send_worker_logic, this);
    }
    std::lock_guard lock{subs_mutex};
    for (auto const &topic : subscriptions) {
      subscribe_raw(topic);
    }
  }

  void on_connect_failure(MQTTAsync_failureData *response) {
    (void)response;

    connected = false;
    connecting = false;
  }

  void connlost(char const *cause) {
    (void)cause;
    connected = false;
  }

  int msgarrvd(char *topicName, int topicLen, MQTTAsync_message *message) {
    (void)topicLen;

    auto put_msg = Message{
        topicName,
        Payload{static_cast<char *>(message->payload),
                static_cast<char *>(message->payload) + message->payloadlen},
        Message::DEFAULT_QOS};

    spdlog::info("Message \"{}\" arrived on topic \"{}\"",
                 put_msg.payload_str(), topicName);

    recv_msgs.put(std::move(put_msg));
    MQTTAsync_freeMessage(&message);
    MQTTAsync_free(topicName);
    return 1; // success
  }

private:
  void send_worker_logic() {
    using namespace std::chrono_literals;

    while (running) {
      auto opt_message = send_msgs.get_for(100ms);
      if (!opt_message) {
        continue;
      }
      auto &message = opt_message.value();
      MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
      MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
      pubmsg.qos = message.qos();
      pubmsg.payload = const_cast<void *>(
          static_cast<void const *>(message.payload().data()));
      pubmsg.payloadlen = message.payload().size();
      std::lock_guard lock{mutex};
      auto rc = MQTTAsync_sendMessage(client, message.topic().c_str(), &pubmsg,
                                      &opts);
      if (rc != MQTTASYNC_SUCCESS) {
        spdlog::error("Could not send MQTT message.");
      }
    }
  }

  bool subscribe_raw(SubscribedTopic const &topic) {
    spdlog::info("Subscribing to topic {}", topic.topic);
    MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
    if (subs_cb) {
      subs_cb(topic);
    }
    std::lock_guard lock{mutex};
    auto rc =
        MQTTAsync_subscribe(client, topic.topic.c_str(), topic.qos, &opts);
    if (rc != MQTTASYNC_SUCCESS) {
      spdlog::error("Could not subscribe to topic {} due to {}", topic.topic,
                    MQTTAsync_strerror(rc));
      return false;
    }
    spdlog::info("Successfully subscribed to topic {}", topic.topic);
    return true;
  }

  std::atomic_bool running;
  MQTTAsync client = nullptr;
  std::atomic_bool connected = false;
  std::atomic_bool connecting = false;
  std::thread send_worker;
  Messages recv_msgs;
  Messages send_msgs;
  Subscriptions subscriptions;
  std::mutex subs_mutex;
  std::mutex mutex;
  SubscriptionCb subs_cb;
};

void onConnect_wrapper(void *context, MQTTAsync_successData *response) {
  auto client = static_cast<ClientPahoC *>(context);
  client->on_connect(response);
}

void onConnectFailure_wrapper(void *context, MQTTAsync_failureData *response) {
  auto client = static_cast<ClientPahoC *>(context);
  client->on_connect_failure(response);
}

void onConnected_wrapper(void *context, char *cause) {
  auto client = static_cast<ClientPahoC *>(context);
  client->on_connected(cause);
}

void connlost_wrapper(void *context, char *cause) {
  auto client = static_cast<ClientPahoC *>(context);
  client->connlost(cause);
}

int msgarrvd_wrapper(void *context, char *topicName, int topicLen,
                     MQTTAsync_message *message) {
  auto client = static_cast<ClientPahoC *>(context);
  return client->msgarrvd(topicName, topicLen, message);
}

} // namespace MQTT
} // namespace IO
} // namespace HomeAutomation
