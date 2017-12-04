#include "func.c"

// End
// ls|cd|pwd|mkdir|creat|rmdir|link|unlink|symlink|chmod|stat|touch|open|close|read|write|lseek|cat|cp|mv|mount|umount|quit (permission checking)
// Need work
// mkdir|creat|symlink|stat|chmod|touch|open|close|read|write|lseek|cat|cp|mv|mount|umount|quit (permission checking)
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

// REMOVE DIRECTORY
// Purpose: Remove an empty directory.
void rmdir(char * pathname) {
	int count = 0;
	char name[128], temp[128];
	// get in-memory INODE of pathname:
	int ino = getino(pathname);
	MINODE * mip = iget(dev, ino);
	char *parent = dirname(pathname);
	int pino = getino(parent);
	//verify INODE is a DIR (by INODE.i_mode field);
	if(!S_ISDIR(mip->inode.i_mode))
	{
		printf("File is not a Dir\n");
		return;
	}
	
	//minode is not BUSY (refCount = 1);
	printf("refcount = %d\n", mip->refCount);
	if(mip->refCount > 1)
	{
		printf("Dir is busy\n");
		return;
	}
	
	//verify DIR is empty (traverse data blocks for number of entries = 2);
	if(mip->inode.i_links_count == 2)
	{
		//traverse data blocks insearch of files
		// Go through the blocks WE'll Be using
			// Get the i_block of the inode
			get_block(dev, mip->inode.i_block[0] , buf);
			dp = (DIR *)buf;
			char *cp = buf;
			
			while (cp < buf + BLKSIZE) 
			{
				strncpy(temp, dp->name, dp->name_len);
				temp[dp->name_len] = 0;
				if ((strcmp(temp, ".") != 0) && (strcmp(temp, "..") != 0)) {
					printf("Dir not empty\n");
					return; 
				}
				cp += dp->rec_len;
				dp = (DIR *)cp;
				
			}
	}
	printf("linkcount = %d\n", mip->inode.i_links_count);
	if(mip->inode.i_links_count > 2)
	{
		printf("Dir not empty\n");
		return;
	}
	
	/* get parent's ino and inode */
	MINODE * pmip = iget(mip->dev, pino);
 
	//(4). /* get name from parent DIR’s data block
	findname(pmip, ino, name); //find name from parent DIR
	
	//(5). remove name from parent directory */
	rm_child(pmip, name);
	
	//(6). /* deallocate its data blocks and inode */
	bdalloc(mip->dev, mip->inode.i_block[0]);
	idalloc(mip->dev, mip->ino);
	iput(mip);
	
	//(7). dec parent links_count by 1; mark parent pimp dirty;
	pmip->inode.i_links_count--;
	pmip->dirty = 1;
	iput(pmip);
}

