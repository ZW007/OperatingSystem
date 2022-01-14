
#include "connection.h"

using namespace std;
#define PORT  2408
#define TRUE             1
#define FALSE            0

//Global mutex for receiving threads
pthread_mutex_t list_lock;
pthread_mutex_t server_list_lock;
pthread_mutex_t struct_lock;
pthread_mutex_t Id_thread_lock;
//Global variable for thread control
int Id_thread=0;
int connect_complete=0; 



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
////Struct to pass multiple parameter for all threads
struct Request_thread_data
{
	int client_num;
	int *Seq_num;
	int socket;
	int resource_num;

};
struct Reply_thread_data_array
{
	struct Reply_thread_data *Reply;
};
struct Reply_thread_data
{
	int client_num;
	int Seq_num=0;
	int Highest_Seq_num;
	bool Reply_Deferred[5];
	bool Using;
	bool Waiting;
	bool A[5];
	int socket[5];

};
struct Client_send_data
{
	int client_num;
	list <SocketWrapper> *SocketConnectionList;
	list <SocketWrapper> *SocketConnectionListServer;
	int over_ride;

};

//Function returns the current client number with the ip passed as parameter
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

//Thread function to create a client connection.
//This connects to all clients and server and returns the List of SockerWrapper information about connected clients and server
//The connection triggering client sends connection to all the servers and clients after it.
// Eg: CLIENT0 connects to all servers and CLIENT1,2,3,4
// Eg: When CLIENT1 receives the connection from O, it connects to remaining servers and CLIENT2,3,4 and so on.
void *SendClientConnection(void *threadarg)
{
	char* clientIPList[5]={CLIENT0,CLIENT1,CLIENT2,CLIENT3,CLIENT4};
	char* serverIPList[5]={SERVER0,SERVER1,SERVER2};	
	struct Client_send_data *data;
	data = (struct Client_send_data *) threadarg;
	char *end_msg = "end_connect";
	char *setup_msg = "start";
	int valread=0;
	int received_pre_msg=0;	
	
	for(int i=0;i<3;i++)
	{
		
		Socket_connection s1;
		SocketWrapper w1;
		w1.sender=getIPAddress();
		w1.receiver=serverIPList[i];
		w1.connect_num=i;
		//connecting the clients in mesh
		int stat=s1.connect_socket(serverIPList[i]);
		w1.socket_id=s1;
		if (stat==1)
		{	
				
			char buf[1024]={0};
			valread = read(s1.socket_fd  , buf, 1024); 
			
			if(valread && (strcmp(buf, "received") == 0))
			{
				pthread_mutex_lock(&server_list_lock);
				(data->SocketConnectionListServer)->push_back(w1); //need mutex here
				pthread_mutex_unlock(&server_list_lock);
				cout<<"Sender - connected "<< w1.sender <<" to "<< w1.receiver <<endl;
			}
		}
		else
		{
			cout<<"error in sending the client connect.."<<endl;
		
		}
		

	}	
	for(int i=4;i>(data->client_num);i--)
	{
		
		Socket_connection s1; //created for the sender clients
		SocketWrapper w1;
		w1.sender=getIPAddress();
		w1.receiver=clientIPList[i];
		w1.connect_num=findClientNum(string(clientIPList[i]));
		//connecting the clients in mesh
		int stat=s1.connect_socket(clientIPList[i]);
		w1.socket_id=s1;
		if (stat==1)
		{	
			
			char buf[1024]={0};
			valread = read(s1.socket_fd  , buf, 1024); 
			
			if(valread && (strcmp(buf, "received") == 0))
			{
				
				pthread_mutex_lock(&list_lock);
				(data->SocketConnectionList)->push_back(w1); //need mutex here
				pthread_mutex_unlock(&list_lock);
				cout<<"Sender - connected "<< w1.sender <<" to "<< w1.receiver <<endl;
			}
		}
		else
		{
			cout<<"error in sending the client connect.."<<endl;
		
		}
		

	}
	
	if(data->client_num==4)
	{
		list <SocketWrapper> :: iterator it;	
		for(it = (data->SocketConnectionList)->begin(); it != (data->SocketConnectionList)->end(); ++it) 
		{
			//if((*it)->sender==string(clientIPList[0]))								
			if(send(((*it).socket_id).return_accept_socket() , end_msg , strlen(end_msg) , 0 )<0)
			{
				cout<<"error in sending msg.."<<endl;	
			}
			cout<<"End message sent to "<< (*it).sender<<endl;
			
								
		}		
		connect_complete=1;


	}
	else
	{	
		list <SocketWrapper> :: iterator it;
		for(it = (data->SocketConnectionList)->begin(); it != (data->SocketConnectionList)->end(); ++it) 
		{
			cout<<"reading list "<<(*it).receiver<<" to  "<<(*it).sender<<endl;
			if((*it).receiver==string(clientIPList[4]))
			{
				
				cout<<"here"<<endl;
				char buf[1024]={0};
				valread = read(((*it).socket_id).socket_fd, buf, 1024); 
				cout<<"wait for end msg from "<<(*it).receiver<<" to me "<<(*it).sender<<endl;
				if(valread && (strcmp(buf, "end_connect") == 0))
				{
					//received_pre_msg=1;						
					//cout<<string(buf)<<" #### "<<endl;
					connect_complete=1; 
					break;
				}
					
			}
		}
	}
		
	
}
//Main function to trigger connection to other clients and Servers in thread
int makeConnection(list <SocketWrapper> *SocketConnectionList,list <SocketWrapper> *SocketConnectionListServer)
{	
	cout<<"Main Client Thread created"<<endl;
	char* clientIPList[5]={CLIENT0,CLIENT1,CLIENT2,CLIENT3,CLIENT4};
	char* serverIPList[5]={SERVER0,SERVER1,SERVER2};
	int connection_start=0,status,client_num,flag=0,rc;
	char *setup_msg = "received";
	cout<<"My Ip address is::"<<getIPAddress()<<endl;
	
	for(int i=0;i<5;i++)// find the client num 
	{
		if(getIPAddress()==string(clientIPList[i]))
		{
			client_num=i;
			break;	
		}
	}
	
	if (client_num==0)
	{	
		cout<<"Enter 1 to setup client connection: ";
		cin>>connection_start;
		
		if (!(connection_start==1))
		{	
			cout<<"invalid parameter.. enter 1 to setup connecction ..exiting...";
			return 0;
					
		}
		
	}
	Socket_connection s1;
	s1.listen_socket();
	// first client this value isnt 1, so it skips this func, while other clients run this func
	while(!(connection_start==1)) 
	{	
		
		int stat=s1.return_accept_response();
		
		cout<<"listened.."<<endl;
		if (stat==1) //when return_accept_response returns true 
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
			//cout<<"connected"<<endl;
			if (string(clientIPList[client_num-1])==string(inet_ntoa(s1.address.sin_addr)))
				connection_start=1;
									
		}
		else
		{	
			cout<<"couldnt connect to the socket-receiver side"<<endl;
		
		}
		
		
	}
	
	pthread_t SendConnectThread; 
	struct Client_send_data t;				
	t.SocketConnectionList=SocketConnectionList;
	t.SocketConnectionListServer=SocketConnectionListServer;
	t.client_num=client_num;
	t.over_ride=0;
	rc=pthread_create(&SendConnectThread, NULL, SendClientConnection, (void *)&t);
	if (rc)
	{
		cout<<"problem in creating thread"<<endl;
	}
	//wait here untill all the triggered connections are complete
	while(!connect_complete);
	cout<<"Connection completed"<<endl;	
	
}

