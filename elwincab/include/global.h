#ifndef GLOBAL_H_
#define GLOBAL_H_

#define HOSTNAME_LEN 128
#define PATH_LEN 256
#define BUFFER_MAX 256
#define STDIN 0

#define BROADIP "255.255.255.255"

#define AUTHOR "AUTHOR"
#define IP "IP"
#define PORT "PORT"
#define LIST "LIST"

#define STATISTICS "STATISTICS"
#define BLOCKED "BLOCKED"

#define LOGIN "LOGIN"
#define BROADCAST "BROADCAST"
#define SEND "SEND"
#define BLOCK "BLOCK"
#define UNBLOCK "UNBLOCK"
#define REFRESH "REFRESH"
#define LOGOUT "LOGOUT"
#define EXIT "EXIT"


/**
Below is the definition of addrinfo:

  struct addrinfo {
 int ai_flags; // AI_PASSIVE, AI_CANONNAME, etc.
 int ai_family; // AF_INET, AF_INET6, AF_UNSPEC
 int ai_socktype; // SOCK_STREAM, SOCK_DGRAM
 int ai_protocol; // use 0 for "any"
 size_t ai_addrlen; // size of ai_addr in bytes
 struct sockaddr *ai_addr; // struct sockaddr_in or _in6
 char *ai_canonname; // full canonical hostname
 struct addrinfo *ai_next; // linked list, next node
};

struct sockaddr {
 unsigned short sa_family; // address family, AF_xxx
 char sa_data[14]; // 14 bytes of protocol address
};

// (IPv4 only--see struct sockaddr_in6 for IPv6)
struct sockaddr_in {
 short int sin_family; // Address family, AF_INET
 unsigned short int sin_port; // Port number
 struct in_addr sin_addr; // Internet address
 unsigned char sin_zero[8]; // Same size as struct sockaddr
};

// Internet address (a structure for historical reasons)
struct in_addr {
 uint32_t s_addr; // that's a 32-bit int (4 bytes)
};

struct sockaddr_storage {
 sa_family_t ss_family; // address family
 // all this is padding, implementation specific, ignore it:
 char __ss_pad1[_SS_PAD1SIZE];
 int64_t __ss_align;
 char __ss_pad2[_SS_PAD2SIZE];
};

*/



#endif
