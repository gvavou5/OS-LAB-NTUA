#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define BUFFER_SIZE 512

void create_out_file(char * , char *, char *);//auth h sunarthsh kaleitai otan ta orismata einai ths morfhs A B C h A B 
void read_and_write(int,const char *);
int do_read(int, char *, int);
void do_write(int, const char *, int);
void create_out_file1(char *, char *, char *);//auth h sunarthsh kaleitai an ta orismata einai ths morfhs A B A
void create_out_file2(char *, char *, char *);//auth h sunarthsh kaleitai an ta orismata einai ths morfhs A B B 

int main(int argc, char **argv)
{
	if (argc<3 || argc>=5)
	{
		//8ewrw oti an parw panw apo 3 arxeia san orisma tupwnw to mhnuma kai teleiwnw
		printf("Usage: ./fconc infile1 infile2 [outfiel]\n");
	}
	else if (argc == 3)
	{	
		//an den exw 3o orisma tote 8a prepei na valw to default pou mou dinei h ekfwnhsh 
		create_out_file(argv[1], argv[2], NULL);
	}
	else
	{
		if (strcmp(argv[1], argv[3]) == 0) create_out_file1(argv[1], argv[2],argv[3]);
		else if(strcmp(argv[2], argv[3]) == 0) create_out_file2(argv[1],argv[2],argv[3]); 
		else create_out_file( argv[1], argv[2] , argv[3]);
	}
	return 0;
}


void create_out_file2(char * file1, char * file2, char * file3)
{
        char * out_file="fconc.out";
        int fd, oflags, mode,fd1;
        oflags = O_CREAT | O_WRONLY;
        mode = S_IRUSR | S_IWUSR;
	fd1= open(out_file, oflags,mode);
        fd = open(file3, oflags, mode);
        if (fd == -1 || fd1 == -1)
        {
                perror("open");
                //printf("There is an error in outpout file\n");
                exit(1);
        }
        else
        {
        //edw diavazeis kai grafeis ta 2 arxeia eisodou diadoxika kai ta grafeis sto arxeio eksodou
                read_and_write(fd1,file3);
		read_and_write(fd, file1);
                read_and_write(fd, out_file);
        }
        close(fd);//kleinw to out_file
}

void create_out_file1(char * file1, char * file2, char * file3)
{
        //edw tsekarw poio einai to outpout file
        char * out_file;
        if (file3 == NULL) out_file = "fconc.out";
        else out_file = file3 ;

        int fd, oflags, mode;
        oflags = O_CREAT | O_WRONLY;
        mode = S_IRUSR | S_IWUSR;
        fd = open(out_file, oflags, mode);
        if (fd == -1)
        {
                perror("open");
                //printf("There is an error in outpout file\n");
                exit(1);
        }
        else
        {
        //edw diavazeis kai grafeis ta 2 arxeia eisodou diadoxika kai ta grafeis sto arxeio eksodou
                read_and_write(fd, file1);
                read_and_write(fd, file2);
        }
        close(fd);//kleinw to out_file
}

void create_out_file(char * file1, char * file2, char * file3)
{	
	//edw tsekarw poio einai to outpout file
	char * out_file;
	if (file3 == NULL) out_file = "fconc.out";
	else out_file = file3 ;
	int fd, oflags, mode;
	oflags = O_CREAT | O_WRONLY | O_TRUNC;
	mode = S_IRUSR | S_IWUSR;
	fd = open(out_file, oflags, mode); 	
	if (fd == -1)
	{
       		perror("open");
		//printf("There is an error in outpout file\n");
		exit(1);
	}
	else 
	{
	//edw diavazeis kai grafeis ta 2 arxeia eisodou diadoxika kai ta grafeis sto arxeio eksodou
		read_and_write(fd, file1);
		read_and_write(fd, file2);
	}
	close(fd);//kleinw to out_file
}


void read_and_write(int fd, const char * file1)
{
	int len;
	//int buffer_size=1024;
	char buffer[BUFFER_SIZE];
	int fd1 = open(file1, O_RDONLY);
	if (fd1 == -1)
	{
       		perror("open");
		printf("There is a problem at the opening , please check the code\n");
		exit(1);
	}
	while ( (len = do_read(fd1, buffer, BUFFER_SIZE)) > 0 )
	{	
		//diavazw me thn do_read kai meta auto pou diavasa to grafw me thn do_write sto out_file
		do_write(fd, buffer,len );
	}
	close(fd1);// kleinw ta arxeia eisodou (prwta to 1o kai meta to 2o)
}



int do_read(int fd,char * buffer, int buf_size)
{
	int result=0, reader;
	// mporousa na valw ssize_t anti gia int gia ton reader wstoso to apotelesma den allazei
	//ssize_t : This data type is used to represent the sizes of blocks that can be read or written in 	  //a single operation. It is similar to size_t, but must be a signed type.	
	//size_t is an unsigned integer type of at least 16 bit
	while (buf_size > 0)
	{
		reader = read(fd, buffer, buf_size);
		if (reader == -1) 
		{
			perror("read");
			printf("There is a problem at reading\n");
			exit(1);
		}
		else if (reader == 0)
		{
			//this is the end of file
			return result;
		}
		else
		{
			result += reader;
			buf_size -= reader;
			buffer += reader;
		}
	}
	return result;	
}

	
/*
void  do_write(int fd,const char * buffer, int len)
{
	int writter = write (fd, buffer, len);
	if (writter == -1 ) 
	{
		perror("write");
		printf("There is a problem in writing\n");
		exit(1);
	}
	if (writter <  len) // allos tropos einai to !=
	{
		do_write(fd, buffer, len-writter);
		printf("Hello\n");
	}
}
*/

///*
void do_write(int fd, const char *buffer, int len)
{
	int idx=0;
	int wcnt;
	//len = strlen(buffer); 
	do{
		wcnt = write(fd,buffer + idx, len - idx); 
		if (wcnt == -1){ 
              		perror("write");
			printf("There is an error in writing\n");
			exit(1); 
		}
		idx += wcnt;
		//len -= wcnt;
	}while (idx < len);
}
//*/
