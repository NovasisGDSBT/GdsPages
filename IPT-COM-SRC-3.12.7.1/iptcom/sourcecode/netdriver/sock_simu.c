/*******************************************************************************
 *  COPYRIGHT      : (c) 2006-10 Bombardier Transportation
 *******************************************************************************
 *  PROJECT        : IPTrain
 *
 *  MODULE         : sock_simu.c
 *
 *  ABSTRACT       : Support for IP/UDP address spoofing (used for simulation target)
 *
 *******************************************************************************
 *  HISTORY 
 *
 *	
 * $Id: sock_simu.c 11724 2010-11-24 09:26:23Z gweiss $
 *
 *  CR-432 (Gerhard Weiss, 2010-11-24)
 *          Corrected position of UNUSED Parameter Macros
 *          Added more missing UNUSED Parameter Macros
 *          Removed unused variables
 *
 *  CR_432 (Bernd Loehr, 2010-08-22)
 * 			Compiler error on VS 2008 (with new pcap-library)
 *
 ******************************************************************************/

#ifdef TARGET_SIMU
/*******************************************************************************
* INCLUDES */
#define INT32_ALREADY_DEFINED

#define _CRT_RAND_S
#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <search.h>

/* PCAP header files */
#include "pcap.h"
#include "remote-ext.h"

/* IPTCom includes */
#include "iptcom.h"
#include "iptcom_priv.h"

/* IP Helper API */
#include <iphlpapi.h>


/*******************************************************************************
* DEFINES */

/**************** Communication parameters *************************************/
#define SIM_SIZE_OF_ARP_CACHE    100            /* Max no of entries in  ARP cache */
#define SIM_MAX_NO_OF_SIM_IFS    100            /* Max no of simulated devices per ethernet card */
#define SIM_MAX_NO_OF_IP_FRAGS      100            /* Max no of IP fragments */
#define SIM_MAX_NO_OF_MC_GROUPS     100            /* Max no of multicast groups per card */
#define SIM_MAX_NO_OF_ETH_CARDS     12          /* Max no of Ethernet cards */

#define SIM_HOST_CHUNK           10          /* Memory allocation chunk, for multicast hosts */

#define SIM_FRAGMENT_TIMEOUT     300            /* Fragment times out after 5 minutes (60 * 5 = 300) */

#define SIM_JOIN_MC_GROUP        1           /* Join multicast group  */
#define SIM_LEAVE_MC_GROUP       2           /* Leave multicast group */
#define SIM_MC_V1_MEMBER_REPORT     3           /* Version 1 membership report */
#define SIM_MC_V3_MEMBER_REPORT     4           /* Version 3 membership report */

#define SIM_PD_PORT_NO           20548       /* UDP port used for PD */
#define SIM_MD_PORT_NO           20550       /* UDP port used for MD */
#define SIM_SNMP_PORT_NO         12030       /* UDP port used for SNMP */

#define ETH_ALEN              6           /* Ethernet address length */
#define SIM_MIN_ETH_FRAME     46          /* Minimum Ethernet frame size */

/*******************************************************************************
* TYPEDEFS */
#pragma pack( push, win_basic )

#pragma pack(1)

/* Ethernet header */
typedef struct ethhdr 
{
   UINT8 h_dest[ETH_ALEN];    /* destination eth addr */
   UINT8 h_source[ETH_ALEN];  /* source ether addr */
   UINT16 h_proto;            /* packet type ID field */
} ETH_HDR;

/* Ip header */
typedef struct iphdr
{
   UINT8 verlen;           /* Version and Length */
   UINT8 tos;              /* Type of service */
   UINT16 tot_len;            /* Length */
   UINT16 id;              /* Identity */
   UINT16 frag_off;           /* Fragment offset */
   UINT8 ttl;              /* Time to live */
   UINT8 protocol;            /* Protocol */
   UINT16 check;           /* Checksum */
   UINT32 saddr;           /* Source address */
   UINT32 daddr;           /* Destination address */
} IP_HDR;

/* UDP header */
typedef struct udphdr 
{
   UINT16 source;          /* Source port */
   UINT16 dest;            /* Destination port */
   UINT16 len;             /* Length */
   UINT16 check;           /* Checksum */
} UDP_HDR;

/* ARP telegram */
typedef struct arppacket 
{
   UINT16 hardtype;        /* Hard type */
   UINT16 prottype;        /* Prot type */
   UINT8 hardsize;            /* Hard size */
   UINT8 protsize;            /* Prot size */
   UINT16 op;              /* Op */
   UINT8 ethsender[ETH_ALEN]; /* Source eth addr   */
   UINT32 saddr;           /* Source ip address */
   UINT8 ethtarget[ETH_ALEN]; /* Target eth addr   */
    UINT32 taddr;          /* Target ip address */
} ARP_PACKET;

/* ICMP header */
typedef struct icmppacket
{
   UINT8 typ;           /* Type of ICMP telegram */
   UINT8 cod;           /* Code field */
   UINT16 chksum;          /* Checksum */
} ICMP_PACKET;

/* IGMP header */
typedef struct igmppacket
{
   UINT8 typ;           /* Type of ICMP telegram */
   UINT8 res;           /* Response time */
   UINT16 chksum;          /* Checksum */
} IGMP_PACKET;

#pragma pack( pop, win_basic )


/* IP fragment */
typedef struct ipfragment
{
   struct ipfragment *pNext;     /* Pointer to next fragment in list */
   UINT16 Offset;             /* Offset in telegram */
   int Size;                  /* Size of fragment */
   unsigned char Data[1500];     /* Fragment data */
} IP_FRAGMENT;

/* IP fragment base structure (list head) */
typedef struct ipfragbase
{
   struct ipfragment *pNext;     /* Pointer to list of IP fragments */
   int Size;                  /* Size of entire IP message */
   unsigned int Id;           /* Identity for fragment */
   UINT8  srcEthAddr[ETH_ALEN];  /* Source eth addr   */
   UINT8  destEthAddr[ETH_ALEN]; /* Source eth addr   */
   IP_HDR *pIp;               /* Pointer to IP header */
   time_t CreationTime;       /* Time when this fragment was created */
} IP_FRAGMENT_BASE;

/* This structure contains a received ethernet message */
typedef struct sockmsg
{
   struct sockmsg *pNext;        /* Next fragment in list */
   UINT32 srcaddr;               /* Source ip address */
   UINT16 srcport;               /* Source port no */
   UINT32 destaddr;           /* Destination ip address */
   int Size;                  /* Length of UDP telegram */
   unsigned char Msg[1];         /* The message data */
} SOCK_MSG;

/* List structure for received messages */
typedef struct sockmsglist
{
   SOCK_MSG *pStart;          /* Start of PD list */
   SOCK_MSG *pEnd;               /* End of PD list */
   int   InitializedCS;          /* 0 if cs is not intialized */
   CRITICAL_SECTION cs;       /* Critical section for socket handling */
} SOCK_MSG_LIST;

/* Contains info about one simulate interface */
typedef struct simifinfo
{
   UINT32 ipaddr;             /* Source ip address */
   UINT8  ethaddr[ETH_ALEN];     /* Source eth addr   */
} SIM_IF_INFO;

/* Arp cache item */
typedef struct arpinfo
{
   UINT32 ipaddr;             /* Source ip address */
   UINT8  ethaddr[ETH_ALEN];     /* Source eth addr   */
} ARP_INFO;

/* Multicast host table */
typedef struct mchostinfo
{
   UINT32 simaddr;  /* Simulated device ip address */
   UINT8  pdJoined; /* Flag. Joined for PD */   
   UINT8  mdJoined; /* Flag. Joined for MD */   
} MC_HOST_INFO;

/* Multicast info */
typedef struct mcinfo
{
   UINT32 mcaddr;             /* Multicast address */
   int   NoOfHosts;              /* No of hosts that has joined this multicast group */
   int   NoOfAllocatedHosts;        /* No of hosts that is allocated for TheHosts (i.e. how much memory) */
   MC_HOST_INFO *TheHosts;          /* Pointer to array of hosts */
} MC_INFO;

/* Ethernet card info */
typedef struct ethcard
{
   UINT32      ipaddr;                          /* Source ip address */
   UINT32      gwIpaddr;                        /* Gateway ip address */
   UINT8       ethaddr[ETH_ALEN];                  /* Source eth addr   */
   pcap_t      *pcap_fp;                        /* Handle to PCAP */
   int         NoOfSimulatedIfs;                /* No of positions used in SimulatedIf array */
   SIM_IF_INFO SimulatedIFs[SIM_MAX_NO_OF_SIM_IFS];   /* The simulated devices for this card */
   int         NoOfJoinedMCGs;                     /* No of positions used in JoinedMCGs array */
   MC_INFO     JoinedMCGs[SIM_MAX_NO_OF_MC_GROUPS];   /* Joined multicast groups for this card */
} ETH_CARD;

/*******************************************************************************
* PROTOTYPES */
static int SendARPTgm(ETH_CARD *pCard, BYTE Op, char *destMAC, SIM_IF_INFO *pSource, SIM_IF_INFO *pTarget);
static UINT16 GetEthAdrFromArpCache(ETH_CARD *pCard, SIM_IF_INFO *pSimDev, UINT32 reqIpAddr, UINT8 *pReqEthAddr);
static BYTE *AddEthernetHeader(BYTE *pPacket, char *ethDestination, char *ethSource, UINT16 type);
static int GetIPAddressFromDescription(char *pCardDescription, UINT32 *pIPAddrList, int *pSizeOfIPAddrList);
static int OpenInterface(UINT32 IPAddr);

/*******************************************************************************
* GLOBALS */

/* MAC Address for broadcast */
static BYTE gBroadcastMAC[ETH_ALEN] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

