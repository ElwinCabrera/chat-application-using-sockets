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
  int portnum;
  char serv_hostname[BUFFER_MAX];
  char serv_ip[INET_ADDRSTRLEN];
  struct addrinfo *ai_results;
  struct addrinfo hints;
  struct sockaddr_in *sa_in;
  fd_set master_fds;
  fd_set read_fds;
  vector<string> cmds;
  vector<int> accepted_sockets;
  vector<string> cli_hostname_ip;

  int populate_addr(string hostname_or_ip);
  int get_socket_and_bind();

public:
  Server(string port);
  ~Server();
  void start_server();
  string get_hostname();
  string get_ip();
  void debug_dump();
  /*void cmd_statistics();
  void cmd_blocked();*/

};

#endif // SERVER_H
