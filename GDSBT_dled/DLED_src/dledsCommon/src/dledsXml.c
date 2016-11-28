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
#ifdef WIN32
#include <windows.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "dleds.h"
#include "dledsXml.h"


/*******************************************************************************
 * DEFINES
 */

/* Error code: type definitions */
#define TYPE_WARNING    0x40000000
#define TYPE_ERROR      0x80000000
#define TYPE_EXCEPTION  0xc0000000

/* Error code: IPTCom function definitions */
#define ERR_IPTVCOM     0x00030000
#define IPTCOM_COINST   0
#define IPTCOM_COID     3
#define ERR_TDCCOM      0x00040000

/* Error code: global/local definitions */
#define ERR_LOCAL       0x00008000

#define isException(res)  ((res & TYPE_EXCEPTION) == TYPE_EXCEPTION)


/* XML parser declarations */
#define MAX_SRC_FILTER 8      /* Max number of PD receive source filters per comid */
#define MAX_URI_LEN    101    /* Max length of a URI string */




/*******************************************************************************
 * TYPEDEFS
 */

 
/* Tokens 
 * TOK_OPEN          "<" character
 * TOK_CLOSE         ">" character
 * TOK_OPEN_END      "</" characters. Open for an end tag.
 * TOK_CLOSE_EMPTY   "/> characters. Close for an empty element.
 * TOK_ID            "Identifier". Identifiers are character sequences
 *                                 limited by whitespace characters or
 *                                 special characters ("<>/=").
 *                                 Identifiers in quotes (") may contain
 *                                 whitespace and special characters. 
 *                                 The id is put in tokenValue.
 * TOK_EQUAL         "=" character
 * TOK_EOF           End of file
 * TOK_START_TAG     TOK_OPEN + TOK_ID
 * TOK_END_TAG       TOK_OPEN_END + TOK_ID + TOK_CLOSE
 */
typedef enum {
    XML_TOK_OPEN = 0,
    XML_TOK_CLOSE,
    XML_TOK_OPEN_END,
    XML_TOK_CLOSE_EMPTY,
    XML_TOK_EQUAL,
    XML_TOK_ID,
    XML_TOK_EOF, 
    XML_TOK_START_TAG,
    XML_TOK_END_TAG,
    XML_TOK_ATTRIBUTE,
    XML_TOK_CDATA_OPEN,
    XML_TOK_CDATA_CLOSE,
    XML_TOK_ERROR
} XML_TOKENS;

typedef struct XML_LOCAL_STR
{
    FILE  *infile;
    char   token_value[XML_MAX_TOKLEN+1];
    UINT16 tag_depth;
    UINT16 tag_depth_seek;
    char   token_tag[XML_MAX_TAGLEN+1];
} XML_LOCAL;



/*******************************************************************************
 * LOCAL FUNCTION DECLARATIONS
 */
static INT16 xml_next_token(
    XML_LOCAL  *p_loc,                   /* Pointer to local data */
    XML_TOKENS *p_token);                /* Out: Resulting token */

static INT16 xml_open(
    XML_LOCAL  *p_loc,                   /* Pointer to local data */
    const char *file_name);              /* Name of file to open */

static INT16 xml_close(
    const XML_LOCAL *p_loc);             /* Pointer to local data */

static INT16 xml_seek_start_tag_any(
    XML_LOCAL  *p_loc,                   /* Pointer to local data */
    unsigned    maxlen,                  /* Length of buffer */
    char       *buffer,                  /* Out: Buffer for found tag */
    XML_TOKENS *p_token);                /* Out: The found token */

static INT16 xml_seek_start_tag(
    XML_LOCAL  *p_loc,                   /* Pointer to local data */
    const char *tag);                    /* Tag to be found */

static INT16 xml_enter(
    XML_LOCAL *p_loc);                   /* Pointer to local data */

static INT16 xml_leave(
    XML_LOCAL *p_loc);                   /* Pointer to local data */

static INT16 xml_get_attribute(
    XML_LOCAL  *p_loc,                   /* Pointer to local data */
    XML_TOKENS *p_token,                 /* Out: Found token */
    char       *p_attribute,             /* Out: Pointer to found attribute */
    unsigned    att_len,                 /* IN: Len of p_attribute buffer */
    char       *p_value,                 /* Out: Resulting string value */
    unsigned    val_len);                /* IN: Len of p_value buffer */


static INT16 xml_get_identifier(
    XML_LOCAL  *p_loc,                   /* Pointer to local data */
    XML_TOKENS *p_token,                 /* Out: Found token */
    char       *p_id);                   /* Out: Resulting string value */



/*******************************************************************************
 * LOCAL VARIABLES
 */



/*******************************************************************************
 * GLOBAL VARIABLES
 */



/*******************************************************************************
 * LOCAL FUNCTION DEFINITIONS
 */

/*******************************************************************************
 * 
 * Function name: <function>
 *
 * Abstract:      <description>
 *
 * Return value:  <description>
 *
 * Globals:       <list of used global variables>
 */
/* Parses the next XML token.
 * Skips occurences of whitespace and <?...>
 * Sets p_token to one of XML_TOK_OPEN ("<"),
 *     XML_TOK_CLOSE (">"), XML_TOK_OPEN_END = ("</"),
 *     XML_TOK_CLOSE_EMPTY = ("/>"), XML_TOK_EQUAL = ("="),
 *     XML_TOK_CDATA_OPEN = ("<![CDATA["), XML_TOK_CDATA_CLOSE = ("]]>"),
 *     XML_TOK_ID or XML_TOK_EOF
 * <!--   --> are skipped (treated as whitespace).
 */
/*lint -save -e740 -e611 */
/* Generated by feof() */
static INT16 xml_next_token(
    XML_LOCAL  *p_loc,                            /* Pointer to local data */
    XML_TOKENS *p_token)                          /* Out: Resulting token */
{
    INT16  ret = XML_ERROR;
    int    ch = 0;
    char  *p;
    int   loop = TRUE;

    if (p_loc && p_token)
    {
        while( loop )
        {
            /* Skip whitespace */
            ch = fgetc( p_loc->infile );
            while ((!feof( p_loc->infile )) &&
                   (isspace( ch )))
            {
                ch = fgetc( p_loc->infile );
            }
      
            /* Check for EOF */
            if (feof( p_loc->infile ))
            {
                *p_token = XML_TOK_EOF;
                ret = XML_OK;
                loop = FALSE;
            }
            else if (ch == '"') 
            {
                /* Handle quoted identifiers */
                p = p_loc->token_value;
                ch = fgetc( p_loc->infile );
                while ((!feof( p_loc->infile )) &&
                       (ch != '"'))
                {
                    if (p < p_loc->token_value + XML_MAX_TOKLEN)
                    {
                        *(p++) = (char)ch;
                    }
                    ch = fgetc( p_loc->infile );
                }
         
                *(p++) = 0;
                *p_token = XML_TOK_ID;
                ret = XML_OK;
                loop = FALSE;
            }  
            else if (ch == '<') 
            {
                /* Tag start character */
                ch = fgetc( p_loc->infile );
                if (ch == '?')
                {
                    while ((!feof( p_loc->infile )) &&
                           (ch != '>'))
                    {
                        ch = fgetc(p_loc->infile);
                    }
                    /* Do another round in the outer while loop */
                }
                else if (ch == '!')
                {
                    /*
                      May be either a comment (if it starts with "--")
                      or a CDATA element (if it starts with "[CDATA[")
                    */
                    if (!feof( p_loc->infile ))
                    {
                        ch = fgetc(p_loc->infile);
                        if (ch == '-')
                        {
                            /* Must be comment - skip it */
                            if (!feof( p_loc->infile ))
                            {
                                ch = fgetc(p_loc->infile);
                                if (ch == '-')
                                {
                                    while ((!feof(p_loc->infile))       &&
                                           ((ch = fgetc(p_loc->infile)) != '>'))
                                    {
                                        /* Inside comment - just skip it */
                                    }
                                    loop = TRUE; /* End of comment - try again */
                                }
                                else
                                {
                                    *p_token = XML_TOK_ERROR;
                                    loop = FALSE;
                                }
                            }
                            else
                            {
                                *p_token = XML_TOK_EOF;
                                ret = XML_OK;
                                loop = FALSE;
                            }
                        }
                        else if (ch == '[')
                        {
                            /* Check and handle CDATA only */
                            /* <![CDATA[<id>]]> ->  XML_TOK_ID. */
                            if ((!feof( p_loc->infile ))             &&
                                ((ch = fgetc(p_loc->infile)) == 'C') &&
                                (!feof( p_loc->infile ))             &&
                                ((ch = fgetc(p_loc->infile)) == 'D') &&
                                (!feof( p_loc->infile ))             &&
                                ((ch = fgetc(p_loc->infile)) == 'A') &&
                                (!feof( p_loc->infile ))             &&
                                ((ch = fgetc(p_loc->infile)) == 'T') &&
                                (!feof( p_loc->infile ))             &&
                                ((ch = fgetc(p_loc->infile)) == 'A') &&
                                (!feof( p_loc->infile ))             &&
                                ((ch = fgetc(p_loc->infile)) == '['))
                            {
                                *p_token = XML_TOK_CDATA_OPEN;
                                ret = XML_OK;
                            }
                            else
                            {
                                /* Not <![CDATA */
                                *p_token = XML_TOK_ERROR;
                            }
                            loop = FALSE;
                        }
                        else
                        {
                            /* Not <!- and not <![ */
                            *p_token = XML_TOK_ERROR;
                            loop = FALSE;
                        }
                    }
                }
                else if (ch == '/')
                {
                    *p_token = XML_TOK_OPEN_END;
                    ret = XML_OK;
                    loop = FALSE;
                }
                else 
                {
                    (void)ungetc( ch, p_loc->infile );
                    *p_token = XML_TOK_OPEN;
                    ret = XML_OK;
                    loop = FALSE;
                }
            }
            else if (ch == '/') 
            {
                ch = fgetc(p_loc->infile);
                if (ch == '>') 
                {
                    *p_token = XML_TOK_CLOSE_EMPTY;
                    ret = XML_OK;
                }
                else
                {
                    *p_token = XML_TOK_ERROR;
                    (void)ungetc(ch, p_loc->infile);
                }
                loop = FALSE;
            }
            else if (ch == '>') 
            {
                *p_token = XML_TOK_CLOSE;
                ret = XML_OK;
                loop = FALSE;
            }
            else if (ch == '=') 
            {
                *p_token = XML_TOK_EQUAL;
                ret = XML_OK;
                loop = FALSE;
            }
            else if (ch == ']')
            {
                if ((!feof( p_loc->infile ))             &&
                    ((ch = fgetc(p_loc->infile)) == ']') &&
                    (!feof( p_loc->infile ))             &&
                    ((ch = fgetc(p_loc->infile)) == '>'))
                {
                    /* We have found the CDEnd token */
                    *p_token = XML_TOK_CDATA_CLOSE;
                    ret = XML_OK;
                }
                else
                {
                    *p_token = XML_TOK_ERROR;
                }
                loop = FALSE;
            }
            else 
            {
                /* Unquoted identifier */ 
                p = p_loc->token_value;
                *(p++) = (char)ch;
                if (!feof(p_loc->infile))
                {
                    ch = fgetc(p_loc->infile);
                    while ((!feof(p_loc->infile)) && 
                           (ch != '<')            &&
                           (ch != '>')            &&
                           (ch != '=')            &&
                           (ch != '/')            &&
                           (ch != ']')            &&
                           (!isspace(ch))         &&
                           (p < p_loc->token_value + XML_MAX_TOKLEN - 2))
                    {
                        *(p++) = (char)ch;
                        ch = fgetc(p_loc->infile);
                    }
                }
                *(p++) = 0;                           /* Terminate */
         
                if ((ch == '<') ||
                    (ch == '>') ||
                    (ch == '=') ||
                    (ch == '/') ||
                    (ch == ']') ||
                    isspace(ch))
                {
                    (void)ungetc( ch, p_loc->infile );
                    *p_token = XML_TOK_ID;
                    ret = XML_OK;
                }
                else
                {
                    /* Too long token */
                    *p_token = XML_TOK_ERROR;
                }
                loop = FALSE;
            }
        }
    }   
    else if (p_token)
    {
        *p_token = XML_TOK_ERROR;
    }
    return ret;
}   
/*lint -restore */


/* Parses the next XML token.
 * Skips occurences of whitespace and <?...>
 * Sets p_token to one of XML_TOK_OPEN ("<"),
 *     XML_TOK_CLOSE (">"), XML_TOK_OPEN_END = ("</"),
 *     XML_TOK_CLOSE_EMPTY = ("/>"), XML_TOK_EQUAL = ("="),
 *     XML_TOK_CDATA_OPEN = ("<![CDATA["), XML_TOK_CDATA_CLOSE = ("]]>"),
 *     XML_TOK_ID or XML_TOK_EOF
 * <!--   --> are skipped (treated as whitespace).
 */
