#include "tracker.h"

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

void parseAddressAndPort(const char *addressAndPort, struct sockaddr_in &serverAddress_, string &portStr, string &address_, int &port_)
{
    std::istringstream iss(addressAndPort);
    char delimiter = ':';

    std::string addressStr;
    if (getline(iss, addressStr, delimiter) && getline(iss, portStr, delimiter))
    {
        address_ = addressStr;

        if (inet_pton(AF_INET, addressStr.c_str(), &serverAddress_.sin_addr) <= 0)
        {
            std::cerr << "Invalid address: " << addressStr << std::endl;
            exit(1);
        }

        port_ = std::stoi(portStr);
    }
}
