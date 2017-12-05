#include "func.c"

// End
// ls|cd|pwd|mkdir|creat|rmdir|link|unlink|symlink|chmod|stat|touch|open|close|read|write|lseek|cat|cp|mv|mount|umount|quit (permission checking)
// Need work
// chmod|open|close|read|write|lseek|cat|cp|mv|mount|umount|quit (permission checking)
/*			
 * 			   Rely on iput, which is buggy
               mkdir, creat, symlink, touch;
			   stat, chmod
               ------  LEVEl 2 -------------
               open,  close,  read,  write
               lseek  cat,    cp,    mv

               ------  LEVEl 3 ------------ 
               mount, umount
               File permission checking
*/

// READ
// Purpose:
/**************** Algorithm of kread() in kernel ****************/
int myread(int fd, char buf[ ], int nbytes) //space=K|U
{ 
//(1). validate fd; ensure oft is opened for READ or RW;
//(2). if (oft.mode = READ_PIPE) 
//         return read_pipe(fd, buf, nbytes);/
//(3). if (minode.INODE is a special file) 
//         return read_special(device,buf,nbytes);
//(4). (regular file):
//         return read_file(fd, buf, nbytes, space);
} 
/*************** Algorithm of read regular files ****************/
int read_file(int fd, char *buf, int nbytes) { 
//(1). lock minode;
//(2). count = 0;         // number of bytes read
//     compute bytes available in file: avil = fileSize - offset;
//(3). while (nbytes){
//       compute logical block: lbk   = offset / BLKSIZE;
//       start byte in block:   start = offset % BLKSIZE;
//(4).   convert logical block number, lbk, to physical block number,
//       blk, through INODE.i_block[ ] array;
//(5).   read_block(dev, blk, kbuf); // read blk into kbuf[BLKSIZE];
//       char *cp = kbuf + start; 
//       remain = BLKSIZE - start;
//(6)    while (remain){// copy bytes from kbuf[ ] to buf[ ]
//          (space)? put_ubyte(*cp++, *buf++) : *buf++ = *cp++; 
 //         offset++; count++;           // inc offset, count;
  //        remain--; avil--; nbytes--;  // dec remain, avil, nbytes;
   //       if (nbytes==0 || avail==0) //
 //             break;
} 
//(7). unlock minode;
//(8). return count;*/

// WRITE
//Purpose: 
int mywrite(int fd, char *ubuf, int nbytes) {
	//1. Validate fd; ensure OFT is opened for write
	//2. if (oft.mode = WRITE_PIPE) return write_pipe(fd, buf, nbytes)
	//3 if (minode.inode is a special file return write_special(device, buf.nbytes);
	// return write_file(fd, ubuf, nbytes)
}	
	
// WRITE FILE
// HELPER FUNCTION
// Purpose: to assist write function
int write_file(int fd, char *ubuf, int nbytes) {
	// lock minode;
	// count = 0;
	/*while (nbytes) {
		//int lbk = running->fd[fd]->offset / BLKSIZE;
		//int start = oftp->offset % BLKSIZE;
		// Conver lbk to physical block number
		read_block(dev, blk, kbuf);
		char *cp = kbuf + start;
		remain = BLKSIZE - start;
		while (remain) {
			put_ubyte(*cp++, *ubuf++);
			offset++;
			count++;
			remain--;
			nbytes--;
			if (offset > fileSize) fileSize++;
			if (nbytes <= 0) break;
		}
		write_block(dev, blk, kbuf);
	}
	//set minode dirty = 1;
	unlock(minode);
	return count;*/
}

// LSEEK
//Purpose: 

// CONCATENATE
//Purpose: const int device = running->cwd->device;
void mycat(char *args) {
    // Cat every file given
    while(args) {
        char* pathname = args;
        int fd = myopen(pathname, "RD"), n = 0;
        char buf[BLKSIZE + 1];
        while((n = myread(fd, buf, BLKSIZE))) {
            buf[n] = '\0'; // null terminated string
            int j = 0;
            while(buf[j]) {
                putchar(buf[j]);
                if(buf[j] == '\n') putchar('\r');
                j++;
            }
        } 
        printf("\n\r");
        myclose(fd);
        args = strtok(NULL, " ");
    }
}

