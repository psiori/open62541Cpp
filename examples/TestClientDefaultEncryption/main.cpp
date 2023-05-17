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
    result += "\t timeout = " + to_string(config->timeout) + "ms\n";
    result += "\t secureChannelLifeTime = " + to_string(config->secureChannelLifeTime) + "ms\n";
    result += "\t requestedSessionTimeout = " + to_string(config->requestedSessionTimeout) + "ms\n";
    result += "\t securityPoliciesSize = " + to_string(config->securityPoliciesSize) + "\n";
    if (config->securityPoliciesSize > 0)
        for (size_t i = 0; i < config->securityPoliciesSize; i++)
            result += "\t securityPolicies->policyUri[" + to_string(i) + "] = "
                   + UAString2String(config->securityPolicies[i].policyUri) + "\n";
    // result += "\tsecurityPolicies->localCertificate = " + UAString2String(config->securityPolicies->localCertificate)
    // + "\n";



    result +=
        "\t clientDescription.applicationUri = " + UAString2String(config->clientDescription.applicationUri) + "\n";
    result += "\t applicationUri = " + UAString2String(config->applicationUri) + "\n";
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
    // clientConfig->logger = customLogger;
    cout << "Redirecting output stream...done" << endl;


    // cout << "setCustomConfig..." << endl << endl;
    // client.setCustomConfig(clientConfig);

    auto serverAddress = "opc.tcp://localhost:4840";  // TestServer

    std::string username = "*****"; // FIXME : from parameter
    std::string password = "*****"; // FIXME : from parameter
    cout << "\t username= " << username << endl;
    cout << "\t password= " << password << endl;

    std::string certfile, keyfile;
    certfile = "~/client_certificate.der"; // FIXME : from parameter
    keyfile  = "~/client_private_key.pem"; // FIXME : from parameter
    printf("Reading certfile=%s \n", certfile.c_str());
    printf("Reading keyfile=%s \n", keyfile.c_str());
    UA_ByteString certificate = loadFile(certfile);
    UA_ByteString privateKey  = loadFile(keyfile);

    printf("\n\t UA_ClientConfig_setDefaultEncryption..... \n");
    UA_ClientConfig_setDefaultEncryption(clientConfig, certificate, privateKey, NULL, 0, NULL, 0);
    UA_CertificateVerification_AcceptAll(&clientConfig->certificateVerification);
    UA_ByteString_clear(&certificate);
    UA_ByteString_clear(&privateKey);

    UA_String securityPolicyUri         = UA_STRING_NULL;
    UA_MessageSecurityMode securityMode = UA_MESSAGESECURITYMODE_INVALID; /* allow everything */
    // clientConfig->securityMode      = securityMode; // FIXME : from parameter
    clientConfig->securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;
    // clientConfig->securityPolicyUri = securityPolicyUri; // FIXME : from parameter
    clientConfig->securityPolicyUri = UA_String_fromChars("http://opcfoundation.org/UA/SecurityPolicy#Basic256Sha256");

    /* The application URI must be the same as the one in the certificate.
     * The script for creating a self-created certificate generates a certificate
     * with the Uri specified below.*/
    UA_ApplicationDescription_clear(&clientConfig->clientDescription);
    clientConfig->clientDescription.applicationUri  = UA_STRING_ALLOC("urn:freeopcua:client");
    clientConfig->clientDescription.applicationType = UA_APPLICATIONTYPE_CLIENT;

    cout << "\n# Using client config: " << dumpClientConfigToString(client.getConfig()) << endl;

    cout << "Connecting...." << endl;
    // if (client.connect(serverAddress, false)) {
    if (client.connectUsername(serverAddress, username, password, false)) {
        // int idx = client.namespaceGetIndex("urn:test:test");
        cout << "\n# Connected client config: " << dumpClientConfigToString(client.getConfig()) << endl;
        // if (idx > 1) {
        // cout << "Connected" << endl;
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
        // }
        // else {
        //     cout << "TestServer not running idx = " << idx << endl;
        // }
    }
    else {
        cout << "Subscription Failed" << endl;
    }
    return 0;
}
