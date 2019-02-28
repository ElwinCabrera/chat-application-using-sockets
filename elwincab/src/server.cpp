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
#include <algorithm>
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
  logout_msg = "yo im done homie, for now.";

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
  //free all elements in conn_his
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
  if(cmds.at(0).compare(AUTHOR) == 0) cmd_author();
  if (cmds.at(0).compare(IP) == 0) cmd_ip(serv_ip);
  if(cmds.at(0).compare(PORT) == 0) cmd_port(portnum);
  if(cmds.at(0).compare(LIST) == 0) cmd_list();
  if(cmds.at(0).compare(STATISTICS) == 0) cmd_statistics();
  if(cmds.at(0).compare(BLOCKED) == 0) cmd_blocked(get_rmhi_from_ip(cmds.at(1)));
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

/*bool Server::has_msgs(struct sockaddr_in *sa){
  string ip = get_ip_from_sa(sa);
  it = stored_msgs.find(ip);
  if(it == stored_msgs.end()) return false;  //no message
  return true;
}*/


/*if this function returns -1 then socket is not in the list of conn_socks (defined as vector)*/
int Server::recv_data_from_conn_sock(int idx_socket){
  int conn_sock = 0;
  char *recvd_data = (char*) malloc(sizeof(char*) * BUFFER_MAX);
  memset(recvd_data, 0, BUFFER_MAX);
  //receve data form connected socket here
  int bytes_recvd = recv(idx_socket, recvd_data, BUFFER_MAX, 0);
  string data(recvd_data);
  struct remotehos_info *rmh_i = get_rmhi_from_sock(idx_socket); // nullptr if not in conn_his

  //check if data is zero
  if(bytes_recvd != 0){
    cout<<data<<endl; // debug
    stringstream data_ss(data);
    cmds.erase(cmds.begin(),cmds.end());
    string cmd, token;
    while (getline(data_ss, token, ' ')) {cmds.push_back(token);}

    if(cmds.at(0).compare(BROADCAST)) relay_msg_to_all(rmh_i,cmds.at(1));
    if(cmds.at(0).compare(LOGOUT)) logout(rmh_i);
    if(cmds.at(0).compare(REFRESH)) send_list_loggedin_hosts(rmh_i);  //need implement 
    if(cmds.at(0).compare(BLOCK)) block(rmh_i, cmds.at(1));
    if(cmds.at(0).compare(UNBLOCK)) unblock(rmh_i, cmds.at(1));
    if(cmds.at(0).compare(SEND)) relay_msg_to(rmh_i, cmds.at(1), cmds.at(2));
  } else {

  }
  free(recvd_data);
  return conn_sock;
}



                            /* Operations that we may be asked by a remote host*/

void Server::block(struct remotehos_info *rmh_i, string ip){
  struct remotehos_info *remote_host = get_rmhi_from_ip(ip);
  rmh_i->blocked_hosts.push_back(remote_host->sa);
}

void Server::unblock(struct remotehos_info *rmh_i, string ip){
  int blocked_size = rmh_i->blocked_hosts.size();
  struct sockaddr_in *saddr;
  for (uint i = 0; i < blocked_size; i++) {
    saddr = rmh_i->blocked_hosts.at(i);
    string sa_ip = get_ip_from_sa(saddr);
    if(sa_ip.compare(ip) == 0) {rmh_i->blocked_hosts.erase(rmh_i->blocked_hosts.begin()+ i); return;}
  }
}


void Server::relay_msg_to_all(struct remotehos_info *rmh_i, string msg){
  int conn_size = conn_his.size();
  for(auto &itr : conn_his ){
    struct remotehos_info *rmhi = itr;
    string dst_ip = get_ip_from_sa(rmhi->sa);
    relay_msg_to(rmh_i, dst_ip, msg);
  }

}

void Server::relay_msg_to(struct remotehos_info *rmh_i, string dst_ip, string msg){
  //check if the destined client has not blocked this client if so then dont do anything
  string src_ip = get_ip_from_sa(rmh_i->sa);
  struct remotehos_info *dst_rmh_i = get_rmhi_from_ip(dst_ip);
  if(dest_ip_blocking_src_ip(src_ip, dst_ip)) return;

  int bytes_sent = 0;
  //check if the destined client is logged in if not then buffer the messege
  if(dst_rmh_i->loggedin) {
    bytes_sent = send(dst_rmh_i->sock, msg.c_str(),sizeof(msg),0);
  } else {
    pair<string, string> from_and_msg;
    from_and_msg.first = src_ip;
    from_and_msg.second = msg;
    it = stored_msgs.begin();
    it = stored_msgs.find(dst_ip);

    if(it == stored_msgs.end()){ //currently there are no stored messages for dst_ip
      vector<pair<string,string>> from_and_msg_vec;
      from_and_msg_vec.push_back(from_and_msg);
      stored_msgs.insert(make_pair(dst_ip, from_and_msg_vec));
    } else { // an entry exists so just 
      it->second.push_back(from_and_msg);
    }
  }

  if(bytes_sent == -1) {cout << "error sending to '"<<dst_ip <<"' error#"<<errno<< endl; return;}
  rmh_i->msg_bytes_tx += bytes_sent;
}


