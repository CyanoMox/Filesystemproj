#include "disk_driver.h"
#include "simplefs.h"
#include <stdlib.h>


//This executable will implement high-level operations, like fs browsing and format and a shell.

int newdisk(SimpleFS* fs);
int resume(SimpleFS* fs);
void printDiskStatus(DiskDriver* disk);
int disk_control(SimpleFS* fs, DirectoryHandle root, DirectoryHandle* pwd_handle);

int main(){
	int bad_choice = 1;
	char choice;
	SimpleFS fs;
	DiskDriver disk;	
	fs.disk = &disk;
	DirectoryHandle root;
	
	printf("+++ Welcome to SFS! +++\n");
	printf("\nSelect an option:\n\n");
	printf("1) Create new disk\n");
	printf("2) Open disk\n\n");
	
	printf("Your choice is: ");
	fflush(stdout);
	scanf("%c", &choice);
	printf("\n");
	
	if(strncmp(&choice, "1", sizeof(char)) == 0) {
		newdisk(&fs);
		bad_choice = 0;
	}
	
	if(strncmp(&choice, "2", sizeof(char)) == 0) {
		resume(&fs);
		bad_choice = 0;
	}
	
	if(bad_choice != 0){
		printf("Invalid choice!\n");
		return -1;
	}
	
	SimpleFS_init(&fs, &root);
	fs.current_directory_block = 0;
	
	//Once disk is ready...
	int res = 0;
	DirectoryHandle pwd_handle = root;
	
	while(1){ 
		res = disk_control(&fs, root, &pwd_handle);
		
		//Code 1 is exit, code -2 is serious error
		if(res == 1) break;
		if(res == -2) break;
	}
	
	return 0;
}

int newdisk(SimpleFS* fs){
	char diskname[128];
	int size;
	printf("Choose a name for the disk: ");
	scanf("%s", diskname);
	printf("\n");
	
	printf("Choose a size for the disk: ");
	scanf("%d", &size);
	printf("\n");
	
	if(SimpleFS_format(fs, diskname, size) != 0){
		exit(-1);
	}
	
	strncpy(fs->diskname, diskname, 128*sizeof(char));
	
	return 0;
}


int resume(SimpleFS* fs){
	char diskname[128];
	printf("Insert disk name: ");
	scanf("%s", diskname);
	printf("\n");
	
	if(DiskDriver_resume(fs->disk, diskname) != 0){
		exit(-1);
	}
	
	printDiskStatus(fs->disk);
	
	strncpy(fs->diskname, diskname, 128*sizeof(char));
	
	return 0;
}


//Imported and modified from lowlevel_test.c
void printDiskStatus(DiskDriver* disk){
	
	printf("\nNumber of total Blocks = %d\n", disk->num_entries);
	printf("Number of Free Blocks = %d\n", disk->free_blocks);
	printf("First Block Offset (concerning bitmap) = %d\n", disk->first_block_offset);
}

