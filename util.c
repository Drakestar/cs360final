#include "type.h"

// ALL VARIABLES
MINODE minode[NMINODES];       
MINODE *root;
PROC   proc[NPROC], *running;
MOUNT mtable[4]; 

SUPER *sp;
GD    *gp;
INODE *ip;

char buf[BLKSIZE];
char *cmd;
char *tok;

int dev;
int fd;
/**** same as in mount table ****/
int nblocks; // from superblock
int ninodes; // from superblock
int bmap;    // bmap block 
int imap;    // imap block 
int iblock;  // inodes begin block

/********** Functions as BEFORE ***********/
// From the file descriptor move forward blk amount absolutely
// Then read the next BLKSIZE amount into the buffer
int get_block(int fd, int blk, char *buf) {
  lseek(fd, (long)blk*BLKSIZE, SEEK_SET);
  return read(fd, buf, BLKSIZE);
}

int put_block(int dev, int blk, char buf[ ]) 
{
	printf("Nothing here yet");
}

// load INODE of (dev,ino) into a minode[]; return mip->minode[]
MINODE *iget(int dev, int ino)
{
	// SEEK VALUE
	// ino - 1 / INODESPERBLOCK +inode_table * BLKSIZE + ino-1 % INOPERBLK * BLKSIZE/INOPERBLK
	int seek = ((ino-1)/8 + mtable[0].iblock) * 1024 + (ino-1)%8 * 128;
	int i;
	INODE iptr;
	char buf[1024];
	// Get inode pointer
	lseek(dev, seek, SEEK_SET);
	read(dev, &iptr, sizeof(INODE));
	
     //(1). search minode[ ] array for an item pointed by mip with the SAME (dev,ino)
     for (i=0; i < NMINODES; i++) {
		 if (minode[i].inode.i_block[0] == iptr.i_block[0]) {
			 minode[i].refCount++;
			 return &minode[i];
		 }
	 }
	 
	 //(2). search minode[ ] array for a mip whose refCount=0:
     for (i = 0; i < NMINODES; i++) {
		 if (minode[i].inode.i_size == 0) {
			 minode[i].refCount = 1;
			 minode[i].dev = dev;
			 minode[i].ino = ino;
			 minode[i].dirty = 0;
			 minode[i].mounted = 0;
			 minode[i].mountptr = 0;
			 break;
		 }

	 }
	 // Copy iptr into minode
     memcpy(&minode[i].inode, &iptr, sizeof(INODE));
     //copy INODE into minode.INODE by then set
     minode[i].dev = dev;
     minode[i].ino = ino;
     //(4). return mip;
     return &minode[i];
}


int iput(MINODE *mip)  // dispose of a minode[] pointed by mip
{
	// Reduce Refcount
	mip->refCount--;
	// If refcount is still greater than 0 or dirty we're done
	if (mip->refCount > 0) 
        return 0;
     if (!mip->dirty)       
        return 0;
 
	//(3). write INODE back to disk 
	//Use mip->ino to compute 
	// int seek = ((ino-1)/8 + mtable[0].iblock) * 1024 + (ino-1)%8 * 128;
	
    //blk  = block containing THIS INODE
    int block =  (mip->ino - 1) / INODES_PER_BLOCK + mtable;
    //disp = which INODE in block
    int disp = (mip->ino - 1) % INODES_PER_BLOCK;
	// Need this because we add on disp to the ip
    get_block(mip->dev, block, buf);
    ip = (INODE *)buf + disp;
    // Wait what's the point of setting *ip just after setting ip?
    *ip = mip->inode;         // copy INODE into *ip
	// Put part of everything (also doesn't use ip?
    put_block(mip->dev, block, buf);
} 

int search(MINODE *mip, char *name)
{
	// If dir name is . or .. don't tok with \n
	if (strcmp(name, ".") == 0 | !strcmp(name, "..")) printf("it's . or ..\n");
	else name = strtok(name, "\n");
	char temp[256];
	char *cp;
	DIR *dp;
	int blk;
	// Go through the blocks WE'll Be using
    for (int i = 0; i < 12; i++) 
    {
		// Get the i_block of the inode
		get_block(dev, mip->inode.i_block[i] , buf);
		dp = (DIR *)buf;
		cp = buf;
		
		while (cp < buf + BLKSIZE) 
		{
			strncpy(temp, dp->name, dp->name_len);
			temp[dp->name_len] = 0;
			// Found the right directory return it's inode
			if (!strcmp(name, temp)) return dp->inode;
			cp += dp->rec_len;
			dp = (DIR *)cp;
			// Prevent infinite loop
			if (dp->rec_len == 0) break;
			
		}
	}
	// Didn't find anything
	return -1;
}

int getino(char *pathname)
{
  int i, ino;
  char buf[BLKSIZE];
  MINODE *mip;
  dev = root->dev; // only ONE device so far
  printf("getino: pathname=%s\n", pathname);
  if (strcmp(pathname, "/\n")==0) return 2; // It's the root
  
  if (pathname[0]=='/') mip = iget(dev, 2); // Starts from root
  else mip = iget(dev, running->cwd->ino); // Starts from cwd

  strcpy(buf, pathname); // Copy our pathname into a buffer
  char *token = strtok(buf, "/"); // n = number of token strings

  while (token != NULL) {
    printf("===========================================\n");
    printf("getino: i=%d name[%d]=%s\n", i, i, token);
	
    ino = search(mip, token);
    if (ino < 0){
       iput(mip);
       printf("name %s does not exist\n", token);
       return 0;
    }
    iput(mip);
    mip = iget(dev, ino);
    token = strtok(NULL, "/");
  }
  iput(mip);
  return ino;
}



