//To do:
// - implement event handled
// - implement queue in case there is no connection

#include "CloudConnect.h"

CloudConnect::CloudConnect(byte* server, int port) {
    logger = new Logger("CloudConnect");
    this->server = server;
    this->port = port;
    
    logger->trace("Initializing CloudConnect instance");
    connect();
}

bool CloudConnect::connect() {
    logger->info(String::format("Connecting to brewster communicator server(IP: %d.%d.%d.%d, Port: %d).", server[0], server[1], server[2], server[3], port));
    
    if (client.connect(server, port) && client.connected())
    {
        logger->trace("Connected to Brewster server");
        sendWelcomeMessage();
        reconnectRetries = 0;
        nextConnectionAttemptIn = reconnectTime;

        return true;
    }else{
        if (reconnectRetries>0 && nextConnectionAttemptIn < 3000) {
            nextConnectionAttemptIn *= 2;
        }
        reconnectRetries++;
        
        logger->warn(String::format("Connection to brewster server failed. Retried %d time(s), next retry in %d ms.", reconnectRetries, nextConnectionAttemptIn));

        return false;
    }
}

void CloudConnect::process() {
    charsAvailable = client.available();
    if (charsAvailable)
    {
        logger->trace(String::format("Chars in buffer: %d", charsAvailable));
        lastCommunication = millis();
        //String input = client.readString();
        //logger->info(String::format("Message received (%d chars): "+input, input.length()));
        //distributeEvent(input);
        
        /*
        logger->info("Start");
        char buffer [10];
        int bread = client.readBytes(buffer, 10);
        buffer[bread] = 0;
        logger->info("Done");
        logger->info(String::format("Message received (%d chars): %s", bread, buffer));
        */
        
        String input = "";
        while(client.available()) {
            char c = (char)client.read();
            if (c == 0)
                logger->info("NULL character detected");

            input += c;
        }
        logger->info(String::format("Message received (%d chars): "+input, input.length()));

        const int capacity = JSON_OBJECT_SIZE(25);
        StaticJsonBuffer<capacity> jsonBuffer;
        JsonObject& root = jsonBuffer.parseObject(input.c_str());
        if (root.success()) {
            if (root["type"] == "event")
                distributeEvent(root);
        }else{
            logger->error("Parsing of JSON input message failed ("+input+")");
        }
    }
    
    if (!client.connected())
    {
        if(lastConnectionAttempt + nextConnectionAttemptIn < millis()) {
            logger->warn("Client disconnected from server. Trying to reconnect.");
            connect();
            lastConnectionAttempt = millis();
        }
    }
}

void CloudConnect::emitEvent(String event) {
    if (client != NULL && client.connected()) {
        client.println(event);
    }else{
        logger->error("Cannot emit event to the server. Connection to the server is not established. Event: "+event);
    }
}

void CloudConnect::emitEvent(JsonObject&  event) {
    if (client != NULL && client.connected()) {
        serializeToClient(event);
    }else{
        logger->error("Cannot emit event to the server. Connection to the server is not established.");
        event.prettyPrintTo(Serial);
    }
}

void CloudConnect::registerListener(eventHandlerFunc eventHandler) {
    this->eventHandler = eventHandler;
}
        
void CloudConnect::distributeEvent(JsonObject& event) {
    if (eventHandler != NULL) {
        eventHandler(event);
    }
}

void CloudConnect::sendHearthBeat() {
    const int capacity = JSON_OBJECT_SIZE(4);
    StaticJsonBuffer<capacity> jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    root["type"] = "hearthbeat";
    long ttime = Time.now()*1000;
    root["timestamp"] = ttime;
    std::string str (Time.format(ttime, TIME_FORMAT_ISO8601_FULL).c_str());
    root["timestamp_human"] = str;
    
    if (client != NULL && client.connected()) {
        serializeToClient(root);
    }else{
        logger->error("Cannot send hearthbeat to the server. Connection to the server is not established.");
    }
}

void CloudConnect::sendWelcomeMessage() {
    //const int capacity = JSON_ARRAY_SIZE(2) + 2*JSON_OBJECT_SIZE(2);
    const int capacity = JSON_OBJECT_SIZE(5);
    StaticJsonBuffer<capacity> jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    root["type"] = "connection_established";
    long ttime = Time.now()*1000;
    root["timestamp"] = ttime;
    std::string str (Time.format(ttime, TIME_FORMAT_ISO8601_FULL).c_str());
    root["timestamp_human"] = str;
    root["client_version"] = CCLIB_VERSION;
    
    if (client != NULL && client.connected()) {
        serializeToClient(root);
    }else{
        logger->error("Cannot send welcome message to the server. Connection to the server is not established.");
    }
}

void CloudConnect::serializeToClient(JsonObject& object) {
    object.printTo(client);
    client.write((uint8_t)0);
}