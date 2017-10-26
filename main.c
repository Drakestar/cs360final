#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>
#include "type.h"
#include "util.c"

// globals
MINODE minode[NMINODE];       
MINODE *root;
PROC   proc[NPROC], *running;
MTABLE mtable[4]; 

SUPER *sp;
GD    *gp;
INODE *ip;

char buf[BLKSIZE]

int dev;
/**** same as in mount table ****/
int nblocks; // from superblock
int ninodes; // from superblock
int bmap;    // bmap block 
int imap;    // imap block 
int iblock;  // inodes begin block


void init() 
{
	proc[0].uid = 0;
	proc[1].uid = 1;
	for (int i = 0; i < NPROC; i++) 
	{
		proc[NPROC].cwd = 0;
	}
	for (i = 0; i < 100; i++) 
	{
		minode[i].refCount = 0;
	}
	root = 0;
}

void mount_root(char *device) 
{
	
	fd = open(device, O_RDWR);
	if (fd < 0) 
	{
		printf("failed\n");
		exit(1);
	}
	get_block(fd, 1, buf);
	sp = (SUPER *)buf;
	get_block(fd, 2, buf);
	gp = (GD *)buf;
	printf("%-30s = %8x ", "s_magic", sp->s_magic);
	if (sp->s_magic != 0xEF53) 
	{
		printf("Not an ext2 FS\n");
		exit(2);
	}
	
	root = iget(dev, 2);
	
	mtable[0].dev = fd;
	mtable[0].ninodes = sp->s_inodes_count;
	mtable[0].nblocks = sp->s_blocks_count;
	mtable[0].bmap = gp->bg_block_bitmap;
	mtable[0].imap = gp->bg_inode_bitmap;
	mtable[0].iblock = gp->bg_inode_table;
	mtable[0].mounted_inode = root;
	mtable[0].deviceName = device;
	mtable[0].mountedDirName = "/";
	
	proc[0].cwd = iget(dev, 2);
	proc[1].cwd = iget(dev, 2);
	
	running = &proc[0];

}

void ls_dir(char *dirname) {
	int ino = getino(dirname);
	MINODE *mip = iget(dev, ino);
	get_block(mtable[0].dev, mtable[0].iblock, buf);
	INODE *ip = (INODE *)buf;
	ip++;
	get_block(mtable[0].dev, ip->iblock[0], buf);
	DIR *dp = (DIR *)buf;
	char *cp = buf;
	while (cp < buf + BLKSIZE) {
		strncpy(temp, dp->name, dp->name_len);
		temp[dp->name_len] = 0;
		cp += dp->rec_len;
		dp = (DIR *)cp;
		ls_file(dp.inode);
	}
	
}

void ls_file(int ino) {
	INODE *mip = iget(dev, ino);
	printf("%d\n", mip->mode);
}

void ls(char *pathname) 
{
	// Split pathname into child and parent
	// Check parent exists and is dir
	//
}

void quit() {
	//iput() all minodes with (refCount > 0 && DIRTY);
	for (int i = 0; i < NMINODES; i++) {
		if ((minode[i].refCount > 0) && (minode[i].dirty)) {
			iput(minode[i]);
		}
	}
	exit(0);
}

int main(int argc, char *argv[]) 
{
	char *device = "mydisk";
	if (argc > 1) {
		device = argv[1];
	}
	init();
	mount_root(device);
	
	// Main loop
	while(1) 
	{
		printf("Commands: [ls|cd|pwd|quit]\ninput command:");
		fgets(line, 128, stdin);
	}
}
