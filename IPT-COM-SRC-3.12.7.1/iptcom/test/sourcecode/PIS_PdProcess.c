#include "PIS_Common.h"
#include "PIS_IptCom.h"
#include "PIS_App.h"
#include "PIS_Diag.h"

unsigned char URL[256];
extern unsigned int CSystemMode;
extern unsigned char ErrorDescription;
extern  int CDurationCounter,CBackOnCounter,CNormalStartsCounter,CWdogResetsCounter;

extern unsigned char FBcklightFault,FTempSensor,FTempORHigh,FTempORLow,FAmbLightSensor;

void pd_CCUProcess(CCCUC_INFDIS  pd_InData)
{

static unsigned char flag=FALSE;
static unsigned short int prev_lifecounter=0;
static unsigned short int spacetime=0;

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
        }

    }
    /*
    MON_PRINTF("%s : Lifesign.ICCUCLifeSign:%d\n",__FUNCTION__,pd_InData.Lifesign.ICCUCLifeSign);
    MON_PRINTF("%s : LoadResource2.IURL:%s\n",__FUNCTION__,pd_InData.LoadResource2.IURL);
    sprintf(URL,"%s",pd_InData.LoadResource2.IURL);
    MON_PRINTF("%s : URL is %s\n",__FUNCTION__,URL);
    */
}

int get_value()
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
    CINFDISReport *pd_OutData = (CINFDISReport *)byte_pd_OutData;

    system("cat /tmp/backlight_on_counter | sed 's/BACKLIGHT_ON_COUNTER=//g' > /tmp/ta");
    pd_OutData->CntData.ITFTBacklight = get_value();

    system("cat /proc/uptime | sed 's/\\./ /g' | awk '{ print $1}' > /tmp/ta");
    pd_OutData->CntData.ITFTWorkTime  = get_value();

    system("cat /tmp/reboot_counter | sed 's/REBOOT_COUNTER=//g' > /tmp/ta");
    pd_OutData->CntData.ITFTPowerUp   = get_value();

    system("cat /sys/class/backlight/backlight_lvds1.29/brightness > /tmp/ta");
    pd_OutData->StatusData.IBacklightStatus = get_value();

    pd_OutData->StatusData.ISystemMode = curr_mode;
    pd_OutData->StatusData.ITestMode = CSystemMode;
    pd_OutData->CntData.ITFTWatchdog  = 1;

    pd_OutData->DiagData.TFTFaults.FBcklightFault=FBcklightFault;
    pd_OutData->DiagData.TFTFaults.FTempSensor=FTempSensor;
    pd_OutData->DiagData.TFTFaults.FTempORHigh=FTempORHigh;
    pd_OutData->DiagData.TFTFaults.FTempORLow=FTempORLow;
    pd_OutData->DiagData.TFTFaults.FAmbLightSensor=FAmbLightSensor;

    unsigned char FTempORLow=0;

}


