#include "PIS_Common.h"
#include "PIS_App.h"


static void Application_Core(void);
IPT_SEM   semflag;
unsigned char flag_mode=READ;
unsigned char curr_mode=NORMAL;


int Application(void)
{
    int res=IPT_OK;

 /* Register IPTCom PD-process in the C scheduler */
   IPTVosRegisterCyclicThread(Application_Core,"Application_Core",
                              500,
                              POLICY,
                              APPL1000_PRIO,
                              STACK);

    return res;

}

 /*******************************************************************************
NAME:       Application_Core
ABSTRACT:   application with 500 ms cycle time

RETURNS:    -
*/
static void Application_Core(void)
{
   // MON_PRINTF("Application_Core..........\n");
      switch (curr_mode)
    {
        case   NOTAVAIBLE :
        break;

        case   NORMAL :
        break;
        case   ERROR :
        break;

        case   DEGRADED :
        break;

        case   SHUTDOWN :
        break;

        case   TEST :
        break;

        case   PROGRAMMING :
        break;

        default :
        MON_PRINTF("%s : default , ILLEGAL!\n",__FUNCTION__);
        break;
    }

}
