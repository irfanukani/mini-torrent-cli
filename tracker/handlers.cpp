#include "tracker.h"

int ensureAuthenticated(string username, int socket_)
{
    if (currentlyLoggedIn[username])
        return 1;
    else
    {
        write(socket_, "You must be authenticated!\n", 28);
        return -1;
    }
}

void handle_connection(int socket_)
{
    cout << "----------------------------------------------------\n";
    cout << "Thread Started for client : " << socket_ << "\n";

    string client_user_id = "";
    string client_group_id = "";

    while (true)
    {
        char bufferInput[1024] = {0};

        if (read(socket_, bufferInput, 1024) <= 0)
        {
            currentlyLoggedIn[client_user_id] = false;
            for (auto &x : seeders)
            {
                for (auto &y : x.second)
                {
                    y.second.erase(userToIp[client_user_id]);
                }
            }
            close(socket_);
            break;
        }
        cout << "Client sent : " << bufferInput << "\n";

        string s, in = string(bufferInput);
        stringstream ss(in);
        vector<string> command;

        while (ss >> s)
        {
            command.push_back(s);
        }

        if (command[0] == "login")
        {
            if (command.size() != 4)
            {
                write(socket_, "Invalid Arguments!", 18);
            }
            else
            {
                int res = login_user(command);
                if (res < 0)
                {
                    write(socket_, "Invalid Credentials!", 20);
                }
                else if (res)
                {
                    write(socket_, "Already Logged In!", 18);
                }
                else
                {
                    write(socket_, "Logged In successfully!", 23);
                    client_user_id = command[1];
                    userToIp[client_user_id] = command[3];
                }
            }
            continue;
        }
        if (command[0] == "create_user")
        {
            if (command.size() != 3)
            {
                write(socket_, "Invalid Arguments!", 18);
            }
            else
            {
                int res = register_user(command);
                if (res < 0)
                {
                    write(socket_, "User Already Exists!", 20);
                }
                else
                {
                    write(socket_, "Account Created!", 16);
                }
            }
            continue;
        }
        if (command[0] == "create_group")
        {
            if (ensureAuthenticated(client_user_id, socket_) != -1)
                create_group(command, client_user_id, socket_);

            continue;
        }
        if (command[0] == "list_groups")
        {
            if (ensureAuthenticated(client_user_id, socket_) != -1)
                list_groups(command, socket_);
            continue;
        }
        if (command[0] == "join_group")
        {
            if (ensureAuthenticated(client_user_id, socket_) != -1)
            {
                join_request(command, client_user_id, socket_);
            }
            continue;
        }
        if (command[0] == "accept_request")
        {
            if (ensureAuthenticated(client_user_id, socket_) != -1)
                accept_request(command, client_user_id, socket_);
            continue;
        }
        if (command[0] == "list_requests")
        {
            if (ensureAuthenticated(client_user_id, socket_) != -1)
            {
                list_requests(command, socket_);
            }
            continue;
        }
        if (command[0] == "upload_file")
        {
            if (ensureAuthenticated(client_user_id, socket_) != -1)
            {
                upload_file(command, socket_, client_user_id);
            }
            continue;
        }
        if (command[0] == "list_files")
        {
            if (ensureAuthenticated(client_user_id, socket_) != -1)
                list_files(command, socket_, client_user_id);
            continue;
        }
        if (command[0] == "download_file")
        {
            if (ensureAuthenticated(client_user_id, socket_) != -1)
            {
                download_file(command, socket_, client_user_id);
            }
            continue;
        }
        if (command[0] == "stop_share")
        {
            if (ensureAuthenticated(client_user_id, socket_) != -1)
            {
                if (command.size() == 3)
                {
                    seeders[command[1]][command[2]].erase(userToIp[client_user_id]);
                }
                write(socket_, "Stopped Sharing.", 16);
            }
            continue;
        }
        if (command[0] == "logout")
        {
            if (ensureAuthenticated(client_user_id, socket_) != -1)
            {
                for (auto &x : seeders)
                {
                    for (auto &y : x.second)
                    {
                        y.second.erase(userToIp[client_user_id]);
                    }
                }
                currentlyLoggedIn[client_user_id] = false;
                write(socket_, "Logged out.", 11);
            }
            continue;
        }
        if (command[0] == "leave_group")
        {
            if (ensureAuthenticated(client_user_id, socket_) != -1)
            {
                if (command.size() == 2)
                {
                    groupMembers[command[1]].erase(client_user_id);
                    for (auto &x : seeders[command[1]])
                    {
                        x.second.erase(userToIp[client_user_id]);
                    }
                }
                write(socket_, "Group left.", 11);
            }
            continue;
        }
        if (command[0] == "debug")
        {
            string response = "";
            for (auto x : seeders)
            {
                response += "Group Name : " + x.first + " \n";
                response += "[ ";
                for (auto y : x.second)
                {
                    response += y.first;
                    response += "( ";
                    for (auto z : y.second)
                    {
                        response += z + " , ";
                    }
                    response += ") ";
                }
                response += "] ";
            }
            write(socket_, response.c_str(), response.length());
            continue;
        }

        write(socket_, "Invalid Command!", 16);
    }
    close(socket_);
}

int register_user(vector<string> &command)
{
    if (userCredentials.find(command[1]) != userCredentials.end())
    {
        return -1;
    }

    userCredentials[command[1]] = command[2];
    return 0;
}