// REMOVE CHILD
// HELPER FUNCTION
// Purpose: Remove the child from the parents directory.
int rm_child(MINODE * pmip, char * name) {
	//got through each of the parent's blocks
	int ino, tempRecLen, found = 0, i, prevDirLen;
	char *cp, temp[256];
	ino = search(pmip, name);
	
	for(i = 0; i < 12 ; i++) {
		//get the parent block
		get_block(mtable[0].dev, pmip->inode.i_block[i], buf);
		
		dp = (DIR *)buf;
		cp = buf;
		
		//search through directories by inode number
		while (cp < buf + BLKSIZE) {
			// Difference of ls_dir if they find their inode, break with temp intact
			strncpy(temp, dp->name, dp->name_len);
			temp[dp->name_len] = 0;
			if (!strcmp(name, temp)) {
				found = 1;
				break;
			}
			prevDirLen = dp->rec_len;
			cp += dp->rec_len;
			dp = (DIR *)cp;
			if (dp->rec_len == 0) return;
		}	
		//if the directory is found jump out break out of for loop
		if (found) break;	
	}
	
	//If the directory is the only child
	if(dp->rec_len == BLKSIZE) {
		//deallocate dir
		free(buf);
		idalloc(mtable[0].dev, ino);
		pmip->inode.i_size -= BLKSIZE;
			
		//move parent's blocks left
		for(; i < 11; i++) {
				get_block(dev, pmip->inode.i_block[i], buf);
				put_block(dev, pmip->inode.i_block[i - 1], buf);
		}
	}
	//If the directroy is the last directory
	else if(cp + dp->rec_len == buf + BLKSIZE) //cp is the beginning position of the dir. Buf is the beginning position of first dir
	{
		//holding onto cur dir's len
		tempRecLen = dp->rec_len;
		
		//getting previous dir
		
		//Before: cp -=  dp->rec_len;
		
		//NEED TO SUBTRACT THE PREVIOUS DIRECTORIES LENGTH NOT CURRENT DIRECTORY
		cp -= prevDirLen;
		dp = (DIR *) cp;
			
		//adding the len of cur dir to previous dir
		dp->rec_len += tempRecLen;
			
		//put_block
		put_block(dev, pmip->inode.i_block[i], buf);
	}
	else
	{
		//store deleted rec_len
		tempRecLen = dp->rec_len;
		u8 *start = cp + dp->rec_len;
		u8 *end = buf + BLKSIZE;
		printf("start = %d\nend =   %d\n", start, end);	
		
		//move everythng left
		memmove(cp, start, end - start); //TAKES IN POINTERS FOR FIRST AND SECOND ARGUEMENTS


		dp = (DIR *)cp;
		while(cp + tempRecLen < buf + BLKSIZE)
		{
			if (dp->name_len == 0) break;
			printf("dp name = %s\n", dp->name);
			printf("dp name len = %d\n", dp->name_len);
			cp += dp->rec_len;
			dp = (DIR *)cp;
			
		}	
		
		//add deleted rec_len to last dir
		dp->rec_len += tempRecLen;
	
		
		//put_block
		put_block(mtable[0].dev, pmip->inode.i_block[i], buf);
		
	}
	//make parent's dirty propery true
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

// STAT
// Purpose: Returns file attributes from node.
void myStat(char *name) {
	// Check that the file exists within
	if(!name) {
		printf("No file specified.\n");
		return;
	}
	// Otherwise get to work.
	int ino = getino(name);
	// Print Name
	printf("  File: '%s'\n", name);
	// Size, blocks, IO block, filetype
	printf("%d", ip->i_size);
	if(!S_ISDIR(ip->i_mode)) printf("file\n");
	else printf("directory\n");
	// device, inode, links
	printf("Device: Inode: Links");
	// access, uid, gid
	
	// time info
	
MINODE *mip = iget(mtable[0].dev, ino);
    if(!mip) {
        fprintf(stderr, "list_file: Null pointer to memory inode\n");
        return;
    }

    INODE *ip = &mip->inode;
    u16 mode   = ip->i_mode;
    u16 links  = ip->i_links_count;
    u32 size   = ip->i_size;

    static const char* Permissions = "rwxrwxrwx";

    // Type 
    // The leading 4 bits of mode (2 bytes/16 bits) indicate type
    // 0xF000 = 1111 0000 0000 0000
    switch(mode & 0xF000) 
    {
        case 0x8000:  putchar('-');     break; // 0x8 = 1000
        case 0x4000:  putchar('d');     break; // 0x4 = 0100
        case 0xA000:  putchar('l');     break; // oxA = 1010
        default:      putchar('?');     break;
    }

    // Permissions
    for(int i = 0; i < strlen(Permissions); i++)
        putchar(mode & (1 << (strlen(Permissions) - 1 - i)) ? Permissions[i] : '-');

    // Everything else
    printf("%3hu %6u %2d  ", links, size, ino);

    // Trace link
    if(S_ISLNK(ip->i_mode)) printf(" -> %s", (char*)ip->i_block);
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
	while(1) 
	{
		// Print commands and get input
		printf("user$ ");
		fgets(line, 128, stdin);
		tok = strtok(line," ");
		// look into removing "/n"
		//Get command token
		if(tok) cmd = tok;
		
		// EXIT
		if(!strcmp(cmd, "quit\n")) quit();
		// LS
		else if(!strcmp(cmd, "ls") | (!strcmp(cmd, "ls\n"))) {
			// Use ls with pathname, which is retrieved from second token
			tok = strtok(NULL," ");
			ls(tok);
		}
		// CD
		else if(!strcmp(cmd, "cd\n") | (!strcmp(cmd, "cd"))) { 
			// Also uses second token to get a pathname
			tok = strtok(NULL," ");
			cd(tok);
		}
		// PWD
		else if(!strcmp(cmd, "pwd\n")) {
			
			pwd(running->cwd);
		}
		// mkdir
		else if(!strcmp(cmd, "mkdir")) {
			tok = strtok(NULL, " ");
			Mkdir(tok);
		}
		// creat
		else if(!strcmp(cmd, "creat")) {
			tok = strtok(NULL, " ");
			Creat(tok);
		}
		// rmdir
		else if(!strcmp(cmd, "rmdir")) {
			tok = strtok(NULL, " ");
			rmdir(tok);
		}
		// LINK
		else if(!strcmp(cmd, "link")) {
			tok = strtok(NULL, " ");
			strncpy(file1, tok, sizeof(tok));
			tok = strtok(NULL, " ");
			myLink(file1, tok);
		}
		//UNLINK
		else if(!strcmp(cmd, "unlink")) {
			tok = strtok(NULL, " ");
			myUnlink(tok);
		}
		//SYMLINK
		else if(!strcmp(cmd, "symlink")) {
			tok = strtok(NULL, " ");
			strncpy(file1, tok, sizeof(tok));
			tok = strtok(NULL, " ");
			mySymlink(file1, tok);
		}
		else if(!strcmp(cmd, "chmod")) {
			tok = strtok(NULL, " ");
			mychmod(tok);
		} 
		else if(!strcmp(cmd, "stat")) {
			tok = strtok(NULL, " ");
			myStat(tok);
		} /*
		else if(!strcmp(cmd, "touch")) {
			tok = strtok(NULL, " ");
			mytouch(tok);
		}
		else if(!strcmp(cmd, "open")) {
			tok = strtok(NULL, " ");
			myopen(tok);
		}
		else if(!strcmp(cmd, "close")) {
			tok = strtok(NULL, " ");
			myclose(tok);
		}
		else if(!strcmp(cmd, "read")) {
			tok = strtok(NULL, " ");
			myread(tok);
		}
		else if(!strcmp(cmd, "write")) {
			tok = strtok(NULL, " ");
			mywrite(tok);
		}
		else if(!strcmp(cmd, "lseek")) {
			tok = strtok(NULL, " ");
			mylseek(tok);
		}
		else if(!strcmp(cmd, "cat")) {
			tok = strtok(NULL, " ");
			mycat(tok);
		}
		else if(!strcmp(cmd, "cp")) {
			tok = strtok(NULL, " ");
			mycp(tok);
		}
		else if(!strcmp(cmd, "mv")) {
			tok = strtok(NULL, " ");
			mymv(tok);
		}
		else if(!strcmp(cmd, "mount")) {
			tok = strtok(NULL, " ");
			mymount(tok);
		}
		else if(!strcmp(cmd, "umount")) {
			tok = strtok(NULL, " ");
			myumount(tok);
		}*/
	}
}
