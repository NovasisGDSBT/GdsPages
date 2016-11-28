/*
 *  $Id: tdcConfig.c 11861 2012-04-19 11:30:50Z gweiss $
 *
 *  DESCRIPTION    TDC Configuration
 *
 *  AUTHOR         Manfred Ritz
 *
 *  REMARKS        Either the switch VXWORKS, INTEGRITY, LINUX or WIN32 has to be set
 *
 *  DEPENDENCIES
 *
 *  MODIFICATIONS (log starts 2010-08-11)
 *
 *  CR-3477 (Gerhard Weiss, 2012-04-18)
 *          Adding corrections for TÜV-Assessment
 *
 *  CR-382  (Bernd Loehr, 2010-08-24)
 *          Initialization for standalone mode (tdcGetCstStaFilename...) 
 *
 *  All rights reserved. Reproduction, modification, use or disclosure
 *  to third parties without express authority is forbidden.
 *  Copyright Bombardier Transportation GmbH, Germany, 2002-2012.
 *
 */


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

#include "tdc.h"
#include "tdcConfig.h"
#include "tdcIptCom.h"
#include "tdcXML2bin.h"

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
 
#define CR                    "\r"
#define LF                    "\n"
#define COMMENT               "#"
#define ASSIGNMENT            "="
#define SPACE                 " "
#define TAB_CHAR              '\t'
#define SPACE_CHAR            ' '
#define END_CHAR              '\0'
#define ASSIGNMENT_CHAR       '='

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

const char                grpPrefix[]          = "grp";
const char                groupAll[]           = "grpAll";
const char                allCars[]            = "aCar";
const char                allCsts[]            = "aCst";

const char                localDev[]           = "lDev";
const char                localCar[]           = "lCar";
const char                localCst[]           = "lCst";
const char                localTrn[]           = "lTrain";
const char                anyCar[]             = "anyCar";
const char                cmsAnyCar[]          = "CMS";
const char                iptDirServerAnyCar[] = "iptDirServer";

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

const T_GROUP_ALIAS_MAP          grpNoMap[] =
{
   {"grpAll",       (UINT8) 0},
   {"grp001",       (UINT8) 1},
   {"grp002",       (UINT8) 2},
   {"grp003",       (UINT8) 3},
   {"grp028",       (UINT8) 28},
   {"grpHMI",       (UINT8) 123},
   {"grp1123",      (UINT8) 123},
   {"grpDoor",      (UINT8) 87},
   {"grp087",       (UINT8) 87},
};
const UINT16         grpNoMapSize = (UINT16) TAB_SIZE (grpNoMap);

//-----------------------------------------------------------------------------

const UINT32         cycleTimeIPTDirPD  = TIMER_1_SEC;
const UINT32         retryDelayIPTDirMD = 5;  /* retryDelayIPTDirMD * cycleTimeIPTDirPD */
                                              /* for demanding Configuration data       */

//-----------------------------------------------------------------------------

const UINT32         comIdIPTDirPD        = 100;
const UINT32         schedGrpIPTDirPD     = 2000;

const UINT32         comIdIPTDirIptConfMD = 101;
const UINT32         comIdIPTDirUicConfMD = 102;
const UINT32         comIdIPTDirReqDataMD = 103;

const int            mdMsgQueueSize       = 10;

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// used by IPTCom only

const int            IPTComStartupMode     = 0;
const int            IPTComRamAddr         = 0;
const int            IPTComProcessInterval = 100;

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Debug-Levels for initialization and runtime

char                 tdcDbgLevelRun[DEBUG_LEVEL_STR_LEN] = TDC_DBG_LEVEL_RUN; 

//-----------------------------------------------------------------------------

static T_TDC_BOOL	bCheckMCSupport 					= TRUE;
static T_TDC_BOOL	bStandaloneSupport					= FALSE;	/* CR-382	*/

static char				cstStaFile[TDC_MAX_FILENAME_LEN] = DEFAULT_CSTSTA_FILE;

//-----------------------------------------------------------------------------

#define MAX_LINE_PARAM_CNT             10
typedef struct
{
   UINT32                        maxParams;
   UINT32                        paramCnt;
   char*                         param[MAX_LINE_PARAM_CNT];
} T_LINE_PARAMS;
#define LINE_PARAMS_SIZE(cnt)             ((UINT32) ((2 * sizeof (UINT32)) + (cnt * sizeof (char *))))

