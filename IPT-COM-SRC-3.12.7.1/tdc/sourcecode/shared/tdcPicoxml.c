/*
 *  $Id: tdcPicoxml.c 11861 2012-04-19 11:30:50Z gweiss $
 *
 *  DESCRIPTION    Small footprint XML parser
 *                 Functions for parsing an xml file from a buffered FILE*
 *
 *  AUTHOR         Thomas Gallenkamp
 *
 *  REMARKS        Adapted for TDC usage (CR-382) by B. Loehr
 *
 *  DEPENDENCIES   Either the switch VXWORKS, INTEGRITY, LINUX or WIN32 has to be set
 *
 *  MODIFICATIONS (log starts 2010-08-11)
 *
 *  CR-3477 (Gerhard Weiss, 2012-04-18)
 *          Adding corrections for TÜV-Assessment
 *
 *  All rights reserved. Reproduction, modification, use or disclosure
 *  to third parties without express authority is forbidden.
 *  Copyright Bombardier Transportation GmbH, Germany, 2002-2012.
 *
 */

/*******************************************************************************
 * INCLUDES
 */

#include <stdio.h>
#include <string.h>
#include "tdc.h"
#include "tdcMisc.h"
#include "tdcPicoxml.h"
#include "tdcCrc32.h"


/*******************************************************************************
 * DEFINITIONS
 */

/*
 #define DEBUG 1
 */
#define TOK_MAXLEN            1000          /**< */
#define MAX_LOOKAHEAD         10            /**< */
#define DLU_PACKAGE_IDENT     "DLUPACK\0"   /**< */
#define DLU_CRC_START_VAL     0xFFFFFFFF    /**< */
#define DLU_CRC32_HEADER_LEN  0x00000034    /**< The size of the DLU header
                                                    which is checked by CRC */

/*******************************************************************************
 * TYPEDEFS
 */

/**  */
typedef struct
{
    INT32 idx;
    INT32 buffer[MAX_LOOKAHEAD];
} PICOXML_LOOKAHEAD_T; 

/**  */
typedef struct STR_DLU_HEADER
{
    UINT8  packageIdent[8];
    UINT32 headerLength;
    UINT32 headerCrc32;
    UINT32 headerVersion;
    UINT32 dluVersion;
    UINT32 dluDataSize;
    UINT32 dluTypeCode;
    UINT32 dluCrc32;
    UINT8  dluName[32];
} /*__attribute__((packed))*/ TYPE_DLU_HEADER;

/**  */
typedef enum
{
    TOK_OPEN = 0,
    TOK_CLOSE,
    TOK_ID,
    TOK_TAG,
    TOK_EOF
} PICOXML_TOKEN_T;

/******************************************************************************
 *   LOKALS
 */

/**  */
static PICOXML_LOOKAHEAD_T lookahead = {0, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};
 
#ifdef DEBUG
/**  */
static char *token_id_phony[] = {"Open", "Close", "Id", "Tag", "EOF" };
#endif

/** Pointer to FILE descriptor */
static T_FILE *infile;
/**  */
static char token_value[TOK_MAXLEN];

/**  */
static INT32  tag_depth;
/**  */
static INT32  tag_depth_seek;
/**  */
static char token_tag[PICOXML_MAX_TAGLEN + 1];

/**
 *  @name Static functions 
 *  @{*/
static INT32 my_fgetc(T_FILE *inputfile);
static void my_ungetc(INT32 ch);
static PICOXML_TOKEN_T next_token(void);
static PICOXML_TOKEN_T next_token_hl(void);
/**@}*/

/*****************************************************************************
 *   LOCAL FUNCTIONS
 */

/******************************************************************************/
/** Implementation of "own" fgetc() allows for multiple character unget
 *
 *  @param[in]      inputfile       Pointer to FILE descriptor.
 *  @retval         EOF:            End-of-File
 *  @retval         next char in stream
 */
