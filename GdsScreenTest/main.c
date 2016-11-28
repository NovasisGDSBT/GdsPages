/*
 *  pslash - a lightweight framebuffer splashscreen for embedded devices.
 *
 *  Copyright (c) 2006 Matthew Allum <mallum@o-hand.com>
 *
 *  Parts of this file based on 'usplash' copyright Matthew Garret.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 */

#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#define NAMED_FIFO "GdsScreenTest_fifo"

int main(int argc, char **argv)
{
char *tmpdir,command[256];
int   pipe_fd,i,ret;

    tmpdir = getenv("TMPDIR");

    if (!tmpdir)
        tmpdir = "/tmp";

    if (argc<2)
    {
        fprintf(stderr, "Wrong number of arguments\n");
        exit(-1);
    }

    ret=chdir(tmpdir);

    if ((pipe_fd = open (NAMED_FIFO,O_WRONLY|O_NONBLOCK)) == -1)
    {
        perror("Error unable to open fifo");
        exit (-1);
    }
    bzero(command,sizeof(command));
    for(i=1;i<argc;i++)
    {
        strcat(command,argv[i]);
        strcat(command," ");

    }
    ret=write(pipe_fd, command, strlen(command)+1);
    return 0;
}



