#include <iostream>
#include <pthread.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <cstring>
#include <fstream>
#include <list>
#include <pthread.h>
#include <fcntl.h>
#include <semaphore.h>
#include <arpa/inet.h>
#include <time.h>
#include <dirent.h>



using namespace std;

#define DEFAULT_PORT 8080
#define OK_IMAGE    "HTTP/1.0 200 OK\nContent-Type:image/gif\n\n"
#define OK_TEXT     "HTTP/1.0 200 OK\nContent-Type:text/html\n\n"
#define NOTOK_404   "HTTP/1.0 404 Not Found\nContent-Type:text/html\n\n"
#define MESS_404    "<html><body><h1>FILE NOT FOUND</h1></body></html>"

pthread_mutex_t mutexForWaitingQ = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexForReadyQ = PTHREAD_MUTEX_INITIALIZER;
sem_t semToCountThreads;

int debug = 0;
int reqCount=0;
int enableLog=0;
FILE * Log;
char *logfile = NULL;
char *rootdir="/";



struct RequestClass
{
	char fileName[1024];
	char reqType[1024];
	int fileSize;
	int ClientID;
	char directoryInfo[1024];
    char TimeinsertedinWaitingQ[200];
    char TimeInsertedinReadyQ[200];
	struct RequestClass * link;

}*newnode,*temp,*p,*waitingQueueFront=NULL,*waitingQueueRear=NULL,*readyQueueFront,*readyQueueRear;

void insertnode(char* fname,char* rtypep, int fsize,int ID,char*dirInfo,char* TWQ,char* TRQ ,char QueueName)
{
	newnode=(RequestClass*)malloc(sizeof(RequestClass));
	if(QueueName=='W')
	{
	strcpy(newnode->fileName,fname);
	strcpy(newnode->reqType,rtypep);
	strcpy(newnode->directoryInfo,dirInfo);
	strcpy(newnode->TimeinsertedinWaitingQ,TWQ);
	strcpy(newnode->TimeInsertedinReadyQ,TRQ);
	newnode->fileSize=fsize;
	newnode->ClientID=ID;

	newnode->link=NULL;
	if(waitingQueueFront==NULL)
		waitingQueueFront=newnode;
	        else
	        	waitingQueueRear->link=newnode;
		waitingQueueRear=newnode;
		//display(QueueName);
	}
	else
	{
		strcpy(newnode->fileName,fname);
		strcpy(newnode->reqType,rtypep);
		strcpy(newnode->directoryInfo,dirInfo);
		strcpy(newnode->TimeinsertedinWaitingQ,TWQ);
		strcpy(newnode->TimeInsertedinReadyQ,TRQ);
		newnode->fileSize=fsize;
		newnode->ClientID=ID;

		newnode->link=NULL;
		if(readyQueueFront==NULL)
			readyQueueFront=newnode;
				else
					readyQueueRear->link=newnode;
			readyQueueRear=newnode;
			//display(QueueName);
	}
}

struct RequestClass deletefront(char QueueName)
{
	if(QueueName=='W')
	{
		if(waitingQueueFront==NULL)
		{
		   cout<<"empty list";
		}
		else
		{
			struct RequestClass delreq;
			p=waitingQueueFront;

			waitingQueueFront=waitingQueueFront->link;
			strcpy(delreq.fileName,p->fileName);
			strcpy(delreq.reqType,p->reqType);
			strcpy(delreq.directoryInfo,p->directoryInfo);
			strcpy(delreq.TimeinsertedinWaitingQ,p->TimeinsertedinWaitingQ);
			strcpy(delreq.TimeInsertedinReadyQ,p->TimeInsertedinReadyQ);
			delreq.fileSize=p->fileSize;
			delreq.ClientID=p->ClientID;

			free(p);
			return(delreq);
		}
	}
	else
	{
		if(readyQueueFront==NULL)
				{
				   cout<<"empty list";
				}
				else
				{
					struct RequestClass delreq;
					p=readyQueueFront;

					readyQueueFront=readyQueueFront->link;
					strcpy(delreq.fileName,p->fileName);
					strcpy(delreq.reqType,p->reqType);
					strcpy(delreq.directoryInfo,p->directoryInfo);
					strcpy(delreq.TimeinsertedinWaitingQ,p->TimeinsertedinWaitingQ);
					strcpy(delreq.TimeInsertedinReadyQ,p->TimeInsertedinReadyQ);
					delreq.fileSize=p->fileSize;
					delreq.ClientID=p->ClientID;

					free(p);
					return(delreq);
				}
	}

}

