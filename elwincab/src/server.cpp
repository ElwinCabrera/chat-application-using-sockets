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
  

  gethostname(my_hostname, sizeof(my_hostname));

  my_saddr = populate_addr(my_hostname, portnum);
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
          //cout<<"> ";
    
          string cmd;
          getline(cin, cmd);

          handle_shell_cmds(cmd);
          cout<<"> ";

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
struct sockaddr_in* Server::populate_addr(string hname_or_ip, int port){
  struct addrinfo hints;
  struct addrinfo *ai;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  int gai_ret = getaddrinfo(hname_or_ip.c_str(), to_string(port).c_str(), &hints, &ai);
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
  if(cmds.at(0).compare(BLOCKED) == 0) cmd_blocked(cmds.at(1));
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
  
  check_and_send_stored_msgs(ip);
  

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
    conn_his.push_back(new_host);
    cout<< "New client IP:"<<ip<<endl;
  } else {
    cout<<"Client "<<ip<<" logged back in\n";
    get_host_ptr(ip)->loggedin = true;
  }
  send_current_client_list(new_host);
  return new_conn_sock;
}

void Server::check_and_send_stored_msgs(string dest_ip){
  if(stored_msgs.empty()) return;

  it = stored_msgs.begin();

  it = stored_msgs.find(dest_ip);

  if(it  != stored_msgs.end()){
    cout<< "You got some new messages while you were away"<<endl;
    vector<pair<string, string>> from_msg = it->second;
    for(auto itr = from_msg.begin(); itr != from_msg.end(); itr++){
      //send_msg(itr->first, itr->second);
      //struct remotehos_info host = get_rmhi_from_ip(itr->first);
      relay_msg_to(get_host(itr->first).ip, dest_ip,  itr->second);
      cout<<"sending msg to '"<<dest_ip <<" from: "<<itr->first<<endl;
      cout<<"\tmsg: "<<itr->second<<endl;
    }
    stored_msgs.erase(it);
  }

}

/*if this function returns -1 then socket is not in the list of conn_socks (defined as vector)*/
int Server::recv_data_from_conn_sock(int idx_socket){
  int conn_sock = 0;
  char *recvd_data = (char*) malloc(sizeof(char*) * 500+1);
  memset(recvd_data, 0, 500+1);
  //receve data form connected socket here
  int bytes_recvd = recv(idx_socket, recvd_data, 500-1, 0);
  string data(recvd_data);
  struct remotehos_info host = get_host(idx_socket); // nullptr if not in conn_his 
  cout<< "got '"<<recvd_data<<"' from client '"<<host.ip<<"' \n";     //DEBUG

  //check if data is zero
  if(bytes_recvd != 0 ){
    client_cmds.erase(client_cmds.begin(),client_cmds.end());
    cout<<data<<endl; // DEBUG
    stringstream data_ss(data);
    string cmd, token;
    while (getline(data_ss, token, ' ')) {client_cmds.push_back(token);}
    
    

    if(client_cmds.at(0).compare(BROADCAST) == 0) relay_msg_to_all(host.ip,client_cmds.at(1));  // foramt the string to look like RELAYED:from_ip,msg
    if(client_cmds.at(0).compare(LOGOUT) == 0) logout(host); 
    if(client_cmds.at(0).compare(REFRESH) == 0) send_current_client_list(host);  //need implement 
    if(client_cmds.at(0).compare(BLOCK) == 0 ) block(host, client_cmds.at(1));  // if trying to block self wont work
    if(client_cmds.at(0).compare(UNBLOCK) == 0) unblock(host, client_cmds.at(1)); 
    if(client_cmds.at(0).compare(SEND) == 0) relay_msg_to(host.ip, client_cmds.at(1), client_cmds.at(2)); 

    if(client_cmds.at(0).compare(HOSTNAME) == 0) get_host_ptr(host.ip)->hostname = client_cmds.at(1);
  } else {

  }
  free(recvd_data);
  return conn_sock;
}



                            /* Operations that we may be asked by a remote host*/

void Server::block(struct remotehos_info host, string ip){
  struct remotehos_info remote_host = get_host(ip);
  host.blocked_hosts.push_back(remote_host);
  //int unblock_end_ret = send(host->sock, UNBLOCK_END,sizeof(UNBLOCK_END),0);
  //if(unblock_end_ret == -1) cout << "error sending end UNBLOCK msg to '"<< get_ip_from_sa(host->sa)<<"' error#"<<errno<< endl;
  send_end_cmd(host.sock, BLOCK_END, host.ip);
}

void Server::unblock(struct remotehos_info host, string new_block_ip){
  int blocked_size = host.blocked_hosts.size();
  for (uint i = 0; i < blocked_size; i++) {
    struct remotehos_info blocked = host.blocked_hosts.at(i);
    if(new_block_ip.compare(blocked.ip) == 0) {
      host.blocked_hosts.erase(host.blocked_hosts.begin()+ i); 
      //int unblock_end_ret = send(host->sock, UNBLOCK_END,sizeof(UNBLOCK_END),0);
      //if(unblock_end_ret == -1) cout << "error sending end UNBLOCK msg to '"<< get_ip_from_sa(host->sa)<<"' error#"<<errno<< endl;
      send_end_cmd(host.sock, UNBLOCK_END, host.ip);
      return;
    }
  }
}


