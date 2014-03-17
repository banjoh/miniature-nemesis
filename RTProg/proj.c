/*
This is a server application that serves
upto 50 clients simultaneously
*/

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>

typedef struct
{
	int A;
	int B;
	int C;
	int D;
	int E;
} TICKETS;

typedef struct
{
	pid_t client_pid;
	int req_tickets;
	char req_type;
	int bool;
} REQ;

static TICKETS ticket;
static int served;
static int clients_created;
static int clients_handled;
int  fifo_fd;
pid_t serv_pid;
sigset_t new_mask, old_mask;
struct sigaction act;

void purchase(int slp);
void create_clients(int clients);
static void handle_req(int signo);
static void sig_complete(int sig_no);

/*
The main function is the actual server that deals with the clients. In the main we
have an infinite loop which simulates the ever waiting server. The rand function in
it generates the number of clients that access the server simultaneously while the 
create_clients() creates child processes for every client.
*/

int main (void) 
{
	char buf[20];
	REQ client_req;
	int clients;
	
	clients_created = 0;
	clients_handled = 0;
	served = 0;	
	
	sigemptyset(&new_mask); 
	sigaddset(&new_mask, SIGUSR2);
	sigprocmask(SIG_BLOCK, &new_mask, &old_mask); 
	
	ticket.A = 10;
	ticket.B = 10;
	ticket.C = 10;
	ticket.D = 10;
	ticket.E = 10;
	
	//installing the signal handler
	if(signal(SIGUSR1, handle_req) == SIG_ERR)
              fprintf(stderr, "cannot set signal handler");
	
	if(signal(SIGUSR2, sig_complete) == SIG_ERR)
              fprintf(stderr, "cannot set signal handler");
	
	if((mkfifo("parent_fifo", S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH) < 0)) // Creating a FIFO for the server to read requests from clients
	{
		perror("\nCould not make Parent fifo ");		
		unlink("parent_fifo");
		mkfifo("parent_fifo", S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);
	}
	fifo_fd = open("parent_fifo", O_RDWR | O_NONBLOCK);	
	
	srand ( time(NULL) ); //Initialize random seed
	serv_pid = getpid();
	printf(" %d parent fifo fd\n", fifo_fd);
	
	printf(" %d server pid fd\n", serv_pid);
	
	while(ticket.A || ticket.B || ticket.C || ticket.D || ticket.E)
	{
		clients = rand() % 10 + 5;   // Random number from 10 to (10 + 40) = rand() % 50 + 10
		create_clients(clients); // create the clients		
		clients_created = clients_created + clients;	//increament clients_created to get all the children successfully created
	}
	
	printf("After tickets over clients served are : %d\n",served);
	printf("Number of clients virtally created : %d\n",clients_created);
	printf("Number of clients successfully handled : %d\n",clients_handled);
	
	if((close(fifo_fd)) < 0)
		perror("close error \n");	
	else
		printf("fifo closed\n");
	exit(0);
	
}

/*
create_clients() is the function that takes the random # of clients created and 
by use of a for loop fork()'s # times. Child 1 is created but because of avoiding
zombies we double fork to create child 2 and terminate child 1. By use of child 2
we do the actual client tasks which are taken care of by the purchase function
*/

void create_clients(int num_of_clients)
{
    pid_t ctest_pid;
	pid_t pid;
	int i, x;
	
	for (i=0; i<num_of_clients; i++)
	{
		pid = fork();
		x = rand() % 10;
		
		if (pid < 0) 
		{
			perror("Fork ");	// error in fork()
			exit(1);
		}
		
		if (pid == 0) 
		{ 
			pid = fork();	// child 1
			
			if (pid > 0)	
				exit(0);	// Terminate the child that was first created which is the parent to the
							// 2nd child forked leaving the 2nd child for adoption (by init process)
							
			if (pid == 0)	// child 2	
			{  			
				purchase(x);	// Purchase tickets
				printf("%d ",getpid()); 
				exit(0);
			}
		}
	
	wait(NULL); // wait for child 1 - wait returns immediately because child 1 has been terminated
				// and child 2 already inherited by the init process
	}	
}

