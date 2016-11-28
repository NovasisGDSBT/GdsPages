/*******************************************************************************
*  COPYRIGHT   : (C) 2006 Bombardier Transportation
********************************************************************************
*  PROJECT     : IPTrain
*
*  MODULE      : mem.c
*
*  ABSTRACT    : Class file for memory allocation 
*
********************************************************************************
*  HISTORY     :
*	
* $Id: vos_mem.c 36148 2015-03-24 09:06:25Z gweiss $
*
*  CR-7241 (Bernd Loehr, 2014-01-27)
* 			Check for semaphore to allow threads terminating in
*			Simulation Mode.
*
*  CR-432 (Bernd Loehr, 2010-10-07)
* 			Elimination of compiler warnings
*			(include stdlib.h instead of malloc.h)
*
*  Internal (Bernd Loehr, 2010-08-16) 
* 			Old obsolete CVS history removed
*
*******************************************************************************/

/*******************************************************************************
*  INCLUDES 
*/
#include <stdio.h>
#include <stdlib.h>

#include "iptcom.h"     /* Common type definitions for IPT */
#include "iptcom_priv.h"

/*******************************************************************************
*  DEFINES AND MACROS
*/

/*******************************************************************************
*  TYPEDEFS
*/

/*******************************************************************************
*  GLOBALS
*/

/*******************************************************************************
*  LOCALS
*/

/*******************************************************************************
*  LOCAL FUNCTION DECLARATIONS
*/

/*******************************************************************************
*  LOCAL FUNCTIONS
*/

/*******************************************************************************
*  GLOBAL FUNCTIONS
*/


/*******************************************************************************
*  NAME     : IPTVosMemInit
*
*  ABSTRACT : Initialize memory area.
*             After init memory blocks are pre-allocated and freed. 
*             To avoid problem with too many small blocks and no large. 
*
*             Memory should have been allocated and set up in the global 
*             memory control block.
*
*  RETURNS  : 0 if OK, !=0 if error
*/
int IPTVosMemInit(void)
{
   int i, j, max;
   UINT32 blockSize[MEM_NBLOCKSIZES] = MEM_BLOCKSIZES;  /* Different block sizes */
   UINT32 preAllocate[MEM_NBLOCKSIZES] = MEM_PREALLOCATE; /* No of blocks that should be pre-allocated */
   unsigned char *p[MEM_MAX_PREALLOCATE];

   /* Memory not initiated by parsing the IPTCom XML config file ? */
   if (IPTGLOBAL(mem.noOfBlocks) == 0)
   {
       IPTGLOBAL(mem.noOfBlocks) = MEM_NBLOCKSIZES;
 
      /* Initialize free block headers */
      for (i = 0; i < MEM_NBLOCKSIZES; i++)
      {
         IPTGLOBAL(mem.freeBlock)[i].pFirst = (MEM_BLOCK *)NULL;
         IPTGLOBAL(mem.freeBlock)[i].size = blockSize[i];
         max = preAllocate[i];
         if (max > MEM_MAX_PREALLOCATE)
            max = MEM_MAX_PREALLOCATE;

         for (j = 0; j < max; j++)
         {
            p[j] = IPTVosMalloc(blockSize[i]);
         }

         for (j = 0; j < max; j++)
         {
            IPTVosFree(p[j]);
         }
      }
   }

   return 0;
}

/*******************************************************************************
*  NAME     : IPTVosMemDestroy
*
*  ABSTRACT : shutdown memory area
*
*  RETURNS  : 0 if OK, !=0 if error
*/
int IPTVosMemDestroy()
{
   /* Make sure nobody is waiting on the memory sempahore! */
   if (IPTVosGetSem(&IPTGLOBAL(mem.sem), IPT_WAIT_FOREVER) != (int)IPT_OK)
   {
      IPTVosPrint0(IPT_ERR, "IPTVosMemDestroy can't get semaphore, releasing anyway\n");
   }

   /* Destroy the memory allocation semaphore */
   IPTVosDestroySem(&IPTGLOBAL(mem.sem));

   return 0;
}

