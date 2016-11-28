/*                                                                     */
/* $Id: tdcSock.h 11018 2008-10-15 15:13:56Z tgkamp $                */
/*                                                                     */
/* DESCRIPTION                                                         */
/*                                                                     */
/* AUTHOR         M.Ritz         PPC/EBT                               */
/*                                                                     */
/* REMARKS                                                             */
/*                                                                     */
/* DEPENDENCIES                                                        */
/*                                                                     */
/*                                                                     */
/* All rights reserved. Reproduction, modification, use or disclosure  */
/* to third parties without express authority is forbidden.            */
/* Copyright Bombardier Transportation GmbH, Germany, 2002.            */
/*                                                                     */

/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */

#if !defined (_TDC_SOCK_H)
   #define _TDC_SOCK_H

#include "tdcOsSock.h"

/* ---------------------------------------------------------------------------- */

#ifdef __cplusplus                            /* to be compatible with C++ */
   extern "C" {
#endif

/* ---------------------------------------------------------------------------- */

extern UINT8   tdcH2N8  (UINT8      var8);
extern UINT16  tdcH2N16 (UINT16     var16);
extern UINT32  tdcH2N32 (UINT32     var32);

extern UINT8   tdcN2H8  (UINT8      var8);
extern UINT16  tdcN2H16 (UINT16     var16);
extern UINT32  tdcN2H32 (UINT32     var32);

/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */

#define tdcH2N1(x)                (sizeof (x) == 1)                \
                                    ? tdcH2N8(x)                  \
                                    : (sizeof (x) == 2)           \
                                       ? tdcH2N16 (x)             \
                                       : tdcH2N32 (x)


#define tdcN2H1(x)                (sizeof (x) == 1)                \
                                    ? tdcN2H8(x)                  \
                                    : (sizeof (x) == 2)           \
                                       ? tdcN2H16 (x)             \
                                       : tdcN2H32 (x)

/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */

/* Discard Sendbuffer on close */

#define LINGER_ON_DISCARD_SENDBUFFER            TRUE
#define LINGER_SECS_DISCARD_SENDBUFFER          ((UINT32) 0)

/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */



#define TDC_TIMEVAL_1_SEC                         ((T_TDC_TIMEVAL) 1000)
#define TDC_TIMEVAL_10_SEC                        ((T_TDC_TIMEVAL) 10000)

typedef struct
{
   UINT16            portNo;
   char              ipAddr[100];
} T_TDC_SERVER_ADDR;


typedef struct
{
   T_TDC_BOOL        lingerOn;
   UINT32            seconds;
} T_TDC_SOCKET_LINGER;

typedef struct
{
   T_TDC_TIMEVAL     milliSeconds;
} T_TDC_SOCKET_TIMEOUT;

typedef struct
{
   T_TDC_BOOL        bReuse;
} T_TDC_SOCKET_REUSEADDR;

typedef struct
{
   /*@null@*/ const T_TDC_SOCKET_LINGER*       pLingerOpt;          /* Set Linger Option        if not NULL */
   /*@null@*/ const T_TDC_SOCKET_TIMEOUT*      pSendTimeout;        /* Set Send Timeout Option  if not NULL */
   /*@null@*/ const T_TDC_SOCKET_TIMEOUT*      pRecvTimeout;        /* Set Recv Timeout Option  if not NULL */
   /*@null@*/ const T_TDC_SOCKET_REUSEADDR*    pReuseOpt;           /* Set Reuse Address Option if not NULL */
} T_TDC_SOCK_OPTIONS;


typedef struct
{
   const T_TDC_SOCK_OPTIONS*        pSockOpt;
         UINT32                     timeout;             /* seconds, 0 --> default (75 sec) */
         T_TDC_SERVER_ADDR          serverAddr;          /* Server address            */
         T_TDC_SOCKET*              pSockFd;             /* Client Socket on success  */
} T_TDC_CONNECT_PAR;

typedef struct
{
   /*@shared@*/ const T_TDC_SOCK_OPTIONS*        pSockOpt;
                UINT16                           portNo;
                T_TDC_SOCKLEN_T                  addrLen;
   /*@shared@*/ T_TDC_SOCKET*                    pListenFd;
} T_TDC_LISTEN_PAR;

/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */

void                tdcInitTcpIpWinsock (const char*                 pModname);
                                                                                 /*@ -exportlocal */
extern T_TDC_BOOL   tdcShutDown         (const char*                 pModname,
                                         T_TDC_SOCKET                socketFd,
                                         int                         how);       /*@ =exportlocal */

extern T_TDC_BOOL   tdcCloseSocket      (const char*                 pModname,
                                         T_TDC_SOCKET*               pSockFd);

extern T_TDC_BOOL   tdcTcpListen        (const char*                 pModname,
                                         T_TDC_LISTEN_PAR*           pListenPar);
                                                                                 /*@ -exportlocal */
extern T_TDC_BOOL   tdcSetSocketOptions (const char*                 pModname,
                                         T_TDC_SOCKET                sockFd,
                                         const T_TDC_SOCK_OPTIONS*   pSockOpt);  /*@ =exportlocal */

/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */

extern T_TDC_BOOL   getInetAddr         (const char*                    pServAddr,
                                         /*@out@*/ T_TDC_SOCK_ADDR_IN*  pSockAddr);

extern T_TDC_SOCKET Socket              (int                         family,
                                         int                         type,
                                         int                         protocol);
                                                                                 /*@ -exportlocal */
extern T_TDC_BOOL   tdcTcpSocket4       (const char*                 pModname,
                                         /*@out@*/ T_TDC_SOCKET*     pSockFd);   /*@ =exportlocal */
                                                                                 /*@ -exportlocal */
extern T_TDC_BOOL   tdcUdpSocket4       (const char*                 pModname,
                                         /*@out@*/ T_TDC_SOCKET*     pSockFd);   /*@ =exportlocal */

extern T_TDC_BOOL   Connect             (T_TDC_SOCKET                sockFd,
                                         const T_TDC_SOCK_ADDR*      pServerAddr,
                                         int                         sockAddrSize);
extern T_TDC_BOOL   ConnectTimo         (T_TDC_SOCKET                sockFd,
                                         const T_TDC_SOCK_ADDR*      pServerAddr,
                                         int                         sockAddrSize,
                                         UINT32                      timeout);
extern T_TDC_BOOL   Bind                (T_TDC_SOCKET                sockFd,
                                         const T_TDC_SOCK_ADDR*      pMyAddr,
                                         int                         addrLen);
extern T_TDC_BOOL   Listen              (T_TDC_SOCKET                sockFd,  
                                         int                         backlog);
extern T_TDC_SOCKET Accept              (const char*                 pModname,
                                         T_TDC_SOCKET                fd,
                                         /*@null@*/ T_TDC_SOCK_ADDR* pSa,
                                         /*@null@*/ T_TDC_SOCKLEN_T* pSalen);
extern T_TDC_BOOL   ShutDown            (T_TDC_SOCKET                socketFd, 
                                         int                         how);
extern T_TDC_BOOL   CloseSocket         (T_TDC_SOCKET                sockFd);

extern T_TDC_BOOL   tdcTcpRecvN         (const char*                 pModname,
                                         T_TDC_SOCKET                fdRd,
                                         /*@out@*/ void*             pRecvBuf,
                                         /*@out@*/ UINT32*           pBytesReceived,
                                         UINT32                      bytesExpected);
extern T_TDC_BOOL   tdcUdpRecvN         (const char*                 pModname,
                                         T_TDC_SOCKET                fdRd,
                                         /*@out@*/ void*             pRecvBuf,
                                         UINT16                      bytesExpected);
extern T_TDC_BOOL   tdcTcpSendN         (const char*                 pModname,
                                         T_TDC_SOCKET                fdWr,
                                         void*                       pSendBuf,
                                         /*@out@*/ UINT32*           pBytesSent,
                                         UINT32                      bytesExpected);
extern T_TDC_BOOL   tdcUdpSendN         (const char*                 pModname,
                                         T_TDC_SOCKET                fdWr,
                                         void*                       pSendBuf,
                                         UINT16                      bytesExpected,
                                         void*                       pServerAddr,
                                         T_TDC_SOCKLEN_T             serverSockLen);
                                                                                    /*@ -exportlocal */
extern T_TDC_BOOL   tdcTcpBind4         (const char*                 pModname,
                                         T_TDC_LISTEN_PAR*           pBindPar);     /*@ =exportlocal */

extern T_TDC_BOOL   tdcTcpBind46        (const char*                 pModname,
                                         T_TDC_LISTEN_PAR*           pBindPar);
                                                                                    /*@ -exportlocal */
extern T_TDC_BOOL   tdcTcpConnect4      (const char*                 pModname,
                                         T_TDC_CONNECT_PAR*          pConnPar);     /*@ =exportlocal */
extern T_TDC_BOOL   tdcTcpConnect46     (const char*                 pModname,
                                         T_TDC_CONNECT_PAR*          pConnPar);

extern T_TDC_BOOL   tdcServerUdpSock4   (const char*                 pModname,
                                         T_TDC_SOCKET*               pServerSockFd, 
                                         UINT16                      portNo);
extern T_TDC_BOOL   tdcServerUdpSock46  (const char*                 pModname,
                                         T_TDC_SOCKET*               pServerSockFd, 
                                         UINT16                      portNo);

extern T_TDC_BOOL   tdcClientUdpSock4   (const char*                 pModnamee,
                                         T_TDC_SOCKET*               pClientSockFd,
                                         T_TDC_SERVER_ADDR*          pRemAddr,
                                         /*@out@*/ void**            ppServerAddr,
                                         T_TDC_SOCKLEN_T*            pServerSockLen);
extern T_TDC_BOOL   tdcClientUdpSock46  (const char*                 pModname,
                                         T_TDC_SOCKET*               pClientSockFd,
                                         T_TDC_SERVER_ADDR*          pRemAddr,
                                         void**                      ppServerAddr,
                                         T_TDC_SOCKLEN_T*            pServerSockLen);

/* ---------------------------------------------------------------------------- */

extern int tdcGet_AF_INET           (void);
extern int tdcGet_SOCK_DGRAM        (void);
extern int tdcGet_SOCK_STREAM       (void);

extern void  tdcInitCli_SERV_ADDR   (const char*            pModname,
                                     T_TDC_SOCK_ADDR_IN*    pServAddr,
                                     UINT32*                pLen,
                                     UINT16                 portNo);

extern void  tdcInitSrv_SERV_ADDR   (const char*            pModname,
                                     T_TDC_SOCK_ADDR_IN*    pServAddr,
                                     UINT32*                pLen,
                                     UINT16                 portNo);


extern T_TDC_BOOL setSockTimeoutOpt (const char*            pModname,
                                     T_TDC_SOCKET           sockFd,
                                     T_TDC_TIMEVAL          timeout,
                                     int                    optName);

extern T_TDC_BOOL setLingerOpt      (const char*            pModname,
                                     T_TDC_SOCKET           sockFd,
                                     T_TDC_BOOL             lingerOn,
                                     UINT32                 seconds);

extern T_TDC_BOOL setReuseAddrOpt   (const char*            pModname,
                                     T_TDC_SOCKET           sockFd,
                                     T_TDC_BOOL             bReuseAddr);

extern T_TDC_BOOL setRecvTimeoutOpt (const char*            pModname,
                                     T_TDC_SOCKET           sockFd,
                                     T_TDC_TIMEVAL          timeout);


extern T_TDC_BOOL setSendTimeoutOpt (const char*            pModname,
                                     T_TDC_SOCKET           sockFd,
                                     T_TDC_TIMEVAL          timeout);

/* ---------------------------------------------------------------------------- */

#ifdef __cplusplus                            /* to be compatible with C++ */
   }
#endif

/* ---------------------------------------------------------------------------- */

#endif

