#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include "../include/shell.h"
#include "../include/global.h"
#include "../include/logger.h"
#include "../include/client.h"
#include "../include/server.h"

using std::vector;
using std::string;
using std::cout;
using std::endl;

void cmd_success_start(string cmd){ cse4589_print_and_log("[%s:SUCCESS]\n", cmd.c_str());}
void cmd_fail_start(string cmd){ cse4589_print_and_log("[%s:ERROR]\n", cmd.c_str());}

void cmd_end(string cmd){ cse4589_print_and_log("[%s:END]\n", cmd.c_str());}

void cmd_error(string cmd){
  cmd_fail_start(cmd);
  cmd_end(cmd);
}

void cmd_author(){
  cmd_success_start("AUTHOR");
  cse4589_print_and_log("I, elwincab, have read and understood the course academic integrity policy.\n");
  cmd_end("AUTHOR");
}

void cmd_ip(string ip){
  if (ip == "") {cmd_error("IP"); return;}
  cmd_success_start("IP");
  cse4589_print_and_log("IP:%s\n", ip.c_str());
  cmd_end("IP");
}

void cmd_port(int portnum){
  if (portnum <= 0) {cmd_error("PORT"); return;}
  cmd_success_start("PORT");
  cse4589_print_and_log("PORT:%d\n", portnum);
  cmd_end("PORT");
}

void event_msg_recvd(string from_ip, string msg){
  cmd_success_start("RECEIVED");
  cse4589_print_and_log("msg from:%s\n[msg]:%s\n", from_ip.c_str(), msg.c_str());
  cmd_end("RECEIVED");
}

void event_msg_relayed(string from_ip, string to_ip, string msg){
  cmd_success_start("RELAYED");
  cse4589_print_and_log("msg from:%s, to:%s\n[msg]:%s\n", from_ip.c_str(), to_ip.c_str(), msg.c_str());
  cmd_end("RELAYED");
}

bool is_valid_ip(string ip){

  return false;
}

string itos(int num){
  string ret;
  vector<char> chars;

  while(num){
    int least_sig_digit = num %10;
    char c = least_sig_digit + '0';
    chars.push_back(c);
    num /= 10;
  }
  for(int i=chars.size()-1; i>=0; i--) ret.push_back(chars.at(i)); 
  
  return ret;
}