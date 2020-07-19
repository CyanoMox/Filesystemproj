#ifndef _MYHEADER
#define _MYHEADER
#include "disk_driver.h"
#endif

/*these are structures stored on disk*/

// header, occupies the first portion of each block in the disk
// represents a chained list of blocks
typedef struct {
  int previous_block; // chained list (previous block)
  int next_block;     // chained list (next_block)
  int block_in_file; // position in the file, if 0 we have a file control block
} BlockHeader;


// this is in the first block of a chain, after the header
typedef struct {
  int directory_block; // first block of the parent directory
  int block_in_disk;   // repeated position of the block on the disk
  char name[128];
  int  size_in_bytes;
  int size_in_blocks;
  int is_dir;          // 0 for file, 1 for dir
} FileControlBlock;

// this is the first physical block of a file
// it has a header
// an FCB storing file infos
// and can contain some data

/******************* stuff on disk BEGIN *******************/
typedef struct {
  BlockHeader header;
  FileControlBlock fcb;
  char data[BLOCK_SIZE-sizeof(FileControlBlock) 
			- sizeof(BlockHeader)];
} FirstFileBlock;

// this is one of the next physical blocks of a file
typedef struct {
  BlockHeader header;
  char  data[BLOCK_SIZE-sizeof(BlockHeader)];
} FileBlock;

// this is the first physical block of a directory
typedef struct {
  BlockHeader header;
  FileControlBlock fcb;
  int num_entries;
  int file_blocks[ ((BLOCK_SIZE
		   -sizeof(BlockHeader)
		   -sizeof(FileControlBlock)
		    -sizeof(int))/sizeof(int))];
} FirstDirectoryBlock;

// this is remainder block of a directory
typedef struct {
  BlockHeader header;
  int file_blocks[ ((BLOCK_SIZE
			-sizeof(BlockHeader))/sizeof(int))];
} DirectoryBlock;
/******************* stuff on disk END *******************/


/***IMPORTANT NOTICE: due to the implementation of DiskDriver struct,
 * I can't utilize permanent pointers, because there is no guarantee
 * that they aren't unmapped by next operations on the FS, resulting in
 * Segmentation Fault errors or miswriting and misreading that will 
 * damage the FS itself.
 * 
 * These structures are affected by this, and will be reimplemented:
 * 
 * -DirectoryHandle
 * -FileHandle
 * 
 * ***/
 

typedef struct {
  DiskDriver* disk;
  unsigned int current_directory_block;	  // index of the dir block currently accessed
  char diskname[128];
} SimpleFS;

// this is a file handle, used to refer to open files
typedef struct {
  SimpleFS* sfs;                 // pointer to memory file system structure
  unsigned int fcb;              // index of the first block of the file(read it)
  unsigned int parent_dir;  	 // index of the directory where the file is stored
  
  //fields below were deprecated due to no use in this implementation.
  //unsigned int current_block;   
  //unsigned int pos_in_file;       
} FileHandle;

typedef struct {
  SimpleFS* sfs;                 // pointer to memory file system structure
  unsigned int dcb;       		 // index of the first block of the directory(read it)
  unsigned int parent_dir; 		 // index of the parent directory (null if top level)
  unsigned int current_block;    // current block in the directory
  
  //fields below were deprecated due to no use in this implementation.
  //unsigned int pos_in_dir;      
  //unsigned int pos_in_block;    
} DirectoryHandle;


/*** Design modifications:
 * I want every function in this library to return a status int.
 * For simplicity, eventual handles will be returned via side effect
 * to caller.
 * 
 * This approach has noticeable pros:
 * 1) No memory leak: Handles could be allocated directly by caller
 * 		itself in stack, without the hassle of using malloc() and free()
 * 2) Functions can return different status to caller. This is very 
 * 		useful in some cases.
 * 3) Less code.
 * ***/
 
 
//These global constants are really useful to shorten code 
const int F_FILE_BLOCK_OFFSET = BLOCK_SIZE-sizeof(FileControlBlock) 
			- sizeof(BlockHeader);
const int FILE_BLOCK_OFFSET = BLOCK_SIZE-sizeof(BlockHeader);
const int F_DIR_BLOCK_OFFSET = (BLOCK_SIZE
		   -sizeof(BlockHeader)
		   -sizeof(FileControlBlock)
		    -sizeof(int))/sizeof(int) ;
const int DIR_BLOCK_OFFSET = (BLOCK_SIZE
			-sizeof(BlockHeader))/sizeof(int);
			
// initializes a file system on an already made disk
// returns for side effect a handle to the top level directory 
// stored in the first block. No return status is needed here
void SimpleFS_init(SimpleFS* fs, DirectoryHandle* dest_handle);

// creates the inital structures, the top level directory
// has name "/" and its control block is in the first position
// it also clears the bitmap of occupied blocks on the disk
// and set to the top level directory
int SimpleFS_format(SimpleFS* fs, const char* diskname, int num_blocks);

// creates an empty file in the directory d
// returns -1 on error (file existing)
// returns -2 on error (no free blocks)
// returns -3 on error (generic reading or writing error)
// if no error, returns FileHandle by side effect
// an empty file consists only of a block of type FirstBlock
int SimpleFS_createFile(DirectoryHandle* d, const char* filename, 
											FileHandle* dest_handle);

// reads in the (preallocated) blocks array, the name of all files in a directory 
int SimpleFS_readDir(char* names, DirectoryHandle* d);

// opens a file in the  directory d. The file should be exisiting
int SimpleFS_openFile(DirectoryHandle* d, const char* filename, 
											FileHandle* dest_handle);

// closes a file handle (destroyes it)
//int SimpleFS_close(FileHandle* f);
//No more needed!

