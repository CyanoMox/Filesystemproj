#pragma once
#include "disk_driver.h"

/*these are structures stored on disk*/

/***IMPORTANT NOTICE: due to the implementation of DiskDriver struct,
 * I can't utilize permanent pointers, because there is no guarantee
 * that they aren't unmapped by next operations on the FS, resulting in
 * Segmentation Fault errors or miswriting and misreading that will 
 * damage the FS itself.
 * 
 * These structures are affected by this and will be reimplemented:
 * 
 * -DirectoryHandle
 * -FileHandle
 * 
 * ***/

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
  int size_in_bytes;
  int size_in_blocks;
  int is_dir;          // 0 for file, 1 for dir
} FileControlBlock;

// this is the first physical block of a file
// it has a header
// an FCB storing file infos
// and can contain some data

/******************* stuff on disk BEGIN *******************/
//this is the block that contains FCB info (first block of a file)
typedef struct {
  BlockHeader header;
  FileControlBlock fcb;
  char data[BLOCK_SIZE-sizeof(FileControlBlock) - sizeof(BlockHeader)] ;
} FirstFileBlock; 

// this is one of the next physical blocks of a file
typedef struct {
  BlockHeader header;
  char  data[BLOCK_SIZE-sizeof(BlockHeader)];
} FileBlock;

// this is the first physical block of a directory
/**In directories, the location 0 of the array indicates the position of
 * the reminder of the directory itself. If it is = 0xFFFFFFFF, dir has 
 * no reminder.
 * 
 * In the same way, if a location is 0xFFFFFFFF, it means there is no
 * fcb block linked to that dir in that location.**/
typedef struct {
  BlockHeader header;
  FileControlBlock fcb;
  int num_entries;
  unsigned int file_blocks[ (BLOCK_SIZE
		   -sizeof(BlockHeader)
		   -sizeof(FileControlBlock)
		    -sizeof(int))/sizeof(int) ];
} FirstDirectoryBlock;

// this is remainder block of a directory
typedef struct {
  BlockHeader header;
  unsigned int file_blocks[ (BLOCK_SIZE-sizeof(BlockHeader))/sizeof(int) ];
} DirectoryBlock;
/******************* stuff on disk END *******************/

  
typedef struct {
  DiskDriver* disk;
  // add more fields if needed
  int top_dir = 0;		   		   // handle to the top dir
  int current_directory_block;	   // index of the dir block currently accessed
  char* filename;				   // it contains the filename of the disk
} SimpleFS;

// this is a file handle, used to refer to open files
typedef struct {
  SimpleFS* sfs;                   // pointer to memory file system structure
  int fcb;             			   // index of the first block of the file(read it)
  int parent_dir;  				   // pointer to the directory where the file is stored
  int current_block;      		   // current block in the file
  int pos_in_file;                  // position of the cursor
} FileHandle;

typedef struct {
  SimpleFS* sfs;                   // pointer to memory file system structure
  int dcb;        				   // index of the first block of the directory(read it)
  int parent_dir;                  // index of the parent directory (-1 if top level)
  int current_block;               // current block in the directory
  int pos_in_dir;                  // absolute position of the cursor in the directory
  int pos_in_block;                // relative position of the cursor in the block
} DirectoryHandle;

/*** For design, many of the pointers in this implementation 
 * were rolled to direct variables, within the purpose of avoiding
 * memory leaking and simplyfying variable management.***/


// initializes a file system on an already made disk
// returns a handle to the top level directory stored in the first block
DirectoryHandle* SimpleFS_init(SimpleFS* fs, DiskDriver* disk);

// creates the inital structures, the top level directory
// has name "/" and its control block is in the first position
// it also clears the bitmap of occupied blocks on the disk
// the current_directory_block is cached in the SimpleFS struct
// and set to the top level directory
void SimpleFS_format(SimpleFS* fs, int block_num);

// creates an empty file in the directory d
// returns null on error (file existing, no free blocks)
// an empty file consists only of a block of type FirstBlock
FileHandle* SimpleFS_createFile(DirectoryHandle* d, const char* filename);

// reads in the (preallocated) blocks array, the name of all files in a directory 
int SimpleFS_readDir(char** names, DirectoryHandle* d);


// opens a file in the  directory d. The file should be exisiting
FileHandle* SimpleFS_openFile(DirectoryHandle* d, const char* filename);


// closes a file handle (destroyes it)
int SimpleFS_close(FileHandle* f);

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

/*** Auxiliary functions ***/

//It controls if a file exists in dir (and remainder), then returns its handle
FileHandle SimpleFS_seekFile(DirectoryHandle d, const char* filename);

FileHandle seekFile_aux(DirectoryBlock d, const char* filename, DirectoryHandle handle);


//Closes fs instance
int SimpleFS_close(SimpleFS* fs);

/* Function implementation below */

