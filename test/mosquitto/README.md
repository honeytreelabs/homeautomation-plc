# Credits

Taken from: https://github.com/vvatelot/mosquitto-docker-compose

# Authentication

## Enable authentication

In the config file, just uncomment the `Authentication` part and then restart the container.
The default user is `admin/password`.

**You always have to restart if you want the modification to be taken in account:**

```bash
docker compose up -d --force-recreate
```

### Change user password / create a new user

```bash
docker compose exec mosquitto mosquitto_passwd -b /mosquitto/config/password.txt user password
```

### Delete user

```bash
docker compose exec mosquitto mosquitto_passwd -D /mosquitto/config/password.txt user
```
