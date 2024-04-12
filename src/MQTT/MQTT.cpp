#include "MQTT.hpp"

MQTT::MQTT() {
    this->client.setServer(MQTT_HOST, MQTT_PORT);
    this->client.setClient(espClient);
    #ifdef CERT
    this->espClient.setCACert(root_ca);
    #endif
    using std::placeholders::_1;
    using std::placeholders::_2;
    using std::placeholders::_3;
    this->client.setCallback(std::bind(&MQTT::subscribeCallback, this, _1, _2, _3));
    this->client.setBufferSize(MQTT_PUBLISH_MESSAGE_SIZE);
    this->startup = true;
    this->datetimeSetted = false;
}

void MQTT::reconnect()
{
    // Loop until we’re reconnected
    while (!this->client.connected())
    {
        Serial.print("Attempting MQTT connection…");
        String clientId = BOARD_ID;
        clientId += String(random(0xffff), HEX); // Create a random client ID
        // Attempt to connect
        #if defined(MQTT_USER) && defined(MQTT_PASSWORD)
            bool attempt = client.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWORD);
        #else
            bool attempt = client.connect(clientId.c_str());
        #endif
        if (attempt)
        {
            Serial.println("connected");
            
            if(this->client.subscribe(MQTT_CMD_TOPIC)){
                Serial.println("Subscribed to /cmd");
            }
            if (this->startup)
            {
                if(this->client.subscribe(MQTT_DATETIME_TOPIC))
                {
                    Serial.println("Subscribed to /datetime");
                }
                this->startup = false;
            }
        }
        else
        {
            Serial.print("failed, rc=");
            Serial.print(this->client.state());
            Serial.println(" try again in 5 seconds"); // Wait 5 seconds before retrying
            delay(5000);
        }
    }
}

bool MQTT::publishMessage(const char *topic, String payload, bool retained)
{
    bool status = this->client.publish(topic, payload.c_str(), retained);
    return status;
}

void MQTT::setDatetime(String datetime)
{
    uint64_t datetimeD = datetime.toDouble();
    if (datetimeD > 0){
        setTime(datetimeD);
        // this->client.unsubscribe(MQTT_DATETIME_TOPIC);
    }
    this->datetimeSetted = true;
}

void MQTT::subscribeCallback(char *topic, byte *payload, unsigned int length)
{
    #ifdef DEBUG
        SERIAL_DEBUG.print("Message arrived [");
        SERIAL_DEBUG.print(topic);
        SERIAL_DEBUG.print("]: ");
    #endif
    String payloadStr;
    for (int i = 0; i < length; i++)
    {
        payloadStr += (char)payload[i];
    }
    #ifdef DEBUG
        SERIAL_DEBUG.println(payloadStr);
    #endif

    parseMessagge(topic, payloadStr.c_str());
}

void MQTT::parseMessagge(char *topic, const char *payloadStr)
{
    if (strcmp(topic, MQTT_DATETIME_TOPIC) == 0)
    {
        this->setDatetime(payloadStr);
    }
    if (strcmp(topic, MQTT_CMD_TOPIC) == 0)
    {
        DynamicJsonDocument doc(1024);
        deserializeJson(doc, payloadStr);
        JsonArray array = doc.as<JsonArray>();
        for(JsonVariant v : array) {
            DynamicJsonDocument elem(1024);
            deserializeJson(elem, v.as<String>());
            if (strcmp(elem["id"], BOARD_ID) == 0) {
                this->cmdRun = elem["run"];
                this->cmdLoad = elem["load"];
            }
            elem.clear();
        }
        // OLD VERSION
        // if (strcmp(doc["id"], BOARD_ID) == 0) {
        //     this->cmdRun = doc["run"];
        //     this->cmdLoad = doc["load"];
        // }
        doc.clear();
    }
}