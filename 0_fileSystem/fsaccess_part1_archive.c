/***********************************************************************
 * Author: Zhen WANG, Rong TU
 
* Prof. S Venkatesan

* Project - 2*

***************

* Compilation :-$ gcc -std=c99 fsaccess_part1.c -o fsaccess_part1
* Run using :- $ ./fsaccess_part1
******************/


/***********************************************************************

This program allows user to do two things: 
   1. initfs: Initilizes the file system and redesigning the Unix file system to accept large 
      files of up tp 4GB, expands the free array to 152 elements, expands the i-node array to 
      200 elemnts, doubles the i-node size to 64 bytes and other new features as well.
   2. Quit: save all work and exit the program.
   3. cpin
   4. cpout
   
 User Input:
     - initfs (file path) (# of total system blocks) (# of System i-nodes)
     - q
     - cpin
     - cpout
     
 File name is limited to 14 characters.

******************/

#include<stdio.h>
#include<fcntl.h>
#include<unistd.h>
#include<errno.h>
#include<string.h>
#include<stdlib.h>

#include <sys/stat.h>


#define FREE_SIZE 152  // free[152], free at most has 152 elemnt, it store at most 152 indices of data block.
#define I_SIZE 200    //inode[200]
#define BLOCK_SIZE 1024    
#define ADDR_SIZE 11 //addr[11]
#define INPUT_SIZE 256


// Superblock Structure

typedef struct {
  unsigned short isize;  // isize is the number of blocks devoted i-list, which starts right after super block, block #2
  unsigned short fsize;  //superBlock.fsize = blocks; total number of potentially avaiable blocks to the file system,from user input 
  unsigned short nfree;
  unsigned int free[FREE_SIZE];
  unsigned short ninode;    // ninode is the number of free i-numbers in the inode array. To allocate a inode, decerement it and return inode[ninode] (if niode>0 though)
  unsigned short inode[I_SIZE]; //inode[200]
  char flock;
  char ilock;
  unsigned short fmod;
  unsigned short time[2];
} superblock_type;

superblock_type superBlock;

// I-Node Structure

typedef struct {
unsigned short flags;
unsigned short nlinks;
unsigned short uid;
unsigned short gid;
unsigned int size;
unsigned int addr[ADDR_SIZE];
unsigned short actime[2];
unsigned short modtime[2];
} inode_type; 

inode_type inode;

typedef struct {
  unsigned short inode;
  unsigned char filename[14];
} dir_type;

dir_type root;

int fileDescriptor ;		//file descriptor 
const unsigned short inode_alloc_flag = 0100000;
const unsigned short dir_flag = 040000;
const unsigned short dir_large_file = 010000;
const unsigned short dir_access_rights = 000777; // User, Group, & World have all access privileges 
const unsigned short INODE_SIZE = 64; // inode has been doubled

int preInitialization();
int initfs(char* path, unsigned short total_blcks,unsigned short total_inodes);
void add_block_to_free_list( int blocknumber , unsigned int *empty_buffer );
void create_root();
int file_size2(char* filename);
int cpin();
void request_blocks_from_free_list_Copy_source_to_it(unsigned int fileSize, unsigned int blocksRequested,  int *source);
void add_slice_perFile_to_root_data_blocks(dir_type slice);
void add_inodeInfo_to_inode_blocks(dir_type slice);
int cpout();

