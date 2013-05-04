#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef WIN32
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif
	#include <winsock2.h>
#else
	#if PSP
		#include <pspuser.h>
		#include <psputility.h>
		#include <psputility_netmodules.h>
		#include <pspnet.h>
		#include <pspnet_inet.h>
		#include <pspnet_apctl.h>
	#endif
	#include <unistd.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <sys/socket.h>
	#include <sys/select.h>
	#include <netdb.h>
	#include <errno.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include "new_socket.h"
#include "xmalloc.h"

#ifndef WIN32
	#define closesocket(s)  close(s)
	#define SD_RECEIVE  SHUT_RD
	#define SD_SEND 	SHUT_WR
	#define SD_BOTH 	SHUT_RDWR
#endif

#ifndef USER_AGENT
#define USER_AGENT 	"liboauth-mod-agent/0.9.5"
#endif


static int init_flag = 0;


#ifdef PSP
static int socket_loadinit_module(void)
{
	if (sceUtilityLoadNetModule(PSP_NET_MODULE_COMMON)) {
		return -1;
	}

	if (sceUtilityLoadNetModule(PSP_NET_MODULE_INET)) {
		return -2;
	}

	sceNetInit(0x20000, 0x2A, 0x1000, 0x2A, 0x1000);
	sceNetInetInit();
	sceNetApctlInit(0x8000, 48);

	return 0;
}

static void socket_term_module(void)
{
	sceNetApctlTerm();
	sceNetInetTerm();
	sceNetTerm();
}
#endif // PSP

void socket_init(void)
{
	if (!init_flag) {
#ifdef WIN32
		WSADATA wsa;
		WSAStartup(MAKEWORD(2, 0), &wsa);
#elif PSP
		socket_loadinit_module();
#endif
		init_flag = 1;
	}
}

void socket_release(void)
{
	if (init_flag) {
#ifdef WIN32
		WSACleanup();
#elif PSP
		socket_term_module();
#endif
	}
}

socket_t socket_open(int af, int type, int protocol)
{
	return socket(af, type, protocol);
}

int socket_close(socket_t sock)
{
	shutdown(sock, SD_BOTH);
	return closesocket(sock);
}

int socket_safeclose(socket_t sock)
{
	unsigned char tmp[1024];
	int n;

	shutdown(sock, SD_SEND);

	while( 1 ) {
		n = recv(sock, tmp, sizeof tmp, 0);
		if( n <= 0 )
			break;
	}

	shutdown(sock, SD_BOTH);
	return closesocket(sock);
}

socket_t socket_connect(socket_t sock, const char *hostname, int port)
{
	struct hostent *host = NULL;
	struct sockaddr_in saddr;

	memset(&saddr, 0, sizeof(struct sockaddr_in));
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons((unsigned short)port);

	if( (host = gethostbyname(hostname)) ) {
		saddr.sin_addr.s_addr = *((unsigned long *)host->h_addr_list[0]);
	} else {
		saddr.sin_addr.s_addr = inet_addr(hostname);
	}

	if( connect(sock, (struct sockaddr *)&saddr, sizeof(struct sockaddr_in)) >= 0 ) {
		return sock;
	}

	closesocket(sock);
	return INVALID_SOCKET;
}

int socket_recv(socket_t sock, void *buf, size_t size)
{
	return recv(sock, (char *)buf, size, 0);
}

int socket_send(socket_t sock, const void *data, size_t size)
{
	return send(sock, (const char *)data, size, 0);
}

size_t socket_read(socket_t sock, void *buffer, size_t size)
{
	char *pos = (char *)buffer;
	int nrecv;

	while (size > 0) {
		nrecv = recv(sock, pos, size, 0);
		if (nrecv < 0) {
			// error.
			return nrecv;
		} else if (nrecv == 0) {
			// disconnect.
			break;
		}

		pos += nrecv;
		size -= nrecv;
	}

	return (size_t)(pos - (char *)buffer);
}

