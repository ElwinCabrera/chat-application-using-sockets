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
void cmd_author();
void cmd_ip();
void cmd_port();
void cmd_list();

void cmd_statistics();
void cmd_blocked();

void cmd_login();
void cmd_refresh();
void cmd_send();
void cmd_broadcast();
void cmd_block();
void cmd_unblock();
void cmd_logout();
void cmd_exit();
#endif // SHELL_H
