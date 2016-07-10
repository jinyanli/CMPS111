#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#include <time.h>
#include <sys/file.h> 
#include <sys/types.h>
#include <math.h>

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

int fd;
int32_t *fat;
int magic_num=0;
int total_blocks=0;
int k=0;
int block_size=0;
int rootBlockNum=0;

static int fat_getattr(const char *path, struct stat *stbuf)
{
	int i, res = 0;
    char *tok;
	char filename[24];
	int startingBlock=0, fileLength, flags, nextBlock, thisblock, found=0;
	time_t creationTime, modificationTime, accessTime;
	thisblock=rootBlockNum;
	lseek(fd, thisblock*1024, SEEK_SET);
	//reference: https://stackoverflow.com/questions/2385697/strtok-giving-segmentation-fault
	char* copy = strdup(path);
	
	memset(stbuf, 0, sizeof(struct stat));
	if (strcmp(path, "/") == 0) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	} else{
		    tok = strtok(copy , "/");
		
			while( 1) {
				while(1){
					lseek(fd, thisblock*1024, SEEK_SET);
					read(fd, &filename, 24);
					found=0;
					if (strcmp(filename, tok) == 0){
							found=1;
							read(fd, &creationTime, 8);
							read(fd, &modificationTime, 8) ;
							read(fd, &accessTime, 8);
							read(fd, &fileLength, 4);		
							read(fd, &startingBlock, 4);
							read(fd, &flags, 4);
							break; 
					}				
					nextBlock=fat[thisblock];
					if(nextBlock==-2){
						break;
					}
					thisblock=nextBlock;
				}

				if(found!=1){
					res = -ENOENT;
					break;
				}
				tok = strtok(NULL, "/");
				if(tok == NULL ) break;
				thisblock=startingBlock;
			}		
			
			if(flags==1){
			   stbuf->st_mode = S_IFDIR | 0755;
			   stbuf->st_nlink = 1;
			   stbuf->st_size = fileLength;
			}else if(flags==0){  
				   stbuf->st_mode = S_IFREG | 0444;
				   stbuf->st_nlink = 1; 
				   stbuf->st_size = fileLength;
			}else{
				res = -ENOENT;
			}
	         
			 stbuf->st_atim.tv_sec=accessTime;
			stbuf->st_mtim.tv_sec=modificationTime;
			stbuf->st_birthtim.tv_sec=creationTime;
	}
	
	
	return res;
	
}

static int fat_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		       off_t offset, struct fuse_file_info *fi)
{
	
    (void) offset;
	(void) fi;
	
    char filename[24];
	char* tok;

	
	int startingBlock=0, fileLength, flags, nextBlock, thisblock, found=0;
	time_t creationTime, modificationTime, accessTime;
	
	int placeholder=0;
	
	char* copy = strdup(path);

    thisblock=rootBlockNum;
        
    if (strcmp(path, "/") == 0) {
		//print all file in the root directory
		while(1){
			lseek(fd, thisblock*1024, SEEK_SET);
			read(fd, &filename, 24);
			filler(buf, filename, NULL, 0);

			if(tok!=NULL){
				if (strcmp(filename, tok) == 0){
					  lseek(fd, thisblock*1024+4*13, SEEK_SET);
					  read(fd, &startingBlock, 4);
				}	
			}
			nextBlock=fat[thisblock];
			if(nextBlock==-2){
				break;
			}
			thisblock=nextBlock;
		}
		
	}else{
		//get the starting block of the target directory 
		tok = strtok(copy , "/");	
			while( 1) {
				while(1){
					lseek(fd, thisblock*1024, SEEK_SET);
					read(fd, &filename, 24);
					found=0;
					if (strcmp(filename, tok) == 0){
							found=1;
							read(fd, &creationTime, 8);
							read(fd, &modificationTime, 8) ;
							read(fd, &accessTime, 8);
							read(fd, &fileLength, 4);		
							read(fd, &startingBlock, 4);
							read(fd, &flags, 4);
							break; 
					}				
					nextBlock=fat[thisblock];
					if(nextBlock==-2){
						break;
					}
					thisblock=nextBlock;
				}

				if(found!=1){
					return -ENOENT;
					break;
				}
				tok = strtok(NULL, "/");
				if(tok == NULL ) break;
				thisblock=startingBlock;
			}		
			
			//print all files from the target directory
			thisblock=startingBlock;
			while(1){
				lseek(fd, thisblock*1024, SEEK_SET);
				read(fd, &filename, 24);
				filler(buf, filename, NULL, 0);
				if(tok!=NULL){
					if (strcmp(filename, tok) == 0){
						  lseek(fd, thisblock*1024+4*13, SEEK_SET);
						  read(fd, &startingBlock, 4);
					}	
				}
				nextBlock=fat[thisblock];
				if(nextBlock==-2){
					break;
				}
				thisblock=nextBlock;
			}
	}

	return 0;
}