int main() {
  
  char input[INPUT_SIZE];
  char *splitter;
  unsigned int numBlocks = 0, numInodes = 0;
  char *filepath;
  printf("*******************************************************************\n");
  printf("Size of super block = %d , size of i-node = %d\n",sizeof(superBlock),sizeof(inode));
  printf("*******************************************************************\n");
  printf("(1)Please ensure the file you use is not larger than ADDR_SIZE*BLOCK_SIZE=11264 bytes!\n");
  printf("(2)Please press q to save result when you wanna leave\n");
  printf("*******************************************************************\n");
  printf("usage example:\n");
  printf("initfs /home/zhen/Desktop/5348/project2-fileSystem/disk 10000 100\n");
  printf("cpin fsaccess.c aaa.c\n");
  printf("cpout aaa.c externalOut.c\n");
  printf("q\n");
  printf("rm externalOut.c\n");
  printf("*******************************************************************\n");
  while(1) {
  
    scanf(" %[^\n]s", input);     // \n作为字符串输入的结束符
    splitter = strtok(input," ");
    
    if(strcmp(splitter, "initfs") == 0){

        preInitialization();
        splitter = NULL; 
                       
    } else if (strcmp(splitter, "q") == 0) {
   
       lseek(fileDescriptor, BLOCK_SIZE, 0);
       write(fileDescriptor, &superBlock, BLOCK_SIZE);
       return 0;
     
    } else if (strcmp(splitter,"cpin")==0){

        cpin();
        splitter = NULL;

    } else if (strcmp(splitter,"cpout")==0){

        cpout();
        splitter = NULL;
    }
  }
}

int cpout(){
    char *v6OutPath, *externalOutPath;
    v6OutPath = strtok(NULL, " "); 
    externalOutPath = strtok(NULL, " "); 
    int externalOutFileDescriptor;
    int root_data_block = 2 + superBlock.isize;

    unsigned char filenameLookup[14]; 
    unsigned short inodeNumLookup;
    for (int i = 0; i < I_SIZE; i++)
    {
      // {assume} ONLY consider one root data block 
      if (i<BLOCK_SIZE/16){
        lseek(fileDescriptor, (root_data_block*BLOCK_SIZE + i*16 +2), 0);  // +2 becuase string starts from the 3rd byte
        read(fileDescriptor, filenameLookup, 14);

            printf("# %d string from the root data block is:%s, string from cpout file name is:%s\n",i+1,filenameLookup,v6OutPath);             

                          // printf("# %d string from the root data block is:%s\n",i+1,filenameLookup);                   
        if (strcmp(filenameLookup,v6OutPath)==0){

              lseek(fileDescriptor, (root_data_block*BLOCK_SIZE + i*16), 0);
              read(fileDescriptor, &inodeNumLookup, 2);

                   printf("# \t\t %d string from the root data block is:%s, it matches string from cpout file name:%s!!!!!\n",i+1,filenameLookup,v6OutPath);
                   printf("inodeNum of this matched file is %d\n",inodeNumLookup);
              break;
        }

         else {
            if(i==(BLOCK_SIZE/16 - 1))
              {printf("sorry, we actually only considered storing BLOCK_SIZE/16 files, among them, no one matches your input file name\n");
              return 0;}
          }
      }
    }

    lseek(fileDescriptor, (2*BLOCK_SIZE)+(inodeNumLookup-1)*INODE_SIZE, 0);
    read(fileDescriptor, &inode, INODE_SIZE);

    if((externalOutFileDescriptor = open(externalOutPath,O_RDWR|O_CREAT,0700))== -1)   // O_RDWR: read/write allowed, O_CREAT: If pathname does not exist, create it as a regular file
                                                                // 0700: user (file owner) has read, write, and execute permission
       {
           printf("\n open() external file to cpout failed with the following error [%s]\n",strerror(errno));
           return 0;
       }
    int j =0;
    int cpoutFileSize = 0;

    //int oneBlockDataToCpout[BLOCK_SIZE/4];  // 1024/sizeof(int), initialize the block size of data into zero first
    char oneBlockDataToCpout[BLOCK_SIZE]; 
    char _1stBlockDataToCpout_4bytesLess[ BLOCK_SIZE-4];
    while(inode.addr[j]!=0){

      if(j==0){

      printf("we read file size & data from block #%d during CpOuts\n",inode.addr[j]);

      lseek(fileDescriptor, inode.addr[0]*BLOCK_SIZE, 0);
      read(fileDescriptor, &cpoutFileSize, 4);  // need to read file size first, from the 1 st data block
      printf("read the FileSize that needs to be cpouted is %d from the 1st data block of this v6 file\n",cpoutFileSize);
      read(fileDescriptor, _1stBlockDataToCpout_4bytesLess, BLOCK_SIZE-4); // next read  BLOCK_SIZE-4 data contents from the 1 st data block
      // lseek(externalOutFileDescriptor, 0*BLOCK_SIZE, 0); // start to write to the external file. 
      if (cpoutFileSize>BLOCK_SIZE-4){
            write(externalOutFileDescriptor, _1stBlockDataToCpout_4bytesLess, BLOCK_SIZE-4);
      }
      else{
            write(externalOutFileDescriptor, _1stBlockDataToCpout_4bytesLess, cpoutFileSize);
      }
      
      }


      else{
      
                if ( j*BLOCK_SIZE + BLOCK_SIZE-4 <cpoutFileSize){
                  printf("we continue reading data from block #%d during CpOuts\n",inode.addr[j]);

                lseek(fileDescriptor, inode.addr[j]*BLOCK_SIZE, 0);
                read(fileDescriptor, oneBlockDataToCpout, BLOCK_SIZE);
                
                // lseek(externalOutFileDescriptor, (j-1)*BLOCK_SIZE + BLOCK_SIZE-4, 0);   
                write(externalOutFileDescriptor, oneBlockDataToCpout, BLOCK_SIZE);
                }

                else {
                    printf("we read data from last block #%d during CpOuts, we will read according to file actual size\n",inode.addr[j]);
                    lseek(fileDescriptor, inode.addr[j]*BLOCK_SIZE, 0);
                    read(fileDescriptor, oneBlockDataToCpout, BLOCK_SIZE);
                    // lseek(externalOutFileDescriptor, (j-1)*BLOCK_SIZE + BLOCK_SIZE-4, 0);   
                    write(externalOutFileDescriptor, oneBlockDataToCpout, cpoutFileSize-((j-1)*BLOCK_SIZE + BLOCK_SIZE-4));
                }
          }
      
      j++;
    }
   
      printf("blocks requested by the file stored in v6 is %d\n",j);
      for(int i = 0; i < ADDR_SIZE; i++ ) { //set them all zeros first to ensure safety
        inode.addr[i] = 0;
        }
   return 0;

}

