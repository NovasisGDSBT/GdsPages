/*******************************************************************************
*  COPYRIGHT      : (c) 2006-2010 Bombardier Transportation
********************************************************************************
*  PROJECT        : IPTrain
*
*  MODULE         : GDSBT_iptcom.c
*
*  ABSTRACT       :  application for start up
*
********************************************************************************
*  HISTORY     :
*
*  $Id: GDSBT_iptcom.c 39480 2015-11-06 05:17:02Z atnatu $
*
*  MODIFICATIONS (log starts 2010-08-11)
*
*  CR-432 (Gerhard Weiss, 12-Oct-2010)
*  Removed warnings provoked by -W (for GCC)
*
*
*******************************************************************************/

/*******************************************************************************
* INCLUDES */

#include <string.h>

#include <stdlib.h>
#include <signal.h>
#include <string.h>

#include "iptcom.h"        /* Common type definitions for IPT  */
#include "vos.h"           /* OS independent system calls */

#include "vos_priv.h"
#include "mdcom_priv.h"
#include "iptcom_priv.h"
#include "PIS_Common.h"
#include "PIS_App.h"
#include "PIS_Diag.h"
#include "PIS_IptCom.h"
#include <SDL/SDL.h>
#include <SDL/SDL_gfxPrimitives.h>

/*******************************************************************************
Global variables
*/
char    chromium_server[256];
unsigned char lvds_ptr[256];
int iptcom_connected = 0;

int yellow_square_time=3, green_square_time=3 ,red_square_time=3, iptcom_timeout=10, infotainment_timeout = 15;
int page_exists=0;

/*******************************************************************************
NAME:       LOG_SYS
ABSTRACT:
RETURNS:
*/

int LOG_SYS(char *logtype,char * type, char * funcname, char *message )
{
    int retval=0;
    char cmd[255];

    memset(cmd,0,sizeof(cmd));

    if((logtype!=NULL) && (type!=NULL) && (funcname!=NULL) && (message!=NULL) )
    {
         if ( strncmp(logtype,SYSDIAG,4)== 0 )
         {
            snprintf(cmd,sizeof(cmd)-1,"/tmp/www/logwrite.sh %s %s %s '%s'",SYSDIAG,type,funcname,message);

         }
         else if ( strncmp(logtype,DATAREC,4)== 0 )
         {
             snprintf(cmd,sizeof(cmd)-1,"/tmp/www/logwrite.sh %s %s %s '%s'",DATAREC,type,funcname,message);
         }
         else if ( strncmp(logtype,APPACTI,4)== 0 )
         {
             snprintf(cmd,sizeof(cmd)-1,"/tmp/www/logwrite.sh %s %s %s '%s'",APPACTI,type,funcname,message);
         }
         else
         {
            snprintf(cmd,sizeof(cmd)-1,"echo LOG_SYS:logtype define wrong");
         }
          system(cmd);

    }
    else
    {
        retval=1;
    }

   return(retval);
}

/*******************************************************************************
NAME:       IPTVosGetCh
ABSTRACT:   Returns key from keyboard buffer
RETURNS:    Ascii code of key
*/
static int IPTVosGetCh()
{
int c;
   while ((c =getchar()) == EOF)
   {
   }
   return(c);
}


/*******************************************************************************
NAME:       applInit
ABSTRACT:
RETURNS:    -
*/

int applInit()
{
int res;

res=IptComStart();

    if (res != IPT_OK)
        return(res);

    res=Application();

    if (res != IPT_OK)
        return(res);

    res=Diagnostic();

    if (res != IPT_OK)
        return(res);
    return(0);
}

/*******************************************************************************
NAME:       terminationHandler
ABSTRACT:
RETURNS:    -
*/

static void terminationHandler(int signal_number)
{
   IptComClean();
   IPTVosThreadTerminate();
   IPTCom_terminate(0);
   IPTCom_destroy();
   exit(EXIT_SUCCESS);
}


/*******************************************************************************
NAME:       main
ABSTRACT:
RETURNS:    -
*/

