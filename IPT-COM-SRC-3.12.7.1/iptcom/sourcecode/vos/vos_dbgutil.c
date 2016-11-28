/*******************************************************************************
 *  COPYRIGHT    : (c) 2003-2011 Bombardier Transportation Sweden AB
 *******************************************************************************
 *  PROJECT      :
 *
 *  MODULE       : dbg_util.c
 *
 *  ABSTRACT     :
 *
 *  REMARKS      :
 *
 *******************************************************************************
 *  HISTORY     :
 *	
 * $Id: vos_dbgutil.c 33509 2014-07-13 08:49:39Z gweiss $
 *
 *  CR-7832 (Bernd Loehr, 2013-09-30)
 * 			Remove error message in CSS when no IPTDir server is ready
 *
 *  CR-480 (Gerhard Weiss, 2011-09-07)
 * 			Windows Support for OSBuild
 *          (export debug mode)
 *
 *  Internal (Bernd Loehr, 2010-08-13) 
 * 			Old obsolete CVS history removed
 * 
 ******************************************************************************/

/*******************************************************************************
*   INCLUDES */
#include <stdio.h>			/* Standard includes */
#include <string.h>

#if defined(WIN32)
 #include "vos_socket.h"       /* OS independent socket definitions */
 #include <io.h>
 #include <errno.h>				/* errno */
#elif defined(LINUX)
 #include <unistd.h>
 #include <errno.h>
#elif defined(VXWORKS)
 #include <unistd.h>
#elif defined(__INTEGRITY)
 #include <unistd.h>
 #include <errno.h>
#elif defined(DARWIN)
 #include <unistd.h>
 #include <errno.h>
#endif

#include "iptcom.h"				/* Common type definitions for IPT */
#include "iptcom_priv.h"
#include "vos.h"

/*******************************************************************************
*   DEFINES */
#define MAX_LOG_FILE_SIZE_DEFAULT 64*1024
#define MAX_LOG_FILES 3


/*******************************************************************************
*   LOCALS */
static UINT16 iptDebugInfoMask = INF_FILE | INF_LINE | INF_DATETIME;

static FILE *iptDebugLogFile = 0;
static UINT16 iptLogFileIndex = 0;
static char logFileName[MAX_LOG_FILES][MAX_TOKLEN+1];
static UINT16 iptLogSeekFailed = 0;
static int iptLogFileSize = MAX_LOG_FILE_SIZE_DEFAULT;

/*******************************************************************************
*   GLOBALS */
UINT16 iptDebugMask = IPT_ERR;  /* default at startup */

/*******************************************************************************
*   LOCAL FUNCTIONS */


/*******************************************************************************
NAME:			IPTVosTruncate
ABSTRACT:   Truncates the file to the given length
RETURNS:		0 if the operation completed successfully
            != 0 if an error occurred
*/
static void IPTVosTruncate(FILE *fp, long pos)
{
   int fd;

#if defined(WIN32)
   int errCode;
#elif defined(LINUX) || defined(DARWIN)
   char errBuf[80];
#elif defined(VXWORKS)
   char errBuf[80];
#elif defined(__INTEGRITY)
   char* ErrBuf;
#endif


#if defined(WIN32)
   fd = _fileno(fp);
   if (fd != -1)
   {
      if (_chsize(fd, pos) != 0)
      {
         errCode = GetLastError();
         MON_PRINTF("IPTVosTruncate: Call of _chsize failed, Windows error=%d %s\n",errCode, strerror(errCode)); 
      }
   }
   else
   {
      errCode = GetLastError();
      MON_PRINTF("IPTVosTruncate: Call of _fileno failed, Windows error=%d %s\n",errCode, strerror(errCode)); 
   }
#elif defined(LINUX) || defined(DARWIN)
   fd = fileno(fp);
   if (fd != -1)
   {
      if (ftruncate(fd, pos) != 0)
      {
         (void)strerror_r(errno,errBuf,sizeof(errBuf));
         MON_PRINTF("IPTVosTruncate: Call of ftruncate failed, Linux errno=%d %s\n",errno, errBuf);
      }
   }
   else
   {
      (void)strerror_r(errno,errBuf,sizeof(errBuf));
      MON_PRINTF("IPTVosTruncate: Call of fileno failed, Linux errno=%d %s\n",errno, errBuf);
   }
#elif defined(VXWORKS)
   fd = fileno(fp);
   if (fd != -1)
   {
      if (ftruncate(fd, pos) != 0)
      {
         (void)strerror_r(errno, errBuf);
         MON_PRINTF("IPTVosTruncate: Call of ftruncate failed, VxWorks errno=%#x %s\n",errno, errBuf);
      }
   }
   else
   {
      (void)strerror_r(errno, errBuf);
      MON_PRINTF("IPTVosTruncate: Call of fileno failed, VxWorks errno=%#x %s\n",errno, errBuf);
   }
#elif defined(__INTEGRITY)
   fd = fileno(fp);
   if (fd != -1)
   {
      if (ftruncate(fd, pos) != 0)
      {
         ErrBuf = strerror(errno);
         MON_PRINTF("IPTVosTruncate: Call of ftruncate failed, INTEGRITY errno=%#x %s\n", errno, ErrBuf);
      }
   }
   else
   {
      ErrBuf = strerror(errno);
      MON_PRINTF("IPTVosTruncate: Call of fileno failed, INTEGRITY errno=%#x %s\n", errno, ErrBuf);
   }
#else
#error "Code for target architecture is missing"
#endif

}