int cpin(){

    char *externalInPath, *v6InPath; 
    externalInPath = strtok(NULL, " "); 
    v6InPath = strtok(NULL, " "); 

    int externalInFileDescriptor; 
    unsigned int externalInFileSize  = file_size2(externalInPath);  // call int file_size2() function to get file size , how many bytes 

    unsigned int v6InFileBlocks; 
    if((externalInFileSize%BLOCK_SIZE) == 0)
    v6InFileBlocks = externalInFileSize/BLOCK_SIZE; //determine how many blocks will be used to store the incoming file.
    else
    v6InFileBlocks = (externalInFileSize/BLOCK_SIZE) + 1; 

    unsigned int v6InFileSizeByInt;
    if((externalInFileSize%4) == 0)
    v6InFileSizeByInt = externalInFileSize/4; //determine how many blocks will be used to store the incoming file.
    else
    v6InFileSizeByInt = (externalInFileSize/4) + 1; 
    int v6InFileBuffer[v6InFileSizeByInt]; // v6InFileBuffer[] is used to read external file, and then write it to the data blocks; 4 is sizeof( int)
    // set v6InFileBuffer[] zero first.
    for (int i = 0; i < v6InFileSizeByInt; i++) 
   	  v6InFileBuffer[i] = 0;          
     
    
    if((externalInFileDescriptor = open(externalInPath, O_RDWR, 0600))== -1){
         printf("\n open() external cpin file failed with error [%s]\n", strerror(errno));
         return 1;
      }
    printf("external file to copy in is now opened\n");
    printf("external file to copy size in bytes: %d\n", externalInFileSize);

    //read out external file ,and put it into v6InFileBuffer
    ssize_t a = read(externalInFileDescriptor,v6InFileBuffer,externalInFileSize);
    printf("%d bytes of external file have been read\n",a);
                      // TEST FOR:  initfs /home/zhen/Desktop/5348/project2-fileSystem/disk 1000 100
                      // printf("superBlock.nfree is %d \n",superBlock.nfree);
                      // for (int i = superBlock.nfree-1; i >=0; i--)
                      // {
                      //   printf("superBlock.free[%d] is %d \n",i,superBlock.free[i]);

                      // }
                      // printf("wonder if printf superBlock.free[] can uninterruptedly finised \n"); //without reading 0, good to GO!!!
                      // lseek( fileDescriptor, superBlock.free[0] * BLOCK_SIZE, 0 );
                      // unsigned int bufferTest[BLOCK_SIZE/4];
                      // read( fileDescriptor, bufferTest, BLOCK_SIZE ); 
                      // for ( int i = BLOCK_SIZE/4-1; i >=0; i--)
                      // {
                      //   printf("superBlock.free[0] is block #922, Block_922[%d]: %d \n",i,bufferTest[i]);

                      // }

    /* Writing free list to the current data block
    *  put v6InFileBuffer contents into available blocks, consume data blocks, available blocks are found from free[];
    *  Need to record the block number(s) it consumes, and put them into inode.addr[], so that we can add_inodeInfo_to_inode_blocks();*/

      for(int i = 0; i < ADDR_SIZE; i++ ) { //set them all zeros first to ensure safety
        inode.addr[i] = 0;
        }
    request_blocks_from_free_list_Copy_source_to_it(externalInFileSize,v6InFileBlocks,v6InFileBuffer);

    /* superBlock.ninode inital value is I_SIZE = 200.
     * superBlock.inode[0,1...199] = [1,2..200], 1 is root's inode number, roots parents' inode number is still 1. 
     * so newSlice.inode start from 2, and should be less than 200.  {assume} Dont consider the case when superBlock.ninode ==0. Coz more work to do:( */
    dir_type newSlice;
    superBlock.ninode = superBlock.ninode-1;
    newSlice.inode = superBlock.inode[I_SIZE - superBlock.ninode]; 

    int strlenV6InPath = strlen(v6InPath);
    for (int i = 0; i < strlenV6InPath; i++)
    {
       newSlice.filename[i] = v6InPath[i];
    } 
    newSlice.filename[strlenV6InPath] = '\0'; //END 
                      // printf("strlen(v6Inpath) is %d:\n",strlen(v6InPath));
                      // puts(v6InPath);
                      // puts(newSlice.filename);
                      // printf("strcmp result: %d\n",strcmp(newSlice.filename,v6InPath));
    
    //  {assume} NO sub-directory, all slice info is stored in root_data_block(s)
    add_slice_perFile_to_root_data_blocks(newSlice);

    // correspondingly add inode info of newly added file into the inode in the ilist blocks. 
    add_inodeInfo_to_inode_blocks(newSlice);
        for(int i = 0; i < ADDR_SIZE; i++ ) { //set them all zeros first to ensure safety
        inode.addr[i] = 0;
        }
  
    return 0;
}
void add_slice_perFile_to_root_data_blocks(dir_type slice){
  // {assume} do NOT consider the case when one data block of directory (BLOCK_SIZE/16 = 64 slices) is NOT enough to store all the dir info slices! 
  // since only 64, we also dont consider when inode[I_SIZE=200] is not enough )
  int root_data_block = 2 + superBlock.isize;
  lseek(fileDescriptor, (root_data_block*BLOCK_SIZE + slice.inode*16), 0); // /e.g.: the 1st new file's inode number is 2, the slice info will be written into 3rd entry
                                                                            // becuase root and root's parent are two separate slices. 
  write(fileDescriptor, &slice, 16);  // write 16 bytes of root directory info (1 entry/slice is 16bytes) into the 1st data block

}