void *socket_read_alloc(socket_t sock, size_t *readsize)
{
	const size_t addsize = 2048;
	unsigned char *buffer = NULL;
	size_t allocsize = 0;
	unsigned char *pos = NULL;
	size_t recvsize = 0;
	int res = 0;
	size_t sumsize = 0;

	for(;;)
	{
		allocsize += addsize;
		buffer = (unsigned char *)xrealloc(buffer, allocsize);
		pos = buffer + sumsize;
		recvsize = addsize - sizeof(unsigned char);

		while( recvsize > 0 )
		{
			res = recv(sock, pos, recvsize, 0);
			if( res < 0 ) {
				free(buffer);
				*readsize = 0;
				return NULL;
			}
			else if( res == 0 ) {
				*readsize = sumsize;
				return (void *)buffer;
			}

			pos += res;
			recvsize -= res;
			sumsize += res;
		}
	}
	return NULL;
}

size_t socket_write(socket_t sock, const void *data, size_t size)
{
	const char *pos = (const char *)data;
	int nsend;

	while (size > 0)
	{
		nsend = send(sock, pos, size, 0);
		if (nsend < 0) {
			// error.
			return nsend;
		} else if (nsend == 0) {
			break;
		}

		pos += nsend;
		size -= nsend;
	}

	return (size_t)(pos - (const char *)data);
}

size_t socket_write_str(socket_t sock, const char *string)
{
	return socket_write(sock, string, strlen(string));
}

size_t socket_write_file(socket_t sock, const char *filename, size_t filesize)
{
	const size_t blocksize = 0x2000;
	size_t nwrite = 0, nread = 0, ntotal = 0;
	unsigned char *buffer;
	FILE *fp = NULL;
	struct stat st;

	if( filesize == 0 ) {
		if(stat(filename, &st) == -1) {
			return 0;
		}
		filesize = st.st_size;
	}

#ifdef WIN32
	fp = fopen(filename, "rb");
#else
	fp = fopen(filename, "r");
#endif
	if( !fp ) {
		return 0;
	}

	buffer = (unsigned char *)xmalloc(blocksize);

	while( (nread = fread(buffer, sizeof(unsigned char), blocksize, fp)) > 0 && ntotal < filesize )
	{
		nread = (nread < (filesize - ntotal)) ? nread : (filesize - ntotal);
		nwrite = socket_write(sock, buffer, nread);
		if( nwrite != nread ) {
			break;
		}
		ntotal += nwrite;
	}

	free(buffer);
	fclose(fp);
	return ntotal;
}


/**
* HTTP functions.
*
*/

typedef struct HTTPRequest {
	char *name;
	int port;
	char *uri;
} HTTPRequest;

/* Copy to header.
typedef struct tagHTTPResponse {
	int status_code;
	char *header;
	size_t header_size;
	unsigned char *data;
	size_t data_size;
} HTTPResponse;
*/

static HTTPRequest *malloc_HTTPRequest(void)
{
	HTTPRequest *req = (HTTPRequest *)xmalloc(sizeof(HTTPRequest));
	req->name = NULL;
	req->uri = NULL;
	req->port = 0;
	return req;
}

static void free_HTTPRequest(HTTPRequest *req)
{
	if( req ) {
		if( req->name ) {
			free(req->name);
			req->name = NULL;
		}
		if( req->uri ) {
			free(req->uri);
			req->uri = NULL;
		}
		free(req);
	}
}

static HTTPResponse *malloc_HTTPResponse(void)
{
	HTTPResponse *res = (HTTPResponse *)xmalloc(sizeof(HTTPResponse));
	res->status_code = 0;
	res->header = NULL;
	res->header_size = 0;
	res->data = NULL;
	res->data_size = 0;
	return res;
}

// Not use.
#if 0
static void free_HTTPResponse(HTTPResponse *res)
{
	if (res) {
		if (res->header) {
			free(res->header);
			res->header = NULL;
		}
		if (res->data) {
			free(res->data);
			res->data = NULL;
		}
		free(res);
	}
}
#endif

