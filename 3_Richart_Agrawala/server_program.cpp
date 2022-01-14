#include <ifaddrs.h>
#include "connection.h"
#include <thread>
using namespace std;
#define PORT  2408
#define TRUE             1
#define FALSE            0
//global mutuex
pthread_mutex_t list_lock;
pthread_mutex_t resource_lock;



//Function
//Grab the IP Address of the current machine
string getIPAddress()
{
    	string ipAddress="Unable to get IP Address";
    	struct ifaddrs *interfaces = NULL;
    	struct ifaddrs *temp_addr = NULL;
    	int success = 0;
    
    	success = getifaddrs(&interfaces);
    	if (success == 0) 
	{
        
        	temp_addr = interfaces;
        	while(temp_addr != NULL) 
		{
            		if(temp_addr->ifa_addr->sa_family == AF_INET) 
			{
                
                		if(strcmp(temp_addr->ifa_name, "en0"))
				{
                    			ipAddress=inet_ntoa(((struct sockaddr_in*)temp_addr->ifa_addr)->sin_addr);
                		}
            		}
            	temp_addr = temp_addr->ifa_next;
        	}
    	}
   
    	freeifaddrs(interfaces);
    	return ipAddress;
}
///Creating a class to deal with lower socket 
class Socket_connection
{
public:
	int socket_fd, new_socket, valread; 
	struct sockaddr_in address; 
	int opt = 1; 
	int addrlen = sizeof(address); 
	int iMode=0;
	Socket_connection()
	{
		// Creating socket file descriptor 
		if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) 
		{ 
			perror("socket failed"); 
			exit(EXIT_FAILURE); 
		} 
		bzero((char *) &address, sizeof(address));
		ioctl(socket_fd, FIONBIO, &iMode); 
		address.sin_port = htons( PORT );
		address.sin_family = AF_INET;
		 
	}
	Socket_connection(const Socket_connection &sock)  //copy constructor
	{
	socket_fd=sock.socket_fd;
	new_socket=sock.new_socket; 
	valread=sock.valread;
	address=sock.address; 
	opt=1;
	addrlen =sock.addrlen;
	iMode=0;

	}
	~Socket_connection()  //destructor
	{
	
	}
	int connect_socket(char* IPname)
	{	
		
		
		if(inet_pton(AF_INET, IPname, &address.sin_addr)<=0)  
    		{ 
        		cout<<"Invalid address/ Address not supported "<<endl; 
        		return 0; 
    		}		
		
		if(connect(socket_fd, (struct sockaddr *)&address, sizeof(address))<0) 
		{
			cout<<"Connection Failed "<<endl; 
        		return 0;
		}
		else
		{
			return 1;
		}
		
		
		
    		
	}
	int listen_socket()
	{	
		
		// Forcefully attaching socket to the port 2408 
		if (setsockopt(socket_fd, SOL_SOCKET, (SO_REUSEPORT | SO_REUSEADDR),&opt, sizeof(opt))) 
		{ 
		perror("setsockopt"); 
		exit(EXIT_FAILURE); 
		} 
		address.sin_addr.s_addr = INADDR_ANY; 
		
		// Forcefully attaching socket to the port 8080 
		if (bind(socket_fd, (struct sockaddr *)&address, sizeof(address))<0) 
		{ 
		perror("bind failed"); 
		return 0; 
		} 
		
		if (listen(socket_fd, 32) < 0) 
		{ 
		perror("listen failed"); 
		return 0; 
		} 
		
	}
	int return_accept_response()
	{
		cout<<"waiting to connect here"<<endl;
		if ((new_socket = accept(socket_fd, (struct sockaddr *)&address,(socklen_t*)&addrlen))<0) 
		{ 
		perror("accept failed"); 
		return 0; 
		} 
		else
		{
			return 1;
		}
		
	}
	int return_accept_socket()
	{
		return new_socket;
	}
	
			
};

