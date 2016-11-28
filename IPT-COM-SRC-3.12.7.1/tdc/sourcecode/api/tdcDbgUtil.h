/*  $Id: tdcDbgUtil.h 29150 2013-09-18 12:42:48Z bloehr $                   */
/*                                                                            */
/*  DESCRIPTION    Front-End for some Debug Utilities                         */
/*                                                                            */
/*  AUTHOR         M.Ritz         PPC/EBT                                     */
/*                                                                            */
/*  REMARKS        Either the switch VXWORKS, LINUX or WIN32 has to be set    */
/*                                                                            */
/*  DEPENDENCIES                                                              */
/*                                                                            */
/*                                                                            */
/*  All rights reserved. Reproduction, modification, use or disclosure        */
/*  to third parties without express authority is forbidden.                  */
/*  Copyright Bombardier Transportation GmbH, Germany, 2002.                  */

/*  ----------------------------------------------------------------------------  */
/*  ----------------------------------------------------------------------------  */
/*  ----------------------------------------------------------------------------  */

#if !defined (_TDC_DBG_UTIL_H)
   #define _TDC_DBG_UTIL_H

/*  ----------------------------------------------------------------------------  */

#define DEBUG_CONTINUE        0
#define DEBUG_HALT            1
#define DEBUG_PRINT           2


#define DBG_LEVEL_INFO        2
#define DBG_LEVEL_WARN        1
#define DBG_LEVEL_ERROR       0



#define DBG_ERROR             (char *) __FILE__, (UINT16) __LINE__, (INT16) DEBUG_HALT
#define DBG_WARN              (char *) __FILE__, (UINT16) __LINE__, (INT16) DEBUG_CONTINUE
#define DBG_INFO              (char *) __FILE__, (UINT16) __LINE__, (INT16) DEBUG_PRINT

/*
#undef  DBG_WARN
#define DBG_WARN     DBG_INFO
*/

#define LEFT_MARGIN           "       "
#define SIZE_LEFT_MARGIN      7

/*  ----------------------------------------------------------------------------- */

