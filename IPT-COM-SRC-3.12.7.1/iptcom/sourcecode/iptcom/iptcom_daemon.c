/*******************************************************************************
*  COPYRIGHT   : (C) 2006 Bombardier Transportation
********************************************************************************
*  PROJECT     :  IPTrain
*
*  MODULE      :  iptcom_daemon.c
*
*  ABSTRACT    :  Starts IPTCom as a daemon:
*
********************************************************************************
*  HISTORY     :
*	
* $Id: iptcom_daemon.c 33379 2014-06-30 08:42:59Z gweiss $
*
*  Internal (Bernd Loehr, 2010-08-16) 
* 			Old obsolete CVS history removed
*
*
*
*******************************************************************************/

/*******************************************************************************
*  INCLUDES */
#if defined(LINUX) || defined(DARWIN)
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#endif

#include "iptcom.h"        /* Common type definitions for IPT */
#include "vos.h"
#include "iptcom_priv.h" 

/*******************************************************************************
*  DEFINES
*/

/*******************************************************************************
*  TYPEDEFS
*/

/*******************************************************************************
*  GLOBAL DATA
*/

/*******************************************************************************
*  LOCAL DATA */

static  IPT_SEM dummySemId;

/*******************************************************************************
*  LOCAL FUNCTION DECLARATIONS
*/

/*******************************************************************************
*  LOCAL FUNCTIONS
*/

/*******************************************************************************
NAME:      terminationHandler 
ABSTRACT:  Terminate IPTCom 
RETURNS:   - 
*/
static void terminationHandler(int signal_number)
{
   IPT_UNUSED(signal_number)
   
   IPTVosThreadTerminate();
   IPTCom_terminate(0);
   IPTCom_destroy();  
   /* Destroy a semaphore  */
   IPTVosDestroySem(&dummySemId);
   exit(EXIT_SUCCESS);
}

/*******************************************************************************
*  GLOBAL FUNCTIONS
*/

/*******************************************************************************
NAME:      main 
ABSTRACT:  Start IPTCom as a daemon 
RETURNS:    
*/
int main(int argc, char *argv[])
{
   int res;
#ifndef WIN32
   pid_t pid;
   pid_t sid;
   struct sigaction sa;
#endif
   int largc = argc;

#ifndef WIN32
   /* Fork off the parent process */
   pid = fork();
   if (pid < 0)
   {
      printf("iptcom_daemon FAILED, fork errno=%d\n", errno);
      exit(EXIT_FAILURE);
   }
   else if (pid > 0)
   {
      /* exit parent process */
      exit(EXIT_SUCCESS);
   }

   /* continue with child process */
#endif

   /* Create a semaphore */
   res = IPTVosCreateSem(&dummySemId, 0);
   if (res != (int)IPT_OK)
   {
      printf("iptcom_daemon ERROR creating  dummySemId error=%#x\n", res);
      exit(EXIT_FAILURE);
   }

#ifndef WIN32  
   /* Change file mode mask */
   umask(0);

   /* Create a new SID for the child process */
   sid = setsid();
   if (sid < 0)
   {
      printf("iptcom_daemon FAILED, setsid errno=%d\n", errno);
      exit(EXIT_FAILURE);
   }


   if (argc > 1)
   {
      if ((strcmp("-t",argv[1])) == 0)
      {
         largc--;
      }
      else
      {
         /* Close out the standard file descriptors */
         close(STDIN_FILENO);
         close(STDOUT_FILENO);
         close(STDERR_FILENO);
      }
   }
   else
   {
      /* Close out the standard file descriptors */
      close(STDIN_FILENO);
      close(STDOUT_FILENO);
      close(STDERR_FILENO);
   }

   memset(&sa,0,sizeof(sa));
   sa.sa_handler = &terminationHandler;
   sigaction(SIGTERM, &sa,NULL);

#endif
   if (largc > 2)
   {
      printf("iptcom_daemon reading TDC emulate file %s\n",argv[2 + argc - largc]);
      IPTCom_enableIPTDirEmulation(argv[2 + argc - largc]);
   }

   /* Init the IPTCom object */
   if (largc > 1)
   {
      printf("iptcom_daemon reading IPTCom config file %s\n",argv[1 + argc - largc]);
      res = IPTCom_prepareInit(0, argv[1 + argc - largc]);
   }
   else
   {
      res = IPTCom_prepareInit(0, NULL);
   }

   if (res != (int)IPT_OK)
   {
      if (res != (int)IPT_TAB_ERR_EXISTS)
      {
         printf("IPTCom_prepareInit Failed res =%#x\n",res);
         return(-1);
      }
      else
      {
         printf("IPTCom_prepareInit returned double defined configuration exist\n");
      }
   }

   /* Wait forever */
   (void)IPTVosGetSem(&dummySemId, IPT_WAIT_FOREVER);

   printf("iptcom_daemon exit\n");
   IPTVosThreadTerminate();
   IPTCom_terminate(0);
   IPTCom_destroy();  
   /* Destroy a semaphore  */
   IPTVosDestroySem(&dummySemId);
  
  exit(EXIT_SUCCESS);
}
