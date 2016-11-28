/*******************************************************************************
* COPYRIGHT   : (c) 2010 Bombardier Transportation
********************************************************************************
* PROJECT     : Remote Download
*
* MODULE      : $Workfile: $  $Revision: $
*
* DESCRIPTION : See documentation section.
*
********************************************************************************
* HISTORY  :
*   $Log: $
* 
* 
*******************************************************************************/
#ifndef DLEDSDBG_H
#define DLEDSDBG_H

/*******************************************************************************
*   INCLUDES
*/
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <stdlib.h>

#include "dleds.h"

/*******************************************************************************
*  TYPEDEFS
*/

/*******************************************************************************
*   GLOBAL FUNCTIONS
*/
void dledsEventLogWrite(
    const char* pFileId,    /* file name (__FILE__) */
    UINT32      lineNr,     /* line number (__LINE__) */
    const char* fmt,        /* format control string */
    int         arg1,       /* first of six required args */
    int         arg2,
    int         arg3,
    int         arg4,
    int         arg5,
    int         arg6);

/*******************************************************************************
*  MACROS
*/

/* reentrant localtime wrapper called localtime_v */
#ifndef localtime_v
    #define localtime_v(todArg, resultArg) localtime_r(todArg, resultArg)
#endif /* localtime_v */


/*******************************************************************************
*   DEFINES
*/ 

#define DLEDS_POSITION __FILE__, __LINE__


#define DebugError0(fmt)                        DebugError(fmt,0,0,0,0,0,0)
#define DebugError1(fmt, a)                     DebugError(fmt,a,0,0,0,0,0)
#define DebugError2(fmt, a, b)                  DebugError(fmt,a,b,0,0,0,0)
#define DebugError3(fmt, a, b, c)               DebugError(fmt,a,b,c,0,0,0)
#define DebugError4(fmt, a, b, c, d)            DebugError(fmt,a,b,c,d,0,0)
#define DebugError5(fmt, a, b, c, d, e)         DebugError(fmt,a,b,c,d,e,0)
#define DebugError6(fmt, a, b, c, d, e, f)      DebugError(fmt,a,b,c,d,e,f)

#define DebugError(fmt, p1, p2, p3, p4, p5, p6) \
    dledsEventLogWrite(DLEDS_POSITION,fmt,(int)p1,(int)p2,(int)p3,(int)p4,(int)p5,(int)p6)


#endif /* DLEDSDBG_H */
