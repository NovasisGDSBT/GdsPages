/*                                                                               */
/*  $Id:: tdc.h 33411 2014-07-04 06:23:01Z gweiss                $               */
/*                                                                               */
/*  DESCRIPTION    Definitions, Prototypes ... for the Main Task/Thread of TDC   */
/*                                                                               */
/*  AUTHOR         M.Ritz         PPC/EBT                                        */
/*                                                                               */
/*  REMARKS        Includes all standard TDC - Headers                           */
/*                                                                               */
/*  DEPENDENCIES   Basic Datatypes have to be defined in advance if LINUX        */
/*                 is chosen.                                                    */
/*                                                                               */
/*                                                                               */
/*  All rights reserved. Reproduction, modification, use or disclosure           */
/*  to third parties without express authority is forbidden.                     */
/*  Copyright Bombardier Transportation GmbH, Germany, 2002.                     */
/*                                                                               */

/* ---------------------------------------------------------------------------- */

#if !defined (_TDC_H)
   #define _TDC_H

/* ---------------------------------------------------------------------------- */

#include "iptDef.h"

#include "tdcSyl.h"
#include "tdcApi.h"

#include "tdcDbgUtil.h"
#include "tdcSema.h"
#include "tdcSock.h"
#include "tdcMem.h"
#include "tdcString.h"
#include "tdcTimer.h"
#include "tdcMisc.h"

/* ---------------------------------------------------------------------------- */

#ifndef TDC_VERSION     
#define TDC_VERSION     0
#endif

#ifndef TDC_RELEASE     
#define TDC_RELEASE     0
#endif

#ifndef TDC_UPDATE      
#define TDC_UPDATE      0
#endif

#ifndef TDC_EVOLUTION   
#define TDC_EVOLUTION   0
#endif

/* ---------------------------------------------------------------------------- */

#define ALIGN_4_MASK			0xFFFFFFFC
#define ALIGN_2_MASK			0xFFFFFFFE

#define ALIGN_4(x)                      ((((UINT32) (x)) + 3) & ALIGN_4_MASK)
#define ALIGN_2(x)                      ((((UINT32) (x)) + 1) & ALIGN_2_MASK)
/* ---------------------------------------------------------------------------- */

#define MAKE_PROT_VERSION(ver, rel, upd, evo)      ((ver << 24) + (rel << 16) + (upd << 8) + evo)
#define PROT_GET_VERSION(prot)                     ((UINT8) ((prot >> 24) & 0xFF))
#define PROT_GET_RELEASE(prot)                     ((UINT8) ((prot >> 16) & 0xFF))
#define PROT_GET_UPDATE(prot)                      ((UINT8) ((prot >> 8)  & 0xFF))
#define PROT_GET_EVOLUTION(prot)                   ((UINT8) ((prot >> 0)  & 0xFF))

/* ---------------------------------------------------------------------------- */

#define MOD_MAIN                    "TDC:"
#define MOD_ITIMER                  "TDCI:"
#define MOD_CYC                     "TDC_CYC:"
#define MOD_IPC                     "TDC_IPC:"
#define MOD_MD                      "TDC_MD:"
#define MOD_PD                      "TDC_PD:"
#define MOD_DB                      "TDC_DB:"
#define MOD_LIB                     "TDC_LIB:"

/* ---------------------------------------------------------------------------- */

#undef IPT_LABEL_SIZE
#undef IPT_URI_SIZE
#define IPT_LABEL_SIZE              ((UINT32) sizeof (T_IPT_LABEL))
#define IPT_URI_SIZE                ((UINT32) sizeof (T_IPT_URI))

/* ---------------------------------------------------------------------------- */

#define TIMER_RESOLUTION            ((UINT32) 500)                         /* msec */

