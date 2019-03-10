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
    first_time_login = true;
    exit_program = false;

    /*struct peer_info me;
    me.hostname = my_hostname;
    me.ip= my_ip;
    me.port = server_port;
    peers.push_back(me);*/
}

Client::~Client(){
    //if(my_saddr != NULL) free(my_saddr);
    //if(server_saddr != NULL ) free(server_saddr);
    if(!peers.empty()) peers.erase(peers.begin(), peers.end());

}

void Client::start_client(){

  debug_dump();

  cout<<"> ";

  while(!exit_program){
    memcpy(&read_fds, &master_fds, sizeof(master_fds));
    //cout<<"> ";
    //fflush(stdout);

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
          if(loggedin) receive_data_from_host();

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
        if(cmds.size() != 1) { cout<< "error: command 'AUTHOR' does not take any arguments\n"; cmd_error(cmds.at(0)); return; }
        cmd_author();
    } else if(cmds.at(0).compare(IP) == 0) {
        if(cmds.size() != 1) { cout<< "error: command 'IP' does not take any arguments\n"; cmd_error(cmds.at(0)); return; }
        cmd_ip(my_ip);
    } else if (cmds.at(0).compare(PORT) == 0){
        if(cmds.size() != 1) { cout<< "error: command 'PORT' does not take any arguments\n"; cmd_error(cmds.at(0)); return; }
        cmd_port(server_port);
    } else if(cmds.at(0).compare(LIST) == 0){
        if(cmds.size() != 1) { cout<< "error: command 'LIST' does not take any arguments\n"; cmd_error(cmds.at(0)); return; }
        cmd_list();
    } else if(cmds.at(0).compare(LOGIN) == 0){
        
        if(cmds.size() != 3 || !is_valid_ip(cmds.at(1)) || !is_valid_port(cmds.at(2))) { 
            cout<< "error: command 'LOGIN' takes 2 arguments\n";
            cout<< "example usage LOGIN <server-ip> <server-port>\n";
            cmd_error(cmds.at(0));
            return; 
        }
        if(!first_time_login && get_ip_from_sa(server_saddr).compare(cmds.at(1)) ==0) {
          cout<< "Logged back in\n";
          custom_send(my_socket, "LOGGEDIN ");
          loggedin = true;
          cmd_success_start(LOGIN);
          cmd_end(LOGIN);
          return;
        }

        cmd_login( cmds.at(1), atoi(cmds.at(2).c_str()) );

    } else if(cmds.at(0).compare(EXIT) == 0){
        if(cmds.size() != 1) { cout<< "error: command 'EXIT' does not take any arguments\n"; cmd_error(cmds.at(0)); return; }
        cmd_exit();
    } else if(cmds.at(0).compare(LOGOUT) == 0){
      if(cmds.size() != 1) { cout<< "error: command 'LOGOUT' does not take any arguments\n"; cmd_error(cmds.at(0)); return; }
      cmd_logout();
    } else if( (cmds.at(0).compare(REFRESH) == 0   || 
                cmds.at(0).compare(SEND) == 0      ||
                cmds.at(0).compare(BROADCAST) == 0 || 
                cmds.at(0).compare(BLOCK) == 0     || 
                cmds.at(0).compare(UNBLOCK) == 0 ) ){

                  if(!loggedin) {
                    cout<< "You must be loggedin in order to execute the '"<<cmds.at(0)<<"' command\n"; 
                    cmd_error(cmds.at(0));
                    return;
                  }

                  if(cmds.at(0).compare(SEND) == 0 && (cmds.size() < 1 || !is_valid_ip(cmds.at(1)) || !is_peer_in_list(cmds.at(1)) ) ){ cmd_error(cmds.at(0)); return; }
                  if(cmds.at(0).compare(BROADCAST) == 0 && cmds.size() <1) {cmd_error(cmds.at(0)); return; }
                  
                  if(cmds.at(0).compare(BLOCK) == 0 && 
                    (cmds.size() != 2 || 
                    !is_valid_ip(cmds.at(1)) || 
                    !is_peer_in_list(cmds.at(1) ) || 
                    is_blocked(cmds.at(1)) ) ) { cmd_error(cmds.at(0)); return; }
                  
                  
                  if(cmds.at(0).compare(UNBLOCK) == 0 && 
                    (cmds.size() != 2 || 
                    !is_valid_ip(cmds.at(1)) || 
                    !is_peer_in_list(cmds.at(1)) || 
                     !is_blocked(cmds.at(1)) )) { cmd_error(cmds.at(0)); return; }
                  
                  
                  if(cmds.at(0).compare(REFRESH) == 0 && cmds.size() != 1){ cmd_error(cmds.at(0)); return; }

                  
    
                   send_cmds_to_server();
    
    }else{
      cout << "Unknown command ' " ;
      for(int i=0; i<cmds.size(); i++){ cout << cmds.at(i)<<" ";}
      cout<<"'\n";
      cmd_error(cmds.at(0));

    }
}


