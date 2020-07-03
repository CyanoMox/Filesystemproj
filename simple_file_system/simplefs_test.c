#include "simplefs.h"
#include <stdio.h>

void printDirHandle(DirectoryHandle dir_handle);
void printFileHandle(FileHandle file_handle);

//Designed for testing file and dir remainders allocation
void createFile_test(unsigned int num_files, DirectoryHandle dir_handle ,FileHandle file_handle);
void readDir_test(int num_files, DirectoryHandle dir_handle);
void write_test(FileHandle file_handle, int num_bytes, char* symbol);

int main(int argc, char** argv) {
	printf("FirstBlock size %ld\n", sizeof(FirstFileBlock));
	printf("DataBlock size %ld\n", sizeof(FileBlock));
	printf("FirstDirectoryBlock size %ld\n", sizeof(FirstDirectoryBlock));
	printf("DirectoryBlock size %ld\n", sizeof(DirectoryBlock));

	printf("Number of file allocable in DCB: %d\n",F_DIR_BLOCK_OFFSET);
	printf("Number of file allocable in remainder dir block: %d\n",DIR_BLOCK_OFFSET);

	//return 0;
	printf("\n\n+++ FS TESTING BEGINS +++\n\n");
	
	//And now... the true tests!
	
	SimpleFS fs;
	DiskDriver disk;
	
	fs.disk = &disk;
	int block_number = 3000;
	int file_number = 400;
	
	if (SimpleFS_format(&fs, "SFS_HDD.hex", block_number)!=0){
		printf("Error formatting disk!\n");
		exit(-1);
	}

	//Obtaining top dir
	DirectoryHandle root;
	SimpleFS_init(&fs, &root);
	printDirHandle(root);
	
	//Creating a file
	char filename[128];
	int i;
	for(i=0;i<128;i++){
		filename[i] = '\0'; //Initializing name vector
	}
	
	//printf("Insert filename [max 128 char]: ");
	//scanf("%s", filename);
	
	FileHandle file_handle;
	createFile_test(file_number, root, file_handle);
	/*int res;
	if(res = SimpleFS_createFile(&root, filename, &file_handle)!=0){
		printf("Error code: %d \n", res);
		exit(-1);
	}

	printFileHandle(file_handle);*/
	
	readDir_test(file_number, root);
	
	//Open File test
	SimpleFS_openFile(&root, "AA", &file_handle);
	printFileHandle(file_handle);
	
	
	//Write File test on opened file
	write_test(file_handle, 18000, "!");
	write_test(file_handle, 1001, "?");
	return 0;
}


void printDirHandle(DirectoryHandle dir_handle){
	printf("\n\nDirectory handle descriprion:\n");
	printf("Directory FCB index: %d\n", dir_handle.dcb);
	printf("Parent directory FCB index: %d\n", dir_handle.parent_dir);
	printf("Directory Current block index: %d\n", dir_handle.current_block);
	printf("Current position in Directory index: %d\n", dir_handle.pos_in_dir);
	printf("Current position in block: %d\n\n", dir_handle.pos_in_block);
}


void printFileHandle(FileHandle file_handle){
	printf("\n\nFile handle descriprion:\n");
	printf("File FCB index: %d\n", file_handle.fcb);
	printf("Parent Directory FCB index: %d\n", file_handle.parent_dir);
	printf("File Current block index: %d\n", file_handle.current_block);
	printf("Current position in File: %d\n\n", file_handle.pos_in_file);
}


void createFile_test(unsigned int num_files, DirectoryHandle dir_handle ,FileHandle file_handle){
	char filename[128];
	int i, j='A', k='A', res;
	while(num_files!=0){	
		
		for(i=0;i<2;i+=2){
			filename[i] = k;
			filename[i+1] = j;
			j++;		
		}
		if(j=='Z'+1){
				k++;
				j='A';
			}
		
		//printf("DBG TEST\n");
		if(res = SimpleFS_createFile(&dir_handle , filename, &file_handle)!=0){
			printf("Error code: %d \n", res);
			exit(-1);
		}
		num_files--;
		printFileHandle(file_handle);	
		
	}
}


void readDir_test(int num_files, DirectoryHandle dir_handle){
	
	//Checking for same filename
	char names[num_files][128]; //Allocating name matrix
	//we have num_entries sub-vectors by 128 bytes
	
	SimpleFS_readDir((char*)names, &dir_handle); //TODO: verify problem of char*[128]
	
	int i;
	char printable[128];
	
	
	for(i=0;i<num_files;i++){
		strncpy(printable, names[i], 128*sizeof(char));
		printf("%d) %s\n", i+1 ,printable);
	}
	
}


void write_test(FileHandle file_handle, int num_bytes, char* symbol){
	int i, dim = F_FILE_BLOCK_OFFSET+num_bytes;
	char src[dim];
	for(i=0;i<dim;i++){
		src[i] = *symbol; // ! == 33
	}
	
	int res = SimpleFS_write(&file_handle, src, dim);
	printf("Bytes written: %d\n", res);
}
