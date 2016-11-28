/*                                                                     */
/* $Id: tdcXML2bin.h 11619 2010-07-22 10:04:00Z bloehr $             */
/*                                                                     */
/* DESCRIPTION    TDC Parsing cstSta.xml                               */
/*                                                                     */
/* AUTHOR         B. Loehr, NewTec GmbH							       */
/*                                                                     */
/* REMARKS                                                             */
/*                                                                     */
/* DEPENDENCIES                                                        */
/*                                                                     */
/*                                                                     */
/* All rights reserved. Reproduction, modification, use or disclosure  */
/* to third parties without express authority is forbidden.            */
/* Copyright Bombardier Transportation GmbH, Germany, 2010.            */
/*                                                                     */

/* ---------------------------------------------------------------------------- */

#ifndef TDC_XML2BIN_H
#define TDC_XML2BIN_H

/* ---------------------------------------------------------------------------- */

#if defined (__cplusplus)
extern "C" {
#endif

/* ---------------------------------------------------------------------------- */

#include "iptDef.h"

/* ---------------------------------------------------------------------------- */

extern T_TDC_BOOL tdcBuffer2File (
    const char*    	pFilename,
    const UINT8*	pBuffer,
    UINT32    		bufSize);
       
extern T_TDC_BOOL tdcXML2Packets(
    T_IPT_IP_ADDR	sourceIP,
    const char 		*pCstSta_path,
    const char		*pPD100_path,
    const char		*pPD101_path,
    const char		*pPD102_path);
    
/* ---------------------------------------------------------------------------- */

#if defined (__cplusplus)
}
#endif

#endif
