#include "client.h"

string trackerIp;
int trackerPort;
string currentPortStr;

map<string, string> uploads;
map<string, string> downloads;
map<string, map<int, vector<string>>> currentDownloads;

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        std::cerr << "Usage: " << argv[0] << " <address:port> <tracker_info_file>" << std::endl;
        return 1;
    }

    string fileName = argv[2];
    string portStr = getAddressFromFile(fileName.c_str(), 1);
    currentPortStr = argv[1];

    int sock = 0;
    struct sockaddr_in serv_addr;

    parseAddressAndPort(portStr.c_str(), serv_addr, portStr, trackerIp, trackerPort);

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Socket creation error \n");
        return -1;
    }

    pthread_t serverThread;
    if (pthread_create(&serverThread, NULL, acceptIncomingConnections, NULL) == -1)
    {
        perror("pthread");
        exit(EXIT_FAILURE);
    }

    if (connectWithTracker(serv_addr, sock) < 0)
    {
        cout << "No tracker found! Terminating...";
        exit(EXIT_FAILURE);
    }

    cout << "Connected to tracker.\n";
    cout << "____________________________\n";

    handleCommandsToTracker(sock);

    return 0;
}