/*lint -save -e740 -e611 */
/* Generated by feof() */
static INT16 xml_next_token_id(
    XML_LOCAL  *p_loc,                            /* Pointer to local data */
    XML_TOKENS *p_token)                          /* Out: Resulting token */
{
    INT16  ret = XML_ERROR;
    int    ch = 0;
    char  *p;
    int   loop = TRUE;

    if (p_loc && p_token)
    {
        while( loop )
        {
            /* Skip whitespace */
            ch = fgetc( p_loc->infile );
            while ((!feof( p_loc->infile )) &&
                   (isspace( ch )))
            {
                ch = fgetc( p_loc->infile );
            }
      
            /* Check for EOF */
            if (feof( p_loc->infile ))
            {
                *p_token = XML_TOK_EOF;
                ret = XML_OK;
                loop = FALSE;
            }
            else if (ch == '"') 
            {
                /* Handle quoted identifiers */
                p = p_loc->token_value;
                ch = fgetc( p_loc->infile );
                while ((!feof( p_loc->infile )) &&
                       (ch != '"'))
                {
                    if (p < p_loc->token_value + XML_MAX_TOKLEN)
                    {
                        *(p++) = (char)ch;
                    }
                    ch = fgetc( p_loc->infile );
                }
         
                *(p++) = 0;
                *p_token = XML_TOK_ID;
                ret = XML_OK;
                loop = FALSE;
            }  
            else if (ch == '<') 
            {
                /* Tag start character */
                ch = fgetc( p_loc->infile );
                if (ch == '?')
                {
                    while ((!feof( p_loc->infile )) &&
                           (ch != '>'))
                    {
                        ch = fgetc(p_loc->infile);
                    }
                    /* Do another round in the outer while loop */
                }
                else if (ch == '!')
                {
                    /*
                      May be either a comment (if it starts with "--")
                      or a CDATA element (if it starts with "[CDATA[")
                    */
                    if (!feof( p_loc->infile ))
                    {
                        ch = fgetc(p_loc->infile);
                        if (ch == '-')
                        {
                            /* Must be comment - skip it */
                            if (!feof( p_loc->infile ))
                            {
                                ch = fgetc(p_loc->infile);
                                if (ch == '-')
                                {
                                    while ((!feof(p_loc->infile))       &&
                                           ((ch = fgetc(p_loc->infile)) != '>'))
                                    {
                                        /* Inside comment - just skip it */
                                    }
                                    loop = TRUE; /* End of comment - try again */
                                }
                                else
                                {
                                    *p_token = XML_TOK_ERROR;
                                    loop = FALSE;
                                }
                            }
                            else
                            {
                                *p_token = XML_TOK_EOF;
                                ret = XML_OK;
                                loop = FALSE;
                            }
                        }
                        else if (ch == '[')
                        {
                            /* Check and handle CDATA only */
                            /* <![CDATA[<id>]]> ->  XML_TOK_ID. */
                            if ((!feof( p_loc->infile ))             &&
                                ((ch = fgetc(p_loc->infile)) == 'C') &&
                                (!feof( p_loc->infile ))             &&
                                ((ch = fgetc(p_loc->infile)) == 'D') &&
                                (!feof( p_loc->infile ))             &&
                                ((ch = fgetc(p_loc->infile)) == 'A') &&
                                (!feof( p_loc->infile ))             &&
                                ((ch = fgetc(p_loc->infile)) == 'T') &&
                                (!feof( p_loc->infile ))             &&
                                ((ch = fgetc(p_loc->infile)) == 'A') &&
                                (!feof( p_loc->infile ))             &&
                                ((ch = fgetc(p_loc->infile)) == '['))
                            {
                                *p_token = XML_TOK_CDATA_OPEN;
                                ret = XML_OK;
                            }
                            else
                            {
                                /* Not <![CDATA */
                                *p_token = XML_TOK_ERROR;
                            }
                            loop = FALSE;
                        }
                        else
                        {
                            /* Not <!- and not <![ */
                            *p_token = XML_TOK_ERROR;
                            loop = FALSE;
                        }
                    }
                }
                else if (ch == '/')
                {
                    *p_token = XML_TOK_OPEN_END;
                    ret = XML_OK;
                    loop = FALSE;
                }
                else 
                {
                    (void)ungetc( ch, p_loc->infile );
                    *p_token = XML_TOK_OPEN;
                    ret = XML_OK;
                    loop = FALSE;
                }
            }
            else if (ch == '/') 
            {
                ch = fgetc(p_loc->infile);
                if (ch == '>') 
                {
                    *p_token = XML_TOK_CLOSE_EMPTY;
                    ret = XML_OK;
                }
                else
                {
                    *p_token = XML_TOK_ERROR;
                    (void)ungetc(ch, p_loc->infile);
                }
                loop = FALSE;
            }
            else if (ch == '>') 
            {
                *p_token = XML_TOK_CLOSE;
                ret = XML_OK;
                loop = FALSE;
            }
            else if (ch == '=') 
            {
                *p_token = XML_TOK_EQUAL;
                ret = XML_OK;
                loop = FALSE;
            }
            else if (ch == ']')
            {
                if ((!feof( p_loc->infile ))             &&
                    ((ch = fgetc(p_loc->infile)) == ']') &&
                    (!feof( p_loc->infile ))             &&
                    ((ch = fgetc(p_loc->infile)) == '>'))
                {
                    /* We have found the CDEnd token */
                    *p_token = XML_TOK_CDATA_CLOSE;
                    ret = XML_OK;
                }
                else
                {
                    *p_token = XML_TOK_ERROR;
                }
                loop = FALSE;
            }
            else 
            {
                /* Unquoted identifier */ 
                p = p_loc->token_value;
                *(p++) = (char)ch;
                if (!feof(p_loc->infile))
                {
                    ch = fgetc(p_loc->infile);
                    while ((!feof(p_loc->infile)) && 
                           (ch != '<')            &&
                           (ch != '>')            &&
                           (ch != '=')            &&
                           (ch != '/')            &&
                           (ch != ']')            &&
                           /*(!isspace(ch))         &&*/
                           (p < p_loc->token_value + XML_MAX_TOKLEN - 2))
                    {
                        *(p++) = (char)ch;
                        ch = fgetc(p_loc->infile);
                    }
                }
                *(p++) = 0;                           /* Terminate */
         
                if ((ch == '<') ||
                    (ch == '>') ||
                    (ch == '=') ||
                    (ch == '/') ||
                    (ch == ']') ||
                    isspace(ch))
                {
                    (void)ungetc( ch, p_loc->infile );
                    *p_token = XML_TOK_ID;
                    ret = XML_OK;
                }
                else
                {
                    /* Too long token */
                    *p_token = XML_TOK_ERROR;
                }
                loop = FALSE;
            }
        }
    }   
    else if (p_token)
    {
        *p_token = XML_TOK_ERROR;
    }
    return ret;
}   
/*lint -restore */



/* Returns next high level XML token. 
 * Sets *p_token to one of:
 *    XML_TOK_START_TAG = XML_TOK_OPEN + XML_TOK_ID or
 *    XML_TOK_END_TAG = XML_TOK_OPEN_END + XML_TOK_ID + XML_TOK_CLOSE or
 *    Other tokens are returned as is.
 * Any Id is stored in p_loc->token_value
 */
static INT16 xml_next_token_hl(
    XML_LOCAL  *p_loc,                            /* Pointer to local data */
    XML_TOKENS *p_token)                          /* Out: Resulting token */
{
    INT16 ret = XML_ERROR;

    if (p_loc && p_token)
    {
        if (xml_next_token( p_loc, p_token ) == XML_OK)
        {
            if (*p_token == XML_TOK_OPEN)                 /* "<" */
            {
                if (p_loc->tag_depth < XML_MAX_LEVEL)
                {
                    p_loc->tag_depth++;
                }
                else
                {
                    /* nothing */
                }
                if (xml_next_token( p_loc, p_token ) == XML_OK)
                {
                    if (*p_token == XML_TOK_ID) 
                    {
                        (void)strncpy( p_loc->token_tag, p_loc->token_value, XML_MAX_TAGLEN );
                        *p_token = XML_TOK_START_TAG;       /* XML_TOK_OPEN + XML_TOK_ID */
                        ret = XML_OK;
                    }
                    else
                    {
                        /* Something wrong, < should always be followed by a tag id */
                        *p_token = XML_TOK_EOF;
                    }
                }
                else
                {
                    *p_token = XML_TOK_ERROR;
                }
            }
            else if (*p_token == XML_TOK_OPEN_END)        /* "</" */
            {
                if (p_loc->tag_depth > 0)
                {
                    p_loc->tag_depth--;
                }
                else
                {
                    /* nothing */
                }
                if (xml_next_token( p_loc, p_token ) == XML_OK)
                {
                    if (*p_token == XML_TOK_ID) 
                    {
                        (void)strncpy( p_loc->token_tag, p_loc->token_value, XML_MAX_TAGLEN );
                        /* XML_TOK_OPEN_END + XML_TOK_ID + XML_TOK_CLOSE */
                        *p_token = XML_TOK_END_TAG;
                        ret = XML_OK;
                    }
                    else
                    {
                        /* Something wrong, </ should always be followed by a tag id + ">"*/
                        *p_token = XML_TOK_EOF;
                    }
                }
                else
                {
                    *p_token = XML_TOK_ERROR;
                }
            }
            else if (*p_token == XML_TOK_CLOSE_EMPTY)     /* "/>" */
            {
                if (p_loc->tag_depth > 0)
                {
                    p_loc->tag_depth--;
                    ret = XML_OK;
                }
                else
                {
                    /* nothing */
                }
            }
            else if (*p_token == XML_TOK_CDATA_OPEN)      /* "<![CDATA[" */
            {
                if (xml_next_token( p_loc, p_token ) == XML_OK)
                {
                    if (*p_token == XML_TOK_ID) 
                    {
                        (void)strncpy( p_loc->token_tag, p_loc->token_value, XML_MAX_TAGLEN );
                        if (xml_next_token( p_loc, p_token ) == XML_OK)
                        {
                            if (*p_token == XML_TOK_CDATA_CLOSE)
                            {
                                *p_token = XML_TOK_ID; /* XML_TOK_CDATA_OPEN + XML_TOK_ID */
                                ret = XML_OK;
                            }
                            else
                            {
                                *p_token = XML_TOK_ERROR;
                            }
                        }
                        else
                        {
                            *p_token = XML_TOK_ERROR;
                        }
                    }
                    else
                    {
                        /* Something wrong, < should always be followed by a tag id */
                        *p_token = XML_TOK_EOF;
                    }
                }
                else
                {
                    *p_token = XML_TOK_ERROR;
                }
            }
            else if (*p_token == XML_TOK_ID)
            {
                (void)strncpy( p_loc->token_tag, p_loc->token_value, XML_MAX_TAGLEN );
                *p_token = XML_TOK_ID;
                ret = XML_OK;
            }
            else
            {
                ret = XML_OK;
            }
        }
        else
        {
            *p_token = XML_TOK_ERROR;
        }
    }
    else if (p_token)
    {
        *p_token = XML_TOK_ERROR;
    }
    else
    {
        /* nothing */
    }
    return ret;
}

/* Returns next high level XML token. 
 * Sets *p_token to one of:
 *    XML_TOK_START_TAG = XML_TOK_OPEN + XML_TOK_ID or
 *    XML_TOK_END_TAG = XML_TOK_OPEN_END + XML_TOK_ID + XML_TOK_CLOSE or
 *    Other tokens are returned as is.
 * Any Id is stored in p_loc->token_value
 */
static INT16 xml_next_token_hl_id(
    XML_LOCAL  *p_loc,                            /* Pointer to local data */
    XML_TOKENS *p_token)                          /* Out: Resulting token */
{
    INT16 ret = XML_ERROR;

    if (p_loc && p_token)
    {
        if (xml_next_token_id( p_loc, p_token ) == XML_OK)
        {
            if (*p_token == XML_TOK_OPEN)                 /* "<" */
            {
                if (p_loc->tag_depth < XML_MAX_LEVEL)
                {
                    p_loc->tag_depth++;
                }
                else
                {
                    /* nothing */
                }
                if (xml_next_token_id( p_loc, p_token ) == XML_OK)
                {
                    if (*p_token == XML_TOK_ID) 
                    {
                        (void)strncpy( p_loc->token_tag, p_loc->token_value, XML_MAX_TAGLEN );
                        *p_token = XML_TOK_START_TAG;       /* XML_TOK_OPEN + XML_TOK_ID */
                        ret = XML_OK;
                    }
                    else
                    {
                        /* Something wrong, < should always be followed by a tag id */
                        *p_token = XML_TOK_EOF;
                    }
                }
                else
                {
                    *p_token = XML_TOK_ERROR;
                }
            }
            else if (*p_token == XML_TOK_OPEN_END)        /* "</" */
            {
                if (p_loc->tag_depth > 0)
                {
                    p_loc->tag_depth--;
                }
                else
                {
                    /* nothing */
                }
                if (xml_next_token_id( p_loc, p_token ) == XML_OK)
                {
                    if (*p_token == XML_TOK_ID) 
                    {
                        (void)strncpy( p_loc->token_tag, p_loc->token_value, XML_MAX_TAGLEN );
                        /* XML_TOK_OPEN_END + XML_TOK_ID + XML_TOK_CLOSE */
                        *p_token = XML_TOK_END_TAG;
                        ret = XML_OK;
                    }
                    else
                    {
                        /* Something wrong, </ should always be followed by a tag id + ">"*/
                        *p_token = XML_TOK_EOF;
                    }
                }
                else
                {
                    *p_token = XML_TOK_ERROR;
                }
            }
            else if (*p_token == XML_TOK_CLOSE_EMPTY)     /* "/>" */
            {
                if (p_loc->tag_depth > 0)
                {
                    p_loc->tag_depth--;
                    ret = XML_OK;
                }
                else
                {
                    /* nothing */
                }
            }
            else if (*p_token == XML_TOK_CDATA_OPEN)      /* "<![CDATA[" */
            {
                if (xml_next_token_id( p_loc, p_token ) == XML_OK)
                {
                    if (*p_token == XML_TOK_ID) 
                    {
                        (void)strncpy( p_loc->token_tag, p_loc->token_value, XML_MAX_TAGLEN );
                        if (xml_next_token_id( p_loc, p_token ) == XML_OK)
                        {
                            if (*p_token == XML_TOK_CDATA_CLOSE)
                            {
                                *p_token = XML_TOK_ID; /* XML_TOK_CDATA_OPEN + XML_TOK_ID */
                                ret = XML_OK;
                            }
                            else
                            {
                                *p_token = XML_TOK_ERROR;
                            }
                        }
                        else
                        {
                            *p_token = XML_TOK_ERROR;
                        }
                    }
                    else
                    {
                        /* Something wrong, < should always be followed by a tag id */
                        *p_token = XML_TOK_EOF;
                    }
                }
                else
                {
                    *p_token = XML_TOK_ERROR;
                }
            }
            else if (*p_token == XML_TOK_ID)
            {
                (void)strncpy( p_loc->token_tag, p_loc->token_value, XML_MAX_TAGLEN );
                *p_token = XML_TOK_ID;
                ret = XML_OK;
            }
            else
            {
                ret = XML_OK;
            }
        }
        else
        {
            *p_token = XML_TOK_ERROR;
        }
    }
    else if (p_token)
    {
        *p_token = XML_TOK_ERROR;
    }
    else
    {
        /* nothing */
    }
    return ret;
}



/*******************************************************************************
 * 
 * Function name: <file_function>
 *
 * Abstract:      <description>
 *
 * Return value:  <description>
 *
 * Globals:       <list of used global variables>
 */
/* Opens the XML parsíng. Opens the XML file.
 * Returns OK or
 * ERROR if file could not be opened for reading.
 */
