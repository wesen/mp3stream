/*C
  (c) 2003 Institut fuer Telematik, Universitaet Karlsruhe
**/

#ifndef FILE_H__
#define FILE_H__

#include <sys/types.h>

/*M
  \emph{Error returned by file_read and file_write.}
**/

#define EEOF (-1)
#define ESYNC (-2)

#ifdef __cplusplus
extern "C" {
#endif

typedef struct file_s {
  /*M
    File descriptor.
  **/
  int fd;
  /*M
    Offset into file.
  **/
  unsigned long offset;
  /*M
    Size of file.
  **/
  unsigned long size;
  unsigned short maxsync;
} file_t;

int file_open_read(file_t *file, const char *filename);
int file_open_write(file_t *file, const char *filename);
int file_open_fd(file_t *file, int fd);
int file_close(file_t *file);
int file_read(file_t *file, unsigned char *buf, size_t size);
int file_seek_fwd(file_t *file, size_t size);
int file_write(file_t *file, unsigned char *buf, size_t size);

#ifdef __cplusplus
}
#endif
  
#endif /* FILE_H__ */

/*C
  **/