/*
This is where the actual ticket purchase takes place. In here the client communicates
with the server to ask for tickets and waits for a response from the server. The func
will terminate when after a negative or positive response from the server is received.
*/

void purchase(int slp)
{
	REQ req;
	int temp;
	int w_err;
	int r_err;
	
	temp = rand() % 5 + 1;
	switch(temp)
	{
		case 1:
			req.req_type = 'A';
			break;
		case 2:
			req.req_type = 'B';
			break;
		case 3:
			req.req_type = 'C';
			break;
		case 4:
			req.req_type = 'D';
			break;
		case 5:
			req.req_type = 'E';
			break;
	}
	req.req_tickets = rand() % 100 + 1;
	
	req.client_pid = getpid();
	
	sleep(slp);  // simulation of tasks time estimate of purchase
	
	if((w_err = write(fifo_fd, &req, sizeof(REQ))) < 0)
	{		
		//perror("Write error ");	// error
		//printf("%d ",getpid());
		return;
	}
	else
	{
		kill(serv_pid, SIGUSR1); 	//send signal handle_req(int signo)
		sigsuspend(&old_mask);	
		
		if((r_err = read(fifo_fd, &req, sizeof(REQ))) < 0)
		{
			perror("Read error ");		// error
			printf("Child %d terminated...\n",getpid()); 
			return;
		}
		else
		{
			switch(req.bool)
			{
				case 0:
					//printf("Tickets sold out !!\n");
					break;
				case 1:
					//printf("Client %d successfully bought %d tickets in class %c\n",req.client_pid,req.req_tickets,req.req_type);
					break;
				case 2:
					//printf("No such tickets !!\n");
					break;
				default:
					break;
			}
		}
	}
}

static void handle_req(int signo)	// Use of signals or Asynchronous IO will be used here
{
	REQ req;
	
	read(fifo_fd, &req, sizeof(REQ)); // Server receives request from the client at this point and handles it
	//printf("(signal) child pid_t %d, order %d \n",req.client_pid,req.req_tickets);
	
	switch(req.req_type)
	{
		case 'A':
			if(ticket.A  && ticket.A >= req.req_tickets){
				ticket.A = ticket.A - req.req_tickets;
				req.bool = 1;
				++served;
			}else req.bool = 0;
			break;
		case 'B':
			if(ticket.B  && ticket.B >= req.req_tickets){
				ticket.B = ticket.B - req.req_tickets;
				req.bool = 1;
				++served;
			}else req.bool = 0;
			break;
		case 'C':
			if(ticket.C  && ticket.C >= req.req_tickets){
				ticket.C = ticket.C - req.req_tickets;
				req.bool = 1;
				++served;
			}else req.bool = 0;
			break;
		case 'D':
			if(ticket.D  && ticket.D >= req.req_tickets){
				ticket.D = ticket.D - req.req_tickets;
				req.bool = 1;
				++served;
			}else req.bool = 0;
			break;
		case 'E':
			if(ticket.E  && ticket.E >= req.req_tickets){
				ticket.E = ticket.E - req.req_tickets;
				req.bool = 1;
				++served;
			}else req.bool = 0;
			break;
		default:
			req.bool = 2;
			break;
	}
	++clients_handled;
	write(fifo_fd, &req, sizeof(REQ));	// Server aknowledgement is sent here back to the client with the request results
	//printf("Tickets available are\nA : %d\nB : %d\n",ticket.A,ticket.B);
	//printf("C : %d\nD : %d\nE : %d\n",ticket.C,ticket.D,ticket.E);
	kill(req.client_pid,SIGUSR2);  // send signal sig_complete(int sig_no)
}

static void sig_complete(int sig_no){	
	return;
}