// ----------------------------------------------------------------------------

static char* lineItemizer   (char*              pLine,      
                             T_LINE_PARAMS*     pLineParams);
static void  evalConfigLine (T_LINE_PARAMS*     pLineParams);

static void  setDbgLevelInit     (const char*   pSetting);
static void  setDbgLevelRun      (const char*   pSetting);
static void  setEnableLogfile    (const char*   pSetting);
static void  setLogfileMode      (const char*   pSetting);
static void  setLogfileName      (const char*   pSetting);
static void  setIPTDirServerEmul (const char*   pSetting);
static void  setComId100Filename (const char*   pSetting);
static void  setComId101Filename (const char*   pSetting);
static void  setComId102Filename (const char*   pSetting);
static void  setCheatedIpAddr    (const char*   pSetting);
static void  setCheckMCsupport   (const char*   pSetting);
       void  tdcSetStandaloneSupport (const char* pSetting);	/* CR-382	*/
static void  setCstStaFilename   (const char*	pSetting);	/* CR-382	*/

// -----------------------------------------------------------------------------

typedef void T_SETTING_FUNCTION (const char* pSetting);

typedef struct
{
   char                       setParam[30];
   T_SETTING_FUNCTION*        setFunc;
} T_SETTINGS_TAB;

static const T_SETTINGS_TAB            settingTab[] =
{
   {"TDC_DBG_LEVEL_INIT",                 setDbgLevelInit},
   {"TDC_DBG_LEVEL_RUN",                  setDbgLevelRun},
   {"TDC_ENABLE_LOGFILE",                 setEnableLogfile},
   {"TDC_LOGFILE_MODE",                   setLogfileMode},
   {"TDC_LOGFILE_NAME",                   setLogfileName},
   {"TDC_IPTDIR_SERVER_EMULATION",        setIPTDirServerEmul},
   {"TDC_COMID_100_FILENAME",             setComId100Filename},
   {"TDC_COMID_101_FILENAME",             setComId101Filename},
   {"TDC_COMID_102_FILENAME",             setComId102Filename},
   {"TDC_CHEAT_IP_ADDR",                  setCheatedIpAddr},
   {"TDC_CHECK_MC_SUPPORT",               setCheckMCsupport},
   {"TDC_ENABLE_STANDALONE_SUPPORT",      tdcSetStandaloneSupport},	/* CR-382	*/
   {"TDC_CSTSTA_FILENAME",                setCstStaFilename},		/* CR-382	*/
};


// ----------------------------------------------------------------------------

char* tdcFile2Buffer (const char*      pModname, 
                      const char*      pFilename, 
                      UINT32*          pBufSize)
{
   char*    pBuffer   = NULL;
   char     text[200] = "";

   *pBufSize = 0;

   if (pFilename != NULL)
   {
      UINT32         fileSize = tdcFSize (pModname, pFilename);   

      if (fileSize > 0)
      {
         T_FILE*        pFile = tdcFopen (pFilename, "rb");
   
         if (pFile != NULL)
         {
            // allocate more space than needed, in order to allow the calling function
            // to append a termination '\0' in case of a text string

            pBuffer = (char*)tdcAllocMemChk (pModname, (UINT32) (fileSize + 4));

            if (pBuffer != NULL)
            {
               if (((*pBufSize) = tdcFread ((UINT8 *) pBuffer, (UINT32) 1, fileSize, pFile)) == 0)
               {
                  (void) tdcSNPrintf (text, (UINT32) sizeof (text), 
                                      "tdcFile2Buffer: Failed to read File(%s)", pFilename);
                  tdcFreeMem  (pBuffer);
                  pBuffer = NULL;
               }
            }
            else
            {
               (void) tdcSNPrintf (text, (UINT32) sizeof (text), 
                                   "tdcFile2Buffer: Failed to alloc memory for File(%s)", pFilename);
            }

            (void) tdcFclose (pFile);
         }
         else
         {
            (void) tdcSNPrintf (text, (UINT32) sizeof (text), 
                                "tdcFile2Buffer: Failed to open File(%s) for reading", pFilename);
         }
      }
      else
      {
         (void) tdcSNPrintf (text, (UINT32) sizeof (text), 
                             "tdcFile2Buffer: Failed to determine Filesize (%s)", pFilename);
      }
   }
   else
   {
      (void) tdcStrNCpy (text, "tdcFile2Buffer: No Filename specified", (UINT32) sizeof (text)); 
   }

   if (text[0] != '\0')                            // Is there a Warning to issue?
   {
      DEBUG_WARN  (pModname, text);
   }

   return (pBuffer);
}

