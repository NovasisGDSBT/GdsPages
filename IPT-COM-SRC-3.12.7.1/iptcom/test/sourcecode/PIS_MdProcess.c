#include "PIS_Common.h"
#include "PIS_IptCom.h"
#include "PIS_App.h"

#define NORMAL_OPERATION    1
#define FILL_RED            11
#define FILL_GREEN          12
#define FILL_BLUE           13
#define FILL_BLACK          14
#define FILL_WHITE          15
#define FILL_GRAYSCALE11    16
#define FILL_GRAYSCALE12    17
#define LOOP_TEST           18

unsigned int CSystemMode,cmd_valid = 0;
int CDurationCounter=0,CBackOnCounter=0,CNormalStartsCounter=0,CWdogResetsCounter=0;

/****************************************************************/
/****             Graphic Tests Functions on Op                 */
/****************************************************************/
int     test_in_progress = 0;

void do_tests(int graphic_test)
{

//    if (IPTVosGetSem(&semflag, IPT_WAIT_FOREVER) == IPT_OK)
//   {
    if ( graphic_test != NORMAL_OPERATION)
       curr_mode=TEST;
    switch ( graphic_test )
    {
        case NORMAL_OPERATION   : system("/tmp/www/GdsScreenTestWrite STOP;sleep 1;touch /tmp/start_chrome"); test_in_progress = 0; break;
        case FILL_RED           : system("/tmp/www/GdsScreenTestWrite FILL_RED"); break;
        case FILL_GREEN         : system("/tmp/www/GdsScreenTestWrite FILL_GREEN"); break;
        case FILL_BLUE          : system("/tmp/www/GdsScreenTestWrite FILL_BLUE"); break;
        case FILL_BLACK         : system("/tmp/www/GdsScreenTestWrite FILL_BLACK"); break;
        case FILL_WHITE         : system("/tmp/www/GdsScreenTestWrite FILL_WHITE"); break;
        case FILL_GRAYSCALE11   : system("/tmp/www/GdsScreenTestWrite FILL_GRAYSCALE11"); break;
        case FILL_GRAYSCALE12   : system("/tmp/www/GdsScreenTestWrite FILL_GRAYSCALE12"); break;
        case LOOP_TEST          : system("/tmp/www/GdsScreenTestWrite LOOP_TEST"); break;
    }


//      if (IPTVosPutSemR(&semflag) != IPT_OK)
//      {
//         IPTVosPrint0(IPT_ERR, "do_tests: IPTVosPutSem(semflag) ERROR\n");
//      }
//   }

}

void apply_tests(int graphic_test)
{
    MON_PRINTF("%s : test_in_progress = %d\n",__FUNCTION__,test_in_progress);
    if ( test_in_progress == 0 )
    {
        if ( graphic_test == NORMAL_OPERATION )
            return;
        system("export SDL_NOMOUSE=1;/tmp/www/cgi-bin/screentest.sh");
        sleep(2);
        test_in_progress = 1;
        MON_PRINTF("%s : test_in_progress = %d\n",__FUNCTION__,test_in_progress);
        do_tests(graphic_test);
    }
    else
        do_tests(graphic_test);
}
/****************************************************************/
/****               Graphic Tests Functions END                 */
/****************************************************************/

/****************************************************************/
/****                   Op MD commands                          */
/****************************************************************/