//After connection talks with the Server, send ENQUIRE message to get total number of files and filenames in the server 
int askServerFile(list <SocketWrapper> *SocketConnectionListServer,list <string> *files, int client_num)
{
	//talk with the random server
	int random_server=rand()%3;
	char req_msg[100]={0};
	list <SocketWrapper> :: iterator it1;
	for(it1 = SocketConnectionListServer->begin(); it1 != SocketConnectionListServer->end(); ++it1) 
	{
		if((*it1).connect_num==random_server)
		{
			
			snprintf( req_msg, sizeof(req_msg), "%s:%d:%d", "ENQUIRE",client_num,random_server);
			send(((*it1).socket_id).socket_fd,&req_msg , strlen(req_msg) , 0);
			char buf1[100]={0};
			//sleep(1);
			int valread = read(((*it1).socket_id).socket_fd, buf1, 100); 
			string buffer(buf1);
			cout<<"From server "<<buf1<<endl;
			
			std::string delimiter = ",";

			size_t pos = 0;
			std::string token;
			while ((pos = buffer.find(",")) != std::string::npos) 
			{
			    token = buffer.substr(0, pos);
			    files->push_back(token);
			    buffer.erase(0, pos + delimiter.length());
			}
			break;
		}
	}
	
}



void *REQUEST_CS(void *threadarg)
{
	struct Request_thread_data *data;
	data = (struct Request_thread_data *) threadarg;
	
	char req_msg[100]={0};
	
	snprintf( req_msg, sizeof(req_msg), "%s:%d:%d:%d", "REQ", data->client_num,*data->Seq_num,data->resource_num);

	send(data->socket, &req_msg , strlen(req_msg) , 0 );
	
}
//RICART AGRAWALA Algorithm with Optimization - receiving threads
//Each Machine will invoke 4 always live threads for replying other clients messages
//contains pseudo code from the algorithm - TREAT REQUEST MESSAGE & TREAT REPLY MESSAGE
void *REPLY_CS(void *threadarg)
{
	struct Reply_thread_data_array *data;
	data = (struct Reply_thread_data_array *) threadarg;
	int resource_num;
	int Client_Req_num;// j -according to algorithm
	int Client_Rep_num;
	int Req_Seq_num;//k
	char* reply_msg;
	char* temp;
	int temp_thread_id;
	pthread_mutex_lock(&Id_thread_lock);
	temp_thread_id=Id_thread;
	if(temp_thread_id==data->Reply->client_num)
	{
		Id_thread++;
		temp_thread_id=Id_thread;
	}
	Id_thread++;
	pthread_mutex_unlock(&Id_thread_lock);
	
	
	bool Our_Priority=FALSE;
	

	while(1)
	{	
		
		char buf[100]={0};
		
		int valread = read(data->Reply->socket[temp_thread_id], buf, 100); 
		string buffer(buf);
		
		size_t found = buffer.find("REPLY");
  		size_t found1 = buffer.find("REQ");
		
		if(valread && found!=string::npos)
		{
			std::string delimiter = ":";
			//Parse the REPLY
			size_t pos = 0;
			std::string token;
			pos = buffer.find(":");
			token = buffer.substr(0, buffer.find(":"));
			buffer.erase(0, pos + delimiter.length());
			pos = buffer.find(":");
			token = buffer.substr(0, buffer.find(":"));
			buffer.erase(0, pos + delimiter.length());
			Client_Rep_num=atoi(token.c_str());	
			
			pos = buffer.find(":");
			token = buffer.substr(0, buffer.find(":"));
			
			resource_num=atoi(token.c_str());
			
			pthread_mutex_lock(&struct_lock);
			(data->Reply)[resource_num].A[Client_Rep_num]=TRUE;
			pthread_mutex_unlock(&struct_lock);
			
			valread=0;
		}
		else if (valread && found1!=string::npos)
		{
			std::string delimiter = ":";
			//Parse the REQUEST
			size_t pos = 0;
			std::string token;
			pos = buffer.find(":");
			token = buffer.substr(0, buffer.find(":"));
			buffer.erase(0, pos + delimiter.length());


			pos = buffer.find(":");
			token = buffer.substr(0, buffer.find(":"));
			buffer.erase(0, pos + delimiter.length()); 
	    		Client_Req_num=atoi(token.c_str()); 


			
			pos = buffer.find(":");
			token = buffer.substr(0, buffer.find(":"));
			buffer.erase(0, pos + delimiter.length());
			Req_Seq_num=atoi(token.c_str());
			


			token = buffer.substr(0, buffer.find(":"));
			
			resource_num=atoi(token.c_str());
			
			pthread_mutex_lock(&struct_lock);
			(data->Reply)[resource_num].Highest_Seq_num=max((data->Reply)[resource_num].Highest_Seq_num,Req_Seq_num);
			pthread_mutex_unlock(&struct_lock);
			
			if( (Req_Seq_num > (data->Reply)[resource_num].Seq_num)  ||  ( (Req_Seq_num == (data->Reply)[resource_num].Seq_num) && (Client_Req_num > (data->Reply)[resource_num].client_num) ))
			{
				
				Our_Priority=TRUE;
				
			}
			
			if((data->Reply)[resource_num].Using ||((data->Reply)[resource_num].Waiting && Our_Priority))
			{
				pthread_mutex_lock(&struct_lock);
				(data->Reply)[resource_num].Reply_Deferred[Client_Req_num]=TRUE;
				pthread_mutex_unlock(&struct_lock);	
				cout<<"Deferred Reply to "<<Client_Req_num<<" for file:"<<resource_num<<endl;
			}
			
			if(  !((data->Reply)[resource_num].Using || (data->Reply)[resource_num].Waiting) || ((data->Reply)[resource_num].Waiting && !((data->Reply)[resource_num].A[Client_Req_num]) && !(Our_Priority) ))
			{
				pthread_mutex_lock(&struct_lock);
				(data->Reply)[resource_num].A[Client_Req_num]=FALSE;
				pthread_mutex_unlock(&struct_lock);
				char reply_msg[100]={0};	
				snprintf( reply_msg, sizeof(reply_msg), "%s:%d:%d", "REPLY",(data->Reply)[resource_num].client_num,resource_num);
				cout<<"Sent Reply to "<<Client_Req_num<<" for file:"<<resource_num<<endl;
				send((data->Reply)[resource_num].socket[temp_thread_id], &reply_msg , strlen(reply_msg) , 0 );
			}
			
			if((data->Reply)[resource_num].Waiting && (data->Reply)[resource_num].A[Client_Req_num] && !Our_Priority)
			{
				pthread_mutex_lock(&struct_lock);
				(data->Reply)[resource_num].A[Client_Req_num]=FALSE;
				pthread_mutex_unlock(&struct_lock);
				char reply_msg[100]={0};	
				snprintf( reply_msg, sizeof(reply_msg), "%s:%d:%d", "REPLY",(data->Reply)[resource_num].client_num,resource_num);
				cout<<"Sent Reply to "<<Client_Req_num<<" for file:"<<resource_num<<endl;
				send(data->Reply->socket[temp_thread_id], &reply_msg , strlen(reply_msg) , 0 );

				char req_msg[100]={0};
				snprintf( req_msg, sizeof(req_msg), "%s:%d:%d:%d", "REQ", (data->Reply)[resource_num].client_num,(data->Reply)[resource_num].Seq_num,resource_num);
				send((data->Reply)[resource_num].socket[temp_thread_id], &req_msg , strlen(req_msg) , 0 );
				cout<<"Sent Reply to "<<Client_Req_num<<" for file:"<<resource_num<<endl;
			}
			
		}
		
	
	}

}