struct RequestClass deleteByID(int ClientID)
{
	struct RequestClass *old,*current,delreq;
	current=waitingQueueFront;
	while(current!=NULL)
	{
		if (current->ClientID==ClientID)
		{
			if(current==waitingQueueFront)
			{
				waitingQueueFront=waitingQueueFront->link;

			}
			else
			{
				old->link= current->link;

			}
			strcpy(delreq.fileName,current->fileName);
			strcpy(delreq.reqType,current->reqType);
			strcpy(delreq.directoryInfo,current->directoryInfo);
			strcpy(delreq.TimeinsertedinWaitingQ,p->TimeinsertedinWaitingQ);
			strcpy(delreq.TimeInsertedinReadyQ,p->TimeInsertedinReadyQ);
			delreq.fileSize=current->fileSize;
			delreq.ClientID=current->ClientID;

			free(current);
			return(delreq);
		}
		else
		{
			old=current;
			current=current->link;
		}
	}
}

void *worker(void* arg)
{
	 unsigned int file;
	 char out_buf[1024];
	 unsigned int buf_len;
	// cout<<"IN Worker";
	 while(1)
	 {
		 RequestClass processReq,*temptoChknull;

		 pthread_mutex_lock(&mutexForReadyQ);
		 temptoChknull=readyQueueFront;
		 pthread_mutex_unlock(&mutexForReadyQ);


		 if(temptoChknull!=NULL)
		 {
			 pthread_mutex_lock(&mutexForReadyQ);
			 processReq =deletefront('R');
			 pthread_mutex_unlock(&mutexForReadyQ);
			 //cout<<processReq.fileName<<endl;

			 file= open(processReq.fileName, O_RDONLY, S_IREAD | S_IWRITE);
			 if (file == (unsigned int)-1)
			 {
			   strcpy(out_buf, NOTOK_404);
			   send(processReq.ClientID, out_buf, strlen(out_buf), 0);
			   strcpy(out_buf, MESS_404);
			   send(processReq.ClientID, out_buf, strlen(out_buf), 0);
			 }
			 else
			 {
				 if ((strstr(processReq.fileName, ".jpg") != NULL)||(strstr(processReq.fileName, ".gif") != NULL))
				 {
					 strcpy(out_buf, OK_IMAGE);
				 }
				 else
				 {
					 strcpy(out_buf, OK_TEXT);
				 }
				 int i=send(processReq.ClientID, out_buf, strlen(out_buf), 0);
				  if(i<1)
				  {
					  cout<<"cant write to socket"<<endl;
				  }
				 // cout<<out_buf<<endl;

				  if(strcmp(processReq.reqType,"GET")==0)
				  {
					  if(strstr(processReq.fileName,".")==NULL)
					  {
						  int z= send(processReq.ClientID,processReq.directoryInfo,strlen(processReq.directoryInfo),0);
					  }
					  else
					  {
						  buf_len = 1;
						  while (buf_len > 0)
						  {
							buf_len = read(file, out_buf, 1024);
							if (buf_len > 0)
							{
							int j=  send(processReq.ClientID, out_buf, buf_len, 0);
							if(j<1)
							  {
								  cout<<"cant write to socket"<<endl;
							  }
							  //cout<<out_buf<<endl;
							}

						  }
					  }
			 	   }
				  else
				  {
					  /*cout<<processReq.fileName<<endl;
					  cout<<processReq.fileSize<<endl;
					  cout<<processReq.reqType<<endl;
					  cout<<processReq.ClientID<<endl;*/
					  int j=  send(processReq.ClientID, processReq.fileName, strlen(processReq.fileName), 0);
					  if(j<1)
					  {
						  cout<<"cant write to socket"<<endl;
					  }
					  char fsize[10];
					  sprintf(fsize,"%d",processReq.fileSize);
					  int k= send(processReq.ClientID,fsize,strlen(fsize),0);


				  }
				 }
				  close(processReq.ClientID);
				  sem_post(&semToCountThreads);
				  reqCount=0;
		}
	 }
}