#define TIMER_500_MSEC              ((UINT32) TIMER_RESOLUTION)
#define TIMER_1_SEC                 ((UINT32) (TIMER_500_MSEC * 2))
#define TIMER_1500_MSEC             ((UINT32) (TIMER_500_MSEC * 3))
#define TIMER_2_SEC                 ((UINT32) (TIMER_500_MSEC * 4))
#define TIMER_3_SEC                 ((UINT32) (TIMER_500_MSEC * 6))
#define TIMER_4_SEC                 ((UINT32) (TIMER_500_MSEC * 8))
#define TIMER_5_SEC                 ((UINT32) (TIMER_500_MSEC * 10))
#define TIMER_6_SEC                 ((UINT32) (TIMER_500_MSEC * 12))
#define TIMER_7_SEC                 ((UINT32) (TIMER_500_MSEC * 14))
#define TIMER_8_SEC                 ((UINT32) (TIMER_500_MSEC * 16))
#define TIMER_9_SEC                 ((UINT32) (TIMER_500_MSEC * 18))
#define TIMER_10_SEC                ((UINT32) (TIMER_500_MSEC * 20))
#define TIMER_15_SEC                ((UINT32) (TIMER_500_MSEC * 30))
#define TIMER_20_SEC                ((UINT32) (TIMER_500_MSEC * 40))
#define TIMER_25_SEC                ((UINT32) (TIMER_500_MSEC * 50))
#define TIMER_30_SEC                ((UINT32) (TIMER_500_MSEC * 60))
#define TIMER_40_SEC                ((UINT32) (TIMER_500_MSEC * 80))
#define TIMER_50_SEC                ((UINT32) (TIMER_500_MSEC * 100))
#define TIMER_60_SEC                ((UINT32) (TIMER_500_MSEC * 120))

/* ---------------------------------------------------------------------------- */

#define TDC_MSG_TIMEOUT            ((UINT16) 0)
#define TDC_MSG_IDC_RECV           ((UINT16) 1)
#define TDC_MSG_IDC_SEND           ((UINT16) 2)
#define TDC_MSG_CALL_REQ_V1        ((UINT16) 3)

/* ---------------------------------------------------------------------------- */

#define TDC_UNUSED(x)               (void) x;

/* ---------------------------------------------------------------------------- */

#ifdef __cplusplus                            /* to be compatible with C++ */
   extern "C" {
#endif

/* ---------------------------------------------------------------------------- */

/* Ports 0     ... 1023       --> well known ports (su-rights)    */
/* Ports 1024  ... 49151      --> IANA reserved ports             */
/* Ports 49152 ... 65535      --> dynamic or private (ephemeral)  */

#define TDC_IPC_SERVER_PORT     ((UINT16) 57264)          /* hopefully unique  */

/* --------------------   TDC Main Thread   ---------------------------------- */


extern T_TDC_BOOL       tdcTerminateIpcServ       (void);
extern T_TDC_BOOL       tdcTerminateTMsgData      (void);
extern T_TDC_BOOL       tdcTerminateTCyclic       (void);

extern T_IPT_IP_ADDR    tdcGetLocalIpAddr         (void);

extern void             tdcTCyclic                (/*@in@*/ /*@null@*/void*          pArgV);
extern void             tdcTIpcServ               (/*@in@*/ void*          pArgV);
extern void             tdcTMsgData               (/*@in@*/ void*          pArgV);

extern void             tdcSetLogfileName         (const char*    pPar0,
                                                   const char*    pPar1,
                                                   const char*    pPar2);
extern void             tdcSetLogfileOpenMode     (const char*    pPar0,
                                                   const char*    pPar1,
                                                   const char*    pPar2);

extern T_TDC_BOOL       tdcDetermineOwnDevice     (const char*       pModName);
extern T_TDC_BOOL       tdcDottedQuad2Number      (const char*       pDottedQuad,
                                                   T_IPT_IP_ADDR*    pIpAddr);
extern void             setSimuIpAddr             (T_IPT_IP_ADDR     ipAddr);

/* ---------------------------------------------------------------------------- */

#ifdef __cplusplus                            /* to be compatible with C++ */
   }
#endif


#endif



