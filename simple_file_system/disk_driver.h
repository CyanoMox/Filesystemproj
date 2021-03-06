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
  char* disk_map; // (mmapped) bitmap
  int num_entries; // number of blocks mapped (= bitmap lenght without padding and int)
  
  char* block_map; // mmapped block page (1 page = 4096 byte = 8 blocks).
  unsigned int first_mapped_block; //index of the first block on the block_map.
  
  int fd; // for us
  unsigned int free_blocks;     // free blocks
  unsigned int first_block_offset; // offset for reading first block
} DiskDriver;

/**
   The blocks indexes seen by the read/write functions 
   have to be calculated after the space occupied by the bitmap
*/

// opens the file (creating it if necessary_
// allocates the necessary space on the disk
// calculates how big the bitmap should be
// if the file was new
// compiles a disk header, and fills in the bitmap of appropriate size
// with all 0 (to denote the free space);
int DiskDriver_init(DiskDriver* disk, const char* filename, 
											unsigned int num_blocks);

// reads the block in position block_num
// returns -1 if the block is free according to the bitmap
// 0 otherwise
int DiskDriver_readBlock(DiskDriver* disk, void* dest, 
												unsigned int block_num);

// writes a block in position block_num, and alters the bitmap accordingly
// returns -1 if operation not possible
int DiskDriver_writeBlock(DiskDriver* disk, void* src, 
												unsigned int block_num);

// frees a block in position block_num, and alters the bitmap accordingly
// returns -1 if operation not possible
int DiskDriver_freeBlock(DiskDriver* disk, unsigned int block_num);

// returns the first free block in the disk from position (checking the bitmap)
int DiskDriver_getFreeBlock(DiskDriver* disk, unsigned int start);



/**Auxiliary funcions!**/
//Shuts down the fs
void DiskDriver_close(DiskDriver* disk);

//Recovers DiskDriver struct, bitmap and disk mapping from a non-new disk
int DiskDriver_resume(DiskDriver* disk, const char* filename);

//Upon request, this maps a page containing the block needed and retuns a pointer to it.
char* DiskDriver_getBlock(DiskDriver* disk, unsigned int block_index);




/** Implementation of the functions **/
int DiskDriver_init(DiskDriver* disk, const char* filename, unsigned int num_blocks){
	
	int res = open(filename, O_CREAT | O_TRUNC| O_RDWR, 0777);
	//I want an already made disk to be opened by DiskDriver_resume() function
	
	int bitmap_size = sizeof(int) + num_blocks*sizeof(char);
	int padding_size; 
	
	if(bitmap_size%PAGE_SIZE!=0) padding_size = 
								PAGE_SIZE-(bitmap_size%PAGE_SIZE);
	/*It solves a bug that allocates a unuseful extra page 
	 * if previous page was exactly full
	 */
	 
	else padding_size = bitmap_size%PAGE_SIZE;
	
	char* bitmap_padding = (char*)malloc(sizeof(char)*padding_size); 
	printf("Number of blocks = %d\nBitmap size = \
%d\nPadding bytes = %d\n",num_blocks,bitmap_size,padding_size);

	bitmap_size += padding_size; 
	printf("Final Bitmap size = %d\n", bitmap_size);
	unsigned int disk_size = bitmap_size + num_blocks*BLOCK_SIZE;
	printf("Final Disk size = %ud\n", disk_size);
	
	//Mapping my disk in process memory
	if(ftruncate(res, disk_size) == -1){  
			printf("Error truncating file /n");
			exit(-1);
	}
	
	char* disk_map = (char*)mmap(NULL, bitmap_size, 
		PROT_READ|PROT_WRITE, MAP_SHARED, res, 0); //mmapping bitmap area on fd
	disk->disk_map = disk_map;
	disk->num_entries = num_blocks;
	
	//Writing down bitmap to mmapped area
	memcpy(disk_map, &num_blocks, sizeof(int)); //Writing down first integer
	
	char* dest = disk_map + sizeof(int);
	int i, j;
	char zero, one;
	zero = 0x00;	
	one = 0xFF;
	
	for(i=0;i<num_blocks;i++){	//Writing down bitmap itself
	 memcpy(dest+i, &zero, sizeof(char));  
	}
	
	for(j=0;j<padding_size;j++){		
	 memcpy(dest+i+j, &one, sizeof(char)); //Writing down padding
	}
	
	//Compiling struct
	disk->disk_map = disk_map;
	disk->block_map = NULL; //This will be mapped upon use
	disk->fd = res;
	disk->free_blocks = num_blocks;
	disk->first_block_offset = bitmap_size;
	disk->first_mapped_block = 0xFFFFFFFF;
	
	//Polishment
	free(bitmap_padding);
	
	return 0;
}