static __inline char *ntos(char *dest, size_t num)
{
	sprintf(dest, "%u", num);
	return dest;
}

static void parse_http_url(HTTPRequest *resreq, const char *url)
{
//	Example URL: "http://www.xxx.yy.zz:80/hoge.json"
	char *pos = NULL;
	size_t len = 0;

	if( !strncmp(url, "http://", 7) ) url += 7;
	if( !strncmp(url, "https://", 8) ) url += 8;

	pos = strchr(url, '/');
	if( pos != NULL ) {
		len = pos - url;
		resreq->name = (char *)xmalloc(len + 1);
		strncpy(resreq->name, url, len);
		resreq->name[len] = '\0';

		if( *(pos + 1) != '\0' ) {
			resreq->uri = xstrdup(pos);
		} else {
			resreq->uri = xstrdup("/");
		}
	} else {
		resreq->name = xstrdup(url);
		resreq->uri = xstrdup("/");
	}

#ifdef DEBUG
		fprintf(stderr, "hostname: %s\n", resreq->name);
		fprintf(stderr, "uri: %s\n\n", resreq->uri);
#endif

	pos = strchr(resreq->name, ':');
	if( pos ) {
		resreq->port = strtol(pos + 1, NULL, 10);
		*pos = '\0';
	} else {
		resreq->port = 80;
	}
}

static HTTPResponse *parse_http_result(const char *result, size_t result_size)
{
	const char *header_start = result;
	const char *data_start = NULL;
	const char *result_end = result + result_size;
	const char *cursor = result;

	HTTPResponse *http_response = NULL;
	int size = 0;
	char *space = NULL;

	if( !result || result_size <= 4 || strncmp(result, "HTTP", 4) != 0 ) {
		return NULL;
	}

//	Example: HTTP/1.1 200 OK\r\n
	space = strchr(result, ' ');
	if( !space ) return NULL;

	http_response = malloc_HTTPResponse();
	http_response->status_code = strtol(space + 1, NULL, 10);
#ifdef DEBUG
	fprintf(stderr, "status_code: %d\n\n", http_response->status_code);
#endif

	while( cursor + 4 < result_end )
	{
		if( cursor[0] == '\r' && cursor[1] == '\n' &&
			cursor[2] == '\r' && cursor[3] == '\n' )
		{
			cursor += 4;
			data_start = cursor;
			break;
		}
		cursor++;
	}

#ifdef DEBUG
	fprintf(stderr, "start split header... ");
#endif

	size = cursor - header_start;
	http_response->header = (char *)xmalloc(size + sizeof(char));
	memcpy(http_response->header, header_start, size);
	http_response->header[size] = '\0';
	http_response->header_size = size;

#ifdef DEBUG
	fprintf(stderr, "%d byte.\n", size);
	fprintf(stderr, "start split body... ");
#endif

	if( data_start ) {
		size = result_end - data_start;
		http_response->data = (unsigned char *)xmalloc(size);
		memcpy(http_response->data, data_start, size);
		http_response->data_size = size;
	}

#ifdef DEBUG
	fprintf(stderr, "%d byte.\n", size);
#endif

	return http_response;
}


