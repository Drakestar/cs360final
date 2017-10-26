#include "util.c"

// globals
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
	sp = (SUPER *)buf;
	// Read the group descriptor block
	printf("%-30s = %8x ", "s_magic", sp->s_magic);
	if (sp->s_magic != 0xEF53) 
	{
		printf("Not an ext2 FS\n");
		exit(2);
	}
	
	printf("EXT2 FS OK\n");
	
	root = iget(fd, 2);
	mtable[0].dev = fd;
	mtable[0].ninodes = sp->s_inodes_count;
	mtable[0].nblocks = sp->s_blocks_count;
	get_block(fd, 2, buf);
	gp = (GD *)buf;
	mtable[0].bmap = gp->bg_block_bitmap;
	mtable[0].imap = gp->bg_inode_bitmap;
	mtable[0].iblock = gp->bg_inode_table;
	mtable[0].mounted_inode = root;
	strcpy(mtable[0].deviceName, device);
	strcpy(mtable[0].mountedDirName, "/");
	
	proc[0].cwd = iget(fd, 2);
	proc[1].cwd = iget(fd, 2);
	
	running = &proc[0];

}

void ls_dir(char *dirname) {
	int ino = getino(dirname);
	MINODE *mip = iget(dev, ino);
	char *temp;
	get_block(mtable[0].dev, mtable[0].iblock, buf);
	INODE *ip = (INODE *)buf;
	ip++;
	get_block(mtable[0].dev, ip->i_block[0], buf);
	DIR *dp = (DIR *)buf;
	char *cp = buf;
	while (cp < buf + BLKSIZE) {
		strncpy(temp, dp->name, dp->name_len);
		temp[dp->name_len] = 0;
		cp += dp->rec_len;
		dp = (DIR *)cp;
		ls_file(dp->inode);
	}
	
}

void ls_file(int ino) {
	INODE *mip = iget(mtable[0].dev, ino);
	printf("%d\n", mip->i_mode);
}

void ls(char *pathname) 
{
	// Split pathname into child and parent
	// Check parent exists and is dir
	//
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

/*
void cd(char *pathname) {
	// no pathname mean cd to root
	if (!pathname) {
		// CD to root
	}
	// Otherwise there is a pathname
	else {
		mtable[0].inode = getino(pathname);
		MINODE mip = iget(dev, mtable[0].ino);
		// Verify mip->inode is a dir
		if (mip->inode) {
			iput(running->cwd);
			running->cwd = mip;
		}
	}
}

void pwd(MINODE *wd) {
	if (wd == root) printf("/");
	else rpwd(wd);
}

void rpwd(MINODE *wd) {
	if (wd == root) return;
	// from i_block[0] of wd->INODE: get my_ino, parent_ino
     pip = iget(dev, parent_ino);
     from pip->INODE.i_block[0]: get my_name string as LOCAL

     rpwd(pip);  // recursive call rpwd() with pip

     printf("/%s", my_name);
*/
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
		printf("%s\n", cmd);
		// Exit No code
		if(!strcmp(cmd, "quit\n")) 
		{
			printf("You typed exit, goodbye!\n"); 
			quit();
		}
		// LS
		if(!strcmp(cmd, "ls") | (!strcmp(cmd, "ls\n")))
		{
			// Use ls with pathname, which is retrieved from second token
			tok = strtok(NULL," ");
			//ls();
		}
		// CD
		if(!strcmp(cmd, "cd\n")) 
			// Also uses second token to get a pathname
			tok = strtok(NULL," ");
		
		{
		
		}
		// PWD
		if(!strcmp(cmd, "pwd\n"))
		{
			// Where do i get the parameter for this?
			//pwd();
		}
	}
}
