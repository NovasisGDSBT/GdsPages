#include "../include/GdsScreenTest.h"

SDL_Surface *screen;
SDL_Event       event;
char    keys[8];
char    command[256];
int brightness=128;
int         fifo_fd;
int system_started = 0;


int check_fifo_msgs(int pipe_fd, int timeout)
{
int            err;
ssize_t        length = 0;
fd_set         descriptors;
struct timeval tv;
char          *command_ptr;
char           command[2048];

    while (1)
    {
        command_ptr = command;
        tv.tv_sec = 0;
        tv.tv_usec = timeout*1000;
        FD_ZERO(&descriptors);
        FD_SET(pipe_fd, &descriptors);

        if (timeout != 0)
            err = select(pipe_fd+1, &descriptors, NULL, NULL, &tv);
        else
            err = select(pipe_fd+1, &descriptors, NULL, NULL, NULL);
        if ( err >= 0 )
        {
            length += read (pipe_fd, command_ptr, sizeof(command));
            if (length > 0)
            {
                if (command[length-1] == '\0')
                {
                    printf ("Received %s\n",command);
                    if ( decodecmds(screen,command,length) == STOP)
                        return 0;
                    length = 0;
                }
            }
            else
            {
                close(pipe_fd);
                pipe_fd = open(NAMED_FIFO,O_RDONLY|O_NONBLOCK);
            }
        }
        else
        {
            if ( err < 0 )
            {
                printf("err = %d\n",err);
                return -1;
            }
        }
    }
}

int create_fifo(void)
{
char    *tmpdir;
int     ret,fd;

    tmpdir = getenv("TMPDIR");
    if (!tmpdir)
        tmpdir = "/tmp";
    ret = chdir(tmpdir);
    if (mkfifo(NAMED_FIFO, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP))
    {
        if (errno!=EEXIST)
        {
            printf("%d : mkfifo : unable to create fifo %s\n",ret,NAMED_FIFO);
            exit(-1);
        }
    }
    fd = open (NAMED_FIFO,O_RDONLY|O_NONBLOCK);
    if (fd < 0)
    {
        printf("%d : pipe open failed on %s\n",fd,NAMED_FIFO);
        exit(-2);
    }
    return fd;
}

int main()
{

    fifo_fd = create_fifo();
    check_fifo_msgs(fifo_fd,25);
    unlink("/tmp/SdlSplash_fifo");
    return 0;
}
