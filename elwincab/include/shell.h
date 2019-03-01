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
#endif // SHELL_H
