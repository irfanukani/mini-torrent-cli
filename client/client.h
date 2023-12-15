#ifndef client_h
#define client_h

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

const int CHUNK_SIZE = 512000;

extern string trackerIp;
extern int trackerPort;
extern string currentPortStr;

extern map<string, string> uploads;
extern map<string, string> downloads;
extern map<string, map<int, vector<string>>> currentDownloads;


int connectWithTracker(struct sockaddr_in &serv_addr, int &sock);
int handleCommandsToTracker(int socket_);
void handle_tracker_reply(int socket_);
int upload_file(vector<string> &command, int socket_, string buffer);
int download_file(struct downloadFileInfo *d);
void* acceptIncomingConnections(void* args);

// Utilities
string getAddressFromFile(const char *filename, int i);
void parseAddressAndPort(const char *addressAndPort,
                         struct sockaddr_in &serverAddress_, string &portStr, string &address_,
                         int &port_);
bool isFileValid(const std::string &filePath);
long long getFileSize(const std::string &filePath);
string getFileHash(const char *path);
vector<string> splitString(const char *input, const char *delimiter);
string readChunkByChunkandHash(const char* filename);

#endif