// COPY
//Purpose: Copy a file or directory, not linked so no shared fate
void mycopy(char *file, char *copy) {
	
}

// MOVE
//Purpose: Moves a directory or file
void mymove(char *file, char *location) {
	// Move
	// Remove child then add it to new location
}

// MOUNT
//Purpose: 
/*mymount()  // Usage: mount [filesys mount_point]
 { 
1. If no parameter, display current mounted file systems; 
2. Check whether filesys is already mounted: 
    The MOUNT table entries contain mounted file system (device) names 
    and their mounting points. Reject if the device is already mounted.
    If not, allocate a free MOUNT table entry.
3. filesys is a special file with a device number dev=(major,minor)
.
    Read filesys' superblock to verify it is an EXT2 FS. 
4. find the ino, and then the minode of mount_point:
         call ino = get_ino(&dev, pathname);  to get ino:
         call mip = iget(dev, ino); to load its inode into memory;   
5. Check mount_point is a DIR and not busy, e.g. not someone's CWD.
6. Record dev and filesys name in the MOUNT table entry
;
    also, store its ninodes, nblocks, etc. for quick reference.
7. Mark mount_point's minode as mounted on (mounted flag = 1) and let 
  it point at the MOUNT table entry, which points back to the 
  mount_point minode
  */

// UNMOUNT
//Purpose: 
/*umount(char *filesys)
{ 
1. Search the MOUNT table to check filesys is indeed mounted.
2. Check (by checking all active minode[].dev) whether any file is
    active in the mounted filesys; If so, reject
;
3. Find the mount_point's in-memory inode, which should be in memory
while it's mounted on. Reset the minode's mounted flag to 0; then
iput() the minode
.
4. return SUCCESS;*/

// OPEN
// Purpose:
void myopen(char *file, char *flag) {
	//1. get file's minode:
	int ino = getino(file);
	// If the file doesn't exist make it
	if (ino==0 && O_CREAT){
		Creat(file);
		ino = getino(file);
	} 
	MINODE *mip = iget(mtable[0].dev, ino);
	//2. check file INODE's access permission; (LEVEL 3 THING)
	//for non-special file, check for incompatible open modes
	
	//3. allocate an openTable entry;
	//initialize openTable entries;
	//set byteOffset = 0 for R|W|RW; 
	//set to file size for APPEND mode;

	//4. Search for a FREE fd[ ] entry with the lowest index fd in PROC;
	//let fd[fd]point to the openTable entry;
	
	//5. unlock minode; 
	//return fd as the file descriptor;


    int mode = 0;
    if(strcmp(flag, "RD") == 0) mode = 0;
    else if(strcmp(flag, "WR") == 0) mode = 1;
    else if(strcmp(flag, "RW") == 0) mode = 2;
    else if(strcmp(flag, "APPEND") == 0) mode = 3;
    else {
        fprintf(stderr, "open: unknown mode '%s'\n", flag);
        return;
    }

	//return open_file(file, mode);
}

// CLOSE
// Purpose: 
void myclose() {
/*	*************  Algorithm of close()  *****************
/
int close(int fd) { 
(1). check fd is a valid opened file descriptor;
(2). if (PROC's fd[fd] != 0){
(3).    if (openTable's mode == READ/WRITE PIPE)
return close_pipe(fd); // close pipe descriptor;
(4).    if (--refCount == 0){ // if last process using this OFT
lock(minodeptr); 
iput(minode);      // release minod
e
} 
} 
(5). clear fd[fd] = 0;      
     // clear fd[fd] to 
0
(6). return SUCCESS;*/
}

