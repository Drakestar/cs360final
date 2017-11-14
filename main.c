#include "func.c"

// PWD in dir causes segfault
// Mkdir doesn't actually make a dir
// symlink
// readlink
// quit doesn't quite save things
// add chmod
// add utime


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

void myReadLink(char *file) {
	char* parent, *child;
	char path[250];	
	// Check the file exist
	strcpy(path, file);
	child = basename(path);
	parent = dirname(file);
	int pino = getino(parent);
	MINODE *pmip = iget(mtable[0].dev, pino);
	// Verify link file
	if (!S_ISLNK(pmip->inode.i_mode)) return;
	// While buf loop to look for filename
	//strcpy(buf, pmip->inode.i_block[]);
	// Print out file size i_size
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

void rmdir(char * pathname)
{
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
		printf("Commands: [ls|cd|pwd|mkdir|creat|rmdir|link|unlink|symlink|quit]\ninput command:");
		fgets(line, 128, stdin);
		tok = strtok(line," ");
		// look into removing "/n"
		//Get command token
		if(tok) cmd = tok;
		printf("command: %s\n", cmd);
		
		// Exit, kept it to one line because there's nothing else to it
		if(!strcmp(cmd, "quit\n")) quit();
		
		// LS
		if(!strcmp(cmd, "ls") | (!strcmp(cmd, "ls\n"))) {
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
		//READLINK
		else if(!strcmp(cmd, "readlink")) {
			tok = strtok(NULL, " ");
			myReadLink(tok);
		}
		// Space for other commands (format below)
		// if(!strcmp(cmd, "com\n")) { }
	}
}
