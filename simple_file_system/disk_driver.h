#pragma once
#include "bitmap.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define BLOCK_SIZE 512
#define PAGE_SIZE 4096

typedef struct {
  BitMap* disk_map; // mmapped bitmap
  
  
  char* block_map; // mmapped block page (1 page = 4096 byte = 8 blocks).
  int first_mapped_block; //index of the first block on the block_map.
  
  int fd; // for us
  int free_blocks;     // free blocks
  int first_block_offset; // offset for reading first block
} DiskDriver;

/**
   The blocks indices seen by the read/write functions 
   have to be calculated after the space occupied by the bitmap
*/

// opens the file (creating it if necessary_
// allocates the necessary space on the disk
// calculates how big the bitmap should be
// if the file was new
// compiles a disk header, and fills in the bitmap of appropriate size
// with all 0 (to denote the free space);
int DiskDriver_init(DiskDriver* disk, const char* filename, int num_blocks);

// reads the block in position block_num
// returns -1 if the block is free according to the bitmap
// 0 otherwise
int DiskDriver_readBlock(DiskDriver* disk, void* dest, int block_num);

// writes a block in position block_num, and alters the bitmap accordingly
// returns -1 if operation not possible
int DiskDriver_writeBlock(DiskDriver* disk, void* src, int block_num);

// frees a block in position block_num, and alters the bitmap accordingly
// returns -1 if operation not possible
int DiskDriver_freeBlock(DiskDriver* disk, int block_num);

// returns the first free block in the disk from position (checking the bitmap)
int DiskDriver_getFreeBlock(DiskDriver* disk, int start);

// writes the data (flushing the mmap)
int DiskDriver_flush(DiskDriver* disk);



/**Auxiliary funcions!**/
//Shuts down the fs
void DiskDriver_close(DiskDriver* disk);

//Recovers DiskDriver struct, bitmap and disk mapping from a non-new disk
int DiskDriver_resume(DiskDriver* disk);

//Upon request, this maps a page containing the block needed and retuns a pointer to it.
char* DiskDriver_getBlock(DiskDriver* disk, int block_index);



/** Implementation of the functions **/
int DiskDriver_init(DiskDriver* disk, const char* filename, int num_blocks){
	
	int res = open(filename, O_CREAT | O_EXCL | O_RDWR);
	if (res == -1){
		res = open(filename, O_RDWR);
		if(res == -1){
				printf("Error opening file! \n");
				return -1;
		}
		else {
			disk->fd = res;
			DiskDriver_resume(disk); //In this case, a working disk was already created
			return 0; //Don't initialize a good disk!
		}
	}
		
	/**The code block I commented below did exactly the opposite of what is asked for:
	 * instead of taking a file and truncating it of the right dimension, according to
	 * num_blocks, it took a file of a GIVEN dimension and subdivided it with the max
	 * number of blocks it could contain.
	 * The bitmap was stored in eventual spare space in this subdivision.
	 * If that spare space wasn't large enough, it reduced the number of fs blocks by 1 
	 * an retried until the spare space was enough to fit the whole bitmap. (untested, but cool!)
	 * **/	
	/*
	if(res!=-1){
		if(ftruncate(res, DISK_SIZE) == -1){  
			printf("Error truncating file /n");
			exit(-1);
		}
		int is_ok = 0; //flag used to control while-cycles to find the right amount of blocks in the disk, according with bitmap dimension.
		int n_blocks = DISK_SIZE/BLOCK_SIZE;
		int bitmap_remainder = DISK_SIZE - n_blocks*BLOCK_SIZE; //I want to use this spare space (in bytes) for writing down the bitmap
		
		//I'm assuring that there is enough spare space aside of the blocked fs to write down the bitmap
		while(is_ok == 0){
			if (bitmap_remainder < (sizeof(int) + n_blocks*sizeof(char))){ //bitmap is formed by an int + n_blocks chars
				n_blocks--;
				bitmap_remainder = DISK_SIZE - n_blocks*BLOCK_SIZE;
			}
			else is_ok = 1;
		}
	}
	*/
	
	int i;
	BitMap disk_bitmap;
	disk_bitmap.num_bitmap_cells = num_blocks;
	
	for(i=0;i<num_blocks;i++)
		disk_bitmap.bitmap[i] = 0;
	int bitmap_size = sizeof(int) + num_blocks*sizeof(char);
	
	for(i=0;i<(bitmap_size%PAGE_SIZE);i++) //I'm adjusting bitmap size due to mmap limitations
		disk_bitmap.padding[i] = 0xFF;
		
	bitmap_size = sizeof(disk_bitmap);
	int disk_size = bitmap_size + num_blocks*BLOCK_SIZE;
	
	//Mapping my disk in process memory
	if(ftruncate(res, disk_size) == -1){  
			printf("Error truncating file /n");
			exit(-1);
	}
	BitMap* disk_map = (BitMap*)mmap(NULL, bitmap_size, PROT_READ|PROT_WRITE, MAP_SHARED, res, 0); //mmapping bitmap area on fd
	
	//Writing down bitmap to mmapped area
	memcpy(disk_map, &disk_bitmap, bitmap_size);	
	
	//Compiling struct
	disk->disk_map = disk_map;
	disk->block_map = NULL; //This will be mapped upon use
	disk->fd = res;
	disk->free_blocks = num_blocks;
	disk->first_block_offset = bitmap_size + 1;

	return 0;
}