#ifdef __cplusplus                            /* to be compatible with C++ */
   extern "C" {
#endif

/*  ----------------------------------------------------------------------------- */

#if defined (LINUX) || defined(DARWIN)

   extern int sscanf (const char*, const char*, ...);

   #define     tdcSScanf                  sscanf

   extern int  tdcPrintf   (const char*      pFormat, ...);
   extern void tdcSNPrintf (char*            pBuf,
                            unsigned int     bufSize,
                            const char*      pFormat, ...);
   extern void tdcDebug    (const char*      pFilename,
                                  UINT16     line,
                                  INT16      strategy,
                            const char*      pModname,
                            const char*      pFormat, ... );

   #define DEBUG_INFO(x, y)                        tdcDebug (DBG_INFO,  (x), (y))
   #define DEBUG_INFO1(x, y, z1)                   tdcDebug (DBG_INFO,  (x), (y), (int) (z1))
   #define DEBUG_INFO2(x, y, z1, z2)               tdcDebug (DBG_INFO,  (x), (y), (int) (z1), (int) (z2))
   #define DEBUG_INFO3(x, y, z1, z2, z3)           tdcDebug (DBG_INFO,  (x), (y), (int) (z1), (int) (z2), (int) (z3))
   #define DEBUG_INFO4(x, y, z1, z2, z3, z4)       tdcDebug (DBG_INFO,  (x), (y), (int) (z1), (int) (z2), (int) (z3) , (int) (z4))

   #define DEBUG_WARN(x, y)                        tdcDebug (DBG_WARN,  (x), (y))
   #define DEBUG_WARN1(x, y, z1)                   tdcDebug (DBG_WARN,  (x), (y), (int) (z1))
   #define DEBUG_WARN2(x, y, z1, z2)               tdcDebug (DBG_WARN,  (x), (y), (int) (z1), (int) (z2))
   #define DEBUG_WARN3(x, y, z1, z2, z3)           tdcDebug (DBG_WARN,  (x), (y), (int) (z1), (int) (z2), (int) (z3))
   #define DEBUG_WARN4(x, y, z1, z2, z3, z4)       tdcDebug (DBG_WARN,  (x), (y), (int) (z1), (int) (z2), (int) (z3) , (int) (z4))

   #define DEBUG_ERROR(x, y)                       tdcDebug (DBG_ERROR, (x), (y))
   #define DEBUG_ERROR1(x, y, z1)                  tdcDebug (DBG_ERROR, (x), (y), (int) (z1))
   #define DEBUG_ERROR2(x, y, z1, z2)              tdcDebug (DBG_ERROR, (x), (y), (int) (z1), (int) (z2))
   #define DEBUG_ERROR3(x, y, z1, z2, z3)          tdcDebug (DBG_ERROR, (x), (y), (int) (z1), (int) (z2), (int) (z3))
   #define DEBUG_ERROR4(x, y, z1, z2, z3, z4)      tdcDebug (DBG_ERROR, (x), (y), (int) (z1), (int) (z2), (int) (z3) , (int) (z4))
#endif

/* --------------------------------------------------------------------------------- */
#if defined(__INTEGRITY)

   extern int sscanf (const char*, const char*, ...);

   #define     tdcSScanf                  sscanf

   extern int  tdcPrintf   (const char*      pFormat, ...);
   extern void tdcSNPrintf (char*            pBuf,
                            unsigned int     bufSize,
                            const char*      pFormat, ...);
   extern void tdcDebug    (const char*      pFilename,
                                  UINT16     line,
                                  INT16      strategy,
                            const char*      pModname,
                            const char*      pFormat, ... );

   #define DEBUG_INFO(x, y)                        tdcDebug (DBG_INFO,  (x), (y))
   #define DEBUG_INFO1(x, y, z1)                   tdcDebug (DBG_INFO,  (x), (y), (int) (z1))
   #define DEBUG_INFO2(x, y, z1, z2)               tdcDebug (DBG_INFO,  (x), (y), (int) (z1), (int) (z2))
   #define DEBUG_INFO3(x, y, z1, z2, z3)           tdcDebug (DBG_INFO,  (x), (y), (int) (z1), (int) (z2), (int) (z3))
   #define DEBUG_INFO4(x, y, z1, z2, z3, z4)       tdcDebug (DBG_INFO,  (x), (y), (int) (z1), (int) (z2), (int) (z3) , (int) (z4))

   #define DEBUG_WARN(x, y)                        tdcDebug (DBG_WARN,  (x), (y))
   #define DEBUG_WARN1(x, y, z1)                   tdcDebug (DBG_WARN,  (x), (y), (int) (z1))
   #define DEBUG_WARN2(x, y, z1, z2)               tdcDebug (DBG_WARN,  (x), (y), (int) (z1), (int) (z2))
   #define DEBUG_WARN3(x, y, z1, z2, z3)           tdcDebug (DBG_WARN,  (x), (y), (int) (z1), (int) (z2), (int) (z3))
   #define DEBUG_WARN4(x, y, z1, z2, z3, z4)       tdcDebug (DBG_WARN,  (x), (y), (int) (z1), (int) (z2), (int) (z3) , (int) (z4))

   #define DEBUG_ERROR(x, y)                       tdcDebug (DBG_ERROR, (x), (y))
   #define DEBUG_ERROR1(x, y, z1)                  tdcDebug (DBG_ERROR, (x), (y), (int) (z1))
   #define DEBUG_ERROR2(x, y, z1, z2)              tdcDebug (DBG_ERROR, (x), (y), (int) (z1), (int) (z2))
   #define DEBUG_ERROR3(x, y, z1, z2, z3)          tdcDebug (DBG_ERROR, (x), (y), (int) (z1), (int) (z2), (int) (z3))
   #define DEBUG_ERROR4(x, y, z1, z2, z3, z4)      tdcDebug (DBG_ERROR, (x), (y), (int) (z1), (int) (z2), (int) (z3) , (int) (z4))
#endif

/* --------------------------------------------------------------------------------- */

#if defined (WIN32)

   //extern int sscanf (const char *, const char *, ...);

   #define     tdcSScanf                  sscanf

   extern int  tdcPrintf   (const char* pFormat, ...);
   extern void tdcSNPrintf (char*	pBuf, unsigned int   bufSize,	const char* pFormat, ...);
   extern void tdcDebug    (const char*         pFilename,
                                  UINT16        line,
                                  INT16         strategy,
                            const char*         pModname,
                            const char*         pFormat, ... );

   #define DEBUG_INFO(x, y)                        tdcDebug (DBG_INFO,  (x), (y))
   #define DEBUG_INFO1(x, y, z1)                   tdcDebug (DBG_INFO,  (x), (y), (int) (z1))
   #define DEBUG_INFO2(x, y, z1, z2)               tdcDebug (DBG_INFO,  (x), (y), (int) (z1), (int) (z2))
   #define DEBUG_INFO3(x, y, z1, z2, z3)           tdcDebug (DBG_INFO,  (x), (y), (int) (z1), (int) (z2), (int) (z3))
   #define DEBUG_INFO4(x, y, z1, z2, z3, z4)       tdcDebug (DBG_INFO,  (x), (y), (int) (z1), (int) (z2), (int) (z3) , (int) (z4))

   #define DEBUG_WARN(x, y)                        tdcDebug (DBG_WARN,  (x), (y))
   #define DEBUG_WARN1(x, y, z1)                   tdcDebug (DBG_WARN,  (x), (y), (int) (z1))
   #define DEBUG_WARN2(x, y, z1, z2)               tdcDebug (DBG_WARN,  (x), (y), (int) (z1), (int) (z2))
   #define DEBUG_WARN3(x, y, z1, z2, z3)           tdcDebug (DBG_WARN,  (x), (y), (int) (z1), (int) (z2), (int) (z3))
   #define DEBUG_WARN4(x, y, z1, z2, z3, z4)       tdcDebug (DBG_WARN,  (x), (y), (int) (z1), (int) (z2), (int) (z3) , (int) (z4))

   #define DEBUG_ERROR(x, y)                       tdcDebug (DBG_ERROR, (x), (y))
   #define DEBUG_ERROR1(x, y, z1)                  tdcDebug (DBG_ERROR, (x), (y), (int) (z1))
   #define DEBUG_ERROR2(x, y, z1, z2)              tdcDebug (DBG_ERROR, (x), (y), (int) (z1), (int) (z2))
   #define DEBUG_ERROR3(x, y, z1, z2, z3)          tdcDebug (DBG_ERROR, (x), (y), (int) (z1), (int) (z2), (int) (z3))
   #define DEBUG_ERROR4(x, y, z1, z2, z3, z4)      tdcDebug (DBG_ERROR, (x), (y), (int) (z1), (int) (z2), (int) (z3) , (int) (z4))
#endif

/*  ----------------------------------------------------------------------------  */

#if defined (VXWORKS)

   extern int sscanf               (const char *, const char *, ...);
   extern int mon_broadcast_printf (const char *, ...);
   #if !defined (ANSI_API_H)
      extern int snprintf (char *, unsigned int, const char *, ...);
   #endif

   #define tdcSScanf                   sscanf
   #define tdcPrintf                   mon_broadcast_printf
   #define tdcSNPrintf                 snprintf

   extern void          tdcDebug0 (const char*         pFilename,
                                         UINT16        line,
                                         INT16         strategy,
                                   const char*         pModname,
                                   const char*         pFormat);

   extern void          tdcDebug1 (const char*         pFilename,
                                         UINT16        line,
                                         INT16         strategy,
                                   const char*         pModname,
                                   const char*         pFormat, int x1);

   extern void          tdcDebug2 (const char*         pFilename,
                                         UINT16        line,
                                         INT16         strategy,
                                   const char*         pModname,
                                   const char*         pFormat, int x1, int x2);

   extern void          tdcDebug3 (const char*         pFilename,
                                         UINT16        line,
                                         INT16         strategy,
                                   const char*         pModname,
                                   const char*         pFormat, int x1, int x2, int x3);

   extern void          tdcDebug4 (const char*         pFilename,
                                         UINT16        line,
                                         INT16         strategy,
                                   const char*         pModname,
                                   const char*         pFormat, int x1, int x2, int x3, int x4);

   #define DEBUG_INFO(x, y)                        tdcDebug0 (DBG_INFO,  (x), (y))
   #define DEBUG_INFO1(x, y, z1)                   tdcDebug1 (DBG_INFO,  (x), (y), (int) (z1))
   #define DEBUG_INFO2(x, y, z1, z2)               tdcDebug2 (DBG_INFO,  (x), (y), (int) (z1), (int) (z2))
   #define DEBUG_INFO3(x, y, z1, z2, z3)           tdcDebug3 (DBG_INFO,  (x), (y), (int) (z1), (int) (z2), (int) (z3))
   #define DEBUG_INFO4(x, y, z1, z2, z3, z4)       tdcDebug4 (DBG_INFO,  (x), (y), (int) (z1), (int) (z2), (int) (z3) , (int) (z4))

   #define DEBUG_WARN(x, y)                        tdcDebug0 (DBG_WARN,  (x), (y))
   #define DEBUG_WARN1(x, y, z1)                   tdcDebug1 (DBG_WARN,  (x), (y), (int) (z1))
   #define DEBUG_WARN2(x, y, z1, z2)               tdcDebug2 (DBG_WARN,  (x), (y), (int) (z1), (int) (z2))
   #define DEBUG_WARN3(x, y, z1, z2, z3)           tdcDebug3 (DBG_WARN,  (x), (y), (int) (z1), (int) (z2), (int) (z3))
   #define DEBUG_WARN4(x, y, z1, z2, z3, z4)       tdcDebug4 (DBG_WARN,  (x), (y), (int) (z1), (int) (z2), (int) (z3) , (int) (z4))

   #define DEBUG_ERROR(x, y)                       tdcDebug0 (DBG_ERROR, (x), (y))
   #define DEBUG_ERROR1(x, y, z1)                  tdcDebug1 (DBG_ERROR, (x), (y), (int) (z1))
   #define DEBUG_ERROR2(x, y, z1, z2)              tdcDebug2 (DBG_ERROR, (x), (y), (int) (z1), (int) (z2))
   #define DEBUG_ERROR3(x, y, z1, z2, z3)          tdcDebug3 (DBG_ERROR, (x), (y), (int) (z1), (int) (z2), (int) (z3))
   #define DEBUG_ERROR4(x, y, z1, z2, z3, z4)      tdcDebug4 (DBG_ERROR, (x), (y), (int) (z1), (int) (z2), (int) (z3) , (int) (z4))
#endif

/*  ----------------------------------------------------------------------------  */

extern T_TDC_BOOL    isDisaster        (void);
extern void          tdcSetDebugLevel  (const char*    pPar0,
                                        const char*    pPar1,
                                        const char*    pPar2);
extern void          tdcSetEnableLog   (const char*    pPar0,
                                        const char*    pPar1,
                                        const char*    pPar2);
extern void          tdcSetLogfileMode (const char*    pPar0,
                                        const char*    pPar1,
                                        const char*    pPar2);
extern void          tdcSetLogfileName (const char*    pPar0,
                                        const char*    pPar1,
                                        const char*    pPar2);

/*  ----------------------------------------------------------------------------  */
 
#define TDC_LOG_FILE_NAME           "tdcd.report"

/*  ----------------------------------------------------------------------------  */

#ifdef __cplusplus                            /* to be compatible with C++ */
   }
#endif


#endif
