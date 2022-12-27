#pragma once

#include <circular_buffer.hpp>

#include <mqtt/async_client.h>

#include <atomic>
#include <thread>
#include <vector>

namespace HomeAutomation {
namespace IO {
namespace MQTT {

using Messages = HomeAutomation::circular_buffer<mqtt::message_ptr, 128>;
using ConstMessages =
    HomeAutomation::circular_buffer<mqtt::const_message_ptr, 128>;
using Topics = std::vector<std::string>;

class Callback : public virtual mqtt::callback,
                 public virtual mqtt::iaction_listener {
public:
  Callback(mqtt::async_client &client, Topics &topics, ConstMessages &messages,
           mqtt::connect_options &connOpts)
      : client(client), topics(topics), messages(messages), connOpts(connOpts) {
  }
  virtual void connected(const std::string & /*cause*/) override;
  virtual void connection_lost(const std::string &cause) override;
  virtual void delivery_complete(mqtt::delivery_token_ptr tok) override;
  virtual void message_arrived(mqtt::const_message_ptr msg) override;
  virtual void on_failure(const mqtt::token &asyncActionToken) override {
    (void)asyncActionToken;
  }
  virtual void on_success(const mqtt::token &asyncActionToken) override {
    (void)asyncActionToken;
  }

private:
  void reconnect();

  mqtt::async_client &client;
  Topics &topics;
  ConstMessages &messages;
  mqtt::connect_options &connOpts;
};

class ClientPaho {
public:
  static constexpr const char *DFLT_SERVER_ADDRESS{"tcp://localhost:1883"};
  static constexpr const char *DFLT_CLIENT_ID{"async_publish"};

  static mqtt::connect_options getDefaultConnectOptions() {
    mqtt::connect_options result;
    result.set_keep_alive_interval(20);
    result.set_clean_session(true);
    return result;
  }

  ClientPaho(std::string const &address, std::string const &clientID,
             mqtt::connect_options connOpts)
      : client(address, clientID), conntok{}, send_msgs{}, recv_msgs{},
        quit_cond(false), topics{}, connOpts{connOpts},
        cb{client, topics, recv_msgs, connOpts}, send_worker{} {
    client.set_callback(cb);
  }
  ClientPaho(std::string const &address, std::string const &clientID)
      : ClientPaho(address, clientID, getDefaultConnectOptions()) {}
  ClientPaho(std::string const &address)
      : ClientPaho(address, DFLT_CLIENT_ID) {}
  ClientPaho() : ClientPaho(DFLT_SERVER_ADDRESS) {}
  virtual ~ClientPaho() {}

  void connect();
  void send(mqtt::string_ref topic, mqtt::binary_ref payload, int qos);
  inline void send(mqtt::string_ref topic, mqtt::binary_ref payload) {
    send(topic, payload, 1 /* QOS, TODO */);
  }
  void subscribe(std::string_view const &topic, int qos);
  inline void subscribe(std::string_view const &topic) {
    subscribe(topic, 1 /* QOS, TODO */);
  }
  mqtt::const_message_ptr receive();
  void disconnect();

private:
  ClientPaho(ClientPaho const &);
  ClientPaho &operator=(ClientPaho const &);

  void sendWorkerFun();

  mqtt::async_client client;
  mqtt::token_ptr conntok;
  Messages send_msgs;
  ConstMessages recv_msgs;
  std::atomic_bool quit_cond;
  Topics topics;
  mqtt::connect_options connOpts;
  Callback cb;
  std::thread send_worker;
};

} // namespace MQTT
} // namespace IO
} // namespace HomeAutomation