int DiskDriver_readBlock(DiskDriver* disk, void* dest, 
												unsigned int block_num){
	
	if(block_num<0 || block_num > 
					disk->num_entries -1) return -2; //Invalid block num
	char* cursor = disk->disk_map;
	cursor += sizeof(disk->num_entries);
	if((int)cursor[block_num] != 0 
				&& (int)cursor[block_num]!=1){ //Something went wrong
		printf("The bitmap is damaged!\n");
		exit(-1);
	}
	
	if((int)cursor[block_num] == 0) return -1; //Empty block

	if((int)cursor[block_num] == 1){  //Copying full block in dest memory	
		char* src = DiskDriver_getBlock(disk, block_num);
		//printf("DBG src pointer: %p\n", src);
		if(src==NULL){
			printf("Can't read from invalid address\n");
			return -1;
		}
		memcpy(dest, src, BLOCK_SIZE); //TODO: Serious bug here. Seek and destroy.
	}
	return 0;
	
}


int DiskDriver_writeBlock(DiskDriver* disk, void* src, 
												unsigned int block_num){
	
	if(block_num<0 || block_num>disk->num_entries -1) 
										return -1; //Invalid block num
	char* cursor = disk->disk_map;
	cursor += sizeof(disk->num_entries);
	
	if(cursor[block_num]==0) {
		disk->free_blocks -= 1;
		cursor[block_num] = 1;  //Updating Bitmap
	}
	
	//Copying full block in dest memory
	char* res = DiskDriver_getBlock(disk, block_num);
	if(res!=NULL) memcpy(res, src, BLOCK_SIZE); 
	else {

		return -1;
	}
	
	return 0;
}


int DiskDriver_freeBlock(DiskDriver* disk, unsigned int block_num){
	
	if(block_num<0 || block_num>disk->num_entries-1) return -1; //Invalid block num
	char* cursor = disk->disk_map;
	cursor += sizeof(disk->num_entries);
	
	if(cursor[block_num] == 1) {
		disk->free_blocks++;
		cursor[block_num] = 0; 
	}
		
	return 0;
}


int DiskDriver_getFreeBlock(DiskDriver* disk, unsigned int start){
	
	if(start<0 || start>disk->num_entries-1) return -1; //Invalid start block
	char* cursor = disk->disk_map;
	cursor += sizeof(disk->num_entries);
	
	int i = start;
	while(i<disk->num_entries){
		if(cursor[i] == 0) return i;
		i++;
	}
	
	return -1;
}


void DiskDriver_close(DiskDriver* disk){
	
	//Updating bitmap
	munmap(disk->block_map, sizeof(char)*8); //Unmapping blocks, assuming 8 blocks for portion
	
	int bitmap_size = sizeof(int) + disk->num_entries*sizeof(char);
	int padding_size; 
	if(bitmap_size%PAGE_SIZE!=0) padding_size = 
						PAGE_SIZE-(bitmap_size%PAGE_SIZE); 
	/*It solves a bug that allocates a unuseful extra page 
	 *if previous page was exactly full
	 */
	
	else padding_size = bitmap_size%PAGE_SIZE;
	bitmap_size += padding_size; 
	
	munmap(disk->disk_map, bitmap_size); //Unmapping disk
	close(disk->fd);  //Closing disk file
	exit(0); //Bye!
	
}


