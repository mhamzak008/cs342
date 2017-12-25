#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "myfs.h"

// Global Variables
char disk_name[128];   // name of virtual disk file
int  disk_size;        // size in bytes - a power of 2
int  disk_fd;          // disk file handle
int  disk_blockcount;  // block count on disk

struct FCB
{
	int startBlock;
	int size;	
};

struct directEntry
{
	struct FCB fcb;
	int available;
	char fileName[MAXFILENAMESIZE];	
};

struct directEntry directory[MAXFILECOUNT];
char FAT[BLOCKCOUNT];

struct openFileTableEntry	// element of open file table
{
	int available;
	struct FCB * fcb;
	int position;
	char fileName[MAXFILENAMESIZE];		
	int openCount;
};

struct openFileTableEntry openFileTable[MAXOPENFILES];
int mountedFlag = 0;

/* 
   Reads block blocknum into buffer buf.
   You will not modify the getblock() function. 
   Returns -1 if error. Should not happen.
*/

int getblock (int blocknum, void *buf)
{      
	int offset, n; 
	
	if (blocknum >= disk_blockcount) 
		return (-1); //error

	offset = lseek (disk_fd, blocknum * BLOCKSIZE, SEEK_SET); 
	if (offset == -1) {
		printf ("lseek error\n"); 
		exit(0); 

	}

	n = read (disk_fd, buf, BLOCKSIZE); 
	if (n != BLOCKSIZE) 
		return (-1); 

	return (0); 
}


/*  
    Puts buffer buf into block blocknum.  
    You will not modify the putblock() function
    Returns -1 if error. Should not happen. 
*/
int putblock (int blocknum, void *buf)
{
	int offset, n;
	
	if (blocknum >= disk_blockcount) 
		return (-1); //error

	offset = lseek (disk_fd, blocknum * BLOCKSIZE, SEEK_SET);
	if (offset == -1) {
		printf ("lseek error\n"); 
		exit (1); 
	}
	
	n = write (disk_fd, buf, BLOCKSIZE); 
	if (n != BLOCKSIZE) 
		return (-1); 

	return (0); 
}


/* 
   IMPLEMENT THE FUNCTIONS BELOW - You can implement additional 
   internal functions. 
 */


int myfs_diskcreate (char *vdisk)
{
	int n, size ,ret, i;
	int fd;  
	char vdiskname[128]; 
	char buf[BLOCKSIZE];     
	int numblocks = 0; 

	strcpy (vdiskname, vdisk); 
        size = DISKSIZE; 
	numblocks = DISKSIZE / BLOCKSIZE; 

	printf ("diskname=%s size=%d blocks=%d\n", 
		vdiskname, size, numblocks); 
       
	ret = open (vdiskname,  O_CREAT | O_RDWR, 0666); 	
	if (ret == -1) {
		printf ("could not create disk\n"); 
		exit(1); 
	}
	
	bzero ((void *)buf, BLOCKSIZE); 
	fd = open (vdiskname, O_RDWR); 
	for (i=0; i < (size / BLOCKSIZE); ++i) {
		printf ("block=%d\n", i); 
		n = write (fd, buf, BLOCKSIZE); 
		if (n != BLOCKSIZE) {
			printf ("write error\n"); 
			exit (1); 
		}
	}	
	close (fd); 
	
	printf ("created a virtual disk=%s of size=%d\n", vdiskname, size);	

	return(0); 
}


/* format disk of size dsize */
int myfs_makefs(char *vdisk)
{
	strcpy (disk_name, vdisk); 
	disk_size = DISKSIZE; 
	disk_blockcount = disk_size / BLOCKSIZE; 

	disk_fd = open (disk_name, O_RDWR); 
	if (disk_fd == -1) {
		printf ("disk open error %s\n", vdisk); 
		exit(1); 
	}
	
	// perform your format operations here. 
	printf ("formatting disk2=%s, size=%d\n", vdisk, disk_size);

	// Creating and initializing directory
	struct directEntry directory[MAXFILECOUNT];
	for (int i = 0; i < MAXFILECOUNT; i++)
	{
		directory[i].available = 1;
		directory[i].fcb.size = 0;
		directory[i].fcb.startBlock = -1;
	}

	// Creating and initializing free space manager
	// first 1/4 blocks to be reserved for meta-data
	char FAT[BLOCKCOUNT];
	for (int i = 0; i < BLOCKCOUNT; i++)
	{
		if(i < (BLOCKCOUNT/4))
			FAT[i] = 'n';
		else
			FAT[i] = '0';
	}

	// Storing directory in the disk in blocks 1 and 2
	
	// Creating a disk block
	char block[BLOCKSIZE];

	// copying first half of the directory to the block
	memcpy(block, directory, 64*(sizeof(struct directEntry)));

	// putting the block into disk
	if(putblock(1, block) != 0)
		return -1;

	// copying the second half of the directory to the block
	memcpy(block, &directory[64], 64*(sizeof(struct directEntry)));

	// putting the block into disk
	if(putblock(2, block) != 0)
		return -1;

	// storing FAT in the disk from blocks 3 to 10  
	int blockNum = 3;
	for(int i = 0; i < BLOCKCOUNT; i = i + BLOCKSIZE)
	{
		memcpy(block, &FAT[i], BLOCKSIZE);
		putblock(blockNum, block);
		blockNum++;
	}

	fsync (disk_fd); 
	close (disk_fd); 

	return (0); 
}

