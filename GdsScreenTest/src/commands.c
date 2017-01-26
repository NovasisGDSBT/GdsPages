#include "../include/GdsScreenTest.h"

void print_error(void)
{
    printf("Warning : no START command issued, nothing will be blitted!\n");
}
int stop_cmd(SDL_Surface *surface,char *command, char *parameter1, char *parameter2, char *parameter3,int ret_val)
{
    SDL_Quit();
    usleep(1000000);
    return ret_val;
}

int start_cmd(SDL_Surface *surface,char *command, char *parameter1, char *parameter2, char *parameter3,int ret_val)
{
    surface = initSDL(WIDTH,HEIGHT,BPP);
    screen = surface;
    system_started = 1;
    return ret_val;
}

int fill_red(SDL_Surface *surface,char *command, char *parameter1, char *parameter2, char *parameter3,int ret_val)
{
    if ( system_started != 1 )
    {
        print_error();
        return 1;
    }
    usleep(100000);
    fill_screen(screen,WIDTH,HEIGHT,255,0,0,0,0);
    return ret_val;
}

int fill_green(SDL_Surface *surface,char *command, char *parameter1, char *parameter2, char *parameter3,int ret_val)
{
    if ( system_started != 1 )
    {
        print_error();
        return 1;
    }
    fill_screen(screen,WIDTH,HEIGHT,0,255,0,0,0);
    return ret_val;
}

int fill_blue(SDL_Surface *surface,char *command, char *parameter1, char *parameter2, char *parameter3,int ret_val)
{
    if ( system_started != 1 )
    {
        print_error();
        return 1;
    }
    fill_screen(screen,WIDTH,HEIGHT,0,0,255,0,0);
    return ret_val;
}

int fill_black(SDL_Surface *surface,char *command, char *parameter1, char *parameter2, char *parameter3,int ret_val)
{
    if ( system_started != 1 )
    {
        print_error();
        return 1;
    }
    fill_screen(screen,WIDTH,HEIGHT,0,0,0,0,0);
    return ret_val;
}

int fill_white(SDL_Surface *surface,char *command, char *parameter1, char *parameter2, char *parameter3,int ret_val)
{
    if ( system_started != 1 )
    {
        print_error();
        return 1;
    }
    fill_screen(screen,WIDTH,HEIGHT,255,255,255,0,0);
    return ret_val;
}

int fill_gray(SDL_Surface *surface,char *command, char *parameter1, char *parameter2, char *parameter3,int ret_val)
{
    if ( system_started != 1 )
    {
        print_error();
        return 1;
    }
    fill_screen(screen,WIDTH,HEIGHT,64,64,64,0,0);
    return ret_val;
}

int fill_squareleft(SDL_Surface *surface,char *command, char *parameter1, char *parameter2, char *parameter3,int ret_val)
{
SDL_Rect    rect;
unsigned int colorkey;
    if ( system_started != 1 )
    {
        print_error();
        return 1;
    }
    colorkey = SDL_MapRGBA( surface->format, 0xff, 0xff, 0xff, 0xff ) ;
    fill_screen(screen,WIDTH,HEIGHT,0,0,0,0,0);
    rect.x = (WIDTH/4)-(SQUARE_DIM/2);
    rect.y = (VISIBLE_HEIGHT/2)-(SQUARE_DIM/2);
    rect.w = SQUARE_DIM;
    rect.h = SQUARE_DIM;
    SDL_FillRect(surface, &rect, colorkey);
    SDL_UpdateRect(surface,rect.x,rect.y,rect.w,rect.h);
    return ret_val;
}

int fill_squarecenter(SDL_Surface *surface,char *command, char *parameter1, char *parameter2, char *parameter3,int ret_val)
{
SDL_Rect    rect;
unsigned int colorkey;
    if ( system_started != 1 )
    {
        print_error();
        return 1;
    }
    colorkey = SDL_MapRGBA( surface->format, 0xff, 0xff, 0xff, 0xff ) ;
    fill_screen(screen,WIDTH,HEIGHT,0,0,0,0,0);
    rect.x = (WIDTH/2)-(SQUARE_DIM/2);
    rect.y = (VISIBLE_HEIGHT/2)-(SQUARE_DIM/2);
    rect.w = SQUARE_DIM;
    rect.h = SQUARE_DIM;
    SDL_FillRect(surface, &rect, colorkey);
    SDL_UpdateRect(surface,rect.x,rect.y,rect.w,rect.h);
    //printf("%s : Rect : %d %d %d %d\n",__FUNCTION__,rect.x,rect.y,rect.w,rect.h);
    return ret_val;
}