/*******************************************************************************
NAME:			IPTVosStripFID
ABSTRACT:	Removes the path to the file only showing the filename 
RETURNS:		A pointer to the filename
*/
static char *IPTVosStripFID (
	char *pFileId)		/* File identifier */
{
	char *pTmp;			/* Temporary buffer pointer */
	
	/* Point to end of string */
	pTmp = (char *)pFileId + strlen(pFileId);

	/* Find file name */
	while( (*pTmp != '\\') && (pTmp >= pFileId) )
	{
		pTmp--;
	}

	pTmp++;
	
	return pTmp;
}

/*******************************************************************************
NAME:      IPTVosWriteLogIndex
ABSTRACT:  Write current log file index
RETURNS:
*/
static void IPTVosWriteLogIndex(char *pFileName)
{
   FILE *indexFile = NULL;

   /* Write current file */
   indexFile = fopen(pFileName, "w+");

   if (indexFile)
   {
      fprintf(indexFile, "%d", iptLogFileIndex);

      fclose(indexFile);
   }
   else
   {
      IPTVosPrint0(IPT_ERR, "ERROR open log index file\n");
   }
}

/*******************************************************************************
*   GLOBAL FUNCTIONS */

/*******************************************************************************
NAME:			IPTVosSetPrintMask
ABSTRACT:	Set printout mask
RETURNS:		-
*/
void IPTVosSetPrintMask(
   UINT16 newPrintMask)       /* New print mask */
{
   iptDebugMask = newPrintMask;
}

/*******************************************************************************
NAME:			IPTVosSetInfoMask
ABSTRACT:	Set info level mask
RETURNS:		-
*/
void IPTVosSetInfoMask(
   UINT16 newInfoMask)       /* New print mask */
{
   iptDebugInfoMask = newInfoMask;
}

/*******************************************************************************
NAME:			IPTVosGetPrintMask
ABSTRACT:	Get current printout mask
RETURNS:		-
*/
UINT16 IPTVosGetPrintMask()
{
   return iptDebugMask;
}

/*******************************************************************************
NAME:			IPTVosGetInfoMask
ABSTRACT:	Get current info level mask
RETURNS:		-
*/
UINT16 IPTVosGetInfoMask()
{
   return iptDebugInfoMask;
}

/*******************************************************************************
NAME:			IPTVosGetLogFileSize
ABSTRACT:	Get log file size
RETURNS:		Size in bytes
*/
int IPTVosGetLogFileSize(void)
{
   return iptLogFileSize;
}

/*******************************************************************************
NAME:			IPTVosSetLogFileSize
ABSTRACT:	Set log file size
RETURNS:		-
*/
void IPTVosSetLogFileSize(int size)
{
   iptLogFileSize = size;
}

