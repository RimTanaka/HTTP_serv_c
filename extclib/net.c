#include <asm-generic/socket.h>
#include <sys/socket.h>
#ifdef __linux__
	#include <arpa/inet.h>
	#include <unistd.h>
#elif __WIN32
	#include <winsock2.h>

#else
	#warning "net.h : platform not supported"
#endif

#if defined(__linux__) || defined(__WIN32)

#include "net.h"
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>

static int8_t _parse_address(char *address, char *ipv4, char *port);

typedef enum error_t {
	WINSOCK_ERR = -1,
	SOCET_ERR = -2,
	SEROPT_ERR = -3,
	PARSE_ERR = -4,
	BIND_ERR = -5,
	LISTEN_ERR = -6,
	CONNECT_ERR = -7,
} error_t;

// 127.0.0.1:8080
extern int listen_net(char *address) { // поднимаем сервер
#ifdef __WIN32
	WSDATA wsa;
	if(WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
		return WINSOCK_ERR;
	}
#endif
	int listener = socket(AF_INET, SOCK_STREAM, 0);
	if (listener < 0) {
		return SOCET_ERR;
	}

	if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) {
		return SEROPT_ERR;
	}
	char ipv4[16];
	char port[6];
	if(_parse_address(address, ipv4, port) != 0) {
		return PARSE_ERR;
	}
	struct sockaddr_in addr;

	addr.sin_family = AF_INET;
	addr.sin_port = htons(atoi(port));
	//addr.sin_addr.s_addr = inet_addr(ipv4); // позволяет слушать только на 127,0,0,1
	addr.sin_addr.s_addr = htonl(INADDR_ANY); // Любой ip может подключится

	if (bind(listener, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
		return BIND_ERR;
	}

	if (listen(listener, SOMAXCONN) != 0) {
		return LISTEN_ERR;
	}
	return listener;
}

extern int accept_net(int listener) {
    struct sockaddr_in client_address;
    socklen_t client_len = sizeof(client_address);
    int client_socket = accept(listener, (struct sockaddr *)&client_address, &client_len);
    if (client_socket == -1) {
        perror("accept");
    }
    return client_socket;
}

extern int connect_net(char *address) {
#ifdef __WIN32
	WSDATA wsa;
	if(WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
		return WINSOCK_ERR;
	}
#endif
	int conn = socket(AF_INET, SOCK_STREAM, 0);
	if (conn < 0) {
		return SOCET_ERR;
	}
	char ipv4[16];
	char port[6];
	if(_parse_address(address, ipv4, port) != 0) {
		return PARSE_ERR;
	}
//вычесляем порт и ip
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(atoi(port));
	addr.sin_addr.s_addr = inet_addr(ipv4);
	if (connect(conn, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
		return CONNECT_ERR;
	}
	// если все норм возврощаем сокет
	return conn;

}

extern int close_net(int conn) {
#ifdef __linux__
	return close(conn);
#elif __WIN32
	return closesocket(conn);
#endif
}

extern int send_net(int conn, char *buff, size_t size) {
	return send(conn, buff, (int)size, 0);
}
extern int recv_net(int conn, char *buff, size_t size) {
	return recv(conn, buff, (int)size, 0);
}

static int8_t _parse_address(char *address, char *ipv4, char *port) {
	size_t i = 0, j = 0;
	while (address[i] != ':') {
		if (address[i] == '\0') {
			return 1;
		}
		if (i >= 15) {
			return 2;
		}
		ipv4[i] = address[i];
		i++;
	}
	ipv4[i] = '\0';
	i++;
	while (address[i] != '\0') {
		if (j >= 5) {
			return 3;
		}
		port[j] = address[i];
		i++;
		j++;
	}
	port[j] = '\0';
	return 0;
}

#endif // defined(__linux__) || defined(__WIN32)
