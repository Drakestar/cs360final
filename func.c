#include "util.c"

// INIT
// Purpose: Create root and user process and reset working directories
void init() 
{
	// Set proc 0 to root account and 1 to a normal user
	proc[0].uid = 0;
	proc[1].uid = 1;
	// Move through the users and reset working directories?
	for (int i = 0; i < NPROC; i++) proc[NPROC].cwd = 0;
	// Move through inodes and reset refcount
	for (int i = 0; i < 100; i++) minode[i].refCount = 0;
	// Set root?
	root = 0;
}

// MOUNT ROOT
// Purpose: Opens the device specified in util.c to be ready to run our system.
void mount_root(char *device) 
{
	// Returns smallest integer greater than zero, (the file descriptor)
	fd = open(device, O_RDWR);
	// If the fd is under 0 it means it failed
	if (fd < 0) {
		printf("open: %s failed\n", device);
		exit(1);
	}
	// Read the SuperBlock into a buffer
	// fd is the block, 1 is the array of the super, and buf is where i
	get_block(fd, 1, buf);
	//printf("inode size: %d\n", sizeof(INODE));
	sp = (SUPER *)buf;
	// Read the group descriptor block
	if (sp->s_magic != 0xEF53) {
		printf("Not an ext2 FS\n");
		exit(2);
	}
	
	mtable[0].dev = fd;
	mtable[0].ninodes = sp->s_inodes_count;
	mtable[0].nblocks = sp->s_blocks_count;

	// Switch buffer to Group descriptor
	get_block(fd, 2, buf);
	gp = (GD *)buf;
	
	mtable[0].bmap = gp->bg_block_bitmap;
	mtable[0].imap = gp->bg_inode_bitmap;
	mtable[0].iblock = gp->bg_inode_table;
	mtable[0].mounted_inode = root;
	
	strcpy(mtable[0].deviceName, device);
	strcpy(mtable[0].mountedDirName, "/");
	root = iget(fd, 2);	
	proc[0].cwd = iget(fd, 2);
	proc[1].cwd = iget(fd, 2);
	
	running = malloc(sizeof(PROC));
	memcpy(running, &proc[0], sizeof(PROC));
}

// QUIT
// Purpose: Shut down the file system safely and save changes.
void quit() {
	// Put MINODES that are DIRTY and have a REFCOUNT
	for (int i = 0; i < NMINODES; i++) {
		if ((minode[i].refCount > 0) && (minode[i].dirty)) iput(&minode[i]);
	}
	exit(0);
}

// PWD
// Purpose: Prints the current working directory of the user
void pwd(MINODE *wd) {
	// If it's at the root from the start just print that
	if (wd == root) printf("/");
	else rpwd(wd); // Otherwise recursivley print working directory
	printf("\n"); // Newline to look nice
}

// RPWD
// Helper function
// Purpose: Recursively print the working directory of the user.
void rpwd(MINODE *wd) {
	char temp[256];
	// Base case, reached the root
	if (wd->ino == root->ino) return;
	// Get inode number of itself and parent
	int mynode = search(wd, ".");
	int parent = search(wd, "..");
	// Get MINODE of parent
    MINODE *pip = iget(mtable[0].dev, parent);
    
    // Get the block of the parent.
    get_block(mtable[0].dev, pip->inode.i_block[0], buf);
	DIR *dp = (DIR *)buf;
	char *cp = buf;
	while (cp < buf + BLKSIZE) {
		// Find the child node and break with it intact
		if (dp->inode == mynode) {
			strncpy(temp, dp->name, dp->name_len);
			temp[dp->name_len] = 0;
			break;
		}
		cp += dp->rec_len;
		dp = (DIR *)cp;
	}
	// Recursive call allows printing to be correct.
    rpwd(pip);
    // Print with / so that no root print is necessary
    printf("/%s", temp);
}

