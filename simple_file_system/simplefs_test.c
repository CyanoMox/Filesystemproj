#include "simplefs.h"
#include <stdio.h>

void printDirHandle(DirectoryHandle dir_handle);
void printFileHandle(FileHandle file_handle);

void createFile_test(unsigned int num_files, DirectoryHandle dir_handle ,FileHandle file_handle);

int main(int argc, char** argv) {
	printf("FirstBlock size %ld\n", sizeof(FirstFileBlock));
	printf("DataBlock size %ld\n", sizeof(FileBlock));
	printf("FirstDirectoryBlock size %ld\n", sizeof(FirstDirectoryBlock));
	printf("DirectoryBlock size %ld\n", sizeof(DirectoryBlock));

	printf("Number of file allocable in DCB: %d\n",F_DIR_BLOCK_OFFSET);
	printf("Number of file allocable in remainder dir block: %d\n",DIR_BLOCK_OFFSET);

	printf("\n\n+++ FS TESTING BEGINS +++\n\n");
	
	//And now... the true tests!
	
	SimpleFS fs;
	DiskDriver disk;
	
	fs.disk = &disk;
	int block_number = 300;
	int file_number = 280;
	
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
	int i, res;
	while(num_files!=0){	
		for(i=0;i<128;i++)
			filename[i] = 'A' + num_files-1; //Initializing name vector
			
		//printf("DBG TEST\n");
		if(res = SimpleFS_createFile(&dir_handle , filename, &file_handle)!=0){
			printf("Error code: %d \n", res);
			exit(-1);
		}
		num_files--;
		printFileHandle(file_handle);	
		
	}
}
