#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<errno.h>
#include<unistd.h>
#include<sys/msg.h>
#include<pthread.h>


struct msg_st
{
	long int my_msg_type;
	pid_t my_pid;
	char username[20];
	char message[100];
};
struct node
{
	pid_t my_pid;
	struct node* next;
};

void* thread_func_getpid(void* received_pid);
void* thread_func_received_and_send_msg(void* theNode);
int msgid;
long int msg_to_receive=2;//use number 2 data in queue to receive data
long int msg_to_send=1;//use number 1 data in queue to send data
struct msg_st received_pid;//communication between process and thread
size_t st_size=sizeof(struct msg_st)-4;//sizeof
char ch='s';//for server quit

pthread_mutex_t work_mutex;//usage for mutex
pthread_t getpid_thread,client_thread[100];
int total_client=-1;
struct node* head=NULL,* newNode=NULL,* last=NULL;

int main()
{
	int res;
	/*
	res=pthread_mutex_init(&work_mutex,NULL);
	if(res!=0)
	{
		perror("Mutex initialization failed");
		exit(1);
	}
	*/

	res=pthread_create(&getpid_thread,NULL,thread_func_getpid,(void*)&received_pid);
	if(res!=0)
	{
		perror("Thread creation for getpid failed");
	    exit(1);
	}
	
		
	do
	{
		scanf("%c",&ch);
		getchar();
	}while(ch!='q');
	
	res=pthread_join(getpid_thread,NULL);
	if(res!=0)
	{
		perror("getpid_thread join failed\n");
		exit(1);
	}
	if(msgctl(msgid,IPC_RMID,0)==-1)
	{
		fprintf(stderr,"msgctl(IPC_RMID) for msgid failed",errno);
		exit(1);
	}
	
	int i;
	for(i=0;i<=total_client;++i)
	{
		res=pthread_join(client_thread[i],NULL);
		if(res!=0)
		{
			perror("One of client_threads join failed.");
			exit(1);
		}
	}

	//pthread_mutex_unlock(&work_mutex);
	//pthread_mutex_destroy(&work_mutex);
	
	printf("Bye Bye\n");
	exit(0);
}

void* thread_func_received_and_send_msg(void* theNode)
{
	int running2=1;
	struct node* client_pid = (struct node*)theNode, * tmp_head=NULL; 
	struct msg_st* received_msg = (struct msg_st*)malloc(sizeof(struct msg_st));
	int msgid2,msgid3;
	
	printf("client id: %d log in.\n",client_pid->my_pid);
	fflush(stdout);
	msgid2=msgget((key_t)client_pid->my_pid,0666|IPC_CREAT);
	if(msgid2==-1)
	{
		fprintf(stderr,"msgget for %d failed with error:%d\n",client_pid->my_pid,errno);
		exit(1);
	}

	while(running2)
	{
		if(msgrcv(msgid2,(void*)received_msg,st_size,msg_to_receive,0)==-1)
		{
			fprintf(stderr,"msgrcv for %d failed with error:%d\n",client_pid->my_pid,errno);
			exit(1);
		}

		received_msg->my_msg_type=msg_to_send;

		if(strcmp(received_msg->message,"end\n")==0)
		{
			if(msgsnd(msgid2,(void*)received_msg,st_size,0)==-1)
			{
				fprintf(stderr,"msgsnd for %d failed with error:%d\n",client_pid->my_pid,errno);
				exit(1);
			}
		}

		tmp_head=head;

		//pthread_mutex_lock(&work_mutex);
		while(tmp_head!=NULL)
		{
			if(tmp_head->my_pid != client_pid->my_pid)
			{
				msgid3=msgget((key_t)tmp_head->my_pid,0666|IPC_CREAT);
				if(msgid3==-1)
				{
					fprintf(stderr,"msgget for %d failed with error:%d\n",tmp_head->my_pid,errno);
					exit(1);
				}
				if(msgsnd(msgid3,(void*)received_msg,st_size,0)==-1)
				{
					fprintf(stderr,"msgsnd for %d failed with error:%d\n",tmp_head->my_pid,errno);
					exit(1);
				}
			}
			tmp_head=tmp_head->next;
		}
		//pthread_mutex_unlock(&work_mutex);
		tmp_head=head;
		
		if(strcmp(received_msg->message,"end\n")==0) 
		{
			free(received_msg);
			if(msgctl(msgid2,IPC_RMID,0)==-1)
			{
				fprintf(stderr,"msgctl(IPC_RMID) for %d failed\n",client_pid->my_pid);
		    	exit(1);
			}
			printf("client id: %d log off\n",client_pid->my_pid);
			//pthread_mutex_lock(&work_mutex);		
			if(tmp_head == client_pid)
				head=head->next;
			else
			{
				while(tmp_head->next != client_pid)
					tmp_head=tmp_head->next;
				tmp_head->next=client_pid->next;	
			}
			free(client_pid);
			//pthread_mutex_unlock(&work_mutex);
			pthread_exit(NULL);
		}
	}	
}

void* thread_func_getpid(void* received_pid)
{
	int running1=1,res;

	msgid=msgget((key_t)5566,0666 | IPC_CREAT);
	if(msgid==-1)
	{
		fprintf(stderr,"msgget for msgid fail with error: %d\n",errno);
		exit(1);
	}

	while(running1)
	{
		if(msgrcv(msgid,received_pid,st_size,msg_to_receive,0)==-1)
		{
			fprintf(stderr,"msgrcv for msgid2 fail with error: %d\n",errno);
			exit(1);
		}
		newNode = (struct node*)malloc(sizeof(struct node));
		newNode->my_pid=((struct msg_st*)received_pid)->my_pid;
		newNode->next=NULL;

		//pthread_mutex_lock(&work_mutex);
		if(head==NULL)
			last=head=newNode;
		else
		{
			last->next=newNode;
			last=last->next;
		}
		//pthread_mutex_unlock(&work_mutex);
		
		res=pthread_create(&client_thread[++total_client],NULL,thread_func_received_and_send_msg,(void *)newNode);

		if(ch=='q')
			pthread_exit(NULL);
	}
}

