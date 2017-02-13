#include "PIS_Common.h"
#include "PIS_IptCom.h"
#include "PIS_App.h"
#include <unistd.h>

#define NORMAL_OPERATION    1
#define FILL_RED            11
#define FILL_GREEN          12
#define FILL_BLUE           13
#define FILL_BLACK          14
#define FILL_WHITE          15
#define FILL_GRAYSCALE11    16
#define LOOP_TEST           17
#define DIAG_PAGE           18
#define SQUARE_LEFT         19
#define SQUARE_CENTER       20
#define DEFAULT_PAGE_TEST   21
#define PROJECT_SPECIFIC_PAGE        22

unsigned int CSystemMode;
int CDurationCounter=0,CBackOnCounter=0,CNormalStartsCounter=0,CWdogResetsCounter=0;
extern  unsigned char lvds_ptr[256];
extern  int     test_webpage;
int             backlight_state=1;  /* init as on */
extern  int     ShutDownReceived;

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
        case NORMAL_OPERATION   :
                                    test_webpage = 0;
                                    system("/tmp/www/GdsScreenTestWrite STOP;sleep 1;touch /tmp/start_chrome");
                                    test_in_progress = 0;
                                    break;
        case FILL_RED           : system("/tmp/www/GdsScreenTestWrite FILL_RED"); break;
        case FILL_GREEN         : system("/tmp/www/GdsScreenTestWrite FILL_GREEN"); break;
        case FILL_BLUE          : system("/tmp/www/GdsScreenTestWrite FILL_BLUE"); break;
        case FILL_BLACK         : system("/tmp/www/GdsScreenTestWrite FILL_BLACK"); break;
        case FILL_WHITE         : system("/tmp/www/GdsScreenTestWrite FILL_WHITE"); break;
        case FILL_GRAYSCALE11   : system("/tmp/www/GdsScreenTestWrite FILL_GRAYSCALE11"); break;
        case LOOP_TEST          : system("/tmp/www/GdsScreenTestWrite BTLOOP_TEST"); break;
        case DIAG_PAGE          : system("/tmp/www/GdsScreenTestWrite DIAG_PAGE"); break;
        case SQUARE_LEFT        : system("/tmp/www/GdsScreenTestWrite FILL_SQUARELEFT"); break;
        case SQUARE_CENTER      : system("/tmp/www/GdsScreenTestWrite FILL_SQUARECENTER"); break;
        case PROJECT_SPECIFIC_PAGE       : system("/tmp/www/GdsScreenTestWrite IMAGE_TEST"); break;
        case DEFAULT_PAGE_TEST         :
                                    test_webpage = 1;
                                    system("echo CHROMIUM_SERVER=\"http://127.0.0.1:8080/test_default_page/default_page.html\" > /etc/sysconfig/chromium_var");
                                    system("echo \"http://127.0.0.1:8080/test_default_page/default_page.html\" > /tmp/www/url.txt");
                                    system("/tmp/www/GdsScreenTestWrite STOP;sleep 1;touch /tmp/start_chrome");
                                    break;
    }
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
    char    cmd_string[128];
    char    stringlog[128];
    unsigned int cmd_valid = 0;

    memset(stringlog,0,sizeof(stringlog));

    MON_PRINTF("%s : comId=%d ip=%d.%d.%d.%d\n",__FUNCTION__, comId,
        (srcIpAddr >> 24) & 0xff,  (srcIpAddr >> 16) & 0xff,
        (srcIpAddr >> 8) & 0xff,  srcIpAddr  & 0xff);
    MON_PRINTF("%s :\nCBacklightCmd=%d \nCShutdownCmd=%d \nCSystemMode=%d\n\n",__FUNCTION__,md_InDataOp.CtrlCommands.CBacklightCmd,
        md_InDataOp.CtrlCommands.CShutdownCmd,md_InDataOp.CtrlCommands.CSystemMode);

    if ( md_InDataOp.CtrlCommands.CBacklightCmd == 2 )
    {

        LOG_SYS(DATAREC,INFO, "CINFDISCtrlOp_TASK","CBacklightCmd received value:2");
        sprintf(cmd_string,"echo 1 > %s",BACKLIGHT_CMD);
        system(cmd_string);
        backlight_state = 0;    /* The backlight is off */
    }
    if ( md_InDataOp.CtrlCommands.CBacklightCmd == 1 )
    {
        LOG_SYS(DATAREC,INFO, "CINFDISCtrlOp_TASK","CBacklightCmd received value:1");
        sprintf(cmd_string,"echo 0 > %s",BACKLIGHT_CMD);
        system(cmd_string);
        backlight_state = 1;    /* The backlight is on */
    }

    if ( md_InDataOp.CtrlCommands.CShutdownCmd == 1 )
    {
        LOG_SYS(DATAREC,INFO, "CINFDISCtrlOp_TASK","CShutdownCmd received");
        system("touch /tmp/system_has_been_shutted_down");
        MON_PRINTF("%s : Poweroff\n",__FUNCTION__);
        ShutDownReceived = 1;
        system("echo 1 > /sys/class/graphics/fb0/blank");
        system("kill -9 `pidof fluxbox` ; sleep 5");
        system("poweroff");
    }

    CSystemMode = md_InDataOp.CtrlCommands.CSystemMode;

    if(CSystemMode > 0)
    {
        snprintf(stringlog,sizeof(stringlog)-1,"CSystemMode receivedValue:%d",CSystemMode);
        LOG_SYS(DATAREC,INFO, "CINFDISCtrlOp_TASK",stringlog);
    }

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
        case   LOOP_TEST :
            MON_PRINTF("%s : CSystemMode is %d , LOOP_TEST\n",__FUNCTION__,CSystemMode);
            cmd_valid = 1;
            break;
        case   DIAG_PAGE :
            MON_PRINTF("%s : CSystemMode is %d , DIAG_PAGE\n",__FUNCTION__,CSystemMode);
            cmd_valid = 1;
            break;
