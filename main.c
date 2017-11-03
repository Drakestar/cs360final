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
	printf("ino = %d   size = %d    ", ino, ip->i_size);
	if(!S_ISDIR(ip->i_mode)) printf("FILE\n");
	else printf("Dir\n");
}

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

void Mkdir(char *pathname)
{
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

void my_mkdir(MINODE *pmip, char *name)
{
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
	dp = (char *)dp + 12;
	dp->inode = pmip->ino;
	dp->rec_len = BLKSIZE-12; // rec_len spans block
	dp->name_len = 2;
	dp->name[0] = dp->name[1] = '.';
	put_block(dev, blk, buf); // write to blk on diks
	
	
	//entering newdir into parent dir
	enter_child(pmip, ino, name);
	
	//increment parents inode links by 1 and mark pmip dirty
	pmip->inode.i_links_count++;
	pmip->dirty = 1;
	
	
	
	iput(pmip);
}

void rmdir(char * pathname)
{
	int count = 0;
	char * name;
	// get in-memory INODE of pathname:
	int ino = getino(pathname);
	MINODE * mip = iget(dev, ino);

	//verify INODE is a DIR (by INODE.i_mode field);
	if(!S_ISDIR(mip->inode.i_mode))
	{
		printf("File is not a Dir\n");
		return;
	}
	
	//minode is not BUSY (refCount = 1);
	if(mip->refCount)
	{
		printf("Dir is busy\n");
	}
	
	//verify DIR is empty (traverse data blocks for number of entries = 2);
	if(mip->inode.i_links_count == 2)
	{
		//traverse data blocks insearch of files
		// Go through the blocks WE'll Be using
		for (int i = 0; i < 12; i++) 
		{
			// Get the i_block of the inode
			get_block(dev, mip->inode.i_block[i] , buf);
			dp = (DIR *)buf;
			char *cp = buf;
			
			while (cp < buf + BLKSIZE) 
			{
				if(dp->rec_len == 0) 
				{
					break;
				}
				else
				{
					count++;
				}
				cp += dp->rec_len;
				dp = (DIR *)cp;
			}
		}
	}
	if(mip->inode.i_links_count > 2 || count > 0)
	{
		printf("Dir not empty\n");
		return;
	}
	
	/* get parent's ino and inode */
	int pino = findino(); //get pino from .. entry in INODE.i_block[0]
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
	pmip->inode.i_link_count--;
	pmip->dirty = 1;
	iput(pmip);
}

int rm_child(MINODE * pmip,char * name)
{
	int ino, tempRecLen;
	char * temp;
	ino = search(pmip, name);
	
	for(i = 0; i < 12 ; i++)
	{
		get_block(mtable[0].dev, pmip->inode.i_block[i], buf);
		DIR *dp = (DIR *)buf;
		char *cp = buf;
		
		
		while (cp < buf + BLKSIZE) {
			// Difference of ls_dir if they find their inode, break with temp intact
			if (ino == dp->inode) {
				break;
			}
			cp += dp->rec_len;
			dp = (DIR *)cp;
		}	
		
		if(dp->rec_len == BLKSIZE)
		{
			//deallocate dir
			free(buf);
			idalloc(mtable[0].dev, ino);
			pmip->inode.i_size -= BLKSIZE;
			
			//move things left
			for(; i < 11; i++)
			{
					get_block(dev, pmip->inode.i_block[i], buf);
					put_block(dev, pmip->inode.i_block[i - 1], buf);
			}
		}
		else if(cp + dp->rec_len == buf + BLKSIZE) //cp is the beginning position of Dir. Buf is the beginning position of first dir
		{
			//holding onto cur dir's len
			tempRecLen = dp->rec_len;
			
			//getting previous dir
			cp -=  dp->rec_len;
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
			
			//move everythng left
			memmove(cp, cp + dp->rec_len, buf + BLKSIZE);
			
			/*while (cp < buf + BLKSIZE) 
			{
				// Move things left
				//copy next dir into previous spot
				memcpy(cp, cp + dp->rec_len, dp->rec_len); //wrong size, need the size of the next dir instead of current one
				
				//move on to 
				cp += dp->rec_len;
				dp = (DIR *)cp;
			}	*/

			//add deleted rec_len to last dir
			dp->rec_len += tempRecLen;
			
			//put_block
			put_block(dev, pmip->inode.i_block[i], buf);
			
		}
	}
	pmip->dirty = 1;
	
	iput(pmip);
}

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
		printf("Commands: [ls|cd|pwd|mkdir|creat|rmdir|quit]\ninput command:");
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
		// mkdir
		if(!strcmp(cmd, "mkdir")) {
			tok = strtok(NULL, " ");
			Mkdir(tok);
		}
		// creat
		if(!strcmp(cmd, "creat")) {
			tok = strtok(NULL, " ");
			Creat(tok);
		}
		// rmdir
		if(!strcmp(cmd, "rmdir")) {
			tok = strtok(NULL, " ");
			rmdir(tok);
		}
		// Space for other commands (format below)
		// if(!strcmp(cmd, "com\n")) { }
	}
}