// CHANGE MODE
// Purpose: Change permissions of a file or directory.
void mychmod(char *args) {
    
    const int device = running->cwd->dev;
	int argc = 0;
	char *argv[5];
	
	// Filter args into arg, bad way of doing this technically
	while(args) {
		argv[argc] = args;
		printf("%s\n", args);
		argc++;
		args = strtok(NULL, " ");
	}
	// Not enough arguments
	if (argc < 2) return;
	
    int ino = getino(argv[0]);
    MINODE* mip = iget(device, ino);
    INODE*   ip = &mip->inode;

    long int new_mode = strtol(argv[1], NULL, 8);

    // Change the file's mode to new_mode 
    // The leading 4 bits of type indicate type
    // First clear its current mode by ANDing with 0xF000
    // Then set its mode by ORing with new_mode 
    ip->i_mode = (ip->i_mode & 0xF000) | new_mode;

    mip->dirty = 1;
    iput(mip);
}

// SYMBOLIC LINK
// Purpose: 
void mySymlink(char *oldfile, char *newfile) {
	char* parent, *child;
	char path[250];
	
	// Check the oldfile exists
	strcpy(path, newfile);
	child = basename(path);
	parent = dirname(newfile);
	int pinoold = getino(parent);
	MINODE *pmipOld = iget(mtable[0].dev, pinoold);
	if (search(pmipOld, child) < 0) return;
	
	// Check the new file doesn't exists
	strcpy(path, oldfile);
	child = basename(path);
	parent = dirname(oldfile);
	int pinonew = getino(parent);
	MINODE *pmipNew = iget(mtable[0].dev, pinonew);
	if (search(pmipNew, child) != -1) return;
	
	// Plug in Most of Creat except change INODE to LNK type
	
}

// MAKE DIRECTORY
// Purpose: Create a new empty directory.
void Mkdir(char *pathname) {
	char* parent, *child;
	int ino;
	char path[250];
	strcpy(path, pathname);
	//Divide pathname into dirname and basename using library functions
	child = basename(path);
	parent = dirname(pathname);
	
	//parent must exist and is a dir //
	printf("parent = %s\nchild = %s\n", pathname, child);
	ino = getino(parent);
	
	if(ino == 0) {//does parent exist?
		printf("Parent doesn't exist\n");
		return;
	}
	MINODE *pmip = iget(mtable[0].dev, ino);
	
	//check pmip-> INODE is a dir//
	if(!S_ISDIR(pmip->inode.i_mode)) {
		printf("Parent is not a Dir\n");
		return;
	}
	
	//child must not exist in parent
	if(search(pmip, child) != -1) {
		printf("Child exists in parent\n");
		return;
	}
	my_mkdir(pmip, child);
}

// MAKE DIRECTORY
// Helper Function
// Purpose: Help MKDIR with creating a directory.
void my_mkdir(MINODE *pmip, char *name) {
	//Allocate an Inode and Disk Block
	int ino = ialloc(mtable[0].dev);
	printf("ino = %d\n", ino); 
	int blk = balloc(mtable[0].dev); 
	printf("blk = %d\n", blk);
	
	//Load Inode into an Minode
	MINODE * mip = iget(mtable[0].dev, ino); 
	
	INODE * ip = &mip->inode;
	ip->i_mode = 0x41ED; // 040755: DIR type and permissions
	ip->i_uid = running->uid; // owner uid
	ip->i_gid = running->gid; // group Id
	ip->i_size = BLKSIZE; // size in bytes
	ip->i_links_count = 2; // links count=2 because of . and ..
	ip->i_atime = ip->i_ctime = ip->i_mtime = time(0L);
	ip->i_blocks = 2; // LINUX: Blocks count in 512-byte chunks
	ip->i_block[0] = blk; // new DIR has one data block
	for (int i = 0; i < 15; i++) {
		ip->i_block[i] = 0;
	}
	mip->dirty = 1; // mark minode dirty
	mip->refCount = 2;
	printf("size = %d\n", mip->inode.i_size);
	iput(mip); // write INODE to disk
	
	get_block(mtable[0].dev, blk, buf);
	
	DIR *dp = (DIR *)buf;
	char *cp = buf;
	
	
	
	// make . entry
	dp->inode = ino;
	dp->rec_len = 12;
	dp->name_len = 1;
	dp->file_type = EXT2_FT_DIR;
	dp->name[0] = '.';
	
	// make .. entry: pino=parent DIR ino, blk=allocated block
	cp += dp->rec_len;
	dp = (DIR *)cp;
	
	dp->inode = pmip->ino;
	dp->rec_len = BLKSIZE-12; // rec_len spans block
	dp->name_len = 2;
	dp->file_type = EXT2_FT_DIR;
	dp->name[0] = dp->name[1] = '.';
	put_block(mtable[0].dev, blk, buf); // write to blk on diks
	
	
	//entering newdir into parent dir
	enter_child(pmip, ino, name);
	
	//increment parents inode links by 1 and mark pmip dirty
	pmip->inode.i_links_count++;
	pmip->dirty = 1;
	
	
	
	iput(pmip);
}

