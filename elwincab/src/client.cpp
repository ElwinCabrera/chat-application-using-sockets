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
#include "../include/client.h"
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

//int recv(int sockfd, void *buf, int len, int flags);
//int send(int sockfd, const void *msg, int len, int flags); 
Client::Client(string serv_port){

    my_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(my_socket == -1) perror("failed to create socket\n");

    char hname[HOSTNAME_LEN];
    gethostname(hname, sizeof(hname));
    my_hostname = hname;
    server_port = atoi(serv_port.c_str());
    my_saddr =  populate_addr(my_hostname, server_port);
    my_ip = get_ip_from_sa(my_saddr);

    FD_ZERO(&master_fds);
    FD_ZERO(&read_fds);

    FD_SET(STDIN, &master_fds);
    FD_SET(my_socket, &master_fds);
    max_socket = my_socket;

    loggedin = false;
    exit_program = false;
}

Client::~Client(){
    if(my_saddr) free(my_saddr);
    if(server_saddr) free(server_saddr);
    delete_peers_list();

}

void Client::start_client(){

  debug_dump();

  cout<<"> ";

  while(!exit_program){
    memcpy(&read_fds, &master_fds, sizeof(master_fds));

    int select_res = select(max_socket +1, &read_fds, NULL, NULL, NULL);
    if(select_res <0) cout<< "Select failed\n";

    for (int i = 0; i <=max_socket; i++) {
      if(FD_ISSET(i, &read_fds)) {  // true = at least one socket is ready to be read
        //find the socket that is ready
        if(i == STDIN){ // 0 = stdin
          //process stdin commands here
          string stdin_string;
          getline(cin, stdin_string);
          handle_shell_cmds(stdin_string);

        } else if(i == my_socket){ // ready to receve data from CONNECTED remote host or if doing p2p then peer is requesting to be connected to us
          //handle_new_conn_request();
          receive_data_from_host();

        } else { // in p2p, we are ready to receve data
          //recv_data_from_conn_sock(i);

        }
      }
    }
	}

}

void Client::handle_shell_cmds(string stdin_string){
    cmds.erase(cmds.begin(),cmds.end());
    string token;
    stringstream cmd_ss(stdin_string);
    while (getline(cmd_ss, token, ' ')) cmds.push_back(token); 

    if (cmds.at(0).compare(AUTHOR) == 0){
        if(cmds.size() != 1) { cout<< "error: command 'AUTHOR' does not take any arguments\n"; return; }
        cmd_author();
    } else if(cmds.at(0).compare(IP) == 0) {
        if(cmds.size() != 1) { cout<< "error: command 'IP' does not take any arguments\n"; return; }
        cmd_ip(my_ip);
    } else if (cmds.at(0).compare(PORT) == 0){
        if(cmds.size() != 1) { cout<< "error: command 'PORT' does not take any arguments\n"; return; }
        cmd_port(server_port);
    } else if(cmds.at(0).compare(LIST) == 0){
        if(cmds.size() != 1) { cout<< "error: command 'LIST' does not take any arguments\n"; return; }
        cmd_list();
    } else if(cmds.at(0).compare(LOGIN) == 0){
        
        if(cmds.size() != 3) { 
            cout<< "error: command 'LOGIN' takes 2 arguments\n";
            cout<< "\texample usage LOGIN <server-ip> <server-port>\n";
            return; 
        }

        cmd_login( cmds.at(1), atoi(cmds.at(2).c_str()) );

    } else if(cmds.at(0).compare(EXIT) == 0){
        if(cmds.size() != 1) { cout<< "error: command 'EXIT' does not take any arguments\n"; return; }
        cmd_exit();
    } else if(cmds.at(0).compare(LOGOUT) == 0){
      cmd_logout();
    } else if( (cmds.at(0).compare(REFRESH) == 0   || 
                cmds.at(0).compare(SEND) == 0      ||
                cmds.at(0).compare(BROADCAST) == 0 || 
                cmds.at(0).compare(BLOCK) == 0     || 
                cmds.at(0).compare(UNBLOCK) == 0 ) ){

                  if(!loggedin) {cout<< "You must be loggedin in order to execute the '"<<cmds.at(0)<<"' command\n"; return;}
    
                  /*if(!errors_in_cmd())*/ send_cmds_to_server();
    
    }else{
      cout << "Unknown command '" ;
      for(int i=0; i<cmds.size(); i++){ cout << cmds.at(i)<<" ";}
      cout<<"'"<<endl;

    }
    //cout<< "> ";
}

int Client::receive_data_from_host(){
  string token;
  char data[BUFFER_MAX];
  //int recv(int sockfd, void *buf, int len, int flags);
  int recv_ret = recv(my_socket, data, sizeof(data),0);
  //if(recv_ret < 0 ) cout << "Failed to receive data, error#"<<errno<<endl;

  //cout<<"Got '"<<data<<"' from server"<<endl;  // DEBUG

  stringstream data_ss(data);
  getline(data_ss, token, ':');
  if(token.compare(REFRESH) ==0  || token.compare(REFRESH_START) == 0) serv_res_refresh(data);
  if(token.compare(RELAYED) ==0 ) serv_res_relay_brod(data);

  if(token.compare(REFRESH_END) == 0  ||
     token.compare(SEND_END) == 0     ||
     token.compare(BROADCAST_END) == 0||
     token.compare(BLOCK_END) == 0    ||
     token.compare(UNBLOCK_END) == 0  ) serv_res_success(token);
  
  //if(token.compare(SEND_END) ==0) serv_res_success(token);
  //if(token.compare(BROADCAST_END) ==0) serv_res_success(token); 
  //if(token.compare(BLOCK_END) ==0) serv_res_success(token);
  //if(token.compare(UNBLOCK_END) ==0) serv_res_success(token); 

  return recv_ret;

}

