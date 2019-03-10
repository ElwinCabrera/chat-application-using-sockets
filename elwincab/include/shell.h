#ifndef SHELL_H
#define SHELL_H
#include <iostream>
#include <vector>
#include <string>

using std::vector;
using std::string;

void shell_execute(vector<string> cmds);
void cmd_success_start(string cmd);
void cmd_fail_start(string cmd);
void cmd_end(string cmd);
void cmd_error(string cmd);
void cmd_author();
void cmd_ip(string ip);
void cmd_port(int port);

void event_msg_recvd(string from_ip, string msg);
void event_msg_relayed(string from_ip, string to_ip, string msg);

string itos(int num);
bool is_valid_ip(string ip);
bool is_valid_port(string port);
#endif // SHELL_H