// writes in the file, at current position for size bytes stored in data
// overwriting and allocating new space if necessary
// returns the number of bytes written
int SimpleFS_write(FileHandle* f, void* src_data, int size);

// reads in the file, at current position size bytes stored in data
// returns the number of bytes read
int SimpleFS_read(FileHandle* f, void* dst_data, int size);

// returns the number of bytes read (moving the current pointer to pos)
// returns pos on success
// -1 on error (file too short)
//int SimpleFS_seek(FileHandle* f, int pos); 
//Unuseful in this implementation

// seeks for a directory in d. If dirname is equal to ".." it goes one level up
// 0 on success, negative value on error
// it does side effect on the provided handle
int SimpleFS_changeDir(DirectoryHandle* d, char* dirname);

// creates a new directory in the current one
// 0 on success
// -1 on error
int SimpleFS_mkDir(DirectoryHandle* d, char* dirname);

// It gets as an argument the handle of the dir or file to erase.
// removes the file in the current directory
// returns -1 on failure 0 on success
// if a directory, it removes recursively all contained files
// it side-effects and returns upper dir handle;
int SimpleFS_remove(void* handle);

/*** Auxiliary Funcions ***/
//It calculates free space, reading the bitmap iteratively with DiskDriver_getFreeBlock()
int SimpleFS_checkFreeSpace(SimpleFS* fs);

//This function is part of the remove funcition.
//It frees every block that is in a file.
int remFile(SimpleFS*, int file_index);

//This function is part of the remove funcition.
//It frees every file or sub-dir in its array and then deletes dir itself.
int remDir(SimpleFS* fs, int dir_index);

/*** Function implementation ***/
void SimpleFS_init(SimpleFS* fs, DirectoryHandle* dest_handle){
	
	dest_handle->sfs = fs;
	dest_handle->dcb = 0;
	dest_handle->parent_dir = 0xFFFFFFFF; //invalid block index, because we are top level
	dest_handle->current_block = 0;
}


int SimpleFS_format(SimpleFS* fs, const char* diskname, int num_blocks){
	
	//Creates disk file and initializes bitmap
	int res = DiskDriver_init(fs->disk, diskname, num_blocks);
	if (res!=0) {
		printf("Error in formatting!\n");
		return -1;
	}
	
	fs->current_directory_block = 0; //set on top dir
	strncpy(fs->diskname, diskname, sizeof(char)*128);
	
	BlockHeader top_header;
	FileControlBlock top_fcb;
	FirstDirectoryBlock top_dir;
	
	//Building BlockHeader
	top_header.previous_block = 0xFFFFFFFF;
	top_header.next_block = 0xFFFFFFFF;
	top_header.block_in_file = 0;
	
	//Building FileControlBlock
	top_fcb.directory_block = 0xFFFFFFFF; //Because top dir has no parent
	top_fcb.block_in_disk = 0; //Fixed index for this implementation
	strncpy(top_fcb.name, "/", 128*sizeof(char));
	top_fcb.size_in_bytes = 0; //I assume that a dir has no size
	top_fcb.size_in_blocks = 1; //Will be updated if remainder blocks are added
	top_fcb.is_dir = 1;
	
	//Building FirstDirectoryBlock
	top_dir.header = top_header;
	top_dir.fcb = top_fcb;
	top_dir.num_entries = 0;
	
	int i;
	for(i=0;i<F_DIR_BLOCK_OFFSET;i++) top_dir.file_blocks[i] = 0xFFFFFFFF;
	
	//Writing down to file!!!
	res = DiskDriver_writeBlock(fs->disk, &top_dir, 0);
	if (res==-1){
		printf("Error writing to disk!\n");
		return -1;
	}
	return 0;
}


