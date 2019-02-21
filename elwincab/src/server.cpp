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

  gethostname(serv_hostname, sizeof(serv_hostname));
  populate_addr(serv_hostname);

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

        } else { // if its not a new connection or stdin then it must be an existing connection trying to communicate witn us (we got some data)


        }
      }
    }
	}




}


/*you can imput a hostname, url or an ip address of a remote host and it will
get all information such as IP, socket tyoe, protocal ect..
all this information is stored in a struct addrinfo *ai
we will most likely use this to get our own information for later use
if error occurs then it will retrn non-zero*/
int Server::populate_addr(string hname_or_ip){
  int error_num = 0;
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  error_num = getaddrinfo(hname_or_ip.c_str(), to_string(portnum).c_str(), &hints, &ai);
  if (error_num !=0) cout<< "getaddrinfo: "<< gai_strerror(error_num) <<endl;
  return error_num;

}

string Server::get_hostname(){ return serv_hostname;}
string Server::get_ip() {return serv_ip;}

/*
Creates a socket and binds if either fails then it will return -1
if it succeeds then it will return 0
*/
int Server::sock_and_bind(){
  int error_num =0;

  listen_socket = socket(ai->ai_family, ai->ai_socktype, 0/*ai->ai_protocol*/);
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
  struct sockaddr_storage cli_addr;
  socklen_t addrlen = sizeof(cli_addr);
  new_conn_sock = accept(listen_socket, (struct sockaddr*) &cli_addr, &addrlen);

  if(new_conn_sock > 0) {
    cout<< "Remote client connected\n";

    FD_SET(new_conn_sock, &master_fds);
    if(new_conn_sock > max_socket) max_socket = new_conn_sock;
    conn_socks.push_back(new_conn_sock);
    conn_cli.push_back(cli_addr);
  } else {
    cout<< "Could not connect host, acceop failed, error#"<<errno<<endl;
  }
  return new_conn_sock;
}

/*if this function returns -1 then socket is not in the list of conn_socks (defined as vector)*/
int Server::recv_data_from_conn_sock(int idx_socket){
  int conn_sock = -1;
  for(int i = 0; i < conn_socks.size(); i++) {
    int tmp_sock = conn_socks.at(i);
      if(idx_socket == tmp_sock) { conn_sock = tmp_sock; break;}
  }
  //receve data form connected socket here
  return conn_sock;
}

int Server::close_remote_conn(int socket){
  int success = 0;
  return success;
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
  if(conn_socks.size() ==0) cout<<"No connected hosts\n";
  for (int i =0; i<conn_socks.size(); i++) cout << "socket: " <<conn_socks.at(i)<<endl;

  cout<<endl;
}