void Server::relay_msg_to_all(string src_ip, string msg){
  for(auto &itr : conn_his ){
    relay_msg_to(src_ip, itr.ip, msg);
  }
}

void Server::relay_msg_to(string src_ip, string dest_ip, string msg){
  //check if the destined client has not blocked this client if so then dont do anything
  struct remotehos_info dest_host = get_host(dest_ip);
  if(dest_ip_blocking_src_ip(src_ip, dest_ip)) return;

  int bytes_sent = 0;
  //check if the destined client is logged in if not then buffer the messege
  if(dest_host.loggedin) {
    
    string formatted_msg = "RELAYED:"+src_ip+","+msg;
    bytes_sent = send(dest_host.sock, formatted_msg.c_str(),sizeof(formatted_msg),0);
    if(bytes_sent == -1) { perror("ERROR: In sending to '"); cout<< dest_ip<<"'\n"; return;}
    
    int send_end_ret = 0;
    if(client_cmds.at(0).compare(BROADCAST)) send_end_cmd(dest_host.sock, BROADCAST_END, dest_ip);
    if(client_cmds.at(0).compare(SEND)) send_end_cmd(dest_host.sock, SEND_END, dest_ip);

    //dst_host->msg_bytes_rx += sizeof(msg);
    //add_msg_bytes_rx(dest_ip, sizeof(msg));
    get_host_ptr(dest_ip)->msg_bytes_rx += sizeof(msg);
    event_msg_relayed(src_ip, dest_ip, msg);

  } else {
    
    pair<string, string> from_and_msg;
    from_and_msg.first = src_ip;
    from_and_msg.second = msg;
    it = stored_msgs.begin();
    it = stored_msgs.find(dest_ip);

    if(it == stored_msgs.end()){ //currently there are no stored messages for dst_ip
      vector<pair<string,string>> from_and_msg_vec;
      from_and_msg_vec.push_back(from_and_msg);
      stored_msgs.insert(make_pair(dest_ip, from_and_msg_vec));
    } else { // an entry exists so just 
      it->second.push_back(from_and_msg);
    }
  }
  //add_msg_bytes_tx(src_ip, bytes_sent);
  get_host_ptr(src_ip)->msg_bytes_tx += bytes_sent;
  //host->msg_bytes_tx += bytes_sent;
}


void Server::logout(struct remotehos_info host){
  int success = close(host.sock);
  if(success != 0) { perror("ERROR: Failed to close socket in logout operation, socket#"); cout<<socket<<endl; return; }
  //host->loggedin = false;
  get_host_ptr(host.ip)->loggedin = false;

  FD_CLR(host.sock, &master_fds);
  max_socket =0;
  for(auto &itr : conn_his){ if(itr.sock > max_socket) max_socket = itr.sock;  }
}

void Server::send_current_client_list(struct remotehos_info host){
  string h_hostname, h_ip, h_port;
  sort(conn_his.begin(), conn_his.end(), sort_hosts_by_port);
  int send_ret =0;

  send_ret = send(host.sock, REFRESH_START,sizeof(REFRESH_START),0);
  if(send_ret == -1) { perror("ERROR: In sending start 'REFRESH_START' msg to "); cout<<host.ip<<endl; return; }

  for(int i=0; i< conn_his.size(); i++){
    struct remotehos_info h = conn_his.at(i);
    if(!h.loggedin || host.ip.compare(h.ip) ) continue;
    h_hostname = h.hostname;
    h_ip = h.ip;
    h_port = to_string(h.port);
    string send_data = "REFRESH:"+h_hostname+","+h_ip+","+h_port;

    send_ret = send(host.sock, send_data.c_str(),sizeof(send_data),0);
    if(send_ret == -1) { perror("ERROR: In sending 'REFRESH' msg to '"); cout<< host.ip<<"' of entry#"<<i<<endl;  return; }
  }
  send_end_cmd(host.sock, REFRESH_END, host.ip);


}


int Server::send_end_cmd(int socket, string end_cmd_sig, string to_ip){
  int end_sig_ret = send(socket, end_cmd_sig.c_str(),sizeof(end_cmd_sig),0);
  if(end_sig_ret == -1) { perror("ERROR: In sending end signal msg to '"); cout<<to_ip<< "' , '"<< end_cmd_sig<<"' \n"; }
  return end_sig_ret;
}

int Server::close_remote_conn(int socket){
  int success = 0;
  string ip;

  for(uint i = 0; i<conn_his.size(); i++){
    struct remotehos_info h = conn_his.at(i);
    if(h.sock == socket){
      ip = h.ip;
      conn_his.erase(conn_his.begin()+i);
    }
  }

  it = stored_msgs.begin();
  it = stored_msgs.find(ip);
  if(it != stored_msgs.end()) stored_msgs.erase(it);

  //remove from master fd list and pick a new maxsocket
  FD_CLR(socket, &master_fds);
  max_socket =0 ;
  for(auto &itr : conn_his){ if(itr.sock > max_socket) max_socket = itr.sock;  }

  success = close(socket);
  if(success != 0) { perror("ERROR: Failed to close socket#"); cout<<socket<<"\n"; }

  return success;
}




                                          /* HELPER AND GETTER FUNCTIONS */




