/* Macro constants */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
#define LISTENQ 1024

#ifndef MAX_LINE
#define MAX_LINE 8192 // HTTP Semantics (RFC 9110) recommends >= 8000 characters.
#endif                /*MAX_LINE*/


typedef struct cacheNode
{
    char *URI;
    char *Data;
    struct cacheNode *next;
    struct cacheNode *previous;
} cacheNode_n;

typedef struct cache
{
    int Size;
    cacheNode_n *head;
    cacheNode_n *tail;
} cache_t;

cache_t *pCache;
pthread_rwlock_t readWriteLock;


void handle_request(int fd);
int create_listen_fd(int port);
void get_client_socket_address(struct sockaddr *client_addr, char *hostname, char *port);
void set_listen_socket_address(struct sockaddr_in *listen_addr, int port);
int get_server_socket_address_candidates(struct addrinfo **cand_ai, char *hostname, char *port);
int create_server_fd(char *hostname, char *port);
char *readFromCache(char uri[]);
void writeToCache(char data[], int dataSize, char uri[]);
void removeCacheElement();
void alterCache(cacheNode_n *node);
cacheNode_n *getNode(char *uri);