void Server::logout(struct remotehos_info *rmh_i){
  int success = close(rmh_i->sock);
  if(success != 0) cout<<"failed to close socket#"<<socket<<" in logout operation, error#"<< errno<<endl;
  rmh_i->loggedin = false;
}

void Server::send_list_loggedin_hosts(struct remotehos_info *rmh_i){

}

int Server::close_remote_conn(int socket){
  int success = 0;
  string ip;

  for(uint i = 0; i<conn_his.size(); i++){
    if(conn_his.at(i)->sock == socket){
      ip = get_ip_from_sa(conn_his.at(i)->sa);
      conn_his.erase(conn_his.begin()+i);
    }
  }

  it = stored_msgs.begin();
  it = stored_msgs.find(ip);
  if(it != stored_msgs.end()) stored_msgs.erase(it);

  //remove from master fd list and pick a new maxsocket
  FD_CLR(socket, &master_fds);
  max_socket =0 ;
  for(auto &itr : conn_his){ if(itr->sock > max_socket) max_socket = itr->sock;  }

  success = close(socket);
  if(success != 0) cout<<"failed to close socket#"<<socket<<" error#"<< errno<<endl;

  return success;
}




                                          /* HELPER AND GETTER FUNCTIONS */




/*string Server::get_hostname(){ return serv_hostname;}
string Server::get_ip() {return serv_ip;}*/

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

bool Server::dest_ip_blocking_src_ip(string src_ip, string dest_ip){
  struct remotehos_info *dest = get_rmhi_from_ip(dest_ip);
  vector<struct sockaddr_in*> dst_blocked_lst = dest->blocked_hosts;
  int blocked_lst_size = dst_blocked_lst.size();
  for(uint i = 0; i< blocked_lst_size; i++){
    struct sockaddr_in *saddr = dst_blocked_lst.at(i);
    string dst_blocked_ip = get_ip_from_sa(saddr);
    if(src_ip.compare(dst_blocked_ip)== 0 ) return true;
  }
  return false;
}

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




                                  /* SHELL CMDs */




void Server::cmd_list(){ //get list of logged in hosts sorted by port number

  cmd_success_start("LIST");
  sort(conn_his.begin(), conn_his.end(), sort_rmh_by_port);
  string ip;
  char hostname[HOSTNAME_LEN];
  int port, ni_ret;
  for(int i =0; i<conn_his.size(); i++){
    struct remotehos_info *rmh_i = conn_his.at(i);
    if(!rmh_i->loggedin) continue;
    
    ip = get_ip_from_sa(rmh_i->sa);
    port = ntohs(rmh_i->sa->sin_port);

    ni_ret = getnameinfo((struct sockaddr*) rmh_i->sa, sizeof(rmh_i->sa), hostname, sizeof(hostname), NULL, 0, NI_NOFQDN | NI_NAMEREQD);
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
    ni_ret = getnameinfo((struct sockaddr*) rmh_i->sa, sizeof(rmh_i->sa), hostname, sizeof(hostname), NULL, 0, NI_NOFQDN | NI_NAMEREQD);
    if(ni_ret){
      cmd_error("STATISTICS");
      cout<< "could not get hostname, in statistics error: "<<gai_strerror(errno)<<endl;
      return;
    }

    if(rmh_i->loggedin) 
      login_status = "logged-in";
    else
      login_status = "logged-out";

    cse4589_print_and_log("%-5d%-35s%-8d%-8d%-8s\n", i+1, hostname, rmh_i->msg_bytes_tx, rmh_i->msg_bytes_rx, login_status.c_str());
  }
  cmd_end("STATISTICS");

}

void Server::cmd_blocked(struct remotehos_info *rmh_i){
  //The output should display the hostname, IP address, and the listening port numbers of the bloked clents

  cmd_success_start("BLOCKED");
  vector<struct sockaddr_in*> blocked_hosts = rmh_i->blocked_hosts;
  sort(blocked_hosts.begin(), blocked_hosts.end(), sort_sa_by_port);
  
  string ip;
  char hostname[HOSTNAME_LEN];
  int port, ni_ret;
  for(int i =0; i< blocked_hosts.size(); i++){
    struct sockaddr_in *sockaddr = blocked_hosts.at(i);
    port = ntohs(sockaddr->sin_port);
    ip = get_ip_from_sa(sockaddr);

    ni_ret = getnameinfo((struct sockaddr*) sockaddr, sizeof(sockaddr), hostname, sizeof(hostname), NULL, 0, NI_NOFQDN | NI_NAMEREQD);
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