//Creating a wrapper for the socket with sender and receiver information
class SocketWrapper
{
public:
	string sender;
	string receiver;
	Socket_connection socket_id;
	int connect_num;
	SocketWrapper()
	{
		sender="";
		receiver="";

	}
	SocketWrapper(const SocketWrapper &wrap)//copy constructor
	{
		sender=wrap.sender;
		receiver=wrap.receiver;
		socket_id=wrap.socket_id;
		connect_num=wrap.connect_num;

	}
	~SocketWrapper()//destructor
	{
	}
	
};

///function
//to read all the files present in the directory
string read_directory(char* name)
{
		
	char files[200];
	DIR* dirp = opendir(name);
	list<string> filesList;
	struct dirent * dp;
	
	while ((dp = readdir(dirp)) != NULL) 
	{
	
		if(strcmp(dp->d_name,".")==0 || strcmp(dp->d_name,"..")==0 || string(dp->d_name).back()=='~')
			continue;
		
		filesList.push_back(dp->d_name);
		
	}
	closedir(dirp);
	filesList.sort();
	list<string>::iterator it2;

	
	for (it2 = filesList.begin(); it2 != filesList.end(); it2++)
	{  	
		strcat(files,(*it2).c_str());
		strncat(files,",",sizeof(","));
	}
	return string(files);
}
//find the client number
int findClientNum(string IP)
{
	char* clientIPList[5]={CLIENT0,CLIENT1,CLIENT2,CLIENT3,CLIENT4};
	
	int N=5; //Total number of Clients
	int client_num=0;
	for(int i=0;i<N;i++)// find the client number
	{
		if(IP==string(clientIPList[i]))
		{
			client_num=i;
			break;	
		}
	}
	return client_num;
}

//function to append the string to the file
void writeFile(char* filename,string appendText)
{

  	std::ofstream outfile;

  	outfile.open(filename, std::ios_base::app);
  	outfile << appendText<<endl;;


}
//function to get the last line from the file
string getLastLine(char* filename)
{
	std::string lastline;
	std::ifstream fs;
	fs.open(filename, std::fstream::in);
	if(fs.is_open())
	{
		//Got to the last character before EOF
		fs.seekg(-1, std::ios_base::end);
		if(fs.peek() == '\n')
		{
			//Start searching for \n occurrences
			fs.seekg(-1, std::ios_base::cur);
			int i = fs.tellg();
			for(i;i > 0; i--)
			{
				if(fs.peek() == '\n')
				{
 					 //Found
  					fs.get();
  					break;
				}		
				//Move one character back
				fs.seekg(i, std::ios_base::beg);
			}
		}

		getline(fs, lastline);
		std::cout << lastline << std::endl;
	}
	else
	{
		std::cout << "Could not find end line character" << std::endl;
		lastline="no content";
	}	
	return lastline;
}
// Make connection.. Server waits to connect to all threads
int makeConnection(list <SocketWrapper> *SocketConnectionList)
{	
	cout<<"Main Client Thread created"<<endl;
	
	char* clientIPList[5]={CLIENT0,CLIENT1,CLIENT2,CLIENT3,CLIENT4};
	char* serverIPList[5]={SERVER0,SERVER1,SERVER2};
	int connection_start=0,status,client_num,flag=0,rc;
	char *setup_msg = "received";
	cout<<"My Ip address is::"<<getIPAddress()<<endl;
	
	Socket_connection s1;
	s1.listen_socket();
	while(!(connection_start==1))
	{	
		
		int stat=s1.return_accept_response();
		
		cout<<"listened.."<<endl;
		if (stat==1) 
		{	
			
						
			send(s1.return_accept_socket(), setup_msg , strlen(setup_msg) , 0 ); 

			SocketWrapper w1;
			w1.sender=inet_ntoa(s1.address.sin_addr);
			w1.receiver=getIPAddress();
			w1.socket_id=s1;
			w1.connect_num=findClientNum(string(inet_ntoa(s1.address.sin_addr)));
			pthread_mutex_lock(&list_lock);
			SocketConnectionList->push_back(w1);
			pthread_mutex_unlock(&list_lock);
			cout<<"Receiver - connected from "<<inet_ntoa(s1.address.sin_addr) <<" to "<<getIPAddress()<<endl;
			
			if (SocketConnectionList->size()==5)
				connection_start=1;
									
		}
		else
		{	
			cout<<"couldnt connect to the socket-receiver side"<<endl;
		
		}
		
		
	}
		
	cout<<"************Connection completed*************"<<endl;	
	return 1;
}


