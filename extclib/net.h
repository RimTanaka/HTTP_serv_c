#ifndef NET_H
#define NET_H

#include <stddef.h>

extern int listen_net(char *address); // поднимаем сервер

extern int accept_net(int listener);

extern int connect_net(char *address);
extern int close_net(int connect);

extern int send_net(int connect, char *buff, size_t size);
extern int recv_net(int connect, char *buff, size_t size);

#endif
