#include "client/Client.hpp"
#include <iostream>
#include <string>

int main()
{
    Client client("127.0.0.1", 9036);
    std::string command;

    std::cout << "Connecting to server..." << std::endl;
    client.connectToServer(); // זה יציג את התפריט הראשוני

    std::cout << "Enter commands based on the menu above (type 'exit' to quit):" << std::endl;

    while (true)
    {
        std::cout << "> ";
        std::getline(std::cin, command);

        if (command == "exit")
        {
            client.sendRequest(command + "\n");
            break;
        }

        client.sendRequest(command + "\n");
    }

    return 0;
}