int fill_squareright(SDL_Surface *surface,char *command, char *parameter1, char *parameter2, char *parameter3,int ret_val)
{
SDL_Rect    rect;
unsigned int colorkey;
    if ( system_started != 1 )
    {
        print_error();
        return 1;
    }
    colorkey = SDL_MapRGBA( surface->format, 0xff, 0xff, 0xff, 0xff ) ;
    fill_screen(screen,WIDTH,HEIGHT,0,0,0,0,0);
    rect.x = (WIDTH-WIDTH/4)-(SQUARE_DIM/2);
    rect.y = (VISIBLE_HEIGHT/2)-(SQUARE_DIM/2);
    rect.w = SQUARE_DIM;
    rect.h = SQUARE_DIM;
    SDL_FillRect(surface, &rect, colorkey);
    SDL_UpdateRect(surface,rect.x,rect.y,rect.w,rect.h);
    //printf("%s : Rect : %d %d %d %d\n",__FUNCTION__,rect.x,rect.y,rect.w,rect.h);
    return ret_val;
}

int fill_squarecenter_reverse(SDL_Surface *surface,char *command, char *parameter1, char *parameter2, char *parameter3,int ret_val)
{
SDL_Rect    rect;
unsigned int colorkey;
    if ( system_started != 1 )
    {
        print_error();
        return 1;
    }
    colorkey = SDL_MapRGBA( surface->format, 0x0, 0x0, 0x0, 0xff ) ;
    fill_screen(screen,WIDTH,HEIGHT,0xff,0xff,0xff,0,0);
    rect.x = (WIDTH/2)-(SQUARE_DIM/2);
    rect.y = (VISIBLE_HEIGHT/2)-(SQUARE_DIM/2);
    rect.w = SQUARE_DIM;
    rect.h = SQUARE_DIM;
    SDL_FillRect(surface, &rect, colorkey);
    SDL_UpdateRect(surface,rect.x,rect.y,rect.w,rect.h);
    return ret_val;
}


int fill_borders(SDL_Surface *surface,char *command, char *parameter1, char *parameter2, char *parameter3,int ret_val)
{
SDL_Rect    rect;
unsigned int colorkey;

    if ( system_started != 1 )
    {
        print_error();
        return 1;
    }
    fill_screen(screen,WIDTH,HEIGHT,255,255,255,0,0);

    rect.x = 2; // was 2
    rect.y = 1;
    rect.w = WIDTH-3;
    rect.h = 356; // stops at  357
    colorkey = SDL_MapRGBA( surface->format, 0, 0, 0, 0xff ) ;
    SDL_FillRect(surface, &rect, colorkey);
    SDL_UpdateRect(surface,rect.x,rect.y,rect.w,rect.h);

    rect.x = 0;
    rect.y = 358;
    rect.w = WIDTH;
    rect.h = 1; // was 357
    colorkey = SDL_MapRGBA( surface->format, 0xff, 0, 0, 0xff ) ;
    SDL_FillRect(surface, &rect, colorkey);
    SDL_UpdateRect(surface,rect.x,rect.y,rect.w,rect.h);

    rect.x = 0;
    rect.y = 359;
    rect.w = WIDTH;
    rect.h = 1; // was 357
    colorkey = SDL_MapRGBA( surface->format, 0, 0xff, 0, 0xff ) ;
    SDL_FillRect(surface, &rect, colorkey);
    SDL_UpdateRect(surface,rect.x,rect.y,rect.w,rect.h);

    rect.x = 0;
    rect.y = 360;
    rect.w = WIDTH;
    rect.h = 1; // was 357
    colorkey = SDL_MapRGBA( surface->format, 0, 0, 0, 0xff ) ;
    SDL_FillRect(surface, &rect, colorkey);
    SDL_UpdateRect(surface,rect.x,rect.y,rect.w,rect.h);
    return ret_val;
}

int fill_borders_ORIGINAL(SDL_Surface *surface,char *command, char *parameter1, char *parameter2, char *parameter3,int ret_val)
{
SDL_Rect    rect;
unsigned int colorkey;

    if ( system_started != 1 )
    {
        print_error();
        return 1;
    }
    fill_screen(screen,WIDTH,HEIGHT,255,255,255,0,0);
    rect.x = 2;
    rect.y = 2;
    rect.w = WIDTH-4;
    rect.h = 355; // was 357
    colorkey = SDL_MapRGBA( surface->format, 0, 0, 0, 0xff ) ;
    SDL_FillRect(surface, &rect, colorkey);
    SDL_UpdateRect(surface,rect.x,rect.y,rect.w,rect.h);
    rect.x = 0;
    rect.y = 358;
    rect.w = WIDTH;
    rect.h = 6; // was 357
    colorkey = SDL_MapRGBA( surface->format, 0, 0, 0, 0xff ) ;
    SDL_FillRect(surface, &rect, colorkey);
    SDL_UpdateRect(surface,rect.x,rect.y,rect.w,rect.h);

    return ret_val;
}

