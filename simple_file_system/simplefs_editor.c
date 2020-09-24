#ifndef _MYHEADER
#define _MYHEADER
#include "disk_driver.h"
#endif

#include "simplefs.h"
#include <stdlib.h>


/*This executable will implement high-level operations, 
 *like fs browsing and format and a shell.
 */

int newdisk(SimpleFS* fs);
int resume(SimpleFS* fs);
void printDiskStatus(DiskDriver* disk);
int disk_control(SimpleFS* fs, DirectoryHandle root,
									DirectoryHandle* pwd_handle);

int main(){
	int bad_choice = 1;
	char choice;
	SimpleFS fs;
	DiskDriver disk;	
	fs.disk = &disk;
	DirectoryHandle root;
	
	printf("FirstBlock size %ld\n", sizeof(FirstFileBlock));
	printf("DataBlock size %ld\n", sizeof(FileBlock));
	printf("FirstDirectoryBlock size %ld\n", 
										sizeof(FirstDirectoryBlock));
	printf("DirectoryBlock size %ld\n", sizeof(DirectoryBlock));

	printf("Number of file allocable in DCB: %d\n",F_DIR_BLOCK_OFFSET);
	printf("Number of file allocable in remainder dir block: %d\n",
													DIR_BLOCK_OFFSET);
	
	printf("\n\n+++ Welcome to SFS! +++\n");
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
		printf("<Shell> Invalid choice!\n");
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
	printf("<Shell> Choose a name for the disk: ");
	scanf("%s", diskname);
	printf("\n");
	
	printf("<Shell> Choose a size for the disk: ");
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
	printf("<Shell> Insert disk name: ");
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
	printf("First Block Offset (concerning bitmap) = %d\n", 
												disk->first_block_offset);
}

