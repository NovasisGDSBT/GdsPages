/*                                                                            */
/*  $Id: tdcXML2bin.c 37060 2015-05-20 14:13:16Z rscheja $                      */
/*                                                                            */
/*  DESCRIPTION    TDC cstSta.xml to binary conversion                        */
/*                 UIC data handling										  */
/*                                                                            */
/*  AUTHOR         B.Loehr/G.Weiss, NewTec GmbH                               */
/*                                                                            */
/*  REMARKS        CR-382                                                     */
/*				   Most code borrowed from etbhd_test and iptdir_wire of	  */
/*				   IPT-SW-FW												  */
/*                                                                            */
/*  DEPENDENCIES   tdcPpicoxml.c                                              */
/*                                                                            */
/*  MODIFICATIONS  CR-10060 TDC Stack overrun								  */
/*   																		  */
/*                 Modify tdcXML2Packets so that the stack is not used for    */
/*                 temporary storage of the packets.                          */
/*                                                                            */
/*  All rights reserved. Reproduction, modification, use or disclosure        */
/*  to third parties without express authority is forbidden.                  */
/*  Copyright Bombardier Transportation GmbH, Germany, 2002-2010.             */
/*                                                                            */


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

#include "tdc.h"
#include "tdcConfig.h"
#include "tdcIptCom.h"
#include "tdcMisc.h"
#include "tdcIptdir.h"
#include "tdcIptdir_wire.h"
#include "tdcCst_info.h"

// CR-382 ---------------------------------------------------------------------

// ----------------------------------------------------------------------------

T_TDC_BOOL tdcBuffer2File (
	const char*    	pFilename,
    const UINT8* 	pBuffer,
    UINT32    		bufSize)
{
	T_TDC_BOOL	retValue 	= FALSE;
    char     	text[200]	= "";
    
    if (pFilename != NULL)
    {
		T_FILE*        pFile = tdcFopen (pFilename, "w+b");
		
		if (pFile != NULL)
		{
			if (tdcFwrite (pBuffer, (UINT32) 1, bufSize, pFile) != bufSize)
            {
				tdcSNPrintf (text, (UINT32) sizeof (text), 
						"tdcBuffer2File: Failed to write to File(%s)", pFilename);
            }
            else
            {
            	retValue = TRUE;        
            }

            (void) tdcFclose (pFile);
        }
        else
        {
         	tdcSNPrintf (text, (UINT32) sizeof (text), 
                     	"tdcBuffer2File: Failed to open File(%s)", pFilename);
        }
    }
    else
    {
        tdcStrNCpy (text, "tdcBuffer2File: No Filename specified",
         			(UINT32) sizeof (text));
    }
    
    if (text[0] != '\0')        			// Is there a Warning to issue?
    {
        DEBUG_WARN  (MOD_MAIN, text);
    }
    
    return retValue;
}


/******************************************************************************/
/*
 	Read cstSta.xml file and create the three ready-to-send packets as
 	binary files.
 */

T_TDC_BOOL tdcXML2Packets(
	T_IPT_IP_ADDR	sourceIP,
	const char 		*pCstSta_path,
	const char		*pPD100_path,
    const char		*pPD101_path,
    const char		*pPD102_path)
{
    IPT_CST_T               *pMy_cst;
    T_TDC_BOOL              retValue = FALSE;
    UINT8*                  ipt_traindataset;
    UINT8*                  uic_traindataset;
    IPTDIR_PROCESSDATASET_T iptdir_processdataset;
    INT32                   ipt_traindataset_len;
    INT32                   uic_traindataset_len;
    
    /*
     * Read XML file and return consist data structure, with NULL as nextpointer.
     * I.e. taking pMy_cst as "root" yields a one-consist train
     */

    pMy_cst = cst_info_read_xml_file(pCstSta_path);

    if (NULL == pMy_cst)
    {
         return retValue;
    }

    cst_info_uic_inauguration(pMy_cst, FALSE /*opt.uic_reversed*/);
    
    /*
     *   Generate IPTDir PD and IPT/UIC MD from my_cst and inaug count/status
     *   information.
     */
    
    ipt_traindataset = (UINT8 *)tdcAllocMem(32768);
    uic_traindataset = (UINT8 *)tdcAllocMem(32768);
    
    if (NULL == ipt_traindataset)
    {
        return retValue;
    }
	if (NULL == uic_traindataset)
	{
		tdcFreeMem(ipt_traindataset);
		return retValue;
	}

    if (iptdir_wire(pMy_cst,
                    2, 1, 2, 1, 1,
                    2, 2, sourceIP,
                    &iptdir_processdataset,
                    ipt_traindataset, 32768, &ipt_traindataset_len,
                    uic_traindataset, 32768,
                    &uic_traindataset_len) == 0)
    {
    	if (tdcBuffer2File (pPD100_path, (UINT8*) &iptdir_processdataset, sizeof(iptdir_processdataset)) == TRUE &&
        	tdcBuffer2File (pPD101_path, ipt_traindataset, ipt_traindataset_len) == TRUE &&
            tdcBuffer2File (pPD102_path, uic_traindataset, uic_traindataset_len) == TRUE)
            
        {
            retValue = TRUE;
        }
    }
    
    /*	Free all malloced space!	*/
    cst_info_free_cst(pMy_cst, NULL);

    tdcFreeMem(ipt_traindataset);
    tdcFreeMem(uic_traindataset);
    
    return retValue;
}


