#include <iostream>
//
#include <open62541cpp/open62541client.h>
#include <open62541cpp/clientsubscription.h>
#include <open62541cpp/monitoreditem.h>
//
using namespace std;

std::string dumpClientConfigToString(const UA_ClientConfig* config)
{
    std::string result;

    result += "UA_ClientConfig {\n";
    result += "\ttimeout = " + std::to_string(config->timeout) + "ms\n";
    result += "\tsecureChannelLifeTime = " + std::to_string(config->secureChannelLifeTime) + "ms\n";
    result += "\trequestedSessionTimeout = " + std::to_string(config->requestedSessionTimeout) + "ms\n";
    result += "\tapplicationUri = " +
              std::string(reinterpret_cast<const char*>(config->applicationUri.data), config->applicationUri.length) +
              "\n";
    result +=
        "\tsecurityPolicyUri = " +
        std::string(reinterpret_cast<const char*>(config->securityPolicyUri.data), config->securityPolicyUri.length) +
        "\n";
    result += "}\n";

    return result;
}

/* Define a custom logger */
void customLogger(void* context, UA_LogLevel level, UA_LogCategory category, const char* msg, va_list args)
{
    // FIXME : Also make use of UA_LogLevel
    std::string buffer;
    int n = vsnprintf(NULL, 0, msg, args);
    if (n > 0) {
        buffer.resize(n);
        vsnprintf(buffer.data(), n + 1, msg, args);
    }
    cout << " (!)_LOG: " << buffer << endl;
}

int main()
{
    cout << "Client Subscription Test - TestServer must be running" << endl;
    Open62541::Client client;

    cout << "Initialising client with no configuration" << endl;
    client.initialiseNoConfig();

    cout << "Getting client config..." << endl;
    UA_ClientConfig* clientConfig = client.getConfig();

    std::string dump = dumpClientConfigToString(clientConfig);
    cout << "Current client config: " << dump << endl;

    cout << "Redirecting output stream..." << endl;
    // Create a function pointer to the myLogger function
    Open62541::Client::LoggerFuncPtr customLoggerFunction = &customLogger;
    // Create a UA_Logger structure with the custom log function
    // https://github.com/open62541/open62541/blob/2851b990a3b
    //     /include/open62541/plugin/log.h#L55-L66
    UA_Logger customLogger = (UA_Logger){
        .log     = customLoggerFunction,  //
        .context = NULL,                  //
        .clear   = NULL                   //
    };
    // Set the clientConfig logger to use the customLogger
    clientConfig->logger = customLogger;
    cout << "Redirecting output stream...done" << endl;

    cout << "setCustomConfig..." << endl;
    client.setCustomConfig(clientConfig);

    // Alternatively, just override the logger function
    // client.setCustomLogger(myLoggerFuncPtr);

    auto serverAddress = "opc.tcp://localhost:4840";
    if (client.connectNoInit(serverAddress)) {
        int idx = client.namespaceGetIndex("urn:test:test");
        if (idx > 1) {
            cout << "Connected" << endl;
            UA_UInt32 subId = 0;
            if (client.addSubscription(subId)) {
                cout << "Subscription Created id = " << subId << endl;
                exit(-1);
            }
            else {
                cout << "Subscription Failed" << endl;
            }
        }
        else {
            cout << "TestServer not running idx = " << idx << endl;
        }
    }
    else {
        cout << "Subscription Failed" << endl;
    }
    return 0;
}
