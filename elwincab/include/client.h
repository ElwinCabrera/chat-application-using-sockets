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
    vector<struct peer_info> peers;

    void handle_shell_cmds(string stdin_string);
    int receive_data_from_host();
    struct sockaddr_in* populate_addr(string hname_or_ip, int port);
    int connect_to_host(string server_ip, int port);
    string get_ip_from_sa(struct sockaddr_in *sa);
    struct peer_info get_peer(string ip);
    struct peer_info* get_peer_ptr(string ip);

    int send_cmds_to_server();

    void cmd_list();
    void cmd_login(string host_ip, int port);
    void cmd_logout();
    void cmd_exit();

    int custom_send(int socket, string msg);
    int custom_recv(int socket, string &buffer);

    void serv_res_refresh(string data);
    void serv_res_relay_brod(string data);
    void serv_res_success(string cmd_end);
    void debug_dump();


public:

    Client(string serv_port);
    ~Client();
    void start_client();
    //bool is_valid_ip(string ip);
    bool is_ip_in_peers_list(string ip);
};

#endif // CLIENT_H