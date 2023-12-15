#ifndef tracker_h
#define tracker_h

#include <bits/stdc++.h>
#include <string.h>
#include <openssl/sha.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdarg.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
using namespace std;

#define um unordered_map

#define MAX_WAITING_CONNECTIONS 5

extern um<string, string> userCredentials;
extern um<string, bool> currentlyLoggedIn;
extern um<string, bool> groups;
extern um<string, string> admins;
extern um<string, set<string>> groupMembers;
extern um<string, set<string>> pendingRequests;
extern um<string, um<string, set<string>>> seeders;
extern um<string, long long> fileSize;
extern um<string, vector<string>> hashes;
extern um<string, string> userToIp;

void handle_connection(int socket);
int register_user(vector<string> &command);
int login_user(vector<string> &command);
int create_group(vector<string> &command, string client_user_id, int socket_);
int list_groups(vector<string> &command, int socket_);
int accept_request(vector<string> &command, string client_user_id, int socket_);
int join_request(vector<string> &command, string client_user_id, int socket_);
int list_requests(vector<string> &command, int socket_);
int upload_file(vector<string> command, int socket_, string client_user_id);
int list_files(vector<string> command, int socket_, string client_user_id);
int download_file(vector<string> command, int socket_, string client_user_id);

// Utilities
string getAddressFromFile(const char *filename, int i);
void parseAddressAndPort(const char *addressAndPort,
                         struct sockaddr_in &serverAddress_, string &portStr, string &address_,
                         int &port_);
vector<string> splitString(const char *input, const char *delimiter);

#endif