int SimpleFS_createFile(DirectoryHandle* d, const char* filename,
											FileHandle* dest_handle){
	int i, remainder_index, is_full, file_index, prev_remainder;
	
	//Fetching first dir block
	FirstDirectoryBlock pwd_dcb;
	if(DiskDriver_readBlock(d->sfs->disk, &pwd_dcb, d->dcb)!=0) {
		printf("Error reading first dir block\n");
		return -3;
	}
	
	//Checking for same filename
	char names[pwd_dcb.num_entries][128]; //Allocating name matrix
	//we have num_entries sub-vectors by 128 bytes
	
	if(SimpleFS_readDir((char*)names, d)==-1){ 
		return -3;
	}
		
	for(i=0;i<pwd_dcb.num_entries;i++){
		if(strncmp(filename, names[i], 128*sizeof(char))==0){
			printf("Filename already exists!\n");
			return -1;
		}
	}
	
	//Reserving index for file
	file_index = DiskDriver_getFreeBlock(d->sfs->disk, 0);
	if(file_index<0){
		printf("No free space available!\n");
		return -2;
	}
	
	//Temporary block occupation. This will be freed upon real file write
	if(DiskDriver_writeBlock(d->sfs->disk, &pwd_dcb, file_index)!=0) {
		printf("Error reserving file block\n");
		return -3;
	}
	
	//Checking for remainders
	//First remainder
	remainder_index = pwd_dcb.header.next_block;
	//printf("DBG REM INDEX = %d\n", remainder_index);
	
	if(remainder_index!=0xFFFFFFFF){ //we have a remainder
		//For the first remainder, fetching has to be done manually
		DirectoryBlock pwd_rem;
		if(DiskDriver_readBlock(d->sfs->disk, &pwd_rem,
												remainder_index) != 0){
			printf("Error reading first dir block\n");
			return -3;
		}
		prev_remainder = remainder_index;
		remainder_index = pwd_rem.header.next_block;
		
		//For further remainders, fetching is done automatically
		while(remainder_index!=0xFFFFFFFF){
			if(DiskDriver_readBlock(d->sfs->disk, &pwd_rem, 
												remainder_index) !=0 ){
				printf("Error reading first dir block\n");
				return -3;
			}
			prev_remainder = remainder_index;
			remainder_index = pwd_rem.header.next_block;
		}
		is_full = 1;
		//Checking if remainder is full
		for(i=0;i<DIR_BLOCK_OFFSET;i++){
			if(pwd_rem.file_blocks[i]==0xFFFFFFFF){
				is_full = 0;
				break;
			}
		}
		if(is_full == 0){ //I can allocate file directly
			pwd_rem.file_blocks[i] = file_index;
			
			//Updating dir rem
			//printf("DBG prev remainder %d\n", prev_remainder);
			if(DiskDriver_writeBlock(d->sfs->disk, &pwd_rem , 
												prev_remainder) !=0 ){
				printf("Can't update last dir rem\n");
				return -3;
			}
		}
		else{ //I have to allocate a new rem dir block
			DirectoryBlock new_rem;
			
			new_rem.header.previous_block = prev_remainder;
			new_rem.header.next_block = 0xFFFFFFFF;
			new_rem.header.block_in_file = pwd_rem.header.block_in_file+1;
			new_rem.file_blocks[0] = file_index;
			
			for(i=1;i<DIR_BLOCK_OFFSET;i++){ //Initializing file array
				new_rem.file_blocks[i] = 0xFFFFFFFF;
			}
			
			//Writing down this rem dir block
			remainder_index = DiskDriver_getFreeBlock(d->sfs->disk, 0);
			if(remainder_index<0){
				printf ("No free block for dir rem\n");
				return -2;
			}
			
			if(DiskDriver_writeBlock(d->sfs->disk, &new_rem ,
											remainder_index) !=0 ){
				printf("Can't write new dir rem\n");
				return -3;
			}
			
			//Updating dir rem
			pwd_rem.header.next_block = remainder_index;
			if(DiskDriver_writeBlock(d->sfs->disk, &pwd_rem ,
												prev_remainder) !=0 ){
				printf("Can't update last dir rem\n");
				return -3;
			}
			
			pwd_dcb.fcb.size_in_blocks++; //Will be updated with dcb later
		}
	}
	else{ //If we don't have remainders...
				
		is_full = 1;
		//Checking if dcb is full
		for(i=0;i<F_DIR_BLOCK_OFFSET;i++){
			if(pwd_dcb.file_blocks[i]==0xFFFFFFFF){
				is_full = 0;
				break;
			}
		}
		if(is_full == 0){ //I can allocate directly
			
			pwd_dcb.file_blocks[i] = file_index;
			
			//Updating dir dcb
			if(DiskDriver_writeBlock(d->sfs->disk, &pwd_dcb ,
														d->dcb) !=0 ){
				printf("Can't update dcb\n");
				return -3;
			}
		}
		else{ //I have to allocate a new rem dir block
			DirectoryBlock new_rem;
				
			new_rem.header.previous_block = d->dcb;
			new_rem.header.next_block = 0xFFFFFFFF;
			new_rem.header.block_in_file = 1;
			new_rem.file_blocks[0] = file_index;
				
			for(i=1;i<DIR_BLOCK_OFFSET;i++){ //Initializing file array
				new_rem.file_blocks[i] = 0xFFFFFFFF;
			}
				
			//Writing down this rem dir block
			remainder_index = DiskDriver_getFreeBlock(d->sfs->disk, 0);
			if(remainder_index<0){
				printf ("No free block for dir rem\n");
				return -2;
			}
				
			if(DiskDriver_writeBlock(d->sfs->disk, &new_rem ,
												remainder_index) !=0 ){
				printf("Can't write new dir rem\n");
				return -3;
			}
			
			pwd_dcb.fcb.size_in_blocks++;
			pwd_dcb.header.next_block = remainder_index; //will be written in dcb update				
		}
	}
	//Updating dcb
	pwd_dcb.num_entries++;
	if(DiskDriver_writeBlock(d->sfs->disk, &pwd_dcb, d->dcb)!=0){
		printf("Error updating dir\n");
		return -1;
	}
	
	//Creating FirstFileBlock
	FirstFileBlock ffb;
	ffb.header.previous_block = 0xFFFFFFFF; //First block
	ffb.header.next_block = 0xFFFFFFFF; //Not allocated yet - last block
	ffb.header.block_in_file = 0; //First block
	ffb.fcb.directory_block = pwd_dcb.fcb.block_in_disk; //Parent dir
	ffb.fcb.block_in_disk = file_index;
	strncpy(ffb.fcb.name, filename , 128*sizeof(char)); //Setting name
	ffb.fcb.size_in_bytes = 0; //File is size TODO: in write, update this value
	ffb.fcb.size_in_blocks = 1; //Only first block
	ffb.fcb.is_dir = 0; //No, it's a file.
	
	for(i=0;i<F_FILE_BLOCK_OFFSET;i++){
		ffb.data[i] = 0; //Initializing data field
	}
	
	//Freeing file_index block and writing down file
	if(DiskDriver_freeBlock(d->sfs->disk, file_index)!=0){
		printf("Error freeing file block\n");
		return -3;
	}
	
	if(DiskDriver_writeBlock(d->sfs->disk, &ffb, file_index)!=0){
		printf("Error writing down block\n");
		return -3;
	}
	
	//Creating FileHandle
	dest_handle->sfs = d->sfs;
	dest_handle->fcb = ffb.fcb.block_in_disk;
	dest_handle->parent_dir = ffb.fcb.directory_block;
	
	return 0;
}
int SimpleFS_readDir(char* names, DirectoryHandle* d){	
	
	FirstDirectoryBlock pwd_dcb;
	
	if(DiskDriver_readBlock(d->sfs->disk, &pwd_dcb, d->dcb)!=0) {
		printf("Error reading first dir block\n");
		return -1;
	}

	//Exploring dir
	int i, j=0 , remaining_files = pwd_dcb.num_entries;
	FirstFileBlock temp_ffb;
	
	for(i=0;i<F_DIR_BLOCK_OFFSET;i++){		
		if (remaining_files==0) break;
			
		//Reading ffb of every file to obtain name from fcb
		if(DiskDriver_readBlock(d->sfs->disk, &temp_ffb, pwd_dcb.file_blocks[i])!=0) {
			printf("Error reading file block\n");
			return -1;
		}
		
		strncpy(names+(j*128*sizeof(char)),temp_ffb.fcb.name,
			128*sizeof(char)); //I'm assuming char names[pwd_dcb.num_entries][128]
		j++;
		
		remaining_files--;
		
	}
	if (remaining_files==0) return 0; //if no remainder
	//else...
	
	if(pwd_dcb.header.next_block==0xFFFFFFFF){
			printf("Invalid num_entries, Directory is damaged!\n");
			return -1;
		}
	//Read first directory remainder
		DirectoryBlock pwd_rem;
		
		if(DiskDriver_readBlock(d->sfs->disk, &pwd_rem, 
								pwd_dcb.header.next_block) != 0) {
			printf("Error reading first directory remainder block\n");
			return -1;
		}	
	
	while(pwd_rem.header.next_block!=0xFFFFFFFF){ //Scan every remainder	
		
		for(i=0;i<DIR_BLOCK_OFFSET;i++){
			if(pwd_rem.file_blocks[i]==0xFFFFFFFF){
				printf("Invalid num_entries, Directory is damaged!\n");
				return -1;
			}

			if(DiskDriver_readBlock(d->sfs->disk, &temp_ffb, 
										pwd_rem.file_blocks[i]) !=0 ) {
				printf("Error reading directory remainder block\n");
				return -1;
			}
			
			strncpy(names+(j*128*sizeof(char)),temp_ffb.fcb.name,
				128*sizeof(char)); //I'm assuming char names[pwd_dcb.num_entries][128]
			j++;
			remaining_files--;						
		}
		
		if(DiskDriver_readBlock(d->sfs->disk, &pwd_rem, 
									pwd_rem.header.next_block) !=0 ) {
			printf("Error reading directory remainder block\n");
			return -1;
		}	
	}
	
	//Last remainder
	i = 0; 
	int block_index = pwd_rem.file_blocks[i];
	while(block_index!=0xFFFFFFFF && remaining_files!=0){
			
		//Next fcb. No Check here: if 0xFFFFFFFF, simply while will exit
		DiskDriver_readBlock(d->sfs->disk, &temp_ffb, 
												pwd_rem.file_blocks[i]);
		block_index = pwd_rem.file_blocks[i];
		
		strncpy(names+(j*128*sizeof(char)),temp_ffb.fcb.name,
			128*sizeof(char)); //I'm assuming char names[pwd_dcb.num_entries][128]
		i++;
		j++;
		remaining_files--;
	}
	
	if(remaining_files!=0){
		printf("Invalid num_entries, Directory is damaged!\n");
		return -1;
	}
	
	return 0;
}