/*******************************************************************************
*  NAME     : IPTVosMallocBuf
*
*  ABSTRACT : Allocate memory and return allocated buffer size
*
*  RETURNS  : Pointer to memory area, or NULL if error
*/
BYTE *IPTVosMallocBuf(
               UINT32 size,      /* IN: Size of wanted memory area */
               UINT32 *pbufSize) /* OUT: Pointer to buffer size */
{
   UINT32 i, blockSize;
   MEM_BLOCK *pBlock;

   if (!pbufSize)
   {
      return (BYTE *)NULL;   /* No block size big enough */
   }
   
   /* Adjust size to get one which is a multiple of UINT32's */
   size += (ALIGNOF(UINT64ST) - 1);
   size &= ~(ALIGNOF(UINT64ST) - 1);

   /* Find appropriate blocksize */
   for (i = 0; i < IPTGLOBAL(mem.noOfBlocks); i++)
   {
      if (size <= IPTGLOBAL(mem.freeBlock)[i].size)
         break;
   }

   if (i >= IPTGLOBAL(mem.noOfBlocks))
   {
      IPTGLOBAL(mem.memCnt.allocErrCnt)++;
      
      IPTVosPrint1(IPT_ERR, "IPTVosMalloc No block size big enough. Requested size=%d\n",size);

      *pbufSize = 0;
      return (BYTE *)NULL;   /* No block size big enough */
   }

   /* Get memory sempahore */
   if (IPTVosGetSem(&IPTGLOBAL(mem.sem), IPT_WAIT_FOREVER) != (int)IPT_OK)
   {
      IPTGLOBAL(mem.memCnt.allocErrCnt)++;
      
      IPTVosPrint0(IPT_ERR, "IPTVosMalloc can't get semaphore\n");
      
      *pbufSize = 0;
      return (BYTE *)NULL;
   }

   blockSize = IPTGLOBAL(mem.freeBlock)[i].size;
   pBlock = IPTGLOBAL(mem.freeBlock)[i].pFirst;

   /* Check if there is a free block ready */
   if (pBlock != NULL)
   {    
      /* There is, get it. */ 
      /* Set start pointer to next free block in the linked list */
      IPTGLOBAL(mem.freeBlock)[i].pFirst = pBlock->pNext;
   }
   else
   {
      /* There was no suitable free block, create one from the free area */
      
      /* Enough free memory left ? */
      if ((IPTGLOBAL(mem.allocSize) + blockSize + sizeof(MEM_BLOCK)) < IPTGLOBAL(mem.memSize))
      {
         pBlock = (MEM_BLOCK *) IPTGLOBAL(mem.pFreeArea); /*lint !e826 Allocation of MEM_BLOCK from free area*/
      
         IPTGLOBAL(mem.pFreeArea) = (BYTE *) IPTGLOBAL(mem.pFreeArea) + (sizeof(MEM_BLOCK) + blockSize);
         IPTGLOBAL(mem.allocSize) += blockSize+ sizeof(MEM_BLOCK);
         IPTGLOBAL(mem.memCnt.blockCnt)[i]++;
      }
      else
      {
         while ((++i < IPTGLOBAL(mem.noOfBlocks)) && (pBlock == NULL)) 
         {
            pBlock = IPTGLOBAL(mem.freeBlock)[i].pFirst;
            if (pBlock != NULL)
            {    
               IPTVosPrint2(IPT_VOS,
                            "IPTVosMalloc Used a bigger buffer size=%d asked size=%d\n",
                            IPTGLOBAL(mem.freeBlock)[i].size,
                            size);
               /* There is, get it. */ 
               /* Set start pointer to next free block in the linked list */
               IPTGLOBAL(mem.freeBlock)[i].pFirst = pBlock->pNext;

               blockSize = IPTGLOBAL(mem.freeBlock)[i].size;
               
            }
         }
         if (pBlock == NULL)
         {
            /* Not enough memory */
            IPTVosPrint0(IPT_ERR, "IPTVosMalloc Not enough memory\n");
         }
      }
   }

   /* Release semaphore */
   if(IPTVosPutSemR(&IPTGLOBAL(mem.sem)) != IPT_OK)
   {
      IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
   }

   if (pBlock != NULL)
   {
      /* Fill in size in memory header of the block. To be used when it is returned.*/
      pBlock->size = blockSize;
      IPTGLOBAL(mem.memCnt.freeSize) -= blockSize + sizeof(MEM_BLOCK);
      if (IPTGLOBAL(mem.memCnt.freeSize) < IPTGLOBAL(mem.memCnt.minFreeSize))
      {
         IPTGLOBAL(mem.memCnt.minFreeSize) = IPTGLOBAL(mem.memCnt.freeSize);
      }
      IPTGLOBAL(mem.memCnt.allocCnt)++;

      /* Return pointer to data area, not the memory block itself */
      *pbufSize = blockSize;
      return (BYTE *) pBlock + sizeof(MEM_BLOCK);
   }
   else
   {
      IPTGLOBAL(mem.memCnt.allocErrCnt)++;
      
      *pbufSize = 0;
      return (BYTE *)NULL;
   }
}