static int fat_read(const char *path, char *buf, size_t size, off_t offset,
		      struct fuse_file_info *fi)
{
	
	size_t fileLength;
	(void) fi;
    printf("fat_read fh:%ld: \n", fi->fh);
	int i, res = 0;
    char *tok;
	char filename[24];
	int startingBlock=0, flags, nextBlock, thisblock, found=0;
	time_t creationTime, modificationTime, accessTime;
	thisblock=rootBlockNum;
	lseek(fd, thisblock*1024, SEEK_SET);
	char* copy = strdup(path);
	
	//get the starting block and the length of the target file from its directory entry
	if (strcmp(path, "/") == 0) {
		return -ENOENT;
	} else{
		    tok = strtok(copy , "/");
		
			while( 1) {
				while(1){
					lseek(fd, thisblock*1024, SEEK_SET);
					read(fd, &filename, 24);
					found=0;
					if (strcmp(filename, tok) == 0){
							found=1;
							read(fd, &creationTime, 8);
							read(fd, &modificationTime, 8) ;
							time_t currenttime;
							currenttime= time(NULL);
							//set access time
							write(fd, &currenttime, 8);
							//read(fd, &accessTime, 8);
							read(fd, &fileLength, 4);		
							read(fd, &startingBlock, 4);
							read(fd, &flags, 4);
							break; 
					}				
					nextBlock=fat[thisblock];
					if(nextBlock==-2){
						break;
					}
					thisblock=nextBlock;
				}

				if(found!=1){
					res = -ENOENT;
					break;
				}
				tok = strtok(NULL, "/");
				if(tok == NULL ) break;
				thisblock=startingBlock;
			}		
			
	}
	
	//create buffer for reading
	int bytes=fileLength;
    int blocks;
	int count=0;
    float d=(float)bytes;
    blocks=(int)ceil(d/1024.0);
	char buffer[blocks*1024];
	
	//read data from file blocks
	thisblock=startingBlock;
	while(1){
		lseek(fd, thisblock*1024, SEEK_SET);
		read(fd, &buffer+count*1024, 1024);		
        	
		nextBlock=fat[thisblock];
		if(nextBlock==-2){
			break;
		}
		thisblock=nextBlock;
		count=count+1;
	}
	printf("buffer: %s\n",buffer);
    
	//put data into buffer
	//reference https://github.com/libfuse/libfuse/blob/master/example/hello.c
	if (offset < fileLength) {
		if (offset + size >  fileLength)
			size =  fileLength - offset;
		memcpy(buf, buffer+offset, fileLength);
	} else
		size = 0;
	printf("buf: %s\n",buf);
	return size;
}