// initializes a file system on an already made disk
// returns a handle to the top level directory stored in the first block
DirectoryHandle* SimpleFS_init(SimpleFS* fs, DiskDriver* newdisk){ //TODO: eliminate pointers here
	
	//Creating DirectoryHandle
	DirectoryHandle* first_dir = (DirectoryHandle*)malloc(sizeof(DirectoryHandle));
	first_dir->sfs = fs;
	first_dir->dcb = 0;
	first_dir->parent_dir = 0xFFFFFFFF;
	first_dir->current_block = 0;
	first_dir->pos_in_dir = 0;
	first_dir->pos_in_block = 0;
	
	//Updating FS information
	fs->disk = newdisk;
	fs->top_dir = 0;
	fs->current_directory_block = 0;
	
	return first_dir;
	
void SimpleFS_format(SimpleFS* fs, int block_num){
	
	if(DiskDriver_init(fs->disk, fs->filename, block_num)!= 0){
		printf("Error formatting disk!\n");
		exit(-1);
	}
	
	//Setting top directory
	fs->current_directory_block = 0;
	
	//Not directly memorized in DirectoryHandle, used only to memorize temporarily in stack. 
	//Write down or read using DiskDriver functions.
	FirstDirectoryBlock top_dcb; 
	BlockHeader top_dcb_header;
	FileControlBlock top_dcb_fcb;
	
	//Header
	top_dcb_header.previous_block = 0xFFFFFFFF;
	top_dcb_header.next_block = 0xFFFFFFFF;
	top_dcb_header.block_in_file = 0; //This block contains a folder FCB.
	
	//FCB
	top_dcb_fcb.durectory_block = 0xFFFFFFFF;
	top_dcb_fcb.block_in_disk = 0;
	strncpy(top_dcb_fcb.name, "/", sizeof(char)); //TODO: check this.
	top_dcb_fcb.size_in_bytes = 512; //It is a folder, I consider all the block full.
	top_dcb_fcb.size_in_blocks = 1;
	top_dcb_fcb.is_dir = 1;
	
	//FirstDirectoryBlock
	top_dcb.header = top_dcb_header;
	top_dcb.fcb = top_dcb_fcb;
	top_dcb.num_entries = 0;
	int i;
	int file_blocks_len = (BLOCK_SIZE-sizeof(BlockHeader)
		-sizeof(FileControlBlock)-sizeof(int))/sizeof(int));
	for(i=0;i<file_blocks_len;i++){
		top_dcb.file_blocks[i] = 0xFFFFFFFF; //initializing file block array
	}
	
	//Writing it down!!!
	DiskDriver_writeBlock(fs->disk, &top_dcb, top_dcb_fcb.block_in_disk);
}

// creates an empty file in the directory d
// returns null on error (file existing, no free blocks)
// an empty file consists only of a block of type FirstBlock
FileHandle SimpleFS_createFile(DirectoryHandle d, const char* filename){
	
	//Return for an already existing file
	FileHandle res = SimpleFS_seekFile(DirectoryHandle d, const char* filename);
	if(res.fcb != 0xFFFFFFFF){
		res.fcb = 0xFFFFFFFF;  //It marks an invalid FileHandle (equivalent to null)
		return res;
	}
	
	//Return for an invalid reading
	if(res.fcb == 0xFFFFFFFF && res.parent == 0){
		res.fcb = 0xFFFFFFFF;  //It marks an invalid FileHandle (equivalent to null)
		return res;
	}
	
	//Here creating file
	FirstDirectoryBlock dir = d.dcb;
	int i = 1; //location 0 is reserved for file_blocks array
	int array_len = (BLOCK_SIZE-sizeof(BlockHeader)-sizeof(FileControlBlock)-sizeof(int))/sizeof(int);
	while(i<array_len){
		if(dir.file_blocks[i]==0xFFFFFFFF){
			dir.file_blocks[i] = DiskDriver_getFreeBlock(d.fs.disk, 0); //It works because -1 is read like 0xFFFFFFFF in unsigned int mode
			
			res.fcb = dir.file_blocks[i]; //If this == -1, no free file blocks available
			res.parent_dir = d.dcb;
			res.current_block = dir.file_blocks[i]; //It corresponds to fcb because it is the first block
			res.pos_in_file = 0;
			return res;
		}
	
	//TODO: check for space in remainder dir blocks, if any, allocate one!
	}
	
	
}

// reads in the (preallocated) blocks array, the name of all files in a directory 
int SimpleFS_readDir(char** names, DirectoryHandle* d){}


// opens a file in the  directory d. The file should be exisiting
FileHandle* SimpleFS_openFile(DirectoryHandle* d, const char* filename){}


// closes a file handle (destroyes it)
int SimpleFS_close(FileHandle* f){}

// writes in the file, at current position for size bytes stored in data
// overwriting and allocating new space if necessary
// returns the number of bytes written
int SimpleFS_write(FileHandle* f, void* data, int size){}

// writes in the file, at current position size bytes stored in data
// overwriting and allocating new space if necessary
// returns the number of bytes read
int SimpleFS_read(FileHandle* f, void* data, int size){}

