/**
 * @author Jonas Westh Nielsen <jwni@itu.dk>
 */

#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

/* The source code for the proxy is split across three files (including this one). */
#include "proxy.h" // proxy
#include "error.h" // error reporting for ^
#include "http.h"  // http-related things for ^
#include "io.h"    // io-related things for ^

void *handle_connection_request(void *arg);

int main(int argc, char **argv)
{
    int listen_fd; // fd for connection requests from clients.

    pCache = malloc(sizeof(cache_t));
    pCache->head = NULL;
    pCache->tail = NULL;

    pthread_rwlock_init(&readWriteLock, NULL);
    /* Check command line args for presence of a port number. */
    if (error_args_fatal(argc, argv))
    {
        exit(1);
    }

    /* Create a `socket`, `bind` it to listen address, configure it to `listen` (for connection requests). */
    listen_fd = create_listen_fd(atoi(argv[1]));

    /* Handle connection requests. */
    while (1)
    {
        int *client_fd = malloc(sizeof(int));
        *client_fd = accept(listen_fd, (struct sockaddr *)NULL, NULL);
        if (client_fd >= 0)
        {
            pthread_t thread;
            pthread_create(&thread, NULL, handle_connection_request, (void *)client_fd);
        }
    }

    return 0; // Indicates "no error" (although this is never reached).
}

void *handle_connection_request(void *arg)
{
    pthread_detach(pthread_self());
    int client_fd = *((int *)arg);
    free(arg);
    int return_cd; // return- (aka. error-) code of function calls.

    printf("\e[1mawaiting connection request...\e[0m\n");

    /* "Kernel, give me the fd of a connected socket for the next connection request."
       NOTE: this blocks the proxy until a connection arrives.
       https://man7.org/linux/man-pages/man2/accept.2.html (a system call) */
    if (error_accept_fatal(client_fd))
    {
        exit(1);
    }
    if (error_accept(client_fd))
    {
        return NULL;
    }

    /* Handle (presumably, a HTTP GET) request. */
    handle_request(client_fd);

    /* "Kernel, we done handling request; close fd." (errors ignored; see man page)
       https://man7.org/linux/man-pages/man2/close.2.html (a system call) */
    return_cd = close(client_fd);
    if (error_close(return_cd))
    {
        /* ignore */
    }

    printf("\e[1mfinished processing request.\e[0m\n");
    pthread_exit(NULL);
}

void handle_request(int client_fd)
{
    int server_fd; // server file descriptor

    /* String variables */
    char buf[MAX_LINE];
    char method[MAX_LINE];
    char uri[MAX_LINE];
    char version[MAX_LINE];
    char hostname[MAX_LINE];
    char path[MAX_LINE];
    char port[MAX_LINE];
    char request_hdr[MAX_LINE];
    char buffer[MAX_OBJECT_SIZE];

    int return_cd;
    ssize_t transferedData;
    ssize_t num_bytes;

    num_bytes = read_line(client_fd, buf);
    if (error_read(num_bytes))
    {
        return;
    }

    /* print what we just read (it's not null-terminated) */
    printf("%.*s", (int)num_bytes, buf); // typeast is safe; num_bytes <= MAX_LINE
    sscanf(buf, "%s %s %s", method, uri, version);

    /* Ignore non-GET requests (your proxy is only tested on GET requests). */
    if (error_non_get(method))
    {
        return;
    }

    /* Parse URI from GET request */
    parse_uri(uri, hostname, path, port);

    if (pCache->head != NULL)
    {
        char *cachebuf = readFromCache(uri);
        if (cachebuf != NULL)
        {
            num_bytes = write_all(client_fd, cachebuf, strlen(cachebuf));
            if (error_write_client(client_fd, num_bytes))
            {
                return;
            }
            cacheNode_n *node = getNode(uri);
            pthread_rwlock_wrlock(&readWriteLock);
            alterCache(node);
            pthread_rwlock_unlock(&readWriteLock);
            return;
        }
    }

    /* Set the request header */
    return_cd = set_request_header(request_hdr, hostname, path, port, client_fd);
    if (error_header(return_cd))
    {
        return;
    }

    /* Create the server fd. */
    server_fd = create_server_fd(hostname, port);
    if (error_socket_server(server_fd))
    {
        return;
    }

    /* Write the request (header) to the server. */
    return_cd = write_all(server_fd, request_hdr, strlen(request_hdr));
    if (error_write_server(server_fd, return_cd))
    {
        return;
    }

    do
    {
        num_bytes = read(server_fd, buf, MAX_LINE);
        if (error_read_server(server_fd, num_bytes))
        {
            return;
        }
        num_bytes = write_all(client_fd, buf, num_bytes);
        if (error_write_client(client_fd, num_bytes))
        {
            return;
        }
        if (transferedData + num_bytes <= MAX_OBJECT_SIZE)
        {
            memcpy(buffer + transferedData, buf, num_bytes);
            transferedData += num_bytes;
        }
    } while (num_bytes > 0);

    if (transferedData <= MAX_OBJECT_SIZE)
    {
        pthread_rwlock_wrlock(&readWriteLock);
        writeToCache(buffer, transferedData, uri);
        pthread_rwlock_unlock(&readWriteLock);
    }

    /* Transfer the response from the server, to the client.
       (until server responds with EOF). */
    /* TODO insert code here! (ballpark: 6 lines +/-)
       working solution is in p1-sol.txt
       try writing your own code; use p1-sol.txt only
       if you are hard-stuck! */

    /* success; close the file descrpitor. */
    return_cd = close(server_fd);
    if (error_close_server(return_cd))
    {
    }
}