// CREATE
// Purpose: Creat a new file.
void Creat(char *pathname) {
	char* parent, *child;
	int ino;
	char path[250];
	strcpy(path, pathname);
	//Divide pathname into dirname and basename using library functions
	child = basename(path);
	parent = dirname(pathname);
	
	//parent must exist and is a dir //
	printf("parent = %s\nchild = %s\n", pathname, child);
	ino = getino(parent);
	
	if(ino == 0)//does parent exist?
	{
		printf("Parent doesn't exist\n");
		return;
	}
	MINODE *pmip = iget(mtable[0].dev, ino);
	
	//check pmip-> INODE is a dir//
	if(!S_ISDIR(pmip->inode.i_mode))
	{
		printf("Parent is not a Dir\n");
		return;
	}
	
	//child must not exist in parent
	if(search(pmip, child) != -1)
	{
		printf("Child exists in parent\n");
		return;
	}
	my_creat(pmip, child);
}

// CREATE
// HELPER FUNCTION
// Purpose: Helps make a new file.
void my_creat(MINODE *pmip, char *name) {
	//Allocate an Inode and Disk Block
	int ino = ialloc(mtable[0].dev);
	int blk = balloc(mtable[0].dev); 
	printf("New ino = %d\nnew blk = %d\n", ino, blk);
	
	//Load Inode into an Minode
	MINODE * mip = iget(mtable[0].dev, ino); 
	
	INODE * ip = &mip->inode;
	ip->i_mode = 0x81A4; // 040755: DIR type and permissions
	ip->i_uid = running->uid; // owner uid
	ip->i_gid = running->gid; // group Id
	ip->i_size = 0; // size in bytes
	ip->i_links_count = 1; // links count=2 because of . and ..
	ip->i_atime = ip->i_ctime = ip->i_mtime = time(0L);
	ip->i_blocks = 2; // LINUX: Blocks count in 512-byte chunks
	ip->i_block[0] = blk; // new DIR has one data block
	for (int i = 0; i < 15; i++) {
		ip->i_block[i] = 0;
	}
	mip->dirty = 1; // mark minode dirty
	printf("size = %d\n", mip->inode.i_size);
	iput(mip); // write INODE to disk
	
	
	
	//make data block 0 of inode contain the . and .. entries
	char buf[BLKSIZE];
	bzero(buf, BLKSIZE); // optional: clear buf[ ] to 0
	DIR *dp = (DIR *)buf;
	// make . entry
	dp->inode = ino;
	dp->rec_len = 12;
	dp->name_len = 1;
	dp->name[0] = '.';
	// make .. entry: pino=parent DIR ino, blk=allocated block
	put_block(mtable[0].dev, blk, buf); // write to blk on diks
	
	
	//entering newdir into parent dir
	enter_child(pmip, ino, name);
	
	//increment parents inode links by 1 and mark pmip dirty
	pmip->inode.i_links_count++;
	pmip->dirty = 1;
	
	printf("pmip = %d\n", pmip->dev);
	
	iput(pmip);
}

int main(int argc, char *argv[]) {
	// Used for getting input
	char line[128];
	char file1[128];
	// In case user has a different file other than mydisk
	if (argc > 1) device = argv[1];
	// Initiate and mount the root
	init();
	mount_root(device);
	// Main loop
	while(1) {
		// Print commands and get input
		printf("user$ ");
		fgets(line, 128, stdin);
		tok = strtok(line," ");
		//Get command token
		if(tok) cmd = tok;
		// Pass of command checking to a function
		command(cmd, tok);
	}
}
