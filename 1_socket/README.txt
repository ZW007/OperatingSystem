To RUN the code, there is a ./D1 folder in the root dir. D1 folder stores F1.txt and F2.txt which we will be transferred from client_c2 to server_c1

fist compile server_c1.cpp and client_c2.cpp using make 

then on server_c1 do ./server_c1, after that on client_c2 do ./client_c2  


Two things need to notiece about how code work:

(1) C2 should read and send contents of file F1 to C2 in successive messages of size 256 bytes (or remaining size
of the file when you get near the end of the file.

To achieve this, first read F1.txt size fileSize and also send it to c1. Now, we can use a for loop to send file and 
also read socket in the same manner. 

(2) While ssize_t read(int fd, void *buf, size_t nbytes); most time it can read nbytes as we tell it to do so, in the 
server_c1.cpp reading socket part, for (int i = 0; i < ((receiveFileLen/256) +1); i++) 
I find it cannot read exact number of bytes, i.e., fileLen%256 from socket when comes to the last iteration of for loop
I later found this problem is common, we cannot trust read(), so I use a function, read_socket(), which uses while to read
till all the bytes we required has been filled.  https://stackoverflow.com/questions/1251392/read-from-socket-is-it-guaranteed-to-at-least-get-x-bytes