/*******************************************************************************
NAME:			IPTVosDPrint
ABSTRACT:	IPTVOS implementation of printf	 
RETURNS:		-
*/
void IPTVosDPrint(
	int DebugSettingsOveride,	/* Override debug settings ? */
	char* pFileId,					/* File name */
	UINT16 lineNr,					/* Line no */
	UINT16 category,				/* Category                   */
	const char * fmt,						/* Format string for print    */
	int    arg1,					/* First of six required args */
	int    arg2,
	int    arg3,
	int    arg4,
	int    arg5,
	int    arg6,
	int    arg7)
{
   char dateTime[25];
   long pos;
   
	/* Print filename ? */
	if (!DebugSettingsOveride)
	{
	   /* Print Category ? */
	   if(iptDebugInfoMask & INF_CATEGORY)
	   {
		   if (category & IPT_ERR)
		   {
		      MON_PRINTF("ERROR: ");
		   }
         else if (category & IPT_WARN)
         {
		      MON_PRINTF("WARNING: ");
         }
         
		   if (category & IPT_VOS)
         {
		      MON_PRINTF("VOS: ");
         }
		   if (category & IPT_NETDRIVER)
         {
		      MON_PRINTF("NETDRIVER: ");
         }
		   if (category & IPT_PD)
         {
		      MON_PRINTF("PD: ");
         }
		   if (category & IPT_MD)
         {
		      MON_PRINTF("MD: ");
         }
		   if (category & IPT_CONFIG)
         {
		      MON_PRINTF("CONFIG: ");
         }
		   if (category & IPT_OTHER)
         {
		      MON_PRINTF("OTHER: ");
         }
	   }
      
	   /* Print Date and Time ? */
      if(iptDebugInfoMask & INF_DATETIME)
      {
        IPVosDateTimeString(dateTime);
		  MON_PRINTF("%s ", dateTime);
      }
	 
	   if(iptDebugInfoMask & INF_FILE) 
	   {
		   MON_PRINTF("FID: %s ", IPTVosStripFID(pFileId));
	   }

	   /* Print line no ? */
	   if(iptDebugInfoMask & INF_LINE) 
	   {
		   MON_PRINTF("LNR: %d ", (int)lineNr);
	   }
   }

	/* Print data */
	MON_PRINTF(fmt, arg1, arg2, arg3, arg4, arg5, arg6, arg7);

   /* Log to file? */
   if (iptDebugLogFile)
   {
      /* Check size and rewind if necessary */
      if ((pos = ftell(iptDebugLogFile)) >= iptLogFileSize)
      {
         IPTVosTruncate(iptDebugLogFile, pos);

         if (fseek(iptDebugLogFile, 0, SEEK_SET) != 0)
         {
            if (iptLogSeekFailed == 0)
            {
               fprintf(iptDebugLogFile, "Seek failed - logging aborted");
               iptLogSeekFailed = 1;
            }

            return;
         }
         else
         {
            /* Seek ok */
            iptLogSeekFailed = 0;
         }
      }

      /* Print filename ? */
	   if (!DebugSettingsOveride)
	   {
	      /* Print Category ? */
	      if(iptDebugInfoMask & INF_CATEGORY)
	      {
		      if (category & IPT_ERR)
		      {
		         fprintf(iptDebugLogFile,"ERROR: ");
		      }
            else if (category & IPT_WARN)
            {
		         fprintf(iptDebugLogFile,"WARNING: ");
            }
	    
		      if (category & IPT_VOS)
            {
		         fprintf(iptDebugLogFile,"VOS: ");
            }
		      if (category & IPT_NETDRIVER)
            {
		         fprintf(iptDebugLogFile,"NETDRIVER: ");
            }
		      if (category & IPT_PD)
            {
		         fprintf(iptDebugLogFile,"PD: ");
            }
		      if (category & IPT_MD)
            {
		         fprintf(iptDebugLogFile,"MD: ");
            }
		      if (category & IPT_CONFIG)
            {
		         fprintf(iptDebugLogFile,"CONFIG: ");
            }
		      if (category & IPT_OTHER)
            {
		         fprintf(iptDebugLogFile,"OTHER: ");
            }
	      }
        
	      /* Print Date and Time ? */
         if(iptDebugInfoMask & INF_DATETIME)
         {
           IPVosDateTimeString(dateTime);
		     fprintf(iptDebugLogFile,"%s ", dateTime);
         }
	    
	      if(iptDebugInfoMask & INF_FILE) 
	      {
		      fprintf(iptDebugLogFile,"FID: %s ", IPTVosStripFID(pFileId));
	      }

	      /* Print line no ? */
	      if(iptDebugInfoMask & INF_LINE) 
	      {
		      fprintf(iptDebugLogFile,"LNR: %d ", (int)lineNr);
	      }

      }

	   /* Print data */
	   fprintf(iptDebugLogFile,fmt, arg1, arg2, arg3, arg4, arg5, arg6, arg7);
	}
}