/*******************************************************************************
*  NAME     : IPTVosMalloc
*
*  ABSTRACT : Allocate memory
*
*  RETURNS  : Pointer to memory area, or NULL if error
*/
BYTE *IPTVosMalloc(
               UINT32 size)     /* Size of wanted memory area */
{
   UINT32 blockSize;
   BYTE *pBuf;

   pBuf = IPTVosMallocBuf(size, &blockSize);

   return(pBuf);
}

/*******************************************************************************
*  NAME     : IPTVosFree
*
*  ABSTRACT : Free allocated memory.
*             The freed memory block is added to a free block linked list, for future use.
*
*  RETURNS  : 0 if OK, !=0 if error
*/
int IPTVosFree(
            const BYTE *pAllocated)    /* Pointer to memory block to free */
{
   int ret = 0;
   UINT32 i;
   UINT32 blockSize;
   MEM_BLOCK *pBlock;

   if (pAllocated == NULL)
   {
      IPTGLOBAL(mem.memCnt.freeErrCnt)++;
      
      IPTVosPrint0(IPT_ERR, "IPTVosFree ERROR NULL pointer\n");
      return (int)IPT_MEM_ERROR;
   }

   /* Check that the returned memory is within the allocated area */
   if ((pAllocated < IPTGLOBAL(mem.pArea)) || (pAllocated >= (IPTGLOBAL(mem.pArea) + IPTGLOBAL(mem.memSize))))
   {
      IPTGLOBAL(mem.memCnt.freeErrCnt)++;
      
      IPTVosPrint0(IPT_ERR, "IPTVosFree ERROR returned memmory not within allocated memory\n");
      return (int)IPT_MEM_ERROR;
   }

   /* Get memory sempahore */
   if (IPTVosGetSem(&IPTGLOBAL(mem.sem), IPT_WAIT_FOREVER) != (int)IPT_OK)
   {
      IPTGLOBAL(mem.memCnt.freeErrCnt)++;
      
      IPTVosPrint0(IPT_ERR, "IPTVosFree can't get semaphore\n");
      return (int)IPT_SEM_ERR;
   }

   /* Set block pointer to start of block, before the returned pointer */
   pBlock = (MEM_BLOCK *) ((BYTE *) pAllocated - sizeof(MEM_BLOCK)); /*lint !e826 Get pointer to allocated block */
   blockSize = pBlock->size;

   /* Find appropriate free block item */
   for (i = 0; i < IPTGLOBAL(mem.noOfBlocks); i++)
   {
      if (blockSize == IPTGLOBAL(mem.freeBlock)[i].size)
         break;
   }

   if (i >= IPTGLOBAL(mem.noOfBlocks))
   {
      IPTGLOBAL(mem.memCnt.freeErrCnt)++;
      
      IPTVosPrint0(IPT_ERR, "IPTVosFree illegal sized memory\n");
      ret = (int)IPT_MEM_ILL_RETURN;     /* Try to return a illegal sized memory */
   }
   else
   {
      IPTGLOBAL(mem.memCnt.freeSize) += blockSize + sizeof(MEM_BLOCK);
      IPTGLOBAL(mem.memCnt.allocCnt)--;
      /* Put the returned block first in the linked list */
      pBlock->pNext = IPTGLOBAL(mem.freeBlock)[i].pFirst;
      IPTGLOBAL(mem.freeBlock)[i].pFirst = pBlock;

      /* Destroy the size first in the block. If user tries to return same memory this will then fail. */
      pBlock->size = 0;
   }

   /* Release semaphore */
   if(IPTVosPutSemR(&IPTGLOBAL(mem.sem)) != IPT_OK)
   {
      IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
   }

   return ret;
}

