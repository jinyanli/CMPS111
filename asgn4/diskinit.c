#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <ctype.h>
/*
   this program creates a disk image. Initialize super block and file allocation table.
   it also creates a directory call "this_is_a_directory" and a file called "hello" which contains words "hello world"

 */
//http://www.mathcs.emory.edu/~cheung/Courses/561/Syllabus/3-C/bin-files.html

#define LAST_BLOCK         -2
#define NO_FAT_ALLOC       -1
#define FREE_BLOCK          0
#define MAX_FAT_POINTERS    256.0
#define BLOCK_SIZE          1024

#define SUPER_BLOCK_OFFSET  0
#define FAT_BLOCK_OFFSET    1
#define WORD_SIZE           4    // 4 bytes or 32 bits
#define DIR_ENTRY_SIZE      64   // 64 bytes
#define FILE_NAME_SIZE      24   // 24 bytes

#define IS_FILE             0
#define IS_DIR              1

int main(int argc, char* argv[]){
   // argv[0] = progname
   // argv[1] = name of disk image
   // argv[2] = number of disk blocks
   int    fd, blockNum, word, k;
   double k_temp;
   char  *diskImageName;
   time_t currenttime;
   
   // ================================================================== 
   // Initializing Setup
   // ================================================================== 
   if(argc != 3){
      // Check for two arguments: 
      // [disk image name] & [number of disk blocks]
      fprintf(stderr, "USAGE: diskinit [Disk Image Name] [Number of Disk Blocks]\n");
      return EXIT_FAILURE;
   } 
   else{
      // check if second argument was a number
      int length = strlen(argv[2]);
      for(int i=0; i<length; i++){
         if(!isdigit(argv[2][i])) {
            fprintf(stderr, "USAGE: diskinit [Disk Image Name] [Number of Disk Blocks]\n");
            return EXIT_FAILURE;
         }
      }

      // Store and display values recieved
      diskImageName = argv[1];
      blockNum      = atoi(argv[2]); // equivalent to N
      
      // Determine number of FAT blocks needed
      // Round up if N is not a perfect multiple of 256
      k_temp        = blockNum/MAX_FAT_POINTERS;
      k             = (int) blockNum/MAX_FAT_POINTERS;
      if(k_temp > k){ k = k+1;}
      
      printf("Name of Disk Image:    %s\n", diskImageName);
      printf("Number of Disk Blocks: %d\n", blockNum);
      printf("Number of FAT Blocks:  %d\n", k);
   }
  
   // Create and Initialize Disk
   // Initialize entire disk to 0
   fd=open(diskImageName, O_CREAT|O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO);
   lseek(fd, 0, SEEK_SET);
   word = FREE_BLOCK;

   for(int i=0; i<(blockNum*BLOCK_SIZE); i++){
      write(fd, &word, 1);
   }


   // ================================================================== 
   // Super Block Setup
   // ================================================================== 
   
   // Super Block
   // Word: 0     Magic number: 0x2345beef
   // Word: 1     N: total number of blocks in the file system
   // Word: 2     k: number of blocks in the file allocation table
   // Word: 3     Block size (1024 for your file system)
   // Word: 4     Starting block of the root directory

   lseek(fd, SUPER_BLOCK_OFFSET*BLOCK_SIZE, SEEK_SET);

   //set Magic number: 0x2345beef
   word=0x2345beef;
   if(write(fd, &word, WORD_SIZE)==-1) {perror("Super Block setup: Magic Number");}

   //set total number of blocks(1024) in the file system
   word=blockNum;
   if(write(fd, &word, WORD_SIZE)==-1) {perror("Super Block setup: N");}

   //set number of blocks in the file allocation table
   word=k;
   if(write(fd, &word, WORD_SIZE)==-1) {perror("Super Block setup: k");}

   //set Block size (1024 for your file system)
   word=BLOCK_SIZE;
   if(write(fd, &word, WORD_SIZE)==-1) {perror("Super Block setup: Block Size");}

   //set starting block of the root directory
   word=k+1;
   if(write(fd, &word, WORD_SIZE)==-1) {perror("Super Block setup: Start Block");}


   // ================================================================== 
   // FAT Block Setup
   // ================================================================== 

   // The first k+1 FAT entries (0 - k) are unused, 
   // since they point to disk blocks that can't be included in files.
   
   // set first k+1 entries of FAT to -1 (NO_FAT_ALLOC)
   lseek(fd, FAT_BLOCK_OFFSET*BLOCK_SIZE, SEEK_SET);
   word=NO_FAT_ALLOC;
   for(int i=0; i<k+1; i++){
      if(write(fd, &word, WORD_SIZE)==-1) {perror("FAT Block setup");}
   }

   // also, set extraneous FAT block entries to -1 as well
   lseek(fd, (FAT_BLOCK_OFFSET*BLOCK_SIZE) + (WORD_SIZE*blockNum), SEEK_SET);
   for(int i=blockNum; i<(k*MAX_FAT_POINTERS); i++){
      if(write(fd, &word, WORD_SIZE)==-1) {perror("FAT Block setup");}
   }

   // ================================================================== 
   // Create A Directory Entry: this_is_a_directory
   // ==================================================================   
   
   // Directory Format
   // Word: 0-5      File name (null-terminated string, up to 24 bytes long including the null)
   // Word: 6-7      Creation time (64-bit integer)
   // Word: 8-9      Modification time (64-bit integer)
   // Word: 10-11    Access time (64-bit integer)
   // Word: 12       File length in bytes
   // Word: 13       Start block
   // Word: 14       Flags
   // Word: 15       Unused
   lseek(fd, (k+1)*BLOCK_SIZE, SEEK_SET);

   //set file name
   if(write(fd, "this_is_a_directory\0", FILE_NAME_SIZE)==-1) {}

   // set creation time
   currenttime= time(NULL);
   if(write(fd, &currenttime, 2*WORD_SIZE)==-1) {}

   // set modification time
   if(write(fd, &currenttime, 2*WORD_SIZE)==-1) {}

   // set access time
   if(write(fd, &currenttime, 2*WORD_SIZE)==-1) {}  

   //set file length in bytes
   word=DIR_ENTRY_SIZE;
   if(write(fd, &word, WORD_SIZE)==-1) {}

   //set start block
   word=k+2;
   if(write(fd, &word, WORD_SIZE)==-1) {}

   //set flag
   word=IS_DIR;
   if(write(fd, &word, WORD_SIZE)==-1) {}

   // ================================================================== 
   // Create A Directory Entry: ..
   // ==================================================================  

   lseek(fd, (k+2)*BLOCK_SIZE, SEEK_SET);

   //set file name
   if(write(fd, "..\0", FILE_NAME_SIZE)==-1) {}

   // set creation time
   currenttime= time(NULL);
   if(write(fd, &currenttime, 2*WORD_SIZE)==-1) {}

   // set modification time
   if(write(fd, &currenttime, 2*WORD_SIZE)==-1) {}

   // set access time
   if(write(fd, &currenttime, 2*WORD_SIZE)==-1) {}  

   //set file length in bytes
   word=DIR_ENTRY_SIZE;
   if(write(fd, &word, WORD_SIZE)==-1) {}

   //set start block
   word=k+1; // k+2?
   if(write(fd, &word, WORD_SIZE)==-1) {}

   //set flag
   word=IS_DIR;
   if(write(fd, &word, WORD_SIZE)==-1) {}

   // ================================================================== 
   // Create A Directory Entry: hello
   // ==================================================================

   lseek(fd, (k+3)*BLOCK_SIZE, SEEK_SET);

   //set file name
   if(write(fd, "hello\0", FILE_NAME_SIZE)==-1) {}

   // set creation time
   currenttime= time(NULL);
   if(write(fd, &currenttime, 2*WORD_SIZE)==-1) {}

   // set modification time
   if(write(fd, &currenttime, 2*WORD_SIZE)==-1) {}

   // set access time
   if(write(fd, &currenttime, 2*WORD_SIZE)==-1) {}  

   //set file length in bytes
   word=strlen("Hello World\n");
   if(write(fd, &word, WORD_SIZE)==-1) {}

   //set start block
   word=k+4; 
   if(write(fd, &word, WORD_SIZE)==-1) {}

   //set flag
   word=IS_FILE;
   if(write(fd, &word, WORD_SIZE)==-1) {}

   //create data for file "hello"-----------------------
   lseek(fd, (k+4)*BLOCK_SIZE, SEEK_SET);
   if(write(fd, "Hello World\n", BLOCK_SIZE)==-1) {}

   // ================================================================== 
   // Set up FAT entries for Data Blocks
   // ==================================================================
   
   // -- FAT Block Entries --
   // In first block (entries 0-255):
   //          0:   -2 (Super Block) 
   //        1-k:   -2 (FAT Blocks)
   // (k+1) & up:   -1 or 0

   // ==== In Our Example ==============================================
   // N = 1024,  k = N/256 = 4
   //
   // -- Data Blocks --
   // k+1 (5):  this_is_a_directory (directory header)
   // k+2 (6):  ..
   // k+3 (7):  hello (file header)
   // k+4 (8):  hello (file data)

   // -- FAT Block Entries --
   // In first block (entries 0-255):
   //   0:   -1 (Super Block) 
   // 1-4:   -1 (FAT Blocks)

   //   5:    7 (k+1) this_is_a_directory
   //   6:   -2 (k+2) ..
   //   7:   -2 (k+3) hello
   //   8:   -2 (k+4) hello (file data)

   // Go to k+1 FAT Block entry
   lseek(fd, BLOCK_SIZE + (WORD_SIZE*(k+1)), SEEK_SET);
   
   word=k+3;
   if(write(fd, &word, WORD_SIZE)==-1){} // k+1  <-  7 
   word=LAST_BLOCK; // LAST_BLOCK = -2 
   if(write(fd, &word, WORD_SIZE)==-1){} // k+2  <- -2
   if(write(fd, &word, WORD_SIZE)==-1){} // k+3  <- -2
   if(write(fd, &word, WORD_SIZE)==-1){} // k+4  <- -2

  close(fd);
  return EXIT_SUCCESS;
}
