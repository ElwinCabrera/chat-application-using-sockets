#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <cstring>
#include <sstream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <map>
#include "../include/server.h"
#include "../include/shell.h"

#include "../include/global.h"
#include "../include/logger.h"

using std::cout;
using std::cin;
using std::endl;
using std::string;
using std::vector;
using std::sort;
using std::to_string;
using std::stringstream;
using std::map;
using std::pair;

//inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);

/**
constructor class doing some basic initialization
gets our hostname and ip address, populates struct addrinfo *ai,
cretes socket and binds the ip and socket to the given port,
zeroes out and adds STDIN andd our socket to our master and read lists for use in select
*/
Server::Server(string port){
  cout<< "DEBUG: class Server, constructor called"<<endl;
  cout<< "started as server"<<endl;

  portnum = atoi(port.c_str());
  broadcastip ="255.255.255.255";
  logout_msg = "yo im done homie, for now."

  gethostname(serv_hostname, sizeof(serv_hostname));
  populate_addr(serv_hostname, portnum);

  sa = (struct sockaddr_in*) ai->ai_addr;
  inet_ntop(ai->ai_family, &(sa->sin_addr), serv_ip, sizeof(serv_ip));

  sock_and_bind();
  max_socket = listen_socket;

  FD_ZERO(&master_fds);
  FD_ZERO(&read_fds);

  FD_SET(STDIN, &master_fds);
  FD_SET(listen_socket, &master_fds);

}

/*this server class was destroyed close open sockets , notify remote hosts, de-allocate allocated space*/
Server::~Server()  {
  cout<< "DEBUG: class Server, DEstructor called"<<endl;
  if(ai) freeaddrinfo(ai);
  //if(sa) free(sa);
  close(listen_socket);
  //for (int i =0; i<conn_socks.size(); i++) cout << "socket: " <<conn_socks.at(i)<<endl;
}

/*at this point we are ready to start our server services and we can start listening.*/
void Server::start_server(){

  debug_dump();

  cout<<"> ";
  listen(listen_socket, 5);

  while(true){
    memcpy(&read_fds, &master_fds, sizeof(master_fds));

    int select_res = select(max_socket +1, &read_fds, NULL, NULL, NULL);
    if(select_res <0) cout<< "Select failed\n";

    for (int i = 0; i <=max_socket; i++) {
      if(FD_ISSET(i, &read_fds)) {  // true = at least one socket is ready to be read
        //find the socket that is ready
        if(i == STDIN){ // 0 = stdin
          //process stdin commands here
          handle_shell_cmds();

        } else if(i == listen_socket){ // true = a client is trying to connect to us, shall we choose to accept
          handle_new_conn_request();

        } else { // if its not a new connection or stdin then it must be an existing connection trying to communicate witn us (we got some data)
          recv_data_from_conn_sock(i);

        }
      }
    }
	}


}


/*you can imput a hostname, url orinet_ntop(their_addr.ss_family,
 get_in_addr((struct sockaddr *)&their_addr),
 s, sizeof s); an ip address of a remote host and it will
get all information such as IP, socket tyoe, protocal ect..
all this information is stored in a struct addrinfo *ai
we will most likely use this to get our own information for later use
if error occurs then it will retrn non-zero*/
int Server::populate_addr(string hname_or_ip, int port){
  int error_num = 0;
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  error_num = getaddrinfo(hname_or_ip.c_str(), to_string(port).c_str(), &hints, &ai);
  if (error_num !=0) cout<< "getaddrinfo: "<< gai_strerror(error_num) <<endl;
  return error_num;

}

string Server::get_hostname(){ return serv_hostname;}
string Server::get_ip() {return serv_ip;}

