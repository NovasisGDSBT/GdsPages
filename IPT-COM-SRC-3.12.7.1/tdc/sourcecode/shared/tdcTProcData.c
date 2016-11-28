/*
 * $Id: tdcTProcData.c 11861 2012-04-19 11:30:50Z gweiss $
 *
 * DESCRIPTION
 *
 * AUTHOR         M.Ritz         PPC/EBT
 *
 * ABSTRACT       This module implements the handling of Proc Data as
 *                it is used by TDC.
 *
 * DEPENDENCIES   Either the switch LINUX or WIN32 has to be set
 *
 * MODIFICATIONS (log starts 2010-08-11)	
 *
 *  CR-3477 (Gerhard Weiss, 2012-04-18)
 *          Adding corrections for TÜV-Assessment
 *
 * Internal       Set the initial value of Ipt and UIC Request Timers to 1,
 *                so that Ipt and UIC requests are sent immediately
 *
 *
 * All rights reserved. Reproduction, modification, use or disclosure
 * to third parties without express authority is forbidden. 
 * Copyright Bombardier Transportation GmbH, Germany, 2002-2010.   
 */
 
/* ---------------------------------------------------------------------------- */

#include "tdc.h"
#include "tdcProcData.h"
#include "tdcMsgData.h"
#include "tdcDB.h"
#include "tdcIptCom.h"
#include "tdcConfig.h"

/* ---------------------------------------------------------------------------- */

#define MD_TIMER_DELAY                 5

/* Initialize counters to 1, so they start immediately */
static UINT32  reqIptMdTimer      = 1;   /* Timer Counter for IPT-MD request */
static UINT32  reqUicMdTimer      = 1;   /* Timer Counter for UIC-MD request */

/* ---------------------------------------------------------------------------- */

static void       consumeProcDataV2  (const T_IPT_IPTDIR_PD*   pIptDirPD);
static void       startIptMdTimer    (void);
static void       startUicMdTimer    (void);
static void       stopIptMdTimer     (void);
static void       stopUicMdTimer     (void);
static void       checkMsgDataUpdate (void);
static void       newIptInaugState   (UINT8                    iptInaugState,
                                      UINT8                    iptTopoCnt);
static void       newIptTopoCnt      (UINT8                    iptInaugState,
                                      UINT8                    iptTopoCnt);
static void       newUicInaugState   (UINT8                    uicInaugState,
                                      UINT8                    uicTopoCnt);
static void       newUicTopoCnt      (UINT8                    uicInaugState,
                                      UINT8                    uicTopoCnt);

/* ---------------------------------------------------------------------------- */

static void startIptMdTimer (void)
{
   if (reqIptMdTimer == 0)       /* Nothing to do, if already running */
   {
      reqIptMdTimer = MD_TIMER_DELAY;
   }
}

/* ---------------------------------------------------------------------------- */

static void startUicMdTimer (void)
{
   if (reqUicMdTimer == 0)       /* Nothing to do, if already running */
   {
      reqUicMdTimer = MD_TIMER_DELAY;
   }
}

/* ---------------------------------------------------------------------------- */

static void stopIptMdTimer (void)
{
   reqIptMdTimer = 0;
}

/* ---------------------------------------------------------------------------- */

static void stopUicMdTimer (void)
{
   reqUicMdTimer = 0;
}

/* ---------------------------------------------------------------------------- */

static void newUicInaugState (UINT8          uicInaugState,
                              UINT8          uicTopoCnt)
{
   switch (uicInaugState)
   {
      case 0:
      {
         dbUicNewInaugState (uicInaugState, uicTopoCnt);
         startUicMdTimer ();
         break;
      }
      case 1:
      {
         dbUicNewInaugState (uicInaugState, uicTopoCnt);
         startUicMdTimer ();
         break;
      }
      case 2:
      {
         dbUicNewInaugState (uicInaugState, uicTopoCnt);
         startUicMdTimer  ();
         break;
      }
      default:
      {
         DEBUG_WARN1 (MOD_PD, "Invalid uicInaugState(%d) received!", uicInaugState);
         break;
      }
   }
}

/* ---------------------------------------------------------------------------- */

static void newIptInaugState (UINT8             iptInaugState,
                              UINT8             iptTopoCnt)
{
   switch (iptInaugState)
   {
      case 0:
      case 1:
      case 2:
      {
         dbIptNewInaugState (iptInaugState, iptTopoCnt);
         startIptMdTimer    ();
         DEBUG_INFO2        (MOD_PD, "New IPT-State: inaugState = %d, topoCnt = %d", iptInaugState, iptTopoCnt);
         break;
      }
      default:
      {
         DEBUG_WARN1 (MOD_PD, "Invalid iptInaugState(%d) received!", iptInaugState);
         break;
      }
   }
}

/* ---------------------------------------------------------------------- */

static void newIptTopoCnt (UINT8       iptInaugState,
                           UINT8       iptTopoCnt)
{
   dbIptNewTopoCnt (iptTopoCnt);

   /* start timer for IPT-MD */

   switch (iptInaugState)
   {
      case 0:
      {
         DEBUG_WARN (MOD_PD, "iptTopoCnt changed while iptInaugState remained 0!");
         break;
      }
      case 1:
      case 2:
      {
         startIptMdTimer ();
         break;
      }
      default:
      {
         DEBUG_WARN1 (MOD_PD, "Invalid iptInaugState(%d) received!", iptInaugState);
         break;
      }
   }
}