// ----------------------------------------------------------------------------

static char* lineItemizer (char*              pLine,
                           T_LINE_PARAMS*     pLineParams)
{
   char*    pNext = NULL;

   pLineParams->paramCnt = 0;

   if (pLine != NULL)
   {
      char*          pCR   = tdcStrStr (pLine, CR);
      char*          pLF   = tdcStrStr (pLine, LF);
      char*          pStop = pLine;
      char*          pStart;

      // ----- prepare the current line (and take care of CR/LF variants) 
      // ----- the next line starts immediately after the first CR or LF character

      if (pCR != NULL)                                            // EOL
      {
         if (pLF != NULL)                                         // EOL
         {
            pNext = (char *)  MIN (((UINT32) pCR), ((UINT32) pLF));
         }
         else
         {
            pNext = pCR;
         }
      }
      else
      {
         pNext = pLF;
      }

      if (pNext != NULL)
      {
         *pNext = END_CHAR;
          pNext++;
      }

      // ----- remove comments - comments start with '#' sign

      if ((pStart = tdcStrStr (pLine, COMMENT)) != NULL)                 
      {
         *pStart = END_CHAR;
      }

      // ----- replace tabs by spaces for ease of search

      for (pStart = pLine; (*pStart) != END_CHAR; pStart++)
      {
         if ((*pStart) == TAB_CHAR)
         {
            *pStart = SPACE_CHAR;
         }
      }

      // -----  start itemizing

      for (pStart = pLine; (*pStart) != END_CHAR; pStart = pStop + 1) 
      {
         for (; (*pStart) == SPACE_CHAR; pStart++)    // skip leading white spaces    
         {
         }

         if ((*pStart) == END_CHAR)                   // EOL reached before new param
         {
            break;                                                   
         }

         pLineParams->param[pLineParams->paramCnt] = pStart;
         pLineParams->paramCnt++;

         if ((pStop = tdcStrStr (pStart, SPACE))  ==  NULL)          // search white space or end
         {
            break;                                                   // EOL reached
         }

         *pStop = END_CHAR;                                          // ensure proper termination

         if (pLineParams->paramCnt == pLineParams->maxParams)
         {
            break;                                                   // No more params allowed
         }
      }
   }

   return (pNext);
}

//-----------------------------------------------------------------------------

static void  evalConfigLine (T_LINE_PARAMS*     pLineParams)
{
   char*    pSetting = NULL;
   char*    pParam   = pLineParams->param[0];

   switch (pLineParams->paramCnt)
   {
      case 1:     // must be an assignment of form: a=b
      {
         if ((pSetting = tdcStrStr (pParam, ASSIGNMENT)) != NULL)
         {
            *pSetting = END_CHAR;
            pSetting  = pSetting + 1;  
         }
         break;
      }
      case 2:     // must either be an assignment of form a =b or a= b
      {
         if (pLineParams->param[1][0] == ASSIGNMENT_CHAR)
         {
            pSetting = &pLineParams->param[1][1];              // assignment of form: a =b
         }
         else
         {
            UINT32      paramLen = tdcStrLen (pParam);

            if (pParam[paramLen] == ASSIGNMENT_CHAR)            // assignment of form: a= b
            {
               pParam[paramLen] = END_CHAR;
               pSetting         = pLineParams->param[1];      
            }
         }
         break;
      }
      case 3:     // must be an assignment of form: a = b
      {
         if (tdcStrCmp (pLineParams->param[1], ASSIGNMENT) == 0)
         {
            pSetting = pLineParams->param[2];
         }
         break;
      }
      default:
      {
         break;
      }
   }

   if (pSetting != NULL)
   {
      int      i;

      for (i = 0; i < TAB_SIZE (settingTab); i++)
      {
         if (tdcStrCmp (pParam, settingTab[i].setParam) == 0)
         {
            settingTab[i].setFunc (pSetting);
            break;
         }
      }
   }
}