void md_Op(CINFDISCtrlOp md_InDataOp,int comId , int srcIpAddr)
{
    MON_PRINTF("%s : comId=%d ip=%d.%d.%d.%d\n",__FUNCTION__, comId,
        (srcIpAddr >> 24) & 0xff,  (srcIpAddr >> 16) & 0xff,
        (srcIpAddr >> 8) & 0xff,  srcIpAddr  & 0xff);
    MON_PRINTF("%s :\nCBacklightCmd=%d \nCShutdownCmd=%d \nCSystemMode=%d\n\n",__FUNCTION__,md_InDataOp.CtrlCommands.CBacklightCmd,
        md_InDataOp.CtrlCommands.CShutdownCmd,md_InDataOp.CtrlCommands.CSystemMode);

    if ( md_InDataOp.CtrlCommands.CBacklightCmd == 1 )
        system ("echo 1 > /sys/class/backlight/backlight_lvds1.29/bl_power");
    else
        system ("echo 0 > /sys/class/backlight/backlight_lvds1.29/bl_power");

    CSystemMode = md_InDataOp.CtrlCommands.CSystemMode;

    switch (CSystemMode)
    {
        case   NORMAL_OPERATION :
            MON_PRINTF("%s : CSystemMode is %d , NORMAL_OPERATION\n",__FUNCTION__,CSystemMode);
            curr_mode=NORMAL;
            cmd_valid = 1;
            break;
        case   FILL_RED :
            MON_PRINTF("%s : CSystemMode is %d , FILL_RED\n",__FUNCTION__,CSystemMode);
            cmd_valid = 1;
            break;
        case   FILL_GREEN :
            MON_PRINTF("%s : CSystemMode is %d , FILL_GREEN\n",__FUNCTION__,CSystemMode);
            cmd_valid = 1;
            break;
        case   FILL_BLUE :
            MON_PRINTF("%s : CSystemMode is %d , FILL_BLUE\n",__FUNCTION__,CSystemMode);
            cmd_valid = 1;
            break;
        case   FILL_BLACK :
            MON_PRINTF("%s : CSystemMode is %d , FILL_BLACK\n",__FUNCTION__,CSystemMode);
            cmd_valid = 1;
            break;
        case   FILL_WHITE :
            MON_PRINTF("%s : CSystemMode is %d , FILL_WHITE\n",__FUNCTION__,CSystemMode);
            cmd_valid = 1;
            break;
        case   FILL_GRAYSCALE11 :
            MON_PRINTF("%s : CSystemMode is %d , FILL_GRAYSCALE11\n",__FUNCTION__,CSystemMode);
            cmd_valid = 1;
            break;
        case   FILL_GRAYSCALE12 :
            MON_PRINTF("%s : CSystemMode is %d , FILL_GRAYSCALE12\n",__FUNCTION__,CSystemMode);
            cmd_valid = 1;
            break;
        case   LOOP_TEST :
            MON_PRINTF("%s : CSystemMode is %d , LOOP_TEST\n",__FUNCTION__,CSystemMode);
            cmd_valid = 1;
            break;
        default :
            MON_PRINTF("%s : CSystemMode is %d , ILLEGAL!\n",__FUNCTION__,CSystemMode);
            cmd_valid = 0;
            break;
    }
    if ( cmd_valid == 1 )
        apply_tests(CSystemMode);
}

/****************************************************************/
/****                   Op MD commands end                      */
/****************************************************************/

/****************************************************************/
/****                   Maint MD commands                       */
/****************************************************************/

void md_Maint(CINFDISCtrlMaint md_InDataMaint,int comId , int srcIpAddr )
{
    MON_PRINTF("%s : comId=%d ip=%d.%d.%d.%d\n",__FUNCTION__, comId,
        (srcIpAddr >> 24) & 0xff,  (srcIpAddr >> 16) & 0xff,
        (srcIpAddr >> 8) & 0xff,  srcIpAddr  & 0xff);
    /*
    MON_PRINTF("%s :\nCReset=%d\nCDurationCounterReset=%d\nCBackOnCounterReset=%d\nCNormalStartsCounterReset=%d \nCResetAllCounters=%d \nCWdogResetsCounterReset=%d\n\n",
        __FUNCTION__,
        md_InDataMaint.Commands.CReset,
        md_InDataMaint.INFDCounterCommands.CDurationCounterReset,md_InDataMaint.INFDCounterCommands.CBackOnCounterReset,
        md_InDataMaint.INFDCounterCommands.CNormalStartsCounterReset,md_InDataMaint.INFDCounterCommands.CResetAllCounters,
        md_InDataMaint.INFDCounterCommands.CWdogResetsCounterReset);
    */
    if ( md_InDataMaint.Commands.CReset == 1 )
    {
        MON_PRINTF("%s : Received RESET Command\n",__FUNCTION__);
        system("sleep 2; reboot");
    }

    if ( md_InDataMaint.INFDCounterCommands.CDurationCounterReset == 1 )
    {
        CDurationCounter = 0;
        system ("echo 0 > /tmp/monitor_on_counter");
        MON_PRINTF("%s : Received Duration Counter Reset Command\n",__FUNCTION__);
    }
    if ( md_InDataMaint.INFDCounterCommands.CBackOnCounterReset == 1 )
    {
        CBackOnCounter = 0;
        MON_PRINTF("%s : Received Back On Counter Reset Command\n",__FUNCTION__);
    }
    if ( md_InDataMaint.INFDCounterCommands.CNormalStartsCounterReset == 1 )
    {
        CNormalStartsCounter = 0;
        MON_PRINTF("%s : Received Normal Starts Counter Reset Command\n",__FUNCTION__);
        system ("echo 0 > /tmp/reboot_counter");
    }
    if ( md_InDataMaint.INFDCounterCommands.CResetAllCounters == 1 )
    {
        CDurationCounter = 0;
        CBackOnCounter = 0;
        CNormalStartsCounter = 0;
        CWdogResetsCounter = 0;
        system ("echo 0 > /tmp/monitor_on_counter");
        system ("echo 0 > /tmp/reboot_counter");
        MON_PRINTF("%s : Received Reset All Counters Command\n",__FUNCTION__);
    }
    if ( md_InDataMaint.INFDCounterCommands.CWdogResetsCounterReset == 1 )
    {
        CWdogResetsCounter = 0;
        MON_PRINTF("%s : Received Wdog Resets Counter Command\n",__FUNCTION__);
    }
}

