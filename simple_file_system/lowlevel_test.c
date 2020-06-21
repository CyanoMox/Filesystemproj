//#pragma once
#include "bitmap.h"
#include "disk_driver.h"
//#include "simplefs.h" 
#include <stdio.h>


//Testing bitmap and disk driver

int main (){
	DiskDriver disk;
	int res = DiskDriver_init(&disk, "test_fs", 4092);
	if (res==-1) printf("OCCAZZO! \n");
	return 0;
}
