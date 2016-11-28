/*******************************************************************************
 * COPYRIGHT: (c) <year> Bombardier Transportation
 * Template: 3EST 207-5197 rev 0
 *******************************************************************************
 * %PCMS_HEADER_SUBSTITUTION_START%
 * COMPONENT:     <component name>
 *
 * ITEM-SPEC:     %PID%
 *
 * FILE:          %PM%
 *
 * REQ DOC:       <requirements document identity>
 * REQ ID:        <list of requirement identities>
 *
 * ABSTRACT:      <short description>
 *
 *******************************************************************************
 * HISTORY:
 %PL%
 *%PCMS_HEADER_SUBSTITUTION_END%
 ******************************************************************************/


/*******************************************************************************
 * INCLUDES
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include "iptcom.h"
#include "dledsDbg.h"
#include "dledsInstall.h"
#include "dledsPlatform.h"

/*******************************************************************************
 * DEFINES
 */





/*******************************************************************************
 * TYPEDEFS
 */



/*******************************************************************************
 * LOCAL FUNCTION DECLARATIONS
 */



/*******************************************************************************
 * LOCAL VARIABLES
 */



/*******************************************************************************
 * GLOBAL VARIABLES
 */
int dledsDebugOn = 0;


/*******************************************************************************
 * LOCAL FUNCTION DEFINITIONS
 */

/*******************************************************************************
 *
 * Function name: stripFID
 *
 * Abstract:      Removes the path to the file only showing the filename.
 *
 * Return value:  Pointer to the position of the filename, or error string if input is null.
 *
 * Globals:       <list of used global variables>
 */
static const char * stripFID(const char* pFileId)
{
    static const char fIdErrStr[] = "File Id is NULL";
    char * tmp_ptr;

    if(pFileId == 0)
    {
        return fIdErrStr;
    }
    tmp_ptr = (char *)pFileId + strlen(pFileId);
    /* find file name */
    while((*tmp_ptr != '\\') && (*tmp_ptr != '/') && (tmp_ptr >= pFileId))
    {
        --tmp_ptr;
    }
    return ++tmp_ptr;
}


/*******************************************************************************
 * GLOBAL FUNCTIONS
 */


/*******************************************************************************
 *
 * Function name: dledsEventLogWrite
 *
 * Abstract:      This function
 *
 * Return value:
 *
 *
 * Globals:       -
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
    int         arg6)
{
    struct stat stStat;
    static int checkDirExist = 0;


    if (dledsDebugOn != 0)
    {
        if (checkDirExist == 0)
        {
            if (stat(DBG_FILE_PATH, &stStat) != 0)
            {
                checkDirExist = 2;
            }
            else
            {
                checkDirExist = 1;
            }
        }
        if(pFileId == 0 || fmt == 0)
        {
            /* null pointer */
        }
        else if(*pFileId == 0 || *fmt == 0)
        {
            /* empty string */
        }
        else if (checkDirExist != 1)
        {
            /* empty string */
        }
        else
        {
            static FILE* debugEventOutput = 0;
            FILE* debugCurrEvent = 0;
            long currFilePos = 0;
            struct tm ltm;
            time_t ltime;

            time(&ltime);
            localtime_v(&ltime, &ltm);

            if(debugEventOutput == 0)
            {
                if((debugCurrEvent = fopen(DBG_EVENT_POS, "r+")) == NULL)  /* event position file does not exist */
                {
                    currFilePos = 0;
                    if((debugEventOutput = fopen(DBG_EVENT_LOG, "w+")) != NULL)
                    {
                        if(fclose(debugEventOutput) != 0)
                        {
                            /* do nothing */
                        }
                    }
                }
                else  /* read current position */
                {
                    fscanf(debugCurrEvent,"%ld", &currFilePos);
                    if ((currFilePos < 0) || (currFilePos >= MAX_EVENTLOG_SIZE))
                    {
                        currFilePos = 0;
                    }
                    if(fclose(debugCurrEvent) != 0)
                    {
                        /* do nothing */
                    }
                }
                if((debugEventOutput = fopen(DBG_EVENT_LOG, "r+")) == NULL)
                {
                    if((debugEventOutput = fopen(DBG_EVENT_LOG, "w+")) == NULL)
                    {
                        fprintf(stderr, "dleds: Unable to open event log file (%s)\r\n", strerror(errno));
                        debugEventOutput = stderr;
                    }
                }
                else
                {
                    if(fseek(debugEventOutput, currFilePos, SEEK_SET))
                    {
                        fprintf(debugEventOutput, "eventHandler - fseek failed aborting logging (%s).", strerror(errno));
                        debugEventOutput = stderr;
                    }
                }
            }
            if ((debugEventOutput != 0) && (debugEventOutput != stderr))
            {
                if(ftell(debugEventOutput) >= MAX_EVENTLOG_SIZE)
                {
                    if(fseek(debugEventOutput, 0, SEEK_SET))
                    {
                        fprintf(debugEventOutput, "eventHandler - fseek failed aborting logging (%s).", strerror(errno));
                        debugEventOutput = stderr;
                    }
                }
            }
            fprintf(debugEventOutput, "%04d-%02d-%02d %02d:%02d:%02d ",
                    1900+(ltm.tm_year), 1+(ltm.tm_mon), ltm.tm_mday,
                    ltm.tm_hour, ltm.tm_min, ltm.tm_sec);
            fprintf(debugEventOutput, "LNR: %4d ", (int)lineNr);
            fprintf(debugEventOutput, "FID: %-23s ", stripFID(pFileId));
            fprintf(debugEventOutput, fmt, arg1, arg2, arg3, arg4, arg5, arg6);
            fprintf(debugEventOutput, "\r\n");
            if((debugCurrEvent = fopen(DBG_EVENT_POS, "w+")) == NULL)
            {
                fprintf(debugEventOutput, "eventHandler - fopen failed aborting logging (%s).", strerror(errno));
                debugEventOutput = stderr;
            }
            else
            {
                fprintf(debugCurrEvent,"%ld",ftell(debugEventOutput));
                if(fclose(debugCurrEvent) != 0)
                {
                    /* do nothing */
                }
            }
            fflush(debugEventOutput);
        }
    }
}
