#include "http.h"
#include "hashtab.h"
#include "net.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define METHOD_SIZE 16
#define PATH_SIZE 2048
#define PROTO_SIZE 16
/*
// длинна истории и размер сообщения
#define MAX_HISTORY_SIZE 100
#define MAX_MESSAGE_SIZE 256

#define MAX_BUFFER_SIZE 1024  // Замените 1024 на нужное вам значение
*/

typedef struct HTTP {
	char *host;
	int32_t len;
	int32_t cap;
	void(**funcs)(int, HTTPreq*);
	HashTab *tab;
/*	//chat history.
    char chat_history[MAX_HISTORY_SIZE][MAX_MESSAGE_SIZE];
    int chat_history_count;
*/
} HTTP;

static HTTPreq _new_request(void);
static void _null_request(HTTPreq *request);
static void _parse_request(HTTPreq *request, char *buffer, size_t size);
static void _page404_http(int conn);
static int8_t _switch_http(HTTP *http, int conn, HTTPreq *request);


extern HTTP *new_http(char *address) {
	HTTP *http = (HTTP *) malloc(sizeof(HTTP));
	http->cap = 1000;
	http->len = 0;
	http->host = (char*)malloc(sizeof (char)*strlen(address)+1);
	strcpy(http->host, address);
	http->tab = new_hashtab(http->cap, STRING_TYPE, DECIMAL_TYPE);
	http->funcs = (void(**)(int, HTTPreq*))malloc(http->cap * (sizeof (void(*)(int, HTTPreq*))));
	return http;
}

extern void free_http(HTTP *http) {
	free_hashtab(http->tab);
	free(http->host);
	free(http->funcs);
	free(http);
}

extern void handle_http(HTTP *http, char *path, void(*handle)(int, HTTPreq*)) {
	set_hashtab(http->tab, string(path), decimal(http->len));
	http->funcs[http->len] = handle;
	http->len += 1;
	if (http->len == http->cap) {
		http->cap <<= 1;
		http->funcs = (void(**)(int, HTTPreq*))realloc(http->funcs, 
			http->cap * (sizeof (void(*)(int, HTTPreq*))));
	}
}

extern int8_t listen_http(HTTP *http) {
	int listener = listen_net(http->host);
	if (listener < 0) {
		return 1;
	}
	while(1) {
		int conn = accept_net(listener);
		if (conn < 0) {
			continue;
		}
		HTTPreq req = _new_request();
		while(1) {
			char buffer[BUFSIZ] = {0};
			int n = recv_net(conn, buffer, BUFSIZ);
			if (n < 0) {
				break;
			}
			_parse_request(&req, buffer, n);
/*
            // Обработка сообщений
            if (strcmp(req.method, "POST") == 0) {
                // Обработка данных, отправленных методом POST
                // Параметр "message", добавляется в историю чата
                char* message_param = strstr(buffer, "message=");
                if (message_param != NULL) {
                    char message[MAX_MESSAGE_SIZE];
                    sscanf(message_param, "message=%s", message);
                    if (http->chat_history_count < MAX_HISTORY_SIZE) {
                        strcpy(http->chat_history[http->chat_history_count], message);
                        http->chat_history_count++;
                    }
                }
            }

*/
			if (n != BUFSIZ) {
				break;
			}
		}
		_switch_http(http, conn, &req);
		close_net(conn);
	}
	close_net(listener);
	return 0;
}

extern void parsehtml_http(int connect, char *filename) {
	char buffer[BUFSIZ] = "HTTP/1.1 200 OK\nContent-type: text/html\n\n";
	size_t readsize = strlen(buffer);
	send_net(connect, buffer, readsize);
	FILE *file = fopen(filename, "r");
	if (file == NULL) {
		return;
	}
	while((readsize = fread(buffer, sizeof(char), BUFSIZ, file)) != 0) {
		send_net(connect, buffer, readsize);
	}
	fclose(file);

}