void *FCFSscheduler(void* arg)
{
	RequestClass fcfsobj;
	char line[255];
	strcat(line, "\n");
		while(1)
		{
			pthread_mutex_lock(&mutexForWaitingQ);
			temp=waitingQueueFront;
			pthread_mutex_unlock(&mutexForWaitingQ);
			if(temp!=NULL)
			{
				pthread_mutex_lock(&mutexForWaitingQ);
				fcfsobj =deletefront('W');
				pthread_mutex_unlock(&mutexForWaitingQ);

				//cout<<fcfsobj.fileName<<" "<<fcfsobj.reqType<<" "<<fcfsobj.fileSize<<" "<<fcfsobj.ClientID<<" "<<endl;

			}
			else
			{
				continue;

			}
			//-------Insert into Ready Queue------------
			sem_wait(&semToCountThreads);
			pthread_mutex_lock(&mutexForReadyQ);

			//timestamp to insert into ready queue----------
			char outstr[200];
				   time_t t;
				   struct tm *tmp;

				   time(&t);
				   tmp = localtime(&t);
				   if (tmp == NULL) {
					   perror("localtime");
					   exit(EXIT_FAILURE);
				   }

				  if (strftime(outstr, sizeof(outstr), "[%d/%b/%Y : %H:%M:%S]", tmp) == 0) {
					   fprintf(stderr, "strftime returned 0");
					   exit(EXIT_FAILURE);
				   }

			insertnode(fcfsobj.fileName,fcfsobj.reqType,fcfsobj.fileSize,fcfsobj.ClientID,fcfsobj.directoryInfo,fcfsobj.TimeinsertedinWaitingQ ,outstr,'R');
			pthread_mutex_unlock(&mutexForReadyQ);
			//------------------------------------------
			if(enableLog==1)
			{
				Log= fopen(logfile,"a");
				char fsize[10];
				sprintf(fsize,"%d",fcfsobj.fileSize);
				char cliID[10];
				sprintf(cliID,"%d",fcfsobj.ClientID);


				//cout<<fsize;

				  if (Log!=NULL)
				  {
					  fputs(cliID,Log);
					  fputs(" ",Log);
					  fputs(fcfsobj.TimeinsertedinWaitingQ,Log);
					  fputs(" ",Log);
					  fputs(outstr,Log);
					  fputs(" ",Log);
					  fputs(fcfsobj.reqType,Log);
					  fputs("/",Log);
					  fputs (fcfsobj.fileName,Log);
					  fputs(" ",Log);
					  fputs("HTTP/1.1",Log);
					  fputs(" ",Log);
					  fputs("200",Log);
					  fputs(" ",Log);
					  fputs(fsize,Log);
					  fputs (line,Log);
					  fclose (Log);
				  }

			}
			if(debug==1)
			{
				//cout<<request;
				cout<<fcfsobj.ClientID<<" ";
				cout<<fcfsobj.TimeinsertedinWaitingQ<<" ";
				cout<<outstr<<" ";
				cout<<fcfsobj.reqType<<"/";
				cout<<fcfsobj.fileName<<" ";
				cout<<"HTTP/1.0 200";
				cout<<" "<<fcfsobj.fileSize<<endl;
			}




			//worker(fcfsobj);
		}

}