static int fat_open(const char *path, struct fuse_file_info *fi)
{
	printf("fat_open\n");
	fi->fh=-1;
	size_t fileLength;
	int i, res = 0;
    char *tok;
	char filename[24];
	int startingBlock=0, flags, nextBlock, thisblock, found=0;
	time_t creationTime, modificationTime, accessTime;
	thisblock=rootBlockNum;
	lseek(fd, thisblock*1024, SEEK_SET);
	char* copy = strdup(path);
	
	//get the starting block and its length of the target file from its directory entry
	if (strcmp(path, "/") == 0) {
		return -ENOENT;
	} else{
		    tok = strtok(copy , "/");	
			while( 1) {
				while(1){
					lseek(fd, thisblock*1024, SEEK_SET);
					read(fd, &filename, 24);
					found=0;
					if (strcmp(filename, tok) == 0){
							found=1;
							read(fd, &creationTime, 8);
							read(fd, &modificationTime, 8) ;
							read(fd, &accessTime, 8);
							read(fd, &fileLength, 4);		
							read(fd, &startingBlock, 4);
							read(fd, &flags, 4);
							break; 
					}				
					nextBlock=fat[thisblock];
					if(nextBlock==-2){
						break;
					}
					thisblock=nextBlock;
				}

				if(found!=1){
					//return -ENOENT;
					break;
				}
				tok = strtok(NULL, "/");
				if(tok == NULL ) break;
				thisblock=startingBlock;
			}		
			
	}
	
	if(found==1)
		fi->fh=startingBlock;
	else{
		//---------------create a file---------------------------------------
			
			//------create a directory entry for the file---------
			printf("fat_open's new file's name: %s\n", tok);
			
			char namebuffer[24];
			int x;
			strcpy(namebuffer, tok);

			//get an empty block for file's directory entry
			int entry_idx;
			for(entry_idx=0; fat[entry_idx]!=0; entry_idx++)
			;
			printf("fat_open's dir_idx: %d\n", entry_idx);
			fat[thisblock]=entry_idx;
			fat[entry_idx]=-2;

			//get an empty block for the starting block 
			int file_starting_block;
			for(file_starting_block=0; fat[file_starting_block]!=0; file_starting_block++)
			;
			fat[file_starting_block]=-2;
			
			printf("fat_open's datablock_idxx: %d\n", file_starting_block);
			
			//-------------set directory entry attributes------------------------
			lseek(fd, entry_idx*1024, SEEK_SET);
		
			//set file name
			if(write(fd, namebuffer, 6*4)==-1){
			}
			
			// set creation time
			time_t currenttime;
			currenttime= time(NULL);
			if(write(fd, &currenttime, 2*4)==-1){
			}
			// set modification time
			if(write(fd, &currenttime, 2*4)==-1){
			}
			// set access time
			if(write(fd, &currenttime, 2*4)==-1){
			}	
			//set file length in bytes
			x=16*4;
			if(write(fd, &x, 4)==-1){		
			}				
			//set start block
			x=file_starting_block;
			if(write(fd, &x, 4)==-1){		
			}
			//set flag
			x=0;
			if(write(fd, &x, 4)==-1){		
			}
			
			fi->fh=file_starting_block;
			
			//write FAT back to disk
			lseek(fd, 1024, SEEK_SET);
			for(i=0; i<k*256; i++){
				write(fd, &fat[i], 4);
				if(fat[i]!=0){
					printf("FAT index: i %d, value: %d\n", i, fat[i]);
				}
			}
	}
	printf("fat_open fh:%ld: \n", fi->fh);
	
	return 0;
}


