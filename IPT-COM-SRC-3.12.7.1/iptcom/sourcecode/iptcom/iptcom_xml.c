/*******************************************************************************
*  COPYRIGHT   : (C) 2006 Bombardier Transportation
********************************************************************************
*  PROJECT     : IPTrain
*
*  MODULE      : iptcom_xml.c
*
*  ABSTRACT    : XML parser for iptcom
*
********************************************************************************
*  HISTORY     :
*	
* $Id: iptcom_xml.c 28987 2013-09-12 15:28:26Z bloehr $
*
*  CR-8123 (Bernd Lšhr, 2013-09-12)
* 			Enhanced parsing of XML comments, returning error code in loc.error
*
*  CR-3477 (Bernd Lšhr, 2012-02-10)
* 			T†V Assessment findings, for(;;) instead of while(1)
*  Internal (Bernd Loehr, 2010-08-16) 
* 			Old obsolete CVS history removed
*
* 
*******************************************************************************/

/*******************************************************************************
*  INCLUDES */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "iptcom.h"


/*******************************************************************************
*  DEFINES
*/

/*******************************************************************************
*  TYPEDEFS
*/

/*******************************************************************************
*  LOCAL DATA */

/*******************************************************************************
*  LOCAL FUNCTION DECLARATIONS
*/

/*******************************************************************************
*  LOCAL FUNCTIONS
*/

/*******************************************************************************
NAME:       iptXmlNextToken
ABSTRACT:   Returns next XML token. 
            Skips occurences of whitespace and <!...> and <?...>
RETURNS:    TOK_OPEN ("<"), TOK_CLOSE (">"), TOK_OPEN_END = ("</"),
            TOK_CLOSE_EMPTY = ("/>"), TOK_EQUAL = ("="), TOK_ID, TOK_EOF
*/
static int iptXmlNextToken(
   XML_LOCAL *pLoc) /* Pointer to local data */
{
   int ch = 0;
   char *p;
   
   /*   while(1)    */
   for (;;)
   {
      /* Skip whitespace */
      while (!feof(pLoc->infile) && (ch = fgetc(pLoc->infile)) <= ' ')	/*lint !e160 Lint objects a GNU warning suppression macro - OK */
         ;
      
      /* Check for EOF */
      if (feof(pLoc->infile)) /*lint !e611 Lint for VxWorks gets lost in macro defintions*/ 
      {
         return TOK_EOF;
      }
      
      /* Handle quoted identifiers */
      if (ch == '"') 
      {
         p = pLoc->tokenValue;
         while (!feof(pLoc->infile) && (ch = fgetc(pLoc->infile)) != '"')	/*lint !e160 Lint objects a GNU warning suppression macro - OK */
         {
            if (p < (pLoc->tokenValue + MAX_TOKLEN - 1))
               *(p++) = ch;
         }
         
         *(p++) = 0;
         return TOK_ID;
      }  
      else if (ch == '<') 
      {
         /* Tag start character */
         ch = fgetc(pLoc->infile);		 /*lint !e160 Lint objects a GNU warning suppression macro - OK */
         
         /* Begin fix CR-8123 */

         if (ch == '?')  /* Skip processing instruction */
         {
            while (!feof(pLoc->infile) && (ch = fgetc(pLoc->infile))) /*lint !e160 Lint objects a GNU warning suppression macro - OK */
            {
            	if (ch == '?')
                {
                	if ((ch = fgetc(pLoc->infile)) == '>')
                    {
                        break;
                    }
                    else
                    {
                    	ungetc(ch, pLoc->infile);
                    }
                }
            }
         }
         else if (ch == '!') 
         {
            /* Is it a comment? */
            if (!feof(pLoc->infile) && (ch = fgetc(pLoc->infile)))
            {
                if (ch == '-')
                {
                	if ((ch = fgetc(pLoc->infile) == '-'))
                    {
                        int endTagCnt = 0;
                        while (!feof(pLoc->infile) && (ch = fgetc(pLoc->infile))) /*lint !e160 Lint objects a GNU warning suppression macro - OK */
                        {
                           if (ch == '-')
                           {
                               endTagCnt++;
                           }
                           else if (ch == '>' && endTagCnt == 2)
                           {
                               endTagCnt = 0;
                               break;
                           }
                           else
                           {
                               endTagCnt = 0;
                           }
                        }
                        /* Exit on unexpected end-of-file */
                        if (endTagCnt != 2 && feof(pLoc->infile))
                        {
                            pLoc->error = IPT_PARSE_ERROR;
                            return TOK_EOF;
                        }
                    }
                }
                else
                {
                    while (!feof(pLoc->infile) && (ch = fgetc(pLoc->infile)) != '>')
                    ;
                }
            }
            /* Exit on unexpected end-of-file */
            if (feof(pLoc->infile))
            {
                pLoc->error = IPT_PARSE_ERROR;
                return TOK_EOF;
            }

         /* End fix CR-8123 */

         }
         else if (ch == '/')
         {
            return TOK_OPEN_END;
         }
         else 
         {
            (void) ungetc(ch, pLoc->infile);
            return TOK_OPEN;
         }
      }
      else if (ch == '/') 
      {
         ch = fgetc(pLoc->infile);	/*lint !e160 Lint objects a GNU warning suppression macro - OK */
         if (ch == '>') 
         {
            return TOK_CLOSE_EMPTY;
         }
         else
         {
            (void) ungetc(ch, pLoc->infile);
         }
      }
      else if (ch == '>') 
      {
         return TOK_CLOSE;
      }
      else if (ch == '=') 
      {
         return TOK_EQUAL;
      }
      else 
      {
         /* Unquoted identifier */ 
         p = pLoc->tokenValue;
         *(p++) = ch;
         while (!feof(pLoc->infile) && 
            (ch = fgetc(pLoc->infile)) != '<'	/*lint !e160 Lint objects a GNU warning suppression macro - OK */
            && ch != '>'
            && ch != '=' 
            && ch != '/' 
            && ch > ' ') 
         {
            if (p < (pLoc->tokenValue + MAX_TOKLEN - 1)) 
               *(p++) = ch;
         }
         
         *(p++) = 0;
         
         if (ch == '<' || ch == '>' || ch == '=' || ch == '/') 
            (void) ungetc(ch,pLoc->infile);
         
         return TOK_ID;
      }
   }
}   