void *SJFscheduler(void* arg)
{
	int shortestRequestID=0;
	int minSize;
	int chkNextSize;
	RequestClass sjfobj;
	char line[255];
	strcat(line, "\n");
	while(1)
	{
		pthread_mutex_lock(&mutexForWaitingQ);
		temp=waitingQueueFront;
		pthread_mutex_unlock(&mutexForWaitingQ);
		if (temp==NULL)
		{
			continue;
		}
		else if(temp->link==NULL)
		{
			shortestRequestID=temp->ClientID;
		}
		else
		{
			minSize= temp->fileSize;

			while(temp->link!=NULL)
			{
				chkNextSize=temp->link->fileSize;
				if(minSize<=chkNextSize)
				{
						shortestRequestID=temp->ClientID;
				}
				else if(minSize>chkNextSize)
				{
					minSize=temp->link->fileSize;
						shortestRequestID=temp->link->ClientID;
				}
				temp=temp->link;
			}

		}

		pthread_mutex_lock(&mutexForWaitingQ);
		sjfobj=deleteByID(shortestRequestID);
		pthread_mutex_unlock(&mutexForWaitingQ);

		//-------Insert into Ready Queue------------
		sem_wait(&semToCountThreads);
		pthread_mutex_lock(&mutexForReadyQ);

		//timestamp while adding into ready queue....
		char outstr[200];
			   time_t t;
			   struct tm *tmp;

			   time(&t);
			   tmp = localtime(&t);
			   if (tmp == NULL) {
				   perror("localtime");
				   exit(EXIT_FAILURE);
			   }

			  if (strftime(outstr, sizeof(outstr), "[%d/%b/%Y : %H:%M:%S]", tmp) == 0) {
				   fprintf(stderr, "strftime returned 0");
				   exit(EXIT_FAILURE);
			   }
		//-------------------------------------------
		insertnode(sjfobj.fileName,sjfobj.reqType,sjfobj.fileSize,sjfobj.ClientID,sjfobj.directoryInfo,sjfobj.TimeinsertedinWaitingQ,outstr,'R');
		pthread_mutex_unlock(&mutexForReadyQ);
		//-------------------------------------------

		if(enableLog==1)
		{
			Log= fopen(logfile,"a");
			char fsize[10];
			sprintf(fsize,"%d",sjfobj.fileSize);
			char cliID[10];
			sprintf(cliID,"%d",sjfobj.ClientID);


			//cout<<fsize;

			  if (Log!=NULL)
			  {
				  fputs(cliID,Log);
				  fputs(" ",Log);
				  fputs(sjfobj.TimeinsertedinWaitingQ,Log);
				  fputs(" ",Log);
				  fputs(outstr,Log);
				  fputs(" ",Log);
				  fputs(sjfobj.reqType,Log);
				  fputs("/",Log);
				  fputs (sjfobj.fileName,Log);
				  fputs(" ",Log);
				  fputs("HTTP/1.1",Log);
				  fputs(" ",Log);
				  fputs("200",Log);
				  fputs(" ",Log);
				  fputs(fsize,Log);
				  fputs (line,Log);
				  fclose (Log);
			  }

		}
		if(debug==1)
		{
			//cout<<request;
			cout<<sjfobj.ClientID<<" ";
			cout<<sjfobj.TimeinsertedinWaitingQ<<" ";
			cout<<outstr<<" ";
			cout<<sjfobj.reqType<<"/";
			cout<<sjfobj.fileName<<" ";
			cout<<"HTTP/1.0 200";
			cout<<" "<<sjfobj.fileSize<<endl;
		}

		//worker(sjfobj);

	}
}

