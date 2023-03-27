#include <iostream>
//
#include <open62541cpp/open62541client.h>
#include <open62541cpp/clientsubscription.h>
#include <open62541cpp/monitoreditem.h>
//
using namespace std;
//
class SubTestClient : public Open62541::Client
{
public:
    void asyncService(void* /*userdata*/, UA_UInt32 requestId, void* /*response*/, const UA_DataType* responseType)
    {
        cout << "asyncService requerstId = " << requestId << " Type " << responseType->typeName << endl;
    }
    void asyncConnectService(UA_UInt32 requestId, void* /*userData*/, void* /*response*/)
    {
        cout << "asyncConnectService requestId = " << requestId << endl;
    }
};

int main()
{
    cout << "Client Subscription Test - TestServer must be running" << endl;
    //
    // Test subscription create
    // Monitored Items
    // Events
    //
    Open62541::Client client;
    if (client.connect("opc.tcp://localhost:4840")) {
        int idx = client.namespaceGetIndex("urn:test:test");
        if (idx > 1) {
            cout << "Connected" << endl;
            UA_UInt32 subId = 0;

            if (client.addSubscription(subId)) {
                cout << "Subscription Created id = " << subId << endl;
                auto f = [&client,
                          idx](Open62541::ClientSubscription& c, Open62541::MonitoredItem* m, UA_DataValue* v) {
                    cout << "Data Change SubId " << c.id() << " Monitor Item Id " << m->id() << " Value "
                         << v->value.type->typeName << " " << Open62541::dataValueToString(v) << endl;

                    cout << endl;

                    Open62541::NodeId nodeNumber(idx, "BOOLEAN_Value");
                    Open62541::Variant nodeValue;
                    if (client.readValueAttribute(nodeNumber, nodeValue)) {
                        if (nodeValue) {
                            cout << " -- readValueAttribute: <" << idx << "> , " << nodeValue.toString() << endl;
                        }
                        else {
                            cout << " Could not readValueAttribute for NodeId: "
                                 << Open62541::toString(nodeNumber);
                        }
                    }
                    else {
                        cout << " Could not readValueAttribute";
                    }

                    /////
                    static bool BOOLvalue = true;
                    BOOLvalue             = !BOOLvalue;
                    Open62541::Variant data(BOOLvalue);

                    cout << "  attempting to setValueAttribute < " << data.toString() << " > for ["
                         << BOOLvalue << "] NodeId: " << Open62541::toString(nodeNumber) << endl;
                    // Now write the real data
                    auto writeResult = client.setValueAttribute(nodeNumber, data);
                    if (writeResult) {
                        cout << " successfully wrote data: < " << Open62541::variantToString(data.get()) << " > for ["
                             << BOOLvalue << "] on NodeId: " << Open62541::toString(nodeNumber) << endl;

                        // Retrieve again to check
                        if (client.readValueAttribute(nodeNumber, nodeValue)) {
                            if (nodeValue) {
                                cout << " -- readValueAttribute: <" << idx << "> , " << nodeValue.toString() << endl;
                            }
                            else {
                                cout << " Could not readValueAttribute "
                                        "for NodeId: "
                                     << Open62541::toString(nodeNumber);
                            }
                        }
                        else {
                            cout << " Could not readValueAttribute";
                        }
                    }
                    else {
                        cout << "  could not setValueAttribute for "
                                "NodeID: "
                             << Open62541::toString(nodeNumber) << " , data: " << data << ": " << data.toString();
                        return false;
                    }
                };

                //
                cout << "Adding a data change monitor item" << endl;
                //
                Open62541::NodeId nodeNumber(idx, "Number_Value");
                Open62541::ClientSubscription& cs = *client.subscription(subId);
                unsigned mdc                      = cs.addMonitorNodeId(f, nodeNumber);  // returns monitor id
                if (!mdc) {
                    cout << "Failed to add monitor data change" << endl;
                }
                //
                //
                // run for one minute
                //
                for (int j = 0; j < 60; j++) {
                    std::cout << "[" << j << "] client.runIterate( );" << std::endl;
                    client.runIterate(1000);
                }
                cout << "Ended Run - Test if deletes work correctly" << endl;
                client.subscriptions().clear();
                cout << "Subscriptions cleared - run for another 5 seconds" << endl;
                client.run();
                cout << "Finished" << endl;
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