string Server::get_ip_from_sa(struct sockaddr_in *sa){
  char ip[INET_ADDRSTRLEN];
  inet_ntop(sa->sin_family, (struct sockaddr*) sa, ip, sizeof(ip));
  string ret(ip);
  return ret;
}
struct remotehos_info* Server::get_rmhi_from_ip(string ip){
  int his_size = conn_his.size();
  struct remotehos_info *rmh_i;
  for (uint i = 0; i < his_size; i++) {
    rmh_i = conn_his.at(i);
    string sa_ip = get_ip_from_sa(rmh_i->sa);
    if(sa_ip.compare(ip) == 0) return rmh_i;
  }
  return nullptr;

}
struct remotehos_info* Server::get_rmhi_from_sock(int sock){
  int his_size = conn_his.size();
  struct remotehos_info *rmh_i;
  for (uint i = 0; i < his_size; i++) {
    rmh_i = conn_his.at(i);
    if(rmh_i->sock == sock) return rmh_i;
  }
  return nullptr;
}

/*
Creates a socket and binds if either fails then it will return -1
if it succeeds then it will return 0
*/
int Server::sock_and_bind(){
  int error_num =0;

  listen_socket = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
  if(listen_socket < 0) cout << "Could not create socket, error#"<<errno<<endl;

  int bind_ret = bind(listen_socket, (struct sockaddr*) sa, sizeof(struct sockaddr_in));
  if(bind_ret == -1) cout << "Could not bind ip and socket to port, error#"<<errno<<endl;

  if(listen_socket == -1 || bind_ret == -1) {/*cout << "Quiting...goodbye\n";*/ error_num = -1;}
  return error_num;
}
/*
Takes input from stdin and parses them using space as a delimeter then puts
each parsed token in a vector (vector size should not be greater than 3, in most cases)
*/
void Server::handle_shell_cmds(){
  cmds.erase(cmds.begin(),cmds.end());
  string cmd, token;
  cin >> cmd;
  stringstream cmd_ss(cmd);
  while (getline(cmd_ss, token, ' ')) {cmds.push_back(token);}
  shell_execute(cmds);
  //cout << "> ";
}

/*if this function returns a number other than 0 then the new connection could not be established*/
int Server::handle_new_conn_request(){
  int new_conn_sock;
  struct sockaddr_in cli_addr;
  struct remotehos_info rmh_i;
  it = stored_msgs.begin();

  socklen_t addrlen = sizeof(cli_addr);
  new_conn_sock = accept(listen_socket, (struct sockaddr*) &cli_addr, &addrlen);

  if(new_conn_sock == -1) {
    cout<< "Could not connect host, accept failed, error#"<<errno<<endl;
    return new_conn_sock;
  }
  cout<< "Remote client connected\n";
  string ip = get_ip_from_sa(&cli_addr);
  it = stored_msgs.find(ip);
  

  if(it  != stored_msgs.end()){
    cout<< "You got some new messages while you were away"<<endl;
    vector<pair<string, string>> from_msg = it->second;
    for(auto itr = from_msg.begin(); itr != from_msg.end(); itr++){
      cout<<"from: "<<itr->first<<endl;
      cout<<"\t"<<itr->second<<endl;
    }
    stored_msgs.erase(it);
  }

  if(new_conn_sock > max_socket) max_socket = new_conn_sock;
  FD_SET(new_conn_sock, &master_fds);

  if(!host_in_history(&rmh_i)){
    rmh_i.msg_bytes_tx =0;
    rmh_i.msg_bytes_rx=0;
    rmh_i.loggedin = true;
    rmh_i.sa = &cli_addr;
    rmh_i.sock = new_conn_sock;
    conn_his.push_back(&rmh_i);
    cout<< "New client IP:"<<ip<<endl;
  }
  cout<<"Client "<<ip<<" logged back in\n";
  return new_conn_sock;
}
//if host is in the connection history (conn_his) that means that host is either logged in or logged out but has NOT exited the program
bool Server::host_in_history(struct remotehos_info *rmh_i){
  string ip = get_ip_from_sa(rmh_i->sa);
  string test_ip;
  for (uint i = 0; i < conn_his.size(); i++) {
    test_ip = get_ip_from_sa(conn_his.at(i)->sa);
    if(test_ip.compare(ip) ==0) return true;
  }
  return false;
}
/*bool Server::host_loggedin(struct remotehos_info *rmh_i){ // returns truen if host is logged in (if host is logged in that means thsat host exists)
  int total_clients = conn_his.size();
  for (int i = 0; i < total_clients; i++) {
    struct remotehos_info *rmh = conn_his.at(i);
    if(rmh_i->loggedin ) return true;
  }
  return false;
}*/
/*bool Server::has_msgs(struct sockaddr_in *sa){
  string ip = get_ip_from_sa(sa);
  it = stored_msgs.find(ip);
  if(it == stored_msgs.end()) return false;  //no message
  return true;
}*/