struct Reply_thread_data
{
	int socket;
	int client_num;
};
//Respond to the Client request of ENQUIRE,WRITE and READ
void *REPLY_CS(void *threadarg)
{
	struct Reply_thread_data *data;
	data = (struct Reply_thread_data *) threadarg;
	char finish_msg[100] ={0};
	char directory[200]={0};
	char filesList[100]={0};
	char* dir="/home/eng/z/zxw180035/Ricart-Agrawala/";
	while(1)
	{
		
		char buf[100]={0};
		//pthread_mutex_lock(&resource_lock);
		int valread = read(data->socket, buf, 100); 
		//pthread_mutex_unlock(&resource_lock);
		string buffer(buf);
		
		size_t found0 = buffer.find("ENQUIRE");
		size_t found1 = buffer.find("WRITE");
		size_t found2 = buffer.find("READ");
		std::string delimiter = ":";
		
		
		if(valread && (found0 != string::npos))
		{	
			
			int client,server;
			size_t pos = 0;
			std::string token;
			pos = buffer.find(":");
			token = buffer.substr(0, buffer.find(":"));
			buffer.erase(0, pos + delimiter.length());


			pos = buffer.find(":");
			token = buffer.substr(0, buffer.find(":"));
			buffer.erase(0, pos + delimiter.length()); 
	    		client=atoi(token.c_str()); 


			
			pos = buffer.find(":");
			token = buffer.substr(0, buffer.find(":"));
			buffer.erase(0, pos + delimiter.length());
			server=atoi(token.c_str());

			snprintf( directory, sizeof(directory), "%s%d", dir,server);
    			string ServerFiles=read_directory(directory);
			snprintf( filesList, sizeof(filesList), "%s", ServerFiles.c_str());
			send(data->socket, filesList , strlen(filesList) , 0 );

		}
		else if(valread && (found1 != string::npos))
		{
			string filename,writeString,server;
			size_t pos = 0;
			std::string token;
			pos = buffer.find(":");
			token = buffer.substr(0, buffer.find(":"));
			buffer.erase(0, pos + delimiter.length());


			pos = buffer.find(":");
			token = buffer.substr(0, buffer.find(":"));
			buffer.erase(0, pos + delimiter.length()); 
	    		filename=token;

			pos = buffer.find(":");
			token = buffer.substr(0, buffer.find(":"));
			buffer.erase(0, pos + delimiter.length()); 
	    		server=token;

			pos = buffer.find(":");
			token = buffer.substr(0, buffer.find(":"));
			buffer.erase(0, pos + delimiter.length()); 
	    		writeString=token;
			
			char temp1[200]={0};
			snprintf( temp1, sizeof(temp1), "%s%s/%s", dir,server.c_str(), filename.c_str());
			cout<<temp1<<endl;
			writeFile(temp1,writeString.c_str());
			cout<<"written to file :"<<filename<<endl;
			snprintf( finish_msg, sizeof(finish_msg), "%s%s", "written succesfully on file ",filename.c_str());
			send(data->socket, &finish_msg , strlen(finish_msg) , 0 );
		}
		else if(valread && (found2 != string::npos))
		{
			
			string filename,client,server;
			size_t pos = 0;
			std::string token;
			pos = buffer.find(":");
			token = buffer.substr(0, buffer.find(":"));
			buffer.erase(0, pos + delimiter.length());

			pos = buffer.find(":");
			token = buffer.substr(0, buffer.find(":"));
			buffer.erase(0, pos + delimiter.length()); 
	    		filename=token;
		
			pos = buffer.find(":");
			token = buffer.substr(0, buffer.find(":"));
			buffer.erase(0, pos + delimiter.length()); 
	    		server=token;

			pos = buffer.find(":");
			token = buffer.substr(0, buffer.find(":"));
			buffer.erase(0, pos + delimiter.length()); 
	    		client=token;
			
			char temp1[200]={0};
			snprintf( temp1, sizeof(temp1), "%s%s/%s", dir,server.c_str(), filename.c_str());
			string lastLine=getLastLine(temp1);
			cout<<"Client "<<client<<" is reading the last data from "<<filename<<" : "<<lastLine<<endl;
			if (lastLine.size()<3)
			{
				lastLine="no content";
			}
			snprintf( finish_msg, sizeof(finish_msg), "%s", lastLine.c_str());
			send(data->socket, &finish_msg , strlen(finish_msg) , 0 );
		}
		
	}

}
int ReplyClient(list <SocketWrapper> *SocketConnectionList)
{
	pthread_t REPLY[5];
	struct Reply_thread_data Rep[5];
	int l;
	list <SocketWrapper> :: iterator itt;
	for(l=0, itt = SocketConnectionList->begin(); itt != SocketConnectionList->end(); ++itt,l++) 
	{
		Rep[l].socket=((*itt).socket_id).return_accept_socket();
		Rep[l].client_num=(*itt).connect_num;
		int rc = pthread_create(&REPLY[l], NULL, REPLY_CS, (void *)&Rep[l]);
		if (rc)
		{
			cout<<"Problem with the creating Reply Thread.."<<endl;
			return 0;	
		}
	}
	

}
struct exit_app
{
	list <SocketWrapper> *SocketConnectionList;
	
};
//thread to close all the sockets and exit application 
void *exit_fn(void *threadarg)
{
	struct exit_app *data;
	data = (struct exit_app *) threadarg;
	int l;
	list <SocketWrapper> :: iterator itt;
	list <SocketWrapper> :: iterator it;
	string exit_msg;
	cin>>exit_msg;
	if(exit_msg=="exit")
	{
		for(itt = (data->SocketConnectionList)->begin(); itt != (data->SocketConnectionList)->end(); ++itt) 
		{
			close(((*itt).socket_id).socket_fd);
			close(((*itt).socket_id).new_socket);
		}
		
		cout<<"SYSTEM EXIT"<<endl;
		exit(0);// force all thread to exit
	}
	else
	{
		cout<<"Not a Valid exit message"<<endl;
	}
}