int SimpleFS_openFile(DirectoryHandle* d, const char* filename, 
											FileHandle* dest_handle){
	
	FirstDirectoryBlock pwd_dcb;
	
	if(DiskDriver_readBlock(d->sfs->disk, &pwd_dcb, d->dcb)!=0) {
		printf("Error reading first dir block\n");
		return -1;
	}
	char names[pwd_dcb.num_entries][128]; //Allocating name matrix
	//we have num_entries sub-vectors by 128 bytes
	
	if(SimpleFS_readDir(names[0], d)==-1) return -1;
	
	//now retrieving FirstFileBlock index
	int array_num = -1; //This value will record the position in the array of the filename itself. From that, it's easy to retrieve the fileindex
	int i;
	FirstDirectoryBlock test_dir;
	for(i=0;i<pwd_dcb.num_entries;i++){
		if(strncmp(names[i],filename, 128*sizeof(char))==0){
			array_num = pwd_dcb.file_blocks[i];
			break; //i has to have the value of	lenght of the array.
		}
	}
	//if no filename match
	if(array_num==-1){
		printf("File not found\n");
		
		dest_handle->sfs = d->sfs;
		dest_handle->fcb = 0xFFFFFFFF;
		dest_handle->parent_dir = 0xFFFFFFFF;
		
		return -1;
	}
		
	//if filename is in the first directory block
	if(i<F_DIR_BLOCK_OFFSET){
		
		dest_handle->sfs = d->sfs;
		dest_handle->fcb = array_num; 
		dest_handle->parent_dir = pwd_dcb.fcb.block_in_disk;
		
		return 0;
		
	}
	//else picking that reminder block
	
	int array_len = i - F_DIR_BLOCK_OFFSET; //excluding first dir block
	int offset = array_len % DIR_BLOCK_OFFSET; //this is the offset to obtain the file in that block
	int rem_dir_num = array_len/DIR_BLOCK_OFFSET; //this is the remainder block in the LL that contains the file
	
	//putting reminder block in stack 
	DirectoryBlock pwd_rem;
		
	//passing explicitly from FirstDirectoryBlock to DirectoryBlock
	if(DiskDriver_readBlock(d->sfs->disk, &pwd_rem, 
										pwd_dcb.header.next_block) !=0 )
		return -1;
		
	for(i=0;i<rem_dir_num;i++){
		if(DiskDriver_readBlock(d->sfs->disk, &pwd_rem, 
										pwd_rem.header.next_block) !=0 )
			return -1;
	}
	
	//now returning handle
	dest_handle->sfs = d->sfs;
	dest_handle->fcb = pwd_rem.file_blocks[offset];
	dest_handle->parent_dir = d->dcb;
	
	return 0;
	
}