static INT32 my_fgetc(T_FILE *inputfile)
{
    if (lookahead.idx)
    {
        return lookahead.buffer[--lookahead.idx];
    }
    /*lint -e(160) fgetc is expanded to an COTS macro. This macro cannot
                be apapted to lint's liking */
    return (INT32)fgetc(inputfile); /*lint !e64 type of inputfile ok*/
}

/******************************************************************************/
/** Implementation of "own" ungetc() allows for multiple character unget
 *
 *  @param[in]      ch      Character to "unget".
 */
static void my_ungetc(INT32 ch)
{
    if (lookahead.idx < MAX_LOOKAHEAD)
    {
        lookahead.buffer[lookahead.idx++] = ch;
    }
}

/******************************************************************************/
/** Returns "low level" token.
 *
 *  Low level token will classify input as follows and returns token code.
 *
 *  @retval TOK_OPEN:   "<" character (skip occurences of "<!...> and <?...> ")
 *  @retval TOK_CLOSE:  ">" character
 *  @retval TOK_ID:     "Identifier". Identifiers are charactersequences
 *                      limitedby whitespace characters or special
 *                      characters ("<>").
 *                      Identifiers in quotes (") may contain whitespace and
 *                      special characters.
 *  @retval TOK_EOF:    End of file reached
 *
 */
static PICOXML_TOKEN_T next_token(void)
{
    INT32 ch = 0;
    char    *p;

    while (1)
    {
        /* Skip whitespace */
        while (!feof((FILE *)infile))
        {
            ch = my_fgetc(infile);
            if (ch > ' ')
            {
               break; 
            } 
        }

        /* Check for EOF */
        if (feof((FILE *)infile))
        {
            return TOK_EOF;
        }

        /* Handle quoted identifiers */
        if (ch == '"')
        {
            p = token_value;
            while (!feof((FILE *)infile))
            {
                ch = my_fgetc(infile);
                if (ch == '"')
                {
                    break;
                }
                if ( p < (token_value + TOK_MAXLEN - 2) )
                {
                    *(p++) = (char)ch;
                }
            }
            *(p++) = 0;
            return TOK_ID;
        }
        /* Tag start character */
        else if (ch == '<')
        {
            ch = my_fgetc(infile);
            if ( (ch == '?') || (ch == '!') )
            {
                while (!feof((FILE *)infile))
                {
                    ch = my_fgetc(infile);
                    if (ch == '>')
                    {
                        break;
                    }
                }
            }
            else
            {
                my_ungetc(ch);
                return TOK_OPEN;
            }
        }
        else if (ch == '>')
        {
            return TOK_CLOSE;
        }
        /* Unquoted identifier */
        else
        {
            p      = token_value;
            *(p++) = (char)ch;

            while (!feof((FILE *)infile))
            {
                ch = my_fgetc(infile);
                if ( (ch == '<') || (ch == '>') || (ch <= ' ') )
                {
                    break;
                }
                   
                if ( p < (token_value + TOK_MAXLEN - 2) )
                {
                    *(p++) = (char)ch;
                }
            }

            *(p++) = 0;

            if ( (ch == '<') || (ch == '>') )
            {
                my_ungetc(ch);
            }

            return TOK_ID;
        }
    }
}

/******************************************************************************/
/** Returns "high level" token.
 *
 *  High level token will classify input as follows and return a token code.
 *
 *  @retval TOK_TAG:    XML Tag. token_value contains tag string.
 *  @retval TOK_ID:     "Identifier". Identifiers are charactersequences limited
 *                      by whitespace characters or special characters ("<>").
 *                      Identifiers in quotes (") may contain whitespace and
 *                      special characters.
 *  @retval TOK_EOF:    End of file reached
 */
