#ifndef CLIENT_H
#define CLIENT_H

#include <iostream>
#include <vector>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <map>
#include "global.h"

using std::string;
using std::vector;
using std::map;
using std::pair;

class Client{


public:
    Clinet();
    ~Client();
};

#endif // CLIENT_H