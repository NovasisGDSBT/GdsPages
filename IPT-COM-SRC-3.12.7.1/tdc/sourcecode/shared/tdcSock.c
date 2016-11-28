/*                                                                           */
/* $Id: tdcSock.c 11018 2008-10-15 15:13:56Z tgkamp $                      */
/*                                                                           */
/* DESCRIPTION    OS-Independent part of Ethernet Management                 */
/*                                                                           */
/* AUTHOR         M.Ritz         PPC/EBT                                     */
/*                                                                           */
/* REMARKS                                                                   */
/*                                                                           */
/* DEPENDENCIES   Either the switch VXWORKS, LINUX or WIN32 has to be set    */
/*                                                                           */
/* All rights reserved. Reproduction, modification, use or disclosure        */
/* to third parties without express authority is forbidden.                  */
/* Copyright Bombardier Transportation GmbH, Germany, 2002.                  */
/*                                                                           */

/* ---------------------------------------------------------------------------- */

#include "tdc.h"

/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */

T_TDC_BOOL tdcSetSocketOptions (const char*                 pModname,
                                T_TDC_SOCKET                sockFd,
                                const T_TDC_SOCK_OPTIONS*   pSockOpt)
{
   T_TDC_BOOL       bOK = TRUE;

   if (pSockOpt != NULL)
   {
      if (pSockOpt->pLingerOpt != NULL)
      {
         bOK &= setLingerOpt (pModname, sockFd, pSockOpt->pLingerOpt->lingerOn, pSockOpt->pLingerOpt->seconds);
      }

      if (pSockOpt->pSendTimeout != NULL)
      {
         bOK &= setSendTimeoutOpt (pModname, sockFd, pSockOpt->pSendTimeout->milliSeconds);
      }

      if (pSockOpt->pRecvTimeout != NULL)
      {
         bOK &= setRecvTimeoutOpt (pModname, sockFd, pSockOpt->pRecvTimeout->milliSeconds);
      }

      if (pSockOpt->pReuseOpt != NULL)
      {
         bOK &= setReuseAddrOpt (pModname, sockFd, pSockOpt->pReuseOpt->bReuse);
      }
   }

   return (bOK);
}

/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */

T_TDC_BOOL tdcUdpSocket4 (const char*        pModname,
                          T_TDC_SOCKET*      pSockFd)
{
   if (pSockFd != NULL)
   {
      if ((*pSockFd = Socket (tdcGet_AF_INET (), tdcGet_SOCK_DGRAM (), 0)) != TDC_INVALID_SOCKET)     /* IPv4 */
      {
         DEBUG_INFO1 (pModname, "Successfully called tdcUdpSocket4(%d)", (int) *pSockFd); 
         return (TRUE);
      }
   }
   
   DEBUG_WARN (pModname, "Failed to call tdcUdpSocket4()!");

   return (FALSE);
}

/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */

T_TDC_BOOL tdcTcpSocket4 (const char*        pModname,
                          T_TDC_SOCKET*      pSockFd)
{
   if (pSockFd != NULL)
   {
      if ((*pSockFd = Socket (tdcGet_AF_INET (), tdcGet_SOCK_STREAM (), 0)) != TDC_INVALID_SOCKET)     /* IPv4 */
      {
         /*DEBUG_INFO1 (pModname, "Successfully called tdcTcpSocket4(%d)", (int) *pSockFd); */
         return (TRUE);
      }
   }
   
   DEBUG_WARN (pModname, "Failed to call tdcTcpSocket4()!");

   return (FALSE);
}

/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */

T_TDC_BOOL tdcTcpConnect4 (const char*          pModname,
                           T_TDC_CONNECT_PAR*   pConnPar)
{
   if (    (pConnPar          != NULL)
        && (pConnPar->pSockFd != NULL)
      )
   {
      if (tdcTcpSocket4 (pModname, pConnPar->pSockFd))
      {
         UINT8                   servAddr[200];             /* Array size has to be large enough !!! */
         UINT32                  servAddrLen = (UINT32) sizeof (servAddr);
         char                    text [200];

         (void) tdcSetSocketOptions (pModname, *pConnPar->pSockFd, pConnPar->pSockOpt);

         tdcInitCli_SERV_ADDR (pModname, 
                               (T_TDC_SOCK_ADDR_IN *) servAddr, 
                               &servAddrLen, 
                               pConnPar->serverAddr.portNo);

         if (!getInetAddr (pConnPar->serverAddr.ipAddr, (T_TDC_SOCK_ADDR_IN *) servAddr)) 
         {
            (void) tdcSNPrintf (text, (UINT32) sizeof (text), 
                                "Failed to call tdcTcpConnect4() - Unknown Server(%s)", pConnPar->serverAddr.ipAddr);
            text[sizeof (text) - 1] = '\0';
            DEBUG_WARN (pModname, text);
         }
         else
         {
            if (ConnectTimo (*pConnPar->pSockFd,  (T_TDC_SOCK_ADDR *) servAddr, 
                             (int) servAddrLen, pConnPar->timeout))
            {
               (void) tdcSNPrintf (text, (UINT32) sizeof (text), 
                                  "Successfully called Connect(%d) - serv(%s) - port(%d)", 
                                  (int) *pConnPar->pSockFd, 
                                  pConnPar->serverAddr.ipAddr,
                                  (int) pConnPar->serverAddr.portNo);
               text[sizeof (text) - 1] = '\0';
               DEBUG_INFO (pModname, text);
               return (TRUE);
            }

            (void) tdcSNPrintf (text, (UINT32) sizeof (text), 
                               "Failed to call Connect(%d) - serv(%s) - port(%d)", 
                               (int) *pConnPar->pSockFd, 
                               pConnPar->serverAddr.ipAddr,
                               (int) pConnPar->serverAddr.portNo);
            text[sizeof (text) - 1] = '\0';

            DEBUG_WARN (pModname, text);
         }

         (void) tdcCloseSocket (pModname, pConnPar->pSockFd);
      }
   }

   return (FALSE);
}

