/*******************************************************************************
*  COPYRIGHT      : (c) 2006-2010 Bombardier Transportation
********************************************************************************
*  PROJECT        : IPTCom
*
*  MODULE         : example.c
*
*  ABSTRACT       : Example of how IPTCom can be used
*
********************************************************************************
*  HISTORY     :
*	
*  $Id: example.c 11712 2010-10-12 15:32:07Z gweiss $
*
*  MODIFICATIONS (log starts 2010-08-11) 
*
*  CR-432 (Gerhard Weiss, 12-Oct-2010)
*  Removed warnings provoked by -W (for GCC)
*
*
*******************************************************************************/


/*******************************************************************************
* INCLUDES */

#include "iptcom.h"
#include "vos.h"

/*******************************************************************************
* TYPEDEFS */
 
typedef struct
{
   INT8 a;
   INT16 b;
   INT8 c;
   INT32 d;
} DATASET1;

typedef struct
{
   INT16 a;
   DATASET1 b;
   INT16 c;
   INT16 d;
} DATASET2;

/*******************************************************************************
* GLOBALS */

/*******************************************************************************
* LOCALS */

static PD_HANDLE hs_7000=0, hs_7010=0, hs_7020=0, hs_7100=0;
static PD_HANDLE hp_8000=0, hp_8010=0, hp_8020=0, hp_8100=0;


/*******************************************************************************
* LOCAL FUNCTIONS */


/*******************************************************************************
* GLOBAL FUNCTIONS */


/* Sending without response and without communication result

This example shows how to send data between end devices.
The sending application will not get any information about the communication
result or any response message from the receiving application.

The destination URI has to be configured for the used comId
or
the destination URI has to be configured for the used dsestination Id for the used comId
or
the destination URI has to be given in the call to enable the IPTCom to get the destination address.
In all cases the destination URI has to be known by the IPTDir.

Both unicast and multicast sending can be done in this way.

The MDComAPI_putMsgF can be used in the same way with no callback function
given in the call.

*/

void example1(void)
{
   int  res;
   char buf[100];

   /* Fill buffer with data */

   /* Send message without overriding of destination URI */
   res = MDComAPI_putDataMsg(5100, /* ComId */
                             buf,  /* Data buffer */
                             100,  /* Number of data to be send */
                             0,    /* No queue for communication result */
                             NULL, /* No call-back function */
                             0,    /* No caller reference value */
                             0,    /* Topo counter */
                             0,    /* No destination ID */
                             0,    /* No overriding of destination URI */
                             0);   /* No overriding of source URI */
   if (res != IPT_OK)
   {
      /* The sending couldn't be started. */
      /* Error handling */
   }

   /* Send message with destination ID*/
   res = MDComAPI_putDataMsg(5200,         /* ComId */
                             buf,         /* Data buffer */
                             100,         /* Number of data to be send */
                             0,           /* No queue for communication result */
                             NULL,        /* No call-back function */
                             0,           /* No caller reference value */
                             0,           /* Topo counter */
                             1,           /* Destination ID */
                             0,           /* No overriding of destination URI */
                             0);          /* No overriding of source URI */
            
   if (res != IPT_OK)
   {
      /* The sending couldn't be started. */
      /* Error handling */
   }
  
   /* Send message with overriding of destination URI*/
   res = MDComAPI_putDataMsg(5200,       /* ComId */
                             buf,         /* Data buffer */
                             100,         /* Number of data to be send */
                             0,           /* No queue for communication result */
                             NULL,        /* No call-back function */
                             0,           /* No caller reference value */
                             0,           /* Topo counter */
                             0,           /* No destination ID */
                             "func1@vcu", /* Overriding of destination URI */
                             0);          /* No overriding of source URI */
            
   if (res != IPT_OK)
   {
      /* The sending couldn't be started. */
      /* Error handling */
   }
}  

void example1_OldSyntax(void)
{
   int  res;
   char buf[100];

   /* Fill buffer with data */

   /* Send message without overriding of destination URI */
   res = MDComAPI_putMsgQ(5100, /* ComId */
                          buf,  /* Data buffer */
                          100,  /* Number of data to be send */
                          0,    /* No queue for communication result */
                          0,    /* No caller reference value */
                          0,    /* Topo counter */
                          0,    /* No overriding of destination URI */
                          0);   /* No overriding of source URI */
   if (res != IPT_OK)
   {
      /* The sending couldn't be started. */
      /* Error handling */
   }

   /* Send message with overriding of destination URI*/
   res = MDComAPI_putMsgQ(5200,         /* ComId */
                          buf,         /* Data buffer */
                          100,         /* Number of data to be send */
                          0,           /* No queue for communication result */
                          0,           /* No caller reference value */
                          0,           /* Topo counter */
                          "func1@vcu", /* Overriding of destination URI */
                          0);          /* No overriding of source URI */
            
   if (res != IPT_OK)
   {
      /* The sending couldn't be started. */
      /* Error handling */
   }
}


/* Sending without response but with communication result via queue

This example shows how to send data between end devices. The sending application
will get information about the communication result by reading a queue. In 
example 2 the queue is polled and in example 3 the application waits until the 
result is ready. No response message from the receiving application.

The destination URI has to be configured for the used comId
or
the destination URI has to be configured for the used dsestination Id for the used comId
or
the destination URI has to be given in the call to enable the IPTCom to get the destination address.
In all cases the destination URI has to be known by the IPTDir.

Note that multicast sending can not be done in this way as no result can be 
reported back to the application as there is no acknowledge at all used for 
multicast messages.

*/
static   MD_QUEUE mdq2;

void example2_init(void)
{

   /* Create a queue with 10 elements and name */
   mdq2 = MDComAPI_queueCreate(10, "mdq2");
   if (mdq2 == 0)
   {
      /* error */
   }
}

void example2_init_oldSyntax(void)
{

   /* Create a queue with 10 elements */
   mdq2 = MDComAPI_createQueue(10);
   if (mdq2 == 0)
   {
      /* error */
   }
}

void example2_send(void)
{
   int  res;
   char buf[100];
   void *pCallerRef;

   /* Fill buffer with data */

   /* Set caller reference */
   pCallerRef = (void *)0x1234;

   /* Send message, destination URI configured for the ComId */
   res = MDComAPI_putDataMsg(5100,       /* ComId */
                             buf,        /* Data buffer */
                             100,        /* Number of data to be send */
                             mdq2,       /* Queue for communication result */
                             NULL,       /* No call-back function */
                             pCallerRef, /* Caller reference value */
                             0,          /* Topo counter */
                             0,          /* No destination ID */
                             0,          /* No overriding of destination URI */
                             0);         /* No overriding of source URI */
   if (res != IPT_OK)
   {
      /* The sending couldn't be started. */
      /* Error handling */
   }

   /* Send message destination URI configured for the destination ID for the ComId */
   res = MDComAPI_putDataMsg(5200,       /* ComId */
                             buf,        /* Data buffer */
                             100,        /* Number of data to be send */
                             mdq2,       /* Queue for communication result */
                             NULL,       /* No call-back function */
                             pCallerRef, /* Caller reference value */
                             0,          /* Topo counter */
                             1,          /* No destination ID */
                             0,          /* No overriding of destination URI */
                             0);         /* No overriding of source URI */
   if (res != IPT_OK)
   {
      /* The sending couldn't be started. */
      /* Error handling */
   }

}