//Main Function
int main()
{	
	
	list <SocketWrapper> SocketConnectionList;
	
	int status=makeConnection(&SocketConnectionList);
	if(status==0)
	{
		cout<<"problem with connection"<<endl;
		return 1;	
	}
	pthread_t exit_application;
	struct exit_app e;
	e.SocketConnectionList=&SocketConnectionList;
	
	int rc = pthread_create(&exit_application, NULL, exit_fn, (void *)&e);
	if (rc)
	{
		cout<<"Problem with the creating Reply Thread.."<<endl;
		return 0;	
	}
	int stat=ReplyClient(&SocketConnectionList);
	if(stat==0)
	{
		cout<<"problem with connection"<<endl;
		return 1;	
	}
	
	while(1)
	{
	}
	
		
	return 0;
}

// #include <ifaddrs.h>
// #include "connection.h"
// #include <thread>
// using namespace std;
// #define PORT  2408
// #define FALSE            0
// //global mutuex
// pthread_mutex_t list_lock;
// pthread_mutex_t resource_lock;



// //Function
// //Grab the IP Address of the current machine
// string getIPAddress()
// {
//     	string ipAddress="Unable to get IP Address";
//     	struct ifaddrs *interfaces = NULL;
//     	struct ifaddrs *temp_addr = NULL;
//     	int success = 0;
    
//     	success = getifaddrs(&interfaces);
//     	if (success == 0) 
// 	{
        
//         	temp_addr = interfaces;
//         	while(temp_addr != NULL) 
// 		{
//             		if(temp_addr->ifa_addr->sa_family == AF_INET) 
// 			{
                
//                 		if(strcmp(temp_addr->ifa_name, "en0"))
// 				{
//                     			ipAddress=inet_ntoa(((struct sockaddr_in*)temp_addr->ifa_addr)->sin_addr);
//                 		}
//             		}
//             	temp_addr = temp_addr->ifa_next;
//         	}
//     	}
   