void add_inodeInfo_to_inode_blocks(dir_type slice){
  int root_data_block = 2 + superBlock.isize;
  // inode.flags = inode_alloc_flag | dir_flag | dir_large_file | dir_access_rights;   		// flag for root directory 
  inode.flags = inode_alloc_flag  | dir_access_rights; // flag for small file??? maybe 
  inode.nlinks = 0; 
  inode.uid = 0;
  inode.gid = 0;
  inode.size = INODE_SIZE;

  // inode.addr[] has already been update in request_blocks_from_free_list_Copy_source_to_it(v6InFileBlocks,v6InFileBuffer);

  inode.actime[0] = 0;
  inode.modtime[0] = 0;
  inode.modtime[1] = 0;
  
  lseek(fileDescriptor, 2 * BLOCK_SIZE +  INODE_SIZE * (slice.inode-1), 0); //e.g.: the 1st new file's inode number is 2, the inode info
                                                                       // should be written into 2nd inode in ilist blocks, because root and root's parent only needs 1 block
  write(fileDescriptor, &inode, INODE_SIZE);   
}

// breakpoint zhen Nov 21 Sat night
void request_blocks_from_free_list_Copy_source_to_it(unsigned int fileSize, unsigned int blocksRequested,  int *source){
    int oneBlockSource[BLOCK_SIZE/4];  // 1024/sizeof(int), initialize the block size of data into zero first
    for (int j = 0; j < BLOCK_SIZE/4; j++)
      {
          oneBlockSource[j] = 0;
      }
    for (int i = 0; i < blocksRequested; i++)
    {
        superBlock.nfree--;  //nfree is an integer >0, while free[] starts from free[0], so nfree-- first then proceed.

        for (int j = 0; j < BLOCK_SIZE/4; j++)
        {
            if((i==0)&&(j==0)){ 
              oneBlockSource[j]= (int) fileSize; 
              printf("we record filesize %d at the 1st postion of 1 st data store block\n", oneBlockSource[j]);
            }
            else {
            oneBlockSource[j] = source[i*BLOCK_SIZE/4+j -1];  // source real size is v6InFileBuffer[v6InFileSizeByInt],
                                                          // so it actually less than source[blocksRequested*BLOCK_SIZE] for MOST of time.
                                                          // SO no worry we read only source from source[0] to source[blocksRequested*BLOCK_SIZE-1]. remember -1 in your code
            }
        }
      // if nfree -- = 0, we need to read from data block #nfree[0], put its first element as nfree, and next nfree elements as free[]; 
        if((superBlock.nfree)==0){
          int readBlockIndexedByFreeZero[BLOCK_SIZE / 4];
          lseek( fileDescriptor, (superBlock.free[0] * BLOCK_SIZE), 0 ); 
          read( fileDescriptor,readBlockIndexedByFreeZero, BLOCK_SIZE );
          // superBlock.nfree = readBlockIndexedByFreeZero[0];
          superBlock.nfree = readBlockIndexedByFreeZero[0]-1; 

          for (int k = 1; k < superBlock.nfree+1; k++)
          {
              superBlock.free[k-1] = readBlockIndexedByFreeZero[k];
          }
          
          lseek( fileDescriptor, (superBlock.free[superBlock.nfree] * BLOCK_SIZE), 0 ); 
          write( fileDescriptor, oneBlockSource, BLOCK_SIZE );
          inode.addr[i] = superBlock.free[superBlock.nfree]; // record the block number index where we wrote data into. 
                          printf("we write data to into block #%d during cpin\n",inode.addr[i]);

        }

        else{
        lseek( fileDescriptor, (superBlock.free[superBlock.nfree] * BLOCK_SIZE), 0 ); 
        write( fileDescriptor,  oneBlockSource, BLOCK_SIZE ); // Writing copyed file to data block(s), copy into one free data block every time, v6InFileBlocks times in total 
        inode.addr[i] = superBlock.free[superBlock.nfree]; // record the block number index where we wrote data into. 
                        printf("we write data to into block #%d during cpin\n",inode.addr[i]);


        }
     
   
    }
    
    
}