int login_user(vector<string> &command)
{
    if (userCredentials.find(command[1]) == userCredentials.end())
    {
        return -1;
    }

    if (userCredentials[command[1]] != command[2])
    {
        return -1;
    }
    if (currentlyLoggedIn[command[1]])
        return 1;

    currentlyLoggedIn[command[1]] = true;

    return 0;
}

int create_group(vector<string> &command, string client_user_id, int socket_)
{
    if (command.size() != 2)
    {
        write(socket_, "Invalid Arguments!", 18);
        return -1;
    }
    if (groups.find(command[1]) != groups.end())
    {
        write(socket_, "Group already Exists!", 21);
        return -1;
    }
    groups[command[1]] = true;
    admins[command[1]] = client_user_id;
    groupMembers[command[1]].insert(client_user_id);
    write(socket_, "Group Created!", 14);
    return 1;
}

int list_groups(vector<string> &command, int socket_)
{
    if (command.size() != 1)
    {
        write(socket_, "Invalid Arguments!", 18);
        return -1;
    }
    string response = "";
    for (auto x : groups)
    {
        response += x.first;
        response += '\n';
    }
    write(socket_, response.c_str(), response.length());
    return 0;
}

int join_request(vector<string> &command, string client_user_id, int socket_)
{
    if (command.size() != 2)
    {
        write(socket_, "Invalid Arguments!", 18);
        return -1;
    }
    // If group does not exist return error
    if (groups.find(command[1]) == groups.end())
    {
        write(socket_, "No Such group!", 14);
        return -1;
    }

    pendingRequests[command[1]].insert(client_user_id);
    write(socket_, "Request to join group sent!", 27);
    return 1;
}

int list_requests(vector<string> &command, int socket_)
{
    if (command.size() != 2)
    {
        write(socket_, "Invalid Arguments!", 18);
        return -1;
    }

    if (groups.find(command[1]) == groups.end())
    {
        write(socket_, "No Such group!", 14);
        return -1;
    }

    string response;
    for (auto x : pendingRequests[command[1]])
    {
        response += x;
        response += "\n";
    }
    write(socket_, response.c_str(), response.length());
    return 1;
}

int accept_request(vector<string> &command, string client_user_id, int socket_)
{
    if (command.size() != 3)
    {
        write(socket_, "Invalid Arguments!", 18);
        return -1;
    }

    if (admins[command[1]] != client_user_id)
    {
        write(socket_, "You are not an admin of this group!", 35);
        return -1;
    }

    if (pendingRequests.find(command[1]) == pendingRequests.end())
    {
        write(socket_, "No such request!", 16);
        return -1;
    }

    pendingRequests[command[1]].erase(command[2]);

    if (userCredentials.find(command[2]) == userCredentials.end())
    {
        write(socket_, "No such user to accept request!", 31);
        return -1;
    }
    groupMembers[command[1]].insert(command[2]);
    write(socket_, "Request Accepted!", 17);
    return 1;
}

int upload_file(vector<string> command, int socket_, string client_user_id)
{
    if (command.size() != 3)
    {
        write(socket_, "Invalid Arguments!", 18);
        return -1;
    }

    if (groups.find(command[2]) == groups.end())
    {
        write(socket_, "No Such group!", 14);
        return -1;
    }

    if (groupMembers[command[2]].find(client_user_id) == groupMembers[command[2]].end())
    {
        write(socket_, "You are not part of the group!", 30);
        return -1;
    }
    write(socket_, "Uploading...", 12);

    char fileDetails[10240] = {0};
    if (read(socket_, fileDetails, 10240))
    {
        vector<string> fileInfo = splitString(string(fileDetails).c_str(), string("$_$").c_str());
        string filename = splitString(string(fileInfo[0]).c_str(), "/").back();

        if (seeders[command[2]].find(filename) == seeders[command[2]].end())
        {
            seeders[command[2]].insert({filename, {fileInfo[1]}});
        }
        else
        {
            seeders[command[2]][filename].insert(fileInfo[1]);
        }

        fileSize[filename] = stoi(fileInfo[2]);

        vector<string> peicewiseHashes;

        for (int i = 3; i < (int)fileInfo.size(); i++)
        {
            peicewiseHashes.push_back(fileInfo[i]);
        }

        hashes[filename] = peicewiseHashes;

        write(socket_, "Uploaded!", 9);
    }

    return 0;
}

int list_files(vector<string> command, int socket_, string client_user_id)
{
    if (command.size() != 2)
    {
        write(socket_, "Invalid Arguments!", 18);
        return -1;
    }
    if (groupMembers[command[1]].find(client_user_id) == groupMembers[command[1]].end())
    {
        write(socket_, "Access Denied!", 14);
        return -1;
    }

    string response = "Files in " + command[1] + " :\n";
    for (auto y : seeders[command[1]])
    {
        response += y.first + "\n";
    }
    write(socket_, response.c_str(), response.length());
    return 0;
}

int download_file(vector<string> command, int socket_, string client_user_id)
{
    if (command.size() != 4)
    {
        write(socket_, "Invalid Arguments!", 18);
        return -1;
    }

    if (seeders[command[1]].find(command[2]) == seeders[command[1]].end())
    {
        write(socket_, "No such file!", 13);
        return -1;
    }

    string response = "200$_$";
    for (auto x : seeders[command[1]][command[2]])
    {
        response += x;
        response += "$_$";
    }
    response += to_string(fileSize[command[2]]);

    response += "$$$$$";
    for (auto x : hashes[command[2]])
    {
        response += x;
        response += "$_$";
    }
    send(socket_, response.c_str(), response.length(), MSG_NOSIGNAL);
    return 1;
}