static INT16 xml_open(
    XML_LOCAL  *p_loc,                            /* Pointer to local data */
    const char *file_name)                        /* Name of file to open */
{
    INT16      ret = XML_ERROR;

    if (p_loc && file_name)
    {
        p_loc->tag_depth = 0;
        p_loc->tag_depth_seek = 0;

        p_loc->infile = fopen( file_name, "r" );
        if (p_loc->infile)
        {
            ret = XML_OK;
        }
    }
    return ret;
}

/* Closes the XML parsíng. Closes the XML file.
 */
static INT16 xml_close(
    const XML_LOCAL *p_loc)                       /* Pointer to local data */
{
    INT16 ret = XML_ERROR;

    if (p_loc)
    {
        if (fclose( p_loc->infile ) == OK)
        {
            ret = XML_OK;
        }
    }
    return ret;
}

/* Finds the next tag at current depth and returns it in the provided buffer.
 * Start tags on deeper depths are ignored.
 * Parameter pBuffer should be at least XML_MAX_TAGLEN chars long.
 * Returns OK if a tag is found or
 * ERROR if end of XML file or end of current depth.
 */
static INT16 xml_seek_start_tag_any(
    XML_LOCAL  *p_loc,                            /* Pointer to local data */
    unsigned    maxlen,                           /* Length of buffer */
    char       *buffer,                           /* Out: Buffer for found tag */
    XML_TOKENS *p_token)                          /* Out: The found token */
{
    INT16      ret = XML_ERROR;
    int       done = FALSE;
   
    if (p_loc && buffer && p_token)
    {
        while (done == FALSE)
        {
            if (xml_next_token_hl( p_loc, p_token ) == XML_OK)
            {
                if (*p_token == XML_TOK_EOF)
                {
                    ret = XML_ERROR;                          /* End of file, interrupt */
                    done = TRUE;
                }
                else if (*p_token == XML_TOK_ERROR)
                {
                    ret = XML_ERROR;                          /* Too long token string */
                    done = TRUE;
                }
                else if (p_loc->tag_depth < (p_loc->tag_depth_seek - 1))
                {
                    ret = XML_ERROR;         /* No more tokens on this depth, interrupt */
                    done = TRUE;
                }
                else if ((p_loc->tag_depth == p_loc->tag_depth_seek) &&
                         (*p_token == XML_TOK_START_TAG))
                {
                    /* We are on the correct depth and have found a start tag */
                    (void)strncpy( buffer, p_loc->token_tag, maxlen );
                    ret = XML_OK;
                    done = TRUE;
                }
                else
                {
                    /* ignore */
                }
            }
            else
            {
                /* Error getting next token */
                done = TRUE;
            }
        }
    }
    return ret;
}

/* Skips all XML until the specific tag is found.
 * Returns OK if the specified tag is found or
 * ERROR if end of XML file or end of current depth.
 */
static INT16 xml_seek_start_tag(
    XML_LOCAL  *p_loc,                            /* Pointer to local data */
    const char *tag)                              /* Tag to be found */
{
    INT16      ret = XML_ERROR;
    char       buf[XML_MAX_TAGLEN + 1];
    XML_TOKENS token;
   
    if (p_loc && tag)
    {
        do 
        {
            ret = xml_seek_start_tag_any( p_loc, XML_MAX_TAGLEN, buf, &token );
        } while ((ret == XML_OK) &&
                 (strcmp( buf, tag ) != 0));
    }   
    return ret;
}

/* Enter new depth in the XML file.
 */
static INT16 xml_enter(
    XML_LOCAL *p_loc)                             /* Pointer to local data */
{
    INT16 ret = XML_ERROR;

    if (p_loc)
    {
        if (p_loc->tag_depth_seek < XML_MAX_LEVEL)
        {
            p_loc->tag_depth_seek++;
            ret = XML_OK;
        }
        else
        {
            /* nothing */
        }
    }
    return ret;
}

/* Exits the current depth in the XML file.
 */
static INT16 xml_leave(
    XML_LOCAL *p_loc)                              /* Pointer to local data */
{
    INT16 ret = XML_ERROR;

    if (p_loc)
    {
        if (p_loc->tag_depth_seek > 0)
        {
            p_loc->tag_depth_seek--;
            ret = XML_OK;
        }
        else
        {
            /* nothing */
        }
    }
    return ret;
}


/* Get value of next attribute as a string.
 * Parameter p_attribute must point to a buffer at least XML_MAX_TOKLEN
 * chars long.
 * Parameter p_value must point to a buffer at least XML_MAX_TOKLEN
 * chars long.
 * Returns OK and sets p_attribute and p_value if an attribute is found or
 * ERROR if '=' or value is missing.
 */
static INT16 xml_get_attribute(
    XML_LOCAL  *p_loc,                   /* Pointer to local data */
    XML_TOKENS *p_token,                 /* Out: Found token */
    char       *p_attribute,             /* Out: Pointer to found attribute */
    unsigned    att_len,                 /* IN: Len of p_attribute buffer */
    char       *p_value,                 /* Out: Resulting string value */
    unsigned    val_len)                 /* IN: Len of p_value buffer */
{
    INT16 res = XML_ERROR;
   
    if (p_loc && p_token && p_attribute && p_value && att_len && val_len)
    {
        if (xml_next_token_hl( p_loc, p_token ) == XML_OK)
        {
            if (*p_token == XML_TOK_ID)
            {
                (void)strncpy( p_attribute, p_loc->token_value, att_len );
                p_attribute[att_len - 1] = '\0';

                if (xml_next_token_hl( p_loc, p_token ) == XML_OK)
                {
                    if (*p_token == XML_TOK_EQUAL)
                    {
                        if (xml_next_token_hl( p_loc, p_token ) == XML_OK)
                        {
                            if (*p_token == XML_TOK_ID)
                            {
                                (void)strncpy( p_value, p_loc->token_value, val_len );
                                p_value[val_len - 1] = '\0';
                                *p_token = XML_TOK_ATTRIBUTE;
                                res = XML_OK;
                            }
                            else
                            {
                                /* nothing */
                            }
                        }
                        else
                        {
                            /* nothing */
                        }
                    }
                    else
                    {
                        /* nothing */
                    }
                }
                else
                {
                    /* nothing */
                }
            }
            else
            {
                /* nothing */
            }
        }
        else
        {
            /* nothing */
        }
    }
    else
    {
        /* nothing */
    }
    return res;
}


/* If the next token is an identifier then return its value.
 * Always put the found token into *p_token.
 * This function parses the ID in:
 * <start> ID </start>
 *
 * Also handles if ID is a <[CDATA[ ID ]]> element
 *
 * Returns OK and sets p_value if an identifier is found or
 * ERROR if next token is not XML_TOK_ID
 */
static INT16 xml_get_identifier(
    XML_LOCAL  *p_loc,                            /* Pointer to local data */
    XML_TOKENS *p_token,                          /* Out: Found token */
    char       *p_id)                        /* Out: Resulting string value */
{
    INT16 res = XML_ERROR;

    if (p_loc && p_token && p_id)
    {
        if (xml_next_token_hl( p_loc, p_token ) == XML_OK)
        {
            if (*p_token == XML_TOK_ID)
            {
                (void)strncpy( p_id, p_loc->token_value, XML_MAX_TOKLEN );
                res = XML_OK;
            }
        }
    }
    return res;
}
/* If the next token is an identifier then return its value.
 * Always put the found token into *p_token.
 * This function parses the ID in:
 * <start> ID </start>
 *
 * Also handles if ID is a <[CDATA[ ID ]]> element
 *
 * Returns OK and sets p_value if an identifier is found or
 * ERROR if next token is not XML_TOK_ID
 */
static INT16 xml_get_identifier_id(
    XML_LOCAL  *p_loc,                            /* Pointer to local data */
    XML_TOKENS *p_token,                          /* Out: Found token */
    char       *p_id)                        /* Out: Resulting string value */
{
    INT16 res = XML_ERROR;

    if (p_loc && p_token && p_id)
    {
        if (xml_next_token_hl_id( p_loc, p_token ) == XML_OK)
        {
            if (*p_token == XML_TOK_ID)
            {
                (void)strncpy( p_id, p_loc->token_value, XML_MAX_TOKLEN );
                res = XML_OK;
            }
        }
    }
    return res;
}


/*******************************************************************************
 * GLOBAL FUNCTIONS
 */


/*******************************************************************************
 * 
 * Function name: readDataFromInstStructure
 *
 * Abstract:      This function reads the values for element tags
 *                <Level>, <ListTarFileName> and <ListDluFileName>.
 *
 * Return value:  DLEDS_OK      - Information from file has been read
 *                DLEDS_ERROR   - No information read from file
 *
 * Globals:       -
 */
int readDataFromInstStructure(
    char*           fileName,           /*  IN:  XML file */
    char*           pLevel,             /* OUT: "Function", "EDPackage" or "SCI" */
    UINT32*         pNoTarFiles,        /* OUT: Number of TAR files found */
    char**          pListTarFileName,   /* OUT: String with TAR file names separated with <space> */
    UINT32*         pNoDluFiles,        /* OUT: Number of DLU files found */
    char**          pListDluFileName)   /* OUT: String with DLU file names separated with <space> */
{
    XML_LOCAL xLoc;    /* Local data */
    XML_TOKENS xTok;
    char value[XML_MAX_TOKLEN+1];
    char* pTmpBuf = NULL;
    UINT32 noFiles = 0;

    if (xml_open(&xLoc,fileName) < 0)
    {
        /* Error */
        return DLEDS_ERROR;
    }
    xml_enter(&xLoc);
    if (xml_seek_start_tag(&xLoc,"INST_Structure") == XML_OK)
    {
        xml_enter(&xLoc);
        if (xml_seek_start_tag(&xLoc,"Level") == XML_OK)
        {
            (void)xml_next_token(&xLoc, &xTok);
            /* Only attribute should be returned. No value assigned to attribute Level */
            if (xml_get_identifier(&xLoc, &xTok, value) == XML_ERROR)
            {
                /* Error */
                xml_close(&xLoc);
                return DLEDS_ERROR;
            }
            /* Return attribute for element tag <Level> */
            strcpy(pLevel,value);

            /* Check for TAR files */
            if (xml_seek_start_tag(&xLoc,"ListTarFileName") == XML_OK)
            {
                xml_enter(&xLoc);
                noFiles = 0;
                while (xml_seek_start_tag(&xLoc,"Entry") == XML_OK)
                {
                    (void)xml_next_token(&xLoc, &xTok);
                    /* Only attribute should be returned. No value assigned to attribute Level */
                    if (xml_get_identifier(&xLoc, &xTok, value) == XML_ERROR)
                    {
                        /* Error */
                        xml_close(&xLoc);
                        return DLEDS_ERROR;
                    }
                    /* Copy data to tar file name list */
                    if (noFiles == 0)
                    {
                        /* First file name */
                        *pListTarFileName = (char*)malloc(strlen(value) + 1);
                        if (*pListTarFileName == NULL)
                        {
                            /* Error */
                            xml_close(&xLoc);
                            return DLEDS_ERROR;
                        }
                        pTmpBuf = *pListTarFileName;
                        strcpy(*pListTarFileName,value);
                        noFiles++;
                    }
                    else
                    {
                        *pListTarFileName = (char*)malloc(strlen(pTmpBuf) + strlen(value) + 2);
                        if (*pListTarFileName == NULL)
                        {
                            /* Error */
                            free(pTmpBuf);
                            xml_close(&xLoc);
                            return DLEDS_ERROR;
                        }
                        strncpy(*pListTarFileName, pTmpBuf, strlen(pTmpBuf) + 1);
                        strcat(*pListTarFileName,",");
                        strcat(*pListTarFileName,value);
                        free(pTmpBuf);
                        pTmpBuf = *pListTarFileName;
                        noFiles++;
                    }
                }
                pTmpBuf = NULL;
                *pNoTarFiles = noFiles;
                xml_leave(&xLoc);    
            }
            else
            {
                /* Error */
                xml_close(&xLoc);
                return DLEDS_ERROR;
            }
            /* Check for DLU files */
            if (xml_seek_start_tag(&xLoc,"ListDluFileName") == XML_OK)
            {
                xml_enter(&xLoc);
                noFiles = 0;
                while (xml_seek_start_tag(&xLoc,"Entry") == XML_OK)
                {
                    (void)xml_next_token(&xLoc, &xTok);
                    /* Only attribute should be returned. No value assigned to attribute Level */
                    if (xml_get_identifier(&xLoc, &xTok, value) == XML_ERROR)
                    {
                        /* Error */
                        xml_close(&xLoc);
                        return DLEDS_ERROR;
                    }
                    /* Return attribute for element tag <Level> */
                    /* Copy data to dlu file name list */
                    if (noFiles == 0)
                    {
                        /* First file name */
                        *pListDluFileName = (char*)malloc(strlen(value) + 1);
                        if (*pListDluFileName == NULL)
                        {
                            /* Error */
                            xml_close(&xLoc);
                            return DLEDS_ERROR;
                        }
                        pTmpBuf = *pListDluFileName;
                        strcpy(*pListDluFileName,value);
                        noFiles++;
                    }
                    else
                    {
                        *pListDluFileName = (char*)malloc(strlen(pTmpBuf) + strlen(value) + 2);
                        if (*pListDluFileName == NULL)
                        {
                            /* Error */
                            free(pTmpBuf);
                            xml_close(&xLoc);
                            return DLEDS_ERROR;
                        }
                        strncpy(*pListDluFileName, pTmpBuf, strlen(pTmpBuf) + 1);
                        strcat(*pListDluFileName,",");
                        strcat(*pListDluFileName,value);
                        free(pTmpBuf);
                        pTmpBuf = *pListDluFileName;
                        noFiles++;
                    }
                }
                pTmpBuf = NULL;
                *pNoDluFiles = noFiles;
                xml_leave(&xLoc);    
            }
            else
            {
                /* Error */
                xml_close(&xLoc);
                return DLEDS_ERROR;
            }
        } /* end <Level> */
        xml_leave(&xLoc);                
    } /* end <INST_structure> */
    xml_leave(&xLoc);        
    xml_close(&xLoc);
    return DLEDS_OK;
}


