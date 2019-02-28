#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include "../include/shell.h"
#include "../include/global.h"
#include "../include/logger.h"
//#include "../include/client.h"
#include "../include/server.h"

using std::vector;
using std::string;
using std::cout;
using std::endl;


void shell_execute(vector<string> cmds){

  if(cmds.at(0).compare(AUTHOR) == 0){ // here down general cmds
    cmd_author();

  } else if (cmds.at(0).compare(IP) == 0){

  } else if(cmds.at(0).compare(PORT) == 0) {

  } else if(cmds.at(0).compare(LIST) == 0)  {

  } else if(cmds.at(0).compare(STATISTICS) == 0)  { //here down start server cmds

  } else if(cmds.at(0).compare(BLOCKED) == 0)  {

  } else if(cmds.at(0).compare(LOGIN) == 0)  { //here down start client cmds

  } else if(cmds.at(0).compare(REFRESH) == 0)  {

  } else if(cmds.at(0).compare(SEND) == 0)  {

  } else if(cmds.at(0).compare(BROADCAST) == 0)  {

  } else if(cmds.at(0).compare(BLOCK) == 0)  {

  } else if(cmds.at(0).compare(UNBLOCK) == 0)  {

  } else if(cmds.at(0).compare(LOGOUT) == 0)  {

  } else if(cmds.at(0).compare(EXIT) == 0)  {

  } else {
    cout << "unknown command"<<endl;
  }

}

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


void cmd_login(){

}

void cmd_refresh(){

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

}

void cmd_exit(){

}