// returns the number of bytes read (moving the current pointer to pos)
// returns pos on success
// -1 on error (file too short)
int SimpleFS_seek(FileHandle* f, int pos){}

// seeks for a directory in d. If dirname is equal to ".." it goes one level up
// 0 on success, negative value on error
// it does side effect on the provided handle
 int SimpleFS_changeDir(DirectoryHandle* d, char* dirname){}

// creates a new directory in the current one (stored in fs->current_directory_block)
// 0 on success
// -1 on error
int SimpleFS_mkDir(DirectoryHandle* d, char* dirname){}

// removes the file in the current directory
// returns -1 on failure 0 on success
// if a directory, it removes recursively all contained files
int SimpleFS_remove(SimpleFS* fs, char* filename){}




FileHandle SimpleFS_seekFile(DirectoryHandle d, const char* filename){
	
	int file_blocks_len = (BLOCK_SIZE-sizeof(BlockHeader)
		-sizeof(FileControlBlock)-sizeof(int))/sizeof(int));
		
	FileHandle invalid_FileHandle_badIndex; //Return for invalid index error
	invalid_file_handle.fcb = 0xFFFFFFFF;
	invalid_file_handle.parent_dir = 0;
	
	FileHandle invalid_FileHandle_noFile; //Return for inexistent file error
	invalid_file_handle.fcb = 0xFFFFFFFF;
	invalid_file_handle.parent_dir = 0xFFFFFFFF;
	
	//reading folder block
	FirstDirectoryBlock pwd;
	int res = DiskDriver_readBlock(fs->disk, &pwd, d.dcb); //TODO: is &pwd correct?
	if (res==-2){
		printf("Invalid Block Num\n");
		return invalid_FileHandle_badIndex;
	}
	if (res==-1){
		printf("Empty Block\n");
		return invalid_FileHandle_badIndex;
	}
	
	int i=1; //0 is a location reserved to remainder folder (if any)
	FirstFileBlock ffb;
	
	//checking for file existence in first dir block
	while(i<pwd.num_entries &&i<file_blocks_len){ //TODO: file_blocks array has to be contiguous.
			res = DiskDriver_readBlock(fs->disk, &ffb, pwd.file_blocks[i]);
		if (res==-2){
			printf("Invalid Block Num\n");
			return invalid_FileHandle_badIndex;
		}
		if (res==-1){
			printf("Empty Block\n");
			return invalid_FileHandle_badIndex;
		}
		if(memcmp(ffb.fcb.name, filename, sizeof(char)*128)==0){ //this only works if filename is created by strncpy
			printf("File exists!\n");
			FileHandle file;
			file.sfs = d.sfs;
			file.fcb = ffb;
			file.parent_dir = d.dcb;
			file.current_block = pwd.file_blocks[i];
			file.pos_in_file = 0;
			return file;
		}
		i++;
	}
	
	//checking eventual remainder folder blocks
	if(pwd.file_blocks[0]==0xFFFFFFFF){
		printf("File doesn't exist!\n");
		return invalid_FileHandle_noFile;
	}
	else return seekFile_aux(pwd.file_blocks[i], filename);
}


FileHandle seekFile_aux(DirectoryBlock d, filename, DirectoryHandle handle){
	int file_blocks_len = (BLOCK_SIZE-sizeof(BlockHeader))/sizeof(int);
	
	//Reading file array
	int i=1; //pos 0 is reserved for eventual further remainder
	int array_len = (BLOCK_SIZE-sizeof(BlockHeader))/sizeof(int);
	
	FileHandle invalid_FileHandle_badIndex; //Return for invalid index error
	invalid_file_handle.fcb = 0xFFFFFFFF;
	invalid_file_handle.parent_dir = 0;
	
	FileHandle invalid_FileHandle_noFile; //Return for inexistent file error
	invalid_file_handle.fcb = 0xFFFFFFFF;
	invalid_file_handle.parent_dir = 0xFFFFFFFF;
	
	FirstFileBlock ffb;
	while(i<array_len){ //I don't check again num_entries for simplicity. I assume that blocks are contiguous in the array.
		res = DiskDriver_readBlock(fs->disk, &ffb, d.file_blocks[i]);
		if (res==-2){
			printf("Invalid Block Num\n");
			return invalid_FileHandle_badIndex;
		}
		if (res==-1){
			printf("Empty Block\n");
			return invalid_FileHandle_badIndex;
		}
		if(memcmp(ffb.fcb.name, filename, sizeof(char)*128)==0){ //this only works if filename is created by strncpy
			printf("File exists!\n");
			FileHandle file;
			file.sfs = handle.sfs;
			file.fcb = ffb;
			file.parent_dir = handle.dcb;
			file.current_block = d.file_blocks[i];
			file.pos_in_file = 0;
			return file;
		}
		i++;
	}
	//checking eventual remainder folder blocks
	if(pwd.file_blocks[0]==0xFFFFFFFF){
		printf("File doesn't exist!\n");
		return invalid_FileHandle_noFile;
	}
	else return seekFile_aux(d.file_blocks[i], filename, handle);
}