void example2_send_OldSyntax(void)
{
   int  res;
   char buf[100];
   void *pCallerRef;

   /* Fill buffer with data */

   /* Set caller reference */
   pCallerRef = (void *)0x1234;

   /* Send message */
   res = MDComAPI_putMsgQ(5100,       /* ComId */
                          buf,        /* Data buffer */
                          100,        /* Number of data to be send */
                          mdq2,       /* Queue for communication result */
                          pCallerRef, /* Caller reference value */
                          0,          /* Topo counter */
                          0,          /* No overriding of destination URI */
                          0);         /* No overriding of source URI */
   if (res != IPT_OK)
   {
      /* The sending couldn't be started. */
      /* Error handling */
   }
}

void example2_checkResult(void)
{
   UINT32 size;
   MSG_INFO msg_info;

   while (MDComAPI_getMsg(mdq2,         /* Queue ID */
                          &msg_info,    /* Message info */
                          NULL,        /* No data buffer needed */
                          &size,       /* Data buffer size, dummy */
                          IPT_NO_WAIT) /* Poll for message */
                           == MD_QUEUE_NOT_EMPTY)
   {
      /* The caller reference value may be checked here */

      switch (msg_info.resultCode)
      {
         case MD_SEND_OK:
            /* Sending OK, i.e. the message has been acknowledged 
               and the is a listener in the receiving end */
            break;

         case MD_NO_ACK_RECEIVED:
            /* Acknowledge has been received */
            break;

         case MD_SEND_FAILED:
            /* Sending failed */
            break;

         case MD_NO_LISTENER:
            /* No listener in the receiving end */
            break;

         case MD_NO_BUF_AVAILABLE:
            /* Receiving end out of memory */
            break;

         default:
            break;
      }
   }
}

static   MD_QUEUE mdq3;

void example3_init(void)
{
   /* Create a queue with 10 elements and name */
   mdq3 = MDComAPI_queueCreate(10, "mdq3");
   if (mdq3 == 0)
   {
      /* error */
   }
}

void example3_send(void)
{
   int  res;
   char buf[100];
   UINT32 size;
   MSG_INFO msg_info;
   static void *pCallerRef;

   /* Fill buffer with data */

   /* Set caller reference */
   pCallerRef = (void *)0x1234;

   /* Send message */
   res = MDComAPI_putDataMsg(5100,       /* ComId */
                             buf,        /* Data buffer */
                             100,        /* Number of data to be send */
                             mdq3,       /* Queue for communication result */
                             NULL,       /* No call-back function */
                             pCallerRef, /* Caller reference value */
                             0,          /* Topo counter */
                             0,          /* No destination ID */
                             0,          /* No overriding of destination URI */
                             0);         /* No overriding of source URI */
   if (res != IPT_OK)
   {
      /* The sending couldn't be started. */
      /* Error handling */
   }
   else
   {
      res = MDComAPI_getMsg(mdq3,              /* Queue ID */
                            &msg_info,         /* Message info */
                            NULL,              /* No data buffer needed */
                            &size,             /* Data buffer size, dummy */
                            IPT_WAIT_FOREVER); /* Wait for result */
      if (res == MD_QUEUE_NOT_EMPTY)
      {
         switch (msg_info.resultCode)
         {
            case MD_SEND_OK:
               /* Sending OK, i.e. the message has been acknowledged 
                  and the is a listener in the receiving end */
               break;

            case MD_NO_ACK_RECEIVED:
               /* Acknowledge has been received */
               break;

            case MD_SEND_FAILED:
               /* Sending failed */
               break;

            case MD_NO_LISTENER:
               /* No listener in the receiving end */
               break;

            case MD_NO_BUF_AVAILABLE:
               /* Receiving end out of memory */
               break;

            default:
               break;
         }
      }
      else
      {
         /* error */
      }
   }
}

void example3_send_OldSyntax(void)
{
   int  res;
   char buf[100];
   UINT32 size;
   MSG_INFO msg_info;
   static void *pCallerRef;

   /* Fill buffer with data */

   /* Set caller reference */
   pCallerRef = (void *)0x1234;

   /* Send message */
   /* Send message */
   res = MDComAPI_putMsgQ(5100,       /* ComId */
                          buf,        /* Data buffer */
                          100,        /* Number of data to be send */
                          mdq3,       /* Queue for communication result */
                          pCallerRef, /* Caller reference value */
                          0,          /* Topo counter */
                          0,          /* No overriding of destination URI */
                          0);         /* No overriding of source URI */
   if (res != IPT_OK)
   {
      /* The sending couldn't be started. */
      /* Error handling */
   }
   else
   {
      res = MDComAPI_getMsg(mdq3,              /* Queue ID */
                            &msg_info,         /* Message info */
                            NULL,              /* No data buffer needed */
                            &size,             /* Data buffer size, dummy */
                            IPT_WAIT_FOREVER); /* Wait for result */
      if (res == MD_QUEUE_NOT_EMPTY)
      {
         switch (msg_info.resultCode)
         {
            case MD_SEND_OK:
               /* Sending OK, i.e. the message has been acknowledged 
                  and the is a listener in the receiving end */
               break;

            case MD_NO_ACK_RECEIVED:
               /* Acknowledge has been received */
               break;

            case MD_SEND_FAILED:
               /* Sending failed */
               break;

            case MD_NO_LISTENER:
               /* No listener in the receiving end */
               break;

            case MD_NO_BUF_AVAILABLE:
               /* Receiving end out of memory */
               break;

            default:
               break;
         }
      }
      else
      {
         /* error */
      }
   }
}

#ifndef LINUX_MULTIPROC
/* Sending without response but with communication result via callback function

This example shows how to send data between devices. The sending application 
will get information about the communication result via a callback function. No 
response message from the receiving application.

The destination URI has to be configured for the used comId
or
the destination URI has to be configured for the used dsestination Id for the used comId
or
the destination URI has to be given in the call to enable the IPTCom to get the destination address.
In all cases the destination URI has to be known by the IPTDir.

Note that multicast sending can not be done in this way as no result can be 
reported back to the application as there is no acknowledge at all used for 
multicast messages.

*/

void example4_callBack(const MSG_INFO *pMsg_info, /* Message info */
                       const char     *pData,   /* Not used for results */
                       UINT32   length)   /* Not used for results */
{
   IPT_UNUSED (pData)
   IPT_UNUSED (length)

   if (pMsg_info->resultCode == MD_SEND_OK)
   {
      /* Sending was OK */
   }
   else
   {
      /* Error, see example 2 */
   }
}

