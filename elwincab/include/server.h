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
#include <map>
#include "global.h"

using std::string;
using std::vector;
using std::map;
using std::pair;


struct remotehos_info{
  int msg_bytes_rx;
  int msg_bytes_tx;
  int sock;
  bool loggedin;
  vector<struct sockaddr_in*> blocked_hosts;
  struct sockaddr_in *sa;
};


/*
This is an IPv4 server only, meaning all operations are done using IPv4,
a little modification can make this IP agnostic (both IPv4 and IPv6).
*/
class Server{

private:

  int listen_socket;
  int max_socket;
  int portnum;
  char serv_hostname[BUFFER_MAX];
  char serv_ip[INET_ADDRSTRLEN];
  string broadcastip;
  string logout_msg;
  struct addrinfo *ai;
  struct sockaddr_in *sa;
  fd_set master_fds;
  fd_set read_fds;
  vector<string> cmds;
  vector<struct remotehos_info*> conn_his;
  static bool sort_rmh_by_port(struct remotehos_info *rmh1, struct remotehos_info *rmh2) { return ntohs(rmh1->sa->sin_port) < ntohs(rmh2->sa->sin_port); }
  static bool sort_sa_by_port(struct sockaddr_in *sa1, struct sockaddr_in *sa2) { return ntohs(sa1->sin_port) < ntohs(sa2->sin_port); }
  map<string, vector<pair<string, string>> > stored_msgs;
  map<string, vector<pair<string, string>> >::iterator it;



  int populate_addr(string hname_or_ip, int port);
  int sock_and_bind();
  void handle_shell_cmds();
  int handle_new_conn_request();
  int recv_data_from_conn_sock(int idx_socket);
  
  /* Operations that we may be asked by a remote host*/
  void block(struct remotehos_info *rmh_i, string ip);
  void unblock(struct remotehos_info *rmh_i, string ip);
  void relay_msg_to_all(struct remotehos_info *rmh_i, string msg);
  void relay_msg_to(struct remotehos_info *rmh_i, string dst_ip, string msg);
  void logout(struct remotehos_info *rmh_i);
  void send_list_loggedin_hosts(struct remotehos_info *rmh_i);
  int close_remote_conn(int socket);

  /*some helper and getter functions */
  bool host_in_history(struct remotehos_info *rmh_i);
  bool dest_ip_blocking_src_ip(string src_ip, string dest_ip);
  
  string get_ip_from_sa(struct sockaddr_in *sa);
  struct remotehos_info* get_rmhi_from_ip(string ip);
  struct remotehos_info* get_rmhi_from_sock(int sock);



  /*shell commands*/
  void cmd_ip();
  void cmd_port();
  void cmd_list();
  void cmd_statistics();
  void cmd_blocked(struct remotehos_info *rmh_i);

public:
  Server(string port);
  ~Server();
  void start_server();
  string get_hostname(){return serv_hostname; }
  string get_ip() {return serv_ip;}
  void debug_dump();
};

#endif // SERVER_H