int main(int argc, char *argv[])
{
FILE *fp;
int res;
UINT8 inAugState, topoCnt;
struct sigaction sa;
char    cmd[64];
int     do_square_diag = 1;
int     i;

    memset(cmd,0,sizeof(cmd));

    printf("argc = %d\n", argc);
    for(i=1;i<=argc;i++)
        printf("%s : at %d argv is %s\n",__func__,i,argv[i]);
    if ( argc == 6)
    {
        yellow_square_time = atoi(argv[1]);
        green_square_time  = atoi(argv[2]);
        red_square_time    = atoi(argv[3]);
        iptcom_timeout     = atoi(argv[4]);
        infotainment_timeout= atoi(argv[5]);
        page_exists        = atoi(argv[6]);
    }
    printf("yellow_square_time      = %d\n",yellow_square_time);
    printf("green_square_time       = %d\n",green_square_time);
    printf("red_square_time         = %d\n",red_square_time);
    printf("iptcom_timeout          = %d\n",iptcom_timeout);
    printf("infotainment_timeout    = %d\n",infotainment_timeout);
    printf("page_exists             = %d\n",page_exists);

    MON_PRINTF("START MAIN !\n\n");
    sprintf(cmd,"echo 1 > %s",URL_COM);
    system(cmd);

    bzero(chromium_server,sizeof(chromium_server));
    fp=fopen("/tmp/warm_boot","r");
    if ( fp != NULL)
    {
        do_square_diag = 0;
        fclose(fp);
    }
    system("touch /tmp/warm_boot");

    fp=fopen("/etc/sysconfig/chromium_var","r");
    if ( fp !=NULL)
    {
        fscanf(fp,"%s",chromium_server);
        fclose(fp);
    }
    printf("%s : chromium_server = %s\n",__FUNCTION__,chromium_server);
    curr_mode=NORMAL;

    fp = fopen("/tmp/www/cgi-bin/lvds_device","r");
    if ( fp )
    {
        fscanf(fp,"%s",lvds_ptr);
        fclose(fp);
    }

    /* CHECK ERROR POWERON SELF TEST */
    checkErrorPost();


    res = IPTCom_prepareInit(0, "TFT1.car1.lCst_20160812.xml");
    MON_PRINTF("iptcom_startUp load res=%d\n",res);

    memset(&sa,0,sizeof(sa));
    sa.sa_handler = &terminationHandler;
    sigaction(SIGINT, &sa,NULL);

    if ( do_square_diag == 1 )
    {
        LOG_SYS(APPACTI,INFO, "IPTCOM_MAIN_TASK","Waiting TCMS: showing yellow square");
        do_sdl();
        do {
            yellow_square_time--;
            IPTVosTaskDelay(1000);
        } while (yellow_square_time > 0);
    }


    do {
        MON_PRINTF("Wait for TDC to complete\n");
        IPTVosTaskDelay(1000);
        tdcGetIptState(&inAugState, &topoCnt);
        //yellow_square_time--;
        iptcom_timeout--;
        if ( iptcom_timeout < 0)
        {
            snprintf(cmd,sizeof(cmd)-1,"Iptcom Connection Timeout: %d seconds",iptcom_timeout);
            LOG_SYS(APPACTI,ERR, "IPTCOM_MAIN_TASK",cmd);
            break;
        }
    } while (0 == topoCnt);

    //MON_PRINTF("\nStart Demo\n\n");

    res = applInit();
    if (res != IPT_OK)
    {
        MON_PRINTF("\nApplication init failed\n\n");
        system("echo CHROMIUM_SERVER=\"http://127.0.0.1:8080/test_default_page/default_page.html\" > /etc/sysconfig/chromium_var");
        system("echo \"http://127.0.0.1:8080/test_default_page/default_page.html\" > /tmp/www/url.txt");
        if ( do_square_diag == 1 )
            SDL_Quit();
        system("sleep 1 ; touch /tmp/start_chrome");
        LOG_SYS(APPACTI,ERR, "IPTCOM_MAIN_TASK","Showing default page");
        return(-1);
    }
    MON_PRINTF("\nApplication init OK\n\n");

    if ( do_square_diag == 1 )
    {
        draw_green();
        LOG_SYS(APPACTI,ERR, "IPTCOM_MAIN_TASK","IPTCOM connection established: showing green square");
        IPTVosTaskDelay(green_square_time*1000);
    }


    if ( page_exists == 0 )
    {
        if ( do_square_diag == 1 )
        {
            draw_red();
            IPTVosTaskDelay(red_square_time*1000);
            LOG_SYS(APPACTI,ERR, "IPTCOM_MAIN_TASK","No valid URL found: showing red square");
            SDL_Quit();
        }

        system("echo CHROMIUM_SERVER=\"http://127.0.0.1:8080/test_default_page/default_page.html\" > /etc/sysconfig/chromium_var");
        system("echo \"http://127.0.0.1:8080/test_default_page/default_page.html\" > /tmp/www/url.txt");
    }
    else
    {
        sprintf(cmd,"echo 0 > %s",URL_COM);
        system(cmd);
    }
    if ( do_square_diag == 1 )
        SDL_Quit();
    IPTVosTaskDelay(200);
    system("startx &");
    while (1)
    {
        MON_PRINTF("MONITOR THREAD\n");
        printf(">");
       // getCmdString(cmdStr, LINELEN);
        IPTVosGetCh();
        printf("\n");
        iptcom_connected = 1;
    }
    return EXIT_SUCCESS;
}
