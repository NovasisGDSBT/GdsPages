/*
 * $Id: tdcCrc32.h 11619 2010-07-22 10:04:00Z bloehr $
 */

/**
 * @file            tdcCrc32.h
 *
 * @brief           CRC32 generation of a data stream
 *
 * @note            Project: TDC, copied from
 *					TCMS IP-Train - Ethernet train backbone handler
 *
 * @author
 *      This file is derived from crc32.c from the zlib-1.1.3 distribution
 *      by Jean-loup Gailly and Mark Adler.
 *
 * @remarks     Copyright (C) 1995-1998 Mark Adler
 *      For conditions of distribution and use, see copyright notice in zlib.h
 *
 */

#ifndef CRC32_H
#define CRC32_H

#ifdef __cplusplus   /* to be compatible with C++ */
extern "C" {
#endif

/*******************************************************************************
 * INCLUDES
 */

#include "iptDef.h"

/*******************************************************************************
 * GLOBAL FUNCTIONS
 */

UINT32 crc32(UINT32 crc, const UINT8 *buf, UINT32 len);


#ifdef __cplusplus   /* to be compatible with C++ */
}
#endif


#endif  /* CRC32_H */