// LS DIR
// Helper function
// Purpose: Print out a list of files and directories contained within a
// directory, uses ino to achieve this.
void ls_dir(int ino) {
	// Tell the ino number used
	printf("Current ino = %d\n", ino);
	// Dir doesn't exists
	if(!ino) {
		printf("There is no directory of that name.\n");
		return;
	}
	// Get the actual mInode from the number
	MINODE *mip = iget(mtable[0].dev, ino);
	// Temp variable used for printing
	char temp[256];
	// Hopefully get the Directory block into the buffer
	get_block(mtable[0].dev, mip->inode.i_block[0], buf);
	// Set the directory pointer to the buffer
	DIR *dp = (DIR *)buf;
	// Set cp to buf
	char *cp = buf;
	// While there are still directories
	while (cp < buf + BLKSIZE) {
		if (dp->rec_len == 0) return;
		if (dp->name_len == 0) return;
		// Copy directory name to temp variable and print
		strncpy(temp, dp->name, dp->name_len);
		temp[dp->name_len] = 0;
		// Print out permissions, size, then name
		ls_file(dp->inode);
		printf("%s\n", temp);
		// Advance c pointer and set dp to (DIR)cp
		cp += dp->rec_len;
		dp = (DIR *)cp;
	}
}

// LS FILE
// Helper Function
// Purpose: Print out File links, permissions, and size.
void ls_file(int ino)
{
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
    printf("%3hu %6u %2d  %9u   ", links, size, ino, mode);

    // Trace link
    if(S_ISLNK(ip->i_mode)) printf(" -> %s", (char*)ip->i_block);
}

// LS
// Purpose: Call LS DIR to print out files and directories.
void ls(char *pathname) 
{
	// If there is a pathname get it's ino and pass it into the function
	if (pathname) {
		int ino = getino(pathname);
		ls_dir(ino);
	}
	// Otherwise use current directory
	else ls_dir(running->cwd->ino);
}

// CD
// Purpose: Chnage current working directory.
void cd(char *pathname) {
	char currentpath[256];
		// If a pathname is passed in
		if (pathname) {
			// Get new ino, block information for it etc.
			int ino = getino(pathname);
			MINODE *mip = iget(mtable[0].dev, ino);
			printf("New ino will be = %d\n", ino);
			get_block(mtable[0].dev, mip->inode.i_block[0], buf);
			DIR *dp = (DIR *)buf;
			char *cp = buf;
			// Same search LS and PWD use, but now if found, set new cwd.
			while (cp < buf + BLKSIZE) {
				if(mip->ino == dp->inode) {
					printf("Previous Ino: %d\n", running->cwd->ino);
					running->cwd = mip;
				}
				// Prevents infinite loop, and cwd from being set to 0.
				if (!dp->rec_len) {
					running->cwd = root;
					return;
				}
				cp += dp->rec_len;
				dp = (DIR *)cp;
			}
		// No pathname means going to the root.
		} else running->cwd = root;
}

// MYLINK
// Purpose: Essentially copies a file, makes it with same ino.
void myLink(char *oldfile, char *newfile) {
	if (!oldfile || !newfile) return;
	printf("old = %s\nnew = %s\n", oldfile, newfile);
	char* parent, *child;
	char path[250];
	int oino, pino;
	MINODE *omip, *pmip;
	

	// Find the oldfiles Inode and ensure it is a file
	oino = getino(oldfile);
	omip = iget(mtable[0].dev, oino);
	if(S_ISDIR(omip->inode.i_mode)) {
		printf("Oldfile is a Dir\n");
		return;
	}
	
	// Used for entering child
	strcpy(path, newfile);
	parent = dirname(newfile);

	
	
	pino = getino(parent);
	pmip = iget(mtable[0].dev, pino);

	child = basename(path);
	printf("child = %s\nparent%s\n", child, parent);
	

	if(!S_ISDIR(pmip->inode.i_mode)) {
		printf("Parent of newfile is not a Dir\n");
		return;
	}
	// Ensure the child exists within the parent dir
	if (search(pmip, child) != -1) return;
	enter_child(pmip, oino, child);
	omip->inode.i_links_count++;
	omip->dirty = 1;
	iput(omip);
	iput(pmip);
}

