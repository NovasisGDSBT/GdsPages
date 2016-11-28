/*                                                                           */
/* $Id: tdcConfig.h 11669 2010-08-30 10:20:10Z bloehr $                       */
/*                                                                           */
/* DESCRIPTION    TDC Configuration                                          */
/*                                                                           */
/* AUTHOR         M.Ritz         PPC/EBTS                                    */
/*                                                                           */
/* REMARKS                                                                   */
/*                                                                           */
/* DEPENDENCIES                                                              */
/*                                                                     		 */
/*  MODIFICATIONS:                      							         */
/*   																         */
/*  CR-382 (Bernd Loehr, 2010-08-24)   								         */
/* 			Initialization for standalone mode (tdcGetCstStaFilename)        */
/*                                                                           */
/*                                                                           */
/* All rights reserved. Reproduction, modification, use or disclosure        */
/* to third parties without express authority is forbidden.                  */
/* Copyright Bombardier Transportation GmbH, Germany, 2002.                  */
/*                                                                           */

/* ---------------------------------------------------------------------------- */

#if !defined (TDC_CONFIG_H)
   #define TDC_CONFIG_H

/* ---------------------------------------------------------------------------- */

#if defined (__cplusplus)
extern "C" {
#endif

/* ---------------------------------------------------------------------------- */

#define GROUP_PREF_LEN           3

extern const char                grpPrefix[];         
extern const char                groupAll[];          
extern const char                allCars[];           
extern const char                allCsts[];           
                                              
extern const char                localDev[];          
extern const char                localCar[];          
extern const char                localCst[];          
extern const char                localTrn[];          
extern const char                anyCar[];            
extern const char                cmsAnyCar[];         
extern const char                iptDirServerAnyCar[];


/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
/* Mapping for group-numbers and group-aliases to group-numbers
*/
typedef struct
{
   T_IPT_LABEL          grpName;
   UINT8                grpNo;
} T_GROUP_ALIAS_MAP;

extern const T_GROUP_ALIAS_MAP         grpNoMap[];
extern const UINT16                    grpNoMapSize;

/* ---------------------------------------------------------------------------- */

extern const UINT32     cycleTimeIPTDirPD;
/*@ -exportlocal */
extern const UINT32     retryDelayIPTDirMD;     /* retryDelayIPTDirMD * cycleTimeIPTDirPD */
/*@ =exportlocal */                             /* for demanding Configuration data       */

/* ---------------------------------------------------------------------------- */
/*@ -exportlocal */
extern const UINT32     comIdIPTDirPD;
extern const UINT32     schedGrpIPTDirPD;
extern const UINT32     comIdIPTDirIptConfMD;
extern const UINT32     comIdIPTDirUicConfMD;
extern const UINT32     comIdIPTDirReqDataMD;

extern const int        mdMsgQueueSize;
/*@ =exportlocal */

/* ---------------------------------------------------------------------------- */
/*@ -exportlocal */
extern const int        IPTComStartupMode;
extern const int        IPTComRamAddr;
extern const int        IPTComProcessInterval;
/*@ =exportlocal */

/* ---------------------------------------------------------------------------- */

#define DEBUG_LEVEL_STR_LEN         10
#define TDC_DBG_LEVEL_INIT          "ERROR"
#define TDC_DBG_LEVEL_RUN           "ERROR"
extern char                         tdcDbgLevelRun[DEBUG_LEVEL_STR_LEN];

/* ---------------------------------------------------------------------------- */

#define TDC_CONFIG_FILENAME         "tdc.settings"
#define DEFAULT_CSTSTA_FILE         "cstSta.xml"	/* CR-382	*/

extern void       readConfiguration  (/*@in@*/ void*      pArgV);
extern char*      tdcFile2Buffer     (const char*   pModname, 
                                      const char*   pFilename, 
                                      UINT32*       pBufSize);

/* ---------------------------------------------------------------------------- */

extern T_TDC_BOOL tdcGetCheckMCSupport (void);
extern T_TDC_BOOL tdcGetStandaloneSupport (void);
extern void tdcSetStandaloneSupport (const char* pSetting);
extern const char* tdcGetCstStaFilename(void);

/* ---------------------------------------------------------------------------- */

#if defined (__cplusplus)
}
#endif

#endif
