#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h> 
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>

#define BUF_SIZE 256

int main(){

    errno=0;
    FILE * fp;

    //creat socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
     if(errno!=0){
        printf("socket create error： %s\n",strerror(errno));
        exit(0);
    }
    //request to the server_c1 
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));  //fill every byte 0 first
    serv_addr.sin_family = AF_INET;  //use IPv4 type address
    serv_addr.sin_addr.s_addr = inet_addr("10.176.69.32");  //server_c1's ip address 
    serv_addr.sin_port = htons(12340);  
    connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
   
    const char message[8] = "makeDir";
    const char message_1[9] = "makeFile";
    const char message_2[8] = "endFile";
    const char message_3[7] = "endAll";

    const char ackLocal[11] = "ackmakeDir";
    const char ackLocal_1[12] = "ackmakeFile";
    const char ackLocal_2[11] = "ackendFile";


    write(sock, message, sizeof(message));   //send makeDir instruction 
    

   char buffer[BUF_SIZE] = {0};  
   int nCount;
   int fileNum;  //copy two files, F1, F2 from client using socket.  
   char ack[100] = "";

   read(sock, ack, 11);
   printf("ack form server: %s\n", ack);

   if ((strcmp(ack,ackLocal)==0)){
     for (fileNum = 0; fileNum < 2; fileNum++)
    {
        if (fileNum==0) 
            fp = fopen("D1/F1.txt", "rb"); 
        else 
            {fp = fopen("D1/F2.txt", "rb");  printf("F2.txt opened\n");}
        if(fp == NULL){
        printf("Cannot open file, press any key to exit!\n");
        system("pause");
        exit(0);
        }
        write(sock, message_1, sizeof(message_1)); //on receving ack on 'makeDir' ("ackmakeDir") from server, order server to makeFile

        memset(ack, 0, sizeof(ack));
        read(sock, ack, 12);
        printf("ack form server: %s\n", ack);
        if ((strcmp(ack,ackLocal_1)==0)){            //on receving ack on 'makeFile' ("ackmakeFile") from server, start to send file to client 
        fseek(fp, 0L, SEEK_END);  //move fp to the end of the file
        int fileLen =  ftell(fp);  // get the file size because ftell returns the current value of the position indicator
        fseek(fp, 0L, SEEK_SET); //seek back to the begin of the file
        write(sock, &fileLen, sizeof(int));   //send file size to server c1 

        // C2 should read and send contents of file F1 to C2 in successive messages of size 256 bytes 
        // (or remaining size of the file when you get near the end of the file)
        char buffer[BUF_SIZE];  //BUF_SIZE=256
        for (int i = 0; i <((fileLen/256) +1); i++)
        {
            memset(buffer, 0, sizeof(buffer));  //clear buffer
            fread(buffer, 1, i <(fileLen/256)?(sizeof(buffer)):(fileLen%256), fp);  // read BUF_SIZE elements, each one with a size of 1 bytes,
            printf("Message client_c2 sent: %s\n", buffer); //though buffer can be larger than you actually write thus many blanks, just for test
            int writeNum =
            write(sock, buffer,  i <(fileLen/256)?sizeof(buffer):(fileLen%256));
            printf("bytes client_c2 writes into socket: %d,(fileLen%256) is:%d  \n", writeNum,(fileLen%256));

            usleep(1000);
            printf("loop number is: %d\n\n",i); 
        }
        fclose(fp);

        // step 12, C2 should send a message indicating end of file, and C1 should respond with a message indicating successful creation of the file in subdirectory D1copy.
        write(sock, message_2, sizeof(message_2));  
        printf("client_c2 finishes file trasit\n");
        
        memset(ack, 0, sizeof(ack));
        read(sock, ack, sizeof(ackLocal_2));
        printf("ack form server: %s\n", ack);

        //C2 send a message indicating end of session to C1, close the socket connection and terminate.
        if ((strcmp(ack,ackLocal_2)==0)&&(fileNum==1)){ 
        // if ((strcmp(ack,ackLocal_2)==0)){ 
            write(sock, message_3, sizeof(message_3));  
            printf("message_3 has been sent by client_c2: %s\n",message_3);
            close(sock);
            printf("client socket is closed\n");
          }
        }
     
    }
   }
    

    return 0;
}  