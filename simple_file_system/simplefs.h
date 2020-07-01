#pragma once
#include "disk_driver.h"

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
  // add more fields if needed
  unsigned int current_directory_block;	  // index of the dir block currently accessed
} SimpleFS;

// this is a file handle, used to refer to open files
typedef struct {
  SimpleFS* sfs;                 // pointer to memory file system structure
  unsigned int fcb;              // index of the first block of the file(read it)
  unsigned int parent_dir;  	 // index of the directory where the file is stored
  unsigned int current_block;    // number of current block in the file
  unsigned int pos_in_file;       // position of the cursor in the block
} FileHandle;

typedef struct {
  SimpleFS* sfs;                 // pointer to memory file system structure
  unsigned int dcb;       		 // index of the first block of the directory(read it)
  unsigned int parent_dir; 		 // index of the parent directory (null if top level)
  unsigned int current_block;    // current block in the directory
  unsigned int pos_in_dir;       // absolute position of the cursor in the directory
  unsigned int pos_in_block;     // relative position of the cursor in the block
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
// the current_directory_block is cached in the SimpleFS struct
// and set to the top level directory
int SimpleFS_format(SimpleFS* fs, const char* diskname, int num_blocks);

// creates an empty file in the directory d
// returns -1 on error (file existing)
// returns -2 on error (no free blocks)
// returns -3 on error (generic reading or writing error)
// if no error, returns FileHandle by side effect
// an empty file consists only of a block of type FirstBlock
int SimpleFS_createFile(DirectoryHandle* d, const char* filename, FileHandle* dest_handle);

// reads in the (preallocated) blocks array, the name of all files in a directory 
int SimpleFS_readDir(char* names, DirectoryHandle* d);

// opens a file in the  directory d. The file should be exisiting
int SimpleFS_openFile(DirectoryHandle* d, const char* filename, FileHandle* dest_handle);

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

// creates a new directory in the current one (stored in fs->current_directory_block)
// 0 on success
// -1 on error
int SimpleFS_mkDir(DirectoryHandle* d, char* dirname);

// It gets as an argument the index of the dir or the file to erase.
// I made this to avoid rewiting the same code again and to avoid
// 		confusion if a dir and a file have the same name.
// removes the file in the current directory
// returns -1 on failure 0 on success
// if a directory, it removes recursively all contained files
int SimpleFS_remove(SimpleFS* fs, int index);

/*** Auxiliary Funcions ***/
//It calculate free space, reading the bitmap iteratively with DiskDriver_getFreeBlock()
int SimpleFS_checkFreeSpace(SimpleFS* fs);


/*** Function implementation ***/
void SimpleFS_init(SimpleFS* fs, DirectoryHandle* dest_handle){
	
	dest_handle->sfs = fs;
	dest_handle->dcb = 0;
	dest_handle->parent_dir = 0xFFFFFFFF; //invalid block index, because we are top level
	dest_handle->current_block = 0;
	dest_handle->pos_in_dir = 0;
	dest_handle->pos_in_block = 0;

}


int SimpleFS_format(SimpleFS* fs, const char* diskname, int num_blocks){
	
	//Creates disk file and initializes bitmap
	int res = DiskDriver_init(fs->disk, diskname, num_blocks);
	if (res!=0) {
		printf("Error in formatting!\n");
		return -1;
	}
	
	fs->current_directory_block = 0; //set on top dir
	
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
	top_fcb.size_in_blocks = 0; //Will be updated if remainder blocks are added
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


int SimpleFS_createFile(DirectoryHandle* d, const char* filename, FileHandle* dest_handle){
	
	FirstDirectoryBlock pwd_dcb;
	
	int free_block;
	
	int res = DiskDriver_readBlock(d->sfs->disk, &pwd_dcb, d->dcb);
	if(res!=0) {
		printf("Error reading first dir block\n");
		return -3;
	}	

	char names[pwd_dcb.num_entries][128]; //Allocating name matrix
	//we have num_entries sub-vectors by 128 bytes
	
	if(SimpleFS_readDir((char*)names, d)==-1){ //TODO: verify problem of char*[128]
		return -3;
	}
	
	//Checking for same filename
	int i;
	for(i=0;i<pwd_dcb.num_entries;i++){
		if(strncmp(filename, names[i], 128*sizeof(char))==0){
			printf("Filename already exists!\n");
			return -1;
		}
	}
	
	free_block = DiskDriver_getFreeBlock(d->sfs->disk, 0); //retrieving first free block available
	if(res==-1) {
		printf("No free block available!\n");
		return -2;
	}
	printf("File Index will be: %d", res);
	
	//Creating FirstFileBlock
	FirstFileBlock ffb;
	ffb.header.previous_block = 0xFFFFFFFF; //First block
	ffb.header.next_block = 0xFFFFFFFF; //Not allocated yet - last block
	ffb.header.block_in_file = 0; //First block
	ffb.fcb.directory_block = pwd_dcb.fcb.block_in_disk; //Parent dir
	ffb.fcb.block_in_disk = free_block;
	strncpy(ffb.fcb.name, filename , 128*sizeof(char)); //Setting name
	ffb.fcb.size_in_bytes = 0; //File is size 0
	ffb.fcb.size_in_blocks = 1; //Only first block
	ffb.fcb.is_dir = 0; //No, it's a file.
	
	for(i=0;i<F_FILE_BLOCK_OFFSET;i++){
		ffb.data[i] = 0; //Initializing data field
	}	
	
	//Update parent dir status
	i = 0;
	int is_full = 1;
	while(i<F_DIR_BLOCK_OFFSET){
		if(pwd_dcb.file_blocks[i]==0xFFFFFFFF){			
			is_full = 0;
			break;
		}
	} 
	pwd_dcb.file_blocks[i] = free_block;
	printf("\nDBG2: %d", pwd_dcb.file_blocks[i]);
	
	if(DiskDriver_writeBlock(d->sfs->disk, &pwd_dcb, d->dcb)!=0){
		printf("Cannot update parent dir status\n");
		return -1;
	}
	
	//If Directory has remainders
	if(is_full==1){	
		DirectoryBlock pwd_rem;
		int rem_index = pwd_dcb.header.next_block;
		int rem_free_block;
	
		//Fetching first remainder 
		res = DiskDriver_readBlock(d->sfs->disk, &pwd_rem, rem_index);
		if(res!=0){
			printf("Error reading directory remainder\n");
			return -1;
		}
		while (pwd_rem.header.next_block!=0xFFFFFFFF){
			//Proceed to next remainder. This works because I'm assuming
			//	no holes are present in directory allocation vector
			rem_index = pwd_dcb.header.next_block;
			DiskDriver_readBlock(d->sfs->disk, &pwd_rem, rem_index);
		}
		i = 0;
		while(i<DIR_BLOCK_OFFSET){
			if(pwd_rem.file_blocks[i]==0xFFFFFFFF){			
				is_full = 0;
				break;
			}
		} 
		pwd_dcb.file_blocks[i] = rem_free_block;
		printf("\nDBG3: %d", pwd_rem.file_blocks[i]);
		
		//If no remainder is not full
		//Allocating a new dir remainder
		if(is_full==1){
			rem_free_block = DiskDriver_getFreeBlock(d->sfs->disk, 0);
			if(rem_free_block == -1){
				printf("No more free blocks to allocate directory remainder\n");
				return -2;
			}
			pwd_dcb.header.next_block = rem_free_block;
			
			//Updating rem dir chain
			if(DiskDriver_writeBlock(d->sfs->disk, &pwd_rem, rem_index)!=0){
				printf("Error updating previous remainder block\n");
				return -1;
			}
			
			DirectoryBlock new_rem;
			
			new_rem.header.previous_block = rem_index;
			new_rem.header.next_block = 0xFFFFFFFF;
			new_rem.header.block_in_file = pwd_rem.header.block_in_file+1;
			
			for(i=1;i<DIR_BLOCK_OFFSET;i++){
				new_rem.file_blocks[i] = 0xFFFFFFFF;
			}
			new_rem.file_blocks[0] = free_block; //Finallt I'm allocating file index!
			
			//Writing down new dir rem
			if(DiskDriver_writeBlock(d->sfs->disk, &new_rem, rem_free_block)!=0){
				printf("Error updating previous remainder block\n");
				return -1;
			}			
		}
	}
	
	//Writing FirstFileBlock down at free_block position
	res = DiskDriver_writeBlock(d->sfs->disk, &ffb, free_block);
	if(res!=0){
		printf("Error writing down block");
	}
	
	//Creating FileHandle
	dest_handle->sfs = d->sfs;
	dest_handle->fcb = ffb.fcb.block_in_disk;
	dest_handle->parent_dir = ffb.fcb.directory_block;
	dest_handle->current_block = 0;
	dest_handle->pos_in_file = 0;
	
	return 0;
}


int SimpleFS_readDir(char* names, DirectoryHandle* d){
	
	
	FirstDirectoryBlock pwd_dcb;
	
	int res = DiskDriver_readBlock(d->sfs->disk, &pwd_dcb, d->dcb);
	if(res!=0) {
		printf("Error reading first dir block\n");
		return -1;
	}
	
	//BlockHeader* pwd_header = (BlockHeader*)&pwd_dcb.header;
	//FileControlBlock* pwd_fcb = (FileControlBlock*)&pwd_dcb.fcb;
	
	//Exploring dir
	int i, j=0 , remaining_files = pwd_dcb.num_entries;
	FirstFileBlock temp_ffb;
	
	for(i=0;i<F_DIR_BLOCK_OFFSET;i++){
		if(pwd_dcb.file_blocks[i]!=0xFFFFFFFF){ //if index is valid
			
			remaining_files--;
			
			//Reading ffb of every file to obtain name from fcb
			res = DiskDriver_readBlock(d->sfs->disk, &temp_ffb, pwd_dcb.file_blocks[i]);
			if(res!=0) {
				printf("Error reading file block\n");
				return -1;
			}
			
			strncpy(names+j,temp_ffb.fcb.name,128*sizeof(char)); //I'm assuming char names[pwd_dcb.num_entries][128]
			j++;
		}
	}
	if (remaining_files==0) return 0;
	//else...
	
	if(pwd_dcb.header.next_block==0xFFFFFFFF){
			printf("Invalid num_entries, folder is damaged!\n");
			return -1;
		}
	//Read directory remainder
		DirectoryBlock pwd_rem;
		
		res = DiskDriver_readBlock(d->sfs->disk, &pwd_rem, pwd_dcb.header.next_block);
		if(res!=0) {
			printf("Error reading directory remainder block\n");
			return -1;
		}
	
	do{
		//Again...
		for(i=0;i<DIR_BLOCK_OFFSET;i++){
			if(pwd_rem.file_blocks[i]!=0xFFFFFFFF){ //if index is valid
			
				remaining_files--;
			
				//Reading ffb of every file to obtain name from fcb
				res = DiskDriver_readBlock(d->sfs->disk, &temp_ffb, pwd_rem.file_blocks[i]);
				if(res!=0) {
					printf("Error reading file block\n");
					return -1;
				}
			
				strncpy(names+j,temp_ffb.fcb.name,128*sizeof(char)); //I'm assuming char names[pwd_dcb.num_entries][128]
				j++;
			}
			
			if (remaining_files!=0){
				//Checks if we reached the final remainder block	
				if(pwd_dcb.header.next_block==0xFFFFFFFF){
					printf("Invalid num_entries, folder is damaged!\n");
					return -1;
				}
			
				//Read directory remainder	
				res = DiskDriver_readBlock(d->sfs->disk, &pwd_rem, pwd_dcb.header.next_block);
				if(res!=0) {
					printf("Error reading directory remainder block\n");
					return -1;
				}
			}
		}
	}
	while(remaining_files!=0);
	
	return 0;
}


int SimpleFS_openFile(DirectoryHandle* d, const char* filename, FileHandle* dest_handle){
	
	FirstDirectoryBlock pwd_dcb;
	
	int res = DiskDriver_readBlock(d->sfs->disk, &pwd_dcb, d->dcb);
	if(res!=0) {
		printf("Error reading first dir block\n");
		return -1;
	}
	char names[pwd_dcb.num_entries][128]; //Allocating name matrix
	//we have num_entries sub-vectors by 128 bytes
	if(SimpleFS_readDir(names[0], d)==-1) return -1;
	
	
	//now retrieving FirstFileBlock index
	int array_num = -1; //This value will record the position in the array of the filename itself. From that, it's easy to retrieve the fileindex
	
	int i;
	for(i=0;i<pwd_dcb.num_entries;i++){
		if(strncmp(names[i],filename, 128*sizeof(char))==0) array_num = i;
	}
	//if no filename match
	if(array_num==-1){
		printf("File not found\n");
		return -1;
	}
		
	//if filename is in the first directory block
	if(array_num<=F_DIR_BLOCK_OFFSET){
		
		if (array_num<F_DIR_BLOCK_OFFSET){
			dest_handle->sfs = d->sfs;
			dest_handle->fcb = pwd_dcb.file_blocks[i];
			dest_handle->parent_dir = pwd_dcb.fcb.block_in_disk;
			dest_handle->current_block = 0;
			dest_handle->pos_in_file = 0;
			
			return 0;
		}
	}
	//else picking un that reminder block
	
	array_num -= F_DIR_BLOCK_OFFSET; //excluding first dir block
	int offset = array_num % DIR_BLOCK_OFFSET; //this is the offset to obtain the file in that block
	int rem_dir_num = array_num/DIR_BLOCK_OFFSET; //this is the remainder block in the LL that contains the file
	
		
	//putting reminder block in stack 
	DirectoryBlock pwd_rem;
		
	//passing explicitly from FirstDirectoryBlock to DirectoryBlock
	if(DiskDriver_readBlock(d->sfs->disk, &pwd_rem, pwd_dcb.header.next_block)!=0)
		return -1;
		
	//getting right DirBlock trough iterations
	for(i=0;i<rem_dir_num;i++){
		if(DiskDriver_readBlock(d->sfs->disk, &pwd_rem, pwd_rem.header.next_block)!=0) //TODO: check this out
			return -1;
	}
	
	//now returning handle
	dest_handle->sfs = d->sfs;
	dest_handle->fcb = pwd_rem.file_blocks[offset];
	dest_handle->parent_dir = d->dcb;
	dest_handle->current_block = 0;
	dest_handle->pos_in_file = 0;
		
	return 0;
	
}


int SimpleFS_write(FileHandle* f, void* src_data, int size){
	
	int written_size = 0; //For return purposes
	//FirstFileBlock ffb;
	FileBlock ffb; //I prefer to read FirstFileBlock as FileBlock for further iterations!
	FirstFileBlock* ffb_pointer = (FirstFileBlock*)&ffb; //For first time only
	int current_block = f->fcb;
	
	int res = DiskDriver_readBlock(f->sfs->disk, &ffb, current_block);
	if(res!=0) {
		printf("Error reading First File Block\n");
		return -1;
	}
	
	//Writing/Overwriting FirstFileBlock in stack and then writing back
	if(size>F_FILE_BLOCK_OFFSET){
		memcpy(ffb_pointer->data, src_data, F_FILE_BLOCK_OFFSET);
		written_size += F_FILE_BLOCK_OFFSET;
		
		//Writing back Block to File
		res = DiskDriver_writeBlock(f->sfs->disk, &ffb, current_block);
		if(res!=0){
			printf("Error writing First File Block\n");
			return -1;
		}
	}
	else{
		memcpy(ffb_pointer->data, src_data, size);
		written_size += size;
		
		//Writing back Block to File
		res = DiskDriver_writeBlock(f->sfs->disk, &ffb, current_block);
		if(res!=0){
			printf("Error writing First File Block\n");
			return -1;
		}
		return written_size;
	}
	
	//If we aren't done writing the whole array yet...
	FileBlock fb; //Preparing new block to allocate
	size -= F_FILE_BLOCK_OFFSET; //Calculating remainder size
	src_data += F_FILE_BLOCK_OFFSET; //Calculating new pointer
	
	int i;
	
	while(1){ //Until one return condition verifies
		if(ffb.header.next_block==0xFFFFFFFF){ //If no next block
			
			//Allocating new block before writing it
			res = DiskDriver_getFreeBlock(f->sfs->disk, 0);
			if(res!=0){
				printf("Error allocating next File Block\n");
				return -1;
			}
			
			//Updating header of FirstFileBlock
			ffb.header.next_block = res;
			if(DiskDriver_writeBlock(f->sfs->disk, &ffb, current_block)!=0){
				printf("Cannot append next block to previous one");
			}
			
			//Initializing new FileBlock
			for(i=0;i<BLOCK_SIZE;i++){
				fb.data[i] = 0;
			}
			//Creating new FileBlock in memory
			fb.header.previous_block = current_block;
			fb.header.next_block = 0xFFFFFFFF;
			fb.header.block_in_file = ffb.header.block_in_file+1;
		}
		//else simply use existing next block
		else{
			if(DiskDriver_readBlock(f->sfs->disk, &fb, ffb.header.next_block)!=0){
				printf("Error reading next block\n");
				return -1;
			}
		}
			
		//Writing/Overwriting next block
		if(size>FILE_BLOCK_OFFSET){
			memcpy(fb.data, src_data, FILE_BLOCK_OFFSET);
			written_size += FILE_BLOCK_OFFSET;
			
			//Writing new Block to File
			if(DiskDriver_writeBlock(f->sfs->disk, &fb, ffb.header.next_block)!=0){
				printf("Error writing next File Block\n");
				return -1;
			}
		}
		else{
			memcpy(fb.data, src_data, size);
			written_size += size;
			
			//Writing new Block to File
			if(DiskDriver_writeBlock(f->sfs->disk, &fb, ffb.header.next_block)!=0){
				printf("Error writing next File Block\n");
				return -1;
			}
			return written_size;
		}
		//Preparing for next iteration
		size -= FILE_BLOCK_OFFSET;
		src_data += FILE_BLOCK_OFFSET; 
		current_block = ffb.header.next_block;
		ffb = fb;
		
	}
}


int SimpleFS_read(FileHandle* f, void* dst_data, int size){
	//TODO: This implementation is cleaner and more elegant of the one above. Fix that.
	
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
		return read_bytes;
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
		if(DiskDriver_readBlock(f->sfs->disk, &fb, ffb.header.next_block)){
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
		/*This works because header and fcb of FirstFileBlock 
		 * and FirstDirBlock have same offsets, so simply get FileHandle
		 * by openFile() and cast it into a DirectoryHandle*/
		d = (DirectoryHandle*) &dest_handle;	
		
		//TODO: need to check for a file with same name.
		
		FirstDirectoryBlock dir;
		if(DiskDriver_readBlock(d->sfs->disk, &dir, d->dcb)!=0){
			printf("Error reading dir/file\n");
			return -1;
		}
		if(dir.fcb.is_dir==0){
			printf("This is not a dir\n");
			return -1;
		}
		return 0;
	}
	//If name is ".."
	
	//Retrieving parent of the parent dir
	FirstDirectoryBlock parent;
	
	//Checking if we are in top dir. If so, return top dir handle.
	if(d->parent_dir == 0xFFFFFFFF){
		printf("We already are top_dir!\n");
		return 0; //Do not modify handle.
	}
	
	if(DiskDriver_readBlock(d->sfs->disk, &parent, d->parent_dir)!=0){
		printf("Error retrieving parent dir!\n"); 
		
	}
	
	DirectoryHandle updir_handle; 
	updir_handle.sfs = d->sfs;
	updir_handle.dcb = d->parent_dir;
	updir_handle.parent_dir = parent.fcb.directory_block;
	updir_handle.current_block = 0;
	updir_handle.pos_in_dir = 0;
	updir_handle.pos_in_block = 0;
	
	memcpy(d, &updir_handle, sizeof(DirectoryHandle));
	
	return 0;
}


int SimpleFS_mkDir(DirectoryHandle* d, char* dirname){
	
	//I check if the dir has one or more remainders
	FirstDirectoryBlock pwd;
	
	DirectoryBlock* pwd_pointer = (DirectoryBlock*) &pwd; //Necessary for further type casts.
	
	FirstDirectoryBlock new_dir; //New dir dcb
	if(DiskDriver_readBlock(d->sfs->disk, &pwd, d->dcb)!=0){
		printf("Error reading First Directory Block\n");
		return -1;
	}
	
	//I use SimpleFS_changeDir to see if there is already a subdir with same name.
	//I dont' mind it's side effect, so I will allocate a fake handle.
	//This actually includes ".." case, which is not possible.
	DirectoryHandle fake_handle;
	
	if(SimpleFS_changeDir(&fake_handle, dirname) == 0){
		printf("Error, such directory already exists\n");
		return -1;
	}
	
	int is_rem = 0; //If the dir has at least 1 remainder block, it gets true.
	int present_block = 0xFFFFFFFF; //Useful in case of the creation of a remainder of the remainder
	
	while(pwd.header.next_block!=0xFFFFFFFF){
		present_block = pwd.header.next_block;
		//It works also on DirectoryBlock and FirstDirectoryBlock, because header offsets are the same
		//It reaches the last block used by the dir
		if(DiskDriver_readBlock(d->sfs->disk, &pwd, pwd.header.next_block)!=0){
			printf("Error reading Next Directory Block\n");
			return -1;
		}
		is_rem = 1;
	}
	
	int i, is_full = 1, free_index;
	
	//Checking if there is more space in array
	if(is_rem == 0){ //Block is a FirstDirectoryBlock		
		
		for(i=0;i<F_DIR_BLOCK_OFFSET;i++){
			if(pwd.file_blocks[i] != 0xFFFFFFFF) { //Picks the first empty pos in array
				is_full = 0;
				break;
			}
		}
		if(is_full == 0){
			//Getting first available block by bitmap
			free_index = DiskDriver_getFreeBlock(d->sfs->disk, 0);
			if(free_index == -1){
				printf("No free space!\n");
				return -1;
			}
			pwd.file_blocks[i] = free_index; //Folder was allocated on array
			
			//Compiling new dir block
			new_dir.header.previous_block = 0xFFFFFFFF;
			new_dir.header.next_block = 0xFFFFFFFF;
			new_dir.header.block_in_file = 0;
			new_dir.fcb.directory_block = d->parent_dir;
			new_dir.fcb.block_in_disk = free_index;
			strncpy(new_dir.fcb.name, dirname, 128*sizeof(char));
			new_dir.fcb.size_in_bytes = 0; //I assume dirs have no size
			new_dir.fcb.size_in_blocks = 1; //Only this block is occupied TODO: check in above implementation
			new_dir.fcb.is_dir = 1;
			new_dir.num_entries = 0; //Dir is empty
			for(i=0;i<F_DIR_BLOCK_OFFSET;i++){
				new_dir.file_blocks[i] = 0xFFFFFFFF;
			}
			
			//Now write it down!
			if(DiskDriver_writeBlock(d->sfs->disk, &new_dir, free_index)!=0){
				printf("Error writing new dir\n");
				return -1;
			}		
			return 0;		
		}
		
		else{ //Creating a new DirectoryBlock and allocating it in free_index
			DirectoryBlock dir_rem;
			
			free_index = DiskDriver_getFreeBlock(d->sfs->disk, 0);
			if (free_index == -1) {
				printf("No free space allocating new remainder block!\n");
				return -1;
			}
			
			//Compiling remainder block
			dir_rem.header.previous_block = pwd.fcb.block_in_disk;
			dir_rem.header.next_block = 0xFFFFFFFF;
			dir_rem.header.block_in_file = pwd.header.block_in_file+1;
			for(i=0;i<DIR_BLOCK_OFFSET;i++){
				dir_rem.file_blocks[i] = 0xFFFFFFFF;
			}
			
			//Writing remainder block down on the disk
			if(DiskDriver_writeBlock(d->sfs->disk, &dir_rem, free_index)!=0){
				printf("Error writing new dir\n");
				return -1;
			}
			//Recursion!
			return SimpleFS_mkDir(d, dirname);
		}	
		
	}
	else{ //Block is a DirectoryBlock use pwd_pointer instead of pwd itself.
		
		for(i=0;i<DIR_BLOCK_OFFSET;i++){
			if(pwd_pointer->file_blocks[i] != 0xFFFFFFFF) { //Picks the first empty pos in array
				is_full = 0;
				break;
			}
		}
		if(is_full == 0){
			free_index = DiskDriver_getFreeBlock(d->sfs->disk, 0);
			if(free_index == -1){
				printf("No free space!\n");
				return -1;
			}
			pwd_pointer->file_blocks[i] = free_index; //Folder was allocated on array
		
			//Compiling new dir block
			new_dir.header.previous_block = 0xFFFFFFFF; 
			new_dir.header.next_block = 0xFFFFFFFF;
			new_dir.header.block_in_file = 0;
			new_dir.fcb.directory_block = d->parent_dir;
			new_dir.fcb.block_in_disk = free_index;
			strncpy(new_dir.fcb.name, dirname, 128*sizeof(char));
			new_dir.fcb.size_in_bytes = 0; //I assume dirs have no size
			new_dir.fcb.size_in_blocks = 1; //Only this block is occupied TODO: check in above implementation
			new_dir.fcb.is_dir = 1;
			new_dir.num_entries = 0; //Dir is empty
			for(i=0;i<DIR_BLOCK_OFFSET;i++){
				new_dir.file_blocks[i] = 0xFFFFFFFF;
			}
			
			//Now write it down!
			if(DiskDriver_writeBlock(d->sfs->disk, &new_dir, free_index)!=0){
				printf("Error writing new dir\n");
				return -1;
			}		
			return 0;		
		}
		
			
		else{ //Creating a next DirectoryBlock and allocating it in free_index
			DirectoryBlock dir_rem;
			
			free_index = DiskDriver_getFreeBlock(d->sfs->disk, 0);
			if (free_index == -1) {
				printf("No free space allocating new remainder block!\n");
				return -1;
			}
			//Compiling remainder block
			dir_rem.header.previous_block = present_block;
			dir_rem.header.next_block = 0xFFFFFFFF;
			dir_rem.header.block_in_file = pwd_pointer->header.block_in_file+1;
			for(i=0;i<DIR_BLOCK_OFFSET;i++){
				dir_rem.file_blocks[i] = 0xFFFFFFFF;
			}
			
			//Writing remainder block down on the disk
			if(DiskDriver_writeBlock(d->sfs->disk, &dir_rem, free_index)!=0){
				printf("Error writing new dir\n");
				return -1;
			}
			//Recursion!
			return SimpleFS_mkDir(d, dirname);
		}		
	}
}


int SimpleFS_remove(SimpleFS* fs, int index){
	
	FirstDirectoryBlock block; //I need to read block to know what type it has
	int parent_dir_index = block.fcb.directory_block; //Needed for fixing file_blocks array on dirs.
	
	DirectoryBlock* rem_ptr = (DirectoryBlock*)&block; //For directory reminders
	
	if(DiskDriver_readBlock(fs->disk, &block, index)!=0){
		printf("Error reading file/folder Block");
		return -1;
	}
	if(block.fcb.is_dir == 1){ //If we got a dir
		
		while(block.file_blocks[0]!=0xFFFFFFFF){ //Emptying the dir before eliminating
			if(SimpleFS_remove(fs, block.file_blocks[0])!=0){
				printf("Error emptying dir\n");
				return -1;
			} 
			//It works because I'm closing-up holes made by file/sub-dir elimination
		}
		//Now dir is empty
		if(DiskDriver_freeBlock(fs->disk, index)!=0){
			printf("Error eliminating empty dir!\n!");
			return -1;	
		}	
	}
	else{ //If we got a file
		FileBlock* file_block = (FileBlock*)&block;
		
		//Iteratively fetch block chain and delete them		
		int this_block = index;
		int next_block = file_block->header.next_block;
		
		//Eliminating block and fetching next iteratively
		while(file_block->header.next_block != 0xFFFFFFFF){
			
			if(DiskDriver_freeBlock(fs->disk, this_block)!=0){
				printf("Error freeing Block");
				return -1;
			}
			
			if(DiskDriver_readBlock(fs->disk, file_block ,next_block)!=0){
				printf("Error reading next file Block");
				return -1;
			
			this_block = next_block;	
			next_block = file_block->header.next_block;			
			}
		}
		//Eliminating last remaining block
		if(DiskDriver_freeBlock(fs->disk, this_block)!=0){
				printf("Error freeing Block");
				return -1;
		}
		
		
	}
	//Filling the hole in file_blocks array in parent_dir.
	
	//Fetching parent dir
	if(DiskDriver_readBlock(fs->disk, &block, parent_dir_index)!=0){
		printf("Error reading file/folder Block");
		return -1;
	}
	
	//Scanning file_blocks array searching for "index" match
	
	//Trying in the FirstDirectoryBlock
	int i, found = 0, first_dir_block = 1;
	for(i=0;i<F_DIR_BLOCK_OFFSET;i++){
		if(block.file_blocks[i] == index){
			found = 1;
			break;
		}
	}
	if(found == 0){
		//Then trying in remainder DirectoryBlocks
		
		first_dir_block = 0;
		while(rem_ptr->header.next_block!=0xFFFFFFFF){
			for(i=0;i<DIR_BLOCK_OFFSET;i++){
				if(block.file_blocks[i] == index){
					found = 1;
					break;
				}
			}
		}
	}
	if(found == 0){
		printf("Something terrible happened!\n");
		return -1;
	}
	//Now move every next array cell back of 1 position
	
	DirectoryBlock aux_dir; //For taking last array index from next remainder
	
	if(first_dir_block == 1){
		for(;i<F_DIR_BLOCK_OFFSET-1;i++){
			block.file_blocks[i] = block.file_blocks[i+1];
		}
		//Take, if any, the last index form the next remainder
		if(block.header.next_block!=0xFFFFFFFF){
			
			if(DiskDriver_readBlock(fs->disk, &aux_dir, block.header.next_block)!=0){
				printf("Error scaling back indexes from a remainder");
				return -1;
			}
			block.file_blocks[F_DIR_BLOCK_OFFSET] = aux_dir.file_blocks[0];
			//Update parent dir
			if(DiskDriver_writeBlock(fs->disk, &aux_dir, parent_dir_index)!=0){
				printf("Error scaling back indexes from a remainder");
				return -1;
			}
		}
		else{ //If no remainder, fill with invalid value
			block.file_blocks[F_DIR_BLOCK_OFFSET] = 0xFFFFFFFF;
			
			//If a remainder block is now empty, delete!
			found = 0;
			for(i=0;i<DIR_BLOCK_OFFSET;i++){
				if(block.file_blocks[i]!=0xFFFFFFFF){
					found = 1;
					break;
				}
			}
			if(found == 0){ //Eliminate it!
				if(DiskDriver_freeBlock(fs->disk, block.header.next_block)!=0){
					printf("Error deleting dir remainder block\n");
					return -1;
				}
			}
			
			//Update parent dir
			if(DiskDriver_writeBlock(fs->disk, &aux_dir, parent_dir_index)!=0){
				printf("Error scaling back indexes from a remainder");
				return -1;
			}
			return 0; //No further operation to do here. Hooray!
		}
		
		//Now operate on remainders
		int this_block;
		DirectoryBlock dir_rem;
		
		while(block.header.next_block!=0xFFFFFFFF){
			this_block = block.header.next_block;
			if(DiskDriver_readBlock(fs->disk, &dir_rem, this_block)!=0){
				printf("Error scaling back indexes from a remainder");
				return -1;
			}
			for(i=0;i<DIR_BLOCK_OFFSET-1;i++){
				dir_rem.file_blocks[i] = dir_rem.file_blocks[i+1];
			}
			dir_rem.file_blocks[i] = 0xFFFFFFFF; //In case there is no further remainder
			
			//Take, if any, the last index form the next remainder
			if(dir_rem.header.next_block!=0xFFFFFFFF){			
				if(DiskDriver_readBlock(fs->disk, &aux_dir, dir_rem.header.next_block)!=0){
					printf("Error scaling back indexes from a remainder");
					return -1;
				}
				dir_rem.file_blocks[DIR_BLOCK_OFFSET] = aux_dir.file_blocks[0];
			}	
			//Update parent remainder
			if(DiskDriver_writeBlock(fs->disk, &dir_rem, this_block)!=0){
				printf("Error scaling back indexes from a remainder");
				return -1;
			}
		}
		//Last remainder
		for(i=0;i<DIR_BLOCK_OFFSET-1;i++){
			dir_rem.file_blocks[i] = dir_rem.file_blocks[i+1];
		}
		//Update parent remainder
		if(DiskDriver_writeBlock(fs->disk, &dir_rem, this_block)!=0){
			printf("Error scaling back indexes from a remainder");
			return -1;
		}
		
		//If a remainder block is now empty, delete!
		found = 0;
		for(i=0;i<DIR_BLOCK_OFFSET;i++){
			if(block.file_blocks[i]!=0xFFFFFFFF){
				found = 1;
				break;
			}
		}
		if(found == 0){ //Eliminate it!
			if(DiskDriver_freeBlock(fs->disk, block.header.next_block)!=0){
				printf("Error deleting dir remainder block\n");
				return -1;
			}
		}
	}
	
	//Congrats, you survived this nightmere!
		return 0;
}


int SimpleFS_checkFreeSpace(SimpleFS* fs){
	
	int i=0,res=0;
	while(i != -1){
		i = DiskDriver_getFreeBlock(fs->disk, i+1);
		if(i!=-1) res++;
	}
	printf("Number of free blocks = %d", res);
	return res;
}