int DiskDriver_resume(DiskDriver* disk, const char* filename){
	
	int num_blocks;
	int fd;
	
	fd = open(filename, O_RDWR);
	if(fd == -1){
		printf("Error opening file! \n");
		return -1;
		
	}
	disk->fd = fd;
	
	//Retrieving bitmap lenght!
	int* temp = (int*)mmap(NULL, sizeof(int), PROT_READ|PROT_WRITE, 
											MAP_SHARED, disk->fd, 0);
	num_blocks = *temp; 
	munmap(temp, sizeof(int));
	
	disk->num_entries = num_blocks;
		
	int bitmap_size = sizeof(int) + num_blocks*sizeof(char);
	int padding_size; 
	if(bitmap_size%PAGE_SIZE!=0) padding_size = 
					PAGE_SIZE-(bitmap_size%PAGE_SIZE); 
	/*Solves a bug that allocates a unuseful extra page 
	 * if previous page was exactly full
	 */
	 
	else padding_size = bitmap_size%PAGE_SIZE;
	bitmap_size += padding_size; 
	unsigned int disk_size = bitmap_size + num_blocks*BLOCK_SIZE;
	
	//Mapping my disk in process memory
	if(ftruncate(disk->fd, disk_size) == -1){  
			printf("Error truncating file /n");
			exit(-1);
	}
	
	char* disk_map = (char*)mmap(NULL, bitmap_size, 
		PROT_READ|PROT_WRITE, MAP_SHARED, disk->fd, 0); //mmapping bitmap area on fd
	disk->disk_map = disk_map;
	
	disk->block_map = (char*)NULL;
	disk->first_mapped_block = 0xFFFFFFFF; //Int doesn't support null value in C

	disk->first_block_offset = bitmap_size;
	
	//Calculating free space
	int i;
	disk->free_blocks = 0; 
	char* cursor = disk->disk_map;
	cursor += sizeof(disk->num_entries);
	for (i=0; i<num_blocks; i++){
		if (cursor[i] == 0) disk->free_blocks += 1;
	}
	
	printf("Resumed!\n");
	return 0;
	
}


char* DiskDriver_getBlock(DiskDriver* disk, unsigned int block_index){
	
	//printf("DBG block to point: %d\n", block_index);
	
	if(block_index<0 || block_index > disk->num_entries-1){ //Security check
		printf("Invalid block index!\n");
		return (char*)NULL; 
	}
	
	int mapped_blocks = PAGE_SIZE/BLOCK_SIZE; //This has likely to be == 8
	int offset;
	
	if(disk->first_mapped_block != 0xFFFFFFFF){
		if(block_index >= disk->first_mapped_block 
			  && block_index < (disk->first_mapped_block+mapped_blocks)){
			//Calculating relative block index into mmapped block portion
			offset = block_index - disk->first_mapped_block;
			
			//Moving to the first byte of that block and returning it
			return &(disk->block_map[offset*BLOCK_SIZE]); 
		}
	}
	
	//It represents the portion of blocks that has to be mmapped to get the block "block_index"
	offset = (block_index/mapped_blocks)*PAGE_SIZE; 
	if(disk->block_map != NULL)
		munmap(disk->block_map, BLOCK_SIZE*PAGE_SIZE); //Avoids mem leaks
	
	disk->block_map = (char*)mmap(NULL, BLOCK_SIZE*PAGE_SIZE , 
						PROT_READ|PROT_WRITE, MAP_SHARED, disk->fd, 
										disk->first_block_offset+offset);
	disk->first_mapped_block = (block_index/mapped_blocks)*mapped_blocks; 
	//It works because the fraction takes only integer part.
	
	return DiskDriver_getBlock(disk, block_index);
}