void do_grayscale(int intervals)
{
int i,w,color,offsetx,offsety;

    color = 15;
    offsetx = 0;
    offsety = 0;
    w=WIDTH/intervals;
    for(i=0;i<intervals;i++)
    {
        fill_screen(screen,w,HEIGHT,color,color,color,offsetx,offsety);
        color += (256/intervals);
        offsetx = w;
        w += (WIDTH/intervals);
    }
}

int fill_grayscale12(SDL_Surface *surface,char *command, char *parameter1, char *parameter2, char *parameter3,int ret_val)
{
    if ( system_started != 1 )
    {
        print_error();
        return 1;
    }
    do_grayscale(12);
    return ret_val;
}

int fill_grayscale11(SDL_Surface *surface,char *command, char *parameter1, char *parameter2, char *parameter3,int ret_val)
{
    if ( system_started != 1 )
    {
        print_error();
        return 1;
    }
    do_grayscale(11);
    return ret_val;
}

char *check_fifo_msgs_loop(int pipe_fd)
{
    ssize_t length = 0;
    char    *command_ptr;
    char    command[2048];
    char    *return_ptr=NULL; //no receive any Commands

    command_ptr = command;
    length += read (pipe_fd, command_ptr, sizeof(command));
    if (length > 0)
    {
        if (command[length-1] == '\0')
        {
            //printf ("%s : Received %s, %d\n",__FUNCTION__,command,strncmp(command,"STOP",4));
            if ( strncmp(command,"STOP",4) == 0 )
            {
                unlink("/tmp/SdlSplash_fifo");
                exit(0);
            }
            else
            {
                return_ptr=command_ptr;
                printf("altro comando ricevuto %s\n",return_ptr);
            }
        }
    }

    return return_ptr;
}

#define LOOP_DELAY      3000000
#define uLOOP_DELAY       20000
extern  int         fifo_fd;

int sleep_and_check_counter;


char *sleep_and_check(void)
{
    char *retval=NULL; // no commands received
    char cmd_string[255];
    char *strRec=NULL;

    memset(cmd_string,0,sizeof(cmd_string));

    sleep_and_check_counter = 150;
    while ( sleep_and_check_counter-- > 0 )
    {
        strRec=check_fifo_msgs_loop(fifo_fd);

        if(strRec != NULL)
        {
            retval=strRec;
            break;
        }
        usleep(uLOOP_DELAY);
    }

    return retval;
}

int loop_test(SDL_Surface *surface,char *command, char *parameter1, char *parameter2, char *parameter3,int ret_val)
{
    if ( system_started != 1 )
    {
        print_error();
        return 1;
    }
    fill_screen(screen,WIDTH,HEIGHT,255,0,0,0,0);
    printf("red done\n");
    sleep_and_check_counter = uLOOP_DELAY;
    sleep_and_check();

    fill_screen(screen,WIDTH,HEIGHT,0,255,0,0,0);
    printf("green done\n");
    sleep_and_check();

    fill_screen(screen,WIDTH,HEIGHT,0,0,255,0,0);
    printf("blue done\n");
    sleep_and_check();

    fill_screen(screen,WIDTH,HEIGHT,0,0,0,0,0);
    printf("black done\n");
    sleep_and_check();

    fill_screen(screen,WIDTH,HEIGHT,255,255,255,0,0);
    printf("white done\n");
    sleep_and_check();

    do_grayscale(11);
    printf("grayscale done\n");
    sleep_and_check();

    do_grayscale(12);
    printf("grayscale done\n");
    sleep_and_check();

    fill_borders(screen,NULL,NULL,NULL,NULL,0);
    printf("grayscale done\n");
    sleep_and_check();

    fill_squareleft(screen,NULL,NULL,NULL,NULL,0);
    printf("grayscale done\n");
    sleep_and_check();

    fill_squarecenter(screen,NULL,NULL,NULL,NULL,0);
    printf("grayscale done\n");
    sleep_and_check();

    fill_squareright(screen,NULL,NULL,NULL,NULL,0);
    printf("grayscale done\n");
    sleep_and_check();

    return ret_val;
}