void example4_send(void)
{
   int  res;
   char buf[100];
   void *pCallerRef;

   /* Fill buffer with data */

   /* Set caller reference */
   pCallerRef = (void *)0x1234;

   /* Send message */
   res = MDComAPI_putDataMsg(5100,               /* ComId */
                             buf,                /* Data buffer */
                             100,                /* Number of data to be send */
                             0,                  /* No queue for communication result */
                             example4_callBack,  /* Callback function for 
                                                    communication result */
                             pCallerRef,         /* Caller reference value */
                             0,                  /* Topo counter */
                             0,                  /* No destination ID */
                             0,                  /* No overriding of destination URI */
                             0);                 /* No overriding of source URI */
   if (res != IPT_OK)
   {
      /* The sending couldn't be started. */
      /* Error handling */
   }

}
void example4_send_Oldsyntax(void)
{
   int  res;
   char buf[100];
   void *pCallerRef;

   /* Fill buffer with data */

   /* Set caller reference */
   pCallerRef = (void *)0x1234;

   /* Send message */
   res = MDComAPI_putMsgF(5100,  /* ComId */
                           buf,  /* Data buffer */
                           100,  /* Number of data to be send */
             example4_callBack,  /* Callback function for 
                                    communication result */
                    pCallerRef,  /* Caller reference value */
                             0,  /* Topo counter */
                             0,  /* No overriding of destination URI */
                             0); /* No overriding of source URI */
   if (res != IPT_OK)
   {
      /* The sending couldn't be started. */
      /* Error handling */
   }
}
#endif

/* Sending with responses and/or communication result via queue

These examples show how to send data between two end devices. The sending 
application will get response messages send by the receiving application and/or
information about the communication result via a queue. 

The destination URI has to be configured for the used comId
or
the destination URI has to be configured for the used dsestination Id for the used comId
or
the destination URI has to be given in the call to enable the IPTCom to get the destination address.
In all cases the destination URI has to be known by the IPTDir.

Example 5 shows call with known number of expected responses and example 6 call 
with unknown number of expected responses


*/

static   MD_QUEUE mdq5;

void example5_init(void)
{
   /* Create a queue with 10 elements */
   mdq5 = MDComAPI_createQueue(10);
   if (mdq5 == 0)
   {
      /* error */
   }
}

void example5_sendRequest(void)
{
   int  res;
   char sendbuf[100];
   char recbuf[200];
   UINT32 size;
   MSG_INFO msgInfo;
   char *pRecBuf;
   UINT16 expectedNoOfResponses = 2;
   void *pCallerRef;

   /* Fill send buffer with data */

   /* Set caller reference */
   pCallerRef = (void *)0x5678;

   /* Sending a request to destination configured for the ComId */
   res = MDComAPI_putReqMsg(4200, /* ComId */
                        sendbuf, /* Data buffer */
                            100, /* Number of data to be send */
          expectedNoOfResponses, /* Number of expected replies.
                                   0=unspecified */
                              0, /* Time-out value in milliseconds
                                    for receiving replies
                                    0=default value */
                           mdq5, /* Queue for communication result */
                              0, /* No call-back function */
                     pCallerRef, /* Caller reference value */
                              0, /* Topo counter */
                              0, /* No destination ID */
                              0, /* No overriding of destination URI */
                              0);/* No overriding of source URI */


   /* Sending a request to destination configured for the destination ID for the ComId */
   res = MDComAPI_putReqMsg(5200, /* ComId */
                        sendbuf, /* Data buffer */
                            100, /* Number of data to be send */
          expectedNoOfResponses, /* Number of expected replies.
                                   0=unspecified */
                              0, /* Time-out value in milliseconds
                                    for receiving replies
                                    0=default value */
                           mdq5, /* Queue for communication result */
                              0, /* No call-back function */
                     pCallerRef, /* Caller reference value */
                              0, /* Topo counter */
                              1, /* No destination ID */
                              0, /* No overriding of destination URI */
                              0);/* No overriding of source URI */


            
   if (res != IPT_OK)
   {
      /* The sending couldn't be started. */
      /* Error handling */
   }
   else
   {
      do
      {
         size = sizeof(recbuf);
         pRecBuf = recbuf;  /* Use application allocated buffer */
         res = MDComAPI_getMsg(mdq5,     /* Queue ID */
                           &msgInfo, /* Message info */
                           &pRecBuf, /* Pointer to pointer to data 
                                        buffer */
                           &size,    /* Pointer to size. Size shall
                                        be set to own buffer size at
                                        the call. The IPTCom will
                                        return the number of received 
                                        bytes */
                           IPT_WAIT_FOREVER); /* Wait for result */
         if (res == MD_QUEUE_NOT_EMPTY)
         {
            /* Response data ? */
            if (msgInfo.msgType == MD_MSGTYPE_RESPONSE )
            {
               /* Take care of received response data */
            }
            else if (msgInfo.msgType != MD_MSGTYPE_RESULT)
            {
               /* Check the result code */
            }  
         }
         else
         {
            /* error */
         }
      }
      while((msgInfo.msgType != MD_MSGTYPE_RESULT ) &&
            (msgInfo.noOfResponses < expectedNoOfResponses));
   }
}
void example5_sendRequest_OldSyntax(void)
{
   int  res;
   char sendbuf[100];
   char recbuf[200];
   UINT32 size;
   MSG_INFO msgInfo;
   char *pRecBuf;
   UINT16 expectedNoOfResponses = 2;
   void *pCallerRef;

   /* Fill send buffer with data */

   /* Set caller reference */
   pCallerRef = (void *)0x5678;

   /* Send message */
   res = MDComAPI_putRequestMsgQ(4200, /* ComId */
                             sendbuf, /* Data buffer */
                                 100, /* Number of data to be send */
               expectedNoOfResponses, /* Number of expected replies.
                                        0=unspecified */
                                   0, /* Time-out value in milliseconds
                                         for receiving replies
                                         0=default value */
                                mdq5, /* Queue for communication result */
                          pCallerRef, /* Caller reference value */
                                   0, /* Topo counter */
                                   0, /* No overriding of destination URI */
                                   0);/* No overriding of source URI */
            
   if (res != IPT_OK)
   {
      /* The sending couldn't be started. */
      /* Error handling */
   }
   else
   {
      do
      {
         size = sizeof(recbuf);
         pRecBuf = recbuf;  /* Use application allocated buffer */
         res = MDComAPI_getMsg(mdq5,     /* Queue ID */
                           &msgInfo, /* Message info */
                           &pRecBuf, /* Pointer to pointer to data 
                                        buffer */
                           &size,    /* Pointer to size. Size shall
                                        be set to own buffer size at
                                        the call. The IPTCom will
                                        return the number of received 
                                        bytes */
                           IPT_WAIT_FOREVER); /* Wait for result */
         if (res == MD_QUEUE_NOT_EMPTY)
         {
            /* The caller reference value may be checked here */
           
            /* Response data ? */
            if (msgInfo.msgType == MD_MSGTYPE_RESPONSE )
            {
               /* Take care of received response data */
            }
            else if (msgInfo.msgType != MD_MSGTYPE_RESULT)
            {
               /* Check the result code */
            }  
         }
         else
         {
            /* error */
         }
      }
      while((msgInfo.msgType != MD_MSGTYPE_RESULT ) &&
            (msgInfo.noOfResponses < expectedNoOfResponses));
   }
}

static   MD_QUEUE mdq6;

