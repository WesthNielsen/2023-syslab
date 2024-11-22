#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>

int error_args_fatal ( int argc, char **argv )
{
    if ( argc != 2 ) {
	fprintf(stderr, "usage: %s <port>\n", argv[0]);
	return 1;
    } // assumption: the argument provided, is a valid port number.
    return 0;
}

int error_socket_fatal( int returncode )
{
    if ( returncode < 0 ) {
	fprintf(stderr, "\033[31mfailure:\033[0m create socket. fatal.\n");
	return 1;
    }
    printf("\033[32msuccess:\033[0m create socket.\n");
    return 0;
}

int error_socket_option( int returncode )
{
    if ( returncode < 0 ) {
	fprintf(stderr, "\033[31mfailure:\033[0m set socket option. let us try and proceed anyway.\n");
	return 1;
    }
    printf("\033[32msuccess:\033[0m set socket option.\n");
    return 0;
}

int error_socket_server( int server_fd )
{
    if ( server_fd < 0 ) {
	fprintf(stderr, "\033[31mfailure:\033[0m create server socket & connect. dropping requets.\n");
	return 1;
    }
    printf("\033[32msuccess:\033[0m create server socket & connect.\n");
    return 0;
}

int error_bind_fatal( int returncode )
{
    if ( returncode < 0 ) {
	fprintf(stderr, "\033[31mfailure:\033[0m bind socket to address. fatal.\n");
	return 1;
    }
    printf("\033[32msuccess:\033[0m bind socket to address.\n");
    return 0;
}

int error_listen_fatal( int returncode )
{
    if ( returncode < 0 ) {
	fprintf(stderr, "\033[31mfailure:\033[0m listen to socket. fatal.\n");
	return 1;
    }
    printf("\033[32msuccess:\033[0m listen to socket.\n");
    return 0;
}


int error_accept_fatal ( int returncode ) {
    if ( returncode < 0 ) {
	// error occurred. was it a bad one?
	if ( ! ( errno == ENETDOWN   || errno == EPROTO || errno == ENOPROTOOPT  ||
		 errno == EHOSTDOWN  || errno == ENONET || errno == EHOSTUNREACH ||
		 errno == EOPNOTSUPP || errno == ENETUNREACH ) ) {
	    // it's a bad one; terminate.
	    fprintf(stderr, "\033[31mfailure\033[0m to accept connection. fatal.\n");
	    return 1;
	}
    }
    return 0;
}

int error_accept ( int client_fd ) {
    if ( client_fd < 0 ) {
	fprintf(stderr, "\033[31mfailure\033[0m to accept connection. retrying.\n");
	return 1;
    }
    printf("\033[32maccepted\033[0m connection request.\n");
    /* From whom? We don't need to know; we have a socket to reply to them.
       while you /should/ log client hostname & port in a production system,
       I left that out for brevity (finding out: `getnameinfo`, >= 20 LoC) */
    return 0;
}

int error_close ( int returncode ) {
    if ( returncode < 0 ) {
	fprintf(stderr, "\033[31mfailure:\033[0m close client connection. ignoring that.\n");
	return 1;
    }
    printf("\033[32msuccess:\033[0m close client connection.\n");
    return 0;
}

int error_close_server ( int returncode ) {
    if ( returncode < 0 ) {
	fprintf(stderr, "\033[31mfailure:\033[0m close server connection. ignoring that.\n");
	return 1;
    }
    printf("\033[32msuccess:\033[0m close server connection.\n");
    return 0;
}

int error_close_candidate ( int returncode ) {
    if ( returncode < 0 ) {
	fprintf(stderr, "\033[31mfailure:\033[0m close candidate server socket. ignoring that.\n");
	return 1;
    }
    return 0;
}


int error_read ( int n ) {
    if ( n == 0 ) {
	fprintf(stderr, "\033[31mfailure:\033[0m end of client fd (EOF) reached prematurely. dropping request.\n");
	return 1;
    } else
    if ( n  < 0 ) {
	fprintf(stderr, "\033[31mfailure:\033[0m error reading client fd. dropping request.\n");
	return 1;
    }
    printf("read  %*d bytes from client.\n", 4, n );
    return 0;
}

int error_read_server ( int server_fd, int n ) {
    if ( n  < 0 ) {
	fprintf(stderr, "\033[31mfailure:\033[0m error reading server fd. dropping request.\n");
	n = close ( server_fd );
	if ( error_close_server ( n ) ) { /* ignored */}
	return 1;
    }
    if ( n == 0 ) {
	printf("reached end of server fd (EOF).\n");
	return 0;
    }
    printf("read  %*d bytes from server.\n", 4, n );
    return 0;
}

int error_write_server ( int server_fd, int n ) {
    if ( n < 0 ) {
	fprintf(stderr, "\033[31mfailure:\033[0m write to server fd. %s. dropping request.\n", strerror( errno ) );
	n = close ( server_fd );
	if ( error_close_server ( n ) ) { /* ignored */}
	return 1;
    }
    printf("wrote %*d bytes to server.\n", 4, n );
    return 0;
}

int error_write_client ( int client_fd, int n ) {
    if ( n < 0 ) {
	fprintf(stderr, "\033[31mfailure:\033[0m error writing to client fd. dropping request.\n");
	n = close ( client_fd );
	if ( error_close ( n ) ) { /* ignored */}
	return 1;
    }
    printf("wrote %*d bytes to client.\n", 4, n );
    return 0;
}

int error_header ( int return_cd ) {
    if ( return_cd <= 0 ) {
	fprintf(stderr, "\033[31mfailure:\033[0m client request header was malformed.\n");
	return 1;
    } 
    printf("\033[32msuccess:\033[0m set request header.\n");
    return 0;
}

int error_non_get ( char *method ) {
    if (strcasecmp(method, "GET")) {
	fprintf(stderr, "\033[31mfailure:\033[0m not a GET-request. dropping request.\n");
        return 1;
    }
    printf("\033[32msuccess:\033[0m it is a GET request.\n");
    return 0;
}

int error_address_server ( int return_cd ) {
    if ( return_cd != 0 ) {
        fprintf(stderr, "\033[31mfailure:\033[0m %s\n", gai_strerror(return_cd));
        return 1;
    }
    printf("\033[32msuccess:\033[0m generate server addresses.\n");
    return 0;
}