//     	freeifaddrs(interfaces);
//     	return ipAddress;
// }
// ///Creating a class to deal with lower socket 
// class Socket_connection
// {
// public:
// 	int socket_fd, new_socket, valread; 
// 	struct sockaddr_in address; 
// 	int opt = 1; 
// 	int addrlen = sizeof(address); 
// 	int iMode=0;
// 	Socket_connection()
// 	{
// 		// Creating socket file descriptor 
// 		if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) 
// 		{ 
// 			perror("socket failed"); 
// 			exit(EXIT_FAILURE); 
// 		} 
// 		bzero((char *) &address, sizeof(address));
// 		ioctl(socket_fd, FIONBIO, &iMode); 
// 		address.sin_port = htons( PORT );
// 		address.sin_family = AF_INET;
		 
// 	}
// 	Socket_connection(const Socket_connection &sock)  //copy constructor
// 	{
// 	socket_fd=sock.socket_fd;
// 	new_socket=sock.new_socket; 
// 	valread=sock.valread;
// 	address=sock.address; 
// 	opt=1;
// 	addrlen =sock.addrlen;
// 	iMode=0;

// 	}
// 	~Socket_connection()  //destructor
// 	{
	
// 	}
// 	int connect_socket(char* IPname)
// 	{	
		
		
// 		if(inet_pton(AF_INET, IPname, &address.sin_addr)<=0)  
//     		{ 
//         		cout<<"Invalid address/ Address not supported "<<endl; 
//         		return 0; 
//     		}		
		
// 		if(connect(socket_fd, (struct sockaddr *)&address, sizeof(address))<0) 
// 		{
// 			cout<<"Connection Failed "<<endl; 
//         		return 0;
// 		}
// 		else
// 		{
// 			return 1;
// 		}
		
		
		
    		
// 	}
// 	int listen_socket()
// 	{	
		
// 		// Forcefully attaching socket to the port 2408 
// 		if (setsockopt(socket_fd, SOL_SOCKET, (SO_REUSEPORT | SO_REUSEADDR),&opt, sizeof(opt))) 
// 		{ 
// 		perror("setsockopt"); 
// 		exit(EXIT_FAILURE); 
// 		} 
// 		address.sin_addr.s_addr = INADDR_ANY; 
		
// 		// Forcefully attaching socket to the port 8080 
// 		if (bind(socket_fd, (struct sockaddr *)&address, sizeof(address))<0) 
// 		{ 
// 		perror("bind failed"); 
// 		return 0; 
// 		} 
		
// 		if (listen(socket_fd, 32) < 0) 
// 		{ 
// 		perror("listen failed"); 
// 		return 0; 
// 		} 
		
// 	}
// 	int return_accept_response()
// 	{
// 		cout<<"waiting to connect here"<<endl;
// 		if ((new_socket = accept(socket_fd, (struct sockaddr *)&address,(socklen_t*)&addrlen))<0) 
// 		{ 
// 		perror("accept failed"); 
// 		return 0; 
// 		} 
// 		else
// 		{
// 			return 1;
// 		}
		
// 	}
// 	int return_accept_socket()
// 	{
// 		return new_socket;
// 	}
	
			
// };

// //Creating a wrapper for the socket with sender and receiver information
// class SocketWrapper
// {
// public:
// 	string sender;
// 	string receiver;
// 	Socket_connection socket_id;
// 	int connect_num;
// 	SocketWrapper()
// 	{
// 		sender="";
// 		receiver="";

// 	}
// 	SocketWrapper(const SocketWrapper &wrap)//copy constructor
// 	{
// 		sender=wrap.sender;
// 		receiver=wrap.receiver;
// 		socket_id=wrap.socket_id;
// 		connect_num=wrap.connect_num;

// 	}
// 	~SocketWrapper()//destructor
// 	{
// 	}
	
// };

// ///function
// //to read all the files present in the directory
// string read_directory(char* name)
// {
		
