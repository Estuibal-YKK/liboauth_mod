/*
 * OAuth http functions in POSIX-C.
 *
 * Copyright 2007, 2008, 2009, 2010 Robin Gareus <robin@gareus.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

/*
#if HAVE_CONFIG_H
# include <config.h>
#endif
*/

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

/*
#ifdef WIN32
#  define snprintf _snprintf
#endif
*/

#include "xmalloc.h"
#include "oauth.h"
#include "new_socket.h"


/**
 * do a HTTP GET request, wait for it to finish 
 * and return the content of the reply.
 * (requires libcurl or a command-line HTTP client)
 * 
 * more documentation in oauth.h
 *
 * @param u base url to get
 * @param q query string to send along with the HTTP request or NULL.
 * @return  In case of an error NULL is returned; otherwise a pointer to the
 * replied content from HTTP server. latter needs to be freed by caller.
 */
char *oauth_http_get(const char *u, const char *q)
{
	char *result = NULL;
	HTTPResponse *response = NULL;
	response = socket_http_get(u, q, NULL, NOT_KEEPALIVE);
	if( response ) {
		result = (char *)response->data;
		free(response->header);
		free(response);
	}
	return result;
}

/**
 * do a HTTP GET request, wait for it to finish 
 * and return the content of the reply.
 * (requires libcurl)
 * 
 * @param u base url to get
 * @param q query string to send along with the HTTP request or NULL.
 * @param customheader specify custom HTTP header (or NULL for none)
 * @return  In case of an error NULL is returned; otherwise a pointer to the
 * replied content from HTTP server. latter needs to be freed by caller.
 */
char *oauth_http_get2(const char *u, const char *q, const char *customheader)
{
	char *result = NULL;
	HTTPResponse *response = NULL;
	response = socket_http_get(u, q, customheader, NOT_KEEPALIVE);
	if( response ) {
		result = (char *)response->data;
		free(response->header);
		free(response);
	}
	return result;
}

/**
 * do a HTTP POST request, wait for it to finish 
 * and return the content of the reply.
 * (requires libcurl or a command-line HTTP client)
 *
 * more documentation in oauth.h
 *
 * @param u url to query
 * @param p postargs to send along with the HTTP request.
 * @return  In case of an error NULL is returned; otherwise a pointer to the
 * replied content from HTTP server. latter needs to be freed by caller.
 */
char *oauth_http_post(const char *u, const char *p)
{
	char *result = NULL;
	HTTPResponse *response = NULL;
	response = socket_http_post(u, p, strlen(p), NULL, NOT_KEEPALIVE);
	if( response ) {
		result = (char *)response->data;
		free(response->header);
		free(response);
	}
	return result;
}

/**
 * do a HTTP POST request, wait for it to finish 
 * and return the content of the reply.
 * (requires libcurl)
 *
 * more documentation in oauth.h
 *
 * @param u url to query
 * @param p postargs to send along with the HTTP request.
 * @param customheader specify custom HTTP header (or NULL for none)
 * @return  In case of an error NULL is returned; otherwise a pointer to the
 * replied content from HTTP server. latter needs to be freed by caller.
 */
char *oauth_http_post2(const char *u, const char *p, const char *customheader)
{
	char *result = NULL;
	HTTPResponse *response = NULL;
	response = socket_http_post(u, p, strlen(p), customheader, KEEPALIVE);
	if( response ) {
		result = (char *)response->data;
		free(response->header);
		free(response);
	}
	return result;
}

/**
 * http post raw data from file.
 * the returned string needs to be freed by the caller
 *
 * more documentation in oauth.h
 *
 * @param u url to retrieve
 * @param fn filename of the file to post along
 * @param len length of the file in bytes. set to '0' for autodetection
 * @param customheader specify custom HTTP header (or NULL for default)
 * @return returned HTTP reply or NULL on error
 */
char *oauth_post_file(const char *u, const char *fn, const size_t len, const char *customheader)
{
	char *result = NULL;
	HTTPResponse *response = NULL;
	response = socket_http_post_file(u, fn, len, customheader, KEEPALIVE);
	if( response ) {
		result = (char *)response->data;
		free(response->header);
		free(response);
	}
	return result;
}

/**
 * http post raw data.
 * the returned string needs to be freed by the caller
 *
 * more documentation in oauth.h
 *
 * @param u url to retrieve
 * @param data data to post along
 * @param len length of the file in bytes. set to '0' for autodetection
 * @param customheader specify custom HTTP header (or NULL for default)
 * @return returned HTTP reply or NULL on error
 */
char *oauth_post_data(const char *u, const char *data, size_t len, const char *customheader)
{
//	return oauth_curl_post_data (u, data, len, customheader);
	return NULL;
}

char *oauth_send_data(const char *u, const char *data, size_t len, const char *customheader, const char *httpMethod)
{
//	return oauth_curl_send_data (u, data, len, customheader, httpMethod);
	return NULL;
}

char *oauth_post_data_with_callback(const char *u, const char *data, size_t len, const char *customheader, void (*callback)(void*,int,size_t,size_t), void *callback_data)
{
//	return oauth_curl_post_data_with_callback(u, data, len, customheader, callback, callback_data);
	return NULL;
}