int SimpleFS_write(FileHandle* f, void* src_data, int size){
	
	int written_size = 0; //For return purposes

	FirstFileBlock ffb; 
	int current_block = f->fcb;
	
	if(DiskDriver_readBlock(f->sfs->disk, &ffb, current_block)!=0) {
		printf("Error reading First File Block\n");
		return -1;
	}
	
	//Writing/Overwriting FirstFileBlock in stack and then writing back
	if(size>F_FILE_BLOCK_OFFSET){
		//Writing first part, then continue
		memcpy(ffb.data, src_data, F_FILE_BLOCK_OFFSET);
		written_size += F_FILE_BLOCK_OFFSET;
		
		//Writing back Block to File
		if(DiskDriver_writeBlock(f->sfs->disk, &ffb, current_block)!=0){
			printf("Error writing First File Block\n");
			return -1;
		}
	}
	else{
		memcpy(ffb.data, src_data, size);
		written_size += size;
		if(ffb.fcb.size_in_bytes<written_size){
			ffb.fcb.size_in_bytes = written_size;
		}
		
		//Writing back Block to File
		if(DiskDriver_writeBlock(f->sfs->disk, &ffb, current_block)!=0){
			printf("Error writing First File Block\n");
			return -1;
		}
		return written_size;
	}
	
	//If we aren't done writing the whole array yet...
	FileBlock fb1,fb2; //Preparing new block to allocate
	size -= F_FILE_BLOCK_OFFSET; //Calculating remainder size
	char* src_cursor = (char*)src_data;
	//src_cursor += F_FILE_BLOCK_OFFSET; //Calculating new pointer
	
	int i, next_block_index, actual_block_index, first_time=1;
	
	//For the first time:
	next_block_index = ffb.header.next_block;
	if(ffb.header.next_block!=0xFFFFFFFF){ //If next block
		//Fetching next block
		if(DiskDriver_readBlock(f->sfs->disk, &fb2, 
												next_block_index) !=0 ){
			printf("Error reading first next block\n");
			return -1;
		}
		actual_block_index = next_block_index;
	}
	else { //If no next block
		//I'm taking ffb as fb1 for first while iteration
		if(DiskDriver_readBlock(f->sfs->disk, &fb1, f->fcb)!=0){
			printf("Error copying first block for loop\n");
			return -1;
		}
		actual_block_index = f->fcb;
	}
	
	while(size!=0){
		if(next_block_index == 0xFFFFFFFF){ //We have to allocate a new file block
			
			//Retrieving new index to allocate
			next_block_index = DiskDriver_getFreeBlock(f->sfs->disk, 0);
			if(next_block_index<0){
				printf("No free block to allocate new file block\n");
				return -1;
			}
			
			//Building new block to allocate
			fb2.header.previous_block = actual_block_index;
			fb2.header.next_block = 0xFFFFFFFF;
			fb2.header.block_in_file = fb1.header.block_in_file + 1;
			for(i=0;i<FILE_BLOCK_OFFSET;i++){
				fb2.data[i] = 0;
			}
			
			//Writing down new block
			if(DiskDriver_writeBlock(f->sfs->disk, &fb2, 
												next_block_index) !=0 ){ 
				printf("Error writing down new file block");
				return -1;
			}			
			
			//Updating previous header and fcb info
			fb1.header.next_block = next_block_index;
			ffb.fcb.size_in_blocks ++;
			if(ffb.fcb.size_in_bytes<written_size) 
				ffb.fcb.size_in_bytes = written_size;
				
			if(first_time == 1){ //Fixes a bug that doesn't update ffb header
				first_time = 0;
				ffb.header.next_block = next_block_index;
			}
			if(DiskDriver_writeBlock(f->sfs->disk, &fb1, 
											actual_block_index) != 0){ 
				printf("Error writing down new file block\n");
				return -1;
			}
			if(DiskDriver_writeBlock(f->sfs->disk, &ffb, f->fcb)!=0){ 
				printf("Error updating fcb\n");
				return -1;
			}			
			
			//actual_block_index = next_block_index;
		}
		else{ //We need to fill that file block
			actual_block_index = next_block_index;
			
			if(size>FILE_BLOCK_OFFSET){ //Partial copy
				memcpy(fb2.data, src_cursor+written_size,
													FILE_BLOCK_OFFSET);
				written_size += FILE_BLOCK_OFFSET;
				size -= FILE_BLOCK_OFFSET;				
			}
			else{ //Finishing up copy
				memcpy(fb2.data, src_cursor+written_size, size);
				written_size += size;
				size = 0;			
			}
				
			//Writing file block down
			if(DiskDriver_writeBlock(f->sfs->disk, &fb2, 
											actual_block_index) !=0 ){ 
				printf("Error writing down data to file block\n");
				return -1;
			}
				
			//Swapping between fcb1 and fcb2
			fb1 = fb2;
			next_block_index = fb1.header.next_block;
			
			//Preparing next fb2 block
			if(next_block_index != 0xFFFFFFFF){
				if(DiskDriver_readBlock(f->sfs->disk, &fb2, 
												next_block_index) !=0 ){
					printf("Error reading next file block\n");
					return -1;
				}
			}
		}
	}
	
	if(ffb.fcb.size_in_bytes<written_size){ 
		ffb.fcb.size_in_bytes = written_size;
		if(DiskDriver_writeBlock(f->sfs->disk, &ffb, f->fcb)!=0){ 
			printf("Error updating fcb\n");
			return -1;
		}
	}
	return written_size;
}