int Client::receive_data_from_host(){
  string token;
  string data;
  int recv_ret = custom_recv(my_socket, data);

  stringstream data_ss(data);
  getline(data_ss, token, ':');
  if(token.compare(REFRESH) ==0  || token.compare(REFRESH_START) == 0) serv_res_refresh(data);
  if(token.compare(RELAYED) ==0 ) serv_res_relay_brod(data);

  if(token.compare(REFRESH_END) == 0  ||
     token.compare(SEND_END) == 0     ||
     token.compare(BROADCAST_END) == 0||
     token.compare(BLOCK_END) == 0    ||
     token.compare(UNBLOCK_END) == 0  ) serv_res_success(token);
  
  return recv_ret;

}

struct sockaddr_in* Client::populate_addr(string hname_or_ip, int port){
  struct addrinfo *ai;
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  int error_num = getaddrinfo(hname_or_ip.c_str(), itos(port).c_str(), &hints, &ai);
  if (error_num !=0) cout<< "getaddrinfo: "<< gai_strerror(error_num) <<endl;
  return (struct sockaddr_in*) ai->ai_addr;

}

int Client::connect_to_host(string server_ip, int port){
    
    server_saddr = populate_addr(server_ip, port);
    int connect_ret = connect(my_socket, (struct sockaddr*) server_saddr, sizeof(*server_saddr));
    if(connect_ret ==  -1) perror("failed to connect to remote host\n");
    if(connect_ret ==  0 && server_saddr) cout<<"connected to '"<< get_ip_from_sa(server_saddr)<<"' succesfuly\n";

    int send_ret = custom_send(my_socket, "HOSTNAME "+my_hostname);
    if(send_ret == -1) { cout<< "could not send hostname to host"; perror(""); cout<<"\n"; }

    send_ret = custom_send(my_socket, "PORT "+itos(server_port));
    if(send_ret == -1) { cout<< "could not send PORT to host"; perror(""); cout<<"\n"; }
    return connect_ret;
}


string Client::get_ip_from_sa(struct sockaddr_in *sa){
  char ip[INET_ADDRSTRLEN];
  inet_ntop(sa->sin_family, &(sa->sin_addr), ip, INET_ADDRSTRLEN);
  string ret(ip);
  return ret;
}

struct peer_info Client::get_peer(string ip){
  struct peer_info peer_hit;
  for(int i =0; i<peers.size(); i++){
    peer_hit = peers.at(i);
    if(ip.compare(peer_hit.ip) == 0 ) return peer_hit;
  }
  return peer_hit;
}

struct peer_info* Client::get_peer_ptr(string ip){
  for(int i =0; i<peers.size(); i++){
    struct peer_info* pi = &(peers.at(i));
    if(ip.compare(pi->ip) == 0 ) return pi;
  }
  return NULL;
}

bool Client::is_peer_in_list(string ip){
  for(int i=0; i<peers.size(); i++) if(peers.at(i).ip.compare(ip) == 0)  return true;
  return false;
}
bool Client::is_blocked(string ip){
  
  for(int i=0; i<blocked_hosts.size(); i++){
    if(blocked_hosts.at(i).compare(ip)==0) return true;
  }
  return false;
}

void Client::remove_from_blocked_hosts(string ip){

  for(int i=0; i<blocked_hosts.size(); i++){
    if(blocked_hosts.at(i).compare(ip)==0) { blocked_hosts.erase(blocked_hosts.begin()+i); return;}
  }

}

int Client::send_cmds_to_server(){
    int send_ret=0;
    string msg;
    int cmds_size = cmds.size();
    for(uint i = 0; i< cmds_size; i++){
        msg += cmds.at(i);
        if(i != cmds_size -1) msg+=" "; // we don't want to add a space at the end
    }

    if(cmds.at(0).compare(BLOCK) == 0) blocked_hosts.push_back(cmds.at(1));
    if(cmds.at(0).compare(UNBLOCK) == 0) remove_from_blocked_hosts(cmds.at(1));
    
    if(loggedin) 
    send_ret = custom_send(my_socket, msg);
    if(send_ret ==  -1) { perror("failed to send message to remote host msg: '"); cout<<msg<<"' \n"; }
    return send_ret;
}

