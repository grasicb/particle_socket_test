#ifndef _CLOUD_CONNECT_H
#define _CLOUD_CONNECT_H

#include <ArduinoJson.h>
#include "application.h"

#define CCLIB_VERSION "0.1"

class CloudConnect {
    public:
        typedef void (* eventHandlerFunc)(JsonObject& event);
        
        CloudConnect(byte* server, int port);
        boolean connect();
        void process();
        void emitEvent(String event);
        void emitEvent(JsonObject& event);
        void registerListener(eventHandlerFunc eventHandler);
        
    private:
        void distributeEvent(JsonObject& event);
        void sendHearthBeat();
        void sendWelcomeMessage();
        void serializeToClient(JsonObject& event);
    
        Logger *logger;
        byte* server;
        int port;
        TCPClient client;
        long lastCommunication = 0;
        const int reconnectTime = 500;
        int reconnectRetries = 0;
        long lastConnectionAttempt = 0;
        int nextConnectionAttemptIn = reconnectTime;
        int charsAvailable = 0;
        eventHandlerFunc eventHandler = NULL;
};

#endif