// 	char files[200];
// 	DIR* dirp = opendir(name);
// 	list<string> filesList;
// 	struct dirent * dp;
	
// 	while ((dp = readdir(dirp)) != NULL) 
// 	{
	
// 		if(strcmp(dp->d_name,".")==0 || strcmp(dp->d_name,"..")==0 || string(dp->d_name).back()=='~')
// 			continue;
		
// 		filesList.push_back(dp->d_name);
		
// 	}
// 	closedir(dirp);
// 	filesList.sort();
// 	list<string>::iterator it2;

	
// 	for (it2 = filesList.begin(); it2 != filesList.end(); it2++)
// 	{  	
// 		strcat(files,(*it2).c_str());
// 		strncat(files,",",sizeof(","));
// 	}
// 	return string(files);
// }
// //find the client number
// int findClientNum(string IP)
// {
// 	char* clientIPList[5]={CLIENT0,CLIENT1,CLIENT2,CLIENT3,CLIENT4};
	
// 	int N=5; //Total number of Clients
// 	int client_num=0;
// 	for(int i=0;i<N;i++)// find the client number
// 	{
// 		if(IP==string(clientIPList[i]))
// 		{
// 			client_num=i;
// 			break;	
// 		}
// 	}
// 	return client_num;
// }

// //function to append the string to the file
// void writeFile(char* filename,string appendText)
// {

//   	std::ofstream outfile;

//   	outfile.open(filename, std::ios_base::app);
//   	outfile << appendText<<endl;;


// }
// //function to get the last line from the file
// string getLastLine(char* filename)
// {
// 	std::string lastline;
// 	std::ifstream fs;
// 	fs.open(filename, std::fstream::in);
// 	if(fs.is_open())
// 	{
// 		//Got to the last character before EOF
// 		fs.seekg(-1, std::ios_base::end);
// 		if(fs.peek() == '\n')
// 		{
// 			//Start searching for \n occurrences
// 			fs.seekg(-1, std::ios_base::cur);
// 			int i = fs.tellg();
// 			for(i;i > 0; i--)
// 			{
// 				if(fs.peek() == '\n')
// 				{
//  					 //Found
//   					fs.get();
//   					break;
// 				}		
// 				//Move one character back
// 				fs.seekg(i, std::ios_base::beg);
// 			}
// 		}

// 		getline(fs, lastline);
// 		std::cout << lastline << std::endl;
// 	}
// 	else
// 	{
// 		std::cout << "Could not find end line character" << std::endl;
// 		lastline="no content";
// 	}	
// 	return lastline;
// }
// // Make connection.. Server waits to connect to all threads
// int makeConnection(list <SocketWrapper> *SocketConnectionList)
// {	
// 	cout<<"Main Client Thread created"<<endl;
	
// 	char* clientIPList[5]={CLIENT0,CLIENT1,CLIENT2,CLIENT3,CLIENT4};
// 	char* serverIPList[5]={SERVER0,SERVER1,SERVER2};
// 	int connection_start=0,status,client_num,flag=0,rc;
// 	char *setup_msg = "received";
// 	cout<<"My Ip address is::"<<getIPAddress()<<endl;
	
// 	Socket_connection s1;
// 	s1.listen_socket();
// 	while(!(connection_start==1))
// 	{	
		
// 		int stat=s1.return_accept_response();
		
// 		cout<<"listened.."<<endl;
// 		if (stat==1) 
// 		{	
			
						
// 			send(s1.return_accept_socket(), setup_msg , strlen(setup_msg) , 0 ); 

// 			SocketWrapper w1;
// 			w1.sender=inet_ntoa(s1.address.sin_addr);
// 			w1.receiver=getIPAddress();
// 			w1.socket_id=s1;
// 			w1.connect_num=findClientNum(string(inet_ntoa(s1.address.sin_addr)));
// 			pthread_mutex_lock(&list_lock);
// 			SocketConnectionList->push_back(w1);
// 			pthread_mutex_unlock(&list_lock);
// 			cout<<"Receiver - connected from "<<inet_ntoa(s1.address.sin_addr) <<" to "<<getIPAddress()<<endl;
			