/*string Server::get_hostname(){ return my_hostname;}
string Server::get_ip() {return serv_ip;}*/

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
  vector<struct remotehos_info> blocked_list = dest_host.blocked_hosts;

  int list_size = blocked_list.size();
  for(uint i = 0; i< list_size; i++){
    string blocked_ip = blocked_list.at(i).ip;
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
  return nullptr;
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
  return nullptr;
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




                                  /* SHELL CMDs */


/*void Server::cmd_ip(){
  if (serv_ip == "") {cmd_error("IP"); return;}
  cmd_success_start("IP");
  cse4589_print_and_log("IP:%s\n", serv_ip);
  cmd_end("IP");
}

void Server::cmd_port(){
  if (portnum <= 0) {cmd_error("PORT"); return;}
  cmd_success_start("PORT");
  cse4589_print_and_log("PORT:%d\n", portnum);
  cmd_end("PORT");
}*/

void Server::cmd_list(){ //get list of logged in hosts sorted by port number

  cmd_success_start("LIST");
  sort(conn_his.begin(), conn_his.end(), sort_hosts_by_port);
  string ip;
  char hostname[HOSTNAME_LEN];
  int port, ni_ret;
  for(int i =0; i<conn_his.size(); i++){
    struct remotehos_info h = conn_his.at(i);
    if(!h.loggedin) continue;

    ni_ret = getnameinfo((struct sockaddr*) h.sa, sizeof(h.sa), hostname, sizeof(hostname), NULL, 0, NI_NOFQDN | NI_NAMEREQD);
    if(ni_ret){
      cmd_error("LIST");
      perror("ERROR: Could not get hostname for '"); cout<<h.ip<<"'\n";
      return;
    }
    cse4589_print_and_log("%-5d%-35s%-20s%-8d\n", i+1, hostname, h.ip.c_str(), to_string(h.port));
  }
  cmd_end("LIST");
}

void Server::cmd_statistics(){
  cmd_success_start("STATISTICS");
  sort(conn_his.begin(), conn_his.end(), sort_hosts_by_port);
  string login_status;
  char hostname[HOSTNAME_LEN];
  int port, ni_ret;
  for(int i =0; i<conn_his.size(); i++){
    struct remotehos_info h = conn_his.at(i);
    ni_ret = getnameinfo((struct sockaddr*) h.sa, sizeof(h.sa), hostname, sizeof(hostname), NULL, 0, NI_NOFQDN | NI_NAMEREQD);
    if(ni_ret){
      cmd_error("STATISTICS");
      perror("ERROR: Could not get hostname, in statistics");
      return;
    }

    if(h.loggedin) 
      login_status = "logged-in";
    else
      login_status = "logged-out";

    cse4589_print_and_log("%-5d%-35s%-8d%-8d%-8s\n", i+1, hostname, h.msg_bytes_tx, h.msg_bytes_rx, login_status.c_str());
  }
  cmd_end("STATISTICS");

}

void Server::cmd_blocked(string ip){
  //The output should display the hostname, IP address, and the listening port numbers of the bloked clents
  struct remotehos_info host = get_host(ip);
  cout<<"IN FUNCTION 'cmd_blocked' "<<endl;
  cmd_success_start("BLOCKED");
  
  vector<struct remotehos_info> blocked_hosts = host.blocked_hosts;
  if(blocked_hosts.empty()) {cout<< ip<<" has not blocked anyone\n"; return;}

  sort(blocked_hosts.begin(), blocked_hosts.end(), sort_hosts_by_port);
  cout<<"ABOUT TO CALL 'blocked_hosts.size()' "<<endl; //DEBUG
  cout<<blocked_hosts.size()<<endl;
  cout<<"CALLED 'blocked_hosts.size()' "<<endl;  //DEBUG
  
  char hostname[HOSTNAME_LEN];
  int port, ni_ret;
  for(int i =0; i< blocked_hosts.size(); i++){
    struct remotehos_info h = blocked_hosts.at(i);

    ni_ret = getnameinfo((struct sockaddr*) h.sa, sizeof(h.sa), hostname, sizeof(hostname), NULL, 0, NI_NOFQDN | NI_NAMEREQD);
    if(ni_ret){
      cmd_error("BLOCKED");
      perror("ERROR: Could not get hostname for '"); cout<<h.ip<<"'\n";
      return;
    }
    cse4589_print_and_log("%-5d%-35s%-20s%-8d\n", i+1, hostname, h.ip.c_str(), to_string(h.port));
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
  //cout<<"printing a list of connected sockets\n";
  //if(conn_socks.size() ==0) cout<<"No connected hosts\n";
  //for (int i =0; i<conn_socks.size(); i++) cout << "socket: " <<conn_socks.at(i)<<endl;

  cout<<endl;
}