void example6_init(void)
{
   /* Create a queue with 10 elements */
   mdq6 = MDComAPI_createQueue(10);
   if (mdq6 == 0)
   {
      /* error */
   }
}

void example6_sendRequest(void)
{
   int  res;
   char sendbuf[100];
   char recbuf[200];
   UINT32 size;
   MSG_INFO msgInfo;
   char *pRecBuf;

   /* Fill send buffer with data */

   res = MDComAPI_putReqMsg(100, /* ComId */
                        sendbuf, /* Data buffer */
                            100, /* Number of data to be send */
                              0, /* Number of expected replies.
                                   0=unspecified */
                              0, /* Time-out value in milliseconds
                                    for receiving replies
                                    0=default value */
                           mdq6, /* Queue for communication result */
                              0, /* No call-back function */
                      (void *)6, /* Caller reference value */
                              0, /* Topo counter */
                              0, /* No destination ID */
                              0, /* No overriding of destination URI */
                              0);/* No overriding of source URI */
   if (res != IPT_OK)
   {
      /* The sending couldn't be started. */
      /* Error handling */
   }
   else
   {
      do
      {
         size = sizeof(recbuf);
         pRecBuf = recbuf;  /* Use application allocated buffer */
         res = MDComAPI_getMsg(mdq6,     /* Queue ID */
                           &msgInfo, /* Message info */
                           &pRecBuf, /* Pointer to pointer to data 
                                        buffer */
                           &size,    /* Pointer to size. Size shall
                                        be set to own buffer size at
                                        the call. The IPTCom will
                                        return the number of received 
                                        bytes */
                           IPT_WAIT_FOREVER); /* Wait for result */
         if (res == MD_QUEUE_NOT_EMPTY)
         {
            /* Response data ? */
            if (msgInfo.msgType == MD_MSGTYPE_RESPONSE)
            {
               /* Take care of received response data */
            }
            else if (msgInfo.msgType != MD_MSGTYPE_RESULT)
            {
               /* Check the result code and 
                  number of received responses */
            }  
         }
         else
         {
            /* error */
         }
      }
      while(msgInfo.msgType != MD_MSGTYPE_RESULT );
   }
}

void example6_sendRequest_OldSyntax(void)
{
   int  res;
   char sendbuf[100];
   char recbuf[200];
   UINT32 size;
   MSG_INFO msgInfo;
   char *pRecBuf;

   /* Fill send buffer with data */

   /* Send message */
   res = MDComAPI_putRequestMsgQ(100, /* ComId */
                             sendbuf, /* Data buffer */
                                 100, /* Number of data to be send */
                                   0, /* Number of expected replies.
                                         0=unspecified */
                                   0, /* Time-out value  in milliseconds
                                         for receiving replies
                                         0=default value */
                                mdq6, /* Queue for communication result */
                           (void *)6, /* Caller reference value */
                                   0, /* Topo counter */
                                   0, /* No overriding of destination URI */
                                   0);/* No overriding of source URI */
   if (res != IPT_OK)
   {
      /* The sending couldn't be started. */
      /* Error handling */
   }
   else
   {
      do
      {
         size = sizeof(recbuf);
         pRecBuf = recbuf;  /* Use application allocated buffer */
         res = MDComAPI_getMsg(mdq6,     /* Queue ID */
                           &msgInfo, /* Message info */
                           &pRecBuf, /* Pointer to pointer to data 
                                        buffer */
                           &size,    /* Pointer to size. Size shall
                                        be set to own buffer size at
                                        the call. The IPTCom will
                                        return the number of received 
                                        bytes */
                           IPT_WAIT_FOREVER); /* Wait for result */
         if (res == MD_QUEUE_NOT_EMPTY)
         {
            /* Response data ? */
            if (msgInfo.msgType == MD_MSGTYPE_RESPONSE)
            {
               /* Take care of received response data */
            }
            else if (msgInfo.msgType != MD_MSGTYPE_RESULT)
            {
               /* Check the result code and 
                  number of received responses */
            }  
         }
         else
         {
            /* error */
         }
      }
      while(msgInfo.msgType != MD_MSGTYPE_RESULT );
   }
}

/* ComId listeners receiving via queue.

These examples show how to receive data via a queue. First has a queue to be 
created and a listener added.

In example 7 the application waits until a messages has been received and it 
uses an own buffer for the received data.
The application will send back a response when a request 
message has been received.

In example 8 the application polls the queue and it uses a data buffer from 
IPTCom. The buffer has to be released by the application.
The listener are added with a destination ID to join multicast IP addresse(s)
The application will send back a response when a request 
message has been received. The call is using a queue to check the communication result.


*/

static   MD_QUEUE mdq7;

void example7_init(void)
{
   UINT32 mdcomid_array[] = {6000, 6001, 0};
   int res;
   void *pCallerRef = (void *)7;

   /* Create a queue with 10 elements */
   mdq7 = MDComAPI_queueCreate(10, "mdq7");
   if (mdq7 == 0)
   {
      /* error */
   }
   else
   {
      res = MDComAPI_comIdListener(mdq7,           /* Queue ID */
                                   0,              /* No call-back function */
                                   pCallerRef,     /* Caller reference */
                                    mdcomid_array, /* ComId table */
                                    0,             /* No Redundancy function reference */
                                    0,             /* No Destination URI ID */
                                    0);            /* No Pointer to destination URI */
      if (res != IPT_OK)
      {
         /* error */
      }
   }
}

void example7_init_OldSyntax(void)
{
   UINT32 mdcomid_array[] = {6000, 6001, 0};
   int res;
   void *pCallerRef = (void *)7;

   /* Create a queue with 10 elements */
   mdq7 = MDComAPI_createQueue(10);
   if (mdq7 == 0)
   {
      /* error */
   }
   else
   {
      res = MDComAPI_addComIdListenerQ(mdq7, pCallerRef, mdcomid_array);    
      if (res != IPT_OK)
      {
         /* error */
      }
   }
}

void example7_receive(void)
{
   int res;
   char sendbuf[100];
   char recbuf[200];
   UINT32 size;
   MSG_INFO msgInfo;
   char *pRecBuf;
   UINT16 userStatus;

   pRecBuf = recbuf;  /* Use application allocated buffer */
   
   while (1)
   {
      size = sizeof(recbuf);
      res = MDComAPI_getMsg(mdq7,     /* Queue ID */
                           &msgInfo, /* Message info */
                           &pRecBuf, /* Pointer to pointer to data 
                                        buffer */
                           &size,    /* Pointer to size. Size shall
                                        be set to own buffer size at
                                        the call. The IPTCom will
                                        return the number of received 
                                        bytes */
                           IPT_WAIT_FOREVER); /* Wait for result */
      if (res == MD_QUEUE_NOT_EMPTY)
      {
         if (msgInfo.msgType == MD_MSGTYPE_DATA )
         {
            /* Handle received data */
         }
         else if (msgInfo.msgType == MD_MSGTYPE_REQUEST)
         {
            /* Handle received data */

            /* Prepare response data */

            /* Set User status */
            userStatus = 0;

            /* Send a response */
            res = MDComAPI_putRespMsg(5100,  /* ComID */
                                   userStatus,  /* User Status */
                                      sendbuf,  /* Data buffer */
                                           55,  /* Number of data to be send */
                            msgInfo.sessionId,  /* Session ID, has to be the 
                                                   sameas in the received 
                                                   requestmessage */
                                            0,  /* No Caller queue ID */
                                            0,  /* Pointer to callback function */
                                            0,  /* Caller reference */
                                            0,  /* Topo counter */
                            msgInfo.srcIpAddr,  /* Destination IP address, has
                                                   to be the same as in the
                                                   received request message */
                                            0,  /* Destination URI ID */
                                            0,  /* No destination URI string */
                                            0); /* No source URI string */
            if (res != IPT_OK)
            {
               /* The sending couldn't be started. */
               /* Error handling */
            }
         }
         else
         {
            /* Not necessary, no other message types will be returned
               in this case */
         }
      }
      else
      {
         /* error */
      }
   }   
}