int SimpleFS_read(FileHandle* f, void* dst_data, int size){
	
	FileBlock ffb; //same as SimpleFS_write
	FirstFileBlock* ffb_pointer = (FirstFileBlock*)&ffb;
	int read_bytes = 0;
	
	char* data_pointer = ffb_pointer->data; //For the FirstFileBlock
	if(DiskDriver_readBlock(f->sfs->disk, ffb_pointer, f->fcb) != 0){
		printf("Error reading First Block\n");
		return read_bytes;
	}
	
	if(size<=F_FILE_BLOCK_OFFSET){
		memcpy(dst_data, data_pointer, size);
		read_bytes = size;
		return read_bytes;
	}
	
	else{
		memcpy(dst_data, data_pointer, F_FILE_BLOCK_OFFSET);
		read_bytes = F_FILE_BLOCK_OFFSET;
	}
	//If there are more blocks...
	
	FileBlock fb; //This is the next block
	int rem_data = size-read_bytes; //It counts remaining bytes to read
		
	while(1){
		
		//If the previous was the last block
		if(ffb.header.next_block==0xFFFFFFFF){
			printf("File is shorter than size");
			return read_bytes;
		}
		//Loading next block in memory
		if(DiskDriver_readBlock(f->sfs->disk, &fb, 
											ffb.header.next_block) != 0){
			printf("Error reading First Block\n");
			return read_bytes;
		}
		//Reading it
		if(rem_data<=FILE_BLOCK_OFFSET){
			memcpy(dst_data+(size-rem_data), fb.data, rem_data);
			read_bytes += rem_data;
			return read_bytes;
		}
		else{
			memcpy(dst_data+(size-rem_data), fb.data, FILE_BLOCK_OFFSET);
			read_bytes += FILE_BLOCK_OFFSET;
			rem_data = size-read_bytes;
		}
		//Son becomes father of his next
		ffb = fb;
	}
}


int SimpleFS_changeDir(DirectoryHandle* d, char* dirname){
	
	//Generating updir name for comparison
	char updir[128];
	strncpy(updir, "..", 128);
	
	FileHandle dest_handle;
	if(strncmp(dirname, updir, 128)!=0){
		
		if(SimpleFS_openFile(d, dirname, &dest_handle)!=0){
			return -1;
		}
		
		
		FirstDirectoryBlock dir;
		if(DiskDriver_readBlock(d->sfs->disk, &dir, d->dcb)!=0){
			printf("Error reading dir/file\n");
			return -1;
		}
		
		if(dir.fcb.is_dir==0){
			printf("This is not a dir\n");
			return -1;
		}
		
		/*This works because header and fcb of FirstFileBlock 
		 * and FirstDirBlock have same offsets, so simply get FileHandle
		 * by openFile() and cast it into a DirectoryHandle*/
		
		memcpy(d, &dest_handle, sizeof(DirectoryHandle));
		
		return 0;
	}
	//If name is ".."
	
	//Retrieving parent of the parent dir
	FirstDirectoryBlock parent;
	
	//Checking if we are in top dir. If so, return top dir handle.
	if(d->parent_dir == 0xFFFFFFFF){
		printf("We already are in root directory!\n");
		return 0; //Do not modify handle.
	}
	
	if(DiskDriver_readBlock(d->sfs->disk, &parent, d->parent_dir) != 0){
		printf("Error retrieving parent dir!\n"); 
		
	}
	
	DirectoryHandle updir_handle; 
	updir_handle.sfs = d->sfs;
	updir_handle.dcb = d->parent_dir;
	updir_handle.parent_dir = parent.fcb.directory_block;
	updir_handle.current_block = 0;
	
	memcpy(d, &updir_handle, sizeof(DirectoryHandle));
	
	return 0;
}


int SimpleFS_mkDir(DirectoryHandle* d, char* dirname){
	//I'm taking advantage of createFile to allocate block and dir_rems
	//Then, I will use the handle to set is_dir to 1. Magic.
	
	//Generating updir name for comparison
	char updir[128];
	strncpy(updir, "..", 128);
	
	FileHandle dest_handle;
	FirstDirectoryBlock new_dir;
	if(strncmp(dirname, updir, 128) != 0){
		SimpleFS_createFile(d, dirname, &dest_handle);
	}
	else{
	 printf("Cannot create .. dir!\n");	
	 return -1;
	}
	
	//Read the already-created file
	if(DiskDriver_readBlock(d->sfs->disk, &new_dir, 
												dest_handle.fcb) != 0){
		printf("Error reading created file for transormation in dir\n");
		return -1;
	}
	
	//Turn it into a dir!
	new_dir.fcb.is_dir = 1; //BOOM!
	new_dir.num_entries = 0;
	int i;
	for(i=0;i<F_DIR_BLOCK_OFFSET;i++){
		new_dir.file_blocks[i] = 0xFFFFFFFF;
	}
	
	//Update dir status on disk
	if(DiskDriver_writeBlock(d->sfs->disk, &new_dir, 
												dest_handle.fcb) != 0){
		printf("Error turning file into a dir\n");
		return -1;
	}
	
	//Updating DirectoryHandle
	d->parent_dir = d->dcb;
	d->dcb = dest_handle.fcb;
	//TODO: eliminate other values of handles which I don't utilize.
	
	return 0;
}