/*******************************************************************************
NAME:			IPTVosDFile
ABSTRACT:	Setting file pointer for debug outputs.
            If a filename is given a file is open
            otherwise is the filepointer set to standard output	 
RETURNS:		-
*/
void IPTVosDFile(
	char* pFileName)					/* File name */
{
   int i = 0;
   int readIndex;
   char tmpFileName[MAX_TOKLEN+1];

   FILE *indexFile = NULL;

   if ((pFileName != NULL) && (pFileName[0] != 0))
   {
      /* Populate the file names array */
      for (i = 0; i < MAX_LOG_FILES; i++)
      {
         sprintf(tmpFileName, "%s_%d", pFileName, i);
         strncpy(logFileName[i], tmpFileName, MAX_TOKLEN+1);
      }

      /* Create current index file name */
      sprintf(tmpFileName, "%s_current", pFileName);

      /* Any previous open file? */
      if (iptDebugLogFile != 0)
      {
         /* Yes, close the old file */
         fclose(iptDebugLogFile);

         /* Use next log file */
         ++iptLogFileIndex;
         iptLogFileIndex %= MAX_LOG_FILES;

         /* Open the new file */
         iptDebugLogFile = fopen(logFileName[iptLogFileIndex], "w+");

         /* Update log index */
         IPTVosWriteLogIndex(tmpFileName);

         return;
      }

      /* Try to read index file to see which is the next file to use */
      indexFile = fopen(tmpFileName, "r+");

      if (indexFile != NULL)
      {
         /* Read index file to see which is the next log file to use */
         fscanf(indexFile, "%d", &readIndex);

         if ((readIndex >= 0) && (readIndex < MAX_LOG_FILES))
         {
            /* Pick next index file in turn */
            iptLogFileIndex = (readIndex + 1) % MAX_LOG_FILES;
         }

         fclose(indexFile);
      }

      /* Open the new file */
      iptDebugLogFile = fopen(logFileName[iptLogFileIndex], "w+");

      IPTVosWriteLogIndex(tmpFileName);
   }
   else
   {
      if (iptDebugLogFile != 0)
      {
         fclose(iptDebugLogFile);
      }
      iptDebugLogFile = 0;
   }
}

/*******************************************************************************
NAME:			IPTVosGetDebugFileName
ABSTRACT:	get the current debug file name	 
RETURNS:		-
*/
const char * IPTVosGetDebugFileName()
{
   if (iptDebugLogFile != 0)
   {
      return (logFileName[iptLogFileIndex]);
   }
   else
   {
      return(NULL);
   }
}

/*******************************************************************************
NAME:			IPTVosGetDebugFileIndex
ABSTRACT:	Get the current debug file index	 
RETURNS:		-
*/
UINT16 IPTVosGetDebugFileIndex()
{
   return(iptLogFileIndex);
}

/*******************************************************************************
NAME:       IPTVosPrintSocketError
ABSTRACT:   Get latest socket error
RETURNS:    Socket error
*/
void IPTVosPrintSocketError(void)
{
#if defined(WIN32)
   int errCode;

   errCode = WSAGetLastError();
   MON_PRINTF(" Windows error=%d %s\n",errCode, strerror(errCode));
   /* Log to file? */
   if (iptDebugLogFile)
   {
      fprintf(iptDebugLogFile," Windows error=%d %s\n",errCode, strerror(errCode));
   }

#elif defined(LINUX) || defined(DARWIN)
   char errBuf[80];

   (void)strerror_r(errno,errBuf,sizeof(errBuf));
   MON_PRINTF(" Linux errno=%d %s\n", errno, errBuf);

   /* Log to file? */
   if (iptDebugLogFile)
   {
      fprintf(iptDebugLogFile," Linux errno=%d %s\n", errno, errBuf);
   }

#elif defined(VXWORKS)
   char errBuf[80];
   static int last_errno = 0;
   static int count = 0;

   (void)strerror_r(errno, errBuf);

   if (errno != last_errno)
   {
      last_errno = errno;
      count = 1;
      
      MON_PRINTF(" VxWorks errno=%#x %s\n", errno, errBuf);
   }
   else if (count == 1)
   {
      MON_PRINTF(" VxWorks errno=%#x repeated...\n", errno);
      count++;
   }

   /* Log to file? */
   if (iptDebugLogFile)
   {
      fprintf(iptDebugLogFile," VxWorks errno=%#x %s\n", errno, errBuf);
   }

#elif defined(__INTEGRITY)
   char* ErrBuf;

   ErrBuf = strerror(errno);
   MON_PRINTF(" INTEGRITY errno=%#x %s\n", errno, ErrBuf);

   /* Log to file? */
   if (iptDebugLogFile)
   {
      fprintf(iptDebugLogFile," INTEGRITY errno=%#x %s\n", errno, ErrBuf);
   }

#else
#error "Code for target architecture is missing"
#endif
}