void example7_receive_OldSyntax(void)
{
   int res;
   char sendbuf[100];
   char recbuf[200];
   UINT32 size;
   MSG_INFO msgInfo;
   char *pRecBuf;
   UINT16 userStatus;

   pRecBuf = recbuf;  /* Use application allocated buffer */
   
   while (1)
   {
      size = sizeof(recbuf);
      res = MDComAPI_getMsg(mdq7,     /* Queue ID */
                           &msgInfo, /* Message info */
                           &pRecBuf, /* Pointer to pointer to data 
                                        buffer */
                           &size,    /* Pointer to size. Size shall
                                        be set to own buffer size at
                                        the call. The IPTCom will
                                        return the number of received 
                                        bytes */
                           IPT_WAIT_FOREVER); /* Wait for result */
      if (res == MD_QUEUE_NOT_EMPTY)
      {
         if (msgInfo.msgType == MD_MSGTYPE_DATA )
         {
            /* Handle received data */
         }
         else if (msgInfo.msgType == MD_MSGTYPE_REQUEST)
         {
            /* Handle received data */

            /* Prepare response data */

            /* Set User status */
            userStatus = 0;

            /* Send a response */
            res = MDComAPI_putResponseMsg(100,  /* ComID */
                                   userStatus,  /* User Status */
                                      sendbuf,  /* Data buffer */
                                           55,  /* Number of data to be send */
                            msgInfo.sessionId,  /* Session ID, has to be the 
                                                   sameas in the received 
                                                   requestmessage */
                                            0,  /* Topo counter */
                            msgInfo.srcIpAddr,  /* Destination IP address, has
                                                   to be the same as in the
                                                   received request message */
                                            0,  /* No destination URI string */
                                            0); /* No source URI string */
            if (res != IPT_OK)
            {
               /* The sending couldn't be started. */
               /* Error handling */
            }
         }
         else
         {
            /* Not necessary, no other message types will be returned
               in this case */
         }
      }
      else
      {
         /* error */
      }
   }   
}

static   MD_QUEUE mdq8;

void example8_init(void)
{
   UINT32 mdcomid_array[] = {6000, 6001, 0};
   int res;

   /* Create a queue with 10 elements */
   mdq8 = MDComAPI_queueCreate(10, "mdq8");
   if (mdq8 == 0)
   {
      /* error */
   }
   else
   {
      res = MDComAPI_comIdListener(mdq8,           /* Queue ID */
                                   0,              /* No call-back function */
                                   0,              /* Caller reference */
                                    mdcomid_array, /* ComId table */
                                    0,             /* No Redundancy function reference */
                                    1,             /* No Destination URI ID */
                                    0);            /* No Pointer to destination URI */
      if (res != IPT_OK)
      {
         /* error */
      }
   }
}

void example8_receive(void)
{
   int res;
   char sendbuf[100];
   UINT32 size;
   MSG_INFO msgInfo;
   char *pRecBuf;
   UINT16 userStatus;

   pRecBuf = NULL;  /* Use buffer allocated by IPTCom */
   
   res = MDComAPI_getMsg(mdq8,     /* Queue ID */
                        &msgInfo, /* Message info */
                        &pRecBuf, /* Pointer to pointer to data 
                                     buffer */
                        &size,    /* Pointer to size. The IPTCom will
                                     return the number of received 
                                     bytes */
                        IPT_NO_WAIT); /* Poll for result */
   if (res == MD_QUEUE_NOT_EMPTY)
   {
      if (msgInfo.msgType == MD_MSGTYPE_DATA )
      {
         /* Handle received data */
      }
      else if (msgInfo.msgType == MD_MSGTYPE_REQUEST)
      {
         /* Handle received data */

         /* Prepare response data */

         /* Set User status */
         userStatus = 0;

         /* Send a response */
         res = MDComAPI_putRespMsg(5100,  /* ComID */
                                userStatus,  /* User Status */
                                   sendbuf,  /* Data buffer */
                                        55,  /* Number of data to be send */
                         msgInfo.sessionId,  /* Session ID, has to be the 
                                                sameas in the received 
                                                requestmessage */
                                         0,  /* No Caller queue ID */
                                         0,  /* Pointer to callback function */
                                         0,  /* Caller reference */
                                         0,  /* Topo counter */
                         msgInfo.srcIpAddr,  /* Destination IP address, has
                                                to be the same as in the
                                                received request message */
                                         0,  /* Destination URI ID */
                                         0,  /* No destination URI string */
                                         0); /* No source URI string */
         if (res != IPT_OK)
         {
            /* The sending couldn't be started. */
            /* Error handling */
         }
      }

      if (pRecBuf != 0)
      {
         /* Free buffer allocated by IPTCom */
         res = MDComAPI_freeBuf(pRecBuf);
         if (res != 0)
         {
            /* error */
         }
      }
   }
   else if (res == MD_QUEUE_EMPTY)
   {
      /* Queue is empty */
   }
   else
   {
      /* error */
   }
}

#ifndef LINUX_MULTIPROC
/* ComId listeners receiving via callback function

These examples show how to receive data via a call-back function. First has a 
listener to be added.

In the example the call-back function will send back a response when a request 
message has been received.

*/

void example9_callBack(const MSG_INFO *pMsgInfo, /* Message info */
                       const char   *pData,    /* Not used for results */
                       UINT32 length);    /* Not used for results */

void example9_init(void)
{
   UINT32 mdcomid_array[] = {6000, 6001, 0};
   int res;
   int callerRef = 9;
   
   res = MDComAPI_comIdListener(0,                 /* No Queue ID */
                                example9_callBack, /* No call-back function */
                                (void *)callerRef, /* Caller reference */
                                 mdcomid_array,    /* ComId table */
                                 0,                /* No Redundancy function reference */
                                 0,                /* No Destination URI ID */
                                 0);               /* No Pointer to destination URI */
   if (res != IPT_OK)
   {
      /* error */
   }
}

void example9_init_OldSyntax(void)
{
   UINT32 mdcomid_array[] = {6000, 6001, 0};
   int res;
   int callerRef = 9;
   
   res = MDComAPI_addComIdListenerF(example9_callBack, (void *)callerRef, mdcomid_array);    
   if (res != IPT_OK)
   {
      /* error */
   }
}

