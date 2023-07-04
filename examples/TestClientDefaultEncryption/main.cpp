#include <iostream>

#include <open62541cpp/open62541client.h>
#include <open62541cpp/clientsubscription.h>
#include <open62541cpp/monitoreditem.h>
#include <regex>

using namespace std;

std::string UAString2String(const UA_String s)
{
    std::string result((char*)s.data, s.length);
    return result;
}

std::string dumpClientConfigToString(const UA_ClientConfig* config)
{
    std::string result;

    result += "UA_ClientConfig {\n";
    result += "\t timeout = " + to_string(config->timeout) + "ms\n";
    result += "\t secureChannelLifeTime = " + to_string(config->secureChannelLifeTime) + "ms\n";
    result += "\t requestedSessionTimeout = " + to_string(config->requestedSessionTimeout) + "ms\n";
    result += "\t securityPoliciesSize = " + to_string(config->securityPoliciesSize) + "\n";
    if (config->securityPoliciesSize > 0)
        for (size_t i = 0; i < config->securityPoliciesSize; i++)
            result += "\t securityPolicies->policyUri[" + to_string(i) +
                      "] = " + UAString2String(config->securityPolicies[i].policyUri) + "\n";
    // result += "\tsecurityPolicies->localCertificate = " + UAString2String(config->securityPolicies->localCertificate)
    // + "\n";

    result +=
        "\t clientDescription.applicationUri = " + UAString2String(config->clientDescription.applicationUri) + "\n";
    result += "\t applicationUri = " + UAString2String(config->applicationUri) + "\n";
    std::string callback = (config->stateCallback != nullptr) ? "SET" : "ERROR";
    result += "\t stateCallback = " + callback + "\n";
    result += "\t securityMode = " + to_string(config->securityMode) + "\n";
    result += "\t securityPolicyUri = " + UAString2String(config->securityPolicyUri) + "\n";
    result += "\t outStandingPublishRequests = " + to_string(config->outStandingPublishRequests) + "\n";
    result += "\t sessionLocaleIdsSize = " + to_string(config->sessionLocaleIdsSize) + "\n";
    result += "}\n";

    return result;
}

/* loadFile parses the certificate file.
 *
 * @param  path               specifies the file name given in argv[]
 * @return Returns the file content after parsing */