int DiskDriver_readBlock(DiskDriver* disk, void* dest, int block_num){
	
	if(block_num<0 || block_num > disk->disk_map->num_bitmap_cells-1) return -2; //Invalid block num
	
	if((int)disk->disk_map->bitmap[block_num]!=0&&(int)disk->disk_map->bitmap[block_num]!=1){ //Something went wrong
		printf("The bitmap is damaged!\n");
		exit(-1);
	}
	
	if((int)disk->disk_map->bitmap[block_num]==0) return -1; //Empty block
	
	if((int)disk->disk_map->bitmap[block_num]==1)  //Copying full block in dest memory
		memcpy(dest, (void*)DiskDriver_getBlock(disk, block_num), BLOCK_SIZE);
	return 0;
	
}


int DiskDriver_writeBlock(DiskDriver* disk, void* src, int block_num){
	
	if(block_num<0 || block_num>disk->disk_map->num_bitmap_cells-1) return -1; //Invalid block num

	if(disk->disk_map->bitmap[block_num]==0) disk->free_blocks -= 1;
	
	if(BitMap_set(disk->disk_map, block_num, 1) != 0){ //Setting bitmap entry to 1
			printf("Error setting the bitmap!\n");
			exit(-1);
	}
	
	//Copying full block in dest memory
	memcpy((void*)DiskDriver_getBlock(disk, block_num), src, BLOCK_SIZE);
	
	return 0;
}


int DiskDriver_freeBlock(DiskDriver* disk, int block_num){
	
	if(block_num<0 || block_num>disk->disk_map->num_bitmap_cells-1) return -1; //Invalid block num
	
	if(disk->disk_map->bitmap[block_num]==1) disk->free_blocks += 1;
	
	if(BitMap_set(disk->disk_map, block_num, 0) != 0){ //Setting bitmap entry to 0
			printf("Error setting the bitmap!/n");
			exit(-1);
	}
	
	return 0;
}


int DiskDriver_getFreeBlock(DiskDriver* disk, int start){
	
	if(start<0 || start>disk->disk_map->num_bitmap_cells-1) return -1; //Invalid start block
	
	int res = BitMap_get(disk->disk_map, start, 0); 
	
	if(res == -1) return -1; //No free blocks available (I know it's redundant, but it's logical)
	else return res;
}

//Deprecated.
int DiskDriver_flush(DiskDriver* disk){
	//disk->disk_map[0] = disk->bmp; //This writes my bitmap down to the disk.
	//Blocks are directly written onto the disk on modification in this implementation (copy on write)
}


void DiskDriver_close(DiskDriver* disk){
	
	//DiskDriver_flush(disk); //Updating bitmap
	
	munmap(disk->block_map, sizeof(*disk->block_map)); //Unmapping blocks
	munmap(disk->disk_map, sizeof(*disk->disk_map)); //Unmapping disk
	close(disk->fd);  //Closing disk file
	exit(0); //Bye!
	
}


int DiskDriver_resume(DiskDriver* disk){
	
	int num_blocks;
	if (read(disk->fd, &num_blocks, sizeof(int))==-1){
		printf("Error retrieving bitmap lenght/n");
		return -1;
	}
	
	int bitmap_size = sizeof(int) + num_blocks*sizeof(char);
	bitmap_size += bitmap_size%PAGE_SIZE;
	int disk_size = bitmap_size + num_blocks*BLOCK_SIZE;
	
	//Mapping my disk in process memory
	if(ftruncate(disk->fd, disk_size) == -1){  
			printf("Error truncating file /n");
			exit(-1);
	}
	
	BitMap* disk_map = (BitMap*)mmap(NULL, bitmap_size, PROT_READ|PROT_WRITE, MAP_SHARED, disk->fd, 0); //mmapping bitmap area on fd
	disk->disk_map = disk_map;
	//disk->disk_map->num_bitmap_cells = num_blocks; //Already got by mmapping
	
	disk->block_map = (char*)NULL;
	disk->first_mapped_block = -1; //Int doesn't support null value in C

	disk->first_block_offset = bitmap_size + 1;
	
	int i;
	disk->free_blocks = 0; 
	for (i=0; i<num_blocks; i++){
		if (disk->disk_map->bitmap[i]==0) disk->free_blocks += 1;
	}
	return 0;
}


char* DiskDriver_getBlock(DiskDriver* disk, int block_index){
	
	if(block_index<0 || block_index > disk->disk_map->num_bitmap_cells-1) return (char*)NULL; //Security check
	
	int mapped_blocks = PAGE_SIZE/BLOCK_SIZE; //This has likely to be == 8
	int offset;
	
	if(disk->first_mapped_block != -1){
		if(block_index >= disk->first_mapped_block && block_index < (disk->first_mapped_block+mapped_blocks)){
			offset = block_index - disk->first_mapped_block; //Calculating relative block index into mmapped block portion
			return &(disk->block_map[offset*BLOCK_SIZE]); //Moving to the first byte of that block and returning it
		}
	}
	
	offset = (block_index/mapped_blocks)*PAGE_SIZE; //it represents the portion of blocks that has to be mmapped to get the block "block_index"
	
	disk->disk_map = (BitMap*)mmap(NULL, BLOCK_SIZE*PAGE_SIZE , PROT_READ|PROT_WRITE, MAP_SHARED, disk->fd, offset);
	disk->first_mapped_block = (block_index/mapped_blocks)*mapped_blocks; //It works because the fraction takes only integer part.
	
	return DiskDriver_getBlock(disk, block_index);
}
