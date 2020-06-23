#pragma once
#include "disk_driver.h"

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
  char data[F_FILE_BLOCK_OFFSET];
} FirstFileBlock;

// this is one of the next physical blocks of a file
typedef struct {
  BlockHeader header;
  char  data[FILE_BLOCK_OFFSET];
} FileBlock;

// this is the first physical block of a directory
typedef struct {
  BlockHeader header;
  FileControlBlock fcb;
  int num_entries;
  int file_blocks[ (F_DIR_BLOCK_OFFSET];
} FirstDirectoryBlock;

// this is remainder block of a directory
typedef struct {
  BlockHeader header;
  int file_blocks[ (DIR_BLOCK_OFFSET];
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
  int current_directory_block;	  // index of the dir block currently accessed
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
  unsigned int parent_dir;  		 // index of the parent directory (null if top level)
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
 
// initializes a file system on an already made disk
// returns for side effect a handle to the top level directory 
// stored in the first block. No return status is needed here
void SimpleFS_init(SimpleFS* fs, DiskDriver* disk, DirectoryHandle* dest_handle);

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
// returns for side effect the FileHandle, if no error
// an empty file consists only of a block of type FirstBlock
int SimpleFS_createFile(DirectoryHandle* d, const char* filename, FileHandle* dest_handle);

// reads in the (preallocated) blocks array, the name of all files in a directory 
int SimpleFS_readDir(char** names, DirectoryHandle* d);

// opens a file in the  directory d. The file should be exisiting
int SimpleFS_openFile(DirectoryHandle* d, const char* filename, FileHandle* dest_handle);

// closes a file handle (destroyes it)
//int SimpleFS_close(FileHandle* f);
//No more needed!

// writes in the file, at current position for size bytes stored in data
// overwriting and allocating new space if necessary
// returns the number of bytes written
int SimpleFS_write(FileHandle* f, void* data, int size);

// writes in the file, at current position size bytes stored in data
// overwriting and allocating new space if necessary
// returns the number of bytes read
int SimpleFS_read(FileHandle* f, void* data, int size);

// returns the number of bytes read (moving the current pointer to pos)
// returns pos on success
// -1 on error (file too short)
int SimpleFS_seek(FileHandle* f, int pos);

// seeks for a directory in d. If dirname is equal to ".." it goes one level up
// 0 on success, negative value on error
// it does side effect on the provided handle
int SimpleFS_changeDir(DirectoryHandle* d, char* dirname);

// creates a new directory in the current one (stored in fs->current_directory_block)
// 0 on success
// -1 on error
int SimpleFS_mkDir(DirectoryHandle* d, char* dirname);

// removes the file in the current directory
// returns -1 on failure 0 on success
// if a directory, it removes recursively all contained files
int SimpleFS_remove(SimpleFS* fs, char* filename);


/*** Function implementation ***/
void SimpleFS_init(SimpleFS* fs, DiskDriver* disk, DirectoryHandle* dest_handle){
	
	dest_handle->sfs = fs;
	dest_handle->dcb = 0;
	dest_handle->parent_dir = 0xFFFFFFFF; //invalid block index, because we are top level
	dest_handle->current_block = 0;
	dest_handle->pos_in_dir = 0;
	dest_handle->pos_in_block = 0;

}


// creates the inital structures, the top level directory
// has name "/" and its control block is in the first position
// it also clears the bitmap of occupied blocks on the disk
// the current_directory_block is cached in the SimpleFS struct
// and set to the top level directory
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


// creates an empty file in the directory d
// returns -1 on error (file existing)
// returns -2 on error (no free blocks)
// returns -3 on error (generic reading or writing error)
// returns for side effect the FileHandle, if no error
// an empty file consists only of a block of type FirstBlock
int SimpleFS_createFile(DirectoryHandle* d, const char* filename, FileHandle* dest_handle){
	
	FirstDirectoryBlock pwd_dcb;
	
	int res = DiskDriver_readBlock(d->sfs->disk, &pwd_dcb, d->dcb);
	if(res!=0) {
		printf("Error reading first dir block\n");
		return -3;
	}	

	char names[pwd_dcb.num_entries][128]; //Allocating name matrix
	//we have num_entries sub-vectors by 128 bytes
	
	if(SimpleFS_readDir(names, d)==-1){
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
	
	res = DiskDriver_getFreeBlock(d->sfs->disk, 0); //retrieving first free block available
	if(res==-1) {
		printf("No free block available!\n");
		return -2;
	}
	
	//Creating FirstFileBlock
	FirstFileBlock ffb;
	ffb.header.previous_block = 0xFFFFFFFF; //First block
	ffb.header.next_block = 0xFFFFFFFF; //Not allocated yet - last block
	ffb.header.block_in_file = 0; //First block
	ffb.fcb.directory_block = pwd_dcb.block_in_disk; //Parent dir
	ffb.fcb.block_in_disk = res;
	strncpy(ffb.fcb.name, filename , 128*sizeof(char)); //Setting name
	ffb.fcb.size_in_bytes = 0; //File is size 0
	ffb.fcb.size_in_blocks = 1; //Only first block
	ffb.fcb.is_dir = 0; //No, it's a file.
	
	for(i=0;i<F_FILE_BLOCK_OFFSET;i++){
		ffb.data[i] = 0; //Initializing data field
	}
	
	//Writing FirstFileBlock down at res position
	res = DiskDriver_writeBlock(d->sfs->disk, &ffb, res);
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

// reads in the (preallocated) blocks array, the name of all files in a directory 
int SimpleFS_readDir(char** names, DirectoryHandle* d){
	
	
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
			
			strncpy(names[j],temp_ffb.fcb.name,128*sizeof(char)); //I'm assuming char names[pwd_dcb.num_entries][128]
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
		DirectoryBlock pwd_rem,
		
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
			
				strncpy(names[j],temp_ffb.fcb.name,128*sizeof(char)); //I'm assuming char names[pwd_dcb.num_entries][128]
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
	while(remaining_files!=0)
}


// opens a file in the  directory d. The file should be exisiting
int SimpleFS_openFile(DirectoryHandle* d, const char* filename, FileHandle* dest_handle){
	
	FirstDirectoryBlock pwd_dcb;
	
	int res = DiskDriver_readBlock(d->sfs->disk, &pwd_dcb, d->dcb);
	if(res!=0) {
		printf("Error reading first dir block\n");
		return -1;
	}
	char names[pwd_dcb.num_entries][128]; //Allocating name matrix
	//we have num_entries sub-vectors by 128 bytes
	if(SimpleFS_readDir(names, d)==-1) return -1;
	
	
	//now retrieving FirstFileBlock index
	int array_num; //This value will record the position in the array of the filename itself. From that, it's easy to retrieve the fileindex
	for(i=0;i<pwd_dcb.num_entries;i++){
		if(strcmp(names[i],filename, 128*sizeof(char))==0){
			array_num = i;
			//if filename is in the first block
			if (array_num<F_DIR_BLOCK_OFFSET){
					dest_handle->sfs = d->sfs;
					dest_handle->fcb = pwd_dcb.file_blocks[i];
					dest_handle->parent_dir = pwd_dcb.dcb;
					dest_handle->current_block = 0;
					dest_handle->pos_in_file = 0;
					
					return 0;
			}
			//else
			array_num -= F_DIR_BLOCK_OFFSET; //excluding first dir block
			array_num = array_num/DIR_BLOCK_OFFSET; //this is the remainder block in the LL that contains the file
			int offset = array_num % DIR_BLOCK_OFFSET; //this is the offset to obtain the file in that block
			
			//putting in RAM that reminder block
			DirectoryBlock pwd_rem;
			//passing explicitly from FirstDirectoryBlock to DirectoryBlock
			if(DiskDriver_readBlock(pwd_dcb.sfs->disk, &pwd_rem, pwd_dcb.header.next_block)!=0)
				return -1;
			//getting right DirBlock trough iterations
			for(i=0;i<array_num;i++){
				if(DiskDriver_readBlock(pwd_rem.sfs->disk, &pwd_rem, pwd_rem.header.next_block)!=0)
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
	}
	//if no filename match
	printf("File not found\n");
	return -1;
}