//-----------------------------------------------------------------------------

static void  setDbgLevelInit (const char*   pSetting)
{
   if (    (tdcStrCmp (pSetting, "INFO")    == 0)
        || (tdcStrCmp (pSetting, "WARNING") == 0)
        || (tdcStrCmp (pSetting, "ERROR")   == 0)
      )
   {
      (void) tdcPrintf (MOD_MAIN " Overwriting initial debug level according to config file\n");
      tdcSetDebugLevel (pSetting, NULL, NULL);
   }
   else
   {
      (void) tdcPrintf (MOD_MAIN " Strange initial debug level detected (%s)\n", pSetting);
   }
}

//-----------------------------------------------------------------------------

static void setDbgLevelRun (const char*   pSetting)
{
   if (    (tdcStrCmp (pSetting, "INFO")    == 0)
        || (tdcStrCmp (pSetting, "WARNING") == 0)
        || (tdcStrCmp (pSetting, "ERROR")   == 0)
      )
   {
      (void) tdcPrintf (MOD_MAIN " Overwriting run debug level according to config file (%s)\n", pSetting);
      (void) tdcStrNCpy (tdcDbgLevelRun, pSetting, (UINT32) sizeof (tdcDbgLevelRun));
   }
   else
   {
      (void) tdcPrintf (MOD_MAIN " Strange run debug level detected (%s)\n", pSetting);
   }
}

//-----------------------------------------------------------------------------

static void setEnableLogfile (const char*   pSetting)
{
   if (    (tdcStrCmp (pSetting, "TRUE") == 0)
        || (tdcStrCmp (pSetting, "FALSE") == 0)
      )
   {
      (void) tdcPrintf (MOD_MAIN " Overwriting enable logging according to config file (%s)\n", pSetting);
      tdcSetEnableLog  (pSetting, NULL, NULL);
   }
   else
   {
      (void) tdcPrintf (MOD_MAIN " Strange value for enable logging detected (%s)\n", pSetting);
   }
}                                   

//-----------------------------------------------------------------------------

static void setLogfileMode (const char*   pSetting)
{
   if (    (tdcStrCmp (pSetting, "OVERWRITE") == 0)
        || (tdcStrCmp (pSetting, "APPEND")    == 0)
      )
   {
      (void) tdcPrintf (MOD_MAIN " Overwriting logfile-mode according to config file (%s)\n", pSetting);
      tdcSetLogfileMode (pSetting, NULL, NULL);
   }
   else
   {
      (void) tdcPrintf (MOD_MAIN " Strange value for logfile-mode detected (%s)\n", pSetting);
   }
}

//-----------------------------------------------------------------------------

static void setLogfileName (const char*   pSetting)
{
   (void) tdcPrintf (MOD_MAIN " Overwriting logfile-name according to config file (%s)\n", pSetting);

   tdcSetLogfileName (pSetting, NULL, NULL);
}

//-----------------------------------------------------------------------------

static void setIPTDirServerEmul (const char*   pSetting)
{
   T_TDC_BOOL     bEmulate  = FALSE;

   if (tdcStrCmp (pSetting, "TRUE") == 0)
   {
      bEmulate = TRUE;
   }
   else
   {
      if (tdcStrCmp (pSetting, "FALSE") == 0)
      {
         bEmulate  = FALSE;
      }
      else
      {
         (void) tdcPrintf (MOD_MAIN " Strange value for IPTDirServer Emulation detected (%s)\n", pSetting);
         return;
      }
   }

   (void) tdcPrintf (MOD_MAIN " Overwriting IPTDirServer Emulation according to config file (%s)\n", pSetting);
   tdcSetIPTDirServerEmul (bEmulate);
}

// CR-382 ---------------------------------------------------------------------

void tdcSetStandaloneSupport (const char*   pSetting)
{

   if (tdcStrCmp (pSetting, "TRUE") == 0)
   {
      bStandaloneSupport = TRUE;
   }
   else
   {
      if (tdcStrCmp (pSetting, "FALSE") == 0)
      {
         bStandaloneSupport  = FALSE;
      }
      else
      {
         (void) tdcPrintf (MOD_MAIN " Strange value for Standalone Support detected (%s)\n", pSetting);
         return;
      }
   }

   (void) tdcPrintf (MOD_MAIN " Overwriting Standalone Support according to config file (%s)\n", pSetting);
}

