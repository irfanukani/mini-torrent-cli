#include "client.h"

std::mutex download_ds_mutex;
std::mutex file_Write;

struct downloadFileInfo
{
    vector<string> command;
    int socket;
    string buffer;
};

struct chunkDownload
{
    string peerIP;
    int chunkId;
    string fileName;
    string destination_path;
};

struct chunkRequest
{
    string serverIp;
    string fileName;
};

int writeToFileWithSeek(const char *filename, off_t seekPosition, const char *data, size_t dataSize)
{
    std::lock_guard<std::mutex> lock(file_Write);
    int fd = open(filename, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);

    if (fd == -1)
    {
        perror("open");
        return -1;
    }

    if (lseek(fd, seekPosition, SEEK_SET) == -1)
    {
        perror("lseek");
        close(fd);
        return -1;
    }

    ssize_t bytesWritten = write(fd, data, dataSize);

    if (bytesWritten == -1)
    {
        perror("write");
        close(fd);
        return -1;
    }

    close(fd);

    return 0;
}

int getRandomNumber(int min, int max)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(min, max);

    return dist(gen);
}

int handleCommandsToTracker(int socket_)
{
    while (true)
    {
        cout << endl;
        cout << "$ ";
        string bufferInput;
        getline(cin, bufferInput);

        string s, in = string(bufferInput);
        stringstream ss(in);
        vector<string> command;

        while (ss >> s)
        {
            command.push_back(s);
        }

        if (!command.size())
        {
            continue;
        }

        if (command[0] == "login")
        {
            bufferInput += " " + currentPortStr;
        }

        if (command[0] == "show_downloads")
        {
            for (auto x : downloads)
            {
                cout << "[" << x.second << "] " << x.first << endl;
            }
            continue;
        }

        if (command[0] == "upload_file")
        {
            upload_file(command, socket_, bufferInput);
            continue;
        }

        if (command[0] == "download_file")
        {
            struct downloadFileInfo *d = new downloadFileInfo();
            d->buffer = bufferInput;
            d->command = command;
            d->socket = socket_;

            std::thread download_thread(download_file, d);
            download_thread.detach();
            continue;
        }

        if (send(socket_, bufferInput.c_str(), bufferInput.length(), MSG_NOSIGNAL) == -1)
        {
            perror(strerror(errno));
            return -1;
        }

        handle_tracker_reply(socket_);
    }
    return 0;
}

int connectWithTracker(struct sockaddr_in &serv_addr, int &sock)
{
    string curTrackIP = trackerIp;
    int curTrackPort = trackerPort;

    bool error = 0;

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(curTrackPort);

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        error = 1;
    }

    if (error)
        return -1;

    return 1;
}

void handle_tracker_reply(int socket_)
{
    char server_reply[10240];
    bzero(server_reply, 10240);
    read(socket_, server_reply, 10240);
    cout << server_reply << endl;
}

int upload_file(vector<string> &command, int socket_, string buffer)
{
    // Structure
    // [FilePath, IP_PORT_STR, FILE_SIZE, CHUNKWISE_HASH]
    if (!isFileValid(command[1]))
    {
        cout << "Invalid File Path!" << endl;
        return -1;
    }
    write(socket_, buffer.c_str(), buffer.length());

    cout << "Sending Metadata..." << endl;

    string fileInfoBuffer = "";
    fileInfoBuffer += command[1];
    fileInfoBuffer += "$_$";
    fileInfoBuffer += currentPortStr;
    fileInfoBuffer += "$_$";
    fileInfoBuffer += to_string(getFileSize(command[1]));
    fileInfoBuffer += "$_$";
    fileInfoBuffer += readChunkByChunkandHash(command[1].c_str());

    send(socket_, fileInfoBuffer.c_str(), fileInfoBuffer.length(), MSG_NOSIGNAL);
    uploads[splitString(command[1].c_str(), "/").back()] = command[1];
    handle_tracker_reply(socket_);
    handle_tracker_reply(socket_);
    return 0;
}