char   value[128];
void file_helper(char *fname)
{
FILE *fp;
    fp=fopen(fname,"r");
    if ( fp !=NULL)
    {
        fscanf(fp,"%s",value);
        fclose(fp);
    }
}

void create_blit(SDL_Surface *surface,char *Text, SDL_Color STRINGFontColor, SDL_Color STRINGFontBgColor,SDL_Rect rect)
{
SDL_Surface     *TextImage;
    TextImage = TTF_RenderText_Shaded(STRINGFont ,Text,STRINGFontColor,STRINGFontBgColor);
    if ( TextImage == NULL)
    {
        printf("%s TextImage is NULL , Text is %s\n",__FUNCTION__,Text);
        exit(0);
    }
    SDL_BlitSurface(TextImage,NULL,surface,&rect);
    SDL_UpdateRect(surface,rect.x,rect.y,rect.w,rect.h);
    SDL_FreeSurface(TextImage);
}

int diag_page(SDL_Surface *surface,char *command, char *parameter1, char *parameter2, char *parameter3,int ret_val)
{
char            Text[512];
SDL_Rect        rect = { 4,4,1920,1080};
SDL_Color       STRINGFontColor = { 0x3f,0xaa,0xcd } , STRINGFontBgColor = { 0xec,0xec,0xe1 } ;  //376093

char BackOnCounter[32];
char DurationCounter[32];
char NormalStartsCounter[32];
char WDogsResetsCounter[32];
char FBacklightFault[32];
char FTempFault[32];
char FTempOr[32];
char FAmbLightSensor[32];
int  temp_limit_low, temp_limit_high,carrier_temp;

    if ( system_started != 1 )
    {
        print_error();
        return 1;
    }
    fill_screen(screen,WIDTH,HEIGHT,255,255,255,0,0); /* white*/

    /* Software */
    system("cat /tmp/sw_version | sed 's/IMAGE_REV=//g' > /tmp/vs");
    file_helper("/tmp/vs");
    sprintf(Text,"System Software  : %s",value);
    create_blit(surface,Text, STRINGFontColor, STRINGFontBgColor,rect);
    rect.y += 36;

    /* Hardware */
    system("cat /tmp/hw_version | grep BOARD_REV | sed 's/BOARD_REV=//g' > /tmp/vn");
    file_helper("/tmp/vn");
    sprintf(Text,"System Hardware : %s ",value);
    create_blit(surface,Text, STRINGFontColor, STRINGFontBgColor,rect);
    rect.y += 36;
    /* */
    sprintf(Text,"INFDISReport.ISystemMode : 5 -- INFDISReport.ITestMode : 18 -- INFDISReport.FBacklightStatus : ok");
    create_blit(surface,Text, STRINGFontColor, STRINGFontBgColor,rect);
    rect.y += 36;

    system("cat /tmp/hw_version | grep MONITOR_SN| sed 's/MONITOR_SN=//g' > /tmp/vn");
    file_helper("/tmp/vn");
    sprintf(Text,"INFDISReport.IUniqueSerialNo : %s -- INFDISReport.ISupplierName : GDS -- INFDISReport.IDevType : PIS",value);
    create_blit(surface,Text, STRINGFontColor, STRINGFontBgColor,rect);
    rect.y += 36;

    file_helper("/tmp/my_ip"); /* */
    sprintf(Text,"INFDISReport.IDevInstance : 1 -- INFDISReport.IDevIpAddress : %s -- INFDISReport.IDevFqdn : xxx -- INFDISReport.ISystemLifeSign : Active",value);
    create_blit(surface,Text, STRINGFontColor, STRINGFontBgColor,rect);
    rect.y += 36;

    system("cat /tmp/backlight_on_counter | grep BACKLIGHT_ON_COUNTER | sed 's/BACKLIGHT_ON_COUNTER=//g' > /tmp/vn");
    file_helper("/tmp/vn");
    sprintf(BackOnCounter,"%s",value);

    system("cat /tmp/monitor_on_counter | grep MONITOR_ON_COUNTER | sed 's/MONITOR_ON_COUNTER=//g' > /tmp/vn");
    file_helper("/tmp/vn");
    sprintf(DurationCounter,"%s",value);

    system("cat /tmp/reboot_counter | grep REBOOT_COUNTER | sed 's/REBOOT_COUNTER=//g' > /tmp/vn");
    file_helper("/tmp/vn");
    sprintf(NormalStartsCounter,"%s",value);

    system("cat /tmp/wdog_counter | grep WATCHDOG_COUNTER | sed 's/WATCHDOG_COUNTER=//g' > /tmp/vn");
    file_helper("/tmp/vn");
    sprintf(WDogsResetsCounter,"%s",value);

    file_helper("/tmp/ext_backlight_fault");
    sprintf(FBacklightFault,"%s",value);

    file_helper("/tmp/ext_temp_fault");
    sprintf(FTempFault,"%s",value);

    file_helper("/tmp/tempLimitsDOWN");
    temp_limit_low=atoi(value);

    file_helper("/tmp/tempLimitsUP");
    temp_limit_high=atoi(value);

    system("cat /tmp/carrier_temp | grep INTERNAL_TEMPERATURE | sed 's/INTERNAL_TEMPERATURE=//g' > /tmp/vn");
    file_helper("/tmp/vn");
    carrier_temp=atoi(value);

    if (( carrier_temp > temp_limit_high) || (carrier_temp < temp_limit_low ))
        sprintf(FTempOr,"1");
    else
        sprintf(FTempOr,"0");

    printf("temp_limit_high=%d , temp_limit_low=%d,carrier_temp=%d , FTempOr=%s\n",temp_limit_high,temp_limit_low,carrier_temp,FTempOr);

    system("cat /tmp/ambientlight_value | grep AMBIENT_LIGHT | sed 's/AMBIENT_LIGHT=//g' > /tmp/vn");
    file_helper("/tmp/vn");
    sprintf(FAmbLightSensor,"%s",value);

    sprintf(Text,"INFDISReport.IDurationCounter : %s -- INFDISReport.IBackOnCounter : %s -- INFDISReport.INormalStartsCounter : %s -- INFDISReport.IWDogResetsCounter : %s",
            DurationCounter,BackOnCounter,NormalStartsCounter,WDogsResetsCounter);
    create_blit(surface,Text, STRINGFontColor, STRINGFontBgColor,rect);
    rect.y += 36;

    sprintf(Text,"INFDISReport.FBacklightFault : %s -- INFDISReport.FTempFault : %s -- INFDISReport.FTempOr : %s -- INFDISReport.FAmbLightSensor : %s",
            FBacklightFault,FTempFault,FTempOr,FAmbLightSensor);
    printf("%s\n",Text);
    create_blit(surface,Text, STRINGFontColor, STRINGFontBgColor,rect);
    rect.y += 36;

    return ret_val;
}

