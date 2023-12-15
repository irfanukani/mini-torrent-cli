#include "client.h"

long long getFileSize(const string &filePath)
{
    struct stat fileStat;

    if (stat(filePath.c_str(), &fileStat) == 0)
    {
        return static_cast<long long>(fileStat.st_size);
    }
    else
    {
        return -1;
    }
}

string getAddressFromFile(const char *filename, int i)
{
    int fd = open(filename, O_RDONLY);
    if (fd == -1)
    {
        cerr << "Error opening file" << endl;
        exit(1);
    }

    char buffer[1];
    vector<string> addresses;

    string line;
    while (read(fd, buffer, 1) == 1)
    {
        line += buffer[0];
        if (buffer[0] == '\n')
        {
            addresses.push_back(line);
            line.clear();
            continue;
        }
        if (buffer[0] == '\0')
        {
            if (line.size())
            {
                addresses.push_back(line);
            }
            break;
        }
    }

    close(fd);

    if (i < 1 || i > (int)addresses.size() + 1)
    {
        cout << "Invalid Tracker Number \n";
        exit(0);
    }

    return addresses[i - 1];
}

void parseAddressAndPort(const char *addressAndPort, struct sockaddr_in &serverAddress_, string &portStr, string &address_, int &port_)
{
    istringstream iss(addressAndPort);
    char delimiter = ':';

    string addressStr;
    if (getline(iss, addressStr, delimiter) && getline(iss, portStr, delimiter))
    {
        address_ = addressStr;

        if (inet_pton(AF_INET, addressStr.c_str(), &serverAddress_.sin_addr) <= 0)
        {
            cerr << "Invalid address: " << addressStr << endl;
            exit(1);
        }

        port_ = stoi(portStr);
    }
}

vector<string> splitString(const char *input, const char *delimiter)
{
    string inp = string(input);
    string del = string(delimiter);

    std::vector<std::string> result;
    size_t start = 0;
    size_t end = 0;

    while ((end = inp.find(del, start)) != std::string::npos)
    {
        if (end - start > 0)
        {
            result.push_back(inp.substr(start, end - start));
        }
        start = end + del.length();
    }

    if (start < inp.size())
    {
        result.push_back(inp.substr(start));
    }

    return result;
}

bool isFileValid(const string &filePath)
{
    if (access(filePath.c_str(), F_OK) == 0 && access(filePath.c_str(), R_OK) == 0)
    {
        return true;
    }
    return false;
}

string readChunkByChunkandHash(const char *filename)
{
    char buffer[CHUNK_SIZE];
    string result;

    int fileDescriptor = open(filename, O_RDONLY);
    if (fileDescriptor == -1)
    {
        cerr << "Error opening file." << endl;
        return result;
    }

    while (true)
    {
        string temp;
        ssize_t bytesRead = read(fileDescriptor, buffer, CHUNK_SIZE);
        if (bytesRead == -1)
        {
            cerr << "Error reading from file." << endl;
            break;
        }

        if (bytesRead == 0)
        {
            break;
        }

        unsigned char messageDigest[SHA256_DIGEST_LENGTH];
        if (!SHA256(reinterpret_cast<const unsigned char *>(&buffer), bytesRead, messageDigest))
        {
            cout << "Can't Hash!\n";
        }
        else
        {
            for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
            {
                char buf[3];
                sprintf(buf, "%02x", messageDigest[i] & 0xff);
                temp += string(buf);
            }
        }
        result += temp.substr(0, 20) + "$_$";
        ;
    }

    close(fileDescriptor);
    return result;
}

// HASHING PART ----------------------------------------------------------------------
string getFileHash(const char *path)
{

    ostringstream buf;
    ifstream input(path);
    buf << input.rdbuf();
    string contents = buf.str(), hash;

    unsigned char messageDigest[SHA256_DIGEST_LENGTH];
    if (!SHA256(reinterpret_cast<const unsigned char *>(&contents[0]), contents.length(), messageDigest))
    {
        cout << "Can't Hash!\n";
    }
    else
    {
        for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
        {
            char buf[3];
            sprintf(buf, "%02x", messageDigest[i] & 0xff);
            hash += string(buf);
        }
    }
    return hash.substr(0, 20);
}