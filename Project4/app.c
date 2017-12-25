#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "myfs.h"

int main(int argc, char *argv[])
{
	char diskname[128]; 
	char filename[16][MAXFILENAMESIZE]; 
	int i, n; 
	int fd0, fd1, fd2;       // file handles
	char buf[MAXREADWRITE];

	strcpy (filename[0], "file0"); 
	strcpy (filename[1], "file1"); 
	strcpy (filename[2], "file2"); 
	
	if (argc != 2) {
		printf ("usage: app <diskname>\n"); 
		exit (1);
	}
       
	strcpy (diskname, argv[1]); 
	
	if (myfs_mount (diskname) != 0) {
		printf ("could not mount %s\n", diskname); 
		exit (1); 
	}
	else 
		printf ("filesystem %s mounted\n", diskname); 
	
	for (i=0; i<3; ++i) {
		if (myfs_create (filename[i]) != 0) {
			printf ("could not create file %s\n", filename[i]); 
			exit (1); 
		}
		else 
			printf ("file %s created\n", filename[i]); 
	}

	int y = myfs_close (fd0); 
	if (y != 0) 
		printf ("You cannot close before opening it!\n");


	fd0 = myfs_open (filename[0]); 	
        if (fd0 == -1) {
		printf ("file open failed: %s\n", filename[0]); 
		exit (1); 
	}
	else
		printf ("With %d id, file opened: %s\n", fd0,filename[0]);
	
	for (i=0; i<100; ++i) {
		n = myfs_write (fd0, buf, 500);  
		if (n != 500) {
			printf ("vsfs_write failed\n"); 
			exit (1); 
		}
	}

	int x = myfs_close (fd0); 
	if (x == 0) 
		printf ("With %d id, file closed: %s\n", fd0,filename[0]);

	fd0 = myfs_open (filename[0]); 
	if (fd0 == -1) {
		printf ("file open failed: %s\n", filename[0]); 
		exit (1); 
	}
	else
		printf ("With %d id, file opened: %s\n", fd0,filename[0]);
 
	for (i=0; i<(100*500); ++i) 
	{
		n = myfs_read (fd0, buf, 1); 
		if (n != 1) {
			printf ("vsfs_read failed\n"); 
			exit(1); 
		}
	}
	
	 fd1 = myfs_open (filename[1]); 
	 if (fd0 == -1) {
		printf ("file open failed: %s\n", filename[1]); 
		exit (1); 
	 }
	 else
		printf ("With %d id, file opened: %s\n", fd1,filename[1]);

	 fd2 = myfs_open (filename[2]);
	if (fd2 == -1) {
		printf ("file open failed: %s\n", filename[2]); 
		exit (1); 
	}
	else
		printf ("With %d id, file opened: %s\n", fd2,filename[2]); 

	 myfs_close (fd1);
	 myfs_close (fd2); 	 

	myfs_umount(); 

	return (0);		
}