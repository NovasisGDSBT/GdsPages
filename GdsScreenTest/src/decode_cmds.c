#include "../include/GdsScreenTest.h"

CommandsStruct Commands[]=
{
    {
        .command = "STOP",
        .fun_ptr = stop_cmd,
        .retval = STOP,
    },
    {
        .command = "START",
        .fun_ptr = start_cmd,
        .retval = 0,
    },
    {
        .command = "FILL_RED",
        .fun_ptr = fill_red,
        .retval = 0,
    },    {
        .command = "FILL_GRAY",
        .fun_ptr = fill_gray,
        .retval = 0,
    },
    {
        .command = "FILL_GREEN",
        .fun_ptr = fill_green,
        .retval = 0,
    },
    {
        .command = "FILL_BLUE",
        .fun_ptr = fill_blue,
        .retval = 0,
    },
    {
        .command = "FILL_BLACK",
        .fun_ptr = fill_black,
        .retval = 0,
    },
    {
        .command = "FILL_WHITE",
        .fun_ptr = fill_white,
        .retval = 0,
    },
    {
        .command = "FILL_GRAYSCALE12",
        .fun_ptr = fill_grayscale12,
        .retval = 0,
    },
    {
        .command = "FILL_GRAYSCALE11",
        .fun_ptr = fill_grayscale11,
        .retval = 0,
    },
    {
        .command = "FILL_SQUARELEFT",
        .fun_ptr = fill_squareleft,
        .retval = 0,
    },
    {
        .command = "FILL_SQUARECENTER",
        .fun_ptr = fill_squarecenter,
        .retval = 0,
    },
    {
        .command = "FILL_SQUARERIGHT",
        .fun_ptr = fill_squareright,
        .retval = 0,
    },
    {
        .command = "FILL_SQUARECENTER_REVERSE",
        .fun_ptr = fill_squarecenter_reverse,
        .retval = 0,
    },
    {
        .command = "FILL_BORDERS",
        .fun_ptr = fill_borders,
        .retval = 0,
    },
    {
        .command = "LOOP_TEST",
        .fun_ptr = loop_test,
        .retval = 0,
    },
    {
        .command = "DIAG_PAGE",
        .fun_ptr = diag_page,
        .retval = 0,
    },
    {
        .command = "BTLOOP_TEST",
        .fun_ptr = btloop_test,
        .retval = 0,
    },
    {
        .command = "IMAGE_TEST",
        .fun_ptr = image_test,
        .retval = 0,
    },
};

int array_len=(int )(sizeof(Commands) / sizeof(Commands[0]));

int decodecmds(SDL_Surface *surface,char *command_ptr , int len)
{
int     i;
char    *command,*parameter1,*parameter2,*parameter3;

    command = strtok(command_ptr," ");
    parameter1 = strtok(NULL," ");
    parameter2 = strtok(NULL," ");
    parameter3 = strtok(NULL,"\0");

    for (i=0;i<array_len;i++)
        if ( strncmp(Commands[i].command,command,strlen(command)) == 0 )
            return Commands[i].fun_ptr(surface,command,parameter1,parameter2,parameter3,Commands[i].retval);
    printf("Invalid command received %s\n",command);
    return 0;
}