#define SQUARE_LEFT         19
#define SQUARE_CENTER       20
#define DEFAULT_PAGE_TEST          21
#define PROJECT_SPECIFIC_PAGE        22
        case   SQUARE_LEFT :
            MON_PRINTF("%s : CSystemMode is %d , SQUARE_LEFT\n",__FUNCTION__,CSystemMode);
            cmd_valid = 1;
            break;
        case   SQUARE_CENTER :
            MON_PRINTF("%s : CSystemMode is %d , SQUARE_CENTER\n",__FUNCTION__,CSystemMode);
            cmd_valid = 1;
            break;
        case   DEFAULT_PAGE_TEST :
            MON_PRINTF("%s : CSystemMode is %d , DEFAULT_PAGE_TEST\n",__FUNCTION__,CSystemMode);
            cmd_valid = 1;
            break;
        case   PROJECT_SPECIFIC_PAGE :
            MON_PRINTF("%s : CSystemMode is %d , PROJECT_SPECIFIC_PAGE\n",__FUNCTION__,CSystemMode);
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
        LOG_SYS(DATAREC,INFO, "CINFDISCtrlMaint_Task","CReset received");
        MON_PRINTF("%s : Received RESET Command\n",__FUNCTION__);
        system("kill -9 `pidof chrome_starter.sh`  >/dev/null 2>&1");
        system("kill -9 `pidof backlight_counter.sh`  >/dev/null 2>&1");
        system("kill -9 `pidof monitor_counter.sh`  >/dev/null 2>&1");
        system("kill -9 `pidof ntp_hwclock_update.sh`  >/dev/null 2>&1");
        system("kill -9 `pidof auto_backlight_bkg` >/dev/null 2>&1");
        system("rm -rf /tmp/store_mountpoint");
        system("mkdir /tmp/store_mountpoint");
        system("mount /dev/mmcblk0p3 /tmp/store_mountpoint");
        system("cp /tmp/chromium_var /tmp/store_mountpoint/sysconfig/etc/sysconfig/chromium_var");
        system("cp /tmp/chromium_var /etc/sysconfig/chromium_var");
        system("umount /tmp/store_mountpoint");
        system("mkdir -p /tmp/log_mountpoint");
        system("mount /dev/mmcblk0p2 /tmp/log_mountpoint");
        system("cp /tmp/www/check_xml_fileSizeD.xml /tmp/log_mountpoint/");
        system("umount /tmp/log_mountpoint");
        system("sync");
        system("sleep 2; reboot");
    }

    if ( md_InDataMaint.INFDCounterCommands.CDurationCounterReset == 1 )
    {
        LOG_SYS(DATAREC,INFO, "CINFDISCtrlMaint_Task","CDurationCounterReset received");
        CDurationCounter = 0;
        system ("touch /tmp/monitor_on_reset");
        MON_PRINTF("%s : Received Duration Counter Reset Command\n",__FUNCTION__);
    }
    if ( md_InDataMaint.INFDCounterCommands.CBackOnCounterReset == 1 )
    {
        LOG_SYS(DATAREC,INFO, "CINFDISCtrlMaint_Task","CDurationCounterReset received");

        CBackOnCounter = 0;
        system ("touch /tmp/backlight_on_reset");
        MON_PRINTF("%s : Received Back On Counter Reset Command\n",__FUNCTION__);
    }
    if ( md_InDataMaint.INFDCounterCommands.CNormalStartsCounterReset == 1 )
    {
        LOG_SYS(DATAREC,INFO,"CINFDISCtrlMaint_Task","CNormalStartsCounterReset received");
        CNormalStartsCounter = 0;
        MON_PRINTF("%s : Received Normal Starts Counter Reset Command\n",__FUNCTION__);
        system("/tmp/www/store_all_counters");
        system ("echo REBOOT_COUNTER=0 > /tmp/reboot_counter");
    }
    if ( md_InDataMaint.INFDCounterCommands.CResetAllCounters == 1 )
    {
        LOG_SYS(DATAREC,INFO,"CINFDISCtrlMaint_Task","CResetAllCounters received");
        CDurationCounter = 0;
        CBackOnCounter = 0;
        CNormalStartsCounter = 0;
        CWdogResetsCounter = 0;
        system ("touch /tmp/monitor_on_reset");
        system ("touch /tmp/backlight_on_reset");
        system ("echo REBOOT_COUNTER=0 > /tmp/reboot_counter");
        system ("echo WATCHDOG_COUNTER=0 > /tmp/wdog_counter");
        system ("echo 0 > /tmp/wdog_api_counter");
        system("/tmp/www/store_all_counters");
        MON_PRINTF("%s : Received Reset All Counters Command\n",__FUNCTION__);
    }
    if ( md_InDataMaint.INFDCounterCommands.CWdogResetsCounterReset == 1 )
    {
        LOG_SYS(DATAREC,INFO,"CINFDISCtrlMaint_Task","CWdogResetsCounterReset received");
        CWdogResetsCounter = 0;
        system ("echo 0 > /tmp/wdog_counter");
        system("/tmp/www/store_all_counters");
        MON_PRINTF("%s : Received Wdog Resets Counter Command\n",__FUNCTION__);
    }
}