int preInitialization(){

  char *n1, *n2;
  unsigned int numBlocks = 0, numInodes = 0;
  char *filepath;
  
  filepath = strtok(NULL, " "); //Some library implementations store strtok()'s internal status in a global variable, which may induce some nasty suprises, 
                                //if strtok() is called from multiple threads at the same time. Thread unsafety
                                 //在第一次调用时，char *strtok(char *s, char *delim)必需给予参数s字符串，往后的调用则将参数s设置成NULL。每次调用成功则返回指向被分割出片段的指针。
  n1 = strtok(NULL, " ");
  n2 = strtok(NULL, " ");
         
      
  if(access(filepath, F_OK) != -1) {
      
      if((fileDescriptor = open(filepath, O_RDWR, 0600)) == -1){
      
         printf("\n filesystem already exists but open() failed with error [%s]\n", strerror(errno));
         return 1;
      }
      printf("filesystem already exists and the same will be used. Your setup will not take effect unless you remove exsting file system\n");
      // bootstrap: read superBlock content of exsting v6 file system.
      lseek(fileDescriptor, BLOCK_SIZE, 0); // move to the position after skipping BLOCK_SIZE number of bytes, SEEK_SET is the beginning of the file. 
      read(fileDescriptor, &superBlock, BLOCK_SIZE);
      return 0;
  
  } else {
  
        	if (!n1 || !n2)
              printf(" All arguments(path, number of inodes and total number of blocks) have not been entered\n");
            
       		else {
          		numBlocks = atoi(n1);
          		numInodes = atoi(n2);

          		if( initfs(filepath,numBlocks, numInodes )){
          		  printf("The file system is initialized\n");	
                  return 0;
          		} else {
            		printf("Error initializing file system. Exiting... \n");
            		return 1;
          		}
       		}
  }
}