/*if this function returns -1 then socket is not in the list of conn_socks (defined as vector)*/
int Server::recv_data_from_conn_sock(int idx_socket){
  int conn_sock = -1;
  char *recvd_data = (char*) malloc(sizeof(char*) * BUFFER_MAX);
  memset(recvd_data, 0, BUFFER_MAX);
  //receve data form connected socket here
  int bytes_recvd = recv(conn_sock, recvd_data, BUFFER_MAX, 0);
  string data(recvd_data);
  struct remotehos_info *rmh_i = get_rmhi_from_sock(idx_socket); // nullptr if not in conn_his

  //check if data is zero
  if(bytes_recvd != 0){
    cout<<data<<endl; // debug
    stringstream data_ss(data);
    cmds.erase(cmds.begin(),cmds.end());
    string cmd, token;
    while (getline(data_ss, token, ' ')) {cmds.push_back(token);}

    if(cmds.at(0).compare(BROADCAST)) send_msg_to_all(cmds.at(1));
    if(cmds.at(0).compare(LOGOUT)) logout(rmh_i);
    if(cmds.at(0).compare(REFRESH)) send_conn_clinet_list(rmh_i);
    if(cmds.at(0).compare(BLOCK)) block(rmh_i, cmds.at(1));
    if(cmds.at(0).compare(UNBLOCK)) unblock(rmh_i, cmds.at(1));
    if(cmds.at(0).compare(SEND)) send_msg_to(rmh_i, cmds.at(1), cmds.at(2));
  } else {

  }
  free(recvd_data);
  return conn_sock;
}

void Server::block(struct remotehos_info *rmh_i, string ip){
  struct remotehos_info *remote_host = get_rmhi_from_ip(ip);
  rmh_i->blocked_hosts.push_back(remote_host->sa);
}
void Server::unblock(struct remotehos_info *rmh_i, string ip){
 
}

void Server::send_msg_to_all(string msg){

}
void send_msg_to(struct remotehos_info *rmh_i, string to_ip, string msg){
  //check if the destined client is logged in if not then buffer the messege
  //check if the destined client has not blocked this client if so then dont do anything

}
void logout(struct remotehos_info *rmh_i){

}

int Server::close_remote_conn(int socket){
  int success = 0;
  return success;
}




/* SHELL CMDs */


void Server::cmd_ip(){
  if (serv_ip == "") {cmd_error("IP"); return;}
  cmd_success_start("IP");
  cse4589_print_and_log("IP:%s\n", serv_ip);
  cmd_end("IP");
}

void Server::cmd_port(){
  if (port <= 0) {cmd_error("PORT"); return;}
  cmd_success_start("PORT");
  cse4589_print_and_log("PORT:%d\n", portnum);
  cmd_end("PORT");
}