/* 
   Mount disk and its file system. This is not the same mount
   operation we use for real file systems:  in that the new filesystem
   is attached to a mount point in the default file system. Here we do
   not do that. We just prepare the file system in the disk to be used
   by the application. For example, we get FAT into memory, initialize
   an open file table, get superblock into into memory, etc.
*/

int myfs_mount (char *vdisk)
{
	if (mountedFlag == 0)
	{
		struct stat finfo; 

		strcpy (disk_name, vdisk);
		disk_fd = open (disk_name, O_RDWR); 
		if (disk_fd == -1) {
			printf ("myfs_mount: disk open error %s\n", disk_name); 
			exit(1); 
		}
	
		fstat (disk_fd, &finfo); 

		printf ("myfs_mount: mounting %s, size=%d\n", disk_name, 
			(int) finfo.st_size);  
		disk_size = (int) finfo.st_size; 
		disk_blockcount = disk_size / BLOCKSIZE; 

		// getting the directory blocks from disk	
		char block[BLOCKSIZE];
		if(getblock(1, block) != 0)
			return -1;
		char block2[BLOCKSIZE];
		if(getblock(2, block2) != 0)
			return -1;

		// Copying directory from the blocks
		memcpy(directory, block, 64*(sizeof(struct directEntry)));
		memcpy(&directory[64], block2, 64*(sizeof(struct directEntry)));

		// getting the FAT from disk
		int blockNum = 3;
		for(int i = 0; i < BLOCKCOUNT; i = i + BLOCKSIZE)
		{
			if(getblock(blockNum, block) != 0) // getting the block
				return -1;	
			memcpy(&FAT[i], block, BLOCKSIZE);	// copying the block to FAT
			blockNum++;
		}

		// initializing the system-wide open-file table
		for (int i = 0; i < MAXOPENFILES; i++)
		{
			openFileTable[i].available = 1;
		}

		mountedFlag = 1;
	}
	else
		return -1;

	return (0); 
}


int myfs_umount()
{
	if (mountedFlag == 1)
	{
		// Storing directory back in the disk in blocks 1 and 2

		// Creating a disk block
		char block[BLOCKSIZE];

		// copying first half of the directory to the block
		memcpy(block, directory, 64*(sizeof(struct directEntry)));

		// putting the block into disk
		if(putblock(1, block) != 0)
			return -1;

		// copying the second half of the directory to the block
		memcpy(block, &directory[64], 64*(sizeof(struct directEntry)));

		// putting the block into disk
		if(putblock(2, block) != 0)
			return -1;

		// storing FAT in the disk from blocks 3 to 10  
		int blockNum = 3;
		for(int i = 0; i < BLOCKCOUNT; i = i + BLOCKSIZE)
		{
			memcpy(block, &FAT[i], BLOCKSIZE);
			putblock(blockNum, block);
			blockNum++;
		}

		fsync (disk_fd); 
		close (disk_fd);

		mountedFlag = 0;
	}
	else
		return -1;

	return (0); 
}

/* create a file with name filename */
int myfs_create(char *filename)
{
	if(sizeof(filename) > MAXFILENAMESIZE)
		return -1;

	if(mountedFlag == 1)
	{
		// check if file already exists
		for(int i = 0; i < MAXFILECOUNT; i++)
		{
			if(strcmp(directory[i].fileName, filename) == 0)
				return -1;
		}

		// find an empty directory to store the file
		int found = 0;
		for(int i = 0; (i < MAXFILECOUNT) && (found == 0); i++)
		{
			if(directory[i].available == 1)	// empty directory found
			{
				found = 1;
				directory[i].available = 0;
				strcpy(directory[i].fileName, filename);
			}
		}
	}	
	else
		return -1;
	
	return 0;
}

