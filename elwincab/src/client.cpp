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
    char hname[HOSTNAME_LEN];
    gethostname(hname, sizeof(hname));
    my_hostname = String(hname);
    server_port = atoi(serv_port.c_str());
    my_saddr = populate_addr(my_hostname, serv_port);
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

}

void Clinet::start_clinent(){

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
          handle_shell_cmds();

        } else if(i == my_socket){ // ready to receve data from remote host
          //handle_new_conn_request();

        } else { 
          //recv_data_from_conn_sock(i);

        }
      }
    }
	}

}

void handle_shell_cmds(){
    cmds.erase(cmds.begin(),cmds.end());
    string cmd, token;
    cin >> cmd;
    stringstream cmd_ss(cmd);
    while (getline(cmd_ss, token, ' ')) cmds.push_back(token); 

    if (cmds.at(0).compare(AUTHOR) == 0) cmd_author();
    if (cmds.at(0).compare(IP) == 0) cmd_ip(my_ip);
    if (cmds.at(0).compare(PORT) == 0) cmd_port(serv_port);
    if (cmds.at(0).compare(LIST) == 0) cmd_list();

    if(cmds.at(0).compare(LOGIN) == 0) ;
    //I think I can get away with just sending the whole string to the server and letting the server handle it.
    /*if(cmds.at(0).compare(REFRESH) == 0) ;
    if(cmds.at(0).compare(SEND) == 0);
    if(cmds.at(0).compare(BROADCAST) == 0);
    if(cmds.at(0).compare(BLOCK) == 0);
    if(cmds.at(0).compare(UNBLOCK) == 0);
    if(cmds.at(0).compare(LOGOUT) == 0);*/
    if(cmds.at(0).compare(EXIT) == 0) exit_program = true;
}

struct addrinfo& Client::populate_addr(string hname_or_ip, int port){
  struct addrinfo *ai;
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  error_num = getaddrinfo(hname_or_ip.c_str(), to_string(port).c_str(), &hints, &ai);
  if (error_num !=0) cout<< "getaddrinfo: "<< gai_strerror(error_num) <<endl;
  return ai;

}

void Clinet::connect_to_host(string server_ip, int port){
    my_socket = socket(my_saddr->sin_family, SOCK_STREAM, 0);
    if(my_socket == -1) cout << "failed to create socket, error#"<<errno<<endl;

    server_saddr = populate_addr(server_ip, port);
    int connect_ret = connect(my_socket, (struct sockaddr*) server_saddr, sizeof(server_saddr));
    if(connect_ret ==  -1) cout << "failed to connect to remote host, error#"<<errno<<endl;
}


string Client::get_ip_from_sa(struct sockaddr_in *sa){
  char ip[INET_ADDRSTRLEN];
  inet_ntop(sa->sin_family, (struct sockaddr*) sa, ip, sizeof(ip));
  string ret(ip);
  return ret;
}


void cmd_login(){

}

/*void cmd_refresh(){

}

void cmd_send(){
}

void cmd_broadcast(){

}

void cmd_block(){

}
void cmd_unblock(){

}

void cmd_logout(){

}*/