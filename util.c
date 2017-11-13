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
// END VARIABLES

/********** Functions as BEFORE ***********/
// From the file descriptor move forward blk amount absolutely
// Then read the next BLKSIZE amount into the buffer
int get_block(int fd, int blk, char *buf) {
  lseek(fd, (long)blk*BLKSIZE, SEEK_SET);
  return read(fd, buf, BLKSIZE);
}

int put_block(int dev, int blk, char buf[ ]) 
{
	lseek(dev, (long)(blk*BLKSIZE), SEEK_SET);
	write(dev, buf, BLKSIZE);	
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
if (minode[i].refCount) {
			if (minode[i].dev == dev && minode[i].ino == ino) {
				//minode[i].refCount++;
				return &minode[i];
			} }
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
    int block =  (mip->ino - 1) / INODES_PER_BLOCK + 5;
    //disp = which INODE in block
    int disp = (mip->ino - 1) % INODES_PER_BLOCK;
	// Need this because we add on disp to the ip
    get_block(mip->dev, block, buf);
    ip = (INODE *)buf + disp;
    // Wait what's the point of setting *ip just after setting ip?
    *ip = mip->inode;       // copy INODE into *ip
	// Put part of everything (also doesn't use ip?
    put_block(mip->dev, block, buf);
    mip->dirty = 0;
} 

int search(MINODE *mip, char *name)
{
	// If dir name is . or .. don't tok with \n
	if (strcmp(name, ".") == 0 | !strcmp(name, "..")) printf("");
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
  char buffer[256], morebuf[256], *token, *save1;
  const char s[2] = "/";
  MINODE *mip;
  dev = root->dev; // only ONE device so far
  // Print what we're trying to get the pathname of
  printf("getino: pathname=%s\n", pathname);
  // If the string is the root
  if ((strcmp(pathname, "/\n")==0) || (strcmp(pathname, "/") ==0)) return 2; // It's the root
  
  if (pathname[0]=='/') {
	  mip = iget(dev, 2);
	  memmove(pathname, pathname+1, strlen(pathname));
  } // Starts from root
  else mip = iget(dev, running->cwd->ino); // Starts from cwd
  // Copy pathname into buffer and change last char into terminating
  strcpy(buffer, pathname);
  // Tokenize with save state to prevent search from ruining the token
  token = strtok_r(buffer, s, &save1);
  while (token) {
    printf("===========================================\n");
    printf("getino: i=%d name[%d]=%s\n", i, i, token);
	strcpy(morebuf, token);
    ino = search(mip, morebuf);
    if (ino < 0){
       iput(mip);
       printf("name %s does not exist\n", token);
       return 0;
    }
    iput(mip);
    mip = iget(dev, ino);
    printf("%s\n", morebuf);
    token = strtok_r(NULL, s, &save1);
  }
  iput(mip);
  printf("ino found = %d\n", ino);
  return ino;
}

int tst_bit(char *buf, int bit)
{
	return buf[bit/8] & (1 << (bit % 8));
}

int set_bit(char *buf, int bit)
{
	return buf[bit/8] |= (1 << (bit % 8));
}

int decFreeInodes(int dev)
{
	//Decrement free inode count in Super Block
	get_block(dev,1, buf);
	sp = (SUPER *) buf;
	sp->s_free_inodes_count--;
	put_block(dev, 1, buf);
	
	//Decremnt free inode count in Group Descriptor Block
	get_block(dev,2, buf);
	gp = (GD *) buf;
	gp->bg_free_inodes_count--;
	put_block(dev ,2 ,buf);
}

int decFreeBlocks(int dev)
{
	//Decrement free inode count in Super Block
	get_block(dev,1, buf);
	sp = (SUPER *) buf;
	sp->s_free_blocks_count--;
	put_block(dev, 1, buf);
	
	//Decremnt free inode count in Group Descriptor Block
	get_block(dev,2, buf);
	gp = (GD *) buf;
	gp->bg_free_blocks_count--;
	put_block(dev ,2 ,buf);
}

int ialloc(int dev)
{
	int i;
	char buf[BLKSIZE];
	
	get_block(dev, mtable[0].imap, buf);
	for(i= 0; i < mtable[0].ninodes; i++)
	{
		if(tst_bit(buf, i) == 0)
		{
			set_bit(buf, i);
			decFreeInodes(dev);
			put_block(dev, mtable[0].imap, buf);
			return i+1;
		}
	} 
	return 0;
}

int balloc(int dev)
{
int i;
	char buf[BLKSIZE];
	
	get_block(dev, mtable[0].bmap, buf);
	for(i= 0; i < mtable[0].nblocks; i++)
	{
		if(tst_bit(buf, i) == 0)
		{
			set_bit(buf, i);
			put_block(dev, mtable[0].bmap, buf);
			decFreeBlocks(dev);
			return i+1;
		}
	} 
	return 0;	
}