// UNLINK
// Purpose: Current iteration acts as a rm for files.
void myUnlink(char *file) {
	char* parent, *child;
	char path[250];
	int ino, pino;
	MINODE *mip, *pmip;
	
	// Get the inode number of the file we're unlinking
	ino = getino(file);
	mip = iget(mtable[0].dev, ino);
	// Check that the file is not a dir
	if(S_ISDIR(mip->inode.i_mode))
	{
		printf("File is a Dir\n");
		return;
	}
	//Get basename and pathname
	strcpy(path, file);
	child = basename(path);
	parent = dirname(file);
	// Get the inode of the parent file to check whether it is still dirty
	pino = getino(parent);
	pmip = iget(mtable[0].dev, pino);
	printf("Child in unlink: %s\n", child);
	printf("mip ino = %d\n", mip->ino);
	rm_child(pmip, child);
	//pmip->dirty = 1;
	//Put the parent mip
	//iput(pmip);
	printf("mip ino = %d\n", mip->ino);
	mip->inode.i_links_count--;
	if(mip->inode.i_links_count > 0) {
		mip->dirty = 1;
	}
	else {
		idalloc(minode[0].dev, mip->ino);
		// deallocate data blocks in inode
		for (int i = 0; i < 12; i++) bdalloc(mip->dev, mip->inode.i_block[0]);
		// deallocate inode;
		mip->dirty = 0;
	}
	printf("mip ino = %d\n", mip->ino);
	// Then put the minode
	iput(mip);
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
	// Setup filename so it doesn't have the newline
	char filename[128];
	strncpy(filename, name, sizeof(name)-1);
	filename[strlen(name)-1] = '\0';
	
	
	
	int ino = getino(name);
	MINODE *mip = iget(mtable[0].dev, ino);
	// Parent doesn't exist
    if(!mip) {
        fprintf(stderr, "list_file: Null pointer to memory inode\n");
        return;
    }
	// Get various things needed for stat
    INODE *ip = &mip->inode;
    u16 mode   = ip->i_mode;
    u16 links  = ip->i_links_count;
    u32 size   = ip->i_size;
    u32 blocks = ip->i_blocks;
    u32 time = ip->i_ctime;
	
	
	
	char* xtime = ctime((time_t*)&ip->i_atime);
	char* ytime = ctime((time_t*)&ip->i_mtime);
	char* ztime = ctime((time_t*)&ip->i_ctime);
    
    static const char* Permissions = "rwxrwxrwx";

    // Trace link
    if(S_ISLNK(ip->i_mode)) printf(" -> %s", (char*)ip->i_block);

	// *************START OF PRINT BLOCK******************
	// Print Name
	printf("\n  File: '%s'\n", filename);
	// Size, blocks, IO block, filetype
	printf("  Size:%10u Blocks: %9u  IO Block:     ",size, blocks);
	if(!S_ISDIR(ip->i_mode)) printf("file\n");
	else printf("directory\n");
	// device, inode, links
	printf("Device: %9d Inode: %10d  Links: %3hu\n", mtable[0].dev, ino, links);
	// access, uid, gid
	printf("Access: (%d/", mode);
	switch(mode & 0xF000) {
        case 0x8000:  putchar('-');     break; // 0x8 = 1000
        case 0x4000:  putchar('d');     break; // 0x4 = 0100
        case 0xA000:  putchar('l');     break; // oxA = 1010
        default:      putchar('?');     break;
    }

    // Permissions
    for(int i = 0; i < strlen(Permissions); i++)
        putchar(mode & (1 << (strlen(Permissions) - 1 - i)) ? Permissions[i] : '-');
    printf(")   ");
    printf("Uid:  %2u  Gid:  %2u\n", ip->i_uid, ip->i_gid);
	// time info
	printf("Access: %s\n", xtime);
	printf("Modify: %s", ytime);
	printf("Change: %s\n", ztime);
	iput(mip);
}

void command(char *cmd, char *tok) {
	char file1[128];
	
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
		}/*
		else if(!strcmp(cmd, "touch")) {
			tok = strtok(NULL, " ");
			mytouch(tok);
		}*/
		else if(!strcmp(cmd, "close")) {
			tok = strtok(NULL, " ");
			myclose(tok);
		}/*
		else if(!strcmp(cmd, "lseek")) {
			tok = strtok(NULL, " ");
			mylseek(tok);
		}/*
		else if(!strcmp(cmd, "cat")) {
			tok = strtok(NULL, " ");
			mycat(tok);
		}*/
		else if(!strcmp(cmd, "cp")) {
			tok = strtok(NULL, " ");
			mycopy(tok);
		}/*
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
	if(!S_ISDIR(mip->inode.i_mode)) {
		printf("File is not a Dir\n");
		return;
	}
	//minode is not BUSY (refCount = 1);
	printf("refcount = %d\n", mip->refCount);
	if(mip->refCount > 1) {
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
