#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "myfs.h"

#define TEN_BLOCK_DATA_COUNT (10 * BLOCKSIZE / sizeof(long))
#define TEN_BLOCK_RW_COUNT   (10 * BLOCKSIZE / MAXREADWRITE)
#define MAX_RW_DATA_COUNT    (MAXREADWRITE / sizeof(long))

#define MULTI_FILE_COUNT     50

#define FAIL_IF(expr) if ((expr)) return test_result(FAIL)

#define LOGFILE      "log.txt"
#define TRUE         1
#define FALSE        0
#define SUCCESS      1
#define FAIL         0

#define TESTDISK4    "disk4"

#define TEST_4_1     "Test 4.1"
#define TEST_4_2     "Test 4.2"

#define TESTFILE4    "testfile4"


char *testname;

/* Logging */

void log_(char *message) {
  FILE *logfile;
  logfile = fopen(LOGFILE, "a");
  
  fprintf(logfile, "%s: %s\n", testname, message);

  fclose(logfile);
}

/* Error Handling */

int generic_handler(int result, char *str) {
  if (result < 0)
    log_(str);

  return result;
}

int Myfs_create(char *filename) {
  return generic_handler(myfs_create(filename), "Cannot create file.");
}

int Myfs_mount(char *diskname) {
  return generic_handler(myfs_mount(diskname), "Cannot mount disk.");
}

int Myfs_open(char *filename) {
  return generic_handler(myfs_open(filename), "Cannot open file.");
}

int Myfs_close(int fd) {
  return generic_handler(myfs_close(fd), "Cannot close file.");
}

int Myfs_read(int fd, void *buf, int n) {
  return generic_handler(myfs_read(fd, buf, n), "Cannot read file.");
}

int Myfs_write(int fd, void *buf, int n) {
  return generic_handler(myfs_write(fd, buf, n), "Cannot write file.");
}

int test_result(int result) {
  if (result == SUCCESS)
    printf("TRUE\n");
  else
    printf("FALSE\n");

  return result;
}

void create_and_mount(char *diskname) {
  printf("Creating disk.\n");
  myfs_diskcreate(diskname);

  printf("Formatting disk.\n");
  myfs_makefs(diskname);

  printf("Mounting disk.\n");
  myfs_mount(diskname);
}

long *generate_data(unsigned long count) {
  long i;
  long *data;

  data = (long *) malloc(count * sizeof(long));

  for (i=0; i < count; i++)
    data[i] = (i % 2 == 0 ? 1 : -1) * i*i*i; 

  return data;
}

int test_4_1() 
{
  printf("Test 4.1 Started\n");	
  int i, fd;
  long *data;

  testname = TEST_4_1;

  data = generate_data(TEN_BLOCK_DATA_COUNT);

  create_and_mount(TESTDISK4);

  printf("Creating file.\n");
  FAIL_IF((fd = Myfs_create(TESTFILE4)) < 0);

  printf("Writing content.\n");
  for (i=0; i < TEN_BLOCK_RW_COUNT; i++)
    FAIL_IF(Myfs_write(fd, (void *) &data[i * MAX_RW_DATA_COUNT], MAXREADWRITE) != MAXREADWRITE);

  myfs_print_blocks(TESTFILE4);

  printf("Closing file.\n");
  FAIL_IF(Myfs_close(fd) < 0);
  
  myfs_umount();
  free(data);

  return test_result(SUCCESS);
}

/* Test 4.2: Read file with 10 blocks */
int test_4_2() {
  printf("Test Started:\n");
  int i, j, fd;
  long buf[MAX_RW_DATA_COUNT];
  long *data;

  testname = TEST_4_2;

  data = generate_data(TEN_BLOCK_DATA_COUNT);

  FAIL_IF(Myfs_mount(TESTDISK4) < 0);

  printf("Opening file.\n");
  FAIL_IF((fd = Myfs_open(TESTFILE4)) < 0);

   myfs_print_blocks(TESTFILE4);

  printf("Reading and comparing content.\n");
  for (i=0; i < TEN_BLOCK_RW_COUNT; i++) 
  {
    FAIL_IF(Myfs_read(fd, (void *)buf, MAXREADWRITE) != MAXREADWRITE);

    for (j=0; j < MAX_RW_DATA_COUNT; j++)
      FAIL_IF(buf[j] != data[i * MAX_RW_DATA_COUNT + j]);
  } 

  printf("Closing file.\n");
  FAIL_IF(Myfs_close(fd) < 0);
  
  myfs_umount();
  free(data);

  return test_result(SUCCESS);
}

int main(int argc, char *argv[])
{
	test_4_1();

	test_4_2();

	return (0);		
}