void Client::cmd_list(){
  sort(peers.begin(), peers.end(), sort_peers_by_port);
  cmd_success_start(LIST);
  for(int i =0; i<peers.size(); i++){
    struct peer_info peer = peers.at(i);
    
    cse4589_print_and_log("%-5d%-35s%-20s%-8d\n", i+1, (peer.hostname).c_str(), (peer.ip).c_str(), peer.port);
  }
  cmd_end(LIST);

}
void Client::cmd_login(string host_ip, int port){
  int ret = connect_to_host(host_ip, port);
  if(ret != 0) {cmd_error(LOGIN); return;}
  loggedin = true;
  first_time_login = false;
  cmd_success_start(LOGIN);
  cmd_end(LOGIN); 
}


void Client::cmd_logout(){
  if(!loggedin) {cout<<"Already logged out\n"; cmd_error(LOGOUT);  return;}
  custom_send(my_socket,"LOGOUT ");

  loggedin = false;
  cout<< "Logged out from host '"<<  get_ip_from_sa(server_saddr)<<"'\n";
  
  cmd_success_start(LOGOUT);
  cmd_end(LOGOUT);
}
void Client::cmd_exit(){
  int ret = close(my_socket);
  if(ret !=0) {cmd_error(EXIT); return;}
  FD_CLR(my_socket, &master_fds);
  max_socket =0;
  cmd_success_start(EXIT);
  cmd_end(EXIT);
  loggedin = false;
  exit_program = true;
}


void Client::serv_res_refresh(string data){
  struct peer_info peer;
  string token, hostname, ip, port;
  stringstream data_ss(data);
  getline(data_ss, token, ':');
  
  if (token.compare(REFRESH_START) == 0){ 
    peers.erase(peers.begin(), peers.end());
  }else {
    getline(data_ss, token, ',');
    peer.hostname = token;
    
    getline(data_ss, token, ',');
    peer.ip = token;
    
    getline(data_ss, token);
    peer.port = atoi(token.c_str());

    if(!is_peer_in_list(peer.ip)) peers.push_back(peer);
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

int Client::custom_send(int socket, string msg){
  int send_ret; 
  uint32_t msg_length = htonl(msg.size());
  
  send_ret= send(socket, &msg_length, sizeof(uint32_t), 0);
  if(send_ret == -1) perror("ERROR: Faliled to send the data length ");

  cout<< "sending '"<<msg<<"' of size "<<msg.size()<<"\n";

  send_ret = send(socket, msg.c_str(), msg.size(), 0);
  if(send_ret == -1) perror("ERROR: Faliled to send the data ");

  return send_ret;

}

int Client::custom_recv(int socket, string &buffer ){
  int recv_ret = 0;
  uint32_t dataLength;
  vector<char> buff; 

  recv_ret = recv(socket, &dataLength, sizeof(uint32_t), 0);
  if(recv_ret == -1) perror("Error: Failed to get the data length from host ");
  dataLength = ntohl(dataLength);

  buff.resize(dataLength+1, '\0');
  recv_ret = recv(socket, &buff[0], dataLength, 0);
  if(recv_ret == -1) perror("Error: Failed to get the data from host ");

  buffer.append(buff.begin(), buff.end());

  cout<< "Got '"<<buffer<<"'\n";

  return recv_ret;
}



                        /* DEBUG */

void Client::debug_dump() {
  cout<<endl;
  cout<<"hostname: " <<my_hostname<<endl;
  cout<<"ip: " <<my_ip<<endl;
  cout<<"port: " <<server_port<<endl;
  cout<<"my_socket: " <<my_socket<<endl;
  cout<<"max_socket: " <<max_socket<<endl;

  cout<<endl;
  cout<<"Available Commands:\n";
  cout<<"AUTHOR\tIP\tPORT\n";
  cout<<"LIST\tLOGIN\tLOGOUT\n";
  cout<<"SEND\tBROADCAST\tREFRESH\n";
  cout<<"BLOCK\tUNBLOCK\tEXIT\n";



  //if(conn_socks.size() ==0) cout<<"No connected hosts\n";
  //for (int i =0; i<conn_socks.size(); i++) cout << "socket: " <<conn_socks.at(i)<<endl;
  cout<<endl;
}