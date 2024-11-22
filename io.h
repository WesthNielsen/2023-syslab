#include <stddef.h>

#define MAX_LINE 8192 // HTTP Semantics (RFC 9110) recommends >= 8000 characters.

int read_line ( int fd, char* bf );
ssize_t write_all ( int fd, void *bf, size_t n) ;
