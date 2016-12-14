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

/*******************************************************************************
Global variables
*/
char    chromium_server[256];
unsigned char lvds_ptr[256];


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


    MON_PRINTF("START MAIN !\n\n");
    sprintf(cmd,"echo 1 > %s",URL_COM);
    system(cmd);

    bzero(chromium_server,sizeof(chromium_server));
    /* /etc/sysconfig/chromium_var contents are : CHROMIUM_SERVER=10.0.1.6 */
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
    /*********

    POWERON SELF TEST

    *********/



    res = IPTCom_prepareInit(0, "TFT1.car1.lCst_20160812.xml");
    MON_PRINTF("iptcom_startUp load res=%d\n",res);

    memset(&sa,0,sizeof(sa));
    sa.sa_handler = &terminationHandler;
    sigaction(SIGINT, &sa,NULL);

    do {
        MON_PRINTF("Wait for TDC to complete\n");
        IPTVosTaskDelay(1000);
        tdcGetIptState(&inAugState, &topoCnt);
    } while (0 == topoCnt);

    MON_PRINTF("\nStart Demo\n\n");

    res = applInit();
    if (res != IPT_OK)
    {
        MON_PRINTF("\nApplication init failed\n\n");
        return(-1);
    }
    MON_PRINTF("\nApplication init OK\n\n");

    while (1)
    {
        MON_PRINTF("MONITOR THREAD\n");
        printf(">");
       // getCmdString(cmdStr, LINELEN);
        IPTVosGetCh();
        printf("\n");
    }
    return EXIT_SUCCESS;
}
