#include "util.c"

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

void mount_root(char *device) 
{
	// Returns smallest integer greater than zero, (the file descriptor)
	fd = open(device, O_RDWR);
	// If the fd is under 0 it means it failed
	if (fd < 0) 
	{
		printf("open: %s failed\n", device);
		exit(1);
	}
	// Read the SuperBlock into a buffer
	// fd is the block, 1 is the array of the super, and buf is where i
	get_block(fd, 1, buf);
	printf("inode size: %d\n", sizeof(INODE));
	sp = (SUPER *)buf;
	// Read the group descriptor block
	if (sp->s_magic != 0xEF53) 
	{
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
	
	//running = &proc[0];
	running = malloc(sizeof(PROC));
	memcpy(running, &proc[0], sizeof(PROC));

	
}

void quit() {
	//iput() all minodes with (refCount > 0 && DIRTY);
	for (int i = 0; i < NMINODES; i++) {
		if ((minode[i].refCount > 0) && (minode[i].dirty)) {
			iput(&minode[i]);
		}
	}
	exit(0);
}

void pwd(MINODE *wd) {
	// If it's at the root from the start just print that
	if (wd == root) printf("/");
	else rpwd(wd); // Otherwise recursivley print working directory
	printf("\n"); // Newline to look nice
}

void rpwd(MINODE *wd) {
	char temp[256];
	// Base case
	if (wd == root) return;
	// Get inodenumber of itself and parent
	int mynode = search(wd, ".");
	int parent = search(wd, "..");
	// Get MINODE of parent
    MINODE *pip = iget(mtable[0].dev, parent);
    //from pip->INODE.i_block[0]: get my_name string as LOCAL
    // Get the block from parent
    get_block(mtable[0].dev, pip->inode.i_block[0], buf);
	DIR *dp = (DIR *)buf;
	char *cp = buf;
	while (cp < buf + BLKSIZE) {
		// Difference of ls_dir if they find their inode, break with temp intact
		if (dp->inode == mynode) {
			strncpy(temp, dp->name, dp->name_len);
			temp[dp->name_len] = 0;
			break;
		}
		cp += dp->rec_len;
		dp = (DIR *)cp;
	}
	// Call rpwd with parent inode pointer
    rpwd(pip);
    printf("/%s", temp);
}

void ls_dir(char *dirname) {
	// Get inode number of directory
	int ino = getino(dirname);
	printf("ino = %d\n", ino);
	if (!ino) return;
	// Get the actual mInode from the number
	MINODE *mip = iget(mtable[0].dev, ino);
	// Temp variable used for printing
	char temp[256];
	// Get the (hopefully) Directory block into the buffer
	get_block(mtable[0].dev, mip->inode.i_block[0], buf);
	// Set the directory pointer to the buffer
	DIR *dp = (DIR *)buf;
	// Set cp to buf
	char *cp = buf;
	// While there are still directories
	while (cp < buf + BLKSIZE) {
		// Copy directory name to temp variable and print
		strncpy(temp, dp->name, dp->name_len);
		temp[dp->name_len] = 0;
		printf("dirname: %s   ", temp);
		// Advance c pointer and set dp to (DIR)cp
		ls_file(dp->inode);
		cp += dp->rec_len;
		dp = (DIR *)cp;
	}
}

void rel_dir(int ino) {
	// Get inode number of directory
	printf("ino = %d\n", ino);
	if (!ino) return;
	// Get the actual mInode from the number
	MINODE *mip = iget(mtable[0].dev, ino);
	// Temp variable used for printing
	char temp[256];
	// Get the (hopefully) Directory block into the buffer
	get_block(mtable[0].dev, mip->inode.i_block[0], buf);
	// Set the directory pointer to the buffer
	DIR *dp = (DIR *)buf;
	// Set cp to buf
	char *cp = buf;
	// While there are still directories
	while (cp < buf + BLKSIZE) {
		// Copy directory name to temp variable and print
		strncpy(temp, dp->name, dp->name_len);
		temp[dp->name_len] = 0;
		printf("dirname: %s   ", temp);
		// Advance c pointer and set dp to (DIR)cp
		ls_file(dp->inode);
		cp += dp->rec_len;
		dp = (DIR *)cp;
	}
}

void ls_file(int ino) {
	// Using the inode pointer we COULD print out information on the directory
	INODE *ip = iget(mtable[0].dev, ino);
	printf("       ino = %d   size = %d    ", ino, ip->i_size);
	if(!S_ISDIR(ip->i_mode)) printf("FILE\n");
	else printf("DIR\n");
}



void ls(char *pathname) 
{
	//MINODE *cwd = malloc(sizeof(MINODE));
	MINODE *mip;
	int ino;
	char *currentpath;
	char path_cp[256], temp[256];
	// A path name was passed in so we either need Absolute or relative LS
	if (pathname) ls_dir(pathname);
	// Current directory
	else {
		// If at root pass root to ls_dir
		if (running->cwd == root) ls_dir("/");
		// Otherwise not at root
		else {
			rel_dir(running->cwd->ino);
		}
	}
}

// Still needs relative implementation
void cd(char *pathname) {
	char currentpath[256];
		// If a pathname is passed in
		if (pathname) {
			// If the pathname is absolute
			if (pathname[0] == '/') {
				int ino = getino(pathname);
				MINODE *mip = iget(mtable[0].dev, ino);
				printf("ino = %d\n", ino);
				get_block(mtable[0].dev, mip->inode.i_block[0], buf);
				DIR *dp = (DIR *)buf;
				char *cp = buf;
				while (cp < buf + BLKSIZE) {
					if(mip->ino == dp->inode)
					{
						printf("Before cwd change: %d\n", running->cwd->ino);
						running->cwd = mip;
						//memcpy(running->cwd, mip, sizeof(MINODE));
						printf("After cwd change: %d\n", running->cwd->ino);
					}
					cp += dp->rec_len;
					dp = (DIR *)cp;
				}
			// Otherwise it's a relative pathname
			} else {
				int ino = getino(pathname);
				MINODE *mip = iget(mtable[0].dev, ino);
				printf("ino = %d\n", ino);
				get_block(mtable[0].dev, mip->inode.i_block[0], buf);
				DIR *dp = (DIR *)buf;
				char *cp = buf;
				while (cp < buf + BLKSIZE) {
					if(mip->ino == dp->inode)
					{
						printf("Before cwd change: %d\n", running->cwd->ino);
						running->cwd = mip;
						//memcpy(running->cwd, mip, sizeof(MINODE));
						printf("After cwd change: %d\n", running->cwd->ino);
					}
					cp += dp->rec_len;
					dp = (DIR *)cp;
				}
			}
		// Otherwise go to the root
		} else {
			running->cwd = root;
}

}
