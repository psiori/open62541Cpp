cmake_minimum_required(VERSION 3.11)
project(Open62541cppExamples)
# Add the tests
add_subdirectory(TestServer)
add_subdirectory(TestClient)
add_subdirectory(TestClientCustomConfig)
add_subdirectory(TestClientDefaultEncryption)
add_subdirectory(VariableTest)
add_subdirectory(SubscriptionTest)
add_subdirectory(ServerDiscoverable)
add_subdirectory(DiscoveryServer)
#add_subdirectory(ConditionTestServer)
add_subdirectory(HistorianClient)
add_subdirectory(HistorianServer)
add_subdirectory(TestEventClient)
add_subdirectory(TestEventServer)


