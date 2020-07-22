This projects consists in a Double LLA-FS, with an operating shell.

## Project explanation  

### List of file contents

1. disk_driver.h 
2. simplefs.h 
3. simplefs_editor.c 
4. lowlevel_test 
5. Makefile

6. write_input.hex and read_output.hex <>

**Other files present in the directory are considered DEPRECATED, and should not be used or controlled. I didn't check their functionality, so they can be crash-y or meaningless.**


### disk_driver.h

Driver that performs low level operations, like acting with blocks, bitmap and initializing a drive.

**ATTENTION:** for simplicity I have included bitmap operations directly in this file, so deprecating bitmap.h library.

1. Bitmap is formed by:

- An integer, that represents the number of blocks in which the disk consists.
- An array of bytes (chars), that contains the status for every allocated block. (0 for empty, 1 for full.)
- A padding array, that rounds the size of the array + the int 4 bytes to a stright multiple of PAGE_SIZE (here, 4096 bytes). This is important for mmapping operations.

2. Blocks work this way:

- Every operation that reads or writes a block invokates a function (_getBlock()) that mappes it into a temporary mmap, linked in the DiskDriver struct.
This is necessary because is not recommended to store the whole FS in a persistent mmap, because very large disks should cause severe memory problems. 
Due to limitations in Linux/POSIX mmap implementation, if the starting offset is non-zero, it has to be a multiple of system PAGE_SIZE.
DiskDriver_getBlock() handles it, and mmappes a system page containing a set of blocks, in which there is the requsted block. Formally, the function divides the FS block area in "FS pages" of PAGE_SIZE size, calculates the offset of the requested block, mmaps the right "FS page" if necessary and returns a pointer to the requested block in that "FS page" (block_map).

**ATTENTION:** the size of the blocks is 512, which is a sub-multiple of system page size (4096 bytes). This will not work otherwise.

- Functions like (_freeBlock() and _getFreeBlock()) work in Bitmap only, so they don't need to map the relative block.

- Function _init() calculates the Bitmap, creates the disk and generates the initial DiskDriver struct info.
Function _resume() takes an already-made disk and re-calculates the struct info, making it operating again even after a close or crash.


### simplefs.h

Driver that performs high level operations, acting with FS logical structure itself.

**ATTENTION:** in this implementation, due to the peculiar functionality of the disk_driver.h library, I could not use direct block pointers in handles, but indexes.
For this cause, I had to **deeply** modify suggested handles and function signatures, resulting in a "revolutionized" library.

1. General operating info:

- Most functions return handles by side-effect instead of functionally. I made this choice because it was really useful to have functions return an exit status, which was more difficult with previous signatures.
- Handles no more contain pointers, but indexes (except of the SimpleFS struct pointer itself).
- Every function that operates on a block has to load it in memory by providing its index (DiskDriver_readBlock()).
- Only then it could modify the block itself, and, after that operation, it has to write it back on the disk (DiskDriver_writeBlock()).

2. Brief function explanation:

- _init() initializes SimpleFS struct
- _format() invokates DiskDriver_init() to (re)truncate the disk file, then creates the root dir and makes a handle to it.
- _createFile() checks if a file/dir with same name is present in pwd (by invokating _readDir()). If negative, it allocates the relative file fcb in Bitmap and block. [A folder could not contain a file and a dir with same name!]
- _openFile() checks if a file/dir with same name is present in pwd (by invokating _readDir()). If affirmative, it returns a handle of that file/dir.
- write() takes a byte array in input, and writes it down to the file pointed by handle, taking regard of allocating new file remainders if necessary. [If a file is witten two or more times, it will overwrite it until size value. There is no way of deliberately "shorten" a file in this implementation.]
- _read() returns for side effect an array containing file content until size value.
- _changeDir() calls _openFile() to have dir handle, if such dir exists, then returns it by side effect.
- _mkDir() like _createFile(), but with dirs.
- _remove(): this is a very complex function, because it is not trivial to mantain FS integrity, expecially having to operate with indexes instead of pointers (and relative temporary mmaps on-the fly).
It works exploring dir tree and removing themselves recursively.
For each dir in the tree, when a file is found, the function eliminates every block of it iteratively.
After every elimination (regardless if file or dir), it's index in the upper dir is eliminated, too.
**Every time that it happens, the function will refill the hole by taking the last element in the array of the last block of the pwd itself and moving it in the empty place that was generated.**

This feature is important for three reasons:
1) No empty holes are allowed in dirs. It should affect _readDir() beheaviour and thus every function that makes use of it, making FS inoperable.
2) It makes easier to implement the _remove() function, because I can control cyclically the first dir array position until the dir is empty.
3) It makes easier to find and remove empty dir remainders.

Other functions are auxiliary.


### simplefs_editor.c

Shell that lets user perform complex high level operations, like FS browsing and editing, and is very useful to test FS capabilities and stability. **THIS IS THE MAIN FILE.**

The shell itself is verbose and simple to use. I don't wave to describe it deeply here.

The shell is also designed to test FS stability. [If, creating a file, "fill_test" is written, it will create a battery of files as requested.]


### lowlevel_test.c

Contains a battery of tests to test disk_driver.h capabilities and stability.


### write_input.hex and read_output.hex

These files are necessary to FS functionality with read and write operations. The former file contains a text (or hex) in input to be copied into a FS file, the latter contains the output generated by reading a FS file itself.
