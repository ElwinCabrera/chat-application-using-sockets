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
#include "../include/server.h"
#include "../include/shell.h"

#include "../include/global.h"
#include "../include/logger.h"

using std::cout;
using std::cin;
using std::endl;
using std::string;
using std::vector;
using std::to_string;
using std::stringstream;

Server::Server(string port){
  cout<< "DEBUG: class Server, constructor called"<<endl;
  cout<< "started as server"<<endl;
  portnum = atoi(port.c_str());
  gethostname(serv_hostname, sizeof(serv_hostname));
  populate_addr(serv_hostname);
  sa_in = (struct sockaddr_in*) ai_results->ai_addr;
  //void *addr = &(sa_in->sin_addr);
  inet_ntop(ai_results->ai_family, &(sa_in->sin_addr), serv_ip, sizeof(serv_ip));
  get_socket_and_bind();
  FD_ZERO(&master_fds);
  FD_ZERO(&read_fds);
}
Server::~Server()  {
  cout<< "DEBUG: class Server, DEstructor called"<<endl;
  if(ai_results) freeaddrinfo(ai_results);
  close(listen_socket);
}

void Server::start_server(){

	string cmd;
  string token;
	while(cmd != "EXIT"){
    cout << "> ";
    cmds.erase(cmds.begin(),cmds.end());
		cin >> cmd;
    stringstream cmd_ss(cmd);
    while (getline(cmd_ss, token, ' ')) cmds.push_back(token);
    shell_execute(cmds);
	}
  debug_dump();

}

int Server::populate_addr(string hostname_or_ip){
  int error_num = 0;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  error_num = getaddrinfo(hostname_or_ip.c_str(), to_string(portnum).c_str(), &hints, &ai_results);
  if (error_num !=0) cout<< "getaddrinfo: "<< gai_strerror(error_num) <<endl;
  return error_num;

}

string Server::get_hostname(){ return serv_hostname;}
string Server::get_ip() {return serv_ip;}

int Server::get_socket_and_bind(){
  int error_num =0;
  listen_socket = socket(ai_results->ai_family, ai_results->ai_socktype, 0/*ai_results->ai_protocol*/);
  int bind_err = bind(listen_socket,  ai_results->ai_addr, ai_results->ai_addrlen);
  if(listen_socket < 0) cout << "Could not create socket\n";
  if(bind_err < 0) cout << "Could not bind socket\n";
  if(listen_socket < 0 || bind_err < 0) {cout << "Quiting...goodbye\n"; error_num = -1;}
  return error_num;
}

void Server::debug_dump(){
  cout<<"hostname: " <<serv_hostname<<endl;
  cout<<"ip: " <<serv_ip<<endl;
  cout<<"port: " <<portnum<<endl;
  cout<<"listen_socket: " <<listen_socket<<endl;
}
/*void Server::shell_execute(){

}

void Shell::cmd_statistics(){

}
void Shell::cmd_blocked(){

}*/