int connectAndSend(string addressOfPeer, string command)
{
    int peer_socket;
    struct sockaddr_in address;

    if ((peer_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    string peerIp;
    int peerPort;

    string tempPortStr = addressOfPeer;

    std::istringstream iss(tempPortStr);
    char delimiter = ':';

    std::string addressStr;
    if (getline(iss, addressStr, delimiter) && getline(iss, tempPortStr, delimiter))
    {
        peerIp = addressStr;
        peerPort = std::stoi(tempPortStr);
    }

    address.sin_family = AF_INET;
    address.sin_port = htons(peerPort);

    if (inet_pton(AF_INET, &peerIp[0], &address.sin_addr) <= 0)
    {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    if (connect(peer_socket, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("Peer Connection Error");
    }

    send(peer_socket, command.c_str(), command.size(), MSG_NOSIGNAL);

    vector<string> cmd = splitString(command.c_str(), " ");
    if (cmd[0] == "get_chunk")
    {
        int n, total = 0;
        string s;
        while (total < CHUNK_SIZE)
        {
            char buffer[CHUNK_SIZE];
            bzero(buffer, CHUNK_SIZE);

            n = read(peer_socket, buffer, CHUNK_SIZE);
            if (n <= 0)
            {
                break;
            }

            s += string(buffer, static_cast<size_t>(n));

            total += n;
        }

        writeToFileWithSeek((cmd[3] + cmd[1]).c_str(), stoi(cmd[2]) * CHUNK_SIZE, s.c_str(), total);
        close(peer_socket);
        return 0;
    }

    char buffer[10240];
    bzero(buffer, 10240);
    int readedBytes = read(peer_socket, buffer, 10240);

    if (readedBytes <= 0)
    {
        return -1;
    }

    // cout << "Bytes Recieved : " << readedBytes << endl;

    vector<string> splitted = splitString(string(buffer, static_cast<size_t>(readedBytes)).c_str(), "$_$");

    if (splitted[0] == "chunk_info")
    {
        // [chunk_info, file_name, ip_addr, available_chunks]
        std::lock_guard<std::mutex> lock(download_ds_mutex);
        for (int i = 3; i < (int)splitted.size(); i++)
        {
            currentDownloads[splitted[1]][stoi(splitted[i])].push_back(splitted[2]);
        }
    }

    close(peer_socket);

    return 0;
}

void collectInformation(struct chunkRequest *c)
{
    // Implement :
    cout << "Fetching chunk info from : " << c->serverIp << " " << endl;
    string command = "get_chunk_info " + c->fileName;
    connectAndSend(c->serverIp, command);
}

void download_chunk(struct chunkDownload *c)
{
    string command = "get_chunk " + c->fileName + " " + to_string(c->chunkId) + " " + c->destination_path;
    connectAndSend(c->peerIP, command);
}

int peiceSelectionAlgorithm(vector<string> command, vector<string> peers)
{
    // command : [download_file, grp_id, file_name, destination_path]
    // peers : [IP1, IP2, . . ., fileSize]
    long long download_file_size = stoi(peers.back());
    peers.pop_back();

    cout << "PeiceWise Algorithm Initiated...\n";
    cout << peers.size() << " peers found!" << endl;

    downloads[command[2]] = "D";

    vector<thread> download_threads;
    for (int i = 0; i < (int)peers.size(); i++)
    {
        struct chunkRequest *c = new chunkRequest;
        c->fileName = command[2];
        c->serverIp = peers[i];
        // Connect to peers and ask for chunks that they have...
        download_threads.push_back(thread(collectInformation, c));
    }
    cout << endl;

    for (auto it = download_threads.begin(); it != download_threads.end(); it++)
    {
        if (it->joinable())
            it->join();
    }
    // Chunks Info Fetched
    download_threads.clear();

    int downloadable_chunks = ceil(download_file_size / (CHUNK_SIZE / 1.0));

    for (int i = 0; i < downloadable_chunks; i++)
    {
        if (currentDownloads[command[2]][i].size() == 0)
        {
            cout << "All chunks Not available for download!" << endl;
            return -1;
        }
        else
        {
            int randomPeer = getRandomNumber(0, currentDownloads[command[2]][i].size() - 1);
            struct chunkDownload *c = new chunkDownload;
            c->chunkId = i;
            c->fileName = command[2];
            c->destination_path = command[3];
            c->peerIP = currentDownloads[command[2]][i][randomPeer];
            download_threads.push_back(thread(download_chunk, c));
        }
    }

    cout << "Download Started..." << endl;
    for (auto it = download_threads.begin(); it != download_threads.end(); it++)
    {
        if (it->joinable())
            it->join();
    }

    // Verify Hashes...
    downloads[command[2]] = "C";
    cout << command[2] << " Downloaded..!" << endl;
    return 0;
}

int download_file(struct downloadFileInfo *d)
{
    vector<string> download_file_hashes;
    write(d->socket, d->buffer.c_str(), d->buffer.size());
    char server_reply[10240];
    bzero(server_reply, 10240);
    cout << "Fetching Peerlist and Hashes.."
         << endl;

    if (read(d->socket, server_reply, 10240) <= 0)
    {
        cout << "Can't Connect to tracker!" << endl;
        return -1;
    }
    vector<string> splitPeersandHash = splitString(string(server_reply).c_str(), "$$$$$");
    vector<string> splitted = splitString(splitPeersandHash[0].c_str(), "$_$");

    if (splitted[0] != "200")
    {
        cout << server_reply << endl;
        return -1;
    }

    splitted.erase(splitted.begin());

    peiceSelectionAlgorithm(d->command, splitted);
    return 0;
}

bool isChunkValid(const std::string &filename, int chunkNumber)
{
    int fd = open(filename.c_str(), O_RDONLY);

    if (fd == -1)
    {
        std::cerr << "Unable to open file." << std::endl;
        return false;
    }

    char buffer[CHUNK_SIZE];
    ssize_t bytesRead = pread(fd, buffer, CHUNK_SIZE, chunkNumber * CHUNK_SIZE);

    close(fd);

    if (bytesRead == -1)
    {
        std::cerr << "Error reading the block." << std::endl;
        return false;
    }

    bool blockIsValid = true;

    return blockIsValid;
}

string getChunk(const std::string &filename, int chunkNumber)
{
    int fd = open(filename.c_str(), O_RDONLY);

    if (fd == -1)
    {
        std::cerr << "Unable to open file." << std::endl;
        return "";
    }

    char buffer[CHUNK_SIZE];
    bzero(buffer, CHUNK_SIZE);
    ssize_t bytesRead = pread(fd, buffer, CHUNK_SIZE, chunkNumber * CHUNK_SIZE);
    close(fd);

    if (bytesRead == -1)
    {
        std::cerr << "Error reading the block." << std::endl;
        return "";
    }

    return std::string(buffer, static_cast<size_t>(bytesRead));
}

void handle_peer_connection(int peer_socket_)
{

    while (true)
    {
        char clientMessage[10240] = {0};

        if (read(peer_socket_, clientMessage, 1024) <= 0)
        {
            close(peer_socket_);
            return;
        }

        vector<string> command = splitString(string(clientMessage).c_str(), " ");

        if (command[0] == "get_chunk_info")
        {
            // check how many chunks are avaliable...
            int chunks = ceil(getFileSize(uploads[command[1]]) / (CHUNK_SIZE / 1.0));
            string response = "chunk_info$_$" + command[1] + "$_$" + currentPortStr + "$_$";
            cout << uploads[command[1]] << " " << chunks << endl;
            for (int i = 0; i < chunks; i++)
            {
                if (isChunkValid(uploads[command[1]], i))
                {
                    response += to_string(i);
                    response += "$_$";
                }
            }
            send(peer_socket_, response.c_str(), response.length(), MSG_NOSIGNAL);
            close(peer_socket_);
            continue;
        }
        if (command[0] == "get_chunk")
        {
            // [get_chunk, fileName, chunkId]
            string response = "";
            response += getChunk(uploads[command[1]], stoi(command[2]));
            // cout << "Sending  : " << response.length() << endl;
            send(peer_socket_, response.c_str(), response.size(), MSG_NOSIGNAL);
            close(peer_socket_);
            continue;
        }
        send(peer_socket_, clientMessage, strlen(clientMessage), MSG_NOSIGNAL);
        close(peer_socket_);
    }

    return;
}

void *acceptIncomingConnections(void *args)
{
    int server_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    int opt = 1;

    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    string peerIp;
    int peerPort;

    string tempPortStr = currentPortStr;

    std::istringstream iss(tempPortStr);
    char delimiter = ':';

    std::string addressStr;
    if (getline(iss, addressStr, delimiter) && getline(iss, tempPortStr, delimiter))
    {
        peerIp = addressStr;

        if (inet_pton(AF_INET, addressStr.c_str(), &address.sin_addr) <= 0)
        {
            std::cerr << "Invalid address: " << addressStr << std::endl;
            exit(1);
        }

        peerPort = std::stoi(tempPortStr);
    }

    address.sin_family = AF_INET;
    address.sin_port = htons(peerPort);

    if (inet_pton(AF_INET, &peerIp[0], &address.sin_addr) <= 0)
    {
        printf("\nInvalid address/ Address not supported \n");
        return NULL;
    }

    if (bind(server_socket, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 5) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    vector<thread> threads;
    while (true)
    {
        int client_socket;

        if ((client_socket = accept(server_socket, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            perror("Acceptance error");
        }

        threads.push_back(thread(handle_peer_connection, client_socket));
    }
    for (auto it = threads.begin(); it != threads.end(); it++)
    {
        if (it->joinable())
            it->join();
    }
    close(server_socket);
    return nullptr;
}