int SimpleFS_remove(void* handle){
	FileHandle* file_handle = (FileHandle*)handle;
	int index = ((FileHandle*)handle) -> fcb;
	FirstDirectoryBlock temp;
	
	if(DiskDriver_readBlock(file_handle->sfs->disk, &temp, index) != 0){
		printf("Error reading file or folder\n");
		return -1;
	}
	
	//Checking if file or folder
	if(temp.fcb.is_dir == 0){ //If file
		printf("File detected! Index: %d\n", index);
		
		if(remFile(file_handle->sfs, index) != 0){
			printf("Error deleting file\n");
			return -1;
		}
	}
	else{
		printf("Dir detected! Index: %d\n", index);
		
		if(remDir(file_handle->sfs, index) != 0){
			printf("Error deleting dir\n");
			return -1;
		}
	}
	
	//Now compacting dir array
	//Searching for deleted position in the array
	FirstDirectoryBlock upper_dir;
	DirectoryBlock rem_dir;
	DirectoryBlock temp_rem;
	int actual_index = ((FileHandle*)handle) -> parent_dir;
	int i, item_to_move, rem_index, last_rem_index;
	
	if(DiskDriver_readBlock(file_handle->sfs->disk, 
									&upper_dir, actual_index) != 0){
		printf("Error reading upper dir\n");
		return -1;
	}
	
	//Trying in dcb
	int is_inside = 0;
	for(i=0;i<F_DIR_BLOCK_OFFSET;i++){
		if(upper_dir.file_blocks[i] == index){
			is_inside = 1;
			break;
		}
	}
	
	if(is_inside == 0){ //If pos is not in first dir block
		//Find position in the array where the eliminated entry is
		while(actual_index != 0xFFFFFFFF){
			
			if(DiskDriver_readBlock(file_handle->sfs->disk, 
										&rem_dir, actual_index) != 0){
				printf("Error reading dir block to compact\n");
				return -1;
			}
			
			for(i=0;i<DIR_BLOCK_OFFSET;i++){
				if(rem_dir.file_blocks[i] == index){
					is_inside = 1;
					break;
				}
			}
			
			if(is_inside!=0) break;
			actual_index = rem_dir.header.next_block;
		}	
		
		if(is_inside == 0){
			printf("Dir or handle is damaged!\n");
			return -1;
		}
		
		//Moving last item to erased one, then modifying num_entries in dcb
		temp_rem = rem_dir;
		rem_index = actual_index;
		while(actual_index != 0xFFFFFFFF){
			
			//Moving to last remainder
			last_rem_index = actual_index;
			if(DiskDriver_readBlock(file_handle->sfs->disk, 
										&temp_rem, actual_index) != 0){
				printf("Error reading dir block to compact\n");
				return -1;
			}
			
			actual_index = rem_dir.header.next_block;
		}
		
		//Finding last element in array
		for(item_to_move=0;i<DIR_BLOCK_OFFSET;item_to_move++){
			if(temp_rem.file_blocks[item_to_move] == 0xFFFFFFFF){
				item_to_move--;
				break;
			}
		}
		
		//Making and writing changes to blocks
		rem_dir.file_blocks[i] = temp_rem.file_blocks[item_to_move];		
		
		if(DiskDriver_writeBlock(file_handle->sfs->disk, 
											&rem_dir, rem_index) != 0){
			printf("Error writing dir block to compact\n");
			return -1;
		}
		
		//If rem is now empty, delete it!
		if(item_to_move==0){
			if(DiskDriver_freeBlock(file_handle->sfs->disk, 
												last_rem_index) != 0){
				printf("Error deleting empty remainder!\n");
				return -1;
			}
		}
		else{
			temp_rem.file_blocks[item_to_move] = 0xFFFFFFFF;
			if(DiskDriver_writeBlock(file_handle->sfs->disk, &temp_rem,
												last_rem_index) != 0){
				printf("Error writing last dir block to compact\n");
				return -1;
			}
		}
	}
	else{ //If deleted item is in dcb
		if(upper_dir.header.next_block==0xFFFFFFFF){ //If no remainders
			printf("No remainders detected!\n");
			
			is_inside = 0;
			for(item_to_move=0;item_to_move<F_DIR_BLOCK_OFFSET;
														item_to_move++){
				if(upper_dir.file_blocks[item_to_move]==0xFFFFFFFF){
					upper_dir.file_blocks[i] = 
								upper_dir.file_blocks[item_to_move-1];
					upper_dir.file_blocks[item_to_move-1] = 0xFFFFFFFF;
					is_inside = 1;
					break;
				}
			}
			if(is_inside == 0) { //If first dir block is full but no rem
				upper_dir.file_blocks[i] = 
							upper_dir.file_blocks[F_DIR_BLOCK_OFFSET-1];
				upper_dir.file_blocks[F_DIR_BLOCK_OFFSET-1] = 0xFFFFFFFF;
			}
		}
		else{ //If item in dcb and remainders
			printf("Remainders detected!\n");
			while(actual_index!=0xFFFFFFFF){
				//Moving to last remainder
				last_rem_index = actual_index;
				if(DiskDriver_readBlock(file_handle->sfs->disk, 
										&temp_rem, actual_index) != 0){
					printf("Error reading dir block to compact\n");
					return -1;
				}
			
				actual_index = temp_rem.header.next_block;
			}
			
			
			//Finding last element in array
			for(item_to_move=0;i<DIR_BLOCK_OFFSET;item_to_move++){
				if(temp_rem.file_blocks[item_to_move] == 0xFFFFFFFF){
					item_to_move--;
					break;
				}
			}
			
			//Making and writing changes to blocks
			upper_dir.file_blocks[i] = temp_rem.file_blocks[item_to_move];		
			
			//If rem is now empty, delete it!
			if(item_to_move==0){
				if(DiskDriver_freeBlock(file_handle->sfs->disk, 
												last_rem_index) != 0){
					printf("Error deleting empty remainder!\n");
					return -1;
				}
				upper_dir.fcb.size_in_blocks--;
				actual_index = temp_rem.header.previous_block;
				
				if(actual_index!=0){
					if(DiskDriver_readBlock(file_handle->sfs->disk, 
										&temp_rem, actual_index) != 0){
						printf("Error reading dir block to compact\n");
						return -1;
					}
					temp_rem.header.next_block = 0xFFFFFFFF; //Detaching prev block
					
					if(DiskDriver_writeBlock(file_handle->sfs->disk, 
										&temp_rem, actual_index) != 0){
						printf("Error writing last dir \
												block to compact\n");
						return -1;
					}
				} //If prev block is dcb, special treatment below
				
				printf("Empty dir rem deleted\n");
			}
			else{
				temp_rem.file_blocks[item_to_move] = 0xFFFFFFFF;
				if(DiskDriver_writeBlock(file_handle->sfs->disk, 
										&temp_rem, last_rem_index)!=0){
					printf("Error writing last dir block to compact\n");
					return -1;
				}
			}	
		}
	}
	
	//Update upper_dir
	upper_dir.num_entries--;
	if(actual_index == 0){
		upper_dir.header.next_block = 0xFFFFFFFF;
		//If the next rem was removed, detach that block by dcb.
		//It has no real effect if there was no rem.
	}
	
	if(DiskDriver_writeBlock(file_handle->sfs->disk, &upper_dir, 
							((FileHandle*)handle) -> parent_dir)!=0){
		printf("Error writing updates on dcb\n");
		return -1;
	}
	
	//side-effect on handle
	SimpleFS_changeDir(handle, "..");
	
	return 0;
}

