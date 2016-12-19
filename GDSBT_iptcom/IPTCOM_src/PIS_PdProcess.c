#include "PIS_Common.h"
#include "PIS_IptCom.h"
#include "PIS_App.h"
#include "PIS_Diag.h"

extern unsigned int CSystemMode;
extern unsigned char ErrorDescription;
extern  int CDurationCounter,CBackOnCounter,CNormalStartsCounter,CWdogResetsCounter;

extern unsigned char FBcklightFault,FTempSensor,FTempORHigh,FTempORLow,FAmbLightSensor;
extern  char    chromium_server[256];

unsigned short ISystemLifeSign=0;

/* VAR DIAG DATA */
int     test_webpage=0; /* If set do not update web page */

unsigned char ccmodTimeoutFault=0;
unsigned char FApiMod=0;
unsigned char FWatchdogApiMod=0;

void pd_CCUProcess(CCCUC_INFDIS  pd_InData)
{
static unsigned char      flag=FALSE;
static unsigned char      flagPostCalling=FALSE;
static unsigned short int prev_lifecounter=0;
static unsigned short int spacetime=0;
static char               sys_URL[256];
static char               URL[256];

    /* Placeholders */
    CDurationCounter++;
    CBackOnCounter++;
    CNormalStartsCounter++;
    CWdogResetsCounter++;
    /* Placeholders ends */

    if(flag==FALSE)
    {
        prev_lifecounter=pd_InData.Lifesign.ICCUCLifeSign;
        flag=TRUE;
    }
    else{

        if((prev_lifecounter==pd_InData.Lifesign.ICCUCLifeSign) || ((pd_InData.Lifesign.ICCUCLifeSign==0)&&(prev_lifecounter!= sizeof(unsigned short int))))
        {
            //ERRORE
            spacetime++;

            if(spacetime>=TIMEOUT)
            {
                ErrorDescription=ErrorDescription|IPMODCCUTIMEOUT;
                ccmodTimeoutFault=1;
                LOG_SYS("ERROR", (char *)__FUNCTION__,"CCUTIMEOUT");
            }
            MON_PRINTF("%s : spacetime :%d ErrorDescription=%x\n",__FUNCTION__,spacetime,ErrorDescription);
            MON_PRINTF("%s : pd_InData.Lifesign.ICCUCLifeSign :%d\n",__FUNCTION__,pd_InData.Lifesign.ICCUCLifeSign);
        }
        else
        {
            /* upload inner lifecounter*/
            prev_lifecounter=pd_InData.Lifesign.ICCUCLifeSign;
            ErrorDescription &= (~IPMODCCUTIMEOUT);
            spacetime=0;
            ccmodTimeoutFault=0;

            if(flagPostCalling==FALSE)
            {
                flagPostCalling=TRUE;
                system("echo 1 > /tmp/www/POST_enable");
            }
        }
    }

    if ( test_webpage == 0)
    {
        if ( strlen(pd_InData.LoadResource2.IURL) != 0)
        {
            sprintf(URL,"CHROMIUM_SERVER=%s",pd_InData.LoadResource2.IURL);
            if ( strcmp(chromium_server,URL) != 0 )
            {
                printf("\n");
                printf("URL             : %s!\n",URL);
                printf("chromium_server : %s!\n",chromium_server);
                sprintf(sys_URL,"echo %s > /tmp/chromium_var",URL);
                system(sys_URL);
                system("rm -rf /tmp/chromium_var_mountpoint");
                system("mkdir /tmp/chromium_var_mountpoint");
                system("mount /dev/mmcblk0p3 /tmp/chromium_var_mountpoint");
                system("cp /tmp/chromium_var /tmp/chromium_var_mountpoint/sysconfig/etc/sysconfig/chromium_var");
                system("cp /tmp/chromium_var /etc/sysconfig/chromium_var");
                system("umount /tmp/chromium_var_mountpoint");
                sprintf(chromium_server,"%s",URL);
                printf("chromium_server is now %s !!!\n",chromium_server);
                system("touch /tmp/start_chrome");
                LOG_SYS("INFO", (char *)__FUNCTION__, "URL CHANGED");
            }
        }
    }
    /*
    MON_PRINTF("%s : Lifesign.ICCUCLifeSign:%d\n",__FUNCTION__,pd_InData.Lifesign.ICCUCLifeSign);
    */
}

int pd_get_value()
{
FILE *fp;
char    p[32];
    fp=fopen("/tmp/ta","r");
    if ( fp !=NULL)
        fscanf(fp,"%s",p);
    fclose(fp);
    return atoi(p);
}

void pd_ReportProcess(BYTE *byte_pd_OutData)
{
    unsigned char BacklightStatus=0;




    CINFDISReport *pd_OutData = (CINFDISReport *)byte_pd_OutData;

    pd_OutData->StatusData.ISystemLifeSign=ISystemLifeSign++;

    system("cat /tmp/backlight_on_counter | sed 's/BACKLIGHT_ON_COUNTER=//g' > /tmp/ta");
    pd_OutData->CntData.ITFTBacklight = pd_get_value();

    system("cat /tmp/monitor_on_counter | sed 's/MONITOR_ON_COUNTER=//g' > /tmp/ta");
    pd_OutData->CntData.ITFTWorkTime  = pd_get_value();

    system("cat /tmp/reboot_counter | sed 's/REBOOT_COUNTER=//g' > /tmp/ta");
    pd_OutData->CntData.ITFTPowerUp   = pd_get_value();

    system("cat /sys/class/backlight/backlight_lvds0.28/brightness > /tmp/ta");
    BacklightStatus=(unsigned char)pd_get_value();
    if(BacklightStatus>=5)
        BacklightStatus=5;

    BacklightStatus=BacklightStatus*20;
    pd_OutData->StatusData.IBacklightStatus = BacklightStatus;

    pd_OutData->StatusData.ISystemMode = curr_mode;
    pd_OutData->StatusData.ITestMode = CSystemMode;
    system("cat /tmp/wdog_counter > /tmp/ta");
    pd_OutData->CntData.ITFTWatchdog  = pd_get_value();

    pd_OutData->DiagData.COMModule.FTimeoutComMod=ccmodTimeoutFault;
    pd_OutData->DiagData.TFTFaults.FBcklightFault=FBcklightFault;
    pd_OutData->DiagData.TFTFaults.FTempSensor=FTempSensor;
    pd_OutData->DiagData.TFTFaults.FTempORHigh=FTempORHigh;
    pd_OutData->DiagData.TFTFaults.FTempORLow=FTempORLow;
    pd_OutData->DiagData.TFTFaults.FAmbLightSensor=FAmbLightSensor;
    system("cat /tmp/api_mod > /tmp/ta");
    FApiMod=pd_get_value();
    pd_OutData->DiagData.ApplicationModule.FApiMod=FApiMod;
    system("cat /tmp/wdog_api_mod > /tmp/ta");
    FWatchdogApiMod=pd_get_value();
    pd_OutData->DiagData.ApplicationModule.FWatchdogApiMod =FWatchdogApiMod;
}


