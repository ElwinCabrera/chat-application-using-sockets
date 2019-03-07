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
#include "shell.h"

using std::string;
using std::vector;
using std::map;
using std::pair;


struct remotehos_info{
  int msg_bytes_rx;
  int msg_bytes_tx;
  int sock;
  int port;
  bool loggedin;
  string hostname;
  string ip;
  vector<struct remotehos_info> blocked_hosts;
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
  char my_hostname[BUFFER_MAX];
  string my_ip;
  struct sockaddr_in *my_saddr;
  fd_set master_fds;
  fd_set read_fds;
  vector<string> cmds;
  vector<string> client_cmds;
  vector<struct remotehos_info> conn_his;
  map<string, vector<pair<string, string>> > stored_msgs;
  map<string, vector<pair<string, string>> >::iterator it;



  static bool sort_hosts_by_port(struct remotehos_info host1, struct remotehos_info host2) { return host1.port < host2.port; }

  struct sockaddr_in* populate_addr(string hname_or_ip, int port);
  int sock_and_bind();
  void handle_shell_cmds(string cmd);
  int handle_new_conn_request();
  int recv_data_from_conn_sock(int idx_socket);
  
  /* Operations that we may be asked by a remote host*/
  void block(struct remotehos_info host, string new_block_ip);
  void unblock(struct remotehos_info host, string unblock_ip);
  void relay_msg_to_all(string src_ip, string msg);
  void relay_msg_to(string src_ip, string dst_ip, string msg);
  void logout(struct remotehos_info host);
  void send_current_client_list(struct remotehos_info host);
  void check_and_send_stored_msgs(string ip);

  int send_end_cmd(int socket, string end_cmd_sig, string to_ip);
  int close_remote_conn(int socket);

  /*some helper and getter functions */
  bool host_in_history(string ip);
  bool dest_ip_blocking_src_ip(string src_ip, string dest_ip);
  
  string get_sa_ip(struct sockaddr_in *sa);
  struct remotehos_info* get_host_ptr(string ip);
  struct remotehos_info get_host(string ip);

  struct remotehos_info* get_host_ptr(int sock);
  struct remotehos_info get_host(int sock);

  int custom_send(int socket, string msg);
  int custom_recv(int socket, string &buffer);



  /*shell commands*/
  //void cmd_ip();
  //void cmd_port();
  void cmd_list();
  void cmd_statistics();
  void cmd_blocked(string ip);


  //debug
  void debug_dump();

public:
  Server(string port);
  ~Server();
  void start_server();
  string get_serv_hostname(){return my_hostname; }
  string get_serv_ip() {return my_ip;}
};

#endif // SERVER_H