int disk_control(SimpleFS* fs, DirectoryHandle root, 
										DirectoryHandle* pwd_handle){
	int bad_choice = 1;
	char choice = 0;
	FirstDirectoryBlock pwd;	
	FileHandle file_handle;
	
	//Initializing filename array
	char filename[128];
	int i;
	for(i=0;i<128;i++) filename[i] = '\0'; 
	
	if(DiskDriver_readBlock(fs->disk, &pwd, 
									fs->current_directory_block) !=0 ){
		printf("<Shell> Error reading present working directory\n");
		return -2;
	}
	
	printf("\n\n<Shell> Disk is ready, select an option:\n\n");
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
	
	printDiskStatus(fs->disk);
	printf("\n");
	
	if(strncmp(&choice, "1", sizeof(char)) == 0) {
		//Create file function
		printf("<Shell> Insert filename: ");
		scanf("%s", filename);
		printf("\n");
		
		//Special test function!
		if (strcmp(filename, "fill_test")==0){
			
			int num_files;
			
			printf("<Shell> *** Special Test activated! ***\n");
			printf("<Shell> Insert number of files to create!\n");
			scanf("%d", &num_files);
			printf("\n");
			
			FileHandle test_file_handle;
			
			char test_filename[128];
			int i, j='A', k='A', res;
		
			for(i=0;i<128;i++){
				test_filename[i] = 0;
			}
		
			while(num_files!=0){	
				
				for(i=0;i<2;i+=2){
					test_filename[i] = k;
					test_filename[i+1] = j;
					j++;		
				}
				if(j=='z'+1){
						k++;
						j='A';
					}

				if(res = SimpleFS_createFile(pwd_handle , test_filename, 
												&test_file_handle) != 0){
					printf("Error code: %d \n", res);
					exit(-1);
				}
				num_files--;				
			}		
		}
		//Normal beheaviour
		else SimpleFS_createFile(pwd_handle, filename, &file_handle);
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
		if(DiskDriver_readBlock(fs->disk, &temp, pwd_handle->dcb) != 0){
			printf("<Shell> Error loading dir in memory\n");
			exit(-1);
		}
		
		printf("Current dir: %s\n", temp.fcb.name);
		int num_files = temp.num_entries;			
		//Checking for same filename
		
		char names[num_files][128]; //Allocating name matrix
		//we have num_entries sub-vectors by 128 bytes
		
		if(SimpleFS_readDir(*names, pwd_handle) != 0){
			printf("<Shell> Error reading dir\n");
		} 
		
		int i;
		char printable[128];
		FileHandle dest_handle;
		
		
		for(i=0;i<num_files;i++){
			SimpleFS_openFile(pwd_handle, names[i], &dest_handle);
			DiskDriver_readBlock(fs->disk, &temp, dest_handle.fcb);
			
			//Print names
			memcpy(printable, names[i], 128*sizeof(char));
			printf("%d) %s", i+1 ,printable);
			if(temp.fcb.is_dir == 1) printf(" <DIR>\n");
			else printf("  <file>\n");
		}
	
		bad_choice = 0;
	}
	
	
	if(strncmp(&choice, "3", sizeof(char)) == 0) {
		//Read file function
		printf("<Shell> Insert filename: ");
		scanf("%s", filename);
		printf("\n");
		
		//File or folder check
		FirstFileBlock temp_check;
		FileHandle dest_handle;
		
		SimpleFS_openFile(pwd_handle, filename, &dest_handle);
		DiskDriver_readBlock(fs->disk, &temp_check, dest_handle.fcb);
			
		if(temp_check.fcb.is_dir == 1) {
			printf("<Shell> Not a file\n");
			return -1;
		}
		
		FirstFileBlock temp;
		
		if(SimpleFS_openFile(pwd_handle, filename, &file_handle) != 0)
			return -1;
		
		if(DiskDriver_readBlock(fs->disk, &temp, file_handle.fcb) != 0){
			printf("<Shell> Error reading fcb\n");
			return -1;
		}
		
		int read_size;
		
		printf("<Shell> Insert read size (-1 for *all*): ");
		scanf("%d", &read_size);
		printf("\n");
		
		if(read_size == -1) read_size = temp.fcb.size_in_bytes;
		if(read_size > temp.fcb.size_in_bytes) 
			read_size = temp.fcb.size_in_bytes;
		
		//Creating output file
		int res = open("read_output.hex", O_CREAT | O_TRUNC| 
														O_RDWR, 0777);
		if(ftruncate(res, read_size) != 0){
			printf("<Shell> Error truncating output file\n");
			return -1;
		}
		char* dst = (char*)mmap(NULL, read_size, PROT_READ|PROT_WRITE,
		 MAP_SHARED, res, 0); //mmapping output file
	
		SimpleFS_read(&file_handle, dst, read_size);
	
		munmap(dst, read_size);	
		close(res);
		printf("\n<Shell> ***Data was written in read_output.hex file***\n");
		
		bad_choice = 0;
	}
	
	if(strncmp(&choice, "4", sizeof(char)) == 0) {
		//Write file function
		printf("<Shell> Insert filename: ");
		scanf("%s", filename);
		printf("\n");
		
		//File or folder check
		FirstFileBlock temp_check;
		FileHandle dest_handle;
		
		SimpleFS_openFile(pwd_handle, filename, &dest_handle);
		DiskDriver_readBlock(fs->disk, &temp_check, dest_handle.fcb);
			
		if(temp_check.fcb.is_dir == 1) {
			printf("<Shell> Not a file\n");
			return -1;
		}
		
		//Opening file
		FirstFileBlock temp;
		
		if(SimpleFS_openFile(pwd_handle, filename, &file_handle) != 0)
			return -1;

		//Getting input length
		FILE* file = fopen("write_input.hex", "r");
		fseek(file, 0L, SEEK_END);
		int write_size = ftell(file);
		fseek(file, 0L, SEEK_SET);
		fclose(file);
		
		printf("<Shell> Total write size: %d\n", write_size);

		//Mmapping input file
		int res = open("write_input.hex",  O_RDWR);
		if(res == -1){
			printf("<Shell> Error reading input file!\nFile \
named write_input.hex must exist.\n");
			return -1;
		}
		char* src = (char*)mmap(NULL, write_size, PROT_READ|PROT_WRITE,
		 MAP_SHARED, res, 0); 
			
		SimpleFS_write(&file_handle, src, write_size);
		munmap(src, write_size);
		close(res);
		printf("<Shell> write_input.hex read succesfully\n");
		
		bad_choice = 0;
	}
	
	if(strncmp(&choice, "5", sizeof(char)) == 0) {
		//MkDir function
		printf("<Shell> Insert dirname: ");
		scanf("%s", filename);
		printf("\n");
		
		if(SimpleFS_mkDir(pwd_handle, filename) != 0){
			printf("<Shell> Error creating a new directory\n");
			return -1;
		}
		
		printf("<Shell> The directory was created succesfully.");
		
		fs->current_directory_block = pwd_handle->dcb;
		
		bad_choice = 0;
	}
	
	if(strncmp(&choice, "6", sizeof(char)) == 0) {
		//Cd function
		printf("<Shell> Insert dirname: ");
		scanf("%s", filename);
		printf("\n");
		
		if(strncmp(filename, "..", sizeof(char)*128)!=0){
			//File or folder check
			FirstFileBlock temp;
			FileHandle dest_handle;
		
			SimpleFS_openFile(pwd_handle, filename, &dest_handle);
			DiskDriver_readBlock(fs->disk, &temp, dest_handle.fcb);
				
			if(temp.fcb.is_dir == 0) {
				printf("<Shell> Not a dir\n");
				return -1;
			}
		}
			
		//Dir change
		if(SimpleFS_changeDir(pwd_handle, filename) != 0){
			printf("<Shell> Error opening directory\n");
			return -1;
		}
		
		fs->current_directory_block = pwd_handle->dcb;
		
		bad_choice = 0;
	}
	
	if(strncmp(&choice, "7", sizeof(char)) == 0) {
		//Remove function
		
		printf("<Shell> Insert file or dir name: ");
		scanf("%s", filename);
		printf("\n");
		
		FileHandle deletion_handle;
		
		if(SimpleFS_openFile(pwd_handle, filename, 
									&deletion_handle) != 0 ){
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
		printf("<Shell> Are you sure? Data will be lost!\n");
		printf("Write *yes* to continue, otherwise \
operation will abort.\n");
		scanf("%s", filename);
		printf("\n");
		
		if(strncmp(filename, "yes", sizeof(char)*3) != 0) return 0;
		
		printf("<Shell> Diskname: %s\n", fs->diskname);
		
		char disk_name[128];
		strncpy(disk_name, fs->diskname, 128*sizeof(char));
		if(SimpleFS_format(fs, disk_name, 
										fs->disk->num_entries) != 0){
			printf("<Shell> A disaster happened!\n");
			exit(-1);
		}
		
		printf("\n<Shell> Disk formatted, please restart the shell!\n");
		
		return 1;
	}
	
	if(bad_choice != 0){
		printf("\n<Shell> Invalid choice!\n");
		return -1;
	}
	
	return 0;
}

