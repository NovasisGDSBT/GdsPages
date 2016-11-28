//
// $Id: tdcOsSock.h 11018 2008-10-15 15:13:56Z tgkamp $
//
// DESCRIPTION
//
// AUTHOR         M.Ritz			PPC/EBT
//
// REMARKS
//
// DEPENDENCIES
//
//
// All rights reserved. Reproduction, modification, use or disclosure
// to third parties without express authority is forbidden.
// Copyright Bombardier Transportation GmbH, Germany, 2002.
//

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------


#if !defined (_TDC_OS_SOCK_H)
   #define _TDC_OS_SOCK_H

// ----------------------------------------------------------------------------

#ifdef __cplusplus                            // to be compatible with C++
   extern "C" {
#endif

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

#define TDC_INVALID_SOCKET                   (T_TDC_SOCKET) (~0)

// used with shutdown() (tdcShutDown())
#define TDC_DISALLOW_RECEIVE                 0
#define TDC_DISALLOW_SEND                    1
#define TDC_DISALLOW_SENDRECV                2

#define SOCKET_RECV_SEND_FLAGS               0     /* NO_FLAGS_SET */

// ----------------------------------------------------------------------------


#define  LISTENQ                             1024    // 2nd argument to listen()
                                             // WInsock2.h --> #define SOMAXCONN       0x7fffffff


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

typedef int                   socklen_t;


typedef int                   T_TDC_STREAM;
typedef UINT32                T_TDC_SOCKET;
typedef int                   T_TDC_REUSEADDR;
typedef UINT32                T_TDC_SOCKLEN_T;
typedef void                  T_TDC_SOCK_ADDR;
typedef void                  T_TDC_SOCK_ADDR_IN;
typedef UINT32                T_TDC_TIMEVAL;         /* milli-seconds */

#define O_IPV4_SUPPORT_ONLY
//#undef O_IPV4_SUPPORT_ONLY       // comment this line, if only IPv4 shall be supported

#if defined (O_IPV4_SUPPORT_ONLY)
   #define tdcTcpBind           tdcTcpBind4
   #define tdcTcpConnect        tdcTcpConnect4
   #define tdcClientUdpSock     tdcClientUdpSock4
   #define tdcServerUdpSock     tdcServerUdpSock4
   
#else   
   #define tdcTcpBind           tdcTcpBind46
   #define tdcTcpConnect        tdcTcpConnect46
   #define tdcClientUdpSock     tdcClientUdpSock46
   #define tdcServerUdpSock     tdcServerUdpSock46
#endif   

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------


#ifdef __cplusplus                            // to be compatible with C++
   }
#endif


#endif