void example9_callBack(const MSG_INFO *pMsgInfo, /* Message info */
                       const char   *pData,    /* Not used for results */
                       UINT32 length)    /* Not used for results */
{
   int res;
   char sendbuf[100];
   UINT16 userStatus;
 
   IPT_UNUSED (pData)
   IPT_UNUSED (length)
   
   if (pMsgInfo->msgType == MD_MSGTYPE_DATA )
   {
      /* Handle received data */
   }
   else if (pMsgInfo->msgType == MD_MSGTYPE_REQUEST)
   {
      /* Handle received data */

      /* Prepare response data */

      /* Set User status */
      userStatus = 0;

      /* Send a response */
      res = MDComAPI_putRespMsg(5100,  /* ComID */
                             userStatus,  /* User Status */
                                sendbuf,  /* Data buffer */
                                     55,  /* Number of data to be send */
                    pMsgInfo->sessionId,  /* Session ID, has to be the 
                                             sameas in the received 
                                             requestmessage */
                                      0,  /* No Caller queue ID */
                                      0,  /* Pointer to callback function */
                                      0,  /* Caller reference */
                                      0,  /* Topo counter */
                    pMsgInfo->srcIpAddr,  /* Destination IP address, has
                                             to be the same as in the
                                             received request message */
                                      0,  /* Destination URI ID */
                                      0,  /* No destination URI string */
                                      0); /* No source URI string */
      if (res != IPT_OK)
      {
         /* The sending couldn't be started. */
         /* Error handling */
      }
   }
   else
   {
      /* Not necessary, no other message types will be returned
         in this case */
   }
}
#endif

/* URI string listeners receiving via queue.

These examples show how to receive data via a queue. First has a queue to be 
created and a listener added.

In example 10 the application waits until a messages has been received and it 
uses an own buffer for the received data.

In example 11 the application polls the queue and it uses a data buffer from 
IPTCom. The buffer has to be released by the application.
One listener are added with a ComId to join a multicast IP address and
one listener are added with a destination Id and a Comid to join a multicast IP address

In both examples the application will send back a response when a request 
message has been received.

*/

static   MD_QUEUE mdq10;

void example10_init(void)
{
   int res;
   void *pCallerRef = (void *)11000;

   /* Create a queue with 10 elements */
   mdq10 = MDComAPI_queueCreate(10, "mdq10");
   if (mdq10 == 0)
   {
      /* error */
   }
   else
   {
      res =  MDComAPI_uriListener(mdq10,             /* Queue ID */
                                      0,             /* No Pointer to callback function */
                                      pCallerRef,    /* Caller reference */
                                      0,             /* No ComId */
                                      0,             /* No Destination URI ID */
                                      "insta.funcB", /* Pointer to destination URI string */
                                      NULL);         /* No Redundancy function reference */
      if (res != IPT_OK)
      {
         /* error */
      }
   }
}

void example10_init_OldSyntax(void)
{
   int res;
   void *pCallerRef = (void *)11000;

   /* Create a queue with 10 elements */
   mdq10 = MDComAPI_createQueue(10);
   if (mdq10 == 0)
   {
      /* error */
   }
   else
   {
      res = MDComAPI_addUriListenerQ(mdq10, pCallerRef, "insta.funcB");    
      if (res != IPT_OK)
      {
         /* error */
      }
   }
}

void example10_receive(void)
{
   int res;
   char sendbuf[100];
   char recbuf[200];
   UINT32 size;
   MSG_INFO msgInfo;
   char *pRecBuf;
   UINT16 userStatus;

   pRecBuf = recbuf;  /* Use application allocated buffer */
   
   while (1)
   {
      size = sizeof(recbuf);
      res = MDComAPI_getMsg(mdq10,     /* Queue ID */
                           &msgInfo, /* Message info */
                           &pRecBuf, /* Pointer to pointer to data 
                                        buffer */
                           &size,    /* Pointer to size. Size shall
                                        be set to own buffer size at
                                        the call. The IPTCom will
                                        return the number of received 
                                        bytes */
                           IPT_WAIT_FOREVER); /* Wait for result */
      if (res == MD_QUEUE_NOT_EMPTY)
      {
         if (msgInfo.msgType == MD_MSGTYPE_DATA )
         {
            /* Handle received data */
         }
         else if (msgInfo.msgType == MD_MSGTYPE_REQUEST)
         {
            /* Handle received data */

            /* Prepare response data */

            /* Set User status */
            userStatus = 0;

            /* Send a response */
            res = MDComAPI_putRespMsg(5100,  /* ComID */
                                   userStatus,  /* User Status */
                                      sendbuf,  /* Data buffer */
                                           55,  /* Number of data to be send */
                            msgInfo.sessionId,  /* Session ID, has to be the 
                                                   sameas in the received 
                                                   requestmessage */
                                            0,  /* No Caller queue ID */
                                            0,  /* Pointer to callback function */
                                            0,  /* Caller reference */
                                            0,  /* Topo counter */
                            msgInfo.srcIpAddr,  /* Destination IP address, has
                                                   to be the same as in the
                                                   received request message */
                                            0,  /* Destination URI ID */
                                            0,  /* No destination URI string */
                                            0); /* No source URI string */
            if (res != IPT_OK)
            {
               /* The sending couldn't be started. */
               /* Error handling */
            }
         }
         else
         {
            /* Not necessary, no other message types will be returned
               in this case */
         }
      }
      else
      {
         /* error */
      }
   }   
}

static   MD_QUEUE mdq11;

void example11_init(void)
{
   int res;

   /* Create a queue with 10 elements */
   mdq11 = MDComAPI_createQueue(10);
   if (mdq11 == 0)
   {
      /* error */
   }
   else
   {
      res =  MDComAPI_uriListener(mdq10,             /* Queue ID */
                                      0,             /* No Pointer to callback function */
                                      (void *)11,    /* Caller reference */
                                      6000,          /* ComId, only for joining a multicast IP address */
                                      0,             /* No Destination URI ID */
                                      "aInst.funcN", /* Pointer to destination URI string */
                                      NULL);         /* No Redundancy function reference */
      if (res != IPT_OK)
      {
         /* error */
      }
      res =  MDComAPI_uriListener(mdq10,             /* Queue ID */
                                      0,             /* No Pointer to callback function */
                                      (void *)100,   /* Caller reference */
                                      6000,          /* ComId, only for joining a multicast IP address */
                                      1,             /* No Destination URI ID */
                                      "instY.aFunc", /* Pointer to destination URI string */
                                      NULL);         /* No Redundancy function reference */
      if (res != IPT_OK)
      {
         /* error */
      }
   }
}

