#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<errno.h>
#include<unistd.h>
#include<sys/msg.h>
#include<pthread.h>
#include<sys/types.h>

struct msg_st
{
	long int my_msg_type;
	pid_t my_pid;
	char username[20];
	char message[100];
};

void* thread_func_received_msg(void* received_msg);
int msgid;
long int msg_to_send=2;//use number 2 data in queue to send data
long int msg_to_receive=1;//use number 1 data in queue to receive data
struct msg_st send_msg,received_msg;//communication between process and thread
size_t st_size=sizeof(struct msg_st)-4;

int main()
{
	//variables for message queue
	int running1=1;	
	char buffer[BUFSIZ];
	char name_buffer[20];
	key_t thekey=getpid();  //set own pid as key
	int msgid2; 			//send own pid to server
	//variables for thread
	int res;
	pthread_t a_thread;
	printf("%d\n",thekey);
	msgid=msgget((key_t)thekey,0666|IPC_CREAT);
	if(msgid==-1)
	{
		fprintf(stderr,"msgget failed with error: %d\n",errno);
		exit(1);
	}
	
	send_msg.my_pid=getpid();
	send_msg.my_msg_type=msg_to_send;

	printf("Enter you name(limits 20 bytes): ");
	scanf("%[^\n]s",name_buffer);
	getchar();
	int length=strlen(name_buffer);
	name_buffer[length]='\0';
	strncpy(send_msg.username,name_buffer,20);
	
	msgid2=msgget((key_t)5566,0666 | IPC_CREAT);
	if(msgid2==-1)
	{
		fprintf(stderr,"msgget for msgid2 fail with error: %d\n",errno);
		exit(1);
	}
	
	if(msgsnd(msgid2,(void*)&send_msg,st_size,0)==-1)
	{
		fprintf(stderr,"msgsnd for msgid2 failed with error: %d\n",errno);
		exit(1);
	}
	
	sleep(3);
	printf("Welcome to the chatroom:\n");

	res=pthread_create(&a_thread,NULL,thread_func_received_msg,(void *)&received_msg);
	if(res!=0)
	{
		perror("Thread creation failed");
		exit(1);
	}
		
	while(running1)				//send the message
	{
		
		fgets(buffer,BUFSIZ,stdin);
		send_msg.my_msg_type=msg_to_send;
		strcpy(send_msg.message,buffer);
		if(msgsnd(msgid,(void*)&send_msg,st_size,0)==-1)
		{
			fprintf(stderr,"msgsnd failed");
			exit(1);
		}
		if(strcmp(send_msg.message,"end\n")==0)
			running1=0;
	}

	res=pthread_join(a_thread,NULL);
	if(res!=0)
	{
		perror("Thread join failed");
		exit(1);
	}

	printf("Bye Bye\n");
	exit(0);
}

void* thread_func_received_msg(void* received_msg)		//read the messages
{
	int running2=1;
	while(running2)
	{
		
		if(msgrcv(msgid,received_msg,st_size,msg_to_receive,0)==-1)
		{
			fprintf(stderr,"msgrcv failed with errpr: %d\n",errno);
			exit(1);
		}
		if(((struct msg_st*)received_msg)->my_pid == getpid())
			pthread_exit(NULL);
		else if(strcmp(((struct msg_st*)received_msg)->message,"end\n")==0)
		{
			printf("%s has disconnected.\n",((struct msg_st*)received_msg)->username);
			fflush(stdout);
		}
		else
		{
			printf("%s: %s",((struct msg_st*)received_msg)->username,((struct msg_st*)received_msg)->message);
			fflush(stdout);
		}
	}
}