/*******************************************************************************
* 
* Function name: readDataFromInstEd
*
* Abstract:      This function reads the values for element tags
*                <Level>, <Source>, <ListSciFileName>, <ListMdluEdFileName>,
*                <ClearNVRAM> and <ClearFlashFileSystem>
*
* Return value:  DLEDS_OK      - Information from file has been read
*                DLEDS_ERROR   - No information read from file
*
* Globals:       -
*/
int readDataFromInstEd(
    char*           fileName,              /*  IN:  XML file */
    char*           pLevel,                /* OUT: "EDSubPackage" */
    char*           pSource,               /* OUT: "BT" or "CU" */
    UINT32*         pNoSciFiles,           /* OUT: Number of SCI files found */
    char**          pListSciFileName,      /* OUT: String with SCI file names separated with "," */
    UINT32*         pNoMdluEdFiles,        /* OUT: Number of MDLU ED files found */
    char**          pListMdluEdFileName,   /* OUT: String with MDLU ED file names separated with "," */
    int*            clearNVRAM,            /* OUT: True or False */
    int*            clearFFS)              /* OUT: True or False */
{
    int res = DLEDS_ERROR;
    XML_LOCAL xLoc;    /* Local data */
    XML_TOKENS xTok;
    char value[XML_MAX_TOKLEN+1];
    char* pTmpBuf = NULL;
    UINT32 noFiles = 0;

    if (xml_open(&xLoc,fileName) < 0)
    {
        return res;
    }
    xml_enter(&xLoc);
    if ((res = xml_seek_start_tag(&xLoc,"INST_ED")) == XML_OK)
    {
        xml_enter(&xLoc);
        if ((res = xml_seek_start_tag(&xLoc,"Level")) == XML_OK)
        {
            (void)xml_next_token(&xLoc, &xTok);
            /* Only attribute should be returned. No value assigned to attribute Level */
            if (xml_get_identifier(&xLoc, &xTok, value) == XML_ERROR)
            {
                /* Error */
                xml_close(&xLoc);
                return DLEDS_ERROR;
            }
            /* Return attribute for element tag <Level> */
            strcpy(pLevel,value);

            if ((res = xml_seek_start_tag(&xLoc,"Source")) == XML_OK)
            {
                (void)xml_next_token(&xLoc, &xTok);
                /* Only attribute should be returned. No value assigned to attribute Level */
                if (xml_get_identifier(&xLoc, &xTok, value) == XML_ERROR)
                {
                    /* Error */
                    xml_close(&xLoc);
                    return DLEDS_ERROR;
                }
                /* Return attribute for element tag <Source> */
                strcpy(pSource,value);
                /* Check for SCI files */
                if (xml_seek_start_tag(&xLoc,"ListSciFileName") == XML_OK)
                {
                    xml_enter(&xLoc);
                    noFiles = 0;
                    while (xml_seek_start_tag(&xLoc,"Entry") == XML_OK)
                    {
                        (void)xml_next_token(&xLoc, &xTok);
                        /* Only attribute should be returned. No value assigned to attribute Level */
                        if (xml_get_identifier(&xLoc, &xTok, value) == XML_ERROR)
                        {
                            /* Error */
                            xml_close(&xLoc);
                            return DLEDS_ERROR;
                        }
                        /* Copy data to sci file name list */
                        if (noFiles == 0)
                        {
                            /* First file name */
                            *pListSciFileName = (char*)malloc(strlen(value) + 1);
                            if (*pListSciFileName == NULL)
                            {
                                /* Error */
                                xml_close(&xLoc);
                                return DLEDS_ERROR;
                            }
                            pTmpBuf = *pListSciFileName;
                            strcpy(*pListSciFileName,value);
                            noFiles++;
                        }
                        else
                        {
                            *pListSciFileName = (char*)malloc(strlen(pTmpBuf) + strlen(value) + 2);
                            if (*pListSciFileName == NULL)
                            {
                                /* Error */
                                free(pTmpBuf);
                                xml_close(&xLoc);
                                return DLEDS_ERROR;
                            }
                            strncpy(*pListSciFileName, pTmpBuf, strlen(pTmpBuf) + 1);
                            strcat(*pListSciFileName,",");
                            strcat(*pListSciFileName,value);
                            free(pTmpBuf);
                            pTmpBuf = *pListSciFileName;
                            noFiles++;
                        }
                    }
                    *pNoSciFiles = noFiles;
                    xml_leave(&xLoc);    
                }
                else
                {
                    /* Error */
                    xml_close(&xLoc);
                    return DLEDS_ERROR;
                }
                /* Check for DLU files */
                if (xml_seek_start_tag(&xLoc,"ListMdluEdFileName") == XML_OK)
                {
                    xml_enter(&xLoc);
                    noFiles = 0;
                    while (xml_seek_start_tag(&xLoc,"Entry") == XML_OK)
                    {
                        (void)xml_next_token(&xLoc, &xTok);
                        /* Only attribute should be returned. No value assigned to attribute Level */
                        if (xml_get_identifier(&xLoc, &xTok, value) == XML_ERROR)
                        {
                            /* Error */
                            xml_close(&xLoc);
                            return DLEDS_ERROR;
                        }
                        /* Copy data to mdlu ed file name list */
                        if (noFiles == 0)
                        {
                            /* First file name */
                            *pListMdluEdFileName = (char*)malloc(strlen(value) + 1);
                            if (*pListMdluEdFileName == NULL)
                            {
                                /* Error */
                                xml_close(&xLoc);
                                return DLEDS_ERROR;
                            }
                            pTmpBuf = *pListMdluEdFileName;
                            strcpy(*pListMdluEdFileName,value);
                            noFiles++;
                        }
                        else
                        {
                            *pListMdluEdFileName = (char*)malloc(strlen(pTmpBuf) + strlen(value) + 2);
                            if (*pListMdluEdFileName == NULL)
                            {
                                /* Error */
                                free(pTmpBuf);
                                xml_close(&xLoc);
                                return DLEDS_ERROR;
                            }
                            strncpy(*pListMdluEdFileName, pTmpBuf, strlen(pTmpBuf) + 1);
                            strcat(*pListMdluEdFileName,",");
                            strcat(*pListMdluEdFileName,value);
                            free(pTmpBuf);
                            pTmpBuf = *pListMdluEdFileName;
                            noFiles++;
                        }
                    }
                    *pNoMdluEdFiles = noFiles;
                    xml_leave(&xLoc);    
                }
                else
                {
                    /* Error */
                    xml_close(&xLoc);
                    return DLEDS_ERROR;
                }
                /* Check for Clear NVRAM and ClearFlashFileSystem in Configuration */
                if ((res = xml_seek_start_tag(&xLoc,"Configuration")) == XML_OK)
                {
                    xml_enter(&xLoc);
                    if (xml_seek_start_tag(&xLoc,"ClearNVRAM") == XML_OK)
                    {
                        (void)xml_next_token(&xLoc, &xTok);
                        /* Only attribute should be returned. No value assigned to attribute Level */
                        if (xml_get_identifier(&xLoc, &xTok, value) == XML_ERROR)
                        {
                            /* Error */
                            xml_close(&xLoc);
                            return DLEDS_ERROR;
                        }
                        if ((strcmp(value, "true") == 0) || (strcmp(value, "1") == 0))
                        {
                            *clearNVRAM = TRUE;
                        }
                        else if ((strcmp(value, "false") == 0) || (strcmp(value, "0") == 0))
                        {
                            *clearNVRAM = FALSE;
                        }
                        else /* Error */
                        {
                            xml_close(&xLoc);
                            return DLEDS_ERROR;
                        }
                    }
                    else
                    {
                        /* Error */
                        xml_close(&xLoc);
                        return DLEDS_ERROR;
                    }
                    if ((res = xml_seek_start_tag(&xLoc,"ClearFlashFileSystem")) == XML_OK)
                    {
                        (void)xml_next_token(&xLoc, &xTok);
                        /* Only attribute should be returned. No value assigned to attribute Level */
                        if (xml_get_identifier(&xLoc, &xTok, value) == XML_ERROR)
                        {
                            /* Error */
                            xml_close(&xLoc);
                            return DLEDS_ERROR;
                        }
                        if ((strcmp(value, "true") == 0) || (strcmp(value, "1") == 0))
                        {
                            *clearFFS = TRUE;
                        }
                        else if ((strcmp(value, "false") == 0) || (strcmp(value, "0") == 0))
                        {
                            *clearFFS = FALSE;
                        }
                        else /* Error */
                        {
                            xml_close(&xLoc);
                            return DLEDS_ERROR;
                        }
                    }
                } /* end <Configuration> */
            } /* end <Source> */
        } /* end <Level> */
    }/* end <INST_ED> */
    xml_close(&xLoc);
    return res;
}


/*******************************************************************************
* 
* Function name: dledsGetSci2VRUE
*
* Abstract:      This function reads the VRUE (Version Revision Update Evolution)
*                attributes from the SCI_Info data
*
* Return value:  DLEDS_OK      - Information from file has been read
*                DLEDS_ERROR   - No information read from XML file
*
* Globals:       -
*/
static DLEDS_RESULT dledsGetSci2VRUE(
    SCI_InfoHeader  *pSciInfoHeader,           /* IN: Pointer to SCI Info Header */
    XML_LOCAL        *p_xLoc,                  /* IN: Pointer to XML parser instance */
    XML_TOKENS       *p_xTok)                  /* IN: Pointer to XML token */
{
    DLEDS_RESULT res = DLEDS_OK;
    char xAttr[16];
    char xAttrVal[4];
    char *pEnd;

    pSciInfoHeader->schemaVersion = 256U;
    pSciInfoHeader->schemaRelease = 256U;
    pSciInfoHeader->schemaUpdate = 256U;
    pSciInfoHeader->schemaEvolution = 256U;

    while ((xml_get_attribute(p_xLoc, p_xTok, xAttr, sizeof(xAttr), xAttrVal, sizeof(xAttrVal)) == OK) &&
        (*p_xTok == XML_TOK_ATTRIBUTE))
    {
        if (strcmp(xAttr, "Version") == 0)
        {
            pSciInfoHeader->schemaVersion = strtol(xAttrVal, &pEnd, 10);
        }
        else if (strcmp(xAttr, "Release") == 0)
        {
            pSciInfoHeader->schemaRelease = strtol(xAttrVal, &pEnd, 10);
        }
        else if (strcmp(xAttr, "Update") == 0)
        {
            pSciInfoHeader->schemaUpdate = strtol(xAttrVal, &pEnd, 10);
        }
        else if (strcmp(xAttr, "Evolution") == 0)
        {
            pSciInfoHeader->schemaEvolution = strtol(xAttrVal, &pEnd, 10);
        }
    }
    if ((pSciInfoHeader->schemaVersion > 255U) || (pSciInfoHeader->schemaRelease > 255U) ||
        (pSciInfoHeader->schemaUpdate > 255U) || (pSciInfoHeader->schemaEvolution > 255U))
    {
        res = DLEDS_ERROR;
    }
    return res;
}


/*******************************************************************************
* 
* Function name: readDataFromSciInfo
*
* Abstract:      This function reads the values for schema version and element
*                tags <SCI_Name>, <SCI_Version>, <SCI_DeviceType>,
*                <SCI_Type>, <Info> and <Created> from the XML
*                file and returns the values in the SCI_InfoHeader structure. 
*
*                The values in the XML file are validated, i.e. version attributes and 
*                all mandatory values exists and all read values both mandatory and 
*                optional are OK according to the schema file. 
*
*                Missing optional values in the XML file will be filled with 0
*                in the SCI_InfoHeader structure. 
*
* Return value:  DLEDS_OK      - Information has been read
*                DLEDS_ERROR   - Error reading the information
*
* Globals:       -
*/
int readDataFromSciInfo(
    char*               fileName,               /*  IN: XML file */
    SCI_InfoHeader*     pSciInfoHeader)         /* OUT: Values read from XML file */
{
    int res = DLEDS_ERROR;
    XML_LOCAL xLoc;    /* Local data */
    XML_LOCAL xLocTmp;
    long filePos = 0;
    XML_TOKENS xTok;
    char value[XML_MAX_TOKLEN+1];


    if ((fileName == NULL) || (pSciInfoHeader == NULL))
    {
        return res;
    }

    if (xml_open(&xLoc,fileName) < 0)
    {
        return res;
    }
    xml_enter(&xLoc);
    if ((res = xml_seek_start_tag(&xLoc,"SCI_Info")) == XML_OK)
    {
        /* Read in the attributes */
        res = dledsGetSci2VRUE(pSciInfoHeader, &xLoc, &xTok);
        if (res != DLEDS_OK)
        {
            /* Error */
            xml_close(&xLoc);
            return DLEDS_ERROR;
        }
        xml_enter(&xLoc);
        if ((res = xml_seek_start_tag(&xLoc,"SCI")) == XML_OK)
        {
            (void)xml_next_token(&xLoc, &xTok);
            /* Only attribute should be returned. No value assigned to attribute Level */
            if (xml_get_identifier(&xLoc, &xTok, value) == XML_ERROR)
            {
                /* Error */
                xml_close(&xLoc);
                return DLEDS_ERROR;
            }
            /* Return attribute for element tag <SCI_Name> */
            strncpy(pSciInfoHeader->sciName,value,sizeof(pSciInfoHeader->sciName));
            
            if ((res = xml_seek_start_tag(&xLoc,"SCI_Version")) == XML_OK)
            {
                (void)xml_next_token(&xLoc, &xTok);
                /* Only attribute should be returned. No value assigned to attribute Level */
                if (xml_get_identifier(&xLoc, &xTok, value) == XML_ERROR)
                {
                    /* Error */
                    xml_close(&xLoc);
                    return DLEDS_ERROR;
                }
                /* Return attribute for element tag <SCI_Version> */
                strncpy(pSciInfoHeader->sciVersion,value,sizeof(pSciInfoHeader->sciVersion));
                if ((res = xml_seek_start_tag(&xLoc,"SCI_DeviceType")) == XML_OK)
                {
                    (void)xml_next_token(&xLoc, &xTok);
                    /* Only attribute should be returned. No value assigned to attribute Level */
                    if (xml_get_identifier(&xLoc, &xTok, value) == XML_ERROR)
                    {
                        /* Error */
                        xml_close(&xLoc);
                        return DLEDS_ERROR;
                    }
                    /* Return attribute for element tag <SCI_DeviceType> */
                    strncpy(pSciInfoHeader->sciDeviceType,value,sizeof(pSciInfoHeader->sciDeviceType));
                    
                    filePos = ftell(xLoc.infile);
                    xLocTmp = xLoc;
                    if ((res = xml_seek_start_tag(&xLoc,"Info")) == XML_OK)
                    {
                        (void)xml_next_token(&xLoc, &xTok);
                        /* Only attribute should be returned. No value assigned to attribute Level */
                        if (xml_get_identifier_id(&xLoc, &xTok, value) == XML_ERROR)
                        {
                            /* Error */
                            xml_close(&xLoc);
                            return DLEDS_ERROR;
                        }
                        /* Return attribute for element tag <Info> */
                        strncpy(pSciInfoHeader->info,value,sizeof(pSciInfoHeader->info));
                    }
                    else
                    {
                        fseek(xLoc.infile, filePos, SEEK_SET);
                        xLoc = xLocTmp;
                    }
                    
                    /* <Info> is optional, OK if missing, Continue */

                    if ((res = xml_seek_start_tag(&xLoc,"Created")) == XML_OK)
                    {
                        (void)xml_next_token(&xLoc, &xTok);
                        /* Only attribute should be returned. No value assigned to attribute Level */
                        if (xml_get_identifier(&xLoc, &xTok, value) == XML_ERROR)
                        {
                            /* Error */
                            xml_close(&xLoc);
                            return DLEDS_ERROR;
                        }
                        /* Return attribute for element tag <Created> */
                        strncpy(pSciInfoHeader->created,value,sizeof(pSciInfoHeader->created));
                    }
                    else
                    {
                        /* <Created> is optional, OK if missing */
                        res = DLEDS_OK;
                    }  
                } /* end <SCI_DeviceType> */
            } /* end <SCI_Version> */
        } /* end <SCI_Name> */
    }/* end <SCI_Info> */
    xml_close(&xLoc);
    return res;
}