/*******************************************************************************
NAME:       iptXmlNextTokenHl
ABSTRACT:   Returns next high level XML token. 
RETURNS:    TOK_START_TAG = TOK_OPEN + TOK_ID
            TOK_END_TAG = TOK_OPEN_END + TOK_ID + TOK_CLOSE
            Any Id is stored in pLoc->tokenValue
            Other tokens are returned as is
*/
static int iptXmlNextTokenHl(
   XML_LOCAL *pLoc) /* Pointer to local data */
{
   int token;
   
   token = iptXmlNextToken(pLoc);
   
   if (token == TOK_OPEN) 
   {
      pLoc->tagDepth++;
      token = iptXmlNextToken(pLoc);
      
      if (token == TOK_ID) 
      {
         strncpy(pLoc->tokenTag, pLoc->tokenValue, MAX_TAGLEN + 1);
         token = TOK_START_TAG;     /* TOK_OPEN + TOK_ID */
      }
      else
         token = TOK_EOF;     /* Something wrong, < should always be followed by a tag id */
   }
   else if (token == TOK_OPEN_END)
   {
      pLoc->tagDepth--;
      token = iptXmlNextToken(pLoc);
      
      if (token == TOK_ID) 
      {
         strncpy(pLoc->tokenTag, pLoc->tokenValue, MAX_TAGLEN + 1);
         token = TOK_END_TAG;  /* TOK_OPEN_END + TOK_ID + TOK_CLOSE */
      }
      else
         token = TOK_EOF;     /* Something wrong, </ should always be followed by a tag id + ">"*/
   }
   else if (token == TOK_CLOSE_EMPTY)
   {
      pLoc->tagDepth--;
   }
   else if (token == TOK_ID)
   {
      strncpy(pLoc->tokenTag, pLoc->tokenValue, MAX_TAGLEN + 1);
      token = TOK_ID;
   }
   
   return token;
}

/*******************************************************************************
*  GLOBAL FUNCTIONS
*/