int create_listen_fd(int port)
{
    /* File descriptors */
    int listen_fd; // fd for connection requests from clients.

    /* Return code */
    int return_cd;

    /* Socket address (on which proxy shall listen for connection requests) (populated soon).
       https://man7.org/linux/man-pages/man3/sockaddr.3type.html */
    struct sockaddr_in listen_addr;

    printf("\e[1mcreating listen_fd\e[0m\n");

    /* Set socket address (on which proxy shall listen for connection requests). */
    set_listen_socket_address(&listen_addr, port);

    /* "Kernel, make me a socket." (for listening to client connection requests).
       https://man7.org/linux/man-pages/man2/socket.2.html (a system call) */
    listen_fd = socket(listen_addr.sin_family, SOCK_STREAM, 0);
    if (error_socket_fatal(listen_fd))
    {
        exit(1);
    }

    /* "Kernel, if you think the address I'm binding to is already in use, then
       this socket may reuse the address." (optional)
       NOTE: quality-of-life; it takes kernel ~1 min to free up an address; w/o
       this, after proxy stopped, you have to wait a bit before you can start again).
       https://man7.org/linux/man-pages/man2/setsockopt.2.html (a system call) */
    return_cd = setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
    if (error_socket_option(return_cd))
    {
        /* ignore */
    }

    /* "Kernel, bind it to this socket address" (i.e. where proxy shall listen).
       https://man7.org/linux/man-pages/man2/bind.2.html (a system call) */
    return_cd = bind(listen_fd, (struct sockaddr *)&listen_addr, sizeof(listen_addr));
    if (error_bind_fatal(return_cd))
    {
        exit(1);
    }

    /* "Kernel, oh btw, that socket? Make it passive." (it's for connection requests)
       https://man7.org/linux/man-pages/man2/listen.2.html (a system call) */
    return_cd = listen(listen_fd, LISTENQ);
    if (error_listen_fatal(return_cd))
    {
        exit(1);
    }

    printf("\e[1mlisten_fd ready\e[0m\n");

    return listen_fd;
}

/* set the proxy socket address (where it listens for connection requests). */
void set_listen_socket_address(struct sockaddr_in *listen_addr, int port)
{
    memset(listen_addr, '0', sizeof(struct sockaddr_in)); // zero out the address
    listen_addr->sin_family = AF_INET;
    listen_addr->sin_addr.s_addr = htonl(INADDR_ANY);
    listen_addr->sin_port = htons(port);
    /* NOTE: we /should/ use `getnameinfo` & `getaddrinfo` (in real world, so should you).
       with `getaddrinfo`, we get a list of potential socket addresses, and for each
       socket address in the list, we should attempt to create + bind a socket to it
       (stopping on the first successful `socket` (i.e. create)  and 'bind'). why:
        * more robust (can bind to 32-bit and 256-bit addresses, whichever server has)
        * more secure (an attacker on `cos` cannot hijack this socket by
                       binding to a more specific address than INADDR_ANY).
       instead, here we hard-code port, pick 32-bit IP addresses, and all available interfaces.
       why: because I know cos supports this, and it is simpler; `getaddrinfo` is
       intimidating for the uninitiated. (why: check out the server socket code.) */
    printf("\033[32msuccess:\033[0m set socket address of proxy.\n");
}