HTTPResponse *socket_http_get(const char *url, const char *query, const char *custom_header, int keepalive)
{
	socket_t sock;
	char *uri = NULL;
	HTTPRequest *http_request = NULL;
	HTTPResponse *http_response = NULL;
	char *request = NULL;
	size_t request_size = 0;
	char *response = NULL;
	size_t response_size = 0;

	// make http request.
	http_request = malloc_HTTPRequest();
	parse_http_url(http_request, url);

	request_size = custom_header ? 0x800 + strlen(custom_header) + 1 : 0x800;
	request = (char *)xmalloc(request_size);

	if( query ) {
		uri = (char *)xmalloc(strlen(http_request->uri) + strlen(query) + 1);
		strcpy(uri, http_request->uri);
		strcat(uri, "?");
		strcat(uri, query);
	} else {
		uri = http_request->uri;
	}

	strcpy(request, "GET ");
	strcat(request, uri);
	strcat(request, " HTTP/1.1\r\n");
	strcat(request, "User-Agent: " USER_AGENT "\r\n");
	strcat(request, "Host: ");
	strcat(request, http_request->name);
	strcat(request, "\r\n");
	strcat(request, "Accept: */*\r\n");
	if( !keepalive ) {
		strcat(request, "Connection: close\r\n");
	}
	if( custom_header ) {
		strcat(request, custom_header);
	}
	strcat(request, "\r\n"); // End http request header line.

	// Connect.
	if( (sock = socket_open(AF_INET, SOCK_STREAM, 0)) < 0 ) {
		return NULL;
	}

	if( socket_connect(sock, http_request->name, http_request->port) == INVALID_SOCKET ) {
		return NULL;
	}
	free_HTTPRequest(http_request);

	socket_write_str(sock, request);
	free(request);

	response = (char *)socket_read_alloc(sock, &response_size);
	response[response_size] = '\0';

#ifdef DEBUG
	fprintf(stderr, "socket_http_get(): response_size = %d\n", response_size);
#endif

	socket_close(sock);

	// parse response.
	if( *response ) {
		http_response = parse_http_result(response, response_size);
	}

	free(response);
	return http_response;
}

HTTPResponse *socket_http_post(const char *url, const char *content, size_t content_size, const char *custom_header, int keepalive)
{
	socket_t sock;
	HTTPRequest *http_request = NULL;
	HTTPResponse *http_response = NULL;
	char *request = NULL;
	size_t request_size = 0;
	char *response = NULL;
	size_t response_size = 0;
	char str_num[16] = "";

	// make http request.
	http_request = malloc_HTTPRequest();
	parse_http_url(http_request, url);

	request_size = custom_header ? 0x800 + strlen(custom_header) + 1 : 0x800;
	request = (char *)xmalloc(request_size);

	strcpy(request, "POST ");
	strcat(request, http_request->uri);
	strcat(request, " HTTP/1.1\r\n");
	strcat(request, "User-Agent: " USER_AGENT "\r\n");
	strcat(request, "Host: ");
	strcat(request, http_request->name);
	strcat(request, "\r\n");
	strcat(request, "Accept: */*\r\n");
	strcat(request, "Content-Length: ");
	strcat(request, ntos(str_num, content_size));
	strcat(request, "\r\n");
	strcat(request, "Content-Type: application/x-www-form-urlencoded\r\n");
	if( !keepalive ) {
		strcat(request, "Connection: close\r\n");
	}
	if( custom_header ) {
		strcat(request, custom_header);
	}
	strcat(request, "\r\n"); // End http request header line.

	// Connect.
	if( (sock = socket_open(AF_INET, SOCK_STREAM, 0)) < 0) {
//		fprintf(stderr, "Error, socket_open()\n\n");
		return NULL;
	}

	if( socket_connect(sock, http_request->name, http_request->port) == INVALID_SOCKET ) {
//		fprintf(stderr, "Error, socket_connect()\n\n");
		return NULL;
	}
	free_HTTPRequest(http_request);

	socket_write_str(sock, request);
	socket_write(sock, content, content_size);
	free(request);

	response = (char *)socket_read_alloc(sock, &response_size);
	response[response_size] = '\0';

	socket_close(sock);

	// parse response.
	if( *response ) {
		http_response = parse_http_result(response, response_size);
	}

	free(response);
	return http_response;
}