/* Empty MAC Address */
static BYTE gEmptyMAC[ETH_ALEN] =     { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

/*******************************************************************************
* LOCALS */
static ARP_INFO ArpCache[SIM_SIZE_OF_ARP_CACHE];            /* Arp cache table */
static int NoOfItemsInArpCache = 0;                      /* No of items in Arp cache */
static IP_FRAGMENT_BASE IpFragments[SIM_MAX_NO_OF_IP_FRAGS];   /* IP fragment table */
static int NoOfIpFrags = 0;                              /* No of items in IP fragment table */
static ETH_CARD gCards[SIM_MAX_NO_OF_ETH_CARDS];            /* Ethernet card table */
static int NoOfCards = 0;                             /* No of ethernet cards */

enum {SIM_PD_LIST = 0, SIM_MD_LIST, SIM_SNMP_LIST, SIM_NO_OF_LISTS};
SOCK_MSG_LIST SockMsgList[SIM_NO_OF_LISTS] = {  {NULL, NULL, 0},  /* The socket list */
                                    {NULL, NULL, 0}, 
                                    {NULL, NULL, 0}};

static UINT16 gPingSeqNo = 0;    /* Sequence no for PING message */
static UINT16 gID = 1;           /* Sequence no for IP frames */
/*******************************************************************************
* LOCAL FUNCTIONS */
static int __stdcall SimulatedNetworkTask(void *pExtThis);

/*******************************************************************************
NAME:       mcgCompare
ABSTRACT:   Compare routine for multicast group
RETURNS: -1 if element 1 > element 2, 1 if element 1 < element 2, 0 if element 1 = element 2
*/
int mcgCompare( 
   const void *arg1,  /* Reference to element 1 */
   const void *arg2 ) /* Reference to element 2 */
{
   MC_INFO *p1 = (MC_INFO *)arg1;   /* Pointer to element 1 */
   MC_INFO *p2 = (MC_INFO *)arg2;   /* Pointer to element 2 */

   if (p1->mcaddr < p2->mcaddr)
      return -1;

   if (p1->mcaddr > p2->mcaddr)
      return 1;

   return 0;
}

/*******************************************************************************
NAME:       mcHostCompare
ABSTRACT:   Compare routine for multicast group hosts
RETURNS: -1 if element 1 > element 2, 1 if element 1 < element 2, 0 if element 1 = element 2
*/
int mcHostCompare( 
   const void *arg1,  /* Reference to element 1 */
   const void *arg2 ) /* Reference to element 2 */
{
   MC_HOST_INFO *p1 = (MC_HOST_INFO *)arg1;  /* Pointer to element 1 */
   MC_HOST_INFO *p2 = (MC_HOST_INFO *)arg2;  /* Pointer to element 2 */

   if (p1->simaddr < p2->simaddr)
      return -1;

   if (p1->simaddr > p2->simaddr)
      return 1;

   return 0;
}

/*******************************************************************************
NAME:       ethCardCompareIpAddr
ABSTRACT:   Compare routine for ETH_CARD items based on ip address
RETURNS: -1 if element 1 > element 2, 1 if element 1 < element 2, 0 if element 1 = element 2
*/
int ethCardCompareIpAddr( 
   const void *arg1,  /* Reference to element 1 */
   const void *arg2 ) /* Reference to element 2 */
{
   ETH_CARD *p1 = (ETH_CARD *)arg1; /* Pointer to element 1 */
   ETH_CARD *p2 = (ETH_CARD *)arg2; /* Pointer to element 2 */

   if (p1->ipaddr < p2->ipaddr)
      return -1;

   if (p1->ipaddr > p2->ipaddr)
      return 1;

   return 0;
}

/*******************************************************************************
NAME:       simIfCompare
ABSTRACT:   Compare routine for SIM_IF_INFO items
RETURNS: -1 if element 1 > element 2, 1 if element 1 < element 2, 0 if element 1 = element 2
*/
int simIfCompare( 
   const void *arg1,  /* Reference to element 1 */
   const void *arg2 ) /* Reference to element 2 */
{
   SIM_IF_INFO *p1 = (SIM_IF_INFO *)arg1; /* Pointer to element 1 */
   SIM_IF_INFO *p2 = (SIM_IF_INFO *)arg2; /* Pointer to element 2 */

   if (p1->ipaddr < p2->ipaddr)
      return -1;

   if (p1->ipaddr > p2->ipaddr)
      return 1;

   return 0;
}

/*******************************************************************************
NAME:       arpInfoCompare
ABSTRACT:   Compare routine for ARP_INFO items
RETURNS: -1 if element 1 > element 2, 1 if element 1 < element 2, 0 if element 1 = element 2
*/
int arpInfoCompare( 
   const void *arg1,  /* Reference to element 1 */
   const void *arg2 ) /* Reference to element 2 */
{
   ARP_INFO *p1 = (ARP_INFO *)arg1; /* Pointer to element 1 */
   ARP_INFO *p2 = (ARP_INFO *)arg2; /* Pointer to element 2 */

   if (p1->ipaddr < p2->ipaddr)
      return -1;

   if (p1->ipaddr > p2->ipaddr)
      return 1;

   return 0;
}

/*******************************************************************************
NAME:       fragBaseCompare
ABSTRACT:   Compare routine for IP_FRAGMENT_BASE items
RETURNS: -1 if element 1 > element 2, 1 if element 1 < element 2, 0 if element 1 = element 2
*/
int fragBaseCompare( 
   const void *arg1,  /* Reference to element 1 */
   const void *arg2 ) /* Reference to element 2 */
{
   IP_FRAGMENT_BASE *p1 = (IP_FRAGMENT_BASE *)arg1;   /* Pointer to element 1 */
   IP_FRAGMENT_BASE *p2 = (IP_FRAGMENT_BASE *)arg2;   /* Pointer to element 2 */

   if (p1->Id < p2->Id)
      return -1;

   if (p1->Id > p2->Id)
      return 1;

   return 0;
}

/*******************************************************************************
NAME:       FindEthernetAddressInArpCache
ABSTRACT:   Add the Ethernet and IP address to the ARP cache
RETURNS:    Pointer to requested item or NULL if item not found
*/
ARP_INFO *FindEthernetAddressInArpCache(
   UINT32 ipAddr) /* IP address */
{
   ARP_INFO searchItem; /* Search key */

   /* Setup search key */
   searchItem.ipaddr = ipAddr;   

   /* Try to get item from ARP cache */
   return (bsearch((const void *)&searchItem, (const void *)ArpCache, NoOfItemsInArpCache, sizeof(ARP_INFO), arpInfoCompare));
}

/*******************************************************************************
NAME:       FindSimulatedInterface
ABSTRACT:   Find simulated interface
RETURNS:    Pointer to requested item or NULL if item not found
*/
SIM_IF_INFO *FindSimulatedInterface(
   ETH_CARD *pCard,  /* Pointer to card info */
   UINT32 ipAddr)    /* Simulated device ip address */
{
   SIM_IF_INFO searchItem;    /* Search key */

   /* Sanity check */
   if (pCard == NULL)
      return NULL;

   /* Setup search key */
   searchItem.ipaddr = ipAddr;   

   /* Try to get item from ARP cache */
   return (bsearch((const void *)&searchItem, (const void *)pCard->SimulatedIFs, 
      pCard->NoOfSimulatedIfs, sizeof(SIM_IF_INFO), simIfCompare));
}

/*******************************************************************************
NAME:       FindMCGroup
ABSTRACT:   Find multicast group
RETURNS:    Pointer to requested item or NULL if item not found
*/
MC_INFO *FindMCGroup(
   ETH_CARD *pCard, /* Pointer to card info */
   UINT32 mcAddr)      /* Multicast address */
{
   MC_INFO searchItem;     /* Search key */

   /* Sanity check */
   if (pCard == NULL)
      return NULL;

   /* Setup search key */
   searchItem.mcaddr = mcAddr;   

   /* Try to get item from ARP cache */
   return (bsearch((const void *)&searchItem, (const void *)pCard->JoinedMCGs,
      pCard->NoOfJoinedMCGs, sizeof(MC_INFO), mcgCompare));
}

/*******************************************************************************
NAME:       FindEthernetCard
ABSTRACT:   Find ethernet card from ip address
RETURNS:    Pointer to requested item or NULL if item not found
*/
ETH_CARD *FindEthernetCard(
   UINT32 ipAddr) /* IP address of simulated card */
{
   ETH_CARD searchItem; /* Search key */

   /* Setup search key */
   searchItem.ipaddr = ipAddr;   

   /* Try to get item from ARP cache */
   return (bsearch((const void *)&searchItem, (const void *)gCards, NoOfCards, sizeof(ETH_CARD), ethCardCompareIpAddr));
}

/*******************************************************************************
NAME:       FindEthernetCardByDeviceIp
ABSTRACT:   Find ethernet card used by a specific simulated device
RETURNS:    Pointer to requested item or NULL if item not found
*/
ETH_CARD *FindEthernetCardByDeviceIp(
   UINT32 deviceIpAddr)   /* Simulated device IP address */
{
   int i;            /* Loop variable */
   SIM_IF_INFO *pIf; /* Pointer to simulated device info */

   /* For all registered cards */
   for (i = 0; i < NoOfCards; i++)
   {
      /* Is the device simulated for this card */
      pIf = FindSimulatedInterface(&gCards[i], deviceIpAddr);
      if (pIf != NULL)
         return &gCards[i];
   }

   return NULL;
}

/*******************************************************************************
NAME:       FindFragment
ABSTRACT:   Find simulated interface
RETURNS:    Pointer to requested item or NULL if item not found
*/
IP_FRAGMENT_BASE *FindFragment(
   UINT16 Id)  /* Fragment id */
{
   IP_FRAGMENT_BASE searchItem;  /* Search key */

   /* Setup search key */
   searchItem.Id = Id;   

   /* Try to get item from fragment list */
   return (bsearch(&searchItem, IpFragments, NoOfIpFrags, sizeof(IP_FRAGMENT_BASE), fragBaseCompare));
}

/*******************************************************************************
NAME:       FindMCHost
ABSTRACT:   Find host in multicast host list
RETURNS:    Pointer to requested item or NULL if item not found
*/
MC_HOST_INFO *FindMCHost(
   UINT32 hostIP,           /* The host to search for */
   MC_HOST_INFO *pHostList, /* The list to search in */
   int  NoOfHosts)          /* No of items in list   */
{
   UINT32 searchItem;      /* Search key */

   /* Setup search key */
   searchItem = hostIP;   

   /* Try to get item from host list */
   return (bsearch((const void *)&searchItem, (const void *)pHostList, NoOfHosts, sizeof(MC_HOST_INFO), mcHostCompare));
}

/*******************************************************************************
NAME:       AddHostToMCGroup
ABSTRACT:   Add a host to the multicast group host list
RETURNS:    1 if success, 0 otherwise
*/
static UINT16 AddHostToMCGroup(
   UINT32 deviceIP,     /* The ip address that joins the multicast group */
   MC_INFO *ptrToItem,  /* Multicast group info */
   UINT8 pd,            /* Join PD */
   UINT8 md)            /* Join MD */
{
   MC_HOST_INFO *ptrToHost = NULL;  /* Pointer to host data */

   /* Try to find the host in the multicast list */
   ptrToHost = FindMCHost(deviceIP, ptrToItem->TheHosts, ptrToItem->NoOfHosts);

   /* Found item ? */
   if (ptrToHost != NULL)
   {
      if (pd)
      {
         ptrToHost->pdJoined = pd;
      }
      if (md)
      {
         ptrToHost->mdJoined = md;
      }
      return 1;
   }

   /* Size available in multicast list ? */
   if (ptrToItem->NoOfHosts >= ptrToItem->NoOfAllocatedHosts)
   {
      /* No. Get some more memory */
      ptrToHost = (MC_HOST_INFO *)malloc((ptrToItem->NoOfAllocatedHosts + SIM_HOST_CHUNK)* sizeof(MC_HOST_INFO));
      if (ptrToHost == NULL)
         return 0;

      /* Copy old data */
      memcpy(ptrToHost, ptrToItem->TheHosts, ptrToItem->NoOfAllocatedHosts * sizeof(MC_HOST_INFO));
      ptrToItem->NoOfAllocatedHosts += SIM_HOST_CHUNK;
      
      /* Return old memory and correct pointer */
      free(ptrToItem->TheHosts);
      ptrToItem->TheHosts = ptrToHost;
   }

   ptrToItem->TheHosts[ptrToItem->NoOfHosts].simaddr = deviceIP;
   ptrToItem->TheHosts[ptrToItem->NoOfHosts].pdJoined = pd;
   ptrToItem->TheHosts[ptrToItem->NoOfHosts].mdJoined = md;
   ptrToItem->NoOfHosts++;

   /* Sort array */
   (void)qsort(ptrToItem->TheHosts, ptrToItem->NoOfHosts, sizeof(MC_HOST_INFO), mcHostCompare);

   return 1;
}
/*******************************************************************************
NAME:       AddToArpCache
ABSTRACT:   Add the Ethernet and IP address to the ARP cache
RETURNS:    1 on success, 0 otherwise
*/
static UINT16 AddToArpCache(
   UINT32 ipAddr,    /* IP address to add */
   UINT8 *pEthAddr)  /* MAC address to add */
{
   ARP_INFO *ptrToItem = NULL;      /* Pointer to ARP info */

   /* Try to find item in ARP cache */
   ptrToItem = FindEthernetAddressInArpCache(ipAddr);

   /* Found item ? */
   if (ptrToItem)
   {
      /* Yes. Copy Ethernet address */
      memcpy(&ptrToItem->ethaddr, pEthAddr, ETH_ALEN);
   }
   else
   {
      /* Size available in ARP cache ? */
      if (NoOfItemsInArpCache >= SIM_SIZE_OF_ARP_CACHE)
         return 0;

      /* No. Add new item */
      ptrToItem = &ArpCache[NoOfItemsInArpCache];
      ptrToItem->ipaddr = ipAddr;
      memcpy(&ptrToItem->ethaddr, pEthAddr, ETH_ALEN);      
      NoOfItemsInArpCache++;

      /* Sort array */
      (void)qsort(ArpCache, NoOfItemsInArpCache, sizeof(ARP_INFO), arpInfoCompare);
   }

   return 1;
}
/*******************************************************************************
NAME:       AddMCG
ABSTRACT:   Add a multicast group to the list
RETURNS:    1 if new and success, 0 otherwise
*/
static UINT16 AddMCG(
   ETH_CARD *pCard, /* Pointer to ethernet adapter data */
   UINT32 MCgroup,  /* Multicast group to remove   */
   UINT32 DeviceIP, /* Simulated device ip address */
   UINT8 pd,        /* Join PD */
   UINT8 md)        /* Join MD */
{
   MC_INFO *ptrToItem;     /* Pointer to requested multicast group */

   /* Try to find item in MCG table */
   ptrToItem = FindMCGroup(pCard, MCgroup);

   /* Found item ? */
   if (!ptrToItem)
   {
      /* Size available in MCG table ? */
      if (pCard->NoOfJoinedMCGs >= SIM_MAX_NO_OF_MC_GROUPS)
         return 0;

      /* Add new item */
      ptrToItem = &pCard->JoinedMCGs[pCard->NoOfJoinedMCGs];
      ptrToItem->mcaddr = MCgroup;
      pCard->NoOfJoinedMCGs++;
      
      /* Alloc new host list */
      ptrToItem->TheHosts = (MC_HOST_INFO *)malloc(SIM_HOST_CHUNK * sizeof (MC_HOST_INFO));
      if (ptrToItem->TheHosts == NULL)
         return 0;
      
      /* Add device */
      ptrToItem->TheHosts->simaddr = DeviceIP;
      ptrToItem->TheHosts->pdJoined = pd;
      ptrToItem->TheHosts->mdJoined = md;
      ptrToItem->NoOfHosts = 1;
      ptrToItem->NoOfAllocatedHosts = SIM_HOST_CHUNK;

      /* Sort array */
      (void)qsort(pCard->JoinedMCGs, pCard->NoOfJoinedMCGs, sizeof(MC_INFO), mcgCompare);
      return 1;
   }
   else
   {
      /* Multicast group found. Add host */
      AddHostToMCGroup(DeviceIP, ptrToItem, pd, md);
      return 0;
   }

}

/*******************************************************************************
NAME:       RemoveMCG
ABSTRACT:   Remove a multicast group for a specific card. 
RETURNS:    1 = leave the multicast group 0 = don't leave
*/
static UINT16 RemoveMCG(
   ETH_CARD *pCard,  /* Pointer to ethernet adapter data */
   UINT32 MCgroup,   /* Multicast group to remove   */
   UINT32 DeviceIP,  /* Simulated device ip address */
   UINT8 pd,         /* Leave PD */
   UINT8 md)         /* Leave MD */
{
   MC_INFO *ptrToItem;              /* Pointer to requested multicast group */
   MC_HOST_INFO *ptrToHost = NULL;  /* Pointer to host data */
   int itemInx;                     /* Array index for multicast item */
   int i;                           /* Loop index */

   /* Try to find item in the multicast data structure */
   ptrToItem = FindMCGroup(pCard, MCgroup);

   /* Found item ? */
   if (ptrToItem != NULL)
   {
      /* Try to find the host in the multicast list */
      ptrToHost = FindMCHost(DeviceIP, ptrToItem->TheHosts, ptrToItem->NoOfHosts);

      /* Host found ? */
      if (ptrToHost != NULL)
      {
         if (pd)
         {
            ptrToHost->pdJoined = 0;   
         }
         if (md)
         {
            ptrToHost->mdJoined = 0;   
         }

         if ((ptrToHost->pdJoined == 0) && (ptrToHost->mdJoined == 0))
         {
            /* Host found. Remove host from multicast host table */
         
            /* Calculate index in array */
            itemInx = ((unsigned long)ptrToHost - (unsigned long)ptrToItem->TheHosts) / sizeof(MC_HOST_INFO);

            /* Remove item by copying remaining data */
            for (i = itemInx + 1; i < ptrToItem->NoOfHosts; i++)
            {
               ptrToItem->TheHosts[i - 1] = ptrToItem->TheHosts[i];     
            }
   
            /* Decrement interface counter */
            ptrToItem->NoOfHosts--;
         }
      }
      
      /* All multicast hosts removed ? */
      if (ptrToItem->NoOfHosts < 1)
      {     
         /* All multicast hosts removed. Remove multicast group */
         
         /* Return memory allocated for host list*/
         if (ptrToItem->TheHosts != NULL)
            free (ptrToItem->TheHosts);
         
         /* Calculate index in array */
         itemInx = ((unsigned long)ptrToItem - (unsigned long)pCard->JoinedMCGs) / sizeof(MC_INFO);

         /* Remove item by copying remaining data */
         for (i = itemInx + 1; i < pCard->NoOfJoinedMCGs; i++)
         {
            pCard->JoinedMCGs[i - 1] = pCard->JoinedMCGs[i];      
         }

         /* Decrement interface counter */
         pCard->NoOfJoinedMCGs--;
      
         return 1;
      }
   }

   return 0;
}

/*******************************************************************************
NAME:       AddSimulatedInterface
ABSTRACT:   Add the Ethernet and IP address to the list of simulated interfaces
RETURNS:    1 if success, 0 otherwise
*/
static UINT16 AddSimulatedInterface(
   ETH_CARD *pCard,   /* Pointer to ethernet card info */
   UINT32 ipAddr,     /* Ip address of the simulated device */
   BYTE *pMACAddress) /* MAC address of simulated device or NULL */
{
   SIM_IF_INFO *ptrToItem = NULL;   /* Pointer to simulated interface */
   int r;                     /* Random number used when creating MAC address */

   /* Try to find item in ARP cache */
   ptrToItem = FindSimulatedInterface(pCard, ipAddr);

   /* Found item ? */
   if (ptrToItem == NULL)
   {
      /* Size available in ARP cache ? */
      if (pCard->NoOfSimulatedIfs >= SIM_MAX_NO_OF_SIM_IFS)
         return 0;

      /* No. Add new item */
      ptrToItem = &pCard->SimulatedIFs[pCard->NoOfSimulatedIfs];
      ptrToItem->ipaddr = ipAddr;

      /* MAC address supplied ? */
      if (pMACAddress == NULL)
      {     
         /* No MAC address. Invent one */
         ptrToItem->ethaddr[0] = 0x00;
         ptrToItem->ethaddr[1] = 0x1c;
         ptrToItem->ethaddr[2] = 0x23;
         rand_s(&r);
         ptrToItem->ethaddr[3] = (r & 0x000000FF);
         rand_s(&r);
         ptrToItem->ethaddr[4] = (r & 0x000000FF);
         rand_s(&r);
         ptrToItem->ethaddr[5] = (r & 0x000000FF);

         printf("\nMAC - %02X%02X%02X%02X%02X%02X\n", ptrToItem->ethaddr[0], ptrToItem->ethaddr[1],
                                             ptrToItem->ethaddr[2], ptrToItem->ethaddr[3],
                                             ptrToItem->ethaddr[4], ptrToItem->ethaddr[5]);
      }
      else
      {
         /* Copy MAC address */
         memcpy(ptrToItem->ethaddr, pMACAddress, ETH_ALEN);
      }

      pCard->NoOfSimulatedIfs++;

      /* Sort array */
      (void)qsort((void *)pCard->SimulatedIFs, pCard->NoOfSimulatedIfs, sizeof(SIM_IF_INFO), simIfCompare);
   }

   return 1;
}

/*******************************************************************************
NAME:       RemoveSimulatedInterface
ABSTRACT:   Remove device from simulation
RETURNS:    1 if success, 0 otherwise
*/
static UINT16 RemoveSimulatedInterface(
   UINT32 ipAddr) /* IP address of the interface to remove */
{
   SIM_IF_INFO *ptrToItem = NULL;   /* Pointer to simulated device */
   ETH_CARD *pCard = NULL;       /* Pointer to card data */
   MC_HOST_INFO *ptrToHost = NULL;     /* Pointer to host data */
   int itemInx;               /* Array index of simulated device */
   int i;                     /* Loop variable */

   /* Locate card */ 
   pCard = FindEthernetCardByDeviceIp(htonl(ipAddr));
   if (pCard == NULL)
      return 0;

   /* Try to find the simulated device */
   ptrToItem = FindSimulatedInterface(pCard, htonl(ipAddr));

   /* Device found ? */
   if (ptrToItem != NULL)
   {
      /* Leave all multicast groups for this interface */
      for (i = 0; i < pCard->NoOfJoinedMCGs; i++)
      {
         /* Member in this group ? */
         ptrToHost = FindMCHost(ipAddr, pCard->JoinedMCGs[i].TheHosts, pCard->JoinedMCGs[i].NoOfHosts);
         if (ptrToHost != NULL)
         {
            /* Yes. Leave it */
            IPTCom_SimLeaveMulticastgroup(ipAddr, pCard->JoinedMCGs[i].mcaddr, 1, 1);
         }
      }

      /* Yes. Calculate which index the device has in the array */
      itemInx = ((unsigned long)ptrToItem - (unsigned long)pCard->SimulatedIFs) / sizeof(SIM_IF_INFO);

      /* Remove device from array by copying the remaining data */
      for (i = itemInx + 1; i < pCard->NoOfSimulatedIfs; i++)
      {
         pCard->SimulatedIFs[i - 1] = pCard->SimulatedIFs[i];     
      }

      /* Decrement interface counter */
      pCard->NoOfSimulatedIfs--;
   }

   return 1;
}

/*******************************************************************************
NAME:       AddEthCard
ABSTRACT:   Add a new ethernet card to the datastructure. Spawns network task
RETURNS:    1 on success, 0 otherwise
*/
static UINT16 AddEthCard(
   UINT32 ipAddr,   /* Ip address of interface */
   BYTE *pEthAddr,  /* Ethernet address of interface */
   char *pDescr,    /* Description of interface */
   pcap_t *pcap_fp) /* Handle to pcap */
{
   ETH_CARD *ptrToItem = NULL;      /* Pointer to card data */
   HANDLE hThread;               /* Thread handle for network task */

   IPT_UNUSED (pDescr)

   /* Is the card already defined ? */
   ptrToItem = FindEthernetCard(ipAddr);

   /* Card found ? */
   if (!ptrToItem)
   {
      /* Size available in table ? */
      if (NoOfCards >= SIM_MAX_NO_OF_ETH_CARDS)
         return 0;

      /* Yes. Add new item */
      ptrToItem = &gCards[NoOfCards];
      ptrToItem->pcap_fp = pcap_fp;
      ptrToItem->ipaddr = ipAddr;
      ptrToItem->gwIpaddr = htonl(0x0a000001);
      memcpy(ptrToItem->ethaddr, pEthAddr, ETH_ALEN);
      ptrToItem->NoOfSimulatedIfs = 0;
      ptrToItem->NoOfJoinedMCGs = 0;

      /* Add network card address to simulated interfaces */
      /* so it will answer to ARP for the card IP         */
      AddSimulatedInterface(ptrToItem, ipAddr, pEthAddr);

      /* Create the second thread. */
      hThread = (HANDLE)_beginthreadex( NULL, 0, SimulatedNetworkTask, (void *)ipAddr, 0, NULL );
      if ((unsigned long)hThread == (unsigned long) -1)
         return 0;

      /* Set the scheduling priority of the thread */
      /*
      if(SetThreadPriority(hThread, THREAD_PRIORITY_ABOVE_NORMAL) == 0)
         return 0;
      */

      NoOfCards++;

      (void)qsort((void *)gCards, NoOfCards, sizeof(ETH_CARD), ethCardCompareIpAddr);
   }

   return 1;
}

/*******************************************************************************
NAME:       LocateOldFragment
ABSTRACT:   Searches for a position in the fragment table which contains a
            fragment that is old.
RETURNS:    Index in IpFragment table that contains the old fragment, or
         SIM_MAX_NO_OF_IP_FRAGS if failed
*/
int LocateOldFragment()
{
   time_t theTime;      /* The timeout time */
   int i;            /* Loop index */

   /* Get current time */
   theTime = time(NULL);
   
   /* Execution time higher than SIM_FRAGMENT_TIMEOUT seconds ? */
   /* This is to avoid underflow of theTime                     */
   if (theTime < SIM_FRAGMENT_TIMEOUT)
      return SIM_MAX_NO_OF_IP_FRAGS;

   /* Calculate the maximum creation time */
   theTime -= SIM_FRAGMENT_TIMEOUT;

   /* For all fragments */
   for (i = 0; i < SIM_MAX_NO_OF_IP_FRAGS; i++)
   {
      /* If this fragment has timed out, reuse this index */
      if (IpFragments[i].CreationTime < theTime)
         return i;
   }

   return SIM_MAX_NO_OF_IP_FRAGS;
}

/*******************************************************************************
NAME:       AddFragment
ABSTRACT:   Handles fragmented IP messages
RETURNS:    Pointer to fragment base or NULL if failed
*/
IP_FRAGMENT_BASE *AddFragment(
   UINT16 Id,         /* Fragment identity      */
   UINT16 FragOffset, /* Fragment offset        */
   ETH_HDR *pEth,     /* Pointer to ethernet header */
   IP_HDR *pIp,       /* Pointer to ip header   */
   UDP_HDR *pUdp,     /* Pointer to udp header  */
   int UDPLen)        /* Length of udp telegram */
{
   IP_FRAGMENT_BASE *ptrToBase = NULL; /* Pointer to fragment base */
   IP_FRAGMENT *pNewItem = NULL;    /* Pointer to the newly added fragment */
   IP_FRAGMENT *pTmpOffset = NULL;     /* Search pointer */

   /* Check if this Id has been received before */
   ptrToBase = FindFragment(Id);

   /* Found any fragments ? */
   if (ptrToBase == NULL) 
   {
      /* No. Fragment flag set ? (Sanity check)*/
      if ((FragOffset & 0x2000) == 0)
      {     
         /* No. Fragment Id found in telegram but flags not set. Discard */
         return NULL;
      }

      /* Is there enough room for the fragment ? */
      if (NoOfIpFrags >= SIM_MAX_NO_OF_IP_FRAGS)
      {
         /* No. Try to reuse old positions */
         NoOfIpFrags = LocateOldFragment();  

         /* Return if a position was not found */
         if (NoOfIpFrags >= SIM_MAX_NO_OF_IP_FRAGS)
            return NULL;

         /* Return allocated memory */
         pNewItem = IpFragments[NoOfIpFrags].pNext;
         while (pNewItem != NULL)
         {
            pTmpOffset = pNewItem;
            pNewItem = pNewItem->pNext;
            free((BYTE *)pTmpOffset);
         }
      }

      /* Found a place in fragment table. Add new base item */
      ptrToBase = &IpFragments[NoOfIpFrags];
      ptrToBase->Id = Id;
      ptrToBase->pNext = NULL;
      ptrToBase->Size = 0;
      ptrToBase->CreationTime = time(NULL);
      ptrToBase->pIp = NULL;

      /* Get data from ethernet header */
      memcpy(ptrToBase->srcEthAddr, pEth->h_source, ETH_ALEN);
      memcpy(ptrToBase->destEthAddr, pEth->h_dest, ETH_ALEN);

      /* Copy IP header if it is supplied*/
      if (pIp != NULL)
      {
         /* Allocate memory */
         ptrToBase->pIp = (IP_HDR *)malloc(((pIp->verlen & 0x0F) * 4)); /*lint !e433 !e826 pIp->verlen (lower 4 bits) holds size of ip header in long words */   
         if (ptrToBase->pIp == NULL)
            return NULL;

         /* Copy header */
         memcpy(ptrToBase->pIp, pIp, ((pIp->verlen & 0x0F) * 4));
      }

      /* Do not increment this counter if a position is reused */
      if (NoOfIpFrags < SIM_MAX_NO_OF_IP_FRAGS)
         NoOfIpFrags++;

      (void)qsort((void *)IpFragments, NoOfIpFrags, sizeof(IP_FRAGMENT_BASE), fragBaseCompare);
   }

   /* At this point we have created a fragment base. Now handle the fragmented data */
   
   /* Create a buffer for the data */  
   pNewItem = (IP_FRAGMENT *)malloc(sizeof(IP_FRAGMENT));
   if (pNewItem == NULL)
      return NULL;

   /* Setup buffer */
   pNewItem->pNext = NULL;
   pNewItem->Offset = (FragOffset & 0x1FFF) * 8;
   pNewItem->Size = UDPLen;
   memcpy(pNewItem->Data, pUdp, UDPLen); 
   ptrToBase->Size += UDPLen;

   /* Fragmented data prepared. Now link fragment and fragment base together */
   
   /* First fragment ? */
   if (ptrToBase->pNext == NULL)
   {
      ptrToBase->pNext = pNewItem;
      return ptrToBase;
   }

   /* First in existing fragment list ? */
   if (ptrToBase->pNext->Offset > (FragOffset & 0x1FFF) * 8)
   {
      /* Yes. Link in item */
      pNewItem->pNext = ptrToBase->pNext;
      ptrToBase->pNext = pNewItem;
      return ptrToBase; 
   }  

   /* Search for insertion point */
   pTmpOffset = ptrToBase->pNext;
   while (pTmpOffset != NULL)
   {
      if (pTmpOffset->pNext == NULL)
      {
         pTmpOffset->pNext = pNewItem;
         return ptrToBase;
      }

      if (pTmpOffset->pNext->Offset > (FragOffset & 0x1FFF) * 8)
      {
         pNewItem->pNext = pTmpOffset->pNext;
         pTmpOffset->pNext = pNewItem;
         return ptrToBase; 
      }

      pTmpOffset = pTmpOffset->pNext;
   }

   /* Failed to link item. Return memory */
   free((BYTE *)pNewItem);    
   return NULL;
}


/*******************************************************************************
NAME:       RestoreIPMessage
ABSTRACT:   Restores IP messages that has been fragmented
RETURNS:    1 on success, 0 otherwise
*/
int RestoreIPMessage(
   IP_FRAGMENT_BASE *pBase,      /* Pointer to fragment list */
   struct pcap_pkthdr **header,  /* PCAP header */
   u_char **pkt_data)            /* The restored telegram */
{
   struct pcap_pkthdr *pTmpHdr;  /* Pointer to created PCAP header */
   IP_FRAGMENT *pIpFr;           /* Pointer to fragment that should be added to IP message */
   IP_FRAGMENT *pDelFr;       /* Points to fragment memory that should be returned */
   u_char *pPacket;           /* The restored message */
   int MsgLen;                /* Size of IP telegram */

   *header = NULL;
   *pkt_data = NULL;

   /* Sanity check 1*/
   if (pBase == NULL)
      return 0;

   /* Sanity check 2*/
   if (pBase->pIp == NULL)
      return 0;

   /* Calculate the size of the IP message */
   MsgLen = 14 + ((pBase->pIp->verlen & 0x0F) * 4) + pBase->Size;

   /* Allocate a PCAP header */
   pTmpHdr = (struct pcap_pkthdr *)malloc(sizeof(struct pcap_pkthdr));
   if (pTmpHdr == NULL)
      return 0;

   /* Fill header */
   pTmpHdr->caplen = MsgLen;
   pTmpHdr->len = MsgLen;

   /* Allocate memory for the IP message*/
   pPacket = malloc(MsgLen);
   if (pPacket == NULL)
   {
      /* Return previously allocated memory */
      free(pTmpHdr);
      return 0;
   }

   *header = pTmpHdr;
   *pkt_data = pPacket;

   /* Add Ethernet header to message */
   pPacket = AddEthernetHeader(pPacket, pBase->destEthAddr, pBase->srcEthAddr, 0x0800);

   /* Add IP header to message */
   memcpy(pPacket, pBase->pIp, ((pBase->pIp->verlen & 0x0F) * 4));
   pPacket += ((pBase->pIp->verlen & 0x0F) * 4);

   /* Add data from IP fragments */
   pIpFr = pBase->pNext;

   /* For all fragments */
   while (pIpFr != NULL)
   {
      /* Copy fragment data */
      memcpy(&pPacket[pIpFr->Offset], pIpFr->Data, pIpFr->Size);

      /* Return fragment memory */
      pDelFr = pIpFr;
      pIpFr = pIpFr->pNext;
      free((BYTE *)pDelFr);
   }

   /* Remove fragment data structure */
   pBase->Id = 0xFFFFFFFF;
   (void)qsort((void *)IpFragments, NoOfIpFrags, sizeof(IP_FRAGMENT_BASE), fragBaseCompare);

   if (NoOfIpFrags > 0)
      NoOfIpFrags--;

   return 1;
}

/*******************************************************************************
NAME:       GetCardRef
ABSTRACT:   Get a reference to the specified Ethernet adapter
RETURNS:    Pointer to card info or NULL
*/
ETH_CARD *GetCardRef(
   char *pCardRef)  /* Reference to Ethernet card */
{
   int byte3, byte2, byte1, byte0;     /* The different digits in the IP address */
   ETH_CARD *pCard = NULL;          /* Pointer to Ethernet card */
   UINT32 ipAddr;                /* Numeric representation of IP address */
   int i;                        /* Loop variable */
   UINT32 ipAddrList[100];          /* Array that holds IP addresses for the card */
   int ipAddrListSize = 100;        /* No of elements in ipAddrList */

   /* Is the card reference an IP address ? */
   if (sscanf(pCardRef, "%d.%d.%d.%d", &byte3, &byte2, &byte1, &byte0) == 4)
   {
      if (((byte3 >= 0) && (byte3 < 256)) && ((byte2 >= 0) && (byte2 < 256)) &&
         ((byte1 >= 0) && (byte1 < 256)) && ((byte0 >= 0) && (byte0 < 256)))
      {
         /* Create ip address */
         ipAddr = (byte3 << 24) + (byte2 << 16) + (byte1 << 8) + (byte0);
         ipAddr = htonl(ipAddr);

         /* Try to locate ethernet card based on ip address */
         pCard = FindEthernetCard(ipAddr);
         if (pCard != NULL)
            return pCard;

         /* Card is not found. Try to open interface */     
         if (OpenInterface(ipAddr) == IPT_OK)
         {
            /* Get reference to Ethernet card */
            pCard = FindEthernetCard(ipAddr);
         }
      }
   }
   else
   {
      /* Find the IP addresses of the Ethernet card that has this description */
      if (GetIPAddressFromDescription(pCardRef, ipAddrList, &ipAddrListSize))
      {
         /* Empty list ? */
         if (ipAddrListSize == 0)
            return NULL;
         
         /* Try to locate card */
         for (i = 0; i < ipAddrListSize; i++)
         {
            /* Get reference to Ethernet card */
            pCard = FindEthernetCard(ipAddrList[i]);
            if (pCard != NULL)
               return pCard;
         }

         /* Card not is found. Try to open interface for the first IP address*/     
         if (OpenInterface(ipAddrList[0]) == IPT_OK)
         {
            /* Get reference to Ethernet card */
            pCard = FindEthernetCard(ipAddrList[0]);
         }
      }
   }

   return pCard;
}

/*******************************************************************************
NAME:       IPTCom_SimRegisterSimulatedInterface
ABSTRACT:   Add an interface to simulation
RETURNS:    1 on success, 0 otherwise
*/
int IPTCom_SimRegisterSimulatedInterface(
   char *pCardRef,      /* Reference to ethernet card (IP address ) */
   char *pIpAddr)       /* Interface to add to simulation */
{
   ETH_CARD *pCard = NULL;          /* Pointer to Ethernet card information*/
   UINT32 ipAddr;                /* Numeric representation of IP address */
   int byte3, byte2, byte1, byte0;     /* The different digits in the IP address */

   /* Get a pointer to Ethernet card information */
   pCard = GetCardRef(pCardRef);

   /* Still no card ? */
   if (pCard == NULL)
      return 0;

   /* Is the simulated device reference a valid ip address ? */
   if (sscanf(pIpAddr, "%d.%d.%d.%d", &byte3, &byte2, &byte1, &byte0) == 4)
   {
      if (((byte3 >= 0) && (byte3 < 256)) && ((byte2 >= 0) && (byte2 < 256)) &&
         ((byte1 >= 0) && (byte1 < 256)) && ((byte0 >= 0) && (byte0 < 256)))
      {
         ipAddr = (byte3 << 24) + (byte2 << 16) + (byte1 << 8) + (byte0);
      }
      else
         return 0;
   }
   else
      return 0;

   /* Add simulated device to this Ethernet card */
   return AddSimulatedInterface(pCard, htonl(ipAddr), NULL);
}

/*******************************************************************************
NAME:       IPTCom_SimUnRegisterSimulatedInterface
ABSTRACT:   Remove interface from simulation
RETURNS:    1 on succeess, 0 otherwise
*/
int IPTCom_SimUnRegisterSimulatedInterface(
   char *pIpAddr)  /* IP address of the interface to remove */
{
   UINT32 ipAddr;                /* IP address, numeric representation */
   int byte3, byte2, byte1, byte0;     /* The different digits in the IP address */

   /* Is the simulated device reference a valid ip address ? */
   if (sscanf(pIpAddr, "%d.%d.%d.%d", &byte3, &byte2, &byte1, &byte0) == 4)
   {
      if (((byte3 >= 0) && (byte3 < 256)) && ((byte2 >= 0) && (byte2 < 256)) &&
         ((byte1 >= 0) && (byte1 < 256)) && ((byte0 >= 0) && (byte0 < 256)))
      {
         ipAddr = (byte3 << 24) + (byte2 << 16) + (byte1 << 8) + (byte0);
      }
      else
         return 0;
   }

   /* Remove interface from simulation */
   return RemoveSimulatedInterface(ipAddr);
}

/*******************************************************************************
NAME:       IPTCom_SimAddDefaultGateway
ABSTRACT:   Remove interface from simulation
RETURNS:    1 on succeess, 0 otherwise
*/
int IPTCom_SimAddDefaultGateway(
   char *pCardRef,      /* Reference to ethernet card (IP address ) */
   char *pIpAddr)       /* Ip address of default gateway */
{
   ETH_CARD *pCard = NULL;             /* Pointer to Ethernet card information*/
   UINT32 ipAddr;                      /* Numeric representation of IP address */
   int byte3, byte2, byte1, byte0;     /* The different digits in the IP address */

   /* Get a pointer to Ethernet card information */
   pCard = GetCardRef(pCardRef);

   /* Still no card ? */
   if (pCard == NULL)
      return 0;

   /* Is the default gateway reference a valid ip address ? */
   if (sscanf(pIpAddr, "%d.%d.%d.%d", &byte3, &byte2, &byte1, &byte0) == 4)
   {
      if (((byte3 >= 0) && (byte3 < 256)) && ((byte2 >= 0) && (byte2 < 256)) &&
         ((byte1 >= 0) && (byte1 < 256)) && ((byte0 >= 0) && (byte0 < 256)))
      {
         ipAddr = (byte3 << 24) + (byte2 << 16) + (byte1 << 8) + (byte0);
      }
      else
         return 0;
   }
   else
      return 0;

   pCard->gwIpaddr = htonl(ipAddr);

   return 1;
}

/*******************************************************************************
NAME:       GetEthAdrFromArpCache
ABSTRACT:   Try to get ethernet address (MAC address) for a host, given the IP addr
RETURNS:    1 on success, 0 otherwise
*/
static UINT16 GetEthAdrFromArpCache(
   ETH_CARD *pCard,     /* Pointer to used Ethernet card */
   SIM_IF_INFO *pSimDev,   /* Pointer to simulated device */
   UINT32 reqIpAddr,    /* Requested ip address */ 
   UINT8 *pReqEthAddr)     /* Requested ethernet address */
{
   SIM_IF_INFO ReqItem;    /* Contains ip address, when sending ARP telegram */
   ARP_INFO *ptrToItem;    /* Pointer to item in ARP cache */
   int res = 0;            /* Return code   */
   int i;                  /* Loop variable */

   /*   Multicast addresses ? */
   if ((reqIpAddr & 0x000000E0) == 0x000000E0)
   {
      /* Yes. Create multicast ethernet address */
      pReqEthAddr[0] = 0x01;
      pReqEthAddr[1] = 0x00;

      /* Copy IP address to the last 4 bytes of the EthAddress */
      memcpy(&pReqEthAddr[2], &reqIpAddr, sizeof(UINT32));

      /* Over write byte 3 in EthAddress */
      pReqEthAddr[2] = 0x5E;

      /* Clear the high bit in 4:th byte */
      pReqEthAddr[3] &= ~0x80;

      return 1;
   }

   for (i = 0; i < 3; i++)
   {
      /* Try to get item from ARP cache */
      ptrToItem = FindEthernetAddressInArpCache(reqIpAddr);

      /* Found ? */
      if (ptrToItem != NULL)
      {
         /* Yes. Copy Ethernet address */
         memcpy(pReqEthAddr, &ptrToItem->ethaddr, ETH_ALEN);   
         res = 1;
         break;
      }

      /* Not found in cache. Try sending an ARP request */
      ReqItem.ipaddr = reqIpAddr;
      memcpy(ReqItem.ethaddr, gEmptyMAC, ETH_ALEN);   

      /* Create ARP request telegram */
      SendARPTgm(pCard, 1, gBroadcastMAC, pSimDev, &ReqItem);

      /* Wait for ARP reply */
      Sleep (500);
   }

    /* Still no match ? */
    if (res == 0)
    {
      /* Try default gateway instead */
       for (i = 0; i < 3; i++)
        {
           /* Try to get item from ARP cache */
           ptrToItem = FindEthernetAddressInArpCache(pCard->gwIpaddr);

           /* Found ? */
           if (ptrToItem != NULL)
           {
              /* Yes. Copy Ethernet address */
              memcpy(pReqEthAddr, &ptrToItem->ethaddr, ETH_ALEN); 
              res = 1;
              break;
           }

           /* Not found in cache. Try sending an ARP request */
           ReqItem.ipaddr = pCard->gwIpaddr;
           memcpy(ReqItem.ethaddr, gEmptyMAC, ETH_ALEN); 

           /* Create ARP request telegram */
           SendARPTgm(pCard, 1, gBroadcastMAC, pSimDev, &ReqItem);

           /* Wait for ARP reply */
           Sleep (500);
        }
    }

   return res;
}

/*******************************************************************************
NAME:       CalcIPHeaderChecksum
ABSTRACT:   Calculate the checksum for an IP header
RETURNS:    Checksum
*/
static UINT16 CalcIPHeaderChecksum(
   unsigned short *pAddr,   /* Pointer to address */
   int count)           /* Number of addresses */
{
   long sum = 0;  /* Checksum */

   /* Add all bytes in header (2 byte based) */
   while( count > 1 )  
   {
      sum += *pAddr++;
      count -= 2;
   }

   /*  Add left-over byte, if any */
   if( count > 0 )
      sum += *pAddr;

   /*  Fold 32-bit sum to 16 bits */
   while (sum>>16)
      sum = (sum & 0xffff) + (sum >> 16);

   /* Invert Checksum */
   sum = ~sum;

   return (UINT16) sum;
}

/*******************************************************************************
NAME:       AddUDPHeaderChecksum
ABSTRACT:   Add the checksum to the UDP header
RETURNS:    -
*/
static void AddUDPHeaderChecksum(
   BYTE *IPHeader,        /* Pointer to IP Header */
   BYTE *pUDPHeader,      /* Pointer to UDP Header */
   int UDPTotalLen)       /* UDP Total length */
{
   BYTE PseudoHeader[12];      /* Pseudo header - buffer to calculate checksum on */
   long sum = 0;            /* Check sum */
   unsigned short *pPseudoHdr;  /* Pointer to pseudo header buffer */
   int HdrSize;             /* Number of bytes in pseudo header */
   unsigned short *pAddr;      /* Pointer to UDP package data */

   /* To calculate correct checksum, we first create a pseudo header */
   pPseudoHdr = (unsigned short *)PseudoHeader;
   HdrSize = 12;

   /* Source IP Header */
   PseudoHeader[0] = IPHeader[12];
   PseudoHeader[1] = IPHeader[13];
   PseudoHeader[2] = IPHeader[14];
   PseudoHeader[3] = IPHeader[15];

   /* Destination IP Header */
   PseudoHeader[4] = IPHeader[16];
   PseudoHeader[5] = IPHeader[17];
   PseudoHeader[6] = IPHeader[18];
   PseudoHeader[7] = IPHeader[19];

   /* Zero */
   PseudoHeader[8] = 0;

   /* 8 bit protocol */
   PseudoHeader[9] = IPHeader[9];

   /* Length */
   PseudoHeader[10] = pUDPHeader[4];
   PseudoHeader[11] = pUDPHeader[5];

   /* Count checksum for UDP pseudo header */
   while( HdrSize > 1 )  
   {
      sum += *pPseudoHdr++;
      HdrSize -= 2;
   }

   pAddr = (unsigned short *)pUDPHeader;/*lint !e826 Checksum calculation is based on 16 bit arithmetics */   

   /* Count checksum for UDP package */
   while( UDPTotalLen > 1 )  
   {
      sum += *pAddr++;
      UDPTotalLen -= 2;
   }

   /*  Pad with additional zero byte if nessescary */
   if( UDPTotalLen > 0 )
   {
      sum += (*pAddr & 0x00FF);
   }

   /*  Fold 32-bit sum to 16 bits */
   while (sum>>16)
      sum = (sum & 0xffff) + (sum >> 16);

   sum = ~sum;

   pUDPHeader[6] = ((UINT16) sum) % 256;
   pUDPHeader[7] = ((UINT16) sum) / 256;
}

/*******************************************************************************
NAME:       AddEthernetHeader
ABSTRACT:   Adds Ethernet Layer 3 to packet 
RETURNS:    Start position for IP (Layer 2)
*/
static BYTE *AddEthernetHeader(
   BYTE *pPacket,       /* Pointer to packet buffer */
   char *ethDestination,   /* Destination Ethernet address */
   char *ethSource,        /* Source Ethernet address */
   UINT16 type)            /* Telegram type Ipv4 */
{
   /* Sanity check */
   if ((pPacket == NULL) || (ethSource == NULL) || (ethDestination == NULL))
      return pPacket;

   /* Copy destination MAC address */
   memcpy(pPacket, ethDestination, ETH_ALEN);
   pPacket += ETH_ALEN;

   /* Copy source MAC address */
   memcpy(pPacket, ethSource, ETH_ALEN);
   pPacket += ETH_ALEN;

   /* Add type field */
   *pPacket++ = type / 256;
   *pPacket++ = type % 256;

   return pPacket;
}

/*******************************************************************************
NAME:       AddARPTgm
ABSTRACT:   Add ARP request/reply to buffer
RETURNS:    Pointer to ARP tgm start
*/
static BYTE *AddARPTgm(
   BYTE *ThePacket,       /* Pointer to packet buffer */
   BYTE Op,            /* Op (ARP req = 1, ARP reply = 2, RARP req = 3, RARP reply = 4) */
   SIM_IF_INFO *pSource,  /* Source info */
   SIM_IF_INFO *pTarget)  /* Target info */
{  
   /* Hard type */
   *ThePacket++ = 0x00;
   *ThePacket++ = 0x01;

   /* Prot type */
   *ThePacket++ = 0x08;
   *ThePacket++ = 0x00;

   /* Hard size */
   *ThePacket++ = 0x06;

   /* Prot size */
   *ThePacket++ = 0x04;

   /* Op (ARP req = 1, ARP reply = 2, RARP req = 3, RARP reply = 4)*/
   *ThePacket++ = Op / 256;
   *ThePacket++ = Op % 256;

   /* Source MAC address */
   memcpy(ThePacket, pSource->ethaddr, ETH_ALEN);
   ThePacket += ETH_ALEN;

   /* Source IP address */
   memcpy(ThePacket, &pSource->ipaddr, 4);
   ThePacket = ThePacket + 4;

   /* Target MAC address */
   memcpy(ThePacket, pTarget->ethaddr, ETH_ALEN);
   ThePacket += ETH_ALEN;

   /* Target IP address */
   memcpy(ThePacket, &pTarget->ipaddr, 4);
   ThePacket = ThePacket + 4;

   /* 18 byte pad to fill minimum length of 802.3 protocol */
   memset(ThePacket, 0, 18);
   ThePacket = ThePacket + 18;

   return ThePacket;
}

/*******************************************************************************
NAME:       SendARPTgm
ABSTRACT:   Allocate memory, create ARP message and send it on the Ethernet
RETURNS:    IPT_OK on success, IPT_ERROR otherwise
*/
static int SendARPTgm(
   ETH_CARD *pCard,       /* Reference to ethernet card used for this telegram */
   BYTE Op,               /* Op (ARP req = 1, ARP reply = 2, RARP req = 3, RARP reply = 4) */
   char *destMAC,         /* Destination MAC address */
   SIM_IF_INFO *pSource,  /* Source info */
   SIM_IF_INFO *pTarget)  /* Target info */
{
   BYTE *pEthFrame;           /* The Ethernet frame  */
   BYTE *pPacket;             /* Temporary pointer used when building telegram */
   UINT16 TheType;               /* Type of data in Ethernet frame */
   int SizeOfEthHeader = 14;     /* Size of Ethernet header */
   int SizeOfARPTgm = 28;        /* Size of ARP telegram */
   int SizeOfARPTrailer = 18;    /* Size of ARP trailer */
   int SizeOfTelegram;           /* Size of entire telegram */

   SizeOfTelegram = 0;

   /* Allocate data for the message */
   pEthFrame = (BYTE *)malloc(SizeOfEthHeader + SizeOfARPTgm + SizeOfARPTrailer);
   if (pEthFrame == NULL)
      return IPT_ERROR;

   pPacket = pEthFrame;

   /* ARP */
   TheType = 0x0806;

   /* Add ethernet header to the buffer (14 bytes)*/
   pPacket = AddEthernetHeader(pPacket, destMAC, pSource->ethaddr, TheType);

   /* Add ARP message to the buffer (28 bytes)*/
   pPacket = AddARPTgm(pPacket, Op, pSource, pTarget);

   SizeOfTelegram = (SizeOfEthHeader + SizeOfARPTgm + SizeOfARPTrailer);

   /* Is PCAP initialized ? */
   if (pCard->pcap_fp != NULL)
   {
      /* Send ARP reply on Ethernet */
      if (pcap_sendpacket(pCard->pcap_fp, pEthFrame, SizeOfTelegram) != 0)
      {
         printf("\nError sending ARP telegram:  %s\n", pcap_geterr(pCard->pcap_fp));
      }
   }

   /* Return memory */
   if (pEthFrame != NULL)
      free(pEthFrame);

   return IPT_OK;
}

/*******************************************************************************
NAME:       AddIPHeader
ABSTRACT:   Add the IP header to the buffer
RETURNS:    Pointer to UDP packet start
*/
static BYTE *AddIPHeader(
   BYTE *ThePacket,          /* Pointer to packet buffer */
   UINT16 IPTotalLen,        /* Total length of IP */
   BYTE Protocol,            /* UDP etc... */
   UINT32 srcIPAddr,         /* Source address (spoofed) */
   UINT32 dstIPAddr,         /* Destination address */
   int RouterAlert,          /* 1 = use Router alert option */
   UINT16 FragOffset,        /* Flags plus fragment offset */
   UINT16 Id)                /* Identification */
{
   BYTE *pHdrStart;     /* Pointer to start of header */
   BYTE *pHdrChecksum;     /* Pointer to header checksum field */
   UINT16 ChkSum;       /* Checksum container */

   pHdrStart = ThePacket;

   /* 4 bit version + 4 bit header length */
   if (RouterAlert)
      *ThePacket++ = 0x40 | 0x06;   /* 0x40 = IP version 4, 0x05 = No of 32 bit words in the IP header */
   else
      *ThePacket++ = 0x40 | 0x05;   /* 0x40 = IP version 4, 0x05 = No of 32 bit words in the IP header */

   /* Type of service */
   *ThePacket++ = 0x00; /* binary mask abcdefgh - a,b,c ignored c minimize delay, d maximize throughput */
   /* e maximize reliability, f mimize monetary cost                               */

   /* Ethereal output Diferentiated service field : 0x00 (DSCP 0x00: Default; ECN : 0x00)
   Differentiated Services Code Point  .... ..0. = ECN-Capable Transport (ECT)
   .... ...0 = ECN-CE: 0
   */

   /* Total length of the IP frame in bytes */
   *ThePacket++ = IPTotalLen / 256;
   *ThePacket++ = IPTotalLen % 256;

   /* Identification. Normally increments by one for each send */
   *ThePacket++ = Id / 256;
   *ThePacket++ = Id % 256;

   /* NOTE Side effect: Increments global identity */
   gID++;
   if (gID == 0)
      gID = 1;

   /* Flags (3 bit) + 13 bit fragment offset */
   *ThePacket++ = FragOffset / 256;
   *ThePacket++ = FragOffset % 256;

   /* Time to live (TTL) */
   *ThePacket++ = 0x80;    /* Default 128 */

   /* Protocol */
   *ThePacket++ = Protocol;      

   /* Header checksum */

   /* Save a reference for later use */
   pHdrChecksum = ThePacket;
   *ThePacket++ = 0x00;
   *ThePacket++ = 0x00;

   /* Source IP address */
   memcpy(ThePacket, &srcIPAddr, 4);
   ThePacket = ThePacket + 4;

   /* Destination IP address */
   memcpy(ThePacket, &dstIPAddr, 4);
   ThePacket = ThePacket + 4;

   /* Add router alert (rfc 2113) if specified */
   if (RouterAlert)
   {
      *ThePacket++ = 0x94;
      *ThePacket++ = 0x04;
      *ThePacket++ = 0x00;
      *ThePacket++ = 0x00;
      ChkSum = CalcIPHeaderChecksum((unsigned short *)pHdrStart, 24);/*lint !e826 Checksum calculation is based on 16 bit arithmetics */   
   }
   else
      ChkSum = CalcIPHeaderChecksum((unsigned short *)pHdrStart, 20);/*lint !e826 Checksum calculation is based on 16 bit arithmetics */   

   *pHdrChecksum++ = ChkSum % 256;
   *pHdrChecksum++ = ChkSum / 256;

   return ThePacket;
}

/*******************************************************************************
NAME:       SendICMPTgm
ABSTRACT:   Allocate memory and create ICMP telegram
RETURNS:    -
*/
static int SendICMPTgm(
   ETH_CARD *pCard,     /* Reference to ethernet card used for this telegram */
   BYTE ICMPType,       /* ICMP type */
   BYTE ICMPCode,       /* ICMP code */
   UINT32 srcIPAddr,    /* IP layer source address (spoofed) */
   UINT32 dstIPAddr,    /* IP layer destination address */
   BYTE *pData)         /* Additional data */
{
   SIM_IF_INFO *pSimDev;      /* Pointer to simulated device */
   BYTE *pEthFrame;        /* The Ethernet frame  */
   BYTE *pPacket;          /* Temporary pointer used when building telegram */
   BYTE *IPHeader;            /* Pointer to start of IP Header */
   BYTE *ICMPHeader;       /* Pointer to start of IP Header */
   UINT16 TheType;            /* Type of data in Ethernet frame */
   UINT16 IPMsgLen;        /* Length of IP frame */
   UINT16 ICMPChkSum;         /* ICMP check sum */
   BYTE DestMAC[ETH_ALEN];    /* MAC Address of destination interface */
   int SizeOfEthHeader;    /* Size of Ethernet header */
   int SizeOfIPHeader;        /* Size of IP header */
   int SizeOfICMPTgm;         /* Size of ICMP tgm */
   int SizeOfEthTrailer;      /* Size of Ethernet trailer */
   int SizeOfTelegram;        /* Size of entire telegram*/  
   int i;                  /* Loop variable */

   SizeOfEthHeader = 14;
   SizeOfIPHeader = 20;
   if (ICMPType == 0)
      SizeOfICMPTgm = 40;
   else
      SizeOfICMPTgm = 400;

   SizeOfEthTrailer = 4;

   /* Get reference to simulated interface */
   pSimDev = FindSimulatedInterface(pCard, srcIPAddr);
   if (pSimDev == NULL)
      return IPT_ERROR; 

   /* Try to get Ethernet address of destination */
   if (!GetEthAdrFromArpCache(pCard, pSimDev, dstIPAddr, DestMAC))
      return IPT_ERROR; 

   /* Allocate data for the message */
   pEthFrame = (BYTE *)malloc(SizeOfEthHeader + SizeOfIPHeader + SizeOfICMPTgm + SizeOfEthTrailer);
   if (pEthFrame == NULL)
      return IPT_ERROR; 

   pPacket = pEthFrame;

   /* IP data */
   TheType = 0x0800;

   /* Add ethernet header to the buffer (14 bytes)*/
   pPacket = AddEthernetHeader(pPacket, DestMAC, pSimDev->ethaddr, TheType);

   /* Calculate size of IP datagram */
   IPMsgLen = SizeOfIPHeader + SizeOfICMPTgm;
   IPHeader = pPacket;

   /* Add IP header to the buffer (20 bytes)*/
   pPacket = AddIPHeader(pPacket, IPMsgLen, 0x01, srcIPAddr, dstIPAddr, 0, 0, gID);

   ICMPHeader = pPacket;

   /* Add ICMP Type */
   ICMPHeader[0] = ICMPType;

   /* Add ICMP Code */  
   ICMPHeader[1] = ICMPCode;
   ICMPHeader[2] = 0;
   ICMPHeader[3] = 0;

   switch(ICMPType)
   {
   case 0:
      /* Identifier */
      ICMPHeader[4] = pData[0];
      ICMPHeader[5] = pData[1];

      /* SeqNo */
      ICMPHeader[6] = pData[2];
      ICMPHeader[7] = pData[3];

      pPacket = &ICMPHeader[8];
      
      /* Pad packet witch ASCII data */
      for (i = 0; i < 32; i++)
      {
         pPacket[i] = 0x61 + (i % 23);
      }

      SizeOfTelegram = (int)(&pPacket[i] - pEthFrame);
      SizeOfICMPTgm = (int)(&pPacket[i] - &ICMPHeader[0]);
      break;
   case 3:
      break;
   case 4:
      break;
   case 5:
      break;
   case 8:
      SizeOfICMPTgm = 8;

      /* Identifier */
      ICMPHeader[4] = 0;
      ICMPHeader[5] = 0;

      /* SeqNo */
      ICMPHeader[6] = gPingSeqNo / 256;
      ICMPHeader[7] = gPingSeqNo % 256;

      gPingSeqNo++;
      break;
   case 9:
      break;
   case 10:
      break;
   case 11:
      break;
   case 12:
      break;
   case 13:
      break;
   case 14:
      break;
   case 15:
      break;
   case 16:
      break;
   case 17:
      break;
   case 18:
      break;
   default:
      break;
   }

   /* Add ICMP checksum */

   ICMPChkSum = CalcIPHeaderChecksum((unsigned short *)ICMPHeader, SizeOfICMPTgm);/*lint !e826 Checksum calculation is based on 16 bit arithmetics */ 

   ICMPHeader[2] = ICMPChkSum % 256;
   ICMPHeader[3] = ICMPChkSum / 256;

   /* Is PCAP initialized ? */
   if ((pCard->pcap_fp != NULL) && (pEthFrame != NULL))
   {
      /* Send ARP reply on Ethernet */
      if (pcap_sendpacket(pCard->pcap_fp, pEthFrame, SizeOfTelegram) != 0)
      {
         printf("\nError sending ICMP telegram:  %s\n", pcap_geterr(pCard->pcap_fp));
      }
   }

   /* Return memory */
   if (pEthFrame != NULL)
      free(pEthFrame);

   return IPT_OK;
}

/*******************************************************************************
NAME:       SendIGMPTgm
ABSTRACT:   Allocate memory, create IGMP telegram and send it on Ethernet
RETURNS:    -
*/
static void SendIGMPTgm(
   ETH_CARD *pCard,           /* Reference to ethernet card used for this telegram */
   BYTE TypeOfTgm,            /* Which IGMP telegram that should be sent */
   BYTE IGMPVersion,       /* Version of IGMP */
   UINT32 srcAddr,            /* IP layer source address (spoofed) */
   UINT32 destAddr)            /* IP layer destination address (multicast) */
{
   SIM_IF_INFO *pSimDev;      /* The simulated device */
   BYTE *pEthFrame;        /* The Ethernet frame  */
   BYTE *pPacket;          /* Temporary pointer used when building telegram */
   BYTE *IPHeader;            /* Pointer to start of IP Header */
   BYTE *IGMPHeader;       /* Pointer to start of IP Header */
   BYTE IGMPType = 0;         /* IGMP type */
   UINT16 TheType;            /* Type of data in Ethernet frame */
   UINT16 IPMsgLen;        /* Length of IP frame */
   UINT16 IGMPChkSum;         /* ICMP check sum */
   BYTE DestMAC[ETH_ALEN];    /* MAC Address of destination interface */
   int SizeOfEthHeader;    /* Size of Ethernet header */
   int SizeOfIPHeader;        /* Size of IP header */
   int SizeOfIGMPTgm;         /* Size of ICMP tgm */
   int SizeOfEthTrailer;      /* Size of Ethernet trailer */
   int SizeOfTelegram;        /* Size of IGMP telegram */   

   SizeOfEthHeader = 14;
   SizeOfIPHeader = 24;    /* Normal size is 20, but router alert option is added */

   /* Which version of IGMP ? */
   switch (IGMPVersion)
   {
   case 1:
      /* IGMP Version 1 */
      
      /* Leave is not part of standard */
      if (TypeOfTgm == SIM_LEAVE_MC_GROUP)
         return;  

      if (TypeOfTgm == SIM_JOIN_MC_GROUP)
         IGMPType = 0x11;

      if (TypeOfTgm == SIM_MC_V1_MEMBER_REPORT)
         IGMPType = 0x12;     

      SizeOfIGMPTgm = 8;
      break;

   case 2:
      /* IGMP Version 2 */
      if (TypeOfTgm == SIM_JOIN_MC_GROUP)
         IGMPType = 0x16;

      if (TypeOfTgm == SIM_LEAVE_MC_GROUP)
         IGMPType = 0x17;

      SizeOfIGMPTgm = 8;
      break;

   case 3:
      /* IGMP Version 3 */
      if (TypeOfTgm == SIM_MC_V3_MEMBER_REPORT)
         IGMPType = 0x22;

      if (TypeOfTgm == SIM_JOIN_MC_GROUP)
         IGMPType = 0x22;

      if (TypeOfTgm == SIM_LEAVE_MC_GROUP)
         IGMPType = 0x22;

      SizeOfIGMPTgm = 20;
      break;
   default:
      return;  
   }

   if (IGMPType == 0)
      return;  

   SizeOfEthTrailer = 4;

   /* Get reference to simulated interface */
   pSimDev = FindSimulatedInterface(pCard, srcAddr);
   if (pSimDev == NULL)
      return;  

   /* Try to get Ethernet address of destination */
   if (!GetEthAdrFromArpCache(pCard, pSimDev, destAddr, DestMAC))
      return;  

   /* Allocate data for the message */
   pEthFrame = (BYTE *)malloc(SizeOfEthHeader + SizeOfIPHeader + SizeOfIGMPTgm + SizeOfEthTrailer);
   if (pEthFrame == NULL)
      return;  

   pPacket = pEthFrame;

   /* IP data */
   TheType = 0x0800;

   /* Add ethernet header to the buffer (14 bytes)*/
   pPacket = AddEthernetHeader(pPacket, DestMAC, pSimDev->ethaddr, TheType);

   /* Calculate size of IP datagram */
   IPMsgLen = SizeOfIPHeader + SizeOfIGMPTgm;
   IPHeader = pPacket;

   /* Add IP header to the buffer (20 bytes)*/
   pPacket = AddIPHeader(pPacket, IPMsgLen, 0x02, srcAddr, destAddr, 1, 0, gID);

   IGMPHeader = pPacket;

   switch (IGMPVersion)
   {
   case 1:
      SizeOfTelegram = (int)(&IGMPHeader[8] - pEthFrame);
      break;
   case 2:
      IGMPHeader[0] = IGMPType;
      IGMPHeader[1] = 0;
      IGMPHeader[2] = 0;
      IGMPHeader[3] = 0;
      memcpy(&IGMPHeader[4], &destAddr, 4);
      SizeOfTelegram = (int)(&IGMPHeader[8] - pEthFrame);
      break;
   case 3:
      IGMPHeader[0] = IGMPType;
      IGMPHeader[1] = 0;
      IGMPHeader[2] = 0;
      IGMPHeader[3] = 0;

      IGMPHeader[4] = 0;
      IGMPHeader[5] = 0;
      IGMPHeader[6] = 0;   /* One group record */
      IGMPHeader[7] = 1;

      // ---------------------- MEMBERSHIP REPORT OXÅ !!!!!!!!!!!!!!!!!!!!!!!!!--------------------

      if (TypeOfTgm == SIM_JOIN_MC_GROUP)
         IGMPHeader[8] = 1;

      if (TypeOfTgm == SIM_LEAVE_MC_GROUP)
         IGMPHeader[8] = 2;

      // ---------------------- MEMBERSHIP REPORT OXÅ !!!!!!!!!!!!!!!!!!!!!!!!!--------------------
      IGMPHeader[9] = 0;
      IGMPHeader[10] = 0;  /* One address specified */
      IGMPHeader[11] = 1;
      memcpy(&IGMPHeader[12], &destAddr, 4);
      memcpy(&IGMPHeader[16], &srcAddr, 4);
      SizeOfTelegram = (int)(&IGMPHeader[20] - pEthFrame);
      break;
   default:
      break;
   }

   /* Add IGMP checksum */ 
   IGMPChkSum = CalcIPHeaderChecksum((unsigned short *)IGMPHeader, SizeOfIGMPTgm);/*lint !e826 Checksum calculation is based on 16 bit arithmetics */ 

   IGMPHeader[2] = IGMPChkSum % 256;
   IGMPHeader[3] = IGMPChkSum / 256;

   /* Is PCAP initialized ? */
   if ((pCard->pcap_fp != NULL) && (pEthFrame != NULL))
   {
      /* Send ARP reply on Ethernet */
      if (pcap_sendpacket(pCard->pcap_fp, pEthFrame, SizeOfTelegram) != 0)
      {
         printf("\nError sending IGMP telegram: %d\n", pcap_geterr(pCard->pcap_fp));
      }
   }

   /* Return memory */
   if (pEthFrame != NULL)
      free(pEthFrame);
}

/*******************************************************************************
NAME:       AddUDPHeader
ABSTRACT:   Add the UDP header to the buffer
RETURNS:    Pointer that indicates how far buffer has been written
*/
static BYTE *AddUDPHeader(
   BYTE *ThePacket,        /* Packet buffer */
   UINT16 UDPFrameLen,     /* UDP Message Length */
   UINT16 srcPortNo,       /* Source address (spoofed), only port used*/
   UINT16 dstPortNo)       /* Destination address, only port used */
{
   /* Source port, should already be in network byte order */
   memcpy(ThePacket, &srcPortNo, sizeof(UINT16));
   ThePacket += sizeof(UINT16);

   /* Destination port, should already be in network byte order */
   memcpy(ThePacket, &dstPortNo, sizeof(UINT16));
   ThePacket += sizeof(UINT16);

   /* Length */
   *ThePacket++ = UDPFrameLen / 256;
   *ThePacket++ = UDPFrameLen % 256;

   /* Checksum */
   *ThePacket++ = 0;
   *ThePacket++ = 0;

   return ThePacket;
}

/*******************************************************************************
NAME:       SendUDPTgm
ABSTRACT:   Allocate memory and create Ethernet frame
RETURNS:    Pointer to Ethernet frame or NULL
*/
static int SendUDPTgm(
   ETH_CARD *pCard,      /* Reference to ethernet card used for this telegram */
   BYTE *pMsg,           /* Message data, UDP payload data */
   int MsgLen,           /* Message length */
   UINT32 srcIPAddr,     /* IP layer source address (spoofed) */
   UINT32 dstIPAddr,     /* IP layer destination address */
   UINT16 srcPortNo,     /* UDP source port */
   UINT16 dstPortNo)     /* UDP destination port */
{
   SIM_IF_INFO *pSimDev;      /* Pointer to simulated device */
   BYTE *pEthFrame;        /* The Ethernet frame  */
   BYTE *pPacket;          /* Temporary pointer used when building telegram */
   BYTE *IPHeader;            /* Pointer to start of IP Header */
   BYTE *UDPHeader;        /* Pointer to start of UDP Header */
   UINT16 TheType;            /* Type of data in Ethernet frame */
   UINT16 IPMsgLen;        /* Length of IP frame */
   UINT16 UDPMsgLen;       /* Length of UDP frame */
   BYTE DestMAC[ETH_ALEN];    /* MAC Address of destination interface */
   int SizeOfIPHeader;        /* Size of IP header */
   int SizeOfEthHeader;    /* Size of Ethernet header */
   int SizeOfEthTrailer;      /* Size of Ethernet trailer */
   int SizeOfUDPHeader;    /* Size of UDP header */
   int res = IPT_OK;          /* Return code */ 
   int SizeOfMsg;          /* Length of the original UDP telegram */
   
   int FrameLen;           /* Length of a fragmented ethernet frame */
   int BytesSent;          /* No of bytes sent when fragmenting data */
   int BytesToSend;        /* Size of a fragmented IP frame */
   BYTE *pSndBuf;          /* A fragmented IP frame  */
   BYTE *pFrPacket;        /* Temporary pointer used when building IP fragments */
   UINT16 FragOffset;      /* Fragment offset and flags */
   UINT16 FragID;          /* Fragment identity */

   SizeOfEthHeader = 14;
   SizeOfEthTrailer = 4;
   SizeOfIPHeader = 20;
   SizeOfUDPHeader = 8;

   /* Get reference to simulated interface */
   pSimDev = FindSimulatedInterface(pCard, srcIPAddr);
   if (pSimDev == NULL)
      return IPT_ERROR; 

   /* Try to get Ethernet address of destination */
   if (!GetEthAdrFromArpCache(pCard, pSimDev, dstIPAddr, DestMAC))
   {
      /* Could not find Ethernet address */
      return IPT_ERROR; 
   }

   /* Allocate data for the message */
   pEthFrame = (BYTE *)malloc(SizeOfEthHeader + SizeOfIPHeader + SizeOfUDPHeader + MsgLen + SizeOfEthTrailer);
   if (pEthFrame == NULL)
      return IPT_ERROR; 

   pPacket = pEthFrame;

   /* IP data */
   TheType = 0x0800;

   /* Add ethernet header to the buffer (14 bytes)*/
   pPacket = AddEthernetHeader(pPacket, DestMAC, pSimDev->ethaddr, TheType);

   /* Calculate size of IP datagram */
   IPMsgLen = SizeOfIPHeader + SizeOfUDPHeader + MsgLen;
   IPHeader = pPacket;

   /* Add IP header to the buffer (20 bytes)*/
   pPacket = AddIPHeader(pPacket, IPMsgLen, 0x11, srcIPAddr, dstIPAddr, 0, 0, gID);

   /* Calculate size of UDP datagram */
   UDPMsgLen = SizeOfUDPHeader + MsgLen;
   UDPHeader = pPacket;

   /* Add UDP header to the buffer */
   pPacket = AddUDPHeader(pPacket, UDPMsgLen, srcPortNo, dstPortNo);

   /* Add data */
   memcpy(pPacket, pMsg, MsgLen);   

   SizeOfMsg = (int)(pPacket - pEthFrame) + MsgLen;

   /* Add UDP checksum */
   AddUDPHeaderChecksum(IPHeader, UDPHeader, UDPMsgLen);

   /********************** At this point the telegram is ready for sending *******************/
   
   /* Does it fit into one Ethernet frame ? */
   if (SizeOfMsg <= (SizeOfEthHeader + 1500))
   {
      /* Sanity check */
      if ((pEthFrame != NULL) && (pCard->pcap_fp != NULL))
      {
         /* Send frame on Ethernet */
         if (pcap_sendpacket(pCard->pcap_fp, pEthFrame, SizeOfMsg) != 0)
         {
            printf("\nError sending the packet: %d\n", pcap_geterr(pCard->pcap_fp));
            res = IPT_ERROR;
         }
      }
   }
   else
   {
      /* No. Message is to large. Unfortunately it needs to be fragmented :-(  */
      
      /* Get reference to UDP message */
      BytesSent = (SizeOfEthHeader + SizeOfIPHeader);
      pPacket = pEthFrame + BytesSent;
      FragOffset = 0;
      FragID = gID + 20000;
      if (FragID == 0)
         FragID =1;

      /* As long as there are any UDP data left to send */
      while (BytesSent < SizeOfMsg)
      {
         BytesToSend = SizeOfMsg - BytesSent;
         if (BytesToSend > 1480)
            BytesToSend = 1480;

         /* Allocate memory */
         pSndBuf = malloc(SizeOfEthHeader + 1500 + SizeOfEthTrailer);
         if (pSndBuf == NULL)
         {
            /* Return memory */
            if (pEthFrame != NULL)
               free(pEthFrame);

            return IPT_ERROR;
         }

         pFrPacket = pSndBuf;

         /* Add ethernet header to the buffer (14 bytes)*/
         pFrPacket = AddEthernetHeader(pFrPacket, DestMAC, pSimDev->ethaddr, TheType);

         /* Add IP header to the buffer (20 bytes)*/
         IPMsgLen = SizeOfIPHeader + BytesToSend;
         
         /* Signal if there are more fragments */
         if ((SizeOfMsg - BytesSent - BytesToSend) > 0)
            FragOffset = FragOffset | 0x2000;

         pFrPacket = AddIPHeader(pFrPacket, IPMsgLen, 0x11, srcIPAddr, dstIPAddr, 0, FragOffset, FragID);

         /* Remove more fragments flag */
         FragOffset &= ~0x2000;

         /* Copy payload */
         memcpy(pFrPacket, pPacket, BytesToSend);

         pPacket = pPacket + BytesToSend;

         /* Sanity check */
         if ((pSndBuf != NULL) && (pCard->pcap_fp != NULL))
         {
            /* Send frame on Ethernet */
            FrameLen = (int)(pFrPacket - pSndBuf) + BytesToSend;
            
            /* Less than minimum Ethernet frame? */
            if (FrameLen < SIM_MIN_ETH_FRAME + sizeof(ETH_HDR))
            {
              memset(&pSndBuf[FrameLen], 0, SIM_MIN_ETH_FRAME + sizeof(ETH_HDR) - FrameLen);
              FrameLen =  SIM_MIN_ETH_FRAME + sizeof(ETH_HDR);
            }
            if (pcap_sendpacket(pCard->pcap_fp, pSndBuf, FrameLen) != 0)
            {
               printf("\nError sending the packet: %d\n", pcap_geterr(pCard->pcap_fp));
               res = IPT_ERROR;
            }
         }

         /* Return memory */
         if (pSndBuf != NULL)
            free(pSndBuf);

         FragOffset += BytesToSend / 8;
         BytesSent += BytesToSend;
      }
   }

   /* Return memory */
   if (pEthFrame != NULL)
      free(pEthFrame);

   return res;
}

/*******************************************************************************
NAME:       IPTGetLocalMACAddr
ABSTRACT:   Returns MAC address of interface with associated ipAddr
RETURNS:    IPT_OK if succeded, !=0 if error
*/
int IPTGetLocalMACAddr(
   UINT32 ipaddr,         /* Ip address of interface*/
   UINT8  *pEthaddr)      /* Ethernet address  */
{
   IP_ADAPTER_INFO adInfo[16];         /* Holds info about network adapters */
   DWORD dwBufLen = sizeof(adInfo); /* Size of adapter omfo array */
   PIP_ADAPTER_INFO pAdapterInfo;      /* Pointer to specific adapter info */
   IP_ADDR_STRING *ifIPAddr;        /* Pointer to interface address */

   /* Get adapter information */
   if (GetAdaptersInfo(adInfo, &dwBufLen) == ERROR_SUCCESS)
   {
      pAdapterInfo = adInfo;
      
      do
      {
         /* Get first item in the IP address list */
         ifIPAddr = &(pAdapterInfo->IpAddressList);

         /* For all addresses in the list */
         while (ifIPAddr != NULL)
         {
            /* Is it this one ? */
            if (ipaddr == inet_addr(ifIPAddr->IpAddress.String))
            {
               /* Yes. Copy ethernet address */
               memcpy(pEthaddr, pAdapterInfo->Address, ETH_ALEN);
               return (int)IPT_OK;
            }
            
            /* Get next address */
            ifIPAddr = ifIPAddr->Next;
         }

         /* Get next adapter */
         pAdapterInfo = pAdapterInfo->Next; 

      } while (pAdapterInfo);
   }

   /* Failed to get MAC address. Invent one */     
   pEthaddr[0] = 0x00; 
   pEthaddr[1] = 0x1c; 
   pEthaddr[2] = 0x23;
   pEthaddr[3] = 0x17; 
   pEthaddr[4] = 0x18; 
   pEthaddr[5] = 0x19;


   return -1;
}

/*******************************************************************************
NAME:       GetIPAddressFromDescription
ABSTRACT:   Returns IP address list, given a cad description
RETURNS:    IPT_OK if succeded, !=0 if error
*/
static int GetIPAddressFromDescription(
   char *pCardDescription,   /* The card description */
   UINT32 *pIPAddrList,      /* The list of IP addresses */
   int *pSizeOfIPAddrList)   /* [IN, OUT]The size of the list of IP addresses */
{
   IP_ADAPTER_INFO adInfo[16];         /* Holds info about network adapters */
   DWORD dwBufLen = sizeof(adInfo); /* Size of adapter omfo array */
   PIP_ADAPTER_INFO pAdapterInfo;      /* Pointer to specific adapter info */
   IP_ADDR_STRING *ifIPAddr;        /* Pointer to interface address */
   int TheCardIsFound = 0;          /* 1 if Card is found */
   char ExtendedCardDescription[250];  /* Extended card description */
   int NoOfIPAddresses = 0;         /* No of IP addrsses for this card */

   sprintf_s(ExtendedCardDescription, 250, "%s - Packet Scheduler Miniport", pCardDescription);

   /* Get adapter information */
   if (GetAdaptersInfo(adInfo, &dwBufLen) == ERROR_SUCCESS)
   {
      pAdapterInfo = adInfo;
      
      while (pAdapterInfo)
      {
         /* Was it this one ? */
         TheCardIsFound = (strcmp(pCardDescription, pAdapterInfo->Description) == 0);
         
         /* Or maybe the extended one */
         if (!TheCardIsFound)
            TheCardIsFound = (strcmp(ExtendedCardDescription, pAdapterInfo->Description) == 0);

         /* Found the card ? */
         if (TheCardIsFound)
         {
            /* Get first item in the IP address list */
            ifIPAddr = &(pAdapterInfo->IpAddressList);

            /* For all addresses in the list */
            while ((ifIPAddr != NULL) && (NoOfIPAddresses < *pSizeOfIPAddrList))
            {
               /* Put the IP address in the list */
               *pIPAddrList = inet_addr(ifIPAddr->IpAddress.String);
               pIPAddrList++;
               
               NoOfIPAddresses++;
               
               /* Get next address */
               ifIPAddr = ifIPAddr->Next;
            }

            /* Return number of IP addresses found */
            *pSizeOfIPAddrList = NoOfIPAddresses;
            
            return 1;
         }

         /* Get next adapter */
         pAdapterInfo = pAdapterInfo->Next; 
      }
   }

   return 0;
}

/*******************************************************************************
* GLOBAL FUNCTIONS */

/*******************************************************************************
NAME:       OpenInterface
ABSTRACT:   Opens the interface with the IP-address passed
RETURNS:    (int)IPT_OK if succeded, !=0 if error
*/
static int OpenInterface(
   UINT32 IPAddr)             /* IP Address for interface to open (ie "10.160.7.20")*/
{
   char errbuf[PCAP_ERRBUF_SIZE] = {0};   /* Error string */
   BYTE EthAddr[ETH_ALEN];       /* Ethernet address of adapter */
   pcap_t *pcap_fp;           /* Handle to pcap */
   pcap_if_t *listOfAllDevices;  /* Pointer to list of adapters */
   pcap_if_t *testDev;           /* Pointer to adapter */
   pcap_addr_t *ifAddr;       /* Pointer to adapter address */

   /* Get the list of all ethernet adapters */
   if (pcap_findalldevs(&listOfAllDevices, errbuf) == -1)
   {
      printf("IPT-COM SIM: pcap_findalldevs_ex() failed for 0x%X: %s\n", IPAddr, errbuf);
      pcap_freealldevs(listOfAllDevices);
      return (int)IPT_SIM_ERR_PCAP_IFOPEN;
   }

   /* For all known adapters */
   for (testDev = listOfAllDevices; (testDev != NULL); testDev = testDev->next)
   {
      /* For all assigned addresses for this adapter */
      for (ifAddr = testDev->addresses; (ifAddr != NULL); ifAddr = ifAddr->next)
      {
         /* Is it this address ? */
         if ((ifAddr->addr->sa_family == AF_INET) && 
            (IPAddr == ((struct sockaddr_in *) ifAddr->addr)->sin_addr.s_addr))
         {
            /* Yes. The card is found. Get the MAC address */
            if (IPTGetLocalMACAddr(IPAddr, EthAddr) != (int)IPT_OK)
            {
               printf("IPT-COM SIM: Unable to retrieve local MAC address on 0x%X (using spoofed instead)\n", IPAddr);
            }

            /* Open pcap for this adapter */
            pcap_fp = pcap_open_live(testDev->name, 65536, PCAP_OPENFLAG_PROMISCUOUS, 5, errbuf);

            /* Success ? */
            if (pcap_fp != NULL)
            {
               /* Yes. Save card info in array */
               if (AddEthCard(IPAddr, EthAddr, testDev->description,  pcap_fp))
               {
                  /* Free device list */
                  pcap_freealldevs(listOfAllDevices);

                  return (int)IPT_OK;
               }
            }

            /* Free device list */
            pcap_freealldevs(listOfAllDevices);

            printf("IPT-COM SIM: OpenInterface failed for 0x%X: %s (0x%X)\n", IPAddr, errbuf, pcap_fp);

            return (int)IPT_SIM_ERR_PCAP_IFOPEN;
         }
      }
   }

   /* Free device list */
   pcap_freealldevs(listOfAllDevices);

   return (int)IPT_SIM_ERR_PCAP_IFOPEN;
}

/*******************************************************************************
NAME:       IPTCom_SimOpenInterface
ABSTRACT:   Opens an ethernet interface for simulation (only for backward compatibility)
RETURNS:    0 if succeded, <> 0 otherwise
*/
int IPTCom_SimOpenInterface(
   char *ifIP) /* IP Address for interface to open (ie "10.160.7.20")*/
{
   ETH_CARD *pCard = NULL;    /* Pointer to ethernet card info */

   /* Get a pointer to the requested card info */
   pCard = GetCardRef(ifIP);
   if (pCard != NULL)   
      return 0;

   return 1;
}

/*******************************************************************************
NAME:       IPTSimSendUDP
ABSTRACT:   Sends UDP Telegram simulated (spoofed IP (Layer 2 and 3) package)
RETURNS:    IPT_OK if succeded, !=0 if error
*/
int IPTCom_SimSendUDP(
   BYTE *pMsg,                   /* Message data, UDP payload data */
   int msgLen,                   /* Message length */
   struct sockaddr *srcAddr,     /* IP layer source address (spoofed) */
   struct sockaddr *destAddr)    /* IP layer destination address */
{
   ETH_CARD *pCard = NULL;    /* Reference to ethernet card used for this telegram */
   int res = -1;           /* Result code */
   UINT32 srcIPAddr;       /* Source IP address */
   UINT16 srcPortNo;       /* Source port no */
   UINT32 dstIPAddr;       /* Destination IP address */
   UINT16 dstPortNo;       /* Destination port no */

   /* Access through non simulation API.s ? */
   if (((struct sockaddr_in *)srcAddr)->sin_addr.S_un.S_addr == 0)
   {
      if (NoOfCards <= 0)     
         return res;

      /* Set source address to the IP address allocated by tdc */
      srcIPAddr = htonl(IPTCom_getOwnIpAddr());
   }
   else   
   {
      /* Set source address to the supplied value */
      srcIPAddr = ((struct sockaddr_in *)srcAddr)->sin_addr.S_un.S_addr;
   }

   /* Setup destination address and port numbers */
   dstIPAddr = ((struct sockaddr_in *)destAddr)->sin_addr.S_un.S_addr;
   srcPortNo = ((struct sockaddr_in *)srcAddr)->sin_port;
   dstPortNo = ((struct sockaddr_in *)destAddr)->sin_port;

   /* Get a pointer to the card that simulates the source address */
   pCard = FindEthernetCardByDeviceIp(srcIPAddr);
   if (pCard == NULL)
      return res;

   /* Allocate and create complete UDP telegram (Layer 2, Layer 3) */
   res = SendUDPTgm(pCard, (BYTE *)pMsg, msgLen, srcIPAddr, dstIPAddr, srcPortNo, dstPortNo);
   return res;
}

/*******************************************************************************
NAME:       IPTCom_SimJoinMulticastgroup
ABSTRACT:   Called when a multicast group is joined. Sends IGMP telegram
RETURNS:    -
*/
void IPTCom_SimJoinMulticastgroup(
   UINT32 simDevAddr,    /* Simulated device address */
   UINT32 multicastAddr, /* Multicast address */
   UINT8 pd,             /* Join PD */
   UINT8 md)             /* Join MD */

{
   ETH_CARD *pCard = NULL; /* Pointer to Ethernet card used */
   UINT32 ownIPAddr;    /* The simulated (or real) device that joins the mc group */
   char IpAddr[20];     /* String representation of IP address */

   /* Client probably uses non simulation API. Assume source is local "tdc" address */
   if (simDevAddr == 0)
      ownIPAddr = IPTCom_getOwnIpAddr();
   else
      ownIPAddr = simDevAddr;

   /* Get which Ethernet card to use */
   pCard = FindEthernetCardByDeviceIp(htonl(ownIPAddr));
   if (pCard == NULL)
   {
      /* Call from non simulation API ? */
      if (simDevAddr == IPTCom_getOwnIpAddr())
      {
         /* Probably the local IP address is not registered */
         sprintf(IpAddr, "%d.%d.%d.%d", 
            (BYTE)((ownIPAddr & 0xFF000000) >> 24),
            (BYTE)((ownIPAddr & 0x00FF0000) >> 16),
            (BYTE)((ownIPAddr & 0x0000FF00) >> 8),
            (BYTE)((ownIPAddr & 0x000000FF)     ));

         /* Register local IP address */
         IPTCom_SimRegisterSimulatedInterface(IpAddr, IpAddr);

         /* Try to get a pointer to the Ethernet card */
         pCard = FindEthernetCardByDeviceIp(htonl(ownIPAddr));
         if (pCard == NULL)
            return;
      }
      else
         return;
   }

   /* Remember which multicast group we joined */
   if (AddMCG(pCard, multicastAddr, ownIPAddr, pd, md))
   {
      /* Send IGMP telegram */
      SendIGMPTgm(pCard, SIM_JOIN_MC_GROUP, 2, htonl(ownIPAddr), htonl(multicastAddr));
   }
}

/*******************************************************************************
NAME:       IPTCom_SimLeaveMulticastgroup
ABSTRACT:   Called when a multicast group is left
RETURNS:    -
*/
void IPTCom_SimLeaveMulticastgroup(
   UINT32 simDevAddr,    /* Simulated device address */
   UINT32 multicastAddr, /* Multicast address */
   UINT8 pd,             /* Join PD */
   UINT8 md)             /* Join MD */

{
   ETH_CARD *pCard = NULL; /* Pointer to Ethernet card used */
   UINT32 ownIPAddr;    /* The simulated (or real) device that leaves the mc group */

   /* Client probably uses non simulation API. Assume source is local "tdc" address */
   if (simDevAddr == 0)
      ownIPAddr = IPTCom_getOwnIpAddr();
   else
      ownIPAddr = simDevAddr;

   /* Get which Ethernet card to use */
   pCard = FindEthernetCardByDeviceIp(htonl(ownIPAddr));
   if (pCard != NULL)
   {
      if (RemoveMCG(pCard, multicastAddr, ownIPAddr, pd, md))
      {
         /* Send IGMP telegram */
         SendIGMPTgm(pCard, SIM_LEAVE_MC_GROUP, 2, htonl(ownIPAddr), htonl(multicastAddr));
      }
   }
}

/*******************************************************************************
NAME:       IPTSimProcessUDPData
ABSTRACT:   Retrieves packages and forwards them to IPTCom stack when necessary
RETURNS:    IPT_OK if succeded, !=0 if error
*/
int IPTCom_SimProcessUDPData(
   BYTE *pRecBuf,       /* Recieve buffer */
   unsigned short port, /* Port to listen on (IP-PD) */
   UINT32 *srcAddr,     /* Source address */
   UINT16 *srcPort,     /* The udp source port */
   UINT32 *destAddr,    /* Destination address */
   UINT32 *sizeUDP)     /* Size of UDP data */
{
   SOCK_MSG *pMsg = NULL;        /* Pointer to received telegram */
   int i;                     /* Loop variable */
   SOCK_MSG_LIST *pList = NULL;  /* List pointer */

   *sizeUDP = 0;
   for ( i = 0; i < 2; i++)
   {
      /* Which port is it ? */
      switch (port)
      {
      case SIM_PD_PORT_NO:
         /* PD */
         pList = &SockMsgList[SIM_PD_LIST];
         break;
      case SIM_MD_PORT_NO:
         /* MD */
         pList = &SockMsgList[SIM_MD_LIST];
         break;
      case SIM_SNMP_PORT_NO:
         /* SNMP */
         pList = &SockMsgList[SIM_SNMP_LIST];
         break;
      default:
         Sleep(50);
         continue;
      }
      
      /* Anything in the list ? */
      if (pList->pStart != NULL)
      {
         /* Initialized ? */
         if (pList->InitializedCS == 0)
         {
            InitializeCriticalSection(&pList->cs);       
            pList->InitializedCS = 1;
         }

         EnterCriticalSection(&pList->cs);

         /* Yes. Get first message */
         pMsg = pList->pStart;
         pList->pStart = pMsg->pNext;
         if (pList->pStart == NULL)
            pList->pEnd = NULL;

         LeaveCriticalSection(&pList->cs);

         /* Copy data to output parameters */
         memcpy(pRecBuf, pMsg->Msg, pMsg->Size);
         *sizeUDP = pMsg->Size;
         *srcAddr = pMsg->srcaddr;
         *srcPort = pMsg->srcport;
         *destAddr = pMsg->destaddr;         
         free((unsigned char *)pMsg);
         return (int)IPT_OK;
      }

      /* Nothing in list - Wait 50 ms to reduce CPU load */
      Sleep(50);
   }

   *sizeUDP = 0;
   return (int)IPT_OK;
}

/*******************************************************************************
NAME:       HandleUDPMsg
ABSTRACT:   Handles UDP message
RETURNS:    -
*/
void HandleUDPMsg(
   struct pcap_pkthdr *header, /* The header for the received message */
   u_char *pkt_data,           /* The received message */
   int    ThisIsMulticast,     /* 1 => This is a multicast package */
   UINT32 HostIp)              /* Intended receiver for multicast package */
{
   IP_HDR *ip;          /* Pointer to ip header    */
   UDP_HDR *udp;        /* Pointer to udp header   */
   int udpDataStart;    /* Start of UDP data    */
   int udpDataLen;         /* Size of UDP message     */
   SOCK_MSG *pMsg;         /* Pointer to struct containing the received UDP message */
   UINT16 DestPort;     /* Destination port */
   SOCK_MSG_LIST *pList = NULL;  /* List pointer */

   IPT_UNUSED (header)
  
   /* Setup pointer to ip header */
   ip = (IP_HDR *) &pkt_data[sizeof(ETH_HDR)];/*lint !e826 This is the start of the ip header */

   /* Calculate the start of udp data */
   udpDataStart = sizeof(ETH_HDR) + ((ip->verlen & 0x0F) * 4) + sizeof(UDP_HDR);

   /* Pointer to udp header */
   udp = (UDP_HDR *) &pkt_data[sizeof(ETH_HDR) + ((ip->verlen & 0x0F) * 4)];/*lint !e826 This is the start of the udp header */   

   /* Size of udp message */
   udpDataLen = ntohs(udp->len) -  sizeof(UDP_HDR);

   /* Is it PD, MD or SNMP ? */
   DestPort = ntohs(udp->dest);
   if ((DestPort != SIM_PD_PORT_NO) && (DestPort != SIM_MD_PORT_NO) && (DestPort != SIM_SNMP_PORT_NO))
      return;

   /*lint -save -e433 -e826 Allocate memory for struct sock message and the data part */   
   pMsg = (struct sockmsg *)malloc(sizeof(SOCK_MSG) + udpDataLen);
   if (pMsg == NULL)
      return;
   /*lint -restore */

   /* Copy received UDP data to buffer */
   memcpy(&pMsg->Msg, &pkt_data[udpDataStart], udpDataLen);
   pMsg->Size = udpDataLen;

   /* Copy source and destination addresses */
   pMsg->srcaddr = ip->saddr;
   pMsg->srcport = ntohs(udp->source);
   if (ThisIsMulticast)
      pMsg->destaddr = htonl(HostIp);
   else
      pMsg->destaddr = ip->daddr;

   pMsg->pNext = NULL;

   /* Which type of telegram is it ? */
   switch (DestPort)
   {
   case SIM_PD_PORT_NO:
      pList = &SockMsgList[SIM_PD_LIST];
      break;

   case SIM_MD_PORT_NO:
      pList = &SockMsgList[SIM_MD_LIST];
      break;

   case SIM_SNMP_PORT_NO:
      pList = &SockMsgList[SIM_SNMP_LIST];
      break;
   
   default:
      return;
   }

   /* Initialized ? */
   if (pList->InitializedCS == 0)
   {
      InitializeCriticalSection(&pList->cs);       
      pList->InitializedCS = 1;
   }

   EnterCriticalSection(&pList->cs);

   /* This is PD */
   if (pList->pStart == NULL)
      pList->pStart = pMsg;
   else
      pList->pEnd->pNext = pMsg;

   pList->pEnd = pMsg;              

   LeaveCriticalSection(&pList->cs);
}

/*******************************************************************************
NAME:       HandleICMPMsg
ABSTRACT:   Handles received ICMP message
RETURNS:    -
*/
void HandleICMPMsg(
   ETH_CARD *pCard,              /* Which card it was received from     */
   int SendReplyFromAllDevices,     /* 1 = Send ICMP reply from all simulated devices */
   struct pcap_pkthdr *header,      /* The header for the received message */
   u_char *pkt_data)          /* The received message             */
{
   IP_HDR *ip;             /* Pointer to ip header    */
   ICMP_PACKET *icmp;         /* Pointer to icmp packet  */
   int icmpHeaderStart;    /* Start of icmp header    */
   int NoOfTgmsToSend = 1;    /* Number of reply telegrams to send      */
   UINT32 srcIPAddr;       /* IP layer source address (spoofed)      */
   UINT32 dstIPAddr;       /* IP layer destination address           */
   SIM_IF_INFO *pSIF;         /* Pointer to simulated interface         */

   IPT_UNUSED (header)

   /* Get pointer to ip header */
   ip = (IP_HDR *) &pkt_data[sizeof(ETH_HDR)];/*lint !e826 This is the start of the ip header */

   /* Get pointer to icmp header */
   icmpHeaderStart = sizeof(ETH_HDR) + ((ip->verlen & 0x0F) * 4);
   icmp = (ICMP_PACKET *) &pkt_data[icmpHeaderStart];/*lint !e826 This is the start of the icmp header */

   /* Should the response be sent from all devices */
   /* (i.e. subnet directed broadcast of request)  */
   if (SendReplyFromAllDevices)
   {
      /* Yes. Calculate number of telegrams to send */
      NoOfTgmsToSend = pCard->NoOfSimulatedIfs;
      if (NoOfTgmsToSend <= 0)
         return;
   }

   do 
   {  
      /* Which ICMP message was it ? */
      switch (icmp->typ)
      {
      case 0:
         /* echo reply */
         break;
      case 8:
         /* echo request (PING)*/
         
         /* Send ping replay from all simulated devices ? */
         if (SendReplyFromAllDevices)
         {
            /* Yes. Get next simulated interface */
            pSIF = &pCard->SimulatedIFs[NoOfTgmsToSend - 1];
            srcIPAddr = pSIF->ipaddr;
         }
         else
         {
            /* No. Get the requested ip address */
            srcIPAddr = ip->daddr;
         }
         dstIPAddr = ip->saddr;        
         
         /* Create ping reply */
         SendICMPTgm(pCard, 0, 0, srcIPAddr, dstIPAddr, &pkt_data[icmpHeaderStart + sizeof(ICMP_PACKET)]);

         break;
      default:
         break;
      }

      NoOfTgmsToSend--;

   } while (NoOfTgmsToSend > 0);
}

/*******************************************************************************
NAME:       HandleIGMPMsg
ABSTRACT:   Handles received IGMP message
RETURNS:    -
*/
void HandleIGMPMsg(
   ETH_CARD *pCard,            /* Which card it was received from */
   struct pcap_pkthdr *header, /* The header for the received message */
   u_char *pkt_data)           /* The received message */
{
   IP_HDR *ip;             /* Pointer to ip header       */
   IGMP_PACKET *igmp;         /* Pointer to igmp packet     */
   int igmpDataStart;         /* Start of igmp header       */
   MC_INFO *ptrToItem;        /* Pointer to multicast group */
   int i;                  /* Loop varaiable          */
   int j;                  /* Loop varaiable          */

   IPT_UNUSED (header)

   /* Setup pointer to ip header */
   ip = (IP_HDR *) &pkt_data[sizeof(ETH_HDR)];/*lint !e826 This is the start of the ip header */

   /* Setup pointer to icmp header */
   igmpDataStart = sizeof(ETH_HDR) + ((ip->verlen & 0x0F) * 4);
   igmp = (IGMP_PACKET *) &pkt_data[igmpDataStart];/*lint !e826 This is the start of the icmp header */

   /* What kind of message was it */
   switch (igmp->typ)
   {
   case 0x11:
      /* Membership query */
      /* Is it a general query (i.e. is the address 224.0.0.1) */
      if (ip->daddr == ntohl(0xE0000001))
      {
         /* Report all multicast groups for this interface */
         for (i = 0; i < pCard->NoOfJoinedMCGs; i++)
         {
            for (j = 0; j < pCard->JoinedMCGs[i].NoOfHosts; j++)
               SendIGMPTgm(pCard, SIM_JOIN_MC_GROUP, 2, htonl(pCard->JoinedMCGs[i].TheHosts[j].simaddr), htonl(pCard->JoinedMCGs[i].mcaddr));
         }
      }
      else
      {
         /* Report this specific group if it is joined */

         /* Try to find item in MCG table */
         ptrToItem = FindMCGroup(pCard, ip->daddr);
         if (ptrToItem == NULL)
            return;

         /* Send IGMP telegram */
         for (j = 0; j < ptrToItem->NoOfHosts; j++)
            SendIGMPTgm(pCard, SIM_JOIN_MC_GROUP, 2, ptrToItem->TheHosts[j].simaddr, ip->daddr);
      }
      break;
   case 0x12:
   case 0x16:
   case 0x17:
   case 0x22:
      break;
   default:
      break;
   }
}

/*******************************************************************************
NAME:       HandleIPMsg
ABSTRACT:   Handles received IP message
RETURNS:    -
*/
void HandleIPMsg(
   ETH_CARD *pCard,             /* Which card it was received from */
   struct pcap_pkthdr *header,  /* The header for the received message */
   u_char *pkt_data)            /* The received message */
{
   SIM_IF_INFO *pIF;          /* Pointer to simulated interface data  */
   IP_HDR *ip;                /* Pointer to ip header             */
   ETH_HDR *eth;              /* Pointer to ethernet header       */
   UDP_HDR *udp;              /* Pointer to udp header            */
   int udplen;                /* Length of udp message            */
   int ThisMsgWasFragmented = 0; /* 1 = This IP message was fragmented  */
   IP_FRAGMENT_BASE *pFrag;      /* Pointer to fragment data structure   */
   MC_INFO *ptrToMcGroup;        /* Pointer to multicast group           */
   int i;                        /* Loop variable */

   /*lint -save -e826 This is the start of the eth header */   
   
   /* Setup pointer to ethernet header */
   eth = (ETH_HDR *) pkt_data;

   /* Setup pointer to ip header */
   ip = (IP_HDR *) &pkt_data[sizeof(ETH_HDR)];
   
   /*lint -restore */

   /* Update ARP cache with data from IP message */
   (void)AddToArpCache(ip->saddr, eth->h_source);

   /* Which protocol was it ?*/
   switch (ip->protocol)
   {
   case 0x01:
      /* This is an ICMP message */

      /* Try to find intended interface */
      pIF = FindSimulatedInterface(pCard, ip->daddr); 

      /* Device found ? */
      if (pIF == NULL)
      {
         /* No. Was the received ICMP telegram broadcasted ? */
         if (memcmp(eth->h_dest, gBroadcastMAC, ETH_ALEN) == 0)
         {
            /* Yes. Was it subnet directed broadcast (i.e. all hostbits set to 1) ?              */
            /* (The network mask is always 255.255.240.0 according to IPTrain addressing concept) */
            if ((ntohl(ip->daddr) & ~0xFFFFF000) == ~0xFFFFF000)
            {
               /* Yes. Send ping reply from all simulated nodes */      
               HandleICMPMsg(pCard, 1, header, pkt_data);
            }
            else
               break;
         }
         else
            break;
      }
      else
         HandleICMPMsg(pCard, 0, header, pkt_data);   /* Handle ICMP for this specific device */

      break;

   case 0x02:
      /* This is an IGMP message */
      HandleIGMPMsg(pCard, header, pkt_data);
      break;

   case 0x11:
      /* This is UDP */

      /********************************** Handle fragmentation *********************************/
      /* Is this message fragmented ? */
      if (ip->id != 0)
      {        
         /* Yes. Calculate the size of the udp message */
         udplen = header->len - sizeof(ETH_HDR) - ((ip->verlen & 0x0F) * 4);

         /* Setup pointer to udp header */
         udp = (UDP_HDR *) &pkt_data[sizeof(ETH_HDR) + ((ip->verlen & 0x0F) * 4)];/*lint !e826 This is the start of the udp header */        

         /* Add fragment to fragment data structure. */
         pFrag = AddFragment(ntohs(ip->id), ntohs(ip->frag_off), eth, ip, udp, udplen);
         if (pFrag != NULL)
         {
            /* Was this the last fragment for this message ? */
            if ((ntohs(ip->frag_off) & 0x2000) == 0)
            {
               /* Yes. Recreate the original message if possible */              
               if (RestoreIPMessage(pFrag, &header, &pkt_data) == 0)
                  return;

               ThisMsgWasFragmented = 1;

               /*lint -save -e826 This is the start of the eth and ip headers */   
            
               /* Setup pointer to ethernet header */
               eth = (ETH_HDR *) pkt_data;

               /* Setup pointer to ip header */
               ip = (IP_HDR *) &pkt_data[sizeof(ETH_HDR)];

               /*lint -restore */
            }        
            else
            {
               return;
            }
         }
      }

      /****************************************************************************************/

      /* Is this a multicast telegram ? */
      if ((ip->daddr & 0xF0) == 0xE0)
      {        
         /* Yes. Are there any members of this multicast group on this card ?*/
         ptrToMcGroup = FindMCGroup(pCard, ntohl(ip->daddr));
         if (ptrToMcGroup != NULL)
         {
            /* Yes. Send message to all hosts that has joined multicast group*/
            for (i = 0; i < ptrToMcGroup->NoOfHosts; i++)
            {
               HandleUDPMsg(header, pkt_data, 1, ptrToMcGroup->TheHosts[i].simaddr);
            }
         }
      }
      else
      {
         /* No. Was it to one of my interfaces ? */
         pIF = FindSimulatedInterface(pCard, ip->daddr); 
         if (pIF != NULL)
         {
            /* Yes. Handle message */
            HandleUDPMsg(header, pkt_data, 0, 0);
         }
      }

      /* If the message was created from IP fragments,   */
      /* header and pkt_data are allocated on the heap and must be returned*/
      if (ThisMsgWasFragmented)
      {
         if (header != NULL)
            free((BYTE *)header);

         if (pkt_data != NULL)
            free((BYTE *)pkt_data);
      }
      break;
   default:
      break;   
   }
}

/*******************************************************************************
NAME:       HandleARPMsg
ABSTRACT:   Handles received ARP messages
RETURNS:    -
*/
void HandleARPMsg(
   ETH_CARD *pCard,      /* Which card it was received from */
   u_char *pkt_data)     /* The ARP message */
{
   ETH_HDR *eth;           /* Pointer to ethernet header */
   ARP_PACKET *arp;        /* Pointer to arp header */
   SIM_IF_INFO *pIF;       /* Pointer to simulated device */
   SIM_IF_INFO pTempIF;    /* Temporary pointer to simulated device. Used with function call */
   SIM_IF_INFO pTempIF2;      /* Temporary pointer to simulated device. Used with function call */

   /* Setup pointer to arp header */
   arp = (ARP_PACKET *) &pkt_data[sizeof(ETH_HDR)]; /*lint !e826 This is the start of the arp header */

   /* What type of message was it */
   switch (ntohs(arp->op))
   {  
   case 1:
      /* This is an ARP request */

      /* Is the requested IP simulated ? */
      pIF = FindSimulatedInterface(pCard, arp->taddr);
      if (pIF != NULL)
      {
         /* Yes. Setup pointer to ethernet header */
         eth = (ETH_HDR *) pkt_data;/*lint !e826 This is the start of the eth header */   

         /* Prepare destination address */
         pTempIF.ipaddr = arp->saddr;
         memcpy(pTempIF.ethaddr, eth->h_source, ETH_ALEN);

         /* Prepare simulated device address */
         pTempIF2.ipaddr = pIF->ipaddr;
         memcpy(pTempIF2.ethaddr, pIF->ethaddr, ETH_ALEN);

         /* Create ARP reply */
         SendARPTgm(pCard, 2, eth->h_source, &pTempIF2, &pTempIF);
      }
      break;
   case 2:
      /* This is an ARP reply. Save the content in the ARP cache */
      (void)AddToArpCache(arp->saddr, arp->ethsender);
      break;
   default:
      break;
   }
}

/*******************************************************************************
NAME:       SimulatedNetworkTask
ABSTRACT:   Retrieves packages from the Ethernet
RETURNS:    1 if success, 0 otherwise
*/
static int __stdcall SimulatedNetworkTask(
   void *pArgs)   /* Ip address of the ethernet card used by this network task */
{
   struct pcap_pkthdr *header;      /* Pointer to pcap packet header */
   ETH_CARD *pCard = NULL;       /* Pointer to ethernet card data */
   ETH_HDR *eth;              /* Pointer to ethernet header    */
   u_char *pkt_data;          /* Pointer to ethernet packet    */
   UINT32 ipAddr;             /* Ip address of the ethernet card used by network task */
   int res;                /* Result code from call         */

   /* Get ip address to use */
   ipAddr = (UINT32)pArgs; 
   
   /* Get ethernet card reference */
   pCard = FindEthernetCard(ipAddr);

   /* Card found ? */
   if (pCard == NULL)
   {
      /* No. */
      printf("\nNetwork task can not be started with NULL argument\n");
      return 0;
   }

   /* Sanity check */
   if (pCard->pcap_fp == NULL)
   {
      printf("\nNetwork task can not be started with invalid arguments\n");
      return 0;
   }

   /* Retrieve the packets */
   while (!IPTGLOBAL(systemShutdown))
   {
      /* Try to get package from framework */
      res = pcap_next_ex(pCard->pcap_fp, &header, (const u_char **)&pkt_data);
      if (res < 0)
      {
         printf("Failed to get data from pcap : %d \n", res);
         Sleep(100);
         continue;
      }

      if(res > 0)
      {
         /* Received an ethernet frame */
         eth = (ETH_HDR *) pkt_data;/*lint !e826 This is the start of the eth header */   

         /* Which frame was it ? */
         switch(ntohs(eth->h_proto))
         {
            /* ARP frame */
         case 0x0806: 
            HandleARPMsg(pCard, pkt_data);
            break;

            /* RARP frame */
         case 0x8035:         
            break;

            /* IP frame */
         case 0x0800:         
            HandleIPMsg(pCard, header, pkt_data);
            break;
         
         default:
            break;
         }
      }
   }

   return 1;
}
#endif