static PICOXML_TOKEN_T next_token_hl(void)
{
    PICOXML_TOKEN_T token;
    
    token = next_token();

    if (token == TOK_OPEN)
    {
        token = next_token();
        if (token == TOK_ID)
        {
            tdcStrNCpy(token_tag, token_value, PICOXML_MAX_TAGLEN);
            //while ( ((token = next_token()) != TOK_EOF) && (token != TOK_CLOSE) )
            do
            {
                token = next_token(); 
            }
            while ((token != TOK_EOF) && (token != TOK_CLOSE));
            
            if (token == TOK_CLOSE)
            {
                token = TOK_TAG;
            }
        }
    }

#ifdef DEBUG
    printf("HL:%-10s ", token_id_phony[token]);
    if (token == TOK_ID)
    {
        printf("%s\n", token_value);
    }
    if (token == TOK_TAG)
    {
        printf("%s\n", token_tag);
    }
    else
    {
        printf("\n");
    }
#endif

    return token;
}

#ifndef VXWORKS
/******************************************************************************/
/** Compute the CRC of the header
 *
 *  This function has been 'borrowed' from the makedlu project.
 *
 *  @param[in]      p_dlu       Pointer to dl2 header.
 *  @retval         crc
 */
static UINT32 calcDLUHeaderCRC32(TYPE_DLU_HEADER *p_dlu)
{
    UINT32    headerCRC;
    UINT8     *startHeader;

    startHeader = (UINT8 *)&p_dlu->headerVersion;
    headerCRC   = crc32( DLU_CRC_START_VAL, startHeader, DLU_CRC32_HEADER_LEN );

    return headerCRC;
}

/******************************************************************************/
/** Check the integrity of a DL2 header
 *
 *  This function has been 'borrowed' from the makedlu project.
 *
 *  @param[in]      p_dlu       String pointer to path of cstSta.dl2 file.
 *  @retval         0:           no error
 *  @retval         -1:          header error
 */
static INT32 checkDluHeader(TYPE_DLU_HEADER *p_dlu)
{
    if (tdcStrCmp(DLU_PACKAGE_IDENT, (char *)p_dlu->packageIdent) == 0)
    {
        if (calcDLUHeaderCRC32(p_dlu) == (UINT32)tdcN2H32(p_dlu->headerCrc32))
        {
            return 0;
        }
    }
    return -1;
}

/*****************************************************************************
 *   GLOBAL FUNCTIONS
 */

#endif

/******************************************************************************/
/** Initialize the parser, skip possible dl2-header
 *
 *  The function intializes XML parser to use FILE stream pointed to by <in>.
 *
 *  @param[in]      in          FILE pointer
 *  @retval         0:          no error
 */
INT32 picoxml_init(T_FILE *in)
{

#ifndef VXWORKS
    TYPE_DLU_HEADER dluHead;

    /*  Check if this is a DL2 file:   */
    fseek(in, 0L, SEEK_SET);
    if ( (tdcFread(&dluHead, 1, sizeof(dluHead), in) != sizeof(dluHead))
         || (checkDluHeader(&dluHead) != 0) )
    {
        /*  No, this is no DL2, reset to SOF */
        fseek(in, 0L, SEEK_SET);
    }
    else
    {
        fseek(in, (INT32)tdcN2H32(dluHead.headerLength), SEEK_SET);
    }
#endif

    infile         = in;
    tag_depth      = 0;
    tag_depth_seek = 0;
    return 0;
}

/******************************************************************************/
/** Return the next tag
 *
 *  The function "seeks" to the next XML start tag on the current nesting level.
 *  The user supplied <tag> buffer is filled with the XML start tag, 
 *  it's size is limited by <maxlen> (including final \000 termination
 *  character)
 *
 *  @param[in,out]      tag         String of keyword
 *  @param[in]          maxlen      max length of string buffer
 *  @retval             0:          start tag found. <tag> is valid
 *  @retval             -1:         no further tags on this nesting level
 *  @retval             -2:         fatal error (e.g. unexpected end of file)
 */