// RICART AGRAWALA ALGORITHM WITH OPTIMIZATION
// contains pseudo code REQUEST RESOURCE and RELEASE RESOURCE


int ricart_agrawala_algo(int N,int client_num,list <SocketWrapper> *SocketConnectionList,list <SocketWrapper> *SocketConnectionListServer,list <string> *ServerFiles)
{
	
	
	char reply[100]={0};
	char ServerReq[100]={0};	
	///thread
	//create a thread for each other client socket to reply
	pthread_t REPLY[4];
	struct Reply_thread_data *Rep=(struct Reply_thread_data *)malloc(sizeof(struct Reply_thread_data)*ServerFiles->size());
	
	for(int f=0;f<ServerFiles->size();f++)
	{
		Rep[f].Seq_num=5;
		Rep[f].Highest_Seq_num=0;
		
		Rep[f].client_num=client_num;
		Rep[f].Using=FALSE;
		Rep[f].Waiting=FALSE;
		for(int n=0;n<5;n++)
		{
			Rep[f].A[n]={FALSE};
			Rep[f].Reply_Deferred[n]={0};
		}
	
		int l;
		list <SocketWrapper> :: iterator itt;
		for(l=0, itt = SocketConnectionList->begin(); itt != SocketConnectionList->end(); ++itt,l++) 
		{		
		
			if((*itt).connect_num>client_num)
			{
				Rep[f].socket[(*itt).connect_num]=((*itt).socket_id).socket_fd;
			}
			else
			{
				Rep[f].socket[(*itt).connect_num]=((*itt).socket_id).return_accept_socket();
			}
			
		}
	}
	struct Reply_thread_data_array RepPointer;
	RepPointer.Reply=Rep;
	for(int r=0;r<5;r++)
	{
		if (r==client_num)
			continue;
		int rc = pthread_create(&REPLY[r], NULL, REPLY_CS, (void *)&RepPointer);
		if (rc)
		{
			cout<<"Problem with the creating Reply Thread.."<<endl;
			return 0;	
		}
	}
	//send request to all other clients
	pthread_t REQ[5];
	struct Request_thread_data Req[5];
	int s;
	list <SocketWrapper> :: iterator it;

	//entiring to critical section.
	for(int count=0;count<20;count++)
	{
		int sleep_time=rand()%10;
		cout<<"My Sleep time is "<<sleep_time<<" sec "<<endl;
		usleep(sleep_time*1000000);
		int resource_file=rand()%ServerFiles->size();
		
		int R_W=rand()%2;
		list <string> :: iterator file;
		int k;
		string resource;
		for(k=0, file = ServerFiles->begin(); file != ServerFiles->end(); ++file,k++) 
		{
			if(k==resource_file)
			{
				resource=*file;
				break;
			}
    			
		}
		if(R_W==1)
			cout<<"Requesting to write file "<<resource<<endl;
		else
			cout<<"Requesting to read file "<<resource<<endl;
		cout<<"Request entry to the Critical Section"<<endl;
		pthread_mutex_lock(&struct_lock);
		Rep[resource_file].Waiting=TRUE;
		Rep[resource_file].Seq_num=Rep[resource_file].Highest_Seq_num+1;
		pthread_mutex_unlock(&struct_lock);
		for(s=0, it = SocketConnectionList->begin(); it != SocketConnectionList->end(); ++it,s++) 
		{
			
			Req[s].client_num=client_num;
			Req[s].Seq_num=&Rep[resource_file].Seq_num;
			Req[s].resource_num=resource_file;
			if((*it).connect_num>client_num)
			{
				Req[s].socket=((*it).socket_id).socket_fd;
			}
			else
			{
				Req[s].socket=((*it).socket_id).return_accept_socket();
			}
			
			int rc = pthread_create(&REQ[s], NULL, REQUEST_CS, (void *)&Req[s]);
			if (rc)
			{
				cout<<"Problem with the creating Request Thread.."<<endl;
				return 0;	
			}

		}
		for(int i=0;i<N;i++)   //optimization waiting
		{	
			if(i==client_num)
				continue;
			while(!Rep[resource_file].A[i]);
						
		}
		//pthread_mutex_lock(&struct_lock);
		Rep[resource_file].Waiting=FALSE;
		Rep[resource_file].Using=TRUE;
		//pthread_mutex_unlock(&struct_lock);
		
		cout<<"PERFORM CRITICAL SECTION"<<endl;
		
		int server_no=rand() % 3;//contact a random server to write to file
		
		
		if (R_W==1)
		{	
			cout<<"Writing to all Servers"<<endl;
			list <SocketWrapper> :: iterator it1;
			for(it1 = SocketConnectionListServer->begin(); it1 != SocketConnectionListServer->end(); ++it1) 
			{
			
				snprintf( ServerReq, sizeof(ServerReq), "%s:%s:%d:IP Address - %s with Seq_num - %d", "WRITE",resource.c_str(),(*it1).connect_num,getIPAddress().c_str(),Rep[resource_file].Seq_num);
				send(((*it1).socket_id).socket_fd,&ServerReq , strlen(ServerReq) , 0);
				char buf1[100]={0};
				//sleep(1);
				int valread = read(((*it1).socket_id).socket_fd, buf1, 100); 
				string buffer1(buf1);
				cout<<"Message from Server - "<<buf1<<endl;
				
					
				
			}
		}
		else
		{
			cout<<"Connecting to Random Server "<<server_no<<endl;
			list <SocketWrapper> :: iterator it1;
			for(it1 = SocketConnectionListServer->begin(); it1 != SocketConnectionListServer->end(); ++it1) 
			{
				
				if((*it1).connect_num==server_no)
				{
					//cout<<"req sent to server"<<endl;
					snprintf( ServerReq, sizeof(ServerReq), "%s:%s:%d:%d", "READ",resource.c_str(),server_no,client_num);
					send(((*it1).socket_id).socket_fd,&ServerReq , strlen(ServerReq) , 0);
					char buf1[100]={0};
					
					int valread = read(((*it1).socket_id).socket_fd, buf1, 100); 
					string buffer1(buf1);
					cout<<"Last Line From Server :"<<buf1<<endl;
					
				}
			}
		}
		
		Rep[resource_file].Using=FALSE;
		
		for(int i=0;i<N;i++)
		{
			if(Rep[resource_file].Reply_Deferred[i]==TRUE)
			{
				//pthread_mutex_lock(&struct_lock);
				Rep[resource_file].A[i]=FALSE;
				Rep[resource_file].Reply_Deferred[i]=FALSE;
				//pthread_mutex_lock(&struct_lock);
				for(it = SocketConnectionList->begin(); it != SocketConnectionList->end(); ++it) 
				{
					if((*it).connect_num==i)
					{
						snprintf( reply, sizeof(reply), "%s:%d:%d", "REPLY",client_num,resource_file);
						if(i>client_num)
						{
							send(((*it).socket_id).socket_fd,&reply , strlen(reply) , 0);
						}
						else
						{
							send(((*it).socket_id).return_accept_socket(),&reply , strlen(reply) , 0);
						}
						cout<<"Release resource Reply to "<<i<<endl;
					}
				}
			}
		}
	

	}
//kept all threads waiting not to loose other thread variables
cout<<"waiting state"<<endl;
while(1){}
}
//struct for exit
struct exit_app
{
	list <SocketWrapper> *SocketConnectionList;
	list <SocketWrapper> *SocketConnectionListServer;
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
		