int create_server_fd(char *hostname, char *port)
{
    int server_fd;
    int return_cd;

    struct addrinfo *cand_ai; // pointer to heap-allocated candidate server addresses (free this!)

    /* Get list of candidate server socket addresses. */
    return_cd = get_server_socket_address_candidates(&cand_ai, hostname, port);
    if (error_address_server(return_cd))
    {
        return -1;
    }

    struct addrinfo *curr_ai; // pointer to current candidate server address in the above list.

    /* produces a socket (server_fd) bound to the first candidate address (in cand_ai)
       for which creating (resp. binding) a socket for (resp. to) it was successful. */
    for (curr_ai = cand_ai; curr_ai != NULL; curr_ai = curr_ai->ai_next)
    {
        /* "Kernel, make me a socket." (for curr_ai)
           https://man7.org/linux/man-pages/man2/socket.2.html (a system call) */
        printf("HERE\n");
        server_fd = socket(curr_ai->ai_family, curr_ai->ai_socktype, curr_ai->ai_protocol);
        printf("HERE\n");
        if (server_fd == -1)
            continue; // try the next ai.

        /* "Kernel, please (attempt to) connect to said socket."
           https://man7.org/linux/man-pages/man2/connect.2.html (a system call) */
        return_cd = connect(server_fd, curr_ai->ai_addr, curr_ai->ai_addrlen);
        // return_cd = connect ( server_fd, (struct sockaddr *)&curr_ai, sizeof(curr_ai) );
        if (return_cd < 0)
        {
            printf("failure connecting to socket. trying next one.\n");
        }
        if (return_cd == 0)
            break; // success

        /* couldn't bind the socket to curr_ai. try the next ai. */
        close(server_fd);
    }
    /* free up the heap-allocated linked list. */
    freeaddrinfo(curr_ai);

    /* report errors if any. */
    if (return_cd < 0)
    {
        return -1;
    }

    /* success; return the server fd. */
    return server_fd;
}

int get_server_socket_address_candidates(struct addrinfo **cand_ai, char *hostname, char *port)
{
    struct addrinfo hints_ai; // hints for proposing candidate server addresses (i.e. for generating cand_ai)
    /* set hints. network socket, numeric port, avoid IPv6 socket for hosts that don't support those. */
    memset(&hints_ai, 0, sizeof(struct addrinfo));
    hints_ai.ai_socktype = SOCK_STREAM;
    hints_ai.ai_flags = AI_NUMERICSERV | AI_ADDRCONFIG;
    return getaddrinfo(hostname, port, &hints_ai, cand_ai);
}

char *readFromCache(char *uri)
{
    cacheNode_n *temp = pCache->head;

    while (temp != NULL)
    {
        if (strcmp(temp->URI, uri) == 0)
        {
            return temp->Data;
        }
        else
        {
            temp = temp->next;
        }
    }

    return NULL;
}

void writeToCache(char *data, int dataSize, char *uri)
{
    if (pCache->Size + dataSize > MAX_CACHE_SIZE)
    {
        removeCacheElement();
    }
    
    cacheNode_n *newN;
    newN = malloc(sizeof(cacheNode_n));
    newN->Data = strdup(data);
    newN->URI = strdup(uri);
    newN->next = NULL;
    newN->previous = NULL;

    if (pCache->head != NULL)
    {
        newN->next = pCache->head;
        pCache->head->previous = newN;
    }

    if (pCache->tail == NULL)
    {
        pCache->tail = newN;
    }

    pCache->head = newN;
    pCache->Size += dataSize;
}

void removeCacheElement()
{
    if (pCache->tail != NULL)
    {
        cacheNode_n *nodeToRemove = pCache->tail;
        pCache->tail = pCache->tail->previous;

        if (pCache->tail != NULL)
        {
            pCache->tail->next = NULL;
        }

        free(nodeToRemove->URI);
        free(nodeToRemove->Data);
        free(nodeToRemove);
    }
}

void alterCache(cacheNode_n *node)
{
    if (node->next != NULL)
    {
        if (pCache->head == node)
        {
            node->next->previous = node;
        }
        else
        {
            node->next->previous = node->previous;
        }
    }

    if (node->previous != NULL)
    {
        node->previous->next = node->next;
        node->next = pCache->head;
        pCache->head->previous = node;
    }

    if (pCache->tail == node && pCache->head != node)
    {
        pCache->tail = node->previous;
    }

    node->previous = NULL;
    pCache->head = node;
}

cacheNode_n *getNode(char *uri)
{
    cacheNode_n *temp = pCache->head;
    while (temp->next != NULL)
    {
        if (strcmp(temp->URI, uri) == 0)
        {
            return temp;
        }

        else
        {
            temp = temp->next;
        }
    }

    if (strcmp(temp->URI, uri) == 0)
    {
        return temp;
    }

    return NULL;
}