/* open file filename */
int myfs_open(char *filename)
{
	int index = -1;

	if(mountedFlag == 1){
		//file is already open
		for(int i = 0; i < MAXOPENFILES; i++){
			if((openFileTable[i].available == 0) && (strcmp(openFileTable[i].fileName, filename) == 0)){
				openFileTable[i].position = 0;	// reset position
				openFileTable[i].openCount++;	
				return i;	// if exists then return its handler
			}
		}	

		// search for file in the directory
		int fileFound = 0;
		for(int i = 0; (i < MAXFILECOUNT) && (fileFound == 0); i++){
			if( (strcmp(directory[i].fileName, filename) == 0) && (directory[i].available == 0) )
			{	
				// file found
				fileFound = 1;
				int spaceFound = 0;
				// search for empty space in the open file table
				for(int j = 0; (j < MAXOPENFILES) && (spaceFound == 0); j++)
				{
					if (openFileTable[j].available == 1)	// empty space found
					{
						spaceFound = 1;
						openFileTable[j].available = 0;
						openFileTable[j].position = 0;
						openFileTable[i].openCount = 1;
						strcpy(openFileTable[j].fileName, filename);
						//memcpy(&(openFileTable[j].fcb), &(directory[i].fcb), sizeof(struct FCB));
						openFileTable[j].fcb = &(directory[i].fcb);
						index = j;
					}
				}
			}
		}
	}
	
	return index; 
}

/* close file filename */
int myfs_close(int fd)
{
	if(mountedFlag == 1)
	{
		if(openFileTable[fd].available == 0)	// check if file is open
		{
			openFileTable[fd].openCount--;	// close it

			if(openFileTable[fd].openCount < 1)	// all instances of file closed
			{
				openFileTable[fd].available = 1; 	
			}

			return 0;
		}
	}

	return -1; 
}

int myfs_delete(char *filename)
{
	if(mountedFlag == 1)
	{
		for(int i = 0; i < MAXOPENFILES; i++)
		{
			if(strcmp(openFileTable[i].fileName, filename) == 0 && openFileTable[i].available == 0)	// check if file is open
				return -1;
			else if(strcmp(directory[i].fileName, filename) == 0)
			{
				directory[i].available = 1;

				// clear FAT table
				int entry = directory[i].fcb.startBlock;
				if(entry != -1)
				{
					while(FAT[entry] != 'n')
					{
						char tempEntry = FAT[entry];
						FAT[entry] = '0';
						entry = tempEntry - '0';
					}

					directory[i].fcb.startBlock = -1;
				}

				return 0;
			}
		}
	}

	return -1; 
}

int myfs_read(int fd, void *buf, int n)
{
	int bytes_read = -1; 

	if(mountedFlag == 1)	// mount check
	{
		if(openFileTable[fd].available == 0)	// file opened check
		{
			if(n <= 1024)	// upper limit of n check
			{
				if(openFileTable[fd].fcb->startBlock != -1)	// check if atleast one block exists for the file
				{
					// finding the block index to read from	
					int blockReadPosition = openFileTable[fd].position / BLOCKSIZE;

					int blockReadAddress = openFileTable[fd].fcb->startBlock;
					int i = 0;					
					while((i < blockReadPosition) && (FAT[blockReadAddress] != 'n'))	
					{
						blockReadAddress = FAT[blockReadAddress] - '0';
						i++;
					}

					// finding the displacement within the block
					int displacement;
					if(openFileTable[fd].position < BLOCKSIZE)
						displacement = openFileTable[fd].position;
					else
						displacement = openFileTable[fd].position % BLOCKSIZE;

					// Read from the block
					char block[BLOCKSIZE];
					getblock(blockReadAddress, block);
					bytes_read = 0;
					int blocksFinished = 0;
					while((bytes_read < n) && (blocksFinished == 0))
					{
						memcpy(&(buf[displacement]), &(block[displacement]), 1);
						displacement++;
						bytes_read++;

						if(displacement >= BLOCKSIZE)	// If current block full
						{
							// get the next file block from FAT
							if(FAT[blockReadAddress] != 'n')	// check if not last block of file
							{
								blockReadAddress = FAT[blockReadAddress] - '0';
								getblock(blockReadAddress, block);	// get the new block
								displacement = 0;
							}
							else
								blocksFinished = 1;
						}
					}
				}
			}
		}
	}	
	return (bytes_read); 

}

