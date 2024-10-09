#include "client/Client.hpp"
#include <iostream>
#include <string>

int main()
{
    // Create a Client object to connect to the server at 127.0.0.1:9036
    Client client("127.0.0.1", 9036);
    std::string command;

    // Inform the user that we're connecting to the server
    std::cout << "Connecting to server..." << std::endl;
    // Connect to the server and display the initial menu
    client.connectToServer(); // This will display the initial menu

    // Prompt the user to enter commands
    std::cout << "Enter commands based on the menu above (type 'exit' to quit):" << std::endl;

    // Main loop for user interaction
    while (true)
    {
        // Display prompt for user input
        std::cout << "> ";
        // Read a line of input from the user
        std::getline(std::cin, command);

        // Check if the user wants to exit
        if (command == "exit")
        {
            // Send the exit command to the server
            client.sendRequest(command + "\n");
            // Break out of the loop to end the program
            break;
        }

        // Send the user's command to the server
        client.sendRequest(command + "\n");
    }

    // Exit the program
    return 0;
}