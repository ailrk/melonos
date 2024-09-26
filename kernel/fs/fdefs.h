#pragma once
#include <stdbool.h>
#include "fdefs.fwd.h"
#include "mutex.h"


/* Configurartion for the file system parameters */

#define BSIZE   512            // block size
#define NDEV    32             // max number of devices
#define NFILE   128            // max number of open filese in the system
#define NINODE  128            // max number of inodes
#define NOPBLKS 512            // max # of blocks any FS op writes
#define NBUF    (NOPBLKS * 5)  // max buffer size
#define NLOG    (NOPBLKS * 5)  // max log size
#define DIR_SZ  512            // max number of directories
#define MAXBLKS 1000           // max file system size



/* In disk representation of an inode */
typedef struct DInode {
    FileType        type;
    unsigned short  major; // major device number
    unsigned short  minor; // minor device number
    unsigned short  nlink; // number of links in fs
    unsigned        size;
    unsigned        addr;  // block address.
} DInode;


/* Memory representation of an inode */
typedef struct Inode {
    DevNum   dev;    // device number
    InodeNum inum;   // inode number
    int      nref;   // ref count
    bool     read;   // has been read from disk?
    DInode   dinode; // copy of disk inode.
} Inode;


/* Buffer cache node */
typedef struct BNode {
    struct BNode * next;
    struct BNode * prev;
    struct BNode * qnext; // next node on disk queue.
    Mutex          mutex;
    bool           dirty; // needs to be writtent to disk.
    bool           valid; // has been read from disk.
    unsigned       nref;
    DevNum         dev;
    unsigned       blockno;
    char           cache[BSIZE];
} BNode;


/* Devices need to implement this interface */
typedef struct Dev {
    int (*read)(Inode *ino, char * addr, int n);
    int (*write)(Inode *ino, char * addr, int n);
} Dev;


/* Directory entry */
typedef struct DirEntry {
  InodeNum  inum;
  char      name[DIR_SZ];
} DirEntry;


/* file status */
#define T_DIR  1 // directory
#define T_FILE 2 // file
#define T_DEV  3 // device

typedef struct Stat {
    short    type;  // file type
    DevNum   dev;   // disk device
    InodeNum inum;  // inode number
    short    nlink; // number of links
    unsigned size;  // file size
} Stat;
