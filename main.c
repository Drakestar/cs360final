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


void ls_dir(char *dirname) {
	// Get inode number of directory
	int ino = getino(dirname);
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
	printf("%d\n", ip->i_size);
}

// Given a cwd return string of cwd
void lsHelper(MINODE *wd, char path[]) {
	char temp[1024];
	// Base case
	if (wd == root) {
		strcat(path, "/");
		printf("path = %s\n", path);
		return;
	}
	// Get inode of itself and parent
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
	lsHelper(pip, path);
    strcat(path, temp);
    //printf("/%s", temp);
    return path;
}

void ls(char *pathname) 
{
	//MINODE *cwd = malloc(sizeof(MINODE));
	MINODE *mip;
	int ino;
	char currentpath[256];
	char path_cp[256], temp[256];
	// A path name was passed in so we either need Absolute or relative LS
	if (pathname) {
		// Absolute means that it will either work or it won't (Should check for that in ls_dir?)
		if (pathname[0] == '/') {
			ls_dir(pathname);
		// From current directory go forward
		} else {
			lsHelper(running->cwd, currentpath);
			strcat(currentpath, pathname);
			printf("currentpath = %s\n", currentpath);
			ls_dir(pathname);
		}
	} 
	// Current directory
	else {
		// If at root pass root to ls_dir
		if (running->cwd == root) {
			ls_dir("/\n");
		// Otherwise not at root
		} else {
			lsHelper(running->cwd, currentpath);
			ls_dir(currentpath);
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
				lsHelper(running->cwd, currentpath);
				strcat(currentpath, pathname);
				int ino = getino(currentpath);
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
	// Get inode of itself and parent
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

char *device = "mydisk";
int main(int argc, char *argv[]) 
{
	// Used for getting input
	char line[128];
	// In case user has a different file other than mydisk
	if (argc > 1) device = argv[1];
	// Initiate and mount the root
	init();
	mount_root(device);
	
	// Main loop
	while(1) 
	{
		// Print commands and get input
		printf("Commands: [ls|cd|pwd|quit]\ninput command:");
		fgets(line, 128, stdin);
		tok = strtok(line," ");
		// look into removing "/n"
		//Get command token
		if(tok) cmd = tok;
		
		// Exit, kept it to one line because there's nothing else to it
		if(!strcmp(cmd, "quit\n")) quit();
		
		// LS
		if(!strcmp(cmd, "ls") | (!strcmp(cmd, "ls\n"))) {
			// Use ls with pathname, which is retrieved from second token
			tok = strtok(NULL," ");
			ls(tok);
		}
		
		// CD
		if(!strcmp(cmd, "cd\n") | (!strcmp(cmd, "cd"))) { 
			// Also uses second token to get a pathname
			tok = strtok(NULL," ");
			cd(tok);
		}
		// PWD
		if(!strcmp(cmd, "pwd\n")) {
			
			pwd(running->cwd);
		}
		// Space for other commands (format below)
		// if(!strcmp(cmd, "com\n")) { }
	}
}
