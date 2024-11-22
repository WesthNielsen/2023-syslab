/* String constants */
static const char *REQUEST_LINE_FMT =
    "GET %s HTTP/1.0\r\n";
static const char *USER_AGENT_FLD =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *HOST_FLD_FMT =
    "Host: %s：%s\r\n";
static const char *CONNECTION_FLD =
    "Connection: close\r\n";
static const char *PROXY_CONNECTION_FLD =
    "Proxy-Connection: close\r\n";
static const char *BLANK_LINE =
    "\r\n";

#include <string.h>
#include <stdio.h>

#include "http.h"  // http-related things for ^
#include "io.h"
#include "error.h"

/* HTTP-related helper functions. You are not expected to know HTTP in
   detail, and you are not expected to modify this! However, you are
   expected to understand what is going on here.*/

/* compile a request header from fields provided by the client, as well as 
 * hostname, path and port. write the resulting header to request_hdr. */
int set_request_header ( char* request_hdr, char* hostname, char* path, char* port, int client_fd )
{
    /* an HTTP request header consists of a request line, followed by header fields.
       each header field is a key-value pair of the form `k: v\r\n`. */
    char request_line[MAX_LINE];// request line (first line of a request header)
    char host_fld[MAX_LINE];    // host field
    char other_flds[MAX_LINE];  // other fields
    
    char line[MAX_LINE];        // a buffer for storing lines read from client_fd
    int return_cd;              // return code for reads from client_fd
    
    /* Proxy sets request line (We only handle GET requests, in HTTP/1.0.) */
    sprintf(request_line, REQUEST_LINE_FMT, path);

    /* Proxy sets `User-Agent`, `Connection`, and `Proxy-Connection` fields;
       see proxy.h for their values. */
    
    /* Default host field, in case client request does not contain one. */
    sprintf(host_fld, HOST_FLD_FMT, hostname, port);

    /* Get any other fields from client_fd */
    return_cd = 1;
    while ( return_cd > 0 )
    {
	/* read the next line. */
	return_cd = read_line ( client_fd, line );
	if ( error_read ( return_cd ) ) { return 0; /*error*/ }

	/* null-terminate the string read from client_fd.
	   NOTE: by doing this, we are ignoring an edge case where
	   the line read has length MAX_LEN */
	line[return_cd] = '\0';
	
	/* if we reached end-of-client-request, then stop reading from client_fd. */
        if ( strncasecmp ( line, BLANK_LINE, strlen(BLANK_LINE) ) == 0  ) break;

	/* if client provided a host field, then we use client's host field. */
        if ( strncasecmp ( line, "Host:", strlen("Host:") ) == 0 )
	{
            strcpy ( host_fld, line );
            continue;
        }

	/* if client provides `User-Agent`, `Connection`, and `Proxy-Connection` 
	   fields, then we ignore them. (we use our own hard-coded such fields). */
        if( strncasecmp ( line, "User-Agent:", strlen("User-Agent:") ) == 0 ||
	    strncasecmp ( line, "Connection:", strlen("Connection:") ) == 0 ||
	    strncasecmp ( line, "Proxy-Connection:", strlen("Proxy-Connection:") ) == 0 )
	{
	    continue;
        }

	/* otherwise, this field is a keeper. */
	strcat( other_flds, line );
    }
    
    /* set the request header. */
    request_hdr[0] = '\0';
    strcat ( request_hdr, request_line );
    strcat ( request_hdr, host_fld );
    strcat ( request_hdr, USER_AGENT_FLD );
    strcat ( request_hdr, other_flds );
    strcat ( request_hdr, CONNECTION_FLD );
    strcat ( request_hdr, PROXY_CONNECTION_FLD );
    strcat ( request_hdr, BLANK_LINE );

    /* success. */
    return 1;
}

/* parse the uri into hostname, path, and port. */
void parse_uri(char* uri, char* hostname, char* path, char* port)
{
    char buf[MAX_LINE];
    /*position to "//", the first"/", ":" */
    char* pstart, *ppath, *pport;

    strcpy(buf, uri);

    pstart = strstr(buf, "//") + 2;
    ppath = strchr(pstart, '/');
    if(!ppath){
        strcpy(path, "/");
    }else{
        strcpy(path, ppath);
        *ppath = 0;
    }

    pport = strchr(pstart, ':');
    if ( ! pport ) {
        strcpy(port, "80");
        strcpy(hostname, pstart);
    } else {
        strcpy(port, pport+1);
        *pport = 0;
        strcpy(hostname, pstart);
    }
}
