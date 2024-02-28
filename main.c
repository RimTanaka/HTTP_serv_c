#include "extclib/http.h"
#include <string.h>

void index_page(int connect, HTTPreq *req);
void about_page(int connect, HTTPreq *req);
void chat_page(int connect, HTTPreq *req);

int main(void) {
	HTTP *serv = new_http("0.0.0.0:7545");

	handle_http(serv, "/", chat_page);

	handle_http(serv, "/about", about_page);

	listen_http(serv);
	free_http(serv);
	return 0;
}

void about_page(int connect, HTTPreq *req) {
	if(strcmp(req->path, "/about") != 0) {
		parsehtml_http(connect, "page404.html");
		return;
	}
	parsehtml_http(connect, "about.html");
}
void chat_page(int connect, HTTPreq *req) {
	if(strcmp(req->path, "/") != 0) {
		parsehtml_http(connect, "page404.html");
		return;
	}
	parsehtml_http(connect, "chat.html");
}