void example11_receive(void)
{
   int res;
   char sendbuf[100];
   UINT32 size;
   MSG_INFO msgInfo;
   char *pRecBuf;
   UINT16 userStatus;

   pRecBuf = NULL;  /* Use buffer allocated by IPTCom */
   
   res = MDComAPI_getMsg(mdq11,     /* Queue ID */
                        &msgInfo, /* Message info */
                        &pRecBuf, /* Pointer to pointer to data 
                                     buffer */
                        &size,    /* Pointer to size. The IPTCom will
                                     return the number of received 
                                     bytes */
                        IPT_NO_WAIT); /* Poll for result */
   if (res == MD_QUEUE_NOT_EMPTY)
   {
      if (msgInfo.msgType == MD_MSGTYPE_DATA )
      {
         if (msgInfo.pCallerRef == (void *)11)
         {
            /* Destination URI with function part = funcN received*/

            /* Handle received data */
         }
         else if (msgInfo.pCallerRef == (void *)100)
         {
            /* Destination URI with intance part = instY received*/

            /* Handle received data */
         }
         else
         {
            /* NOT possible */
         }
      }
      else if (msgInfo.msgType == MD_MSGTYPE_REQUEST)
      {
         /* Handle received data */

         /* Prepare response data */

         /* Set User status */
         userStatus = 0;

         /* Send a response */
         res = MDComAPI_putRespMsg(5100,  /* ComID */
                                userStatus,  /* User Status */
                                   sendbuf,  /* Data buffer */
                                        55,  /* Number of data to be send */
                         msgInfo.sessionId,  /* Session ID, has to be the 
                                                sameas in the received 
                                                requestmessage */
                                         0,  /* No Caller queue ID */
                                         0,  /* Pointer to callback function */
                                         0,  /* Caller reference */
                                         0,  /* Topo counter */
                         msgInfo.srcIpAddr,  /* Destination IP address, has
                                                to be the same as in the
                                                received request message */
                                         0,  /* Destination URI ID */
                                         0,  /* No destination URI string */
                                         0); /* No source URI string */
         if (res != IPT_OK)
         {
            /* The sending couldn't be started. */
            /* Error handling */
         }
      }

      if (pRecBuf != 0)
      {
         /* Free buffer allocated by IPTCom */
         res = MDComAPI_freeBuf(pRecBuf);
         if (res != 0)
         {
            /* error */
         }
      }
   }
   else if (res == MD_QUEUE_EMPTY)
   {
      /* Queue is empty */
   }
   else
   {
      /* error */
   }
}

#ifndef LINUX_MULTIPROC
/* URI string listeners receiving via callback function

These examples show how to receive data via a call-back function. First has a 
listener to be added.

In the example the call-back function will send back a response when a request 
message has been received.

*/

void example12_callBack(const MSG_INFO *pMsgInfo, /* Message info */
                       const char   *pData,    /* Not used for results */
                       UINT32 length);    /* Not used for results */

void example12_init(void)
{
   int res;
   int callerRef = 1002;
   
   res =  MDComAPI_uriListener(0,                      /* No Queue ID */
                                   example12_callBack, /*  Pointer to callback function */
                                   (void *)callerRef,  /* Caller reference */
                                   0,                  /* No ComId */
                                   0,                  /* No Destination URI ID */
                                   "insta.funcB",      /* Pointer to destination URI string */
                                   NULL);               /* No Redundancy function reference */
   if (res != IPT_OK)
   {
      /* error */
   }
}

void example12_init_OlsSyntax(void)
{
   int res;
   int callerRef = 1002;
   
   res = MDComAPI_addUriListenerF(example12_callBack, (void *)callerRef, "aFunc");    
   if (res != IPT_OK)
   {
      /* error */
   }
}

void example12_callBack(const MSG_INFO *pMsgInfo, /* Message info */
                       const char   *pData,    /* Not used for results */
                       UINT32 length)    /* Not used for results */
{
   int res;
   char sendbuf[100];
   UINT16 userStatus;
 
   IPT_UNUSED (pData)
   IPT_UNUSED (length)
   
   if (pMsgInfo->msgType == MD_MSGTYPE_DATA )
   {
      /* Handle received data */
   }
   else if (pMsgInfo->msgType == MD_MSGTYPE_REQUEST)
   {
      /* Handle received data */

      /* Prepare response data */

      /* Set User status */
      userStatus = 0;

      /* Send a response */
      res = MDComAPI_putRespMsg(100,  /* ComID */
                         userStatus,  /* User Status */
                            sendbuf,  /* Data buffer */
                                 80,  /* Number of data to be send */
                  pMsgInfo->sessionId,  /* Session ID, has to be the same
                                       as in the received request
                                       message */ 
                                  0,  /* No queue */
                                  0,  /* no call-back function */                            
                                  0,  /* Caller reference */
                                  0,  /* Topo counter */
                  pMsgInfo->srcIpAddr,  /* Destination IP address, has to 
                                       be the same as in the received 
                                       request message */
                                  0,  /* No destination ID */ 
                               NULL,  /* No destination URI string */
                               NULL); /* No source URI string */
      if (res != IPT_OK)
      {
         /* The sending couldn't be started. */
         /* Error handling */
      }
   }
   else
   {
      /* Not necessary, no other message types will be returned
         in this case */
   }
}
#endif

/* PD communication

These examples show how to send and receive PD data

*/

/*******************************************************************************
NAME:       example13_init
ABSTRACT:    
RETURNS:    -
*/
int example13_init(void)
{

   /* Publish comid 8000 scheduler group 1 No overide of destination*/
   hp_8000 = PDComAPI_publish(1, 8000, NULL);
   if (hp_8000 == 0)
   {
      printf("PDComAPI_publish failed\n");
      return(-1);
   }

   /* Publish comid 8010 scheduler group 1 No overide of destination*/
   hp_8010 = PDComAPI_publish(1, 8010, NULL);
   if (hp_8010 == 0)
   {
      printf("PDComAPI_publish failed\n");
      return(-1);
   }

   /* Publish comid 8020 scheduler group 2 No overide of destination*/
   hp_8020 = PDComAPI_publish(2, 8020, NULL);
   if (hp_8020 == 0)
   {
      printf("PDComAPI_publish failed\n");
      return(-1);
   }

   /* Publish comid 8100 scheduler group 2 Overide of destination with
      device ccu-o in car 02 in local consist */
   hp_8100 = PDComAPI_publish(2, 8100, "ccu-o.car02.lCst");
   if (hp_8100 == 0)
   {
      printf("PDComAPI_publish failed\n");
      return(-1);
   }

   /* Subscribe comid 7000  scheduler group 3 from all sources*/
   hs_7000 =  PDComAPI_sub(3,     /* schedule group */
                           7000,  /* comId */
                           0,     /* NoSource URI filter ID */
                           NULL,  /* Pointer to string with source URI. 
                                     Will override information in the configuration 
                                     database. 
                                     Set NULL if not used. */
                           0,      /* Destination URI Id */
                           NULL);  /* Pointer to string with destination URI. 
                                     Will override information in the configuration database. 
                                     Set NULL if not used. */
   if (hs_7000 == 0)
   {
      printf("PDComAPI_subscribe failed\n");
      return(-1);
   }

   /* Subscribe comid 7010  scheduler group 3 with filter ID and destination Id for
      joining a multicast IP address*/
   hs_7010 =  PDComAPI_sub(3,     /* schedule group */
                           7010,  /* comId */
                           1,     /* NoSource URI filter ID */
                           NULL,  /* Pointer to string with source URI. 
                                     Will override information in the configuration 
                                     database. 
                                     Set NULL if not used. */
                           1,      /* Destination URI Id */
                           NULL);  /* Pointer to string with destination URI. 
                                     Will override information in the configuration database. 
                                     Set NULL if not used. */
   if (hs_7010 == 0)
   {
      printf("PDComAPI_subscribe failed\n");
      return(-1);
   }

   /* Subscribe comid 7020  scheduler group 4 from all sources with destination URI
      used for joining a multicast IP address*/
   hs_7020 =  PDComAPI_sub(4,     /* schedule group */
                           7020,  /* comId */
                           0,     /* NoSource URI filter ID */
                           NULL,  /* Pointer to string with source URI. 
                                     Will override information in the configuration 
                                     database. 
                                     Set NULL if not used. */
                           0,     /* Destination URI Id */
                           "grp1.acar"); /* Pointer to string with destination URI. 
                                     Will override information in the configuration database. 
                                     Set NULL if not used. */
   if (hs_7020 == 0)
   {
      printf("PDComAPI_subscribe failed\n");
      return(-1);
   }

   /* Subscribe comid 7100  scheduler group 4 only from device
      efd in car 01 in local consist */
   hs_7100 = PDComAPI_subscribe(4, 7100, "efd.car01.lCst");
   hs_7100 =  PDComAPI_sub(4,                /* schedule group */
                           7100,             /* comId */
                           0,                /* NoSource URI filter ID */
                           "efd.car01.lCst", /* Pointer to string with source URI. 
                                                Will override information in the configuration 
                                                database. 
                                                Set NULL if not used. */
                           0,                /* Destination URI Id */
                           NULL);            /* Pointer to string with destination URI. 
                                                Will override information in the configuration database. 
                                                Set NULL if not used. */
   if (hs_7100 == 0)
   {
      printf("PDComAPI_subscribe failed\n");
      return(-1);
   }

   return(0);
}

