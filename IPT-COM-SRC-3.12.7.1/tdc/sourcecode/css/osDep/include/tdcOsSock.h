/*                                                                      */
/*  $Id: tdcOsSock.h 11763 2010-12-02 09:24:10Z gweiss $              */
/*                                                                      */
/*  DESCRIPTION                                                         */
/*                                                                      */
/*  AUTHOR         M.Ritz          PPC/EBTS                             */
/*                                                                      */
/*  REMARKS                                                             */
/*                                                                      */
/*  DEPENDENCIES                                                        */
/*                                                                      */
/*                                                                      */
/*  All rights reserved. Reproduction, modification, use or disclosure  */
/*  to third parties without express authority is forbidden.            */
/*  Copyright Bombardier Transportation GmbH, Germany, 2002.            */

/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */

#if !defined (_TDC_OS_SOCK_H)
   #define _TDC_OS_SOCK_H

#include <netinet/in.h>

/* ---------------------------------------------------------------------------- */

#ifdef __cplusplus                            /* to be compatible with C++ */
extern "C" {
#endif

/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */

#if ! defined (INADDR_ANY)
   #define INADDR_ANY                       ((u_long) 0)
#endif

#if !defined (SOMAXCONN)
   #define SOMAXCONN                        4   /* from VxWorks Programmer's Guide */
#endif

#if !defined (LISTENQ)
   #define LISTENQ                              SOMAXCONN
#endif

#define TDC_INVALID_SOCKET                  -1

/* used with shutdown() (tdcShutDown()) */
#define TDC_DISALLOW_RECEIVE                0
#define TDC_DISALLOW_SEND                   1
#define TDC_DISALLOW_SENDRECV               2

/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */

typedef int                   T_TDC_SOCKET;
typedef int                   T_TDC_REUSEADDR;
typedef int                   T_TDC_SOCKLEN_T;
typedef struct sockaddr       T_TDC_SOCK_ADDR;
typedef struct sockaddr_in    T_TDC_SOCK_ADDR_IN;
typedef UINT32                T_TDC_TIMEVAL;         /* milli-seconds */


/* MRi, 2005/06/15, No support for IPv6 on CSS/VxWorks platforms at the moment! */

#define tdcTcpBind           tdcTcpBind4
#define tdcTcpConnect        tdcTcpConnect4

#define tdcClientUdpSock     tdcClientUdpSock4
#define tdcServerUdpSock     tdcServerUdpSock4


/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */

#ifdef __cplusplus                            /* to be compatible with C++ */
}
#endif


#endif

