#include <iostream>
//
#include <open62541cpp/open62541client.h>
#include <open62541cpp/clientsubscription.h>
#include <open62541cpp/monitoreditem.h>
//
using namespace std;

std::string UAString2String(const UA_String s)
{
    // cout << "str_lenght: " << s.length << endl;
    std::string result((char*)s.data, s.length);
    // cout << "str: " << result << endl;
    return result;
}

std::string dumpClientConfigToString(const UA_ClientConfig* config)
{
    std::string result;

    result += "UA_ClientConfig {\n";
    result += "\ttimeout = " + to_string(config->timeout) + "ms\n";
    result += "\tsecureChannelLifeTime = " + to_string(config->secureChannelLifeTime) + "ms\n";
    result += "\trequestedSessionTimeout = " + to_string(config->requestedSessionTimeout) + "ms\n";
    result += "\tsecurityPoliciesSize = " + to_string(config->securityPoliciesSize) + "\n";
    if (config->securityPoliciesSize > 0)
        result += "\tsecurityPolicies->policyUri = " + UAString2String(config->securityPolicies[0].policyUri) + "\n";
    // result += "\tsecurityPolicies->localCertificate = " + UAString2String(config->securityPolicies->localCertificate)
    // + "\n";
    result += "\tapplicationUri = " + UAString2String(config->applicationUri) + "\n";
    result += "\tsecurityPolicyUri = " + UAString2String(config->securityPolicyUri) + "\n";
    result += "\toutStandingPublishRequests = " + to_string(config->outStandingPublishRequests) + "\n";
    result += "\tsessionLocaleIdsSize = " + to_string(config->sessionLocaleIdsSize) + "\n";
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
    cout << "Client Subscription Test - (TestServer must be running)" << endl;
    Open62541::Client client;

    cout << "Initialising client with no configuration" << endl;
    client.initialise(false);

    cout << "Getting client config..." << endl;
    UA_ClientConfig* clientConfig = client.getConfig();

    cout << "\n# Initial client config: " << dumpClientConfigToString(client.getConfig()) << endl;

    cout << "Redirecting output stream..." << endl;
    // Create a function pointer to the myLogger function
    Open62541::Client::LoggerFuncPtr customLoggerFunction = &customLogger;
    // Create a UA_Logger structure with the custom log function
    UA_Logger customLogger = (UA_Logger){
        .log     = customLoggerFunction,         //
        .clear   = clientConfig->logger.clear,   //
        .context = clientConfig->logger.context  //
    };
    // Set the clientConfig logger to use the customLogger
    clientConfig->logger = customLogger;
    cout << "Redirecting output stream...done" << endl;

    // Create a UA_String for the applicationUri
    // auto applicationUri = UA_String_fromChars("urn:freeopcua:client");
    // cout << "Custom applicationUri: " << UAString2String(applicationUri) << endl;
    // clientConfig->applicationUri = applicationUri;

    cout << "\n# Custom client config: " << dumpClientConfigToString(clientConfig) << endl;

    cout << "setCustomConfig..." << endl << endl;
    client.setCustomConfig(clientConfig);

    // Alternatively, just override the logger function
    // client.setCustomLogger(myLoggerFuncPtr);

    auto serverAddress = "opc.tcp://localhost:4840";  // TestServer
    cout << "Connecting...." << endl;
    if (client.connect(serverAddress, false)) {
        int idx = client.namespaceGetIndex("urn:test:test");
        cout << "\n# Connected client config: " << dumpClientConfigToString(client.getConfig()) << endl;
        if (idx > 1) {
            cout << "Connected" << endl;
            UA_UInt32 subId = 0;
            if (client.addSubscription(subId)) {
                cout << "Subscription Created id = " << subId << endl;
                cout << "iterating..." << endl;
                client.runIterate();
                exit(-1);
                // client.run();  // this will loop until interrupted
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
