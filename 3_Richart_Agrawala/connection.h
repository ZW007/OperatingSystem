#define CLIENT0    "10.176.69.32"           	//ip addr dc01,  ifconfig -a
#define CLIENT1    "10.176.69.33"           	//ip addr dc02
#define CLIENT2	   "10.176.69.34"		        //ip addr dc03   
#define CLIENT3    "10.176.69.35"           	//ip addr dc04
#define CLIENT4    "10.176.69.36"           	//ip addr dc05

#define SERVER0    "10.176.69.37"           	//ip addr dc06
#define SERVER1    "10.176.69.38"		        //ip addr dc07
#define SERVER2    "10.176.69.39"		        //ip addr dc08
#define PORT	    2408

//////////////////////
//Required Header files
//////////////////////
#include <iostream>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <dirent.h>
#include <ctime>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <fcntl.h>
#include <fstream>
#include <stdlib.h>
#include <errno.h>
#include <list> 
#include <iterator>
#include <vector>
#include <pthread.h>
#include <semaphore.h>
#include <ifaddrs.h>
#include <sys/shm.h> 
#include <sys/stat.h>
#include <regex>




