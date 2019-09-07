#include <stdio.h>
#include <unistd.h>
//#include "zing.h"

void zing()
{
char *name;
name=getlogin();
if (!name)
    perror("getlogin() error");
else
    printf("This is a new message from %s\n", name);

}

