/*
 * $Id: tdcCst_info.h 11632 2010-08-16 15:33:12Z bloehr $
 */

/**
 * @file            tdcCst_info.h
 *
 * @brief           Ethernet train backbone handler utilities
 *
 * @details         Parsing XML file and constructing internal database.
 *
 * @note            Project: TCMS IP-Train - Ethernet train backbone handler
 *
 * @author          Thomas Gallenkamp, PPC/EMTC
 *
 * @remarks All rights reserved. Reproduction, modification, use or disclosure
 *          to third parties without express authority is forbidden,
 *          Copyright Bombardier Transportation GmbH, Germany, 2006.
 *
 */

#ifndef TDC_CST_INFO_H
#define TDC_CST_INFO_H

/*******************************************************************************
 * INCLUDES
 */
#include <stdio.h>
#include "tdc.h"
#include "tdcIptdir.h"

/*******************************************************************************
 * DEFINITIONS
 */

/*******************************************************************************
 *  TYPEDEFS
 */

/*******************************************************************************
 *  GLOBALS
 */

/*******************************************************************************
 * GLOBAL FUNCTIONS
 */

INT32     cst_info_get_cst_no_last(const IPT_CST_T *cst_root);
INT32     cst_info_get_cst_no_local(const IPT_CST_T *cst_root);
IPT_CST_T *cst_info_read_xml_file(const char *filename);

void      cst_info_free_cst(IPT_CST_T *p,
                            IPT_CST_T *keep_this);
INT32     cst_info_sprint_cst_tcl(char            *buf,
                                  const IPT_CST_T *cst);
void      cst_info_cst_set_topo(IPT_CST_T *cst,
                                UINT32     topo);
void      cst_info_uic_inauguration(IPT_CST_T *cst,
                                    INT32     is_reverse);

#endif /* CST_INFO_H */
