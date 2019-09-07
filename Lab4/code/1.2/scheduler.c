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


struct process{
	pid_t mypid;
	struct process* next;
	char *name;
	int data;
};
/*Definition of my list nodes*/
struct process *head=NULL;
struct process *newnode=NULL;
//struct process *delete=NULL;
struct process *last=NULL;
struct process *running=NULL; 
/*
 * SIGALRM handler
 */
static void
sigalrm_handler(int signum)
{
	//assert(0 && "Please fill me!");
	kill(head->mypid,SIGSTOP);
	//alarm(SCHED_TQ_SEC);
	
}

/* 
 * SIGCHLD handler
 */
static void
sigchld_handler(int signum)
{
	//assert(0 && "Please fill me!");
	pid_t p;
	int status;

	while(1){
		p=waitpid(-1,&status,WNOHANG|WUNTRACED);
		if (p<0){
			perror("waitpid");
			exit(1);
		}
		if(p==0) break;		/////////
		
		explain_wait_status(p,status);
		//p has the pid of our child that has changed its status
		
		if (WIFEXITED(status) || WIFSIGNALED(status)){//if terminated or killed by a signal remove it from the list
			struct process* delete=NULL;
			struct process * temp;
			temp=head;
			
			while(temp!=NULL){
				if (temp->mypid==p && temp==head){ //if i got to delete the head
						if (temp->next == NULL){
							 free(temp);
							 printf("I am done\n");
							 exit(0);
						}
						else{
							head=temp->next;
							free(temp);
						}
				}
				else if (temp->mypid==p && temp==last){ //if i got to delete the last element of the list
					last=delete;
					last->next=NULL;
					free(temp);
				}
				else if(temp->mypid==p){ //if i delete a random element in the list
					delete->next=temp->next;
					free(temp);
				}
				else{ //if i got to continue searching , will never access any node after the last element
					delete=temp;
					temp=temp->next;
					continue;	
				}
				break;			
			}
		
			
						
		}
						
		if (WIFSTOPPED(status)){
			//if our process is stopped by the scheduler, continue with the next one
			last->next=head;
			last=head;
			struct process * temp;
			temp=head;
			head=head->next;
			temp->next=NULL;			
		}
		alarm(SCHED_TQ_SEC);
		//printf("%s\n",running->name);
		kill(head->mypid,SIGCONT);
		

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
	int nproc;
	/*
	 * For each of argv[1] to argv[argc - 1],
	 * create a new child process, add it to the process list.
	 */
	char executable[] = "prog";
        char *newargv[] = { executable, NULL, NULL, NULL };
        char *newenviron[] = { NULL };
	
	nproc = argc-1; /* number of proccesses goes here */
	if (nproc == 0) {
                fprintf(stderr, "Scheduler: No tasks. Exiting...\n");
                exit(1);
        }

	pid_t mypid;
	int i;
	/*Fork and create the list*/
	for (i=0;i<nproc;i++){
		mypid=fork();
		if (mypid<0){
			printf("Error with forks\n");
		}
		if(mypid==0){
			raise(SIGSTOP);
			printf("I am %s, PID = %ld\n",
                        argv[0], (long)getpid());
                        printf("About to replace myself with the executable %s...\n",
                                executable);
                        sleep(2);

                        execve(executable, newargv, newenviron);

                        /* execve() only returns on error */
                        perror("execve");
                        exit(1);
			}
		else{	
			if (i==0){
				head=(struct process *)malloc(sizeof(struct process));
				if (head==NULL) printf("Error with malloc\n");
				head->mypid=mypid;
				head->next=NULL;
				head->data=i+1;
				head->name=argv[i+1];
				last=head;
										
			}
			else{
				newnode=(struct process *)malloc(sizeof(struct process));
				if (newnode==NULL) printf("Error with malloc\n");
				newnode->mypid=mypid;
				newnode->next=NULL;
				newnode->data=i+1;
				newnode->name=argv[i+1];
				last->next=newnode;
				last=newnode;
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
	kill(head->mypid,SIGCONT);
	
	/* loop forever  until we exit from inside a signal handler. */
	while (pause())
		;

	/* Unreachable */
	fprintf(stderr, "Internal error: Reached unreachable point\n");
	return 1;
}