/* ---------------------------------------------------------------------- */

static void newUicTopoCnt (UINT8    uicInaugState,
                           UINT8    uicTopoCnt)
{
   TDC_UNUSED (uicInaugState)

   dbUicNewTopoCnt (uicTopoCnt);

   startUicMdTimer ();
}

/* ---------------------------------------------------------------------------- */

static void consumeProcDataV2 (const T_IPT_IPTDIR_PD*   pIptDirPD)
{
   T_DB_TRAIN_STATE        dbTrainState;
   UINT32                  protVer            = tdcN2H32 (pIptDirPD->protVer);
   UINT8                   release            = PROT_GET_RELEASE (protVer);
   T_IPT_IP_ADDR           iptDirServerIpAddr = (T_IPT_IP_ADDR) tdcN2H32 (pIptDirPD->iptDirServerIpAddr);
   UINT16                  iptDirServerHostId = (UINT16) ucAddrGetHostId (iptDirServerIpAddr);
   UINT8                   iptInaugState      = tdcN2H8 (pIptDirPD->iptInaugState);
   UINT8                   iptTopoCnt         = tdcN2H8 (pIptDirPD->iptTopoCnt);
   UINT8                   uicInaugState      = tdcN2H8 (pIptDirPD->uicInaugState);
   UINT8                   uicTopoCnt         = tdcN2H8 (pIptDirPD->uicTopoCnt);

   dbSetIptDirServerHostId (iptDirServerHostId);

   if (release > ((UINT8) 0))
   {
      T_IPT_IP_ADDR     gatewayIpAddr = (T_IPT_IP_ADDR) tdcN2H32 (pIptDirPD->gatewayIpAddr);
      UINT8             tbType        = tdcN2H8 (pIptDirPD->tbType);

      dbSetIptDirGateway (tbType, gatewayIpAddr);
   }

   if (dbGetCurTrainState (&dbTrainState) == DB_OK)
   {
      /* check if something relevant with IP-Train changed */

      if (dbTrainState.iptInaugState != iptInaugState)
      {
         newIptInaugState (iptInaugState, iptTopoCnt);
      }
      else
      {
         if (dbTrainState.iptTopoCnt != iptTopoCnt)
         {
            newIptTopoCnt (iptInaugState, iptTopoCnt);
         }
         else
         {
            stopIptMdTimer ();
         }
      }

      /* check if something relevant with UIC changed */

      if (dbTrainState.uicInaugState != uicInaugState)
      {
         newUicInaugState (uicInaugState, uicTopoCnt);
      }
      else
      {
         if (dbTrainState.uicTopoCnt != uicTopoCnt)
         {
            newUicTopoCnt (uicInaugState, uicTopoCnt);
         }
         else
         {
            stopUicMdTimer ();
         }
      }
   }
}

/* ---------------------------------------------------------------------------- */

static void checkMsgDataUpdate (void)
{
   if (reqIptMdTimer > 0)        /* timer is activated! */
   {
      reqIptMdTimer --;

      if (reqIptMdTimer == 0)    /* timer expired, Request new Message Data Telegrams */
      {
         tdcRequIptMsgData ();

         reqIptMdTimer = MD_TIMER_DELAY;
      }
   }
   else                          /* Apparantly the timer is deactivated! */
   {
   }

   if (reqUicMdTimer > 0)        /* timer is activated! */
   {
      reqUicMdTimer --;

      if (reqUicMdTimer == 0)     /* timer expired, Request new Message Data Telegrams */
      {
         tdcRequUicMsgData ();

         reqUicMdTimer = MD_TIMER_DELAY;
      }
   }
   else                          /* Apparantly the timer is deactivated! */
   {
   }
}

/* ---------------------------------------------------------------------------- */

void tdcProcDataCycle (void)
{
   static int     cnt = 0;
   UINT8          procDataBuf[2 * sizeof (T_IPT_IPTDIR_PD)];      // just to be sure
   T_READ_PD      readPD;

   readPD.pProcData    = procDataBuf;
   readPD.msgLen       = (UINT32) sizeof (procDataBuf);
   readPD.procDataType = RECV_IPT_PROC_DATA;

   if (tdcReadProcData (MOD_PD, &readPD))
   {
      UINT32         protVer = tdcN2H32 (((T_IPT_IPTDIR_PD *) readPD.pProcData)->protVer);

      switch (PROT_GET_VERSION (protVer))
      {
         case 2:
         {
            consumeProcDataV2 ((T_IPT_IPTDIR_PD*)readPD.pProcData);
            break;
         }
         case 0:	/*	BL: correct detection of timed out PD 100 telegram	*/
         {
            DEBUG_WARN (MOD_CYC, "IPTDir PD telegram timed out");
            if (tdcGetStandaloneSupport())			/* CR-382	*/
            {
             	/* Switch local emulation on if we lost the IPTDir server	 */
            	tdcSetIPTDirServerEmul (TRUE);
                DEBUG_WARN (MOD_CYC, "IPTDirServerEmul switched on");
            }
            break;
         }
         default:
         {
            DEBUG_WARN1 (MOD_CYC, "received IPTDir PD telegram with invalid Ver (x%08x)", protVer);
            break;
         }
      }
   }

   checkMsgDataUpdate ();

   if (((++cnt) % 100) == 0)
   {
      DEBUG_INFO1 (MOD_CYC, "tdcProcDataCycle called (%d)", cnt);
   }           /*@ -compdestroy */
}              /*@ =compdestroy */