static HTTPreq _new_request(void) {
	return (HTTPreq){
		.method = {0},
		.path = {0},
		.proto = {0},
		.state = 0,
		.index = 0,
	};
}

/*
 * GET /books HTTP/1.1
 *
 * */

static void _parse_request(HTTPreq *request, char *buffer, size_t size) {
	//printf("%s\n", buffer);
	for (size_t i = 0; i < size; i++) {
		switch (request->state) {
			case 0:
				if (buffer[i] == ' ' || request->index == METHOD_SIZE-1) {
					request->method[request->index] = '\0';
					_null_request(request);
					continue;
				}
				request->method[request->index] = buffer[i];
				break;
			case 1:
				if (buffer[i] == ' ' || request->index == PATH_SIZE-1) {
					request->path[request->index] = '\0';
					_null_request(request);
					continue;
				}
				request->path[request->index] = buffer[i];
				break;
			case 2:
				if (buffer[i] == '\n' || request->index == PROTO_SIZE-1) {
					request->proto[request->index] = '\0';
					_null_request(request);
					continue;
				}
				request->proto[request->index] = buffer[i];
				break;
			default: return;
		}
		request->index += 1;
	}
}

static void _null_request(HTTPreq *request) {
	request->state += 1;
	request->index = 0;
}

/*
 *	/about/1 -> error
 *	if (!in_hashtab... -> решает проблему
 *
 * */

static int8_t _switch_http(HTTP *http, int conn, HTTPreq *request) {
	
/*
    // Проверка пути /history
    if (strcmp(request->path, "/history") == 0) {
        show_chat_history(conn, http);
        _null_request(request);
        return 0;
    }
*/

	
	// Обработка путей с / в конце
	int path_len = strlen(request->path);

	if (path_len > 1 && request->path[path_len - 1] == '/') {
		// Удаление / в конце, кроме случая "/"
		request->path[path_len - 1] = '\0';
	}
	if (!in_hashtab(http->tab, string(request->path))) {
		char buffer[PATH_SIZE];
		memcpy(buffer, request->path, PATH_SIZE);
		int32_t index = strlen(request->path);

		if (index == 0) {
			_page404_http(conn);
			return 1;
		}

		index -= 1;
		buffer[index] = '\0';

		while (index > 0 && buffer[index] != '/') {
			buffer[index] = '\0';
			--index;
		}

		if (!in_hashtab(http->tab, string(buffer))) {
			_page404_http(conn);
			return 2;
		}

		index = get_hashtab(http->tab, string(buffer)).decimal;
		http->funcs[index](conn, request);
		_null_request(request);
		return 0;
	}

	int32_t index = get_hashtab(http->tab, string(request->path)).decimal;
	http->funcs[index](conn, request);
	_null_request(request);
	return 0;
}

static void _page404_http(int conn) {
	char *header = "HTTP/1.1 404 Not Found\n\nnot found";
	size_t headsize = strlen(header);
	send_net(conn, header, headsize);
}

/*
extern void show_chat_history(int connect, HTTP *http) {
    char buffer[MAX_BUFFER_SIZE] = "HTTP/1.1 200 OK\nContent-type: text/html\n\n";
    size_t readsize = strlen(buffer);
    send_net(connect, buffer, readsize);

    // Отправляем HTML-заголовок и начало страницы
    snprintf(buffer, sizeof(buffer), "<html><head><title>Chat History</title></head><body><h1>Chat History</h1>");

    // Отправляем историю чата
    for (int i = 0; i < http->chat_history_count; i++) {
        snprintf(buffer, sizeof(buffer), "<p>%s</p>", http->chat_history[i]);
        send_net(connect, buffer, strlen(buffer));
    }

    // Завершаем страницу
    snprintf(buffer, sizeof(buffer), "</body></html>");
    send_net(connect, buffer, strlen(buffer));
}
*/