int disk_control(SimpleFS* fs, DirectoryHandle root, DirectoryHandle* pwd_handle){
	int bad_choice = 1;
	char choice = 0;
	FirstDirectoryBlock pwd;	
	FileHandle file_handle;
	
	//Initializing filename array
	char filename[128];
	int i;
	for(i=0;i<128;i++) filename[i] = '\0'; 
	
	//printf("DBG current index: %ud\n", fs.current_directory_block);
	if(DiskDriver_readBlock(fs->disk, &pwd, fs->current_directory_block) !=0 ){
		printf("<Shell> Error reading present working directory\n");
		return -2;
	}
	
	printf("\n\nDisk is ready, select an option:\n\n");
	printf("Present working directory: %s\n", pwd.fcb.name);
	printf("1) Create File\n");
	printf("2) Read Dir\n");
	printf("3) Read File\n");
	printf("4) Write File\n");
	printf("5) Make a new directory\n");
	printf("6) Change present directory\n");
	printf("7) Remove file or directory\n");
	
	printf("\nq) Close disk and Exit\n");
	printf("r) Return to root\n");
	printf("x) Format this disk\n");
	
	scanf("%c", &choice); //for flushing purposes
	
	printf("Your choice is: ");
	fflush(stdout);
	scanf("%c", &choice);
	printf("\n");
	
	//Clear screen!
	printf("\e[1;1H\e[2J"); 
	
	if(strncmp(&choice, "1", sizeof(char)) == 0) {
		//Create file function
		printf("Insert filename: ");
		scanf("%s", filename);
		printf("\n");
		
		SimpleFS_createFile(pwd_handle, filename, &file_handle);
		/**I will not directly use file_handle for this implementation.
		 * It is mostly used by tests in simplefs_test.c
		 * When I have to use a file, I will use openFile and retrieve
		 * its handle directly that way instead.
		 * **/
		
		bad_choice = 0;
	}
	
	if(strncmp(&choice, "2", sizeof(char)) == 0) {
		//Read dir function
		/**This function was imported and modified by readDir_test()
		 * in simplefs_test.c **/
		
		FirstDirectoryBlock temp;
		if(DiskDriver_readBlock(fs->disk, &temp, pwd_handle->dcb)!=0){
			printf("<Shell> Error loading dir in memory\n");
			exit(-1);
		}
		
		printf("Current dir: %s\n", temp.fcb.name);
		
		int num_files = temp.num_entries;
		//Checking for same filename
		char names[num_files][128]; //Allocating name matrix
		//we have num_entries sub-vectors by 128 bytes
		
		SimpleFS_readDir((char*)names, pwd_handle); 
		
		int i;
		char printable[128];
		
		
		for(i=0;i<num_files;i++){
			strncpy(printable, names[i], 128*sizeof(char));
			printf("%d) %s\n", i+1 ,printable);
		}
	
		bad_choice = 0;
	}
	
	if(strncmp(&choice, "3", sizeof(char)) == 0) {
		//Read file function
		printf("Insert filename: ");
		scanf("%s", filename);
		printf("\n");
		
		FirstFileBlock temp;
		
		SimpleFS_openFile(pwd_handle, filename, &file_handle);
		
		if(DiskDriver_readBlock(fs->disk, &temp, file_handle.fcb) != 0){
			printf("<Shell> Error reading fcb\n");
			return -1;
		}
		
		int read_size;
		
		printf("Insert read size (-1 for *all*): ");
		scanf("%d", &read_size);
		printf("\n");
		
		if(read_size == -1) read_size = temp.fcb.size_in_bytes;
		if(read_size > temp.fcb.size_in_bytes) 
			read_size = temp.fcb.size_in_bytes;
		
		//Creating output file
		int res = open("read_output.hex", O_CREAT | O_TRUNC| O_RDWR, 0777);
		if(ftruncate(res, read_size) != 0){
			printf("<Shell> Error truncating output file\n");
			return -1;
		}
		char* dst = (char*)mmap(NULL, read_size, PROT_READ|PROT_WRITE,
		 MAP_SHARED, res, 0); //mmapping output file
	
		SimpleFS_read(&file_handle, dst, read_size);
	
		munmap(dst, read_size);	
		close(res);
		printf("***Data was written in read_output.hex file***\n");
		
		bad_choice = 0;
	}
	
	if(strncmp(&choice, "4", sizeof(char)) == 0) {
		//Write file function
		printf("Insert filename: ");
		scanf("%s", filename);
		printf("\n");
		
		FirstFileBlock temp;
		
		SimpleFS_openFile(pwd_handle, filename, &file_handle);

		//Getting input length
		FILE* file = fopen("write_input.hex", "r");
		fseek(file, 0L, SEEK_END);
		int write_size = ftell(file);
		fseek(file, 0L, SEEK_SET);
		fclose(file);
		
		printf("Total write size: %d\n", write_size);

		//Mmapping input file
		int res = open("write_input.hex",  O_RDWR);
		if(res == -1){
			printf("Error reading input file!\nFile named write_input.hex must exist.\n");
			return -1;
		}
		char* src = (char*)mmap(NULL, write_size, PROT_READ|PROT_WRITE,
		 MAP_SHARED, res, 0); 
			
		SimpleFS_write(&file_handle, src, write_size);
		munmap(src, write_size);
		close(res);
		printf("write_input.hex read succesfully\n");
		
		bad_choice = 0;
	}
	
	if(strncmp(&choice, "5", sizeof(char)) == 0) {
		//MkDir function
		printf("Insert dirname: ");
		scanf("%s", filename);
		printf("\n");
		
		if(SimpleFS_mkDir(pwd_handle, filename) != 0){
			printf("<Shell> Error creating a new directory\n");
			return -1;
		}
		
		printf("The directory was created succesfully.");
		
		fs->current_directory_block = pwd_handle->dcb;
		
		bad_choice = 0;
	}
	
	if(strncmp(&choice, "6", sizeof(char)) == 0) {
		//Cd function
		printf("Insert dirname: ");
		scanf("%s", filename);
		printf("\n");
		
		if(SimpleFS_changeDir(pwd_handle, filename) != 0){
			printf("<Shell> Error opening directory\n");
			return -1;
		}
		
		fs->current_directory_block = pwd_handle->dcb;
		
		bad_choice = 0;
	}
	
	if(strncmp(&choice, "7", sizeof(char)) == 0) {
		//Remove function
		
		printf("Insert file or dir name: ");
		scanf("%s", filename);
		printf("\n");
		
		FileHandle deletion_handle;
		
		if(SimpleFS_openFile(pwd_handle, filename, &deletion_handle) !=0 ){
			printf("<Shell> Error finding file/dir to eliminate\n");
			return -1;
		}
		
		if(SimpleFS_remove(&deletion_handle) != 0){
			printf("<Shell> Error opening directory\n");
			return -1;
		}
		else printf("<Shell> Item deleted succesfully\n");
		
		bad_choice = 0;
	}
	
	if(strncmp(&choice, "q", sizeof(char)) == 0) {
		return 1; //Quit from loop
	}
	
	if(strncmp(&choice, "r", sizeof(char)) == 0) {
		//Return to root
		fs->current_directory_block = 0;
		*pwd_handle = root;
		bad_choice = 0;
	}
	
	if(strncmp(&choice, "x", sizeof(char)) == 0) {
		//Format function
		printf("Are you sure? Data will be lost!\n");
		printf("Write *yes* to continue, otherwise operation will abort.\n");
		scanf("%s", filename);
		printf("\n");
		
		if(strncmp(filename, "yes", sizeof(char)*3) != 0) return 0;
		
		printf("Diskname: %s\n", fs->diskname);
		
		if(SimpleFS_format(fs, fs->diskname, fs->disk->num_entries) != 0){
			printf("A disaster happened!\n");
			exit(-1);
		}
		
		printf("\n<Shell> Disk formatted, please restart the shell!\n");
		
		return 1;
	}
	
	if(bad_choice != 0){
		printf("\nInvalid choice!\n");
		return -1;
	}
	
	return 0;
}