void *socket(void* port)
{
    unsigned int server_s;
    struct sockaddr_in  server_addr,client_addr;
    socklen_t ClientLength;
    unsigned int portID= *(unsigned int*)port;
    char request[1024];
	unsigned int client;
	char* reqType;
	char* fileName;

	//char* clientIpAddress;

        server_addr.sin_family = AF_INET;
        std::cout<<"In socket function"<<std::endl;
        //std::cout<<port<<std::endl;
        server_addr.sin_port = htons(portID);
        server_addr.sin_addr.s_addr = INADDR_ANY;

        server_s = socket(AF_INET, SOCK_STREAM, 0);
        if (server_s < 0)
            std::cout<<"error socket"<<std::endl;

        if (bind(server_s, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0)
            std::cout<<"error bind"<<std::endl;

        if (listen(server_s, 50))
            std::cout<<"error listen"<<std::endl;
            std::cout<<"Server Created"<<std::endl;



	do
	{
	    client = accept(server_s, (struct sockaddr*)&client_addr,&ClientLength);

	   char outstr[200];
	   time_t t;
	   struct tm *tmp;

	   time(&t);
	   tmp = localtime(&t);
	   if (tmp == NULL) {
		   perror("localtime");
		   exit(EXIT_FAILURE);
	   }

	  if (strftime(outstr, sizeof(outstr), "[%d/%b/%Y : %H:%M:%S]", tmp) == 0) {
		   fprintf(stderr, "strftime returned 0");
		   exit(EXIT_FAILURE);
	   }



	    if (debug == 1 && reqCount != 0)
	    {
	    	close(client);
	    }
	    else
	    {
	    	reqCount=1;
			if( client < 0)
			{
			std::cout<<"Client does not have Proper Credentials of Port and Socket Number"<<std::endl;

			}
			 else
			{
				 cout<<"Requested accepted"<<endl;
				//unsigned long int clientIpAddress=client_addr.sin_addr.s_addr;
				//  strcpy(clientIpAddress,inet_ntoa(client_addr.sin_addr));
				//cout<<clientIpAddress<<endl;
				memset(request,0,sizeof(request));
				int acceptrequest = recv(client, request, sizeof(request), 0);

				if (acceptrequest < 0)
				{
					std::cout<<" Reading Error ";
				}
				else if (acceptrequest == 0)
				{
							std::cout<<"Connection Closed : Client is not sending Data" ;
				}
				else
				{
					//cout<<" Message is :" <<request<<endl;
					char* reqDetils;

					reqDetils= strtok(request," /");
						int count=0;
					
					while(reqDetils!=NULL)
					{

						if(count==0)
						{
							reqType=reqDetils;
							//std::cout<<reqType<<std::endl;
						}
						if(count==1)
						{
							fileName=reqDetils;

						}
						count++;
						reqDetils= strtok(NULL," /");
					}

					char finalfileName[1024];
					if(strcmp(rootdir,"/")!=0)
					{
						strcpy(finalfileName,rootdir);
						strcat(finalfileName,fileName);
						strcpy(fileName,finalfileName);
					}


					char dirDetails[1024];
					if((strstr(fileName, ".") == NULL))
					{
						int cnt=0;
						DIR *dir;
						struct dirent *ent;

						if ((dir = opendir (fileName)) != NULL) {
						  /* print all the files and directories within directory */
						  while ((ent = readdir (dir)) != NULL) {
							//printf ("%s\n", ent->d_name);
							if(cnt==0)
							{
								strcpy(dirDetails,ent->d_name);
								cnt++;
							}
							else
							{

							strcat(dirDetails,ent->d_name);
							}
							strcat(dirDetails," ");
						  }
						  if(strstr(dirDetails,"index.html")!=NULL)
						  {
							  strcat(fileName,"/index.html");
						  }
						  closedir (dir);
						} else {
						  /* could not open directory */
						  perror ("");

						}
					}
					int fileSize=0;
					if(strcmp(reqType,"GET")==0)
					{
						int ret=0;
						struct stat buff;

						ret = stat((const char*) fileName, &buff);
						fileSize=(long long) buff.st_size;
					}
					else
					{
						fileSize=0;
					}

					if((strstr(fileName, ".") == NULL))
					{
						fileSize=0;
					}

										//cout<<"-----------Request details ends---------"<<endl;
					pthread_mutex_lock(&mutexForWaitingQ);
					if((strstr(fileName, ".") == NULL))
					insertnode(fileName,reqType,fileSize,client,dirDetails,outstr," ",'W');
					else
					{
						insertnode(fileName,reqType,fileSize,client," ",outstr," ",'W');
					}
					pthread_mutex_unlock(&mutexForWaitingQ);


			}




			}
	    }
	}
	while(1);
}

void printUsage()
{
	fprintf(stderr, "Usage: myhttpd [−d] [−h] [−l file] [−p port] [−r dir] [−t time] [−n thread_num] [−s sched]\n");
	 
	fprintf(stderr,"\t−d : Enter debugging mode. That is, do not daemonize, only accept\n"
		"\tone connection at a time and enable logging to stdout. Without\n"
		"\tthis option, the web server should run as a daemon process in the\n"
		"\tbackground.\n"
		"\t−h : Print a usage summary with all options and exit.\n"
		"\t−l file : Log all requests to the given file. See LOGGING for\n"
		"\tdetails.\n"
		"\t−p port : Listen on the given port. If not provided, myhttpd will\n"
		"\tlisten on port 8080.\n"
		"\t−r dir : Set the root directory for the http server to dir.\n"
		"\t−t time : Set the queuing time to time seconds. The default should\n"
		"\tbe 60 seconds.\n"
		"\t−n thread_num : Set number of threads waiting ready in the execution thread pool to\n"
		"\tthreadnum. The d efault should be 4 execution threads.\n"
		"\t−s sched : Set the scheduling policy. It can be either FCFS or SJF.\n"
		"\tThe default will be FCFS.\n");
}

int main ( int argc, char **argv )
{
//int debug = 0;
int port = 8080;
int n_threads = 4;
int queueing_time = 60;
 
char *schedPolicy = "FCFS";
 
int c;
 
opterr = 0;
 
 
if (argc < 2)
{
	printUsage();
	exit(1);
}
 
// You should give a list of options in the third argument to
// getopt() ":" means you want to get a value after the option.
while ( ( c = getopt (argc, argv, "dhl:p:r:t:n:s:") ) != -1 )
{
	switch (c)
	{
	case 'd':
		debug = 1;

		break;
	case 'h':
		printUsage();
		exit(1);
	case 'l':
		enableLog=1;
		logfile = optarg;
		strcat(logfile,"log.txt");
		break;
	case 'p':
		port = atoi(optarg);
		 //std::cout<<port<<std::endl;
		if (port < 1024)
		{
			fprintf(stderr, "[error] Port number must be greater than or equal to 1024\n");	
			exit(1);
		}
		break;
	case 'r':
		rootdir = optarg;
		break;
	case 't':
		queueing_time = atoi(optarg);
		if (queueing_time < 1)
		{
			fprintf(stderr, "[error] queueing time must be greater than 0.\n");
			exit(1);
		}
		break;
	case 'n':
		n_threads = atoi(optarg);
		if (n_threads < 1)
		{
			fprintf(stderr, "[error] number of threads must be greater than 0.\n");
			exit(1);
		}
		break;
	case 's':
		schedPolicy = optarg;
		break;
	default:
		printUsage();
		exit(1);
	}
	} // while (...)
 
	if (debug == 1)
	{
		fprintf(stderr, "myhttpd logfile: %s\n", logfile);
		fprintf(stderr, "myhttpd port number: %d\n", port);
		fprintf(stderr, "myhttpd rootdir: %s\n", rootdir);
		fprintf(stderr, "myhttpd queueing time: %d\n", queueing_time);
		fprintf(stderr, "myhttpd number of threads: %d\n", n_threads);
		fprintf(stderr, "myhttpd scheduling policy: %s\n", schedPolicy);
	}
	//std::cout<<port<<"2"<<std::endl;
	pthread_t listenerThread,schedulerThread,workerThread[n_threads];

	if(debug == 1)
	{
		n_threads=1;

			sem_init(&semToCountThreads,0,n_threads);

			for (int var = 0; var < n_threads; ++var) {
				pthread_create(&workerThread[var],NULL,worker,NULL);

			}

			//socket(port);
			pthread_create(&listenerThread,NULL,socket,&port);
			sleep(queueing_time);
			if(strcmp(schedPolicy,"FCFS")==0)
			{
				pthread_create(&schedulerThread,NULL,FCFSscheduler,NULL);
			}
			else
			{
				pthread_create(&schedulerThread,NULL,SJFscheduler,NULL);
			}

			pthread_join(listenerThread,NULL);
			pthread_join(schedulerThread,NULL);
			return 0;
	}
	else
	{
		int pid= fork();
		if(pid>0)
		{
			//parent process
			cout<<"child process created... PID :"<<pid;
			cout<<"Exiting Parent";
			return 0;

		}
		else
		{
			pthread_t listenerThread,schedulerThread,workerThread[n_threads];

				sem_init(&semToCountThreads,0,n_threads);

				for (int var = 0; var < n_threads; ++var) {
					pthread_create(&workerThread[var],NULL,worker,NULL);

				}

				//socket(port);
				pthread_create(&listenerThread,NULL,socket,&port);
				sleep(queueing_time);
				if(strcmp(schedPolicy,"FCFS")==0)
				{
					pthread_create(&schedulerThread,NULL,FCFSscheduler,NULL);
				}
				else
				{
					pthread_create(&schedulerThread,NULL,SJFscheduler,NULL);
				}

				pthread_join(listenerThread,NULL);
				pthread_join(schedulerThread,NULL);
				return 0;
		}
	}


}
 