/*******************************************************************************
* 
* Function name: dledsGetEdsp2VRUE
*
* Abstract:      This function reads the VRUE (Version Revision Update Evolution)
*                attributes from the EDSP_Info data
*
* Return value:  DLEDS_OK      - Information from file has been read
*                DLEDS_ERROR   - No information read from XML file
*
* Globals:       -
*/
static DLEDS_RESULT dledsGetEdsp2VRUE(
    EDSP_InfoHeader  *pEdspInfoHeader,        /* IN: Pointer to EDSP Info Header */
    XML_LOCAL        *p_xLoc,                 /* IN: Pointer to XML parser instance */
    XML_TOKENS       *p_xTok)                 /* IN: Pointer to XML token */
{
    DLEDS_RESULT res = DLEDS_OK;
    char xAttr[16];
    char xAttrVal[4];
    char *pEnd;

    pEdspInfoHeader->schemaVersion = 256U;
    pEdspInfoHeader->schemaRelease = 256U;
    pEdspInfoHeader->schemaUpdate = 256U;
    pEdspInfoHeader->schemaEvolution = 256U;

    while ((xml_get_attribute(p_xLoc, p_xTok, xAttr, sizeof(xAttr), xAttrVal, sizeof(xAttrVal)) == OK) &&
        (*p_xTok == XML_TOK_ATTRIBUTE))
    {
        if (strcmp(xAttr, "Version") == 0)
        {
            pEdspInfoHeader->schemaVersion = strtol(xAttrVal, &pEnd, 10);
        }
        else if (strcmp(xAttr, "Release") == 0)
        {
            pEdspInfoHeader->schemaRelease = strtol(xAttrVal, &pEnd, 10);
        }
        else if (strcmp(xAttr, "Update") == 0)
        {
            pEdspInfoHeader->schemaUpdate = strtol(xAttrVal, &pEnd, 10);
        }
        else if (strcmp(xAttr, "Evolution") == 0)
        {
            pEdspInfoHeader->schemaEvolution = strtol(xAttrVal, &pEnd, 10);
        }
    }
    if ((pEdspInfoHeader->schemaVersion > 255U) || (pEdspInfoHeader->schemaRelease > 255U) ||
        (pEdspInfoHeader->schemaUpdate > 255U) || (pEdspInfoHeader->schemaEvolution > 255U))
    {
        res = DLEDS_ERROR;
    }
    return res;
}


/*******************************************************************************
* 
* Function name: readDataFromEdspInfo
*
* Abstract:      This function reads the values for schema version and element
*                tags <EDSP_Name>, <EDSP_Version>, <EDSP_EndDeviceType>,
*                <EDSP_Type>, <EDSP_Source>, <Info> and <Created> from the XML
*                file and returns the values in the EDSP_InfoHeader structure. 
*
*                The values in the XML file are validated, i.e. version attributes and 
*                all mandatory values exists and all read values both mandatory and 
*                optional are OK according to the schema file. 
*
*                Missing optional values in the XML file will be filled with 0
*                in the EDSP_InfoHeader structure. The valid parameter is set
*                to 1 even if optional values are missing in the XML file. 
*
* Return value:  DLEDS_OK      - Information has been read
*                DLEDS_ERROR   - Error reading the information
*
* Globals:       -
*/
int readDataFromEdspInfo(
    char*               fileName,               /*  IN: XML file */
    EDSP_InfoHeader*    pEdspInfoHeader)        /* OUT: Values read from XML file */
{
    int res = DLEDS_ERROR;
    XML_LOCAL xLoc;    /* Local data */
    XML_LOCAL xLocTmp;
    long filePos = 0;
    XML_TOKENS xTok;
    char value[XML_MAX_TOKLEN+1];

    if ((fileName == NULL) || (pEdspInfoHeader == NULL))
    {
        return res;
    }

    if (xml_open(&xLoc,fileName) < 0)
    {
        return res;
    }
    xml_enter(&xLoc);
    if ((res = xml_seek_start_tag(&xLoc,"EDSP_Info")) == XML_OK)
    {
        /* Read in the attributes */
        res = dledsGetEdsp2VRUE(pEdspInfoHeader, &xLoc, &xTok);
        if (res != DLEDS_OK)
        {
            /* Error */
            xml_close(&xLoc);
            return DLEDS_ERROR;
        }
        xml_enter(&xLoc);
        if ((res = xml_seek_start_tag(&xLoc,"EDSP_Name")) == XML_OK)
        {
            (void)xml_next_token(&xLoc, &xTok);
            /* Only attribute should be returned. No value assigned to attribute Level */
            if (xml_get_identifier(&xLoc, &xTok, value) == XML_ERROR)
            {
                /* Error */
                xml_close(&xLoc);
                return DLEDS_ERROR;
            }
            /* Return attribute for element tag <EDSP_Name> */
            strncpy(pEdspInfoHeader->edspName,value,sizeof(pEdspInfoHeader->edspName));
            
            if ((res = xml_seek_start_tag(&xLoc,"EDSP_Version")) == XML_OK)
            {
                (void)xml_next_token(&xLoc, &xTok);
                /* Only attribute should be returned. No value assigned to attribute Level */
                if (xml_get_identifier(&xLoc, &xTok, value) == XML_ERROR)
                {
                    /* Error */
                    xml_close(&xLoc);
                    return DLEDS_ERROR;
                }
                /* Return attribute for element tag <EDSP_Version> */
                strncpy(pEdspInfoHeader->edspVersion,value,sizeof(pEdspInfoHeader->edspVersion));
                if ((res = xml_seek_start_tag(&xLoc,"EDSP_EndDeviceType")) == XML_OK)
                {
                    (void)xml_next_token(&xLoc, &xTok);
                    /* Only attribute should be returned. No value assigned to attribute Level */
                    if (xml_get_identifier(&xLoc, &xTok, value) == XML_ERROR)
                    {
                        /* Error */
                        xml_close(&xLoc);
                        return DLEDS_ERROR;
                    }
                    /* Return attribute for element tag <EDSP_EndDeviceType> */
                    strncpy(pEdspInfoHeader->edspEndDeviceType,value,sizeof(pEdspInfoHeader->edspEndDeviceType));

                    filePos = ftell(xLoc.infile);
                    xLocTmp = xLoc;

                    if ((res = xml_seek_start_tag(&xLoc,"EDSP_Type")) == XML_OK)
                    {
                        (void)xml_next_token(&xLoc, &xTok);
                        /* Only attribute should be returned. No value assigned to attribute Level */
                        if (xml_get_identifier(&xLoc, &xTok, value) == XML_ERROR)
                        {
                            /* Error */
                            xml_close(&xLoc);
                            return DLEDS_ERROR;
                        }
                        /* Return attribute for element tag <EDSP_Type> */
                        strncpy(pEdspInfoHeader->edspType,value,sizeof(pEdspInfoHeader->edspType));
                    }
                    else
                    {
                        fseek(xLoc.infile, filePos, SEEK_SET);
                        xLoc = xLocTmp;
                    }
                    /* <EDSP_Type> is optional, OK if missing, Continue */
                    if ((res = xml_seek_start_tag(&xLoc,"EDSP_Source")) == XML_OK)
                    {
                        (void)xml_next_token(&xLoc, &xTok);
                        /* Only attribute should be returned. No value assigned to attribute Level */
                        if (xml_get_identifier(&xLoc, &xTok, value) == XML_ERROR)
                        {
                            /* Error */
                            xml_close(&xLoc);
                            return DLEDS_ERROR;
                        }
                        /* Return attribute for element tag <EDSP_Source> */
                        strncpy(pEdspInfoHeader->edspSource,value,sizeof(pEdspInfoHeader->edspSource));
                        
                        filePos = ftell(xLoc.infile);
                        xLocTmp = xLoc;
                        if ((res = xml_seek_start_tag(&xLoc,"Info")) == XML_OK)
                        {
                            (void)xml_next_token(&xLoc, &xTok);
                            /* Only attribute should be returned. No value assigned to attribute Level */
                            if (xml_get_identifier_id(&xLoc, &xTok, value) == XML_ERROR)
                            {
                                /* Error */
                                xml_close(&xLoc);
                                return DLEDS_ERROR;
                            }
                            /* Return attribute for element tag <Info> */
                            strncpy(pEdspInfoHeader->info,value,sizeof(pEdspInfoHeader->info));
                        }
                        else
                        {
                            fseek(xLoc.infile, filePos, SEEK_SET);
                            xLoc = xLocTmp;
                        }
                        
                        /* <Info> is optional, OK if missing, Continue */

                        if ((res = xml_seek_start_tag(&xLoc,"Created")) == XML_OK)
                        {
                            (void)xml_next_token(&xLoc, &xTok);
                            /* Only attribute should be returned. No value assigned to attribute Level */
                            if (xml_get_identifier(&xLoc, &xTok, value) == XML_ERROR)
                            {
                                /* Error */
                                xml_close(&xLoc);
                                return DLEDS_ERROR;
                            }
                            /* Return attribute for element tag <Created> */
                            strncpy(pEdspInfoHeader->created,value,sizeof(pEdspInfoHeader->created));
                        }
                        else
                        {
                            /* <Created> is optional, OK if missing */
                            res = DLEDS_OK;
                        }
                    } /* end <EDSP_Source> */  
                } /* end <EDSP_EndDeviceType> */
            } /* end <EDSP_Version> */
        } /* end <EDSP_Name> */
    }/* end <EDSP_Info> */
    xml_close(&xLoc);
    return res;
}