		for(it = (data->SocketConnectionListServer)->begin(); itt != (data->SocketConnectionListServer)->end(); ++itt) 
		{
			close(((*it).socket_id).socket_fd);
			close(((*it).socket_id).new_socket);
		}
		cout<<"SYSTEM EXIT"<<endl;
		exit(0);// force all thread to exit
	}
	else
	{
		cout<<"Not a Valid exit message"<<endl;
	}
}

///MAIN FUNCTION
int main()
{	
	char* clientIPList[5]={CLIENT0,CLIENT1,CLIENT2,CLIENT3,CLIENT4};
	char* serverIPList[5]={SERVER0,SERVER1,SERVER2};
	int N=5; //Total number of Clients
	int client_num;
	srand (time(NULL));
	client_num=findClientNum(getIPAddress());
	list <SocketWrapper> SocketConnectionList;
	list <SocketWrapper> SocketConnectionListServer;
	list <string> ServerFiles;
	int status=makeConnection(&SocketConnectionList,&SocketConnectionListServer);
	pthread_t exit_application;
	struct exit_app e;
	e.SocketConnectionList=&SocketConnectionList;
	e.SocketConnectionListServer=&SocketConnectionList;
	int rc = pthread_create(&exit_application, NULL, exit_fn, (void *)&e);
	if (rc)
	{
		cout<<"Problem with the creating Reply Thread.."<<endl;
		return 0;	
	}
	if (status==0)
	{
		cout<<"problem with the connection"<<endl;
		return 0;
	}
	cout<<"*****************Connection Established******************"<<endl;
	int res=askServerFile(&SocketConnectionListServer,&ServerFiles,client_num);
	if (res)
	{
		cout<<"problem with the server"<<endl;
		return 0;
	}
	int stat=ricart_agrawala_algo(N,client_num,&SocketConnectionList,&SocketConnectionListServer,&ServerFiles);
	if (stat)
	{
		cout<<"problem with the algo"<<endl;
		return 0;
	}
	
	
		
	return 0;
}



