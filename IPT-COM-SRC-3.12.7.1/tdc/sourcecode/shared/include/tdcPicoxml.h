/*
 * $Id: tdcPicoxml.h 11642 2010-08-19 12:35:17Z bloehr $
 */

/**
 * @file            tdcPicoxml.h
 *
 * @brief           Small footprint XML parser
 *
 * @details         Functions for parsing an xml file from a buffered FILE*
 *
 * @note            Project: TCMS IP-Train - Ethernet train backbone handler
 *
 * @author          Thomas Gallenkamp, PPC/EMTC
 *
 * @remarks All rights reserved. Reproduction, modification, use or disclosure
 *          to third parties without express authority is forbidden,
 *          Copyright Bombardier Transportation GmbH, Germany, 2004-2009.
 *
 */

#ifndef TDC_PICOXML_H
#define TDC_PICOXML_H

/*******************************************************************************
 * INCLUDES
 */
#include "tdc.h"
#include "iptDef.h"

/*******************************************************************************
 * DEFINITIONS
 */
 
#define PICOXML_MAX_TAGLEN      132     /**< The max length of string buffer*/

/*******************************************************************************
 * GLOBAL FUNCTIONS
 */

/*
 * picoxml_init() intializes XML parser to use FILE stream pointed to by <in>
 *
 * Return value:      0: OK
 */
INT32 picoxml_init(T_FILE *in);

/*
 * picoxml_seek_any() "seeks" to the next XML start tag on the current
 *                    nesting level. The user supplied <tag> buffer is
 *                    filled with the XML start tag, it's size is limited
 *                    by <maxlen> (including final \000 termination
 *                    character)
 * Return value:      0: start tag found. <tag> is valid
 *                   -1: no further tags on this nesting level
 *                   -2: fatal error (e.g. unexpected end of file)
 */
INT32 picoxml_seek_any(	char    *tag,
						UINT32  maxlen);


/*
 * picoxml_seek()     "seeks" to the next XML start tag named <tag>
 *                    on the current nesting level.
 * Return value:      0: Tag  <tag> found.
 *                   -1: no further tags name <tag> on this nesting level
 *                   -2: fatal error (e.g. unexpected end of file)
 */
INT32 picoxml_seek(const char *tag);

/*
 * picoxml_enter()   "enters" into subordinate nesting level. Further
 *                   calls to picoxml_seek() and picoxml_seek_any() work
 *                   on this nesting level.
 */
void picoxml_enter(void);

/*
 * picoxml_leave()   "leaves"  subordinate nesting level. Further
 *                   calls to picoxml_seek() and picoxml_seek_any() work
 *                   on this nesting level.
 */
void picoxml_leave(void);

/*
 * picoxml_data()    is called after picoxml_seek() and picoxml_seek_any()
 *                   to retrieve payload <data> between XML start and end
 *                   tags. <maxlen> specifies the length of the user supplied
 *                   buffer (including the \000 ternination character).
 *                   Data. If the payload contains "white space" multiple
 *                   calls to picoxml_data() are required to retrieve individual
 *                   items.
 *
 * Return value:      0: <data> available..
 *                   -1: no further <data> available (XML end tag reached)
 *                   -2: fatal error (e.g. unexpected end of file)
 *                   -3: indicates: not at "leaf" level. Do a enter/seek
 *                       combination and try again. <data> is not updated
 */
INT32 picoxml_data(	char    *data,
					UINT32  maxlen);

#endif /* PICOXML_H */
