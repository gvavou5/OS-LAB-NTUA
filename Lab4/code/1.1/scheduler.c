#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <assert.h>

#include <sys/wait.h>
#include <sys/types.h>

#include "proc-common.h"
#include "request.h"

/* Compile-time parameters. */
#define SCHED_TQ_SEC 2                /* time quantum */
#define TASK_NAME_SZ 60               /* maximum size for a task's name */


typedef struct Node{
	pid_t id; //my pid
	char * name;
	int data;
	struct Node *  next;
}Node;
typedef Node * nodeptr;

nodeptr head = NULL;
nodeptr end = NULL; 

/*
 * SIGALRM handler
 */
static void
sigalrm_handler(int signum)
{
	kill(head->id,SIGSTOP);
}

/* 
 * SIGCHLD handler
 */
static void
sigchld_handler(int signum)
{
	pid_t p;
	int status;

	for(;;){
		p=waitpid(-1,&status,WNOHANG|WUNTRACED);
		
		if (p<0){
			perror("waitpid");
			exit(1);
		}
		if(p==0) break;
		
		/* a child has changed his status, lets check what happened */
		explain_wait_status(p,status);
		
		
		if (WIFEXITED(status) || WIFSIGNALED(status)){
			/* If i am here means that a child is terminated OR killed by a signal */
			/* Because of that i have to remove it from the list */
			nodeptr previous = NULL;
			nodeptr current = head;
			
			while(current != NULL){
				if (current->id == p && current == head){ 
					/* I have to delete the head of my list */
						if (current->next == NULL){
							/* I will come here only once...when i have only one node... i delete it and i am done */
							 free(current);
							 printf("My list is empty...I don't have any more work to do\n");
							 exit(0);
						}
						else{
							head=current->next;
							free(current);
						}
				}
				else if (current->id == p && current == end){ 
					/* I have to delete the last node of my list */
					end=previous;
					end->next=NULL;
					free(current);
				}
				else if(current->id == p){ 
					/* I have to delete a random node of the list but sure its not head or tail */
					previous->next=current->next;
					free(current);
				}
				else{ 
					/* I will continue searching*/ 
					previous=current;
					current=current->next;
					continue;	
				}
				break;			
			}
		
			
						
		}
						
		if (WIFSTOPPED(status)){
			/* I am here if m process is stopped */
			/* I transfer this process at the end of list and continue with the next one */
			nodeptr temp;
			end->next=head;
			end=head;
			temp=head;
			head=head->next;
			temp->next=NULL;
			/* Only here i have to set the alarm again */
			alarm(SCHED_TQ_SEC);			
		}

		/* Alarm Set */
		//alarm(SCHED_TQ_SEC);

		/* I always send SIGCONT to the head of my list */
		kill(head->id,SIGCONT);
	}
}

/* Install two signal handlers.
 * One for SIGCHLD, one for SIGALRM.
 * Make sure both signals are masked when one of them is running.
 */
static void
install_signal_handlers(void)
{
	sigset_t sigset;
	struct sigaction sa;

	sa.sa_handler = sigchld_handler;
	sa.sa_flags = SA_RESTART;
	sigemptyset(&sigset);
	sigaddset(&sigset, SIGCHLD);
	sigaddset(&sigset, SIGALRM);
	sa.sa_mask = sigset;
	if (sigaction(SIGCHLD, &sa, NULL) < 0) {
		perror("sigaction: sigchld");
		exit(1);
	}

	sa.sa_handler = sigalrm_handler;
	if (sigaction(SIGALRM, &sa, NULL) < 0) {
		perror("sigaction: sigalrm");
		exit(1);
	}

	/*
	 * Ignore SIGPIPE, so that write()s to pipes
	 * with no reader do not result in us being killed,
	 * and write() returns EPIPE instead.
	 */
	if (signal(SIGPIPE, SIG_IGN) < 0) {
		perror("signal: sigpipe");
		exit(1);
	}
}



int main(int argc, char *argv[])
{
	/*
	 * For each of argv[1] to argv[argc - 1],
	 * create a new child process, add it to the process list.
	 */
	
	/* nproc tells us how many processes i gave us arguments */
	int nproc = argc-1, i;
	pid_t p;
	nodeptr newnode = NULL;

	char executable[] = "prog";
        char *newargv[] = { executable, NULL, NULL, NULL };
        char *newenviron[] = { NULL };
	
	if (nproc == 0) {
                fprintf(stderr, "Scheduler: No tasks. Exiting...\n");
                exit(1);
        }

	
	/*Fork and create the list*/
	for (i=0;i<nproc;i++){
		p=fork();
		if (p<0){
			perror("fork");
		}
		if(p == 0){
			/* Child Space */
			raise(SIGSTOP);
			printf("I am %s, PID = %ld\n", argv[0], (long)getpid());
                        printf("About to replace myself with the executable %s...\n", executable);
                        sleep(2);
			/* execve() only returns on error */
                        execve(executable, newargv, newenviron);
                        perror("execve");
                        exit(1);
		}
		else{	
			/* Father Space */
			if( i == 0){
				/* I have to create the head of the list */
				head=(nodeptr)malloc(sizeof(Node));
				if(head == NULL){
					 perror("malloc");
					 exit(2);
				}
				head->id=p;
				head->next=NULL;
				head->data=i+1;
				head->name=argv[i+1];
				end=head; // i do that beacause is the first node ==> head == end
										
			}
			else{
				/* Create a random-new node of the list and put it at the end of the list */
				newnode = (nodeptr)malloc(sizeof(Node));
				if(newnode == NULL){
					perror("malloc");
					exit(2);
				}
				newnode->id=p;
				newnode->next=NULL;
				newnode->data=i+1;
				newnode->name=argv[i+1];
				end->next=newnode;
				end=newnode;
			}	
		
		}
		

	}

	
	/* Wait for all children to raise SIGSTOP before exec()ing. */
	wait_for_ready_children(nproc);

	/* Install SIGALRM and SIGCHLD handlers. */
	install_signal_handlers();

	/*Set the alarm on*/
	alarm(SCHED_TQ_SEC);

	/*Start the first process*/
	kill(head->id,SIGCONT);
	
	/* loop forever  until we exit from inside a signal handler. */
	while (pause())
		;

	/* Unreachable */
	fprintf(stderr, "Internal error: Reached unreachable point\n");
	return 1;
}
