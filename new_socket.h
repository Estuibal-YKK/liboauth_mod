
#ifndef _MY_SOCKET_H_
#define _MY_SOCKET_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef WIN32
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif
	#include <winsock2.h>
	#include <ws2tcpip.h>
#endif

#ifdef WIN32
	typedef SOCKET socket_t;
#else
	typedef int socket_t;
	#define INVALID_SOCKET ((socket_t)(-1))
#endif


enum {
	NOT_KEEPALIVE = 0,
	KEEPALIVE,
};

typedef struct tagHTTPResponse {
	int status_code;
	char *header;
	size_t header_size;
	unsigned char *data;
	size_t data_size;
} HTTPResponse;


void socket_init(void);
void socket_release(void);

socket_t socket_open(int af, int type);
int socket_close(socket_t sock);
int socket_safeclose(socket_t sock);
socket_t socket_connect(socket_t sock, const char *hostname, int port);

int socket_recv(socket_t sock, void *buf, size_t size);
int socket_send(socket_t sock, const void *data, size_t size);

size_t socket_read(socket_t sock, void *buffer, size_t size);
void *socket_read_alloc(socket_t sock, size_t *readsize);
size_t socket_write(socket_t sock, const void *data, size_t size);
size_t socket_write_str(socket_t sock, const char *string);
size_t socket_write_file(socket_t sock, const char *filename, size_t filesize);

HTTPResponse *socket_http_get(const char *url, const char *query, const char *custom_header, int keepalive);
HTTPResponse *socket_http_post(const char *url, const char *content, size_t content_size, const char *custom_header, int keepalive);
HTTPResponse *socket_http_post_file(const char *url, const char *file_name, size_t file_size, const char *custom_header, int keepalive);


#ifdef __cplusplus
}
#endif

#endif // _MY_SOCKET_H_