static int fat_mkdir(const char *path, mode_t mode)
{
	int res, i, x;
	char filename[24];
	char* tok;
	int startingBlock=0, fileLength, flags, nextBlock, thisblock, parentblock, found=0;
	time_t creationTime, modificationTime, accessTime;
	int placeholder=0;
	char* copy = strdup(path);
    printf("fat_mkdir's path: %s\n",path);
    thisblock=rootBlockNum;
	
	 if (strcmp(path, "/") == 0) {
		return -errno;
	}else{
		//get the starting block of the target directory 
		tok = strtok(copy , "/");	
			while( 1) {
				parentblock=thisblock;
				printf("fat_mkdir's parent block: %d\n", parentblock);
				while(1){
					lseek(fd, thisblock*1024, SEEK_SET);
					read(fd, &filename, 24);
					found=0;
					if (strcmp(filename, tok) == 0){
							found=1;
							read(fd, &creationTime, 8);
							read(fd, &modificationTime, 8) ;
							read(fd, &accessTime, 8);
							read(fd, &fileLength, 4);		
							read(fd, &startingBlock, 4);
							read(fd, &flags, 4);
							break; 
					}				
					nextBlock=fat[thisblock];
					if(nextBlock==-2){
						break;
					}
					thisblock=nextBlock;
				}

				if(found!=1){
					//------create the directory---------
					printf("fat_mkdir's tok: %s\n", tok);
					
					char namebuffer[24];
					strcpy(namebuffer, tok);
					//get an empty block for its directory
					int dir_idx;
					for(dir_idx=0; fat[dir_idx]!=0; dir_idx++)
					;
					printf("fat_mkdir's dir_idx: %d\n", dir_idx);
					fat[thisblock]=dir_idx;
					fat[dir_idx]=-2;

					//get an empty block for its .. (parent) directory entry
					int parent_dir_idx;
					for(parent_dir_idx=0; fat[parent_dir_idx]!=0; parent_dir_idx++)
					;
					fat[parent_dir_idx]=-2;
					
				    printf("fat_mkdir's parent_dir_idx: %d\n", parent_dir_idx);
					
					//-------------set new directory attributes------------------------
					lseek(fd, dir_idx*1024, SEEK_SET);
                
					//set file name
					if(write(fd, namebuffer, 6*4)==-1){
					}
					
					// set creation time
					time_t currenttime;
					currenttime= time(NULL);
					if(write(fd, &currenttime, 2*4)==-1){
					}
					// set modification time
					if(write(fd, &currenttime, 2*4)==-1){
					}
					// set access time
					if(write(fd, &currenttime, 2*4)==-1){
					}	
					//set file length in bytes
					x=16*4;
					if(write(fd, &x, 4)==-1){		
					}				
					//set start block
					x=parent_dir_idx;
					if(write(fd, &x, 4)==-1){		
					}
					//set flag
					x=1;
					if(write(fd, &x, 4)==-1){		
					}
					
					//-------------set new directory's parent directory entry attributes------------------------
					lseek(fd, parent_dir_idx*1024, SEEK_SET);
                
					//set file name
					if(write(fd, "..\0", 6*4)==-1){
					}
					
					// set creation time
					currenttime= time(NULL);
					if(write(fd, &currenttime, 2*4)==-1){
					}
					// set modification time
					if(write(fd, &currenttime, 2*4)==-1){
					}
					// set access time
					if(write(fd, &currenttime, 2*4)==-1){
					}	
					//set file length in bytes
					x=16*4;
					if(write(fd, &x, 4)==-1){		
					}				
					//set starting block
					x=parentblock;
					if(write(fd, &x, 4)==-1){		
					}
					//set flag
					x=1;
					if(write(fd, &x, 4)==-1){		
					}
					
					//write FAT back to disk
					lseek(fd, 1024, SEEK_SET);
					for(i=0; i<k*256; i++){
						write(fd, &fat[i], 4);
						if(fat[i]!=0){
							printf("FAT index: i %d, value: %d\n", i, fat[i]);
						}
					}
					
					break;
				}
				tok = strtok(NULL, "/");
				if(tok == NULL ) break;
				thisblock=startingBlock;
			}		

	}
	
	return 0;
}

