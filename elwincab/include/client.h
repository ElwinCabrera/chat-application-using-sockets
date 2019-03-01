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


    struct addrinfo* populate_addr(string hname_or_ip, int port);
    void connect_to_host(string server_ip, int port);
    string get_ip_from_sa(struct sockaddr_in *sa);
    void send_cmds_to_server();

    void cmd_list();
    void debug_dump();

public:

    Client(string serv_port);
    ~Client();
    void start_client();
    //bool is_valid_ip(string ip);
    void handle_shell_cmds();
    bool is_ip_in_peers_list(string ip);
};

#endif // CLIENT_H