/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */

T_TDC_BOOL tdcTcpBind4 (const char*          pModname,
                        T_TDC_LISTEN_PAR*    pListenPar)
{
   if (    (pListenPar            != NULL)
        && (pListenPar->pListenFd != NULL)
      )
   {
      if (tdcTcpSocket4 (pModname, pListenPar->pListenFd))
      {
         UINT8       servAddr[100];             /* Array size has to be large enough !!! */
         UINT32      servAddrLen = (UINT32) sizeof (servAddr);

         (void) tdcSetSocketOptions (pModname, *pListenPar->pListenFd, pListenPar->pSockOpt);

         tdcInitSrv_SERV_ADDR (pModname, 
                               (T_TDC_SOCK_ADDR_IN *) servAddr, 
                               &servAddrLen, 
                               pListenPar->portNo);

         if (Bind (*pListenPar->pListenFd, (T_TDC_SOCK_ADDR *) servAddr, (int) servAddrLen))
         {
            /*DEBUG_INFO2 (mod, "Successfully called tdcTcpBind4(%d) - Port(%d)", *pListenPar->pListenFd, pListenPar->portNo);*/
            
            pListenPar->addrLen = (T_TDC_SOCKLEN_T) servAddrLen;
            return (TRUE);
         }

         DEBUG_WARN1    (pModname, "Failed to call tdcTcpBind4() - PortNo(%d)", (int) pListenPar->portNo);
         (void) tdcCloseSocket (pModname, pListenPar->pListenFd);
      }
   }
   else
   {
      DEBUG_WARN (pModname, "Failed to call tdcTcpBind4()");
   }

   return (FALSE);
}

/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */

T_TDC_BOOL tdcShutDown (const char*       pModname,
                        T_TDC_SOCKET      socketFd, 
                        int               how)
{
   if (    (how == TDC_DISALLOW_RECEIVE)
        || (how == TDC_DISALLOW_SEND)
        || (how == TDC_DISALLOW_SENDRECV)
      )
   {
      if (ShutDown (socketFd, how))
      {
         return (TRUE);
      }

      DEBUG_WARN1 (pModname, "Failed to call os_ip_shutdown (%d) !", how);
   }

   return (FALSE);
}

/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */

T_TDC_BOOL tdcCloseSocket (const char*       pModname,
                           T_TDC_SOCKET*     pSockFd)
{
   T_TDC_BOOL      bOK = (pSockFd != NULL);

   if (pSockFd != NULL)
   {
      if (*pSockFd != TDC_INVALID_SOCKET)
      {
         bOK = CloseSocket (*pSockFd);

         if (!bOK)
         {
            DEBUG_WARN (pModname, "Failed to call TDCCloseSocket() !");
         }

         *pSockFd = TDC_INVALID_SOCKET;
      }
   }

   return (bOK);
}

/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */

T_TDC_BOOL tdcTcpListen (const char*         pModname,
                         T_TDC_LISTEN_PAR*   pListenPar)
{
   if (    (pListenPar            != NULL)
        && (pListenPar->pListenFd != NULL)
      )
   {
      /*char     text [240];  */

      /*tdcSNPrintf (text, sizeof (text), "Try to listen on Port(%d)", (int) pListenPar->portNo); */
      /*text[sizeof (text) - 1] = 0; */
      /*DEBUG_INFO (pModname, text); */

      if (tdcTcpBind (pModname, pListenPar))
      {
         if (Listen (*pListenPar->pListenFd, LISTENQ))
         {
            DEBUG_INFO2 (pModname, "Successfully called Listen(%d) - port(%d)", 
                                 (int) *pListenPar->pListenFd, (int) pListenPar->portNo);
            return (TRUE);
         }

         DEBUG_WARN2 (pModname, "Failed to call Listen(%d) - port(%d)!",
                              (int) *pListenPar->pListenFd, (int) pListenPar->portNo);
      }
   }
   else
   {
      DEBUG_WARN (pModname, "Failed to call tdcTcpListen() - Invalid parameters!");
   }

   return (FALSE);
}