int clr_bit(char *buf, int bit) // clear bit in char buf[BLKSIZE]
{ 
	buf[bit/8] &= ~(1 << (bit%8)); 
}

int incFreeInodes(int dev)
{
	char buf[BLKSIZE];
	// inc free inodes count in SUPER and GD
	get_block(dev, 1, buf);
	sp = (SUPER *)buf;
	sp->s_free_inodes_count++;
	put_block(dev, 1, buf);
	get_block(dev, 2, buf);
	gp = (GD *)buf;
	gp->bg_free_inodes_count++;
	put_block(dev, 2, buf);
}

int incFreeBlocks(int dev)
{
	char buf[BLKSIZE];
	// inc free inodes count in SUPER and GD
	get_block(dev, 1, buf);
	sp = (SUPER *)buf;
	sp->s_free_blocks_count++;
	put_block(dev, 1, buf);
	get_block(dev, 2, buf);
	gp = (GD *)buf;
	gp->bg_free_blocks_count++;
	put_block(dev, 2, buf);
}

int idalloc(int dev, int ino)
{
	int i;
	char buf[BLKSIZE];

	get_block(dev, imap, buf);
	clr_bit(buf, ino-1);
	// write buf back
	put_block(dev, imap, buf);
	// update free inode count in SUPER and GD
	incFreeInodes(dev);
}

int bdalloc(int dev, int bno)
{
	int i;
	char buf[BLKSIZE];
	if (bno > mtable[0].nblocks)
	{ // niodes global
		printf("inumber %d out of range\n", bno);
		return;
	}
	// get inode bitmap block
	get_block(dev, mtable[0].bmap, buf);
	clr_bit(buf, bno-1);
	// write buf back
	put_block(dev, mtable[0].bmap, buf);
	// update free inode count in SUPER and GD
	incFreeBlocks(dev);
}

int findname(MINODE * pmip, int ino, char * name)
{
	int found = 0;
    get_block(mtable[0].dev, pmip->inode.i_block[0], buf);
	DIR *dp = (DIR *)buf;
	char *cp = buf;
	while (cp < buf + BLKSIZE) {
		// Difference of ls_dir if they find their inode, break with temp intact
		if (dp->inode == ino) {
			strncpy(name, dp->name, dp->name_len);
			name[dp->name_len] = 0;
			found = 1;
			break;
		}
		cp += dp->rec_len;
		dp = (DIR *)cp;
	}
	
	return found;
}

int enter_child(MINODE *pip, int ino, char *child)
{
	int i;
	char *cp;
	
	//for each data block of parent dir
	for(i = 0; i < 12; i++)
	{
		if(pip->inode.i_block[i] == 0)
		{
			break;
		}
		
		//getting parent block
		get_block(pip->dev, pip->inode.i_block[i], buf);
		dp = (DIR *)buf;
		cp = buf;
		
		//finding the last directory
		while (cp + dp->rec_len < buf + BLKSIZE)
		{
			cp += dp->rec_len;
			dp = (DIR *)cp;
		} 
		cp = (char *)dp;
		
		//ideal length is the length that the last dir needs
		int ideal_length = 4*( (8 + dp->name_len + 3)/4 );
		//the need legth is the length that is needed for the new dir
		int need_length = 4*( (8 + strlen(child) + 3)/4 );
		
		//the remain is difference between the last directory's length and it's ideal length
		int remain = dp->rec_len - ideal_length;
		
		//if the remian is less than the need length then there isnt enough room for the directorys
		if(remain >= need_length)
		{
			// trim previous rec_len to its ideal_length
			dp->rec_len = ideal_length;
			
			//move the pointer by the ideal_length in order to be at the starting position for the new directory
			cp += ideal_length;
			dp = (DIR *)cp;
			
			//set the length of new dir to the remain
			dp->rec_len = remain;
			//set the name, inode number, and the file type.
			dp->name_len = strlen(child);
			dp->inode = ino;
			dp->file_type = EXT2_FT_DIR;
			
			//display the name of the new directory along with it's inode number
			strncpy(dp->name, child, strlen(child));
			printf("New directory info:\n");
			printf("%s: ino = %d\n\n", dp->name, dp->inode);
		}
		else {
			printf("it didn't fit\n");
			int bnumber = balloc(pip->dev);
			dp = (DIR*)buf;
			dp->rec_len = BLKSIZE;
			dp->name_len = strlen(child);
			strncpy(dp->name, child, strlen(child));
			put_block(pip->dev, bnumber, strlen(child));
		}
		put_block(pip->dev, pip->inode.i_block[i], buf);
	}
	
}