int btloop_test(SDL_Surface *surface,char *command, char *parameter1, char *parameter2, char *parameter3,int ret_val)
{
    char    *innerCommand=NULL;
    ssize_t length = 0;

    if ( system_started != 1 )
    {
        print_error();
        return 1;
    }
    while ( 1 )
    {
        fill_screen(screen,WIDTH,HEIGHT,255,0,0,0,0);
        printf("red done\n");
        sleep_and_check_counter = uLOOP_DELAY;
        if((innerCommand=sleep_and_check())!=NULL)
        {
            break;
        }

        fill_screen(screen,WIDTH,HEIGHT,0,255,0,0,0);
        printf("green done\n");
        if((innerCommand=sleep_and_check())!=NULL)
        {
            break;
        }

        fill_screen(screen,WIDTH,HEIGHT,0,0,255,0,0);
        printf("blue done\n");
        if((innerCommand=sleep_and_check())!=NULL)
        {
            break;
        }

        fill_screen(screen,WIDTH,HEIGHT,0,0,0,0,0);
        printf("black done\n");
        if((innerCommand=sleep_and_check())!=NULL)
        {
            break;
        }

        fill_screen(screen,WIDTH,HEIGHT,255,255,255,0,0);
        printf("white done\n");
        if((innerCommand=sleep_and_check())!=NULL)
        {
            break;
        }

        do_grayscale(11);
        printf("grayscale done\n");
         if((innerCommand=sleep_and_check())!=NULL)
        {
            break;
        }
    }

    if(innerCommand  != NULL)
    {
        length=strlen(innerCommand);
        decodecmds(screen,innerCommand,length);
    }

    return ret_val;
}

int image_test(SDL_Surface *surface,char *command, char *parameter1, char *parameter2, char *parameter3,int ret_val)
{
SDL_Surface *image;
    image = load_image("/tmp/www/page/bkg.png");
    SDL_BlitSurface(image,NULL,screen,NULL);
    SDL_Flip(screen);
    return ret_val;
}