int example13_init_OldSyntax(void)
{

   /* Publish comid 8000 scheduler group 1 No overide of destination*/
   hp_8000 = PDComAPI_publish(1, 8000, NULL);
   if (hp_8000 == 0)
   {
      printf("PDComAPI_publish failed\n");
      return(-1);
   }

   /* Publish comid 8010 scheduler group 1 No overide of destination*/
   hp_8010 = PDComAPI_publish(1, 8010, NULL);
   if (hp_8010 == 0)
   {
      printf("PDComAPI_publish failed\n");
      return(-1);
   }

   /* Publish comid 8020 scheduler group 2 No overide of destination*/
   hp_8020 = PDComAPI_publish(2, 8020, NULL);
   if (hp_8020 == 0)
   {
      printf("PDComAPI_publish failed\n");
      return(-1);
   }

   /* Publish comid 8100 scheduler group 2 Overide of destination with
      device ccu-o in car 02 in local consist */
   hp_8100 = PDComAPI_publish(2, 8100, "ccu-o.car02.lCst");
   if (hp_8100 == 0)
   {
      printf("PDComAPI_publish failed\n");
      return(-1);
   }

   /* Subscribe comid 7000  scheduler group 3 from all sources*/
   hs_7000 = PDComAPI_subscribe(3, 7000, NULL);
   if (hs_7000 == 0)
   {
      printf("PDComAPI_subscribe failed\n");
      return(-1);
   }

   /* Subscribe comid 7010  scheduler group 3 from all sources*/
   hs_7010 = PDComAPI_subscribe(3, 7010, NULL);
   if (hs_7010 == 0)
   {
      printf("PDComAPI_subscribe failed\n");
      return(-1);
   }

   /* Subscribe comid 7020  scheduler group 4 from all sources*/
   hs_7020 = PDComAPI_subscribe(4, 7020, NULL);
   if (hs_7020 == 0)
   {
      printf("PDComAPI_subscribe failed\n");
      return(-1);
   }

   /* Subscribe comid 7100  scheduler group 4 only from device
      efd in car 01 in local consist */
   hs_7100 = PDComAPI_subscribe(4, 7100, "efd.car01.lCst");
   if (hs_7100 == 0)
   {
      printf("PDComAPI_subscribe failed\n");
      return(-1);
   }

   return(0);
}


/*******************************************************************************
NAME:       example13_run
ABSTRACT:    
RETURNS:    -
*/
void example13_run(void)
{
   int res;
   int status;
   DATASET2 pd_ds_out_8000;
   DATASET2 pd_ds_out_8010;
   DATASET2 pd_ds_out_8020;
   DATASET2 pd_ds_out_8100;
   DATASET2 pd_ds_in_7000;
   DATASET2 pd_ds_in_7010;
   DATASET2 pd_ds_in_7020;
   DATASET2 pd_ds_in_7100;

   /* Read received data from netbuffer schedular group 3 */
   PDComAPI_sink(3);
 
   /* Read received data */
   res = PDComAPI_getWStatus(hs_7000, (BYTE*) &pd_ds_in_7000, sizeof(pd_ds_in_7000), &status);
   if (res)
   {
      printf("\nPDComAPI_get failed\n");
   }
   else
   {
      if (status == IPT_VALID)
      {
         /* Use the received data */
      }
      else if (status == IPT_INVALID_OLD)
      {
         /* Old data */;
      }
      else if (status == IPT_INVALID_NOT_RECEIVED)
      {
         /* Not received at all */
      }
      else
      {
         /* not possible */;
      }
   }

   /* Read received data */
   res = PDComAPI_get(hs_7010, (BYTE*) &pd_ds_in_7010, sizeof(pd_ds_in_7010));
   if (res)
   {
      printf("\nPDComAPI_get failed\n");
   }
   else
   {
      /* Use the received data */
   }

   /* Read received data from netbuffer schedular group 4 */
   PDComAPI_sink(4);
 
   /* Read received data */
   res = PDComAPI_get(hs_7020, (BYTE*) &pd_ds_in_7020, sizeof(pd_ds_in_7020));
   if (res)
   {
      printf("\nPDComAPI_get failed\n");
   }
   else
   {
      /* Use the received data */
   }

   /* Read received data */
   res = PDComAPI_get(hs_7100, (BYTE*) &pd_ds_in_7100, sizeof(pd_ds_in_7100));
   if (res)
   {
      printf("\nPDComAPI_get failed\n");
   }
   else
   {
      /* Use the received data */
   }

 
   /* Set output data */

   /* Write output data */
   PDComAPI_put(hp_8000, (BYTE*) &pd_ds_out_8000);

   /* Set output data */

   /* Write output data */
   PDComAPI_put(hp_8010, (BYTE*) &pd_ds_out_8010);

   /* Write output data to net buffer schedular group 1 to be transmitted */
   PDComAPI_source(1);

 
   /* Set output data */

   /* Write output data */
   PDComAPI_put(hp_8020, (BYTE*) &pd_ds_out_8020);

   /* Set output data */

   /* Write output data */
   PDComAPI_put(hp_8100, (BYTE*) &pd_ds_out_8100);

   /* Write output data to net buffer schedular group 2 to be transmitted */
   PDComAPI_source(2);

}

