# :point_right: Getting Started

| [Create the Client Certificate](#lock-create-the-client-certificate) | [Configure Event Grid Namespaces](#triangular_ruler-configure-event-grid-namespaces) | [Configure Mosquitto](#fly-configure-mosquitto) | [Run the PoC](#game_die-run-the-PoC) |

This scenario showcases how to create resources such as client, topic spaces, and permission bindings to publish and subscribe MQTT messages.

The PoC provides step by step instructions on how to perform following tasks:

- Create client certificate, which is used to authenticate the client connection
- Create the resources including client, topic spaces, permission bindings
- Use $all client group, which is the default client group with all the clients in a namespace, to authorize publish and subscribe access in permission bindings
- Connect with MQTT 3.1.1
  - Validate TLS certificate enforcing TLS 1.2
  - Authenticate the client connection using client certificates
  - Configure connection settings such as KeepAlive and CleanSession
- Publish messages to a topic
- Subscribe to a topic to receive messages

|Client|Role|Operation|Topic/Topic Filter|
|------|----|---------|------------------|
|raspberry_pi_client|publisher|publish|devices/raspberry_pi|
|server_client|subscriber|subscribe|devices/+|


##  :lock: Create the client certificate

Using the CA files, as described in [setup](../../Setup.md), 
* create a certificates for `raspberry_pi_client` client.  Client certificate is created with subject name as "raspberry_pi_client".  This must match the authentication name of the client.
* create a certificates for `server_client` client.  Client certificate is created with subject name as "server_client".  This must match the authentication name of the client.

```bash
# from folder scenarios/raspberry_pi
step certificate create \
    raspberry_pi_client raspberry_pi_client.pem raspberry_pi_client.key \
    --ca ~/.step/certs/intermediate_ca.crt \
    --ca-key ~/.step/secrets/intermediate_ca_key \
    --no-password --insecure \
    --not-after 2400h
```


```bash
# from folder scenarios/raspberry_pi
step certificate create \
    server_client server_client.pem server_client.key \
    --ca ~/.step/certs/intermediate_ca.crt \
    --ca-key ~/.step/secrets/intermediate_ca_key \
    --no-password --insecure \
    --not-after 2400h
```

## :triangular_ruler: Configure Event Grid Namespaces

Ensure to create an Event Grid namespace by following the steps in [setup](../setup).  Event Grid namespace requires registering the client, and the topic spaces to authorize the publish/subscribe permissions.

### Create the Client

We will use the SubjectMatchesAuthenticationName validation scheme for `raspberry_pi_client` and `server_client` to create the client from the portal or with the scripts:

```bash
# from folder scenarios/raspberry_pi
source ../../az.env

az resource create --id "$res_id/clients/raspberry_pi_client" --properties '{
    "authenticationName": "raspberry_pi_client",
    "state": "Enabled",
    "clientCertificateAuthentication": {
        "validationScheme": "SubjectMatchesAuthenticationName"
    },
    "attributes": {
        "type": "raspberry_pi-client"
    },
    "description": "This is a test publisher client"
}'
```

```bash
# from folder scenarios/raspberry_pi
source ../../az.env

az resource create --id "$res_id/clients/server_client" --properties '{
    "authenticationName": "server_client",
    "state": "Enabled",
    "clientCertificateAuthentication": {
        "validationScheme": "SubjectMatchesAuthenticationName"
    },
    "attributes": {
        "type": "server-client"
    },
    "description": "This is a test subscriber client"
}'
```

### Create topic spaces and permission bindings
Run the commands to create the "devicespace" topic space, and the two permission bindings that provide publish and subscribe access to $all client group on the devicespace space.

```bash
# from folder scenarios/raspberry_pi
source ../../az.env

az resource create --id "$res_id/topicSpaces/devicespace" --properties '{
    "topicTemplates": ["devices/#"]
}'

az resource create --id "$res_id/permissionBindings/raspberryPiPub" --properties '{
    "clientGroupName":"$all",
    "topicSpaceName":"devicespace",
    "permission":"Publisher"
}'

az resource create --id "$res_id/permissionBindings/serverSub" --properties '{
    "clientGroupName":"$all",
    "topicSpaceName":"devicespace",
    "permission":"Subscriber"
}'
```

### Create the .env file with connection details

The required `.env` files can be configured manually, we provide the script below as a reference to create those files, as they are ignored from git.

```bash
# from folder scenarios/raspberry_pi
source ../../az.env
host_name=$(az resource show --ids $res_id --query "properties.topicSpacesConfiguration.hostname" -o tsv)

echo "MQTT_HOST_NAME=$host_name" > raspberry_pi_client.env
echo "MQTT_USERNAME=raspberry_pi_client" >> raspberry_pi_client.env
echo "MQTT_CLIENT_ID=raspberry_pi_client" >> raspberry_pi_client.env
echo "MQTT_CERT_FILE=raspberry_pi_client.pem" >> raspberry_pi_client.env
echo "MQTT_KEY_FILE=raspberry_pi_client.key" >> raspberry_pi_client.env
```

```bash
# from folder scenarios/raspberry_pi
source ../../az.env
host_name=$(az resource show --ids $res_id --query "properties.topicSpacesConfiguration.hostname" -o tsv)

echo "MQTT_HOST_NAME=$host_name" > server_client.env
echo "MQTT_USERNAME=server_client" >> server_client.env
echo "MQTT_CLIENT_ID=server_client" >> server_client.env
echo "MQTT_CERT_FILE=server_client.pem" >> server_client.env
echo "MQTT_KEY_FILE=server_client.key" >> server_client.env
```


## :game_die: Run the PoC

The PoC are designed to be executed from the root scenario folder.

### C

```bash
# from the root of the repo
cmake --preset=raspberry_pi
cmake --build --preset=raspberry_pi
```
This will generate the produced binary in `scenarios/raspberry_pi/c/build/`

```bash
# from folder scenarios/raspberry_pi
c/build/raspberry_pi_client raspberry_pi_client.env
```
```bash
c/build/server_client server_client.env
```

For alternate building/running methods and more information, see the [C documentation](../../mqttclients/c/README.md).