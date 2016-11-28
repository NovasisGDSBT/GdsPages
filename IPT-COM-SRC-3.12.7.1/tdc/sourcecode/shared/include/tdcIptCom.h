/*                                                                            */
/* $Id: tdcIptCom.h 11626 2010-08-16 13:33:12Z bloehr $                        */
/*                                                                            */
/* DESCRIPTION                                                                */
/*                                                                            */
/* AUTHOR         M.Ritz         PPC/EBT                                      */
/*                                                                            */
/* REMARKS                                                                    */
/*                                                                            */
/* DEPENDENCIES                                                       		  */
/*                                                                     		  */
/*  MODIFICATIONS (log starts 2010-08-11)									  */
/*   																		  */
/*  CR-382 (Bernd Loehr, 2010-08-11)   										  */
/* 			Additional inauguration state LOCAL	     						  */
/*                                                                            */
/*                                                                            */
/* All rights reserved. Reproduction, modification, use or disclosure         */
/* to third parties without express authority is forbidden.                   */
/* Copyright Bombardier Transportation GmbH, Germany, 2002.                   */
/*                                                                            */

/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */

#if !defined (_TDC_IPTCOM_H)
   #define TDC_IPTCOM_H

/* ---------------------------------------------------------------------------- */

#ifdef __cplusplus                            /* to be compatible with C++*/
   extern "C" {
#endif

/* ---------------------------------------------------------------------------- */

#define TDC_MAX_FILENAME_LEN           200
#define TEST_IPTDIR_IPT_MD_FILE        "tdcTestIptMD.bin"
#define TEST_IPTDIR_UIC_MD_FILE        "tdcTestUicMD.bin"
#define TEST_IPTDIR_PD_FILE            "tdcTestPD.bin"

//
//#define TEST_IPTDIR_IPT_MD_FILE        "../../test/config/tdcTestIptMD.bin"
//#define TEST_IPTDIR_UIC_MD_FILE        "../../test/config/tdcTestUicMD.bin"
//#define TEST_IPTDIR_PD_FILE            "../../test/config/tdcTestPD.bin"
//
//#if defined (VXWORKS)
//   #undef  TEST_IPTDIR_IPT_MD_FILE
//   #undef  TEST_IPTDIR_UIC_MD_FILE
//   #undef  TEST_IPTDIR_PD_FILE
//   #define TEST_IPTDIR_IPT_MD_FILE        "/tffs0/tdcTestIptMD.bin"
//   #define TEST_IPTDIR_UIC_MD_FILE        "/tffs0/tdcTestUicMD.bin"
//   #define TEST_IPTDIR_PD_FILE            "/tffs0/tdcTestPD.bin"
//#else
//
//#endif

/* ---------------------------------------------------------------------------- */

typedef enum
{
   RECV_IPT_MSG_DATA = 0,
   RECV_UIC_MSG_DATA,
} T_IPTDIR_RECV_MD_TYPE;

typedef enum
{
   SEND_IPT_MSG_DATA = 0,
} T_IPTDIR_SEND_MD_TYPE;


typedef struct
{
   char*                	pMsgData;
   UINT32               	msgLen;           /* max msgLen at call - total msgLen at return */
   INT32                	timeout;          /* msec, or -1 forever */
   T_IPT_IP_ADDR        	srcIpAddr;
   T_IPTDIR_RECV_MD_TYPE   msgDataType;
} T_READ_MD;

typedef struct
{
   void*                	pMsgData;
   UINT32               	msgLen;
   T_IPTDIR_SEND_MD_TYPE 	msgDataType;
   T_IPT_URI            	destUri;
} T_WRITE_MD;

/* ---------------------------------------------------------------------------- */

typedef enum
{
   RECV_IPT_PROC_DATA = 0
} T_IPTDIR_PD_TYPE;

typedef struct
{
   void*                pProcData;
   UINT32               msgLen;           /* max msgLen at call - total msgLen at return */
   T_IPTDIR_PD_TYPE     procDataType;
} T_READ_PD;

/* ---------------------------------------------------------------------------- */

extern void          tdcTerminateMD         (           const char*           pModname);
extern void          tdcTerminatePD         (           const char*           pModname);

extern void          tdcInitIptCom          (           const char*           pModname);
extern T_TDC_BOOL    tdcReadProcData        (           const char*           pModname,
                                              /*@out@*/ const T_READ_PD*      pReadProcData);
extern T_TDC_BOOL    tdcReadMsgData         (           const char*           pModname,
                                              /*@out@*/ T_READ_MD*            pReadMsgData);
extern T_TDC_BOOL    tdcWriteMsgData        (           const char*           pModname,
                                                        const T_WRITE_MD*     pWriteMsgData);
extern int           tdcFreeMDBuf           (           char*                 pBuf);
extern void          tdcSetIPTDirServerEmul (           T_TDC_BOOL            bEmulate);
extern T_TDC_BOOL 	 tdcGetIPTDirServerEmul ();
extern void          tdcSetComId100Filename (           const char*           pFilename);
extern void          tdcSetComId101Filename (           const char*           pFilename);
extern void          tdcSetComId102Filename (           const char*           pFilename);

extern const char* 	tdcGetComId100Filename ();       
extern const char* 	tdcGetComId101Filename ();
extern const char* 	tdcGetComId102Filename ();


#ifdef __cplusplus                            /* to be compatible with C++ */
   }
#endif





#endif



