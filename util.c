#include "type.h"

// globals
MINODE minode[NMINODES];       
MINODE *root;
PROC   proc[NPROC], *running;
MOUNT mtable[4]; 

SUPER *sp;
GD    *gp;
INODE *ip;

char buf[BLKSIZE];

int dev;
/**** same as in mount table ****/
int nblocks; // from superblock
int ninodes; // from superblock
int bmap;    // bmap block 
int imap;    // imap block 
int iblock;  // inodes begin block

/********** Functions as BEFORE ***********/
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
	int i;
	char buf[1024];
     //(1). search minode[ ] array for an item pointed by mip with the SAME (dev,ino)
     for (i=0; i < NMINODES; i++) 
     {
		 if (minode[i].ino == ino && minode[i].dev == dev) {
			 minode[i].refCount++;
			 return &minode[i];
		 }
	 }
	 
	 //(2). search minode[ ] array for a mip whose refCount=0:
     for (i = 0; i < NMINODES; i++) 
     {
		 if (!minode[i].refCount) {
			 minode[i].refCount = 1;
			 minode[i].dev = dev;
			 minode[i].ino = ino;
			 minode[i].dirty = 0;
			 minode[i].mounted = 0;
			 minode[i].mountptr = 0;
			 break;
		 }

	 }
     //(3). use mailman to compute
     //blk  = block containing THIS INODE
     int blk =  ((ino - 1) / INODES_PER_BLOCK + mtable);
     //disp = which INODE in block
     int disp = ((ino - 1) % INODES_PER_BLOCK);
     //load blk into buf[ ];
     get_block(dev, disp, buf);
     //INODE *ip point at INODE in buf[ ];
     INODE *ip = (INODE *) buf;
     
     //copy INODE into minode.INODE by
     minode[i].inode = *ip;

     //(4). return mip;
     return &minode[i];
}


int iput(MINODE *mip)  // dispose of a minode[] pointed by mip
{
	//(1). 
	mip->refCount--;
 
	//(2). 
	if (mip->refCount > 0) 
        return 0;
     if (!mip->dirty)       
        return 0;
 
	//(3). write INODE back to disk 

	printf("iput: dev=%d ino=%d\n", mip->dev, mip->ino); 

	//Use mip->ino to compute 
    //blk  = block containing THIS INODE
    int block =  (mip->ino - 1) / INODES_PER_BLOCK + mtable;
    //disp = which INODE in block
    int disp = (mip->ino - 1) % INODES_PER_BLOCK;

    get_block(mip->dev, block, buf);

    ip = (INODE *)buf + disp;
    *ip = mip->inode;         // copy INODE into *ip

    put_block(mip->dev, block, buf);
} 

int search(MINODE *mip, char *name)
{
    // YOUR search function !!!
}

int getino(char *pathname)
{
  int i, ino, blk, disp;
  char buf[BLKSIZE];
  INODE *ip;
  MINODE *mip;
  dev = root->dev; // only ONE device so far

  printf("getino: pathname=%s\n", pathname);
  if (strcmp(pathname, "/")==0)
     return 2;

  if (pathname[0]=='/')
     mip = iget(dev, 2);
  else
     mip = iget(dev, running->cwd->ino);

  strcpy(buf, pathname);
  char *token = strtok(buf, " "); // n = number of token strings
  
  while (token != NULL) {
    printf("===========================================\n");
    printf("getino: i=%d name[%d]=%s\n", i, i, token);
 
    ino = search(mip, token);

    if (ino==0){
       iput(mip);
       printf("name %s does not exist\n", token);
       return 0;
    }
    iput(mip);
    mip = iget(dev, ino);
    token = strtok(NULL, " ");
  }
  iput(mip);

  return ino;
}