static int fat_rmdir(const char *path)
{
	int res, i, x;
	char filename[24];
	char* tok;
	int startingBlock=0, fileLength, flags, nextBlock, thisblock, parentblock, previousblock, found=0;
	time_t creationTime, modificationTime, accessTime;
	int placeholder=0;
	char* copy = strdup(path);
    printf("fat_rmdir's path: %s\n",path);
    thisblock=rootBlockNum;
	
    if (strcmp(path, "/") == 0) {
		return -ENOENT;
	} else{
		    tok = strtok(copy , "/");	
			//get the starting block of the target directory from its directory entry
			while( 1) {
				while(1){
					lseek(fd, thisblock*1024, SEEK_SET);
					read(fd, &filename, 24);
					found=0;
					if (strcmp(filename, tok) == 0){
						    printf("rmdir's filename: %s\n", filename);
							found=1;
							lseek(fd, thisblock*1024+4*13, SEEK_SET);
							read(fd, &startingBlock, 4);
							break;
					}
					
                    previousblock=thisblock;					
					nextBlock=fat[thisblock];
					if(nextBlock==-2){
						break;
					}
					thisblock=nextBlock;
				}

				if(found!=1){
					return -ENOENT;
					break;
				}
				tok = strtok(NULL, "/");
				if(tok == NULL ) break;
				thisblock=startingBlock;
			}				
	}
	
				int currentblock=thisblock;
				//check if the target directory is empty
				thisblock=startingBlock;
				int isempty=0;
				while(1){
						lseek(fd, thisblock*1024, SEEK_SET);
						read(fd, &filename, 24);
						found=0;
						
						if (strcmp(filename, "..\0") != 0){
								isempty=1;
								return -ENOTEMPTY; 
						}
						
						nextBlock=fat[thisblock];
						
						if(nextBlock==-2){
							break;
						}
						
						thisblock=nextBlock;
				}

				//delete the target directory if it's empty
				if(isempty==0){
					//delete .. entry
					fat[startingBlock]=0;
					
					thisblock=currentblock;
					if((nextBlock=fat[thisblock])==-2){
						fat[previousblock]=-2;
						fat[thisblock]=0;
					}else{
						nextBlock=fat[thisblock];
						fat[previousblock]=nextBlock;
						fat[thisblock]=0;
					}
				}else
					return -ENOTEMPTY; 						
				
				//write FAT back to disk
				lseek(fd, 1024, SEEK_SET);
				for(i=0; i<k*256; i++){
					write(fd, &fat[i], 4);
					if(fat[i]!=0){
						printf("FAT index: i %d, value: %d\n", i, fat[i]);
					}
				}
	
	return  0;
}


static int fat_write(const char *path, const char *buf, size_t size,
		     off_t offset, struct fuse_file_info *fi)
{
	int res, i;
	printf("fat_write()\n");
	//get the starting block of the file
	int file_starting_block=fi->fh;
	(void) fi;
	if (file_starting_block== -1)
		return -errno;
	
	//calculate the size in blocks
	int num_of_blocks;
    float d=(float)size;
    num_of_blocks=(int)ceil(d/1024.0);
	
	if(num_of_blocks==1){
		lseek(fd, file_starting_block*1024, SEEK_SET);
			res = pwrite(fd, buf, size, offset);
			if (res == -1)
				res = -errno;
	}else{
			//allocate free blocks for writing
			return -errno;
			int block_idx;
			for(i=0; i< num_of_blocks; i++){
				for(block_idx=0; fat[block_idx]!=0; block_idx++){
					
					
				}
							
			}
	}


	return res;
}

static int fat_rename (const char *path, const char *newname) {
	int res, i, x;
	char filename[24];
	char* tok;
	int startingBlock=0, nextBlock, thisblock, found=0;

	char* copy = strdup(path);
	char* newnamecopy=strdup(newname);;
	char* name=strtok(newnamecopy , "/");
    printf("fat_rename's path: %s\n", path);
	printf("fat_rename's newname: %s\n", name);
    thisblock=rootBlockNum;
	
    if (strcmp(path, "/") == 0) {
		return -ENOENT;
	} else{
		    tok = strtok(copy , "/");	
			//get the starting block of the target directory from its directory entry
			while( 1) {
				while(1){
					lseek(fd, thisblock*1024, SEEK_SET);
					read(fd, &filename, 24);
					found=0;
					if (strcmp(filename, tok) == 0){
						    printf("fat_rename's filename: %s\n", filename);
							found=1;
							lseek(fd, thisblock*1024+4*13, SEEK_SET);
							read(fd, &startingBlock, 4);
							break;
					}					
					nextBlock=fat[thisblock];
					if(nextBlock==-2){
						break;
					}
					thisblock=nextBlock;
				}

				if(found!=1){
					return -ENOENT;
					break;
				}
				tok = strtok(NULL, "/");
				if(tok == NULL ) break;
				thisblock=startingBlock;
			}				
	}
	
	char namebuffer[24];
	strcpy(namebuffer, name);
	lseek(fd, thisblock*1024, SEEK_SET);
	write(fd, namebuffer, 6*4);
	return 0;
}


