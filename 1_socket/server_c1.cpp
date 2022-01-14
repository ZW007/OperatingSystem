#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h> // for mkdir 
#include <errno.h>

#define BUF_SIZE 256

//though you specficy the num of bytes N you want to read, read() cannot 
// always read N bytes in one shot, so need to use while()
int read_socket(int sock, char *buf, int size) {
   int bytes_read = 0, len = 0;
   while (bytes_read < size && 
         ((len = read(sock, buf + bytes_read,size-bytes_read)) > 0)) {
       bytes_read += len;
   }
   if (len == 0 || len < 0) printf("c1 read socket ERROR\n");
   return bytes_read;
}

int main(){
    errno = 0;
    //create socket
    int serv_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(errno!=0){
        printf("socket create error： %s\n",strerror(errno));
        exit(0);
    }

    //bind
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));  //fill every byte with 0 first.b
    serv_addr.sin_family = AF_INET;  //Use IPv4 address 
    serv_addr.sin_addr.s_addr = inet_addr("10.176.69.32");  //server DC01's IP     
    serv_addr.sin_port = htons(12340);  //use port 1234. 
    bind(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

    
    listen(serv_sock, 20);

    
    struct sockaddr_in clnt_addr;
    socklen_t clnt_addr_size = sizeof(clnt_addr);
    //code after listen() will keep runing until accept()。accept() will block the process until being connected by the client. 
    int clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size);

    char message[100] = "";
    const char messageLocal[8] = "makeDir";
    const char ack[11] = "ackmakeDir";
    const char messageLocal_1[9] = "makeFile";
    const char ack_1[12] = "ackmakeFile";
    const char messageLocal_2[8] = "endFile";
    const char ack_2[11] = "ackendFile";
    const char messageLocal_3[7] = "endAll";


    read(clnt_sock, message, 8);
    printf("Message form client: %s\n", message);

    // create dir upon receiving instructions. 
    if ((strcmp(message,messageLocal)==0)){
        // mkdir("./D1copy", S_IRWXU | S_IRWXG | S_IRWXO);  
        mkdir("./D1copy", 0777);
        printf("dir D1copy created\n");
        write(clnt_sock, ack, 11);

    }
   
    char buffer[BUF_SIZE];  
    int nCount;
    int fileNum;  //copy two files, F1, F2 from client using socket.  
    for (fileNum = 0; fileNum < 2; fileNum++)
    {
         // create file upon receiving instructions. 
        memset(message, 0, sizeof(message));
        read(clnt_sock, message, sizeof(messageLocal_1));
        printf("Message form client: %s\n", message);
        if ((strcmp(message,messageLocal_1)==0)){
        FILE * fp;
        if (fileNum==0) 
            fp = fopen("D1copy/F1.txt", "wb+"); 
        else 
            fp = fopen("D1copy/F2.txt", "wb+");   
        if(fp == NULL){
            printf("Cannot open file, press any key to exit!\n");
            system("pause");
            exit(0);
        }
        write(clnt_sock, ack_1, sizeof(ack_1)); //step 9, C1 create a file F1 and acknowledge the successful creation of the file.

        int receiveFileLen; 
        read(clnt_sock, &receiveFileLen, sizeof(int));
        printf("clinet_c2 file size is %d\n",receiveFileLen);
        for (int i = 0; i < ((receiveFileLen/256) +1); i++)
        {
            memset(buffer, 0, sizeof(buffer));  //clear buffer
            int readNum =
            read_socket(clnt_sock, buffer, i < (receiveFileLen/256) ? sizeof(buffer) :(receiveFileLen%256));
            printf("buffer that writen into file is: %s\n",buffer);
            printf("bytes server_c1 reads from socket: %d ,(receiveFileLen%256) is : %d   \n", readNum,receiveFileLen%256);
            int writeFileNum = fwrite(buffer, 1, strlen(buffer), fp);
            printf("bytes num written into local file:  %d \n\n",writeFileNum);
   
        }

        memset(message, 0, sizeof(message));
        read(clnt_sock, message, sizeof(messageLocal_2));
        printf("message from client is: %s\n",message);

        if ((strcmp(message,messageLocal_2)==0)){
            //  step 12, C1 respond indicating successful creation of the file in subdirectory D1copy
            write(clnt_sock, ack_2, sizeof(ack_2));
        }

        fclose(fp);

        }
    }
    
    puts("sever_c1 is done!");

    //C1 also close the connection and terminate on receiving the end of session message from C2.
    memset(message, 0, sizeof(message));
    read(clnt_sock, message, sizeof(messageLocal_3));
    printf("message_3 has been received by server_c1: %s\n", message);
    if ((strcmp(message,messageLocal_3)==0)){
    close(clnt_sock);
    close(serv_sock);
    printf("server_c1 sockets are closed\n");
    }
    return 0;
}