void Server::cmd_list(){ //get list of logged in hosts sorted by port number

  cmd_success_start("LIST");
  sort(conn_his.begin(), conn_his.end(), sort_rmh_by_port);
  string hostname, ip;
  char hostname[HOSTNAME_LEN];
  int port, ni_ret;
  for(int i =0; i<conn_his.size(); i++){
    struct remotehos_info *rmh_i = conn_his.at(i);
    if(!rmh_i->loggedin) continue;
    
    ip = get_ip_from_sa(rmh_i->sa);
    port = ntohs(rmh_i->sa->sin_port);

    ni_ret = getnameinfo((struct sockaddr) rmh_i->sa, sizeof(rmh_i->sa), hostname, sizeof(hostname), NULL, 0, NI_NOFQDN | NI_NAMEREQ);
    if(ni_ret){
      cmd_error("LIST");
      cout<< "could not get hostname for '"<<ip<<"' error: "<<gai_strerror(errno)<<endl;
      return;
    }
    cse4589_print_and_log("%-5d%-35s%-20s%-8d\n", i+1, hostname, ip.c_str(), port);
  }
  cmd_end("LIST");
}

void Server::cmd_statistics(){
  cmd_success_start("STATISTICS");
  sort(conn_his.begin(), conn_his.end(), sort_rmh_by_port);
  string login_status;
  char hostname[HOSTNAME_LEN];
  int port, ni_ret;
  for(int i =0; i<conn_his.size(); i++){
    struct remotehos_info *rmh_i = conn_his.at(i);
    ni_ret = getnameinfo((struct sockaddr) rmh_i->sa, sizeof(rmh_i->sa), hostname, sizeof(hostname), NULL, 0, NI_NOFQDN | NI_NAMEREQ);
    if(ni_ret){
      cmd_error("STATISTICS");
      cout<< "could not get hostname for '"<<ip<<"' error: "<<gai_strerror(errno)<<endl;
      return;
    }

    if(rmh_i->logged) 
      login_status = "logged-in";
    else
      login_status = "logged-out"

    cse4589_print_and_log("%-5d%-35s%-8d%-8d%-8s\n", i+1, hostname, rmh_i->msg_bytes_tx, rmh_i->msg_bytes_tx, login_status.c_str());
  }
  cmd_end("STATISTICS");

}

void Server::cmd_blocked(struct remotehos_info *rmh_i){
  //The output should display the hostname, IP address, and the listening port numbers of the bloked clents

  cmd_success_start("BLOCKED");
  vector<struct sockaddr_in*> bloked_hosts = rmh_i->bloked_hosts;
  sort(blocked_hosts.begin(), blocked_hosts.end(), sort_sa_by_port);
  
  string ip;
  char hostname[HOSTNAME_LEN];
  int port, ni_ret;
  for(int i =0; i< blocked_hosts.size(); i++){
    struct sockaddr_in *sockaddr = blocked_hosts.at(i);
    port = ntohs(sockaddr->sin_port);
    ip = get_ip_from_sa(sockaddr);

    ni_ret = getnameinfo((struct sockaddr) sockaddr, sizeof(sockaddr), hostname, sizeof(hostname), NULL, 0, NI_NOFQDN | NI_NAMEREQ);
    if(ni_ret){
      cmd_error("BLOCKED");
      cout<< "could not get hostname for '"<<ip<<"' error: "<<gai_strerror(errno)<<endl;
      return;
    }
    cse4589_print_and_log("%-5d%-35s%-20s%-8d\n", i+1, hostname, ip.c_str(), port);
  }
  cmd_end("BLOCKED");
  

}


/*Just some helpful debug information */
void Server::debug_dump(){
  cout<<endl;
  cout<<"hostname: " <<serv_hostname<<endl;
  cout<<"ip: " <<serv_ip<<endl;
  cout<<"port: " <<portnum<<endl;
  cout<<"listen_socket: " <<listen_socket<<endl;
  cout<<"max_socket: " <<max_socket<<endl;
  cout<<"printing a list of connected sockets\n";
  //if(conn_socks.size() ==0) cout<<"No connected hosts\n";
  //for (int i =0; i<conn_socks.size(); i++) cout << "socket: " <<conn_socks.at(i)<<endl;

  cout<<endl;
}
