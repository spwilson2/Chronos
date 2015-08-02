#ifndef _DIRENT_H_
#define _DIRENT_H_

#ifdef __cplusplus
extern "C" {
#endif

#undef FILE_MAX_NAME
#define FILE_MAX_NAME 255
#define MAXNAMLEN FILE_MAX_NAME

struct chronos_dirent {
        ino_t d_ino; /* inode number */
        off_t d_off; /* next directory index number */
        unsigned short d_reclen; /* The length of this structure */
        unsigned short d_type; /* File type (fs specific) */
        char name[FILE_MAX_NAME]; /* The name of the directory */
};

/**
 * Older version of the linux directory entry structure.
 */
struct old_linux_dirent
{
        long d_ino; /* inode number*/
        off_t d_off; /* Offset into the directory file */
        ushort d_reclen; /* Max size of d_name */
        char d_name[FILE_MAX_NAME]; /* Name of the entry */
};

typedef struct
{
	int dd_fd; /* Directory file descriptor */
	int dd_loc; /* Position in buffer */
	int dd_seek;
	char* dd_buff; /* buffer */
	int dd_len; /* buffer length */
	int dd_size; /* amount of data in the buffer */
} DIR;

#undef HAVE_DD_LOCK
#define HAVE_NO_D_NAMLEN

/**
 * Reads a directory entry from the file fd into dirp. 1 is returned
 * on success, 0 is returned on end of directory. -1 is returned on
 * failure. Count is ignored (compatibility).
 */
int readdir(int fd, struct old_linux_dirent* dirp, uint count);

/**
 * Better method of obtaining directory entries. Count specifies how
 * many entries can be read at a time into the dirp buffer from the open
 * file descriptor fd. Returns the number of bytes read on success, 0
 * on end of diretory and -1 on error.
 */
int getdents(int fd, struct chronos_dirent* dirp, uint count);

/**
 * Open a diretory for reading.
 */
DIR* opendir(const char* dir);

/**
 * Close a directory.
 */
int closedir(DIR* dir);

#ifdef __cplusplus
}
#endif

#endif