INT32 picoxml_seek_any(	char    *tag,
                        UINT32  maxlen)
{
    PICOXML_TOKEN_T token = TOK_OPEN;

#ifdef DEBUG
    printf("+++ seek_any seek/current %d/%d\n", tag_depth_seek, tag_depth);
#endif

    while (tag_depth > tag_depth_seek)
    {
        token = next_token_hl();
        if (token == TOK_EOF)
        {
            break;
        }
        
        if (token == TOK_TAG)
        {
            if (token_value[0] == '/')
            {
                tag_depth--;
            }
            else if (token_value[tdcStrLen(token_value) - 1] != '/')
            {
                tag_depth++;
            }
        }
    }

    if (token == TOK_EOF)
    {
        return -1;
    }
    token = next_token_hl();

    if (token == TOK_TAG)
    {
        if (token_value[0] != '/')
        {
            tag_depth++;
            tdcStrNCpy(tag, token_tag, maxlen);
            if (token_value[tdcStrLen(token_value) - 1 ] == '/')
            {
                tag[tdcStrLen(tag) - 1] = 0;
                my_ungetc('>');
                my_ungetc('/');
                my_ungetc('<');
            }
 #ifdef DEBUG
            printf("--- seek_any returns %d/%d/%s\n",
                   tag_depth_seek, tag_depth, token_tag);
 #endif
            return 0;
        }
        else
        {
            tag_depth--;
            return -2;
        }
    }
    return -1;
}

/******************************************************************************/
/** Search for a tag
 *
 *  The function "seeks" to the next XML start tag named <tag> on the current
 *  nesting level.
 *
 *  @param[in]      tag     String of keyword
 *  @retval         0:      tag  <tag> found
 *  @retval         -1:     no further tags name <tag> on this nesting level
 *  @retval         -2:     fatal error (e.g. unexpected end of file)
 */
INT32 picoxml_seek(const char *tag)
{
    INT32 rv;
    char    buf[PICOXML_MAX_TAGLEN + 1];

    do
    {
        rv = picoxml_seek_any(buf, PICOXML_MAX_TAGLEN);
    }
    while ( (rv == 0) && (tdcStrCmp(buf, tag) != 0) );
    return rv;
}

/******************************************************************************/
/** Enter into subordinate nesting level
 *
 *  The function "enters" into subordinate nesting level. 
 *  Further calls to picoxml_seek() and picoxml_seek_any() work
 *  on this nesting level.
 */
void picoxml_enter(void)
{
    tag_depth_seek++;
}

/******************************************************************************/
/** Leave the subordinate nesting level
 *
 *  The function "leaves" subordinate nesting level.
 *  Further calls to picoxml_seek() and picoxml_seek_any() work
 *  on this nesting level.
 */
void picoxml_leave(void)
{
    tag_depth_seek--;
}

/******************************************************************************/
/** Return token value
 *
 *  The function is called after picoxml_seek() and picoxml_seek_any()
 *  to retrieve payload <data> between XML start and end tags.
 *  <maxlen> specifies the length of the user supplied buffer (including
 *  the \000 ternination character).
 *  Data. If the payload contains "white space" multiple calls
 *  to picoxml_data() are required to retrieve individual items.
 *
 *  @param[out]     data        Pointer to string buffer
 *  @param[in]      maxlen      max length of string buffer
 *  @retval         0:          no error, <data> available..
 *  @retval         -1:         fatal error (e.g. unexpected end of file)
 *  @retval         -3:         Open tag. Indicates: not at "leaf" level.
 *                              Do a enter/seek combination and try again.
 *                              <data> is not updated
 */
INT32 picoxml_data(	char    *data,
					UINT32  maxlen)
{
    PICOXML_TOKEN_T token;
    
    token = next_token();
    
    if (token == TOK_ID)
    {
        tdcStrNCpy(data, token_value, maxlen - 1);
        return 0;
    }
    else if (token == TOK_OPEN)
    {
        my_ungetc('<');
        return -3;
    }
    else
    {
        return -1;
    }
}
