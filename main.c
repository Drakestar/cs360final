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
	int ino = getino(dirname);
	MINODE *mip = iget(dev, ino);
	char temp[256];
	get_block(dev, mip->inode.i_block[0], buf);
	DIR *dp = (DIR *)buf;
	char *cp = buf;
	while (cp < buf + BLKSIZE) {
		strncpy(temp, dp->name, dp->name_len);
		temp[dp->name_len] = 0;
		printf("dirname: %s   ", temp);
		cp += dp->rec_len;
		dp = (DIR *)cp;
		ls_file(dp->inode);
	}
}

void ls_file(int ino) {
	int n;
	INODE *ip = iget(dev, ino);
	printf("%d\n", ip->i_size);
}

void ls(char *pathname) 
{
	//MINODE *cwd = malloc(sizeof(MINODE));
	MINODE *mip;
	int ino;
	char path_cp[256], temp[256];
	// Absolute or relative pathname
	if (pathname) {
		// Absolute
		if (pathname[0] == '/') {
			ls_dir(pathname);
		}
		// Relative, find dirname starting from current location
		else {
			ls_dir(pathname);
		}
	} 
	// Current directory
	else {
		// If at root pass that through
		if (running->cwd == root) {
			printf("am root\n");
			printf("ROOT ino: %d\n", root->ino);

			ls_dir("/");
		}
		else
		{
			printf("LS running ino: %d\n", running->cwd->ino);
			char temp[256];
			int mynode = search(running->cwd, ".");
			int parent = search(running->cwd, "..");
			printf("LS mynode ino: %d\n", mynode);
			printf("LS parents ino: %d\n", parent);
			
			// from i_block[0] of wd->INODE: get my_ino, parent_ino
			MINODE *pip = iget(dev, parent);
			//from pip->INODE.i_block[0]: get my_name string as LOCAL
			get_block(mtable[0].dev, pip->inode.i_block[0], buf);
			DIR *dp = (DIR *)buf;
			char *cp = buf;
			while (cp < buf + BLKSIZE) {
				if (dp->inode == mynode) {
					strncpy(temp, dp->name, dp->name_len);
					temp[dp->name_len] = 0;
				}
				cp += dp->rec_len;
				dp = (DIR *)cp;
			}
			printf("temp = %s\n", temp);
			//ls_dir(temp);
		}
	}
}

void cd(char *pathname) {

		// Works for given pathname
		int ino = getino(pathname);
		MINODE *mip = iget(dev, ino);
		printf("ino = %d\n", ino);
		get_block(dev, mip->inode.i_block[0], buf);
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

void pwd(MINODE *wd) {
	if (wd == root) printf("/");
	else {
		printf("not root\n");
		rpwd(wd);
	}
	printf("\n");
}

void rpwd(MINODE *wd) {
	char *temp;
	if (wd == root) return;
	int mynode = search(wd, ".");
	int parent = search(wd, "..");
	// from i_block[0] of wd->INODE: get my_ino, parent_ino
    MINODE *pip = iget(dev, parent);
    //from pip->INODE.i_block[0]: get my_name string as LOCAL
    get_block(mtable[0].dev, ip->i_block[0], buf);
	DIR *dp = (DIR *)buf;
	char *cp = buf;
	while (cp < buf + BLKSIZE) {
		if (dp->inode == mynode) {
			strncpy(temp, dp->name, dp->name_len);
			temp[dp->name_len] = 0;
		}
		cp += dp->rec_len;
		dp = (DIR *)cp;
	}
	
    rpwd(pip);  // recursive call rpwd() with pip
    printf("/%s", temp);
}

char *device = "mydisk";
int main(int argc, char *argv[]) 
{
	// Used for getting input
	char line[128];
	// In case user has a different file other than mydisk
	if (argc > 1) device = argv[1];
	
	init();
	mount_root(device);
	
	// Main loop
	while(1) 
	{
		printf("Commands: [ls|cd|pwd|quit]\ninput command:");
		fgets(line, 128, stdin);
		tok = strtok(line," ");
		//Get command token
		if(tok) cmd = tok;
		// Exit No code
		if(!strcmp(cmd, "quit\n")) 
		{
			printf("You typed quit, goodbye!\n"); 
			quit();
		}
		// LS
		if(!strcmp(cmd, "ls") | (!strcmp(cmd, "ls\n")))
		{
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
		if(!strcmp(cmd, "pwd\n"))
		{
			// Where do i get the parameter for this?
			pwd(running->cwd);
		}
	}
}
