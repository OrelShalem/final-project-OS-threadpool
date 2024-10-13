// This file implements a simple client for connecting to a server and exchanging messages.

#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

int main()
{
    // Create a socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        std::cerr << "Socket creation error" << std::endl;
        return -1;
    }

    // Set up the server address structure
    sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(9039); // Convert port to network byte order

    // Convert IP address from string to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0)
    {
        std::cerr << "Invalid address/ Address not supported" << std::endl;
        return -1;
    }

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        std::cerr << "Connection Failed" << std::endl;
        return -1;
    }

    std::cout << "Connected to server" << std::endl;

    // Main communication loop
    while (true)
    {
        char buffer[1024] = {0};
        // Read data from the server
        int valread = read(sock, buffer, 1024);
        if (valread <= 0)
        {
            std::cout << "Server disconnected" << std::endl;
            break;
        }
        std::cout << buffer << std::endl;

        // Get user input
        std::string input;
        std::getline(std::cin, input);
        // Send user input to the server
        send(sock, input.c_str(), input.length(), 0);

        // Check if user wants to exit
        if (input == "exit" || input == "9")
        {
            std::cout << "Exiting..." << std::endl;
            break;
        }
    }

    // Close the socket
    close(sock);
    return 0;
}