HTTPResponse *socket_http_post_file(const char *url, const char *file_name, size_t file_size, const char *custom_header, int keepalive)
{
	socket_t sock;
	HTTPRequest *http_request = NULL;
	HTTPResponse *http_response = NULL;
	char *request = NULL;
	size_t request_size = 0;
	char *response = NULL;
	size_t response_size = 0;

	struct stat st;
	char str_num[16] = "";

	if( file_size == 0 ) {
		if( stat(file_name, &st) == -1 ) {
			return NULL;
		}
		file_size = st.st_size;
	}

	// make http request.
	http_request = malloc_HTTPRequest();
	parse_http_url(http_request, url);

	request_size = custom_header ? 0x800 + strlen(custom_header) + 1 : 0x800;
	request = (char *)xmalloc(request_size);

	strcpy(request, "POST ");
	strcat(request, http_request->uri);
	strcat(request, " HTTP/1.1\r\n");
	strcat(request, "User-Agent: " USER_AGENT "\r\n");
	strcat(request, "Host: ");
	strcat(request, http_request->name);
	strcat(request, "\r\n");
	strcat(request, "Accept: */*\r\n");
	strcat(request, "Content-Length: ");
	strcat(request, ntos(str_num, file_size));
	strcat(request, "\r\n");
	if( !keepalive ) {
		strcat(request, "Connection: close\r\n");
	}
	if( custom_header ) {
		strcat(request, custom_header);
	} else {
		strcat(request, "Content-Type: image/jpeg;\r\n");
	}
	strcat(request, "\r\n"); // End http request header line.

	// Connect.
	if( (sock = socket_open(AF_INET, SOCK_STREAM, 0)) < 0 ) {
		return NULL;
	}

	if( socket_connect(sock, http_request->name, http_request->port) == INVALID_SOCKET ) {
		return NULL;
	}
	free_HTTPRequest(http_request);

	socket_write_str(sock, request);
	socket_write_file(sock, file_name, file_size);
	free(request);

	response = socket_read_alloc(sock, &response_size);
	response[response_size] = '\0';

	socket_close(sock);

	// parse response.
	if( *response ) {
		http_response = parse_http_result(response, response_size);
	}

	free(response);
	return http_response;
}


#if 0
HTTPResponse *socket_post_data(const char *url, const char *data, size_t data_size, const char *custom_header, int keepalive)
{
	socket_t sock;
	HTTPRequest *http_request = NULL;
	HTTPResponse *http_response = NULL;
	char *request = NULL;
	size_t request_size = 0;
	char *response = NULL
	size_t resposne_size = 0;

	char str_num[16] = "";

	// make http request.
	http_request = malloc_HTTPRequest();
	parse_http_url(http_request, url);

	request_size = custom_header ? 0x800 + strlen(custom_header) + 1 : 0x800;
	request = (char *)xmalloc(request_size);

	strcpy(request, "GET ");
	strcat(request, http_request->uri);
	strcat(request, " HTTP/1.1\r\n");
	strcat(request, "User-Agent: " USER_AGENT "\r\n");
	strcat(request, "Host: ");
	strcat(request, http_request->name);
	strcat(requset, "\r\n");
	strcat(request, "Accept: */*\r\n");
	strcat(req, "Content-Length: ");
	strcat(req, ntos(str_num, data_size));
	strcat(req, "\r\n");
	if( !keepalive ) {
		strcat(request, "Connection: close\r\n");
	}
	if( custom_header ) {
		strcat(request, custom_header);
	} else {
		strcat(request, "Content-Type: image/jpeg;\r\n");
	}
	strcat(request, "\r\n"); // End http request header line.

	// Connect.
	sock = socket_open(AF_INET, SOCK_STREAM, 0);
	socket_connect(sock, http_request->name, http_request->port);
	free_HTTPRequest(http_request);

	socket_write_str(sock, request);
	socket_write(sock, data, data_size);
	free(request);

	response = socket_read_alloc(sock, &response_size);
	response[response_size] = '\0';

	socket_close(sock);

	// parse response.
	if( *response ) {
		http_response = parse_http_result(response, response_size);
	}

	free(response);
	return http_response;
}
#endif