int remFile(SimpleFS* fs, int file_index){
	FirstFileBlock ffb;
	int actual_index = file_index;
	
	//Iterative destruction!
	while(actual_index != 0xFFFFFFFF){
		
		//Loading next block in memory
		if(DiskDriver_readBlock(fs->disk, &ffb, actual_index)!=0){
			printf("Error reading file block to delete\n");
			return -1;
		}
		
		//Destroying actual block
		if(DiskDriver_freeBlock(fs->disk, actual_index)!=0){
			printf("Error freeing file block!\n");
			return -1;
		}
		
		actual_index = ffb.header.next_block;				
	}
	
	return 0;
}


int remDir(SimpleFS* fs, int dir_index){
	FirstDirectoryBlock pwd;
	DirectoryBlock pwd_rem;
	FirstFileBlock temp;
	
	printf("DBG dir index: %d\n", dir_index);
	
	if(DiskDriver_readBlock(fs->disk, &pwd, dir_index)!=0){
		printf("Error reading dir dcb to delete\n");
		return -1;
	}	
	
	int actual_index = pwd.header.next_block;
		
	int i=0;
	//For remainders
	//Iterative destruction!
	while(actual_index != 0xFFFFFFFF){
		
		printf("DBG dir rem index: %d\n", actual_index);
		
		//Loading next block in memory
		if(DiskDriver_readBlock(fs->disk, &pwd_rem, actual_index) != 0){
			printf("Error reading dir remainder block to delete\n");
			return -1;
		}
		
		//Emptying that dir block
		for(i=0;i<DIR_BLOCK_OFFSET;i++){
			//Read every item in array,
			
			//printf("DBG1: %d\n", i);
			
			//Check if array is now empty
			if (pwd_rem.file_blocks[i] == 0xFFFFFFFF) break;
			
			//If not empty, if file, invoke remFile, if dir, recursion!
			if(DiskDriver_readBlock(fs->disk, &temp, 
										pwd_rem.file_blocks[i]) != 0){
											
				//printf("DBG!\n");
				printf("Error reading dir dcb to delete\n");
				return -1;
			}
			
			if(temp.fcb.is_dir == 0){ //If file, invoke remFile
				if(remFile(fs, pwd_rem.file_blocks[i]) != 0){
					return -1;
				}
			}
			else{ //If dir, recursion				
				printf("DBG Recursion!\n");
				if(remDir(fs, pwd_rem.file_blocks[i]) != 0){
					return -1;
				}
			}
		}
		
		//Destroying actual dir block
		if(DiskDriver_freeBlock(fs->disk, actual_index) != 0){
			printf("Error freeing dir remainder block!\n");
			return -1;
		}
		
		actual_index = pwd_rem.header.next_block;				
	}
	
	//Explore dcb array
	for(i=0;i<F_DIR_BLOCK_OFFSET;i++){
		//Read every item in array,
		//if file, invoke remFile, if dir, recursion!
		if(pwd.file_blocks[i] == 0xFFFFFFFF) break;
		
		if(DiskDriver_readBlock(fs->disk, &temp, 
											pwd.file_blocks[i]) != 0){
			printf("Error reading dir dcb to delete\n");
			return -1;
		}
		
		if(temp.fcb.is_dir == 0){ //If file, invoke remFile
			if(remFile(fs, pwd.file_blocks[i]) != 0){
				return -1;
			}
		}
		else{ //If dir
			if(remDir(fs, pwd.file_blocks[i]) != 0){
				return -1;
			}
		}
	}
	
	//Then eliminate dir dcb!
	if(DiskDriver_freeBlock(fs->disk, dir_index) != 0){
		printf("Error freeing dir dcb block!\n");
		return -1;
	}
	
	return 0;
}

int SimpleFS_checkFreeSpace(SimpleFS* fs){
	
	int i=0,res=0;
	while(i != -1){
		i = DiskDriver_getFreeBlock(fs->disk, i+1);
		if(i != -1) res++;
	}
	printf("Number of free blocks = %d\nFrom disk it results = %d\n", 
											res, fs->disk->free_blocks);
	return res;
}
