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
  

  gethostname(my_hostname, sizeof(my_hostname));

  my_saddr = populate_addr(my_hostname, port);
  my_ip = get_sa_ip(my_saddr);

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
  //if(ai) freeaddrinfo(ai);
  //if(sa) free(sa);
  close(listen_socket);
  //free all elements in conn_his
  //for (int i =0; i<conn_socks.size(); i++) cout << "socket: " <<conn_socks.at(i)<<endl;
}

/*at this point we are ready to start our server services and we can start listening.*/
void Server::start_server(){

  debug_dump();

  
  listen(listen_socket, 5);

  cout<<"> ";
  while(true){
    
    memcpy(&read_fds, &master_fds, sizeof(master_fds));

    int select_res = select(max_socket +1, &read_fds, NULL, NULL, NULL);
    if(select_res <0) cout<< "Select failed\n";

    for (int i = 0; i <=max_socket; i++) {

      if(FD_ISSET(i, &read_fds)) {  // true = at least one socket is ready to be read
        //find the socket that is ready
        if(i == STDIN){ // 0 = stdin
          //cout<<"> ";
    
          string cmd;
          getline(cin, cmd);

          handle_shell_cmds(cmd);
          cout<<"> ";
          fflush(stdout);

        } else if(i == listen_socket){ // true = a client is trying to connect to us, shall we choose to accept
          handle_new_conn_request();
          fflush(stdout);

        } else { // if its not a new connection or stdin then it must be an existing connection trying to communicate witn us (we got some data)
          recv_data_from_conn_sock(i);
          fflush(stdout);
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
struct sockaddr_in* Server::populate_addr(string hname_or_ip, string port){
  struct addrinfo hints;
  struct addrinfo *ai;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  int gai_ret = getaddrinfo(hname_or_ip.c_str(), port.c_str(), &hints, &ai);
  if (gai_ret !=0) cout<< "getaddrinfo: "<< gai_strerror(gai_ret) <<endl;
  return (struct sockaddr_in*) ai->ai_addr;

}



/*
Creates a socket and binds if either fails then it will return -1
if it succeeds then it will return 0
*/
int Server::sock_and_bind(){
  int error_num =0;

  listen_socket = socket(AF_INET, SOCK_STREAM, 0);
  if(listen_socket < 0) perror("ERROR: Could not create socket\n");

  int bind_ret = bind(listen_socket, (struct sockaddr*) my_saddr, sizeof(struct sockaddr_in));
  if(bind_ret == -1) perror("ERROR: Could not bind ip and socket to port\n");

  if(listen_socket == -1 || bind_ret == -1) {/*cout << "Quiting...goodbye\n";*/ error_num = -1;}
  return error_num;
}
/*
Takes input from stdin and parses them using space as a delimeter then puts
each parsed token in a vector (vector size should not be greater than 3, in most cases)
*/
void Server::handle_shell_cmds(string cmd){
  cmds.erase(cmds.begin(),cmds.end());
  string  token;
  stringstream cmd_ss(cmd);
  while (getline(cmd_ss, token, ' ')) cmds.push_back(token);


  if(cmds.at(0).compare(AUTHOR) == 0) cmd_author();
  if(cmds.at(0).compare(IP) == 0) cmd_ip(my_ip);
  if(cmds.at(0).compare(PORT) == 0) cmd_port(portnum);
  if(cmds.at(0).compare(LIST) == 0) cmd_list();
  if(cmds.at(0).compare(STATISTICS) == 0) cmd_statistics();
  if(cmds.at(0).compare(BLOCKED) == 0) {
    if(cmds.size() != 2 || !is_valid_ip(cmds.at(1)) || !host_in_history(cmds.at(1))) { cmd_error(BLOCKED); return;}
    cmd_blocked(cmds.at(1));
  }
}

/*if this function returns a number other than 0 then the new connection could not be established*/
int Server::handle_new_conn_request(){
  int new_conn_sock;
  struct sockaddr_in cli_addr;
  struct remotehos_info new_host;

  socklen_t addrlen = sizeof(cli_addr);
  new_conn_sock = accept(listen_socket, (struct sockaddr*) &cli_addr, &addrlen);

  if(new_conn_sock == -1) {
    perror("ERROR: Could not connect host, accept failed\n");
    return new_conn_sock;
  }
  string ip = get_sa_ip(&cli_addr);
  cout<< "Remote client '" <<ip<<"' successfuly connected\n";
  
  
  

  if(new_conn_sock > max_socket) max_socket = new_conn_sock;
  FD_SET(new_conn_sock, &master_fds);

  if(!host_in_history(ip)){
    new_host.msg_bytes_tx =0;
    new_host.msg_bytes_rx=0;
    new_host.loggedin = true;
    new_host.sa = &cli_addr;
    new_host.sock = new_conn_sock;
    new_host.hostname = "NULL";
    new_host.ip = ip;
    new_host.port=ntohs(cli_addr.sin_port);
    conn_his.push_back(new_host);
    cout<< "New client IP:"<<ip<<endl;
  } 
  send_current_client_list(new_host);
  return new_conn_sock;
}

void Server::check_and_send_stored_msgs(string dest_ip){
  struct remotehos_info *host = get_host_ptr(dest_ip);
  vector<string> stored_msg_from_ips = host->stored_msg_from_ips;
  vector<string> stored_msgs = host->stored_msgs;

  for(int i=0; i<stored_msgs.size(); i++){
    string from_ip = stored_msg_from_ips.at(i);
    string msg = stored_msgs.at(i);
    relay_msg_to(from_ip, dest_ip,  msg);
    host->stored_msg_from_ips.erase(host->stored_msg_from_ips.begin()+i);
    host->stored_msgs.erase(host->stored_msgs.begin()+i);
  }
}

/*if this function returns -1 then socket is not in the list of conn_socks (defined as vector)*/
int Server::recv_data_from_conn_sock(int idx_socket){
  int conn_sock = 0;
  string data;
 
  int bytes_recvd = custom_recv(idx_socket, data);
  struct remotehos_info host = get_host(idx_socket); 
  

  //check if data is zero
  if(bytes_recvd != 0 && host.loggedin){
    client_cmds.erase(client_cmds.begin(),client_cmds.end());
   
    stringstream data_ss(data);
    string cmd, token;
    
    getline(data_ss, token, ' ');
    client_cmds.push_back(token);

    if(token.compare(BROADCAST) == 0) {
      getline(data_ss, token);
      client_cmds.push_back(token);
      relay_msg_to_all(host.ip,token);
    }
      
    if(token.compare(BLOCK) == 0 ) {
      getline(data_ss, token);
      client_cmds.push_back(token);
      block(host, token);  
    }
    if(token.compare(UNBLOCK) == 0) {
      getline(data_ss, token);
      client_cmds.push_back(token);
      unblock(host.ip, token);
    } 
    if(token.compare(SEND) == 0) {
      string to_ip, msg;
      getline(data_ss, to_ip, ' ');
      getline(data_ss, msg);
      client_cmds.push_back(to_ip);
      client_cmds.push_back(msg);
      relay_msg_to(host.ip, to_ip, msg);
    } 

    if(token.compare(HOSTNAME) == 0) {
      getline(data_ss, token);
      client_cmds.push_back(token);
      get_host_ptr(host.ip)->hostname = token;
    }
    if(token.compare(PORT) == 0) {
      getline(data_ss, token);
      get_host_ptr(host.ip)->port = atoi(token.c_str()); 
    }

    if(token.compare(LOGGEDIN) == 0) {
      get_host_ptr(host.ip)->loggedin = true;
      cout<<"Client "<<host.ip<<" logged back in\n";
      check_and_send_stored_msgs(host.ip);  
    }

    if(token.compare(LOGOUT) == 0) get_host_ptr(host.ip)->loggedin = false; 
    if(token.compare(REFRESH) == 0) send_current_client_list(host);
  } else {
    //close connection
    close_remote_conn(host.sock);

  }
  
  return conn_sock;
}



                            /* Operations that we may be asked by a remote host*/

void Server::block(struct remotehos_info host, string new_block_ip){
  struct remotehos_info remote_host = get_host(new_block_ip);
  get_host_ptr(host.ip)->blocked_hosts.push_back(remote_host);
  send_end_cmd(host.sock, BLOCK_END, host.ip);
}

void Server::unblock(string src_ip, string unblock_ip){
  struct remotehos_info *host = get_host_ptr(src_ip);

  for(int i=0; i< get_host_ptr(src_ip)->blocked_hosts.size(); i++){
    if(get_host_ptr(src_ip)->blocked_hosts.at(i).ip.compare(unblock_ip) == 0){
      get_host_ptr(src_ip)->blocked_hosts.erase(get_host_ptr(src_ip)->blocked_hosts.begin()+i);
      return;
    }
  }
}


void Server::relay_msg_to_all(string src_ip, string msg){
  for(int i=0; i<conn_his.size(); i++ ){
    relay_msg_to(src_ip, conn_his.at(i).ip, msg);
  }
}

void Server::relay_msg_to(string src_ip, string dest_ip, string msg){
  //check if the destined client has not blocked this client if so then dont do anything
  struct remotehos_info dest_host = get_host(dest_ip);
  if(dest_ip_blocking_src_ip(src_ip, dest_ip) || dest_ip.compare(src_ip) ==0 ) return;
  if( !is_valid_ip(dest_ip)) {cout<<"not valid ip\n"; return; }

  int bytes_sent = 0;
  //check if the destined client is logged in if not then buffer the messege
  if(dest_host.loggedin) {
    
    string formatted_msg = "RELAYED:"+src_ip+","+msg;
    
    bytes_sent = custom_send(dest_host.sock, formatted_msg);
    if(bytes_sent == -1) { perror("ERROR: In sending to '"); cout<< dest_ip<<"'\n"; return;}
    
    int send_end_ret = 0;
    if(client_cmds.at(0).compare(BROADCAST) == 0) send_end_cmd(dest_host.sock, BROADCAST_END, dest_ip);
    if(client_cmds.at(0).compare(SEND) == 0) send_end_cmd(dest_host.sock, SEND_END, dest_ip);

    
    get_host_ptr(dest_ip)->msg_bytes_rx += msg.size();
    event_msg_relayed(src_ip, dest_ip, msg);

  } else {
    struct remotehos_info *h = get_host_ptr(dest_ip);
    h->stored_msg_from_ips.push_back(src_ip);
    h->stored_msgs.push_back(msg);

    } 
  
  get_host_ptr(src_ip)->msg_bytes_tx += msg.size();
  
}




void Server::send_current_client_list(struct remotehos_info host){
  sort(conn_his.begin(), conn_his.end(), sort_hosts_by_port);
  int send_ret =0;

  //send_ret = send(host.sock, REFRESH_START,sizeof(REFRESH_START),0);
  send_ret = custom_send(host.sock, REFRESH_START);
  if(send_ret == -1) { perror("ERROR: In sending start 'REFRESH_START' msg to "); cout<<host.ip<<endl; return; }

  for(int i=0; i< conn_his.size(); i++){
    struct remotehos_info h = conn_his.at(i);
    if(!h.loggedin || host.ip.compare(h.ip) ==0 ) continue;

  
    string send_data = "REFRESH:"+h.hostname+","+h.ip+","+itos(h.port);

    //send_ret = send(host.sock, send_data.c_str(),sizeof(send_data),0);
    send_ret = custom_send(host.sock, send_data);
    if(send_ret == -1) { perror("ERROR: In sending 'REFRESH' msg to '"); cout<< host.ip<<"' of entry#"<<i<<endl;  return; }
  }
  send_end_cmd(host.sock, REFRESH_END, host.ip);


}


int Server::send_end_cmd(int socket, string end_cmd_sig, string to_ip){
  //int end_sig_ret = send(socket, end_cmd_sig.c_str(),sizeof(end_cmd_sig),0);
  int end_sig_ret = custom_send(socket, end_cmd_sig);
  if(end_sig_ret == -1) { perror("ERROR: In sending end signal msg to '"); cout<<to_ip<< "' , '"<< end_cmd_sig<<"' \n"; }
  return end_sig_ret;
}

int Server::close_remote_conn(int socket){

  for(uint i = 0; i<conn_his.size(); i++){
    if(conn_his.at(i).sock == socket) conn_his.erase(conn_his.begin()+i);
  }
  return clear_and_close_sock(socket);
}

int Server::logout_host(int socket){
  get_host_ptr(socket)->loggedin = false;
  return clear_and_close_sock(socket);
}

int Server::clear_and_close_sock(int socket){
  //remove from master fd list and pick a new maxsocket
  int success = 0;
  FD_CLR(socket, &master_fds);
  max_socket =0 ;
  for(int i =0; i<conn_his.size(); i++){ if(conn_his.at(i).sock > max_socket) max_socket = conn_his.at(i).sock;  }

  success = close(socket);
  if(success != 0) { perror("ERROR: Failed to close socket#"); cout<<socket<<"\n"; }
  return success; 

}




                                          /* HELPER AND GETTER FUNCTIONS */




//if host is in the connection history (conn_his) that means that host is either logged in or logged out but has NOT exited the program
bool Server::host_in_history(string ip){
  string test_ip;
  for (uint i = 0; i < conn_his.size(); i++) {
    if(conn_his.at(i).ip.compare(ip) ==0) return true;
  }
  return false;
}

bool Server::dest_ip_blocking_src_ip(string src_ip, string dest_ip){
  struct remotehos_info dest_host = get_host(dest_ip);
  //vector<struct remotehos_info> blocked_list = dest_host.blocked_hosts;

  int list_size = dest_host.blocked_hosts.size();
  for(uint i = 0; i< list_size; i++){
    string blocked_ip = dest_host.blocked_hosts.at(i).ip;
    if(src_ip.compare(blocked_ip)== 0 ) return true;
  }
  return false;
}

string Server::get_sa_ip(struct sockaddr_in *sa){
  char ip[INET_ADDRSTRLEN];
  inet_ntop(sa->sin_family, &(sa->sin_addr), ip, INET_ADDRSTRLEN);
  string ret(ip);
  return ret;
}

struct remotehos_info* Server::get_host_ptr(string ip){
  int his_size = conn_his.size();
  struct remotehos_info *host;
  for (uint i = 0; i < his_size; i++) {
    host = &(conn_his.at(i));
    if(ip.compare(host->ip) == 0) return host;
  }
  return NULL;
}

struct remotehos_info Server::get_host(string ip){
  int his_size = conn_his.size();
  struct remotehos_info host;
  for (uint i = 0; i < his_size; i++) {
    host = conn_his.at(i);
    if(ip.compare(host.ip) == 0) return host;
  }
  return host;
}


struct remotehos_info* Server::get_host_ptr(int sock){
  int his_size = conn_his.size();
  struct remotehos_info *host;
  for (uint i = 0; i < his_size; i++) {
    host = &(conn_his.at(i));
    if(host->sock == sock) return host;
  }
  return NULL;
}

struct remotehos_info Server::get_host(int sock){
  int his_size = conn_his.size();
  struct remotehos_info host;
  for (uint i = 0; i < his_size; i++) {
    host = conn_his.at(i);
    if(host.sock == sock) return host;
  }
  return host;
}


int Server::custom_send(int socket, string msg){
  int send_ret = 0; 
  //msg += '\0';
  uint32_t msg_length = htonl(msg.size());
  
  send_ret= send(socket, &msg_length, sizeof(uint32_t), 0);
  if(send_ret == -1) perror("ERROR: Faliled to send the data length ");

  cout<< "sending '"<<msg<<"' of size "<<msg.size()<<"\n";
 
  send_ret = send(socket, msg.c_str(), msg.size(), 0);
  if(send_ret == -1) perror("ERROR: Faliled to send the data ");

  return send_ret;

}

int Server::custom_recv(int socket, string &buffer ){
  int recv_ret = 0;
  uint32_t dataLength;
  vector<char> buff; 

  recv_ret = recv(socket, &dataLength, sizeof(uint32_t), 0);
  if(recv_ret == -1) perror("Error: Failed to get the data length from host ");
  dataLength = ntohl(dataLength);
  
  buff.resize(dataLength, '\0');
  recv_ret = recv(socket, &(buff[0]), dataLength, 0);
  if(recv_ret == -1) perror("Error: Failed to get the data from host ");

  buffer.append(buff.begin(), buff.end());

  cout<< "Got '"<<buffer<<"' of size " << dataLength <<" from " << get_host(socket).ip<<"\n";

  return recv_ret;
}
   

                                  /* SHELL CMDs */


void Server::cmd_list(){ //get list of logged in hosts sorted by port number

  cmd_success_start("LIST");
  if(!conn_his.empty()) sort(conn_his.begin(), conn_his.end(), sort_hosts_by_port);
  
  int count =1;
  for(int i =0; i<conn_his.size(); i++){
    struct remotehos_info h = conn_his.at(i);
    if(!h.loggedin) continue;

    
    cse4589_print_and_log("%-5d%-35s%-20s%-8d\n", count, (h.hostname).c_str(), h.ip.c_str(), h.port);
    count++;
  }
  cmd_end("LIST");
}

void Server::cmd_statistics(){
  cmd_success_start("STATISTICS");
  if(!conn_his.empty()) sort(conn_his.begin(), conn_his.end(), sort_hosts_by_port);
  
  string login_status;
  
  for(int i =0; i<conn_his.size(); i++){
    struct remotehos_info h = conn_his.at(i);
    
    if(h.loggedin) 
      login_status = "logged-in";
    else
      login_status = "logged-out";

    cse4589_print_and_log("%-5d%-35s%-8d%-8d%-8s\n", i+1, (h.hostname).c_str(), h.msg_bytes_tx, h.msg_bytes_rx, login_status.c_str());
  }
  cmd_end("STATISTICS");

}

void Server::cmd_blocked(string ip){
  //The output should display the hostname, IP address, and the listening port numbers of the bloked clents
  struct remotehos_info host = get_host(ip);
  cmd_success_start("BLOCKED");
  
  if(host.blocked_hosts.empty()) cout<< ip<<" has not blocked anyone\n"; 

  sort(host.blocked_hosts.begin(), host.blocked_hosts.end(), sort_hosts_by_port);
  
  
  for(int i =0; i< host.blocked_hosts.size(); i++){
    //struct remotehos_info h = host.blocked_hosts.at(i);
  
    cse4589_print_and_log("%-5d%-35s%-20s%-8d\n", i+1, (host.blocked_hosts.at(i).hostname).c_str(), (host.blocked_hosts.at(i).ip).c_str(), host.blocked_hosts.at(i).port);
  }
  cmd_end("BLOCKED");
  

}


/*Just some helpful debug information */
void Server::debug_dump(){
  cout<<endl;
  cout<<"hostname: " <<my_hostname<<endl;
  cout<<"ip: " <<my_ip<<endl;
  cout<<"port: " <<portnum<<endl;
  cout<<"listen_socket: " <<listen_socket<<endl;
  cout<<"max_socket: " <<max_socket<<endl;

  cout<<endl;
  cout<<"Available commands:\n";
  cout<<"AUTHOR\tIP\tPORT\n";
  cout<<"LIST\tSTATISTICS\tBLOCKED\n";
  
  cout<<endl;
}
