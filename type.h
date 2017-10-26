/*	type.h for CS360 Project             */

#include <stdio.h>
#include <stdlib.h>

#include <fcntl.h>
#include <ext2fs/ext2_fs.h>
#include <libgen.h>
#include <string.h>
#include <sys/stat.h>

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

// define shorter TYPES, save typing efforts
typedef struct ext2_group_desc  GD;
typedef struct ext2_super_block SUPER;
typedef struct ext2_inode       INODE;
typedef struct ext2_dir_entry_2 DIR;    // need this for new version of e2fs

GD    *gp;
SUPER *sp;
INODE *ip;
DIR   *dp; 

#define BLOCK_SIZE        1024
#define BLKSIZE           1024

#define BITS_PER_BLOCK    (8*BLOCK_SIZE)
#define INODES_PER_BLOCK  (BLOCK_SIZE/sizeof(INODE))

// Block number of EXT2 FS on FD
#define SUPERBLOCK        1
#define GDBLOCK           2
#define ROOT_INODE        2

// Default dir and regulsr file modes
#define DIR_MODE          0x41ED
#define FILE_MODE         0x81A4
#define SUPER_MAGIC       0xEF53
#define SUPER_USER        0

// Proc status
#define FREE              0
#define BUSY              1
#define KILLED            2

// Table sizes
#define NMINODES         100
#define NMOUNT            10
#define NPROC              2
#define NFD               16
#define NOFT              50

// Open File Table
typedef struct Oft{
  int   mode;
  int   refCount;
  struct Minode *inodeptr;
  long  offset;
} OFT;

// PROC structure
typedef struct Proc{
  struct Proc *next;  // for PROC link list 
  int   uid;          // uid = 0 or nonzero  
  int   pid;          // pid = 1, 2
  int   gid;          
  int   ppid;         
  int   status;       
  struct Minode *cwd; // CWD pointer
  OFT   *fd[NFD];     // file descriptor array
} PROC;
      
// In-memory inodes structure
typedef struct Minode{		
  INODE INODE;        // disk inode
  int   dev, ino;
  int   refCount;
  int   dirty;
  int   mounted;          // mounted on flag  
  struct Mount *mountptr; // mount table pointer if mounted on
}MINODE;

// Mount Table structure
typedef struct Mount{
  int    dev;       // dev (opened vdisk fd number) 
  int    busy;      // FREE or in-use
  int    ninodes;   // from superblock
  int    nblocks;
  int    bmap;      // from GD block  
  int    imap;
  int    iblock;
  struct Minode *mounted_inode;
  char   deviceName[64];  // device name, e.g. mydisk
  char   mountedDirName[64]; // mounted DIR pathname
} MOUNT;