static int fat_unlink(const char *path)
{
	int res, i, x;
	char filename[24];
	char* tok;
	int startingBlock=0, fileLength, flags, nextBlock, thisblock, parentblock, previousblock, found=0;
	char* copy = strdup(path);
    printf("fat_unlink's path: %s\n",path);
    thisblock=rootBlockNum;
	
	if (strcmp(path, "/") == 0) {
		return -ENOENT;
	} else{
		    tok = strtok(copy , "/");	
			//get the block position of the target file's directory entry and the starting block of the file
			while( 1) {
				while(1){
					lseek(fd, thisblock*1024, SEEK_SET);
					read(fd, &filename, 24);
					found=0;
					if (strcmp(filename, tok) == 0){
						    printf("rmdir's filename: %s\n", filename);
							found=1;
							lseek(fd, thisblock*1024+4*13, SEEK_SET);
							read(fd, &startingBlock, 4);
							break;
					}
					
                    previousblock=thisblock;					
					nextBlock=fat[thisblock];
					if(nextBlock==-2){
						break;
					}
					thisblock=nextBlock;
				}

				if(found!=1){
					return -ENOENT;
					break;
				}
				tok = strtok(NULL, "/");
				if(tok == NULL ) break;
				thisblock=startingBlock;
			}				
	}
	
	
	    //delete the target file directory and it's data
		fat[startingBlock]=0;			
		if((nextBlock=fat[thisblock])==-2){
			fat[previousblock]=-2;
			fat[thisblock]=0;
		}else{
			nextBlock=fat[thisblock];
			fat[previousblock]=nextBlock;
			fat[thisblock]=0;
		}
					
		
		//write FAT back to disk
		lseek(fd, 1024, SEEK_SET);
		for(i=0; i<k*256; i++){
			write(fd, &fat[i], 4);
			if(fat[i]!=0){
				printf("FAT index: i %d, value: %d\n", i, fat[i]);
			}
		}

	return 0;
}

static struct fuse_operations fat_oper = {
	.getattr	= fat_getattr,
	.readdir	= fat_readdir,
	.read	= fat_read,
	.open    = fat_open,
	.mkdir   = fat_mkdir,
	.rmdir	= fat_rmdir,
	.write 	=	fat_write,
	.rename = fat_rename,
	.unlink	= fat_unlink,
};

int main(int argc, char *argv[])
{
	
	fd=open("diskImage", O_CREAT|O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO);
     int i;
	
	//load superblock
	lseek(fd, SUPER_BLOCK_OFFSET, SEEK_SET);
    read(fd,&magic_num, WORD_SIZE);
	printf("magic number: %08x\n", magic_num);
	read(fd,&total_blocks, WORD_SIZE);
	printf("total blocks: %d\n", total_blocks);
	read(fd,&k, WORD_SIZE);
	printf("k: %d\n", k);
	read(fd,&block_size, WORD_SIZE);
	printf("block size: %d\n", block_size);
	read(fd, &rootBlockNum, WORD_SIZE);
	printf("root block number: %d\n", rootBlockNum);
	 
	 //load FAT into memory
	lseek(fd, 1024, SEEK_SET);
	fat=realloc(fat, (k*256)*sizeof( int));
	for(i=0; i<k*256; i++){
		read(fd, &fat[i], 4);
		//printf("%d\n", fat[i]);
	}
	
	return fuse_main(argc, argv, & fat_oper, NULL);
}