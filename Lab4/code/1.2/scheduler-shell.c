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
#define SHELL_EXECUTABLE_NAME "shell" /* executable for shell */

typedef struct Node{
        pid_t id;
        struct Node* next;
        char* name;
        int data;
}Node;
typedef Node * nodeptr;

/*Definition of my list nodes*/
nodeptr head=NULL;
nodeptr end=NULL;
int i=0,nproc;

/* Print a list of all tasks currently being scheduled.  */
static void
sched_print_tasks(void)
{
	nodeptr temp=head;
	while (temp!=NULL){
		printf("ID: %d PID: %i Name: %s \n",temp->data,temp->id,temp->name);
		if (temp->next != NULL)
			temp=temp->next;
		else break;
	}
	
}

/* Send SIGKILL to a task determined by the value of its
 * scheduler-specific id.
 */

static int
sched_kill_task_by_id(int id)
{
	nodeptr temp=head;
	while (temp!=NULL){
		if (temp->data == id ){
			kill(temp->id,SIGKILL);
			return 0;
		}
		else	
			if(temp->next!=NULL)
				temp=temp->next;
			else break;	
	}
	return -ENOSYS;
}


/* Create a new task.  */
static void
sched_create_task(char *executable)//
{
		nodeptr newnode;

        	char *newargv[] = { executable, NULL, NULL, NULL };
	        char *newenviron[] = { NULL };
 		
		pid_t mypid;
                mypid=fork();
                if (mypid < 0){
                        perror("fork");
                }
                if(mypid == 0){
                        raise(SIGSTOP);
                        printf("I am %s, PID = %ld\n",
                        executable, (long)getpid());
                        printf("About to replace myself with the executable %s...\n",executable);
                        sleep(2);
                        execve(executable, newargv, newenviron);
                        /* execve() only returns on error */
                        perror("execve");
                        exit(1);
                        }
                else{ 
			/* Insert the new process to the list */
		        newnode=(nodeptr)malloc(sizeof(Node));
                        if (newnode == NULL){
		        	perror("malloc");
				exit(2);
                  	}
		        newnode->id=mypid;
                        newnode->next=NULL;
                        newnode->data=i+1;
			newnode->name=(char*)malloc(strlen(executable)+1);
                       	strcpy(newnode->name,executable);
                        end->next=newnode;
                        end=newnode;
			nproc++;
			i++;        
                }

}

/* Process requests by the shell.  */
static int
process_request(struct request_struct *rq)
{
	switch (rq->request_no) {
		case REQ_PRINT_TASKS:
			sched_print_tasks();
			return 0;

		case REQ_KILL_TASK:
			return sched_kill_task_by_id(rq->task_arg);

		case REQ_EXEC_TASK:
			sched_create_task(rq->exec_task_arg);
			return 0;

		default:
			return -ENOSYS;
	}
}

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
			/* I am here if a process is stopped */
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

	
		if(head->data==0){
			printf("I am in Shell again:\n"); 
			alarm(6*SCHED_TQ_SEC);
		}
		//else alarm(SCHED_TQ_SEC);
        
		/* Send SIGCONT to the head of the list */
                kill(head->id,SIGCONT);


        }

}


/* Disable delivery of SIGALRM and SIGCHLD. */
static void
signals_disable(void)
{
	sigset_t sigset;

	sigemptyset(&sigset);
	sigaddset(&sigset, SIGALRM);
	sigaddset(&sigset, SIGCHLD);
	if (sigprocmask(SIG_BLOCK, &sigset, NULL) < 0) {
		perror("signals_disable: sigprocmask");
		exit(1);
	}
}