static UA_INLINE UA_ByteString loadFile(std::string filepath)
{
    UA_ByteString fileContents = UA_STRING_NULL;

    /* Open the file */
    FILE* fp = fopen(filepath.c_str(), "rb");
    if (!fp) {
        errno = 0; /* We read errno also from the tcp layer... */
        return fileContents;
    }

    /* Get the file length, allocate the data and read */
    fseek(fp, 0, SEEK_END);
    fileContents.length = (size_t)ftell(fp);
    fileContents.data   = (UA_Byte*)UA_malloc(fileContents.length * sizeof(UA_Byte));
    if (fileContents.data) {
        fseek(fp, 0, SEEK_SET);
        size_t read = fread(fileContents.data, sizeof(UA_Byte), fileContents.length, fp);
        if (read != fileContents.length)
            UA_ByteString_clear(&fileContents);
    }
    else {
        fileContents.length = 0;
    }
    fclose(fp);

    return fileContents;
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

    // cout << "\n# Initial client config: " << dumpClientConfigToString(client.getConfig()) << endl;

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
    cout << "Redirecting output stream...done" << endl;

    // auto serverAddress = "opc.tcp://localhost:4840";  // TestServer
    auto serverAddress = "opc.tcp://192.168.177.192:49320";  // Kepserver
    cout << "\t Connecting to server= " << serverAddress << endl;

    std::string username = "*****";
    std::string password = "*****";
    cout << "\t username= " << username << endl;
    cout << "\t password= " << password << endl;

    std::string certfile, keyfile, server_cert;
    certfile = "/Users/sergio/Downloads/client_certificate.der";
    keyfile  = "/Users/sergio/Downloads/client_private_key.pem";
    printf("Reading certfile=%s \n", certfile.c_str());
    printf("Reading keyfile=%s \n", keyfile.c_str());
    UA_ByteString certificate = loadFile(certfile);
    UA_ByteString privateKey  = loadFile(keyfile);

    printf("\n\t UA_ClientConfig_setDefaultEncryption..... \n");
    UA_ClientConfig_setDefaultEncryption(clientConfig, certificate, privateKey, NULL, 0, NULL, 0);
    UA_CertificateVerification_AcceptAll(&clientConfig->certificateVerification);
    UA_ByteString_clear(&certificate);
    UA_ByteString_clear(&privateKey);

    clientConfig->securityMode      = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;
    clientConfig->securityPolicyUri = UA_String_fromChars("http://opcfoundation.org/UA/SecurityPolicy#Basic256Sha256");

    /* The application URI must be the same as the one in the certificate.
     * The script for creating a self-created certificate generates a certificate
     * with the Uri specified below.*/
    UA_ApplicationDescription_clear(&clientConfig->clientDescription);
    clientConfig->clientDescription.applicationUri  = UA_String_fromChars("urn:freeopcua:client");
    clientConfig->clientDescription.applicationType = UA_APPLICATIONTYPE_CLIENT;
    // clientConfig->outStandingPublishRequests = 0;

    cout << "setting CustomConfig..." << endl << endl;
    client.setCustomConfig(clientConfig, false);

    cout << "\n# Using client config: " << dumpClientConfigToString(client.getConfig()) << endl;

    cout << "Connecting...." << endl;
    if (client.connectUsername(serverAddress, username, password, false)) {
        cout << "\n# Connected client config: " << dumpClientConfigToString(client.getConfig()) << endl;
        // cout << "Connected" << endl;
        UA_UInt32 subId = 0;
        if (client.addSubscription(subId)) {

            /* Read the server-time */
            cout << "Check server time:" << endl;
            UA_Variant value;
            UA_Variant_init(&value);
            UA_Client_readValueAttribute(client.client(),
                                         UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME),
                                         &value);
            if (UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_DATETIME])) {
                UA_DateTimeStruct dts = UA_DateTime_toStruct(*(UA_DateTime*)value.data);
                UA_LOG_INFO(UA_Log_Stdout,
                            UA_LOGCATEGORY_USERLAND,
                            "The server date is: %02u-%02u-%04u %02u:%02u:%02u.%03u",
                            dts.day,
                            dts.month,
                            dts.year,
                            dts.hour,
                            dts.min,
                            dts.sec,
                            dts.milliSec);
            }
            UA_Variant_clear(&value);

            cout << "Subscription Created id = " << subId << endl;

            auto f = [](Open62541::ClientSubscription& c, Open62541::MonitoredItem* m, UA_DataValue* v) {
                cout << "Data Change SubId " << c.id() << " Monitor Item Id " << m->id() << " Value "
                     << v->value.type->typeName << " " << Open62541::dataValueToString(v) << endl;
            };

            // Extract node names from node_path string
            // std::string node_path = "0:Objects, 2:OPCUA_new, 2:AutonomousWoodyard, 0:Objects, 2:CraneSouth,
            // 2:Weather, 2:Temperature";
            // std::string node_path = "0:Objects, 2:OPCUA_new, 2:AutonomousWoodyard, 0:Objects, 2:CraneSouth,
            // 2:WorkingCycles_Available";
            std::string node_path =
                "0:Objects, 2:OPCUA_new, 2:AutonomousWoodyard, 0:Objects, 2:CraneSouth, 2:WorkingCycles";
            std::vector<std::string> varpath;
            const std::regex re("\\d+:(\\w+)");
            for (std::sregex_iterator it(node_path.begin(), node_path.end(), re); it != std::sregex_iterator(); ++it) {
                varpath.push_back((*it)[1]);
                std::cout << " ## " << (*it)[1] << std::endl;
            }

            Open62541::NodeId found_node;
            bool found_node_id = false;
            static const Open62541::NodeId node_root(0, UA_NS0ID_ROOTFOLDER);
            found_node_id = client.nodeIdFromPath(node_root, varpath, found_node);
            cout << "node found?=" << found_node_id << ", found_node=" << Open62541::toString(found_node) << endl;

            cout << "Adding a data change monitor item" << endl;
            Open62541::ClientSubscription& cs = *client.subscription(subId);
            cout << "client_subscription=" << &cs << endl;
            unsigned mdc = cs.addMonitorNodeId(f, found_node);  // returns monitor id
            if (!mdc) {
                cout << "Failed to add monitor data change " << endl;
            }
            cout << "Added MonitorNodeId=" << mdc << endl;

            cout << "iterating...1min" << endl;

            // run for one minute
            //
            for (int j = 0; j < 60; j++) {
                cout << "iterating..." << j << endl;
                Open62541::Variant VALUE("string_test_value" + std::to_string(j));

                auto result = client.setValueAttribute(found_node, VALUE);
                cout << "writing result=" << result << endl;

                client.runIterate(1000);
            }
            cout << "iterating...ended" << endl;
            // client.run();  // this will loop until interrupted
        }
        else {
            cout << "Subscription Failed" << endl;
        }
    }
    else {
        cout << "Subscription Failed" << endl;
    }
    return 0;
}
