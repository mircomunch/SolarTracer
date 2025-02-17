#include <MQTT.hpp>

// MQTT::MQTT() {
//     this->client.setServer(MQTT_HOST, MQTT_PORT);
//     this->client.setClient(espClient);
//     #ifdef CERT
//     this->espClient.setCACert(root_ca);
//     #endif
//     using std::placeholders::_1;
//     using std::placeholders::_2;
//     using std::placeholders::_3;
//     this->client.setCallback(std::bind(&MQTT::subscribeCallback, this, _1, _2, _3));
//     this->client.setBufferSize(MQTT_PUBLISH_MESSAGE_SIZE);
//     this->startup = true;
//     this->datetimeSetted = false;
// }
MQTT::MQTT(String board_id, char *host, uint16_t port)
{
    this->_board_id = board_id;
    this->_host = host;
    this->_port = port;
    this->_cert_en = false;
    this->client.setServer(this->_host, this->_port);
    this->client.setClient(this->_unsecuredClient);
}
MQTT::MQTT(String board_id, char *host, uint16_t port, int msg_size)
{
    this->_board_id = board_id;
    this->_host = host;
    this->_port = port;
    this->_msg_size = msg_size;
    this->_cert_en = false;
    this->client.setServer(this->_host, this->_port);
    this->client.setClient(this->_unsecuredClient);
    this->client.setBufferSize(this->_msg_size);
}
MQTT::MQTT(String board_id, char *host, uint16_t port, int msg_size, bool cert_en, char *user, char *psw)
{
    this->_board_id = board_id;
    this->_host = host;
    this->_port = port;
    this->_msg_size = msg_size;
    this->_cert_en = cert_en;
    this->_user = user;
    this->_psw = psw;
    this->client.setServer(this->_host, this->_port);
    this->client.setClient(this->_securedClient);
    if (_cert_en)
    {
        this->_securedClient.setCACert(root_ca);
    }
    this->client.setBufferSize(this->_msg_size);
}

// void MQTT::reconnect()
// {
//     // Loop until we’re reconnected
//     while (!this->client.connected())
//     {
            
//             if(this->client.subscribe(MQTT_CMD_TOPIC)){
//                 Serial.println("Subscribed to /cmd");
//             }
//             if (this->startup)
//             {
//                 if(this->client.subscribe(MQTT_DATETIME_TOPIC))
//                 {
//                     Serial.println("Subscribed to /datetime");
//                 }
//                 this->startup = false;
//             }
//         }
//         else
//         {
//             Serial.print("failed, rc=");
//             Serial.print(this->client.state());
//             Serial.println(" try again in 5 seconds"); // Wait 5 seconds before retrying
//             delay(5000);
//         }
//     }
// }
bool MQTT::reconnect()
{
    Serial.print("Attempting MQTT connection… ");
    String clientId = this->_board_id;
    clientId += String(random(0xffff), HEX); // Create a random client ID

    // Attempt to connect
    if (this->_user && this->_psw)
    {
        Serial.println("authenticated");
        this->client.connect(clientId.c_str(), this->_user, this->_psw);
    }
    else
    {
        Serial.println("un-authenticated");
        this->client.connect(clientId.c_str());
    }
    return client.connected();
}

bool MQTT::publishMessage(const char *topic, String payload, bool retained)
{
    return this->client.publish(topic, payload.c_str(), retained);
}