// CR-382 ---------------------------------------------------------------------

T_TDC_BOOL tdcGetStandaloneSupport (void)
{
   return bStandaloneSupport;
}

// CR-382 ---------------------------------------------------------------------

static void setCstStaFilename (const char*   pSetting)
{
	(void) tdcPrintf (MOD_MAIN " Overwriting default CstSta filename according to config file (%s)\n", pSetting);

	(void) tdcStrNCpy (cstStaFile, pSetting, (UINT32) sizeof (cstStaFile));
}

// CR-382 ---------------------------------------------------------------------

const char* tdcGetCstStaFilename (void)
{
   return cstStaFile;
}

//-----------------------------------------------------------------------------

static void setComId100Filename (const char*   pSetting)
{
   (void) tdcPrintf (MOD_MAIN " Overwriting ComId100Filename according to config file (%s)\n", pSetting);

   tdcSetComId100Filename (pSetting);
}

//-----------------------------------------------------------------------------

static void  setComId101Filename (const char*   pSetting)
{
   (void) tdcPrintf (MOD_MAIN " Overwriting ComId101Filename according to config file (%s)\n", pSetting);

   tdcSetComId101Filename (pSetting);
}

//-----------------------------------------------------------------------------

static void  setComId102Filename (const char*   pSetting)
{
   (void) tdcPrintf (MOD_MAIN " Overwriting ComId102Filename according to config file (%s)\n", pSetting);

   tdcSetComId102Filename (pSetting);
}

//-----------------------------------------------------------------------------

static void setCheatedIpAddr (const char*   pSetting)
{
   T_IPT_IP_ADDR        ipAddr;

   if (tdcDottedQuad2Number (pSetting, &ipAddr))
   {
      (void) tdcPrintf (MOD_MAIN " Overwriting own IP-Addr according to config file (%s)\n", pSetting);
      (void) tdcPrintf (MOD_MAIN " Only for simulation purposes !!!\n");

      setSimuIpAddr (ipAddr);
   }
   else
   {
      (void) tdcPrintf (MOD_MAIN " Strange value for Cheat of own IP-Addr detected (%s)\n", pSetting);
   }
}

//-----------------------------------------------------------------------------

static void setCheckMCsupport (const char*   pSetting)
{
   if (tdcStrCmp (pSetting, "TRUE") == 0)
   {
      bCheckMCSupport = TRUE;
   }
   else
   {
      if (tdcStrCmp (pSetting, "FALSE") == 0)
      {
         bCheckMCSupport = FALSE;
      }
      else
      {
         (void) tdcPrintf (MOD_MAIN " Strange value for checking Multicast support (%s)\n", pSetting);
         return;
      }
   }

   (void) tdcPrintf (MOD_MAIN " Overwriting check for Multicast support according to config file (%s)\n", pSetting);
}

//-----------------------------------------------------------------------------

T_TDC_BOOL tdcGetCheckMCSupport (void)
{
   return (bCheckMCSupport);
}

//-----------------------------------------------------------------------------

void readConfiguration (/*@in@*/ void*      pArgV)
{
   char*          pConfigBuf;
   UINT32         configBufSize    = 0;
   const char     tdcSettingFile[] = TDC_CONFIG_FILENAME;

   TDC_UNUSED (pArgV)

   if ((pConfigBuf = tdcFile2Buffer (MOD_MAIN, tdcSettingFile, &configBufSize)) != NULL)
   {
      char*             pLine     = pConfigBuf;
      char*             pNextLine = NULL;
      T_LINE_PARAMS     lineParams;

      // Ensure that the read buffer is properly terminated. (The buffer-size
      // is large enogh, since tdcFile2Buffer reserved some bytes at the end)
      pConfigBuf [configBufSize] = '\0';

      for (pLine = pConfigBuf, lineParams.maxParams = 3; (pLine != NULL); pLine = pNextLine)
      {
         pNextLine = lineItemizer (pLine, &lineParams);

         if (lineParams.paramCnt > 0)
         {
            evalConfigLine (&lineParams);
         }
      }

      tdcFreeMem (pConfigBuf);
   }
   
}


