#ifndef CLIENT_H
#define CLIENT_H

#include <iostream>
#include <vector>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <map>
#include "global.h"

using std::string;
using std::vector;
using std::map;
using std::pair;

struct peer_info{
    string hostname;
    string ip;
    int port;
};

class Client{

private:
    string my_ip;
    string my_hostname;
    int server_port;
    struct sockaddr_in *server_saddr;
    struct sockaddr_in *my_saddr;
    fd_set master_fds;
    fd_set read_fds;
    int max_socket;
    int my_socket;
    bool loggedin;
    bool exit_program;
    vector<string> cmds;
    vector<struct peer_info*> peers;


public:

    Clinet(string serv_port);
    ~Client();
    bool is_valid_ip(string ip);
    bool is_ip_in_peers_list(string ip);
};

#endif // CLIENT_H