/*******************************************************************************
*  NAME     : IPTVosMemAllocCnt
*
*  ABSTRACT : Returns the current value of allocated buffers
*
*  RETURNS  : Current value of allocated buffers
*/

UINT32 IPTVosMemAllocCnt(void)
{
   return(IPTGLOBAL(mem.memCnt.allocCnt));
}

/*******************************************************************************
*  NAME     : IPTVosMemShow
*
*  ABSTRACT :  Displays the memmory usage and statistic
*
*  RETURNS  : 
*/

void IPTVosMemShow(void)
{
   UINT32 i,j,k;
   MEM_BLOCK *pBlock;

   MON_PRINTF("Memory size = %d\n",IPTGLOBAL(mem.memSize));
   MON_PRINTF("Size allocated for blocks = %d\n",IPTGLOBAL(mem.allocSize));
   MON_PRINTF("Size not allocated for blocks = %d\n",IPTGLOBAL(mem.memSize) - IPTGLOBAL(mem.allocSize));
   MON_PRINTF("Free size = %d\n",IPTGLOBAL(mem.memCnt.freeSize));
   MON_PRINTF("Minimum free size = %d\n",IPTGLOBAL(mem.memCnt.minFreeSize));
   MON_PRINTF("No of alloc errors = %d\n",IPTGLOBAL(mem.memCnt.allocErrCnt));
   MON_PRINTF("No of free errors = %d\n",IPTGLOBAL(mem.memCnt.freeErrCnt));

   k = 0;
   for (i=0; i<IPTGLOBAL(mem.noOfBlocks); i++)
   {
      pBlock = IPTGLOBAL(mem.freeBlock)[i].pFirst;
      j = 0;
      /* Check if there is a free block ready */
      while (pBlock != NULL)
      {    
         j++;
         pBlock = pBlock->pNext;
      }
      k += IPTGLOBAL(mem.memCnt.blockCnt)[i];
      
      MON_PRINTF("Block size = %d cnt = %d free = %d\n",
              IPTGLOBAL(mem.freeBlock)[i].size,
              IPTGLOBAL(mem.memCnt.blockCnt)[i],
              j);
   }
   MON_PRINTF("Total no of blocks = %d Allocated blocks = %d\n",k,IPTGLOBAL(mem.memCnt.allocCnt));
          
}

/*******************************************************************************
NAME:       IPTVosMemIsPtr
ABSTRACT:   Checks if a pointer is a pointer into the IPTVos memory area.
RETURNS:    TRUE if OK, FALSE if not.
*/
int IPTVosMemIsPtr(
                   BYTE *p)   /* Pointer to memory area */
{
   return ((p >= IPTGLOBAL(mem.pArea)) && 
            (p < (IPTGLOBAL(mem.pArea) + IPTGLOBAL(mem.memSize))));
}

/*******************************************************************************
NAME:       IPTVosGetUniqueNumber
ABSTRACT:   Returns a number that is unique, 1..2**32
RETURNS:    Unique number.
*/
UINT32 IPTVosGetUniqueNumber(void)
{
   static int first = TRUE;
   static UINT32 n;

   /* Seed the unique number to get semi-random start value */
   if (first)
   {
      n = IPTVosGetMicroSecTimer();
      first = FALSE;
   }

   return n++;
}