/* Enable delivery of SIGALRM and SIGCHLD.  */
static void
signals_enable(void)
{
	sigset_t sigset;

	sigemptyset(&sigset);
	sigaddset(&sigset, SIGALRM);
	sigaddset(&sigset, SIGCHLD);
	if (sigprocmask(SIG_UNBLOCK, &sigset, NULL) < 0) {
		perror("signals_enable: sigprocmask");
		exit(1);
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

static void
do_shell(char *executable, int wfd, int rfd)
{
	char arg1[10], arg2[10];
	char *newargv[] = { executable, NULL, NULL, NULL };
	char *newenviron[] = { NULL };

	sprintf(arg1, "%05d", wfd);
	sprintf(arg2, "%05d", rfd);
	newargv[1] = arg1;
	newargv[2] = arg2;

	raise(SIGSTOP);
	execve(executable, newargv, newenviron);

	/* execve() only returns on error */
	perror("scheduler: child: execve");
	exit(1);
}

/* Create a new shell task.
 *
 * The shell gets special treatment:
 * two pipes are created for communication and passed
 * as command-line arguments to the executable.
 */
static void
sched_create_shell(char *executable, int *request_fd, int *return_fd)
{
	pid_t p;
	int pfds_rq[2], pfds_ret[2];

	if (pipe(pfds_rq) < 0 || pipe(pfds_ret) < 0) {
		perror("pipe");
		exit(1);
	}

	p = fork();
	if (p < 0) {
		perror("scheduler: fork");
		exit(1);
	}

	if (p == 0) {
		/* Child */
		close(pfds_rq[0]);
		close(pfds_ret[1]);
		do_shell(executable, pfds_rq[1], pfds_ret[0]);
		assert(0);
	}
	/* Parent */
	close(pfds_rq[1]);
	close(pfds_ret[0]);
	*request_fd = pfds_rq[0];
	*return_fd = pfds_ret[1];

	/* I have to insert the Shell as the head of my list */
	head->data=0;
	head->id=p;
	head->name="shell";
	head->next=NULL;
}

static void
shell_request_loop(int request_fd, int return_fd)
{
	int ret;
	struct request_struct rq;

	/*
	 * Keep receiving requests from the shell.
	 */
	for (;;) {
		if (read(request_fd, &rq, sizeof(rq)) != sizeof(rq)) {
			perror("scheduler: read from shell");
			fprintf(stderr, "Scheduler: giving up on shell request processing.\n");
			break;
		}

		signals_disable();
		ret = process_request(&rq);
		signals_enable();

		if (write(return_fd, &ret, sizeof(ret)) != sizeof(ret)) {
			perror("scheduler: write to shell");
			fprintf(stderr, "Scheduler: giving up on shell request processing.\n");
			break;
		}
	}
}

int main(int argc, char *argv[])
{
	
	/* Two file descriptors for communication with the shell */
	static int request_fd, return_fd;

	/* Initialize the head and the end of the list to put the Shell on the head and the end as well because at the start Shell will be the only node-process */	
	head=(nodeptr)malloc(sizeof(Node));
	if(head == NULL){
		perror("malloc");
		exit(2);
	}
	end=head;

	/* Create the Shell and put Shell at the head of the list*/

	sched_create_shell(SHELL_EXECUTABLE_NAME, &request_fd, &return_fd);

	/* TODO: add the shell to the scheduler's tasks */


	/*
	 * For each of argv[1] to argv[argc - 1],
	 * create a new child process, add it to the process list.
	 */

	nproc = argc-1; /* number of proccesses goes here */

        nproc = argc-1; /* number of proccesses goes here */
        if (nproc < 0) {
                fprintf(stderr, "Scheduler: No tasks. Exiting...\n");
                exit(1);
        }

	char executable[] = "prog";
        char *newargv[] = { executable, NULL, NULL, NULL };
        char *newenviron[] = { NULL };

	nodeptr newnode;
        pid_t mypid;

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
			/* I dont have to check if my head is NULL because Shell is in the head for sure */
                        newnode=(nodeptr)malloc(sizeof(Node));
                        if (newnode == NULL){
				perror("malloc");
				exit(2);
			} 
                        newnode->id=mypid;
                        newnode->next=NULL;
                        newnode->data=i+1;
                        newnode->name=argv[i+1];
                        end->next=newnode; 
                        end=newnode;
                }
        }

		
	/* Wait for all children to raise SIGSTOP before exec()ing. */
	wait_for_ready_children(nproc);

	/* Install SIGALRM and SIGCHLD handlers. */
	install_signal_handlers();
	
        /*Set the alarm on*/
        alarm(SCHED_TQ_SEC);

        /*Start the first process*/
	kill(head->id,SIGCONT); //wake up the shell

	shell_request_loop(request_fd, return_fd);

	//kill(head->mypid,SIGCONT);

	/* Now that the shell is gone, just loop forever
	 * until we exit from inside a signal handler.
	 */
	while (pause())
		;

	/* Unreachable */
	fprintf(stderr, "Internal error: Reached unreachable point\n");
	return 1;
}