// 			if (SocketConnectionList->size()==5)
// 				connection_start=1;
									
// 		}
// 		else
// 		{	
// 			cout<<"couldnt connect to the socket-receiver side"<<endl;
		
// 		}
		
		
// 	}
		
// 	cout<<"************Connection completed*************"<<endl;	
// 	return 1;
// }


// struct Reply_thread_data
// {
// 	int socket;
// 	int client_num;
// };
// //Respond to the Client request of ENQUIRE,WRITE and READ
// void *REPLY_CS(void *threadarg)
// {
// 	struct Reply_thread_data *data;
// 	data = (struct Reply_thread_data *) threadarg;
// 	char finish_msg[100] ={0};
// 	char directory[200]={0};
// 	char filesList[100]={0};
// 	char* dir="/home/eng/z/zxw180035/Ricart-Agrawala/";
// 	while(1)
// 	{
		
// 		char buf[100]={0};
// 		//pthread_mutex_lock(&resource_lock);
// 		int valread = read(data->socket, buf, 100); 
// 		//pthread_mutex_unlock(&resource_lock);
// 		string buffer(buf);
		
// 		size_t found0 = buffer.find("ENQUIRE");
// 		size_t found1 = buffer.find("WRITE");
// 		size_t found2 = buffer.find("READ");
// 		std::string delimiter = ":";
		
		
// 		if(valread && (found0 != string::npos))
// 		{	
			
// 			int client,server;
// 			size_t pos = 0;
// 			std::string token;
// 			pos = buffer.find(":");
// 			token = buffer.substr(0, buffer.find(":"));
// 			buffer.erase(0, pos + delimiter.length());


// 			pos = buffer.find(":");
// 			token = buffer.substr(0, buffer.find(":"));
// 			buffer.erase(0, pos + delimiter.length()); 
// 	    		client=atoi(token.c_str()); //client that sends me this message


			
// 			pos = buffer.find(":");
// 			token = buffer.substr(0, buffer.find(":"));
// 			buffer.erase(0, pos + delimiter.length());
// 			server=atoi(token.c_str());  

// 			snprintf( directory, sizeof(directory), "%s%d", dir,server);
//     			string ServerFiles=read_directory(directory);
// 			snprintf( filesList, sizeof(filesList), "%s", ServerFiles.c_str());
// 			send(data->socket, filesList , strlen(filesList) , 0 );

// 		}
// 		else if(valread && (found1 != string::npos))
// 		{
// 			string filename,writeString,server;
// 			size_t pos = 0;
// 			std::string token;
// 			pos = buffer.find(":");
// 			token = buffer.substr(0, buffer.find(":"));
// 			buffer.erase(0, pos + delimiter.length());


// 			pos = buffer.find(":");
// 			token = buffer.substr(0, buffer.find(":"));
// 			buffer.erase(0, pos + delimiter.length()); 
// 	    		filename=token;

// 			pos = buffer.find(":");
// 			token = buffer.substr(0, buffer.find(":"));
// 			buffer.erase(0, pos + delimiter.length()); 
// 	    		server=token;

// 			pos = buffer.find(":");
// 			token = buffer.substr(0, buffer.find(":"));
// 			buffer.erase(0, pos + delimiter.length()); 
// 	    		writeString=token;
			
// 			char temp1[200]={0};
// 			snprintf( temp1, sizeof(temp1), "%s%s/%s", dir,server.c_str(), filename.c_str());
// 			cout<<temp1<<endl;
// 			writeFile(temp1,writeString.c_str());
// 			cout<<"written to file :"<<filename<<endl;
// 			snprintf( finish_msg, sizeof(finish_msg), "%s%s", "written succesfully on file ",filename.c_str());
// 			send(data->socket, &finish_msg , strlen(finish_msg) , 0 );
// 		}
// 		else if(valread && (found2 != string::npos))
// 		{
			
// 			string filename,client,server;
// 			size_t pos = 0;
// 			std::string token;
// 			pos = buffer.find(":");
// 			token = buffer.substr(0, buffer.find(":"));
// 			buffer.erase(0, pos + delimiter.length());

