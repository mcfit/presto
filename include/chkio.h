#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#ifdef USE_PIOFS
#include <sys/limits.h>
#include <piofs/piofs_ioctl.h>
#else
#include <sys/stat.h>
#endif

FILE *chkfopen(char *path, const char *mode);
/* Preform a file open with error checking.  */

int chkfread(void *data, size_t type, size_t number, FILE * stream);
/* Preform a file read with error checking.  */

int chkfwrite(void *data, size_t type, size_t number, FILE * stream);
/* Preform a file write with error checking. */

int chkfseek(FILE * stream, long offset, int whence);
/* NOTE:  This is meant only for backwards compatibility.  */
/* You should probably be calling chkfileseek() directly.  */

int chkfileseek(FILE * stream, long offset, size_t size, int whence);
/* Preform a file seek with error checking.  */

unsigned long chkfilelen(FILE *file, size_t size);
/* Return the length of a file (in blocks of 'size').  */