/*******************************************************************************
* 
* Function name: dledsRetrieveCompleteSciInfo
*
* Abstract:      This function reads the values for the complete SCI_Info XML file and
*                returns the values in the SCI_Info structure. 
*
*                The values in the XML file are validated, i.e. version attributes and 
*                all mandatory values exists and all read values both mandatory and 
*                optional are OK according to the schema file. 
*
*                Missing optional values in the XML file will be filled with 0
*                in the SCI_Info structure.  
*
* Return value:  DLEDS_OK      - Information has been read
*                DLEDS_ERROR   - Error reading the information
*
* Globals:       -
*/
int dledsRetrieveCompleteSciInfo(   
	char*           fileName,       /*  IN: XML file */
  	SCI_Info*       pSciInfo)       /* OUT: Values read from SCI_Info XML file */
{
    int res = DLEDS_ERROR;
    XML_LOCAL xLoc;    /* Local data */
    XML_LOCAL xLocTmp;
    long filePos = 0;
    XML_TOKENS xTok;
    char value[XML_MAX_TOKLEN+1];
    DLU_Info*  pCurrentDluInfo;

    if ((fileName == NULL) || (pSciInfo == NULL))
    {
        return res;
    }

    if (xml_open(&xLoc,fileName) < 0)
    {
        return res;
    }
    xml_enter(&xLoc);
    if ((res = xml_seek_start_tag(&xLoc,"SCI")) == XML_OK)
    {
        /* Read in the attributes */
        res = dledsGetSci2VRUE(&(pSciInfo->sciInfoHeader), &xLoc, &xTok);
        if (res != DLEDS_OK)
        {
            /* Error */
            xml_close(&xLoc);
            return DLEDS_ERROR;
        }
        xml_enter(&xLoc);
        if ((res = xml_seek_start_tag(&xLoc,"SCI_Name")) == XML_OK)
        {
            (void)xml_next_token(&xLoc, &xTok);
            /* Only attribute should be returned. No value assigned to attribute Level */
            if (xml_get_identifier(&xLoc, &xTok, value) == XML_ERROR)
            {
                /* Error */
                xml_close(&xLoc);
                return DLEDS_ERROR;
            }
            /* Return attribute for element tag <SCI_Name> */
            strncpy(pSciInfo->sciInfoHeader.sciName,value,sizeof(pSciInfo->sciInfoHeader.sciName));
            
            if ((res = xml_seek_start_tag(&xLoc,"SCI_Version")) == XML_OK)
            {
                (void)xml_next_token(&xLoc, &xTok);
                /* Only attribute should be returned. No value assigned to attribute Level */
                if (xml_get_identifier(&xLoc, &xTok, value) == XML_ERROR)
                {
                    /* Error */
                    xml_close(&xLoc);
                    return DLEDS_ERROR;
                }
                /* Return attribute for element tag <SCI_Version> */
                strncpy(pSciInfo->sciInfoHeader.sciVersion,value,sizeof(pSciInfo->sciInfoHeader.sciVersion));

                if ((res = xml_seek_start_tag(&xLoc,"SCI_DeviceType")) == XML_OK)
                {
                    (void)xml_next_token(&xLoc, &xTok);
                    /* Only attribute should be returned. No value assigned to attribute Level */
                    if (xml_get_identifier(&xLoc, &xTok, value) == XML_ERROR)
                    {
                        /* Error */
                        xml_close(&xLoc);
                        return DLEDS_ERROR;
                    }
                    /* Return attribute for element tag <SCI_DeviceType> */
                    strncpy(pSciInfo->sciInfoHeader.sciDeviceType,value,sizeof(pSciInfo->sciInfoHeader.sciDeviceType));

                    filePos = ftell(xLoc.infile);
                    xLocTmp = xLoc;
                    if ((res = xml_seek_start_tag(&xLoc,"SCI_Type")) == XML_OK)
                    {
                        (void)xml_next_token(&xLoc, &xTok);
                        /* Only attribute should be returned. No value assigned to attribute Level */
                        if (xml_get_identifier(&xLoc, &xTok, value) == XML_ERROR)
                        {
                            /* Error */
                            xml_close(&xLoc);
                            return DLEDS_ERROR;
                        }
                        /* Return attribute for element tag <SCI_Type> */
                        strncpy(pSciInfo->sciInfoHeader.sciType,value,sizeof(pSciInfo->sciInfoHeader.sciType));
                    }
                    else
                    {
                        fseek(xLoc.infile, filePos, SEEK_SET);
                        xLoc = xLocTmp;
                    }
                    /* <SCI_Type> is optional, OK if missing, Continue */

                        
                    filePos = ftell(xLoc.infile);
                    xLocTmp = xLoc;
                    if ((res = xml_seek_start_tag(&xLoc,"Info")) == XML_OK)
                    {
                        (void)xml_next_token(&xLoc, &xTok);
                        /* Only attribute should be returned. No value assigned to attribute Level */
                        if (xml_get_identifier_id(&xLoc, &xTok, value) == XML_ERROR)
                        {
                            /* Error */
                            xml_close(&xLoc);
                            return DLEDS_ERROR;
                        }
                        /* Return attribute for element tag <Info> */
                        strncpy(pSciInfo->sciInfoHeader.info,value,sizeof(pSciInfo->sciInfoHeader.info));
                    }
                    else
                    {
                        fseek(xLoc.infile, filePos, SEEK_SET);
                        xLoc = xLocTmp;
                    }
                        
                    /* <Info> is optional, OK if missing, Continue */

                    if ((res = xml_seek_start_tag(&xLoc,"Created")) == XML_OK)
                    {
                        (void)xml_next_token(&xLoc, &xTok);
                        /* Only attribute should be returned. No value assigned to attribute Level */
                        if (xml_get_identifier(&xLoc, &xTok, value) == XML_ERROR)
                        {
                            /* Error */
                            xml_close(&xLoc);
                            return DLEDS_ERROR;
                        }
                        /* Return attribute for element tag <Created> */
                        strncpy(pSciInfo->sciInfoHeader.created,value,sizeof(pSciInfo->sciInfoHeader.created));
                    }
                    else
                    {
                        fseek(xLoc.infile, filePos, SEEK_SET);
                        xLoc = xLocTmp;
                    }

                    /* <Created> is optional, OK if missing, Continue */

                    if ((res = xml_seek_start_tag(&xLoc,"DLU_List")) == XML_OK)
                    {
                        /* create DLU_Info object */
                        pSciInfo->pDluInfo = (DLU_Info*)malloc(sizeof(DLU_Info));
                        pCurrentDluInfo = pSciInfo->pDluInfo;
                        memset(pCurrentDluInfo, 0, sizeof(DLU_Info));
 
                        xml_enter(&xLoc);
                        while ((res = xml_seek_start_tag(&xLoc,"DLU")) == XML_OK)
                        {
                            xml_enter(&xLoc);

                            filePos = ftell(xLoc.infile);
                            xLocTmp = xLoc;
                            if ((res = xml_seek_start_tag(&xLoc,"productId")) == XML_OK)
                            {
                                (void)xml_next_token(&xLoc, &xTok);
                                /* Only attribute should be returned. No value assigned to attribute Level */
                                if (xml_get_identifier(&xLoc, &xTok, value) == XML_ERROR)
                                {
                                    /* Error */
                                    xml_close(&xLoc);
                                    return DLEDS_ERROR;
                                }

                                /* Return attribute for element tag <productId> */
                                strncpy(pCurrentDluInfo->dluInfoHeader.productId,value,sizeof(pCurrentDluInfo->dluInfoHeader.productId));
                            }
                            else
                            {
                                fseek(xLoc.infile, filePos, SEEK_SET);
                                xLoc = xLocTmp;
                            }

                            /* <productId> is optional, OK if missing, Continue */

                            filePos = ftell(xLoc.infile);
                            xLocTmp = xLoc;
                            if ((res = xml_seek_start_tag(&xLoc,"type")) == XML_OK)
                            {
                                (void)xml_next_token(&xLoc, &xTok);
                                /* Only attribute should be returned. No value assigned to attribute Level */
                                if (xml_get_identifier(&xLoc, &xTok, value) == XML_ERROR)
                                {
                                    /* Error */
                                    xml_close(&xLoc);
                                    return DLEDS_ERROR;
                                }

                                /* Return attribute for element tag <type> */
                                strncpy(pCurrentDluInfo->dluInfoHeader.type,value,sizeof(pCurrentDluInfo->dluInfoHeader.type));
                            }
                            else
                            {
                                fseek(xLoc.infile, filePos, SEEK_SET);
                                xLoc = xLocTmp;
                            }

                            /* <type> is optional, OK if missing, Continue */

                            if ((res = xml_seek_start_tag(&xLoc,"name")) == XML_OK)
                            {
                                (void)xml_next_token(&xLoc, &xTok);
                                /* Only attribute should be returned. No value assigned to attribute Level */
                                if (xml_get_identifier(&xLoc, &xTok, value) == XML_ERROR)
                                {
                                    /* Error */
                                    xml_close(&xLoc);
                                    return DLEDS_ERROR;
                                }

                                /* Return attribute for element tag <name> */
                                strncpy(pCurrentDluInfo->dluInfoHeader.name,value,sizeof(pCurrentDluInfo->dluInfoHeader.name));

                                filePos = ftell(xLoc.infile);
                                xLocTmp = xLoc;
                                if ((res = xml_seek_start_tag(&xLoc,"fileName")) == XML_OK)
                                {
                                    (void)xml_next_token(&xLoc, &xTok);
                                    /* Only attribute should be returned. No value assigned to attribute Level */
                                    if (xml_get_identifier(&xLoc, &xTok, value) == XML_ERROR)
                                    {
                                        /* Error */
                                        xml_close(&xLoc);
                                        return DLEDS_ERROR;
                                    }

                                    /* Return attribute for element tag <fileName> */
                                    strncpy(pCurrentDluInfo->dluInfoHeader.fileName,value,sizeof(pCurrentDluInfo->dluInfoHeader.fileName));
                                }
                                else
                                {
                                    fseek(xLoc.infile, filePos, SEEK_SET);
                                    xLoc = xLocTmp;
                                }

                                /* <fileName> is optional, OK if missing, Continue */

                                if ((res = xml_seek_start_tag(&xLoc,"version")) == XML_OK)
                                {
                                    (void)xml_next_token(&xLoc, &xTok);
                                    /* Only attribute should be returned. No value assigned to attribute Level */
                                    if (xml_get_identifier(&xLoc, &xTok, value) == XML_ERROR)
                                    {
                                        /* Error */
                                        xml_close(&xLoc);
                                        return DLEDS_ERROR;
                                    }

                                    /* Return attribute for element tag <version> */
                                    strncpy(pCurrentDluInfo->dluInfoHeader.version,value,sizeof(pCurrentDluInfo->dluInfoHeader.version));

                                    filePos = ftell(xLoc.infile);
                                    xLocTmp = xLoc;
                                    if ((res = xml_seek_start_tag(&xLoc,"created")) == XML_OK)
                                    {
                                        (void)xml_next_token(&xLoc, &xTok);
                                        /* Only attribute should be returned. No value assigned to attribute Level */
                                        if (xml_get_identifier(&xLoc, &xTok, value) == XML_ERROR)
                                        {
                                            /* Error */
                                            xml_close(&xLoc);
                                            return DLEDS_ERROR;
                                        }

                                        /* Return attribute for element tag <created> */
                                        strncpy(pCurrentDluInfo->dluInfoHeader.created,value,sizeof(pCurrentDluInfo->dluInfoHeader.created));
                                    }
                                    else
                                    {
                                        fseek(xLoc.infile, filePos, SEEK_SET);
                                        xLoc = xLocTmp;
                                    }

                                    /* <created> is optional, OK if missing, Continue */

                                    filePos = ftell(xLoc.infile);
                                    xLocTmp = xLoc;
                                    if ((res = xml_seek_start_tag(&xLoc,"modified")) == XML_OK)
                                    {
                                        (void)xml_next_token(&xLoc, &xTok);
                                        /* Only attribute should be returned. No value assigned to attribute Level */
                                        if (xml_get_identifier(&xLoc, &xTok, value) == XML_ERROR)
                                        {
                                            /* Error */
                                            xml_close(&xLoc);
                                            return DLEDS_ERROR;
                                        }

                                        /* Return attribute for element tag <modified> */
                                        strncpy(pCurrentDluInfo->dluInfoHeader.modified,value,sizeof(pCurrentDluInfo->dluInfoHeader.modified));
                                    }
                                    else
                                    {
                                        fseek(xLoc.infile, filePos, SEEK_SET);
                                        xLoc = xLocTmp;
                                    }

                                    /* <modified> is optional, OK if missing, Continue */

                                    filePos = ftell(xLoc.infile);
                                    xLocTmp = xLoc;
                                    if ((res = xml_seek_start_tag(&xLoc,"size")) == XML_OK)
                                    {
                                        (void)xml_next_token(&xLoc, &xTok);
                                        /* Only attribute should be returned. No value assigned to attribute Level */
                                        if (xml_get_identifier(&xLoc, &xTok, value) == XML_ERROR)
                                        {
                                            /* Error */
                                            xml_close(&xLoc);
                                            return DLEDS_ERROR;
                                        }

                                        /* Return attribute for element tag <size> */
                                        strncpy(pCurrentDluInfo->dluInfoHeader.size,value,sizeof(pCurrentDluInfo->dluInfoHeader.size));
                                    }
                                    else
                                    {
                                        fseek(xLoc.infile, filePos, SEEK_SET);
                                        xLoc = xLocTmp;
                                    }

                                    /* <size> is optional, OK if missing, Continue */

                                    filePos = ftell(xLoc.infile);
                                    xLocTmp = xLoc;
                                    if ((res = xml_seek_start_tag(&xLoc,"checksum")) == XML_OK)
                                    {
                                        (void)xml_next_token(&xLoc, &xTok);
                                        /* Only attribute should be returned. No value assigned to attribute Level */
                                        if (xml_get_identifier(&xLoc, &xTok, value) == XML_ERROR)
                                        {
                                            /* Error */
                                            xml_close(&xLoc);
                                            return DLEDS_ERROR;
                                        }

                                        /* Return attribute for element tag <checksum> */
                                        strncpy(pCurrentDluInfo->dluInfoHeader.checksum,value,sizeof(pCurrentDluInfo->dluInfoHeader.checksum));
                                    }
                                    else
                                    {
                                        fseek(xLoc.infile, filePos, SEEK_SET);
                                        xLoc = xLocTmp;
                                    }

                                    /* <checksum> is optional, OK if missing, Continue */

                                    filePos = ftell(xLoc.infile);
                                    xLocTmp = xLoc;
                                    if ((res = xml_seek_start_tag(&xLoc,"crc32")) == XML_OK)
                                    {
                                        (void)xml_next_token(&xLoc, &xTok);
                                        /* Only attribute should be returned. No value assigned to attribute Level */
                                        if (xml_get_identifier(&xLoc, &xTok, value) == XML_ERROR)
                                        {
                                            /* Error */
                                            xml_close(&xLoc);
                                            return DLEDS_ERROR;
                                        }

                                        /* Return attribute for element tag <crc32> */
                                        strncpy(pCurrentDluInfo->dluInfoHeader.crc32,value,sizeof(pCurrentDluInfo->dluInfoHeader.crc32));
                                    }
                                    else
                                    {
                                        fseek(xLoc.infile, filePos, SEEK_SET);
                                        xLoc = xLocTmp;
                                    }

                                    /* <crc32> is optional, OK if missing, Continue */

                                    filePos = ftell(xLoc.infile);
                                    xLocTmp = xLoc;
                                    if ((res = xml_seek_start_tag(&xLoc,"address")) == XML_OK)
                                    {
                                        (void)xml_next_token(&xLoc, &xTok);
                                        /* Only attribute should be returned. No value assigned to attribute Level */
                                        if (xml_get_identifier(&xLoc, &xTok, value) == XML_ERROR)
                                        {
                                            /* Error */
                                            xml_close(&xLoc);
                                            return DLEDS_ERROR;
                                        }

                                        /* Return attribute for element tag <address> */
                                        strncpy(pCurrentDluInfo->dluInfoHeader.address,value,sizeof(pCurrentDluInfo->dluInfoHeader.address));
                                    }
                                    else
                                    {
                                        fseek(xLoc.infile, filePos, SEEK_SET);
                                        xLoc = xLocTmp;
                                    }

                                    /* <address> is optional, OK if missing, Continue */

                                    filePos = ftell(xLoc.infile);
                                    xLocTmp = xLoc;
                                    if ((res = xml_seek_start_tag(&xLoc,"supplier")) == XML_OK)
                                    {
                                        (void)xml_next_token(&xLoc, &xTok);
                                        /* Only attribute should be returned. No value assigned to attribute Level */
                                        if (xml_get_identifier(&xLoc, &xTok, value) == XML_ERROR)
                                        {
                                            /* Error */
                                            xml_close(&xLoc);
                                            return DLEDS_ERROR;
                                        }

                                        /* Return attribute for element tag <supplier> */
                                        strncpy(pCurrentDluInfo->dluInfoHeader.supplier,value,sizeof(pCurrentDluInfo->dluInfoHeader.supplier));
                                    }
                                    else
                                    {
                                        /* <supplier> is optional, OK if missing, Continue */
                                        res = DLEDS_OK;
                                        fseek(xLoc.infile, filePos, SEEK_SET);
                                        xLoc = xLocTmp;
                                    }

                                    if (res == XML_OK)
                                    {
                                        /* Complete DLU parsed, increment no of found DLU */
                                        (pSciInfo->noOfDlu)++;
                                        /* Prepare for next DLU */
                                        pCurrentDluInfo->pNextDluInfo = (DLU_Info*)malloc(sizeof(DLU_Info));
                                        pCurrentDluInfo = pCurrentDluInfo->pNextDluInfo;
                                        memset(pCurrentDluInfo, 0, sizeof(DLU_Info));
                                    }

                                } /* end <version> */
                                else
                                {
                                    /* mandatory tag <version> missing */
                                    pSciInfo->noOfDlu = 0;
                                }
                            } /* end <name> */
                            else
                            {
                                /* mandatory tag <name> missing */
                                pSciInfo->noOfDlu = 0;
                            } 
                            xml_leave(&xLoc);               
                        } /* end while <DLU> */
                        /* Check that at least 1 DLU found */
                        if ((pSciInfo->noOfDlu) > 0)
                        {
                            res = DLEDS_OK;
                        }
                        xml_leave(&xLoc);
                    } /* end <DLU_List> */
                } /* end <SCI_DeviceType> */
            } /* end <SCI_Version> */
        } /* end <SCI_Name> */
    }/* end <SCI_Info> */
    xml_close(&xLoc);
    return res;
}