/*******************************************************************************
NAME:       iptXmlOpen
ABSTRACT:   Opens the XML parsíng. 
RETURNS:    0 if OK, !=0 if not.
*/
int iptXmlOpen(
   XML_LOCAL *pLoc,  /* Pointer to local data */
   const char *file) /* Name of file to open */
{
   if ((pLoc->infile = fopen(file, "r")) == NULL)
      return (int)IPT_ERROR;
   
   pLoc->tagDepth = 0;
   pLoc->tagDepthSeek = 0;
   pLoc->error = IPT_OK;      /* CR-8123 */
   return 0;
}

/*******************************************************************************
NAME:       iptXmlClose
ABSTRACT:   Closes the XML parsíng. 
RETURNS:    0 if OK, !=0 if not.
*/
int iptXmlClose(
   XML_LOCAL *pLoc) /* Pointer to local data */
{
   fclose(pLoc->infile);
   return 0;
}

/*******************************************************************************
NAME:       iptXmlSeekStartTagAny
ABSTRACT:   Seek next tag on starting depth and return it in provided buffer.
Start tags on deeper depths are ignored.
RETURNS:    0 if found, !=0 if not or error
*/
int iptXmlSeekStartTagAny(
   XML_LOCAL *pLoc,/* Pointer to local data */
   char *tag,      /* Buffer for found tag */
   int maxlen)     /* Length of buffer */
{
   int token;
   int ret = 99;
   
   while (ret == 99)
   {
      token = iptXmlNextTokenHl(pLoc);
      
      if (token == TOK_EOF)
      {
         ret = -1;               /* End of file, interrupt */
      }
      else if (pLoc->tagDepth < (pLoc->tagDepthSeek - 1))
      {
         ret = -2;               /* No more tokens on this depth, interrupt */
      }
      else if (pLoc->tagDepth == pLoc->tagDepthSeek && token == TOK_START_TAG)
      {
         /* We are on the correct depth and have found a start tag */
         strncpy(tag, pLoc->tokenTag,maxlen);
         ret = 0;
      }
      /* else ignore */
   }
   
   return ret;
}

/*******************************************************************************
NAME:       iptXmlSeekStartTag
ABSTRACT:   Seek for a specific tag 
RETURNS:    0 if found, !=0 if not or error
*/
int iptXmlSeekStartTag(
   XML_LOCAL *pLoc,   /* Pointer to local data */
   const char *tag)   /* Tag to be found */
{
   int ret;
   char buf[MAX_TAGLEN+1];
   
   do 
   {
      ret = iptXmlSeekStartTagAny(pLoc, buf, sizeof(buf));
   } while (ret == 0 && strcmp(buf, tag) != 0);
   
   return ret;
}

/*******************************************************************************
NAME:       iptXmlEnter
ABSTRACT:   Enter new depth in XML file 
RETURNS:    -
*/
void iptXmlEnter(
   XML_LOCAL *pLoc) /* Pointer to local data */
{
   pLoc->tagDepthSeek++;
}

/*******************************************************************************
NAME:       iptXmlLeave
ABSTRACT:   Leave depth in XML file 
RETURNS:    -
*/
void iptXmlLeave(
   XML_LOCAL *pLoc) /* Pointer to local data */
{
   pLoc->tagDepthSeek--;
}

/*******************************************************************************
NAME:       iptXmlGetAttribute
ABSTRACT:   get value of next attribute, as string and if possible as integer 
RETURNS:    TOK_ATTRIBUTE if attribute found, returned token if not
*/
int iptXmlGetAttribute(
   XML_LOCAL *pLoc,   /* Pointer to local data */
   char *attribute,   /* Pointer to attribute */
   int *pValueInt,    /* Resulting integer value */
   char *value)       /* Resulting string value */
{
   int token;
   
   token = iptXmlNextTokenHl(pLoc);
   
   if (token == TOK_ID)
   {
      strncpy(attribute, pLoc->tokenValue, MAX_TOKLEN);
      token = iptXmlNextTokenHl(pLoc);
      
      if (token == TOK_EQUAL)
      {
         token = iptXmlNextTokenHl(pLoc);
         
         if (token == TOK_ID)
         {
            strncpy(value, pLoc->tokenValue, MAX_TOKLEN);
            *pValueInt = atoi(value);
            token = TOK_ATTRIBUTE;
         }
      }
   }
   
   return token;
}