struct sockaddr_in* Client::populate_addr(string hname_or_ip, int port){
  struct addrinfo *ai;
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  int error_num = getaddrinfo(hname_or_ip.c_str(), to_string(port).c_str(), &hints, &ai);
  if (error_num !=0) cout<< "getaddrinfo: "<< gai_strerror(error_num) <<endl;
  return (struct sockaddr_in*) ai->ai_addr;

}

int Client::connect_to_host(string server_ip, int port){
    
    server_saddr = populate_addr(server_ip, port);
    int connect_ret = connect(my_socket, (struct sockaddr*) server_saddr, sizeof(*server_saddr));
    if(connect_ret ==  -1) perror("failed to connect to remote host\n");
    if(connect_ret ==  0 && server_saddr) { perror("connected to '"); cout<<get_ip_from_sa(server_saddr)<<"' succesfuly\n"; }
    return connect_ret;
}


string Client::get_ip_from_sa(struct sockaddr_in *sa){
  char ip[INET_ADDRSTRLEN];
  inet_ntop(sa->sin_family, &(sa->sin_addr), ip, INET_ADDRSTRLEN);
  string ret(ip);
  return ret;
}
struct peer_info* Client::get_pi_from_ip(string ip){
  for(int i =0; i<peers.size(); i++){
    struct peer_info* pi = peers.at(i);
    if(ip.compare(pi->ip) == 0 ) return pi;
  }
  return nullptr;
}


int Client::send_cmds_to_server(){
    string msg;
    int cmds_size = cmds.size();
    for(uint i = 0; i< cmds_size; i++){
        msg += cmds.at(i);
        if(i != cmds_size -1) msg+=" "; // we don't want to add a space at the end
    }
    int send_ret = send(my_socket, msg.c_str(), sizeof(msg), 0);
    if(send_ret ==  -1) { perror("failed to send message to remote host msg: '"); cout<<msg<<"' \n"; }
    return send_ret;
}

void Client::cmd_list(){
  cmd_success_start(LIST);
  for(int i =0; i<peers.size(); i++){
    struct peer_info* pi = peers.at(i);
    cse4589_print_and_log("%-5d%-35s%-20s%-8d\n", i, pi->hostname, pi->ip, pi->port);
  }
  cmd_end(LIST);

}
void Client::cmd_login(string host_ip, int port){
  int ret = connect_to_host(host_ip, port);
  if(ret != 0) {cmd_error(LOGIN); return;}
  loggedin = true;
  cmd_success_start(LOGIN);
  cmd_end(LOGIN); 
}


void Client::cmd_logout(){
  int ret = close(my_socket);
  if(ret !=0) {cmd_error(LOGOUT); return;}
  cmd_success_start(LOGOUT);
  cmd_end(LOGOUT);
}
void Client::cmd_exit(){
  int ret = close(my_socket);
  if(ret !=0) {cmd_error(EXIT); return;}
  cmd_success_start(EXIT);
  cmd_end(EXIT);
  exit_program = true;
}


void Client::delete_peers_list(){
  for(uint i =0; i< peers.size(); i++){
    struct peer_info *pi = peers.at(i);
    if(pi) free(pi);
  }
  peers.erase(peers.begin(), peers.end());
}

void Client::serv_res_refresh(string data){
  struct peer_info *pi;
  string token, hostname, ip, port;
  stringstream data_ss(data);
  getline(data_ss, token, ':');
  
  if (token.compare(REFRESH_START) == 0){ 
    delete_peers_list();
  }else {
    getline(data_ss, token, ',');
    hostname = token;
    getline(data_ss, token, ',');
    ip = token;
    getline(data_ss, token);
    port = atoi(token.c_str());

    //if(!peers.empty()) {cout << "When trying to reconstruct peers list found that peers list is not empty\n"; return; }
    pi = (struct peer_info*) malloc( sizeof(struct peer_info));
    peers.push_back(pi);
  }
}

void Client::serv_res_relay_brod(string data){
  string token, from_ip, msg;
  stringstream data_ss(data);
  getline(data_ss, token, ':');

  getline(data_ss, token, ',');
  from_ip = token;
  getline(data_ss, token);
  msg = token;
  event_msg_recvd(from_ip, msg);
  
}

void Client::serv_res_success(string cmd_fin){
  string token;
  stringstream data_ss(cmd_fin);
  getline(data_ss, token, '_');

  cmd_success_start(token);
  cmd_end(token);

}



                        /* DEBUG */

void Client::debug_dump() {
  cout<<endl;
  cout<<"hostname: " <<my_hostname<<endl;
  cout<<"ip: " <<my_ip<<endl;
  cout<<"port: " <<server_port<<endl;
  cout<<"my_socket: " <<my_socket<<endl;
  cout<<"max_socket: " <<max_socket<<endl;
  //if(conn_socks.size() ==0) cout<<"No connected hosts\n";
  //for (int i =0; i<conn_socks.size(); i++) cout << "socket: " <<conn_socks.at(i)<<endl;
}