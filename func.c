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
    printf("%3hu %6u %2d  ", links, size, ino);

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
