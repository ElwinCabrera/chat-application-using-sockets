#ifndef SERVER_H
#define SERVER_H
#include <iostream>
#include <vector>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "global.h"

using std::string;
using std::vector;

class Server{

private:
  int listen_socket;
  int max_socket;
  int portnum;
  char serv_hostname[BUFFER_MAX];
  char serv_ip[INET_ADDRSTRLEN];
  struct addrinfo *ai;
  struct sockaddr_in *sa;
  fd_set master_fds;
  fd_set read_fds;
  vector<string> cmds;
  vector<int> conn_socks;
  vector<struct sockaddr_storage> conn_cli;

  int populate_addr(string hname_or_ip);
  int sock_and_bind();
  void handle_shell_cmds();
  int handle_new_conn_request();
  int recv_data_from_conn_sock(int idx_socket);
  int close_remote_conn(int socket);

public:
  Server(string port);
  ~Server();
  void start_server();
  string get_hostname();
  string get_ip();
  void debug_dump();
};

#endif // SERVER_H