/*******************************************************************************
* 
* Function name: dledsRetrieveCompleteEdspInfo
*
* Abstract:      This function reads the values for the complete EDSP_Info XML file and
*                returns the values in the EDSP_Info structure. 
*
*                The values in the XML file are validated, i.e. version attributes and 
*                all mandatory values exists and all read values both mandatory and 
*                optional are OK according to the schema file. 
*
*                Missing optional values in the XML file will be filled with 0
*                in the EDSP_Info structure. 
*
* Return value:  DLEDS_OK      - Information has been read
*                DLEDS_ERROR   - Error reading the information
*
* Globals:       -
*/
int dledsRetrieveCompleteEdspInfo(   
	char*           fileName,       /*  IN: XML file */
  	EDSP_Info*      pEdspInfo)      /* OUT: Values read from EDSP_Info XML file */
{
    int res = DLEDS_ERROR;
    XML_LOCAL xLoc;    /* Local data */
    XML_LOCAL xLocTmp;
    long filePos = 0;
    XML_TOKENS xTok;
    char value[XML_MAX_TOKLEN+1];
    SCI_Info*  pCurrentSciInfo;
    DLU_Info*  pCurrentDluInfo;

    if ((fileName == NULL) || (pEdspInfo == NULL))
    {
        return res;
    }

    if (xml_open(&xLoc,fileName) < 0)
    {
        return res;
    }
    xml_enter(&xLoc);
    if ((res = xml_seek_start_tag(&xLoc,"EDSP_Info")) == XML_OK)
    {
        /* Read in the attributes */
        res = dledsGetEdsp2VRUE(&(pEdspInfo->edspInfoHeader), &xLoc, &xTok);
        if (res != DLEDS_OK)
        {
            /* Error */
            xml_close(&xLoc);
            return DLEDS_ERROR;
        }
        xml_enter(&xLoc);
        if ((res = xml_seek_start_tag(&xLoc,"EDSP_Name")) == XML_OK)
        {
            (void)xml_next_token(&xLoc, &xTok);
            /* Only attribute should be returned. No value assigned to attribute Level */
            if (xml_get_identifier(&xLoc, &xTok, value) == XML_ERROR)
            {
                /* Error */
                xml_close(&xLoc);
                return DLEDS_ERROR;
            }
            /* Return attribute for element tag <EDSP_Name> */
            strncpy(pEdspInfo->edspInfoHeader.edspName,value,sizeof(pEdspInfo->edspInfoHeader.edspName));
            
            if ((res = xml_seek_start_tag(&xLoc,"EDSP_Version")) == XML_OK)
            {
                (void)xml_next_token(&xLoc, &xTok);
                /* Only attribute should be returned. No value assigned to attribute Level */
                if (xml_get_identifier(&xLoc, &xTok, value) == XML_ERROR)
                {
                    /* Error */
                    xml_close(&xLoc);
                    return DLEDS_ERROR;
                }
                /* Return attribute for element tag <EDSP_Version> */
                strncpy(pEdspInfo->edspInfoHeader.edspVersion,value,sizeof(pEdspInfo->edspInfoHeader.edspVersion));

                if ((res = xml_seek_start_tag(&xLoc,"EDSP_EndDeviceType")) == XML_OK)
                {
                    (void)xml_next_token(&xLoc, &xTok);
                    /* Only attribute should be returned. No value assigned to attribute Level */
                    if (xml_get_identifier(&xLoc, &xTok, value) == XML_ERROR)
                    {
                        /* Error */
                        xml_close(&xLoc);
                        return DLEDS_ERROR;
                    }
                    /* Return attribute for element tag <EDSP_EndDeviceType> */
                    strncpy(pEdspInfo->edspInfoHeader.edspEndDeviceType,value,sizeof(pEdspInfo->edspInfoHeader.edspEndDeviceType));

                    filePos = ftell(xLoc.infile);
                    xLocTmp = xLoc;

                    if ((res = xml_seek_start_tag(&xLoc,"EDSP_Type")) == XML_OK)
                    {
                        (void)xml_next_token(&xLoc, &xTok);
                        /* Only attribute should be returned. No value assigned to attribute Level */
                        if (xml_get_identifier(&xLoc, &xTok, value) == XML_ERROR)
                        {
                            /* Error */
                            xml_close(&xLoc);
                            return DLEDS_ERROR;
                        }
                        /* Return attribute for element tag <EDSP_Type> */
                        strncpy(pEdspInfo->edspInfoHeader.edspType,value,sizeof(pEdspInfo->edspInfoHeader.edspType));
                    }
                    else
                    {
                        fseek(xLoc.infile, filePos, SEEK_SET);
                        xLoc = xLocTmp;
                    }
                    /* <EDSP_Type> is optional, OK if missing, Continue */

                    if ((res = xml_seek_start_tag(&xLoc,"EDSP_Source")) == XML_OK)
                    {
                        (void)xml_next_token(&xLoc, &xTok);
                        /* Only attribute should be returned. No value assigned to attribute Level */
                        if (xml_get_identifier(&xLoc, &xTok, value) == XML_ERROR)
                        {
                            /* Error */
                            xml_close(&xLoc);
                            return DLEDS_ERROR;
                        }
                        /* Return attribute for element tag <EDSP_Source> */
                        strncpy(pEdspInfo->edspInfoHeader.edspSource,value,sizeof(pEdspInfo->edspInfoHeader.edspSource));
                        
                        filePos = ftell(xLoc.infile);
                        xLocTmp = xLoc;
                        if ((res = xml_seek_start_tag(&xLoc,"Info")) == XML_OK)
                        {
                            (void)xml_next_token(&xLoc, &xTok);
                            /* Only attribute should be returned. No value assigned to attribute Level */
                            if (xml_get_identifier_id(&xLoc, &xTok, value) == XML_ERROR)
                            {
                                /* Error */
                                xml_close(&xLoc);
                                return DLEDS_ERROR;
                            }
                            /* Return attribute for element tag <Info> */
                            strncpy(pEdspInfo->edspInfoHeader.info,value,sizeof(pEdspInfo->edspInfoHeader.info));
                        }
                        else
                        {
                            fseek(xLoc.infile, filePos, SEEK_SET);
                            xLoc = xLocTmp;
                        }
                        
                        /* <Info> is optional, OK if missing, Continue */

                        if ((res = xml_seek_start_tag(&xLoc,"Created")) == XML_OK)
                        {
                            (void)xml_next_token(&xLoc, &xTok);
                            /* Only attribute should be returned. No value assigned to attribute Level */
                            if (xml_get_identifier(&xLoc, &xTok, value) == XML_ERROR)
                            {
                                /* Error */
                                xml_close(&xLoc);
                                return DLEDS_ERROR;
                            }
                            /* Return attribute for element tag <Created> */
                            strncpy(pEdspInfo->edspInfoHeader.created,value,sizeof(pEdspInfo->edspInfoHeader.created));
                        }
                        else
                        {
                            fseek(xLoc.infile, filePos, SEEK_SET);
                            xLoc = xLocTmp;
                        }

                        /* <Created> is optional, OK if missing, Continue */

                        if ((res = xml_seek_start_tag(&xLoc,"SCI_List")) == XML_OK)
                        {
                            /* create SCI_Info object */
                            pEdspInfo->pSciInfo = (SCI_Info*)malloc(sizeof(SCI_Info));
                            pCurrentSciInfo = pEdspInfo->pSciInfo;
                            memset(pCurrentSciInfo, 0, sizeof(SCI_Info));
 
                            xml_enter(&xLoc);

                            while ((res = xml_seek_start_tag(&xLoc,"SCI")) == XML_OK)
                            {
                                /* Read in the attributes */
                                res = dledsGetSci2VRUE(&(pCurrentSciInfo->sciInfoHeader), &xLoc, &xTok);
                                if (res != DLEDS_OK)
                                {
                                    /* Error */
                                    xml_close(&xLoc);
                                    return DLEDS_ERROR;
                                }

                                xml_enter(&xLoc);

                                if ((res = xml_seek_start_tag(&xLoc,"SCI_Name")) == XML_OK)
                                {
                                    (void)xml_next_token(&xLoc, &xTok);
                                    /* Only attribute should be returned. No value assigned to attribute Level */
                                    if (xml_get_identifier(&xLoc, &xTok, value) == XML_ERROR)
                                    {
                                        /* Error */
                                        xml_close(&xLoc);
                                        return DLEDS_ERROR;
                                    }

                                    /* Return attribute for element tag <SCI_Name> */
                                    strncpy(pCurrentSciInfo->sciInfoHeader.sciName,value,sizeof(pCurrentSciInfo->sciInfoHeader.sciName));

                                    if ((res = xml_seek_start_tag(&xLoc,"SCI_Version")) == XML_OK)
                                    {
                                        (void)xml_next_token(&xLoc, &xTok);
                                        /* Only attribute should be returned. No value assigned to attribute Level */
                                        if (xml_get_identifier(&xLoc, &xTok, value) == XML_ERROR)
                                        {
                                            /* Error */
                                            xml_close(&xLoc);
                                            return DLEDS_ERROR;
                                        }

                                        /* Return attribute for element tag <SCI_Version> */
                                        strncpy(pCurrentSciInfo->sciInfoHeader.sciVersion,value,sizeof(pCurrentSciInfo->sciInfoHeader.sciVersion));

                                        if ((res = xml_seek_start_tag(&xLoc,"SCI_DeviceType")) == XML_OK)
                                        {
                                            (void)xml_next_token(&xLoc, &xTok);
                                            /* Only attribute should be returned. No value assigned to attribute Level */
                                            if (xml_get_identifier(&xLoc, &xTok, value) == XML_ERROR)
                                            {
                                                /* Error */
                                                xml_close(&xLoc);
                                                return DLEDS_ERROR;
                                            }

                                            /* Return attribute for element tag <SCI_DeviceType> */
                                            strncpy(pCurrentSciInfo->sciInfoHeader.sciDeviceType,value,sizeof(pCurrentSciInfo->sciInfoHeader.sciDeviceType));


                                            filePos = ftell(xLoc.infile);
                                            xLocTmp = xLoc;
                                            if ((res = xml_seek_start_tag(&xLoc,"SCI_Type")) == XML_OK)
                                            {
                                                (void)xml_next_token(&xLoc, &xTok);
                                                /* Only attribute should be returned. No value assigned to attribute Level */
                                                if (xml_get_identifier(&xLoc, &xTok, value) == XML_ERROR)
                                                {
                                                    /* Error */
                                                    xml_close(&xLoc);
                                                    return DLEDS_ERROR;
                                                }

                                                /* Return attribute for element tag <SCI_Type> */
                                                strncpy(pCurrentSciInfo->sciInfoHeader.sciType,value,sizeof(pCurrentSciInfo->sciInfoHeader.sciType));
                                            }
                                            else
                                            {
                                                fseek(xLoc.infile, filePos, SEEK_SET);
                                                xLoc = xLocTmp;
                                            }

                                            /* <SCI_Type> is optional, OK if missing, Continue */

                                            filePos = ftell(xLoc.infile);
                                            xLocTmp = xLoc;
                                            if ((res = xml_seek_start_tag(&xLoc,"Info")) == XML_OK)
                                            {
                                                (void)xml_next_token(&xLoc, &xTok);
                                                /* Only attribute should be returned. No value assigned to attribute Level */
                                                if (xml_get_identifier_id(&xLoc, &xTok, value) == XML_ERROR)
                                                {
                                                    /* Error */
                                                    xml_close(&xLoc);
                                                    return DLEDS_ERROR;
                                                }

                                                /* Return attribute for element tag <Info> */
                                                strncpy(pCurrentSciInfo->sciInfoHeader.info,value,sizeof(pCurrentSciInfo->sciInfoHeader.info));
                                            }
                                            else
                                            {
                                                fseek(xLoc.infile, filePos, SEEK_SET);
                                                xLoc = xLocTmp;
                                            }

                                            /* <Info> is optional, OK if missing, Continue */

                                            filePos = ftell(xLoc.infile);
                                            xLocTmp = xLoc;
                                            if ((res = xml_seek_start_tag(&xLoc,"Created")) == XML_OK)
                                            {
                                                (void)xml_next_token(&xLoc, &xTok);
                                                /* Only attribute should be returned. No value assigned to attribute Level */
                                                if (xml_get_identifier(&xLoc, &xTok, value) == XML_ERROR)
                                                {
                                                    /* Error */
                                                    xml_close(&xLoc);
                                                    return DLEDS_ERROR;
                                                }

                                                /* Return attribute for element tag <Created> */
                                                strncpy(pCurrentSciInfo->sciInfoHeader.created,value,sizeof(pCurrentSciInfo->sciInfoHeader.created));
                                            }
                                            else
                                            {
                                                fseek(xLoc.infile, filePos, SEEK_SET);
                                                xLoc = xLocTmp;
                                            }

                                            /* <Created> is optional, OK if missing, Continue */

                                            if ((res = xml_seek_start_tag(&xLoc,"DLU_List")) == XML_OK)
                                            {
                                                /* create DLU_Info object */
                                                pCurrentSciInfo->pDluInfo = (DLU_Info*)malloc(sizeof(DLU_Info));
                                                pCurrentDluInfo = pCurrentSciInfo->pDluInfo;
                                                memset(pCurrentDluInfo, 0, sizeof(DLU_Info));
 
                                                xml_enter(&xLoc);
                                                while ((res = xml_seek_start_tag(&xLoc,"DLU")) == XML_OK)
                                                {
                                                    xml_enter(&xLoc);

                                                    filePos = ftell(xLoc.infile);
                                                    xLocTmp = xLoc;
                                                    if ((res = xml_seek_start_tag(&xLoc,"productId")) == XML_OK)
                                                    {
                                                        (void)xml_next_token(&xLoc, &xTok);
                                                        /* Only attribute should be returned. No value assigned to attribute Level */
                                                        if (xml_get_identifier(&xLoc, &xTok, value) == XML_ERROR)
                                                        {
                                                            /* Error */
                                                            xml_close(&xLoc);
                                                            return DLEDS_ERROR;
                                                        }

                                                        /* Return attribute for element tag <productId> */
                                                        strncpy(pCurrentDluInfo->dluInfoHeader.productId,value,sizeof(pCurrentDluInfo->dluInfoHeader.productId));
                                                    }
                                                    else
                                                    {
                                                        fseek(xLoc.infile, filePos, SEEK_SET);
                                                        xLoc = xLocTmp;
                                                    }

                                                    /* <productId> is optional, OK if missing, Continue */

                                                    filePos = ftell(xLoc.infile);
                                                    xLocTmp = xLoc;
                                                    if ((res = xml_seek_start_tag(&xLoc,"type")) == XML_OK)
                                                    {
                                                        (void)xml_next_token(&xLoc, &xTok);
                                                        /* Only attribute should be returned. No value assigned to attribute Level */
                                                        if (xml_get_identifier(&xLoc, &xTok, value) == XML_ERROR)
                                                        {
                                                            /* Error */
                                                            xml_close(&xLoc);
                                                            return DLEDS_ERROR;
                                                        }

                                                        /* Return attribute for element tag <type> */
                                                        strncpy(pCurrentDluInfo->dluInfoHeader.type,value,sizeof(pCurrentDluInfo->dluInfoHeader.type));
                                                    }
                                                    else
                                                    {
                                                        fseek(xLoc.infile, filePos, SEEK_SET);
                                                        xLoc = xLocTmp;
                                                    }

                                                    /* <type> is optional, OK if missing, Continue */

                                                    if ((res = xml_seek_start_tag(&xLoc,"name")) == XML_OK)
                                                    {
                                                        (void)xml_next_token(&xLoc, &xTok);
                                                        /* Only attribute should be returned. No value assigned to attribute Level */
                                                        if (xml_get_identifier(&xLoc, &xTok, value) == XML_ERROR)
                                                        {
                                                            /* Error */
                                                            xml_close(&xLoc);
                                                            return DLEDS_ERROR;
                                                        }

                                                        /* Return attribute for element tag <name> */
                                                        strncpy(pCurrentDluInfo->dluInfoHeader.name,value,sizeof(pCurrentDluInfo->dluInfoHeader.name));

                                                        filePos = ftell(xLoc.infile);
                                                        xLocTmp = xLoc;
                                                        if ((res = xml_seek_start_tag(&xLoc,"fileName")) == XML_OK)
                                                        {
                                                            (void)xml_next_token(&xLoc, &xTok);
                                                            /* Only attribute should be returned. No value assigned to attribute Level */
                                                            if (xml_get_identifier(&xLoc, &xTok, value) == XML_ERROR)
                                                            {
                                                                /* Error */
                                                                xml_close(&xLoc);
                                                                return DLEDS_ERROR;
                                                            }

                                                            /* Return attribute for element tag <fileName> */
                                                            strncpy(pCurrentDluInfo->dluInfoHeader.fileName,value,sizeof(pCurrentDluInfo->dluInfoHeader.fileName));
                                                        }
                                                        else
                                                        {
                                                            fseek(xLoc.infile, filePos, SEEK_SET);
                                                            xLoc = xLocTmp;
                                                        }

                                                        /* <fileName> is optional, OK if missing, Continue */

                                                        if ((res = xml_seek_start_tag(&xLoc,"version")) == XML_OK)
                                                        {
                                                            (void)xml_next_token(&xLoc, &xTok);
                                                            /* Only attribute should be returned. No value assigned to attribute Level */
                                                            if (xml_get_identifier(&xLoc, &xTok, value) == XML_ERROR)
                                                            {
                                                                /* Error */
                                                                xml_close(&xLoc);
                                                                return DLEDS_ERROR;
                                                            }

                                                            /* Return attribute for element tag <version> */
                                                            strncpy(pCurrentDluInfo->dluInfoHeader.version,value,sizeof(pCurrentDluInfo->dluInfoHeader.version));

                                                            filePos = ftell(xLoc.infile);
                                                            xLocTmp = xLoc;
                                                            if ((res = xml_seek_start_tag(&xLoc,"created")) == XML_OK)
                                                            {
                                                                (void)xml_next_token(&xLoc, &xTok);
                                                                /* Only attribute should be returned. No value assigned to attribute Level */
                                                                if (xml_get_identifier(&xLoc, &xTok, value) == XML_ERROR)
                                                                {
                                                                    /* Error */
                                                                    xml_close(&xLoc);
                                                                    return DLEDS_ERROR;
                                                                }

                                                                /* Return attribute for element tag <created> */
                                                                strncpy(pCurrentDluInfo->dluInfoHeader.created,value,sizeof(pCurrentDluInfo->dluInfoHeader.created));
                                                            }
                                                            else
                                                            {
                                                                fseek(xLoc.infile, filePos, SEEK_SET);
                                                                xLoc = xLocTmp;
                                                            }

                                                            /* <created> is optional, OK if missing, Continue */

                                                            filePos = ftell(xLoc.infile);
                                                            xLocTmp = xLoc;
                                                            if ((res = xml_seek_start_tag(&xLoc,"modified")) == XML_OK)
                                                            {
                                                                (void)xml_next_token(&xLoc, &xTok);
                                                                /* Only attribute should be returned. No value assigned to attribute Level */
                                                                if (xml_get_identifier(&xLoc, &xTok, value) == XML_ERROR)
                                                                {
                                                                    /* Error */
                                                                    xml_close(&xLoc);
                                                                    return DLEDS_ERROR;
                                                                }

                                                                /* Return attribute for element tag <modified> */
                                                                strncpy(pCurrentDluInfo->dluInfoHeader.modified,value,sizeof(pCurrentDluInfo->dluInfoHeader.modified));
                                                            }
                                                            else
                                                            {
                                                                fseek(xLoc.infile, filePos, SEEK_SET);
                                                                xLoc = xLocTmp;
                                                            }

                                                            /* <modified> is optional, OK if missing, Continue */

                                                            filePos = ftell(xLoc.infile);
                                                            xLocTmp = xLoc;
                                                            if ((res = xml_seek_start_tag(&xLoc,"size")) == XML_OK)
                                                            {
                                                                (void)xml_next_token(&xLoc, &xTok);
                                                                /* Only attribute should be returned. No value assigned to attribute Level */
                                                                if (xml_get_identifier(&xLoc, &xTok, value) == XML_ERROR)
                                                                {
                                                                    /* Error */
                                                                    xml_close(&xLoc);
                                                                    return DLEDS_ERROR;
                                                                }

                                                                /* Return attribute for element tag <size> */
                                                                strncpy(pCurrentDluInfo->dluInfoHeader.size,value,sizeof(pCurrentDluInfo->dluInfoHeader.size));
                                                            }
                                                            else
                                                            {
                                                                fseek(xLoc.infile, filePos, SEEK_SET);
                                                                xLoc = xLocTmp;
                                                            }

                                                            /* <size> is optional, OK if missing, Continue */

                                                            filePos = ftell(xLoc.infile);
                                                            xLocTmp = xLoc;
                                                            if ((res = xml_seek_start_tag(&xLoc,"checksum")) == XML_OK)
                                                            {
                                                                (void)xml_next_token(&xLoc, &xTok);
                                                                /* Only attribute should be returned. No value assigned to attribute Level */
                                                                if (xml_get_identifier(&xLoc, &xTok, value) == XML_ERROR)
                                                                {
                                                                    /* Error */
                                                                    xml_close(&xLoc);
                                                                    return DLEDS_ERROR;
                                                                }

                                                                /* Return attribute for element tag <checksum> */
                                                                strncpy(pCurrentDluInfo->dluInfoHeader.checksum,value,sizeof(pCurrentDluInfo->dluInfoHeader.checksum));
                                                            }
                                                            else
                                                            {
                                                                fseek(xLoc.infile, filePos, SEEK_SET);
                                                                xLoc = xLocTmp;
                                                            }

                                                            /* <checksum> is optional, OK if missing, Continue */

                                                            filePos = ftell(xLoc.infile);
                                                            xLocTmp = xLoc;
                                                            if ((res = xml_seek_start_tag(&xLoc,"crc32")) == XML_OK)
                                                            {
                                                                (void)xml_next_token(&xLoc, &xTok);
                                                                /* Only attribute should be returned. No value assigned to attribute Level */
                                                                if (xml_get_identifier(&xLoc, &xTok, value) == XML_ERROR)
                                                                {
                                                                    /* Error */
                                                                    xml_close(&xLoc);
                                                                    return DLEDS_ERROR;
                                                                }

                                                                /* Return attribute for element tag <crc32> */
                                                                strncpy(pCurrentDluInfo->dluInfoHeader.crc32,value,sizeof(pCurrentDluInfo->dluInfoHeader.crc32));
                                                            }
                                                            else
                                                            {
                                                                fseek(xLoc.infile, filePos, SEEK_SET);
                                                                xLoc = xLocTmp;
                                                            }

                                                            /* <crc32> is optional, OK if missing, Continue */

                                                            filePos = ftell(xLoc.infile);
                                                            xLocTmp = xLoc;
                                                            if ((res = xml_seek_start_tag(&xLoc,"address")) == XML_OK)
                                                            {
                                                                (void)xml_next_token(&xLoc, &xTok);
                                                                /* Only attribute should be returned. No value assigned to attribute Level */
                                                                if (xml_get_identifier(&xLoc, &xTok, value) == XML_ERROR)
                                                                {
                                                                    /* Error */
                                                                    xml_close(&xLoc);
                                                                    return DLEDS_ERROR;
                                                                }

                                                                /* Return attribute for element tag <address> */
                                                                strncpy(pCurrentDluInfo->dluInfoHeader.address,value,sizeof(pCurrentDluInfo->dluInfoHeader.address));
                                                            }
                                                            else
                                                            {
                                                                fseek(xLoc.infile, filePos, SEEK_SET);
                                                                xLoc = xLocTmp;
                                                            }

                                                            /* <address> is optional, OK if missing, Continue */

                                                            filePos = ftell(xLoc.infile);
                                                            xLocTmp = xLoc;
                                                            if ((res = xml_seek_start_tag(&xLoc,"supplier")) == XML_OK)
                                                            {
                                                                (void)xml_next_token(&xLoc, &xTok);
                                                                /* Only attribute should be returned. No value assigned to attribute Level */
                                                                if (xml_get_identifier(&xLoc, &xTok, value) == XML_ERROR)
                                                                {
                                                                    /* Error */
                                                                    xml_close(&xLoc);
                                                                    return DLEDS_ERROR;
                                                                }

                                                                /* Return attribute for element tag <supplier> */
                                                                strncpy(pCurrentDluInfo->dluInfoHeader.supplier,value,sizeof(pCurrentDluInfo->dluInfoHeader.supplier));
                                                            }
                                                            else
                                                            {
                                                                /* <supplier> is optional, OK if missing, Continue */
                                                                res = DLEDS_OK;
                                                                fseek(xLoc.infile, filePos, SEEK_SET);
                                                                xLoc = xLocTmp;
                                                            }

                                                            if (res == XML_OK)
                                                            {
                                                                /* Complete DLU parsed, increment no of found DLU */
                                                                (pCurrentSciInfo->noOfDlu)++;
                                                                /* Prepare for next DLU */
                                                                pCurrentDluInfo->pNextDluInfo = (DLU_Info*)malloc(sizeof(DLU_Info));
                                                                pCurrentDluInfo = pCurrentDluInfo->pNextDluInfo;
                                                                memset(pCurrentDluInfo, 0, sizeof(DLU_Info));
                                                            }

                                                        } /* end <version> */
                                                        else
                                                        {
                                                            /* mandatory tag <version> missing */
                                                            pCurrentSciInfo->noOfDlu = 0;
                                                        }
                                                    } /* end <name> */
                                                    else
                                                    {
                                                        /* mandatory tag <name> missing */
                                                        pCurrentSciInfo->noOfDlu = 0;
                                                    } 
                                                    xml_leave(&xLoc);               
                                                } /* end while <DLU> */
                                                /* Check that at least 1 DLU found */
                                                if ((pCurrentSciInfo->noOfDlu) > 0)
                                                {
                                                    res = DLEDS_OK;
                                                }
                                                xml_leave(&xLoc);
                                            } /* end <DLU_List> */

                                            if (res == XML_OK)
                                            {
                                                /* Complete SCI parsed, increment no of found SCI */
                                                (pEdspInfo->noOfSci)++;
                                                /* printf("No of found SCI = (%d)\n",pEdspInfo->noOfSci); */
                                                /* Prepare for next SCI */
                                                pCurrentSciInfo->pNextSciInfo = (SCI_Info*)malloc(sizeof(SCI_Info));
                                                pCurrentSciInfo = pCurrentSciInfo->pNextSciInfo;
                                                memset(pCurrentSciInfo, 0, sizeof(SCI_Info));
                                            }



                                        } /* end <SCI_DeviceType> */
                                        else
                                        {
                                            /* mandatory tag <SCI_DeviceType> missing */
                                            pEdspInfo->noOfSci = 0;
                                        }
                                    } /* end <SCI_Version> */
                                    else
                                    {
                                        /* mandatory tag <SCI_Version> missing */
                                        pEdspInfo->noOfSci = 0;
                                    }
                                } /* end <SCI_Name> */
                                else
                                {
                                    /* mandatory tag <SCI_Name> missing */
                                    pEdspInfo->noOfSci = 0;
                                }
                                xml_leave(&xLoc);              
                            } /* end while <SCI> */

                            /* Check that at least 1 SCI found */
                            if ((pEdspInfo->noOfSci) > 0)
                            {
                                res = DLEDS_OK;
                            }
                        } /* end <SCI_List> */
                    } /* end <EDSP_Source> */  
                } /* end <EDSP_EndDeviceType> */
            } /* end <EDSP_Version> */
        } /* end <EDSP_Name> */
    }/* end <EDSP_Info> */
    xml_close(&xLoc);
    return res;
}
