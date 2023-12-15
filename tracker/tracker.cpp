#include "tracker.h"

um<string, string> userCredentials;
um<string, bool> currentlyLoggedIn;
um<string, bool> groups;
um<string, set<string>> groupMembers;
um<string, set<string>> pendingRequests;
um<string, string> admins;
um<string, um<string, set<string>>> seeders;
um<string, vector<string>> hashes;
um<string, long long> fileSize;
um<string, string> userToIp;

void handle_input_from_cli()
{
    string command;
    while (true)
    {
        std::getline(std::cin, command);
        if (command == "quit")
        {
            exit(0);
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        cout << "Usage : ./tracker <tracker_file_path> <tracker_number>";
        return -1;
    }

    string fileName = argv[1];
    string portStr = getAddressFromFile(fileName.c_str(), atoi(argv[2]));

    string address_;
    int port_;

    int serverSocket_;
    struct sockaddr_in serverAddress_;

    parseAddressAndPort(portStr.c_str(), serverAddress_, portStr, address_, port_);

    cout << "----------------------------------------------------\n";
    cout << "Attempting to start tracker on port : " << portStr << "\n";

    serverSocket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket_ == -1)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr(address_.c_str());
    serverAddress.sin_port = htons(port_);

    if (bind(serverSocket_, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1)
    {
        cerr << "Error binding socket" << endl;
        close(serverSocket_);
        exit(EXIT_FAILURE);
    }

    if (listen(serverSocket_, MAX_WAITING_CONNECTIONS) == -1)
    {
        cerr << "Error listening on socket" << endl;
        close(serverSocket_);
        exit(EXIT_FAILURE);
    }

    cout << "Tracker listening on " << address_ << ":" << port_ << "..." << endl;
    cout << "----------------------------------------------------\n";

    vector<thread> threads;
    std::thread cliThread(handle_input_from_cli);

    while (true)
    {
        struct sockaddr_in clientAddress;
        socklen_t clientAddressSize = sizeof(clientAddress);
        int clientSocket = accept(serverSocket_, (struct sockaddr *)&clientAddress, &clientAddressSize);
        if (clientSocket == -1)
        {
            cerr << "Error accepting connection" << endl;
            continue;
        }
        threads.push_back(thread(handle_connection, clientSocket));
    }

    for (auto i = threads.begin(); i != threads.end(); i++)
    {
        if (i->joinable())
            i->join();
    }

    if (cliThread.joinable())
    {
        cliThread.join();
    }

    return 0;
}