int myfs_write(int fd, void *buf, int n)
{
	int bytes_written = -1; 

	if(mountedFlag == 1)	// mount check
	{
		if(openFileTable[fd].available == 0)	// file opened check
		{
			if(n <= 1024)	// upper limit of n check
			{
				if(openFileTable[fd].fcb->startBlock == -1)	// check if first write of file
				{
					// search for an empty block
					int emptyBlockFound = 0;
					for(int i = 0; (i < BLOCKCOUNT) && (emptyBlockFound == 0); i++)
					{
						if (FAT[i] == '0') 	// empty block found
						{
							emptyBlockFound = 1;

							openFileTable[fd].fcb->startBlock = i;	// point the block to file
							FAT[i] = 'n';	// mark the block as occupied

							// write to the block
							char block[BLOCKSIZE]; 
							for(int j = 0; j < n; j++)
							{
								memcpy(&(block[j]), &(buf[j]), 1);																
							}
							putblock(i, block);
							openFileTable[fd].position = n;
							bytes_written = n;
							openFileTable[fd].fcb->size = BLOCKSIZE;							
						}
					}
				}
				else	// not the first write of file
				{
					// finding the block index to write to	
					int blockWritePosition = openFileTable[fd].position / BLOCKSIZE;

					int blockWriteAddress = openFileTable[fd].fcb->startBlock;
					int i = 0;					
					while((i < blockWritePosition) && (FAT[blockWriteAddress] != 'n'))	
					{
						blockWriteAddress = FAT[blockWriteAddress] - '0';
						i++;
					}

					// finding the displacement within the block
					int displacement;
					if(openFileTable[fd].position < BLOCKSIZE)
						displacement = openFileTable[fd].position;
					else
						displacement = openFileTable[fd].position % BLOCKSIZE;

					// write to the block
					char block[BLOCKSIZE];
					getblock(blockWriteAddress, block);
					bytes_written = 0;
					int blocksFinished = 0;
					while((bytes_written < n) && (blocksFinished == 0))
					{
						memcpy(&(block[displacement]), &(buf[displacement]), 1);
						displacement++;
						bytes_written++;

						if(displacement >= BLOCKSIZE)	// If current block full
						{
							// search for a new one
							int blockFound = 0;
							for(int i = 0; (i <  BLOCKCOUNT) && (blockFound == 0); i++)
							{
								if(FAT[i] == '0')	// empty block found
								{									
									putblock(blockWriteAddress, block);	// put the current one back
									openFileTable[fd].fcb->size += BLOCKSIZE;

									blockFound = 1;

									// update FAT
									FAT[blockWriteAddress] = i + '0';
									FAT[i] = 'n';

									blockWriteAddress = i;
									getblock(blockWriteAddress, block);	// get the new block
									displacement = 0;
								}
							}

							if(blockFound == 0)
							{
								blocksFinished = 1;
							}
						}
					}
					putblock(blockWriteAddress, block);
					openFileTable[fd].fcb->size += BLOCKSIZE;
				}
			}
		}
	}

	return (bytes_written);  
} 

int myfs_truncate(int fd, int size)
{
	
	return -1; 
} 

int myfs_seek(int fd, int offset)
{
	int position = -1; 

	int fileSize;
	if(mountedFlag == 1){
		if(openFileTable[fd].available == 0){
			fileSize = openFileTable[fd].fcb->size;
			//if offset is bigger than the file size, position will be filesize
			if(fileSize <= offset){
				openFileTable[fd].position = fileSize;
				position = fileSize;
			}
			else{
				openFileTable[fd].position = offset;
				position = offset;
			}
		}
	}

	return (position); 
} 

int myfs_filesize (int fd)
{
	int size = -1; 
	
	if(mountedFlag == 1){
		if(openFileTable[fd].available == 0)
			size = openFileTable[fd].fcb->size;
	}	

	return (size); 
}


void myfs_print_dir ()
{
	if(mountedFlag == 1)
	{
		for(int i = 0; i < MAXFILECOUNT; i++)
		{
			if(directory[i].available == 0)
			{
				printf("%s\n", directory[i].fileName);
			}
		}
	}	
}


void myfs_print_blocks (char *  filename)
{
	for(int i = 0; i < MAXFILECOUNT; i++)
	{
		if( (strcmp(directory[i].fileName, filename) == 0) && (directory[i].available == 0) )
		{
			printf("%s: ",filename);
			int entry = directory[i].fcb.startBlock;
			if(entry != -1)
			{
				printf("%i ", entry);
				while(FAT[entry] != 'n')
				{
					char c = FAT[entry];
					printf("%c ", c);

					entry = FAT[entry] - '0';
				}
			}
			printf("\n");
		}
	}
}

