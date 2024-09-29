#pragma once
#include <string>

class Client
{
public:
    Client(const std::string &address, int port);
    ~Client();
    void sendRequest(const std::string &request);
    void sendMetricsRequests();
    void connectToServer();

private:
    std::string address;
    int port;
    int sock;
    std::string receiveResponse();
};