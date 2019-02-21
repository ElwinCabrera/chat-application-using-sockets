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

  sa = (struct sockaddr_in*) ai->ai_addr;
  inet_ntop(ai->ai_family, &(sa->sin_addr), serv_ip, sizeof(serv_ip));

  sock_and_bind();
  max_socket = listen_socket;

  FD_ZERO(&master_fds);
  FD_ZERO(&read_fds);

}
Server::~Server()  {
  cout<< "DEBUG: class Server, DEstructor called"<<endl;
  if(ai) freeaddrinfo(ai);
  close(listen_socket);
  //for (int i =0; i<conn_socks.size(); i++) cout << "socket: " <<conn_socks.at(i)<<endl;
}

void Server::start_server(){

  debug_dump();


  listen(listen_socket, 5);

  cout<<"> ";
  FD_SET(STDIN, &master_fds);
  FD_SET(listen_socket, &master_fds);

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

        } else if(i == listen_socket){ // true a client is trying to connect to us, accept

        } else { // if its not a new connection or stdin then it must be an existing connection trying to communicate witn us(we got some data)


        }
      }
    }
	}




}

int Server::populate_addr(string hostname_or_ip){
  int error_num = 0;
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  error_num = getaddrinfo(hostname_or_ip.c_str(), to_string(portnum).c_str(), &hints, &ai);
  if (error_num !=0) cout<< "getaddrinfo: "<< gai_strerror(error_num) <<endl;
  return error_num;

}

string Server::get_hostname(){ return serv_hostname;}
string Server::get_ip() {return serv_ip;}

int Server::sock_and_bind(){
  int error_num =0;

  listen_socket = socket(ai->ai_family, ai->ai_socktype, 0/*ai->ai_protocol*/);
  if(listen_socket < 0) cout << "Could not create socket, error#"<<errno<<endl;

  int bind_ret = bind(listen_socket, (struct sockaddr*) sa, sizeof(struct sockaddr_in));
  if(bind_ret == -1) cout << "Could not bind ip and socket to port, error#"<<errno<<endl;

  if(listen_socket == -1 || bind_ret == -1) {/*cout << "Quiting...goodbye\n";*/ error_num = -1;}
  return error_num;
}

void Server::handle_shell_cmds(){
  cmds.erase(cmds.begin(),cmds.end());
  string cmd;
  string token;
  cin >> cmd;
  stringstream cmd_ss(cmd);
  while (getline(cmd_ss, token, ' ')) {cmds.push_back(token);}
  shell_execute(cmds);
  //cout << "> ";
}

int Server::handle_new_conn_request(){
  int new_conn_sock;
  struct sockaddr_storage cli_addr;
  socklen_t addrlen = sizeof(cli_addr);
  new_conn_sock = accept(listen_socket, (struct sockaddr*) &cli_addr, &addrlen);
  if(new_conn_sock < 0)
    cout<< "Could not connect host, acceop failed\n";
  else
    cout<< "Remote client connected\n";

  FD_SET(new_conn_sock, &master_fds);
  if(new_conn_sock > max_socket) max_socket = new_conn_sock;
  conn_socks.push_back(new_conn_sock);
  conn_cli.push_back(cli_addr);
  return new_conn_sock;
}

void Server::debug_dump(){
  cout<<endl;
  cout<<"hostname: " <<serv_hostname<<endl;
  cout<<"ip: " <<serv_ip<<endl;
  cout<<"port: " <<portnum<<endl;
  cout<<"listen_socket: " <<listen_socket<<endl;
  cout<<"max_socket: " <<max_socket<<endl;
  cout<<"printing a list of connected sockets\n";

  //for (int i =0; i<conn_socks.size(); i++) cout << "socket: " <<conn_socks.at(i)<<endl;

  cout<<endl;
}