// 			pos = buffer.find(":");
// 			token = buffer.substr(0, buffer.find(":"));
// 			buffer.erase(0, pos + delimiter.length()); 
// 	    		filename=token;
		
// 			pos = buffer.find(":");
// 			token = buffer.substr(0, buffer.find(":"));
// 			buffer.erase(0, pos + delimiter.length()); 
// 	    		server=token;

// 			pos = buffer.find(":");
// 			token = buffer.substr(0, buffer.find(":"));
// 			buffer.erase(0, pos + delimiter.length()); 
// 	    		client=token;
			
// 			char temp1[200]={0};
// 			snprintf( temp1, sizeof(temp1), "%s%s/%s", dir,server.c_str(), filename.c_str());
// 			string lastLine=getLastLine(temp1);
// 			cout<<"Client "<<client<<" is reading the last data from "<<filename<<" : "<<lastLine<<endl;
// 			if (lastLine.size()<3)
// 			{
// 				lastLine="no content";
// 			}
// 			snprintf( finish_msg, sizeof(finish_msg), "%s", lastLine.c_str());
// 			send(data->socket, &finish_msg , strlen(finish_msg) , 0 );
// 		}
		
// 	}

// }
// int ReplyClient(list <SocketWrapper> *SocketConnectionList)
// {
// 	pthread_t REPLY[5];
// 	struct Reply_thread_data Rep[5];
// 	int l;
// 	list <SocketWrapper> :: iterator itt;
// 	for(l=0, itt = SocketConnectionList->begin(); itt != SocketConnectionList->end(); ++itt,l++) 
// 	{
// 		Rep[l].socket=((*itt).socket_id).return_accept_socket();
// 		Rep[l].client_num=(*itt).connect_num;
// 		int rc = pthread_create(&REPLY[l], NULL, REPLY_CS, (void *)&Rep[l]);
// 		if (rc)
// 		{
// 			cout<<"Problem with the creating Reply Thread.."<<endl;
// 			return 0;	
// 		}
// 	}
	

// }
// struct exit_app
// {
// 	list <SocketWrapper> *SocketConnectionList;
	
// };
// //thread to close all the sockets and exit application 
// void *exit_fn(void *threadarg)
// {
// 	struct exit_app *data;
// 	data = (struct exit_app *) threadarg;
// 	int l;
// 	list <SocketWrapper> :: iterator itt;
// 	list <SocketWrapper> :: iterator it;
// 	string exit_msg;
// 	cin>>exit_msg;
// 	if(exit_msg=="exit")
// 	{
// 		for(itt = (data->SocketConnectionList)->begin(); itt != (data->SocketConnectionList)->end(); ++itt) 
// 		{
// 			close(((*itt).socket_id).socket_fd);
// 			close(((*itt).socket_id).new_socket);
// 		}
		
// 		cout<<"SYSTEM EXIT"<<endl;
// 		exit(0);// force all thread to exit
// 	}
// 	else
// 	{
// 		cout<<"Not a Valid exit message"<<endl;
// 	}
// }






// //Main Function
// int main()
// {	
	
// 	list <SocketWrapper> SocketConnectionList;
	
// 	int status=makeConnection(&SocketConnectionList);
// 	if(status==0)
// 	{
// 		cout<<"problem with connection"<<endl;
// 		return 1;	
// 	}
// 	pthread_t exit_application;
// 	struct exit_app e;
// 	e.SocketConnectionList=&SocketConnectionList;
	
// 	int rc = pthread_create(&exit_application, NULL, exit_fn, (void *)&e);
// 	if (rc)
// 	{
// 		cout<<"Problem with the creating Reply Thread.."<<endl;
// 		return 0;	
// 	}
// 	int stat=ReplyClient(&SocketConnectionList);
// 	if(stat==0)
// 	{
// 		cout<<"problem with connection"<<endl;
// 		return 1;	
// 	}
	
// 	while(1)
// 	{
// 	}
	
		
// 	return 0;
// }
