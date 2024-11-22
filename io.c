#include <unistd.h>
#include <errno.h>
#include "io.h"

/* keeps calling `write` while there are bytes remaining to be written, until
   all bytes are written, or an error occurs. */
ssize_t write_all ( int fd, void *bf, size_t n) 
{
    ssize_t w_tot = 0; // bytes written in total
    ssize_t w_cur = 0; // bytes written in current iteration

    while ( w_tot < n ) {
	/* "Kernel, please (attempt to) write `n` bytes from `bf`, to `fd`."
	   https://man7.org/linux/man-pages/man2/write.2.html (a system call) */
	w_cur = write ( fd, bf, n - w_tot );
	/* Did anything get written? */
	if ( w_cur <= 0 ) {
	    /* no. why not? */
	    if ( errno == EINTR ) {
		/* `write` got interupted by signal handler before 
		   any data got transferred. try again. */
		continue;
	    } else {
		/* `write` failed for some one of many other (ca. 12)
		   different reasons. we assume fatal (see `errno`) */
		return -1;
	    }
	}
	/* `write` wrote some (if not all) the bytes. */
	w_tot += w_cur;
	bf    += w_cur;
    }
    return w_tot; // success (w_tot = n)
}

int read_line ( int fd, char* bf )
{
    int n = 0;     // number of characters read, in total
    int returnval; // return value of system call (number, or a return code)
    /* Read from fd, into bf, 1 byte at a time, until \n is found. */
    do {
	/* "Kernel, read from fd, into bf, at most 1 byte."
	   NOTE: each read blocks until bytes are available in fd.
	   NOTE: return value 0 indicates EOF. return value < 0 is an error.
	   NOTE: we /should/ reduce number of system calls, by reading all bytes 
	   available in fd each iteration, and search for a \n in bytes read. 
	   however, our approach simplifies multi-line reads (less bookkeeping).
	   https://man7.org/linux/man-pages/man2/read.2.html (a system call) */
	returnval = read(fd, bf, 1);
	// In case of error, return the return-code to the caller.
	if ( returnval <= 0 ) { return returnval ; }
	n++;
	// If I just read a \n, then return number of bytes read.
	if ( *bf == '\n' ) { return n; }
	// Otherwise we continue: point to the next byte in bf, and repeat.
	bf++;
    } while ( n < MAX_LINE );
    return 0; // no newline found.
}