int initfs(char* path, unsigned short blocks,unsigned short inodes) {

   unsigned int buffer[BLOCK_SIZE/4]; //1024/4, 4 is sizeof(unsigned int)
   int bytes_written;
   
   unsigned short i = 0;
   superBlock.fsize = blocks;  // total number of potentially avaiable blocks to the file system,from user input 
   unsigned short inodes_per_block= BLOCK_SIZE/INODE_SIZE;
   
   if((inodes%inodes_per_block) == 0)
   superBlock.isize = inodes/inodes_per_block; //determine how many blocks will be used to store all inodes(arg, from user input). Coz One block can store inodes_per_block inodes
   else
   superBlock.isize = (inodes/inodes_per_block) + 1;
   
   if((fileDescriptor = open(path,O_RDWR|O_CREAT,0700))== -1)   // O_RDWR: read/write allowed, O_CREAT: If pathname does not exist, create it as a regular file
                                                                // 0700: user (file owner) has read, write, and execute permission
       {
           printf("\n open() failed with the following error [%s]\n",strerror(errno));
           return 0;
       }
       
   for (i = 0; i < FREE_SIZE; i++)  //152
      superBlock.free[i] =  0;			//initializing free array to 0 to remove junk data. free array will be stored with data block numbers shortly.
    
   superBlock.nfree = 0; 
   superBlock.ninode = I_SIZE; //200
   
   for (i = 0; i < I_SIZE; i++) 
	    superBlock.inode[i] = i + 1;		//Initializing the inode array to inode numbers
                                            //root dir's inode number is 1 , maybe
   
   superBlock.flock = 'a'; 					//flock,ilock and fmode are not used.
   superBlock.ilock = 'b';					
   superBlock.fmod = 0;
   superBlock.time[0] = 0;
   superBlock.time[1] = 1970;
   
   lseek(fileDescriptor, BLOCK_SIZE, SEEK_SET); // move to the position after skipping BLOCK_SIZE number of bytes, SEEK_SET is the beginning of the file. 
   write(fileDescriptor, &superBlock, BLOCK_SIZE); // writing superblock to file system
                                    //write() writes up to BLOCK_SIZE bytes from the superBlock starting at superBlock to the file referred to by the file descriptor.
                                    //write()会把参数superBlock所指的内存写入BLOCK_SIZE个字节到参数fd所指的文件内。

   
   for (i = 0; i < BLOCK_SIZE/4; i++) //buffer[] will take up one block space, since 4 is sizeof(unsigned int)
   	  buffer[i] = 0; 
  // writing zeroes to all inodes in ilist
   for (i = 0; i < superBlock.isize; i++)  //superBlock.isize is number of blocks that are used to store all inodes
   	  write(fileDescriptor, buffer, BLOCK_SIZE);   // now all inodes in these blocks 's inner value is zero, 
                                                    //no worry, we will make root node value nonzero next by create_root()!

   
   int data_blocks = blocks - 2 - superBlock.isize; 
   int data_blocks_for_free_list = data_blocks - 1; // minus 1 maybe root inode? inode 1 always corresponds to a file (block)
   
   // Create root directory
   create_root();
 
  //  for ( i = 2 + superBlock.isize + 1; i < data_blocks_for_free_list; i++ ) {    //this must be wrong, correct to 
      for ( i = 2 + superBlock.isize + 1; i < blocks; i++ ) {

      add_block_to_free_list(i , buffer); // set value of all the data blocks after the 1st data block zero. Unless the superBlock.free[FREE_SIZE] is full, then qiantao
                                          // because when the superBlock.free[] reaches FREE_SIZE, you need to 
                                          //store FREE_SIZE and { superBlock.free[FREE_SIZE] } at the current block, then moves ahead. 
                                          //unsigned int buffer[BLOCK_SIZE/4]; //1024/4, 4 is sizeof(unsigned int), so buffer takes up one block space
   }
   
   return 1;
}

// Add all Data blocks to free list except the 1st data block that is allocated to root
void add_block_to_free_list(int block_number,  unsigned int *empty_buffer){

  if ( superBlock.nfree == FREE_SIZE ) { // if free[FREE_SIZE] is full, need to copy FREE_SIZE and free[FREE_SIZE] to newly added block 

    int free_list_data[BLOCK_SIZE / 4], i;  // 4 is sizeof(int), so free_list_data takes up 1 block space, it works as the newly added block 
    free_list_data[0] = FREE_SIZE;
    
    for ( i = 0; i < BLOCK_SIZE / 4; i++ ) {
       if ( i < FREE_SIZE ) {  // 256>FREE_SIZE=152, so after copying free[FREE_SIZE] to the free_list_data[BLOCK_SIZE / 4], 
                                //there is still space free in free_list_data that needs to be set as 0
         free_list_data[i + 1] = superBlock.free[i];
       } else {
         free_list_data[i + 1] = 0; // getting rid of junk data in the remaining unused bytes of header block
       }
    }
    
    lseek( fileDescriptor, (block_number) * BLOCK_SIZE, 0 );
    write( fileDescriptor, free_list_data, BLOCK_SIZE ); // Writing free list to the current data block
    
    superBlock.nfree = 0;
    
  } else {

	lseek( fileDescriptor, (block_number) * BLOCK_SIZE, 0 ); 
    write( fileDescriptor, empty_buffer, BLOCK_SIZE );  // writing 0 to remaining data blocks to get rid of junk data
  }

  superBlock.free[superBlock.nfree] = block_number;  // Assigning blocks to free array
  ++superBlock.nfree;
}

// Create root directory
void create_root() {

  int root_data_block = 2 + superBlock.isize; // Allocating first data block to root directory
  int i; 
  
  root.inode = 1;   // root directory's inode number is 1.
  root.filename[0] = '.';
  root.filename[1] = '\0';
  
  inode.flags = inode_alloc_flag | dir_flag | dir_large_file | dir_access_rights;   		// flag for root directory 
  inode.nlinks = 0; 
  inode.uid = 0;
  inode.gid = 0;
  inode.size = INODE_SIZE;
  inode.addr[0] = root_data_block;
  
  for( i = 1; i < ADDR_SIZE; i++ ) {
    inode.addr[i] = 0;
  }
  
  inode.actime[0] = 0;
  inode.modtime[0] = 0;
  inode.modtime[1] = 0;
  
  lseek(fileDescriptor, 2 * BLOCK_SIZE, 0);
  write(fileDescriptor, &inode, INODE_SIZE);   // write inode info of the root into the 1st inode in ilist blocks
  
  lseek(fileDescriptor, root_data_block*BLOCK_SIZE, 0); 
  write(fileDescriptor, &root, 16);  // write 16 bytes of root directory info (1 entry/slice is 16bytes) into the 1st data block 's first entry 
  
  root.filename[0] = '.';
  root.filename[1] = '.';
  root.filename[2] = '\0';
  
  write(fileDescriptor, &root, 16); // write 16 bytes of .. parent directory info into the 1st data block 's second entry 
                                    // root parent is still root, so still root.inode=1;
 
}

/*
    function to get file size
    https://blog.csdn.net/yutianzuijin/article/details/27205121
*/
int file_size2(char* filename)
{
    struct stat statbuf;
    stat(filename,&statbuf);
    int size=statbuf.st_size;
 
    return size;
}
