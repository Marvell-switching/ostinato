/*******************************************************************************
*              (c), Copyright 2006, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* prvSBCommon.h
*
* DESCRIPTION:
*       Public header which must be included to tasks' .c files
*       Contains declaration of all error codes and functions declaration
*       convention (IN, OUT, etc).
*       
* FILE REVISION NUMBER:
*       $Revision: 3 $
*
*******************************************************************************/
#ifndef __sbCommonh
#define __sbCommonh

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <assert.h> /* in order to define SMB_ASSERT */
#include <memory.h>
#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>

#if   defined WIN32
    #define STD_CALL_CONVENTION __stdcall
#elif defined WIN16 || defined _Windows || defined _WINDOWS || defined _WINDLL
	#define STD_CALL_CONVENTION _pascal
#else
	#define STD_CALL_CONVENTION
#endif

/* declarations agreements */
#define IN  /* const */
#define OUT /*  */
#define INOUT /* */
#define OPTIONAL

/* boolean flags */
#define TRUE     1
#define FALSE    0

/* error codes */
#define SMB_OK              0   /* On success */
#define SMB_FAIL            -1  /* Generic failure, should not be used,     */
                                /* please use more specific error codes     */
#define SMB_BAD_PARAM       -2  /* Invalid argument was passed              */
#define SMB_BAD_PTR         -3  /* On NULL pointer                          */
#define SMB_OUT_OF_RANGE    -4  /* Index is out of range                    */
#define SMB_OUT_OF_MEMORY   -5  /* Not enough memory to complete operation  */
#define SMB_ILLEGAL         -6  /* Request is illegal for current state     */
#define SMB_TIMEOUT         -7  /* Exit was done by timeout                 */
#define SMB_FILE_NOT_EXIST  -8  /* File not found                           */
#define SMB_BAD_STACK_SZ	-9	/* Ivalid stack size for thread				*/
#define SMB_ATTR_INIT_ERROR	-10	/* Atribute init error						*/



#define SMB_MP_EAGAIN           -16 /* too many threads                     */
#define SMB_MP_EINVAL           -17 /* argument is invalid or               */
#define SMB_MP_EPERM            -117 /* permission error */
                                    /* stack size is incorrect              */
#define SMB_MP_BEGINTHREAD0     -18 /* _beginthreadex returns 0 on an error */
#define SMB_MP_CANNOTSETPRI     -19 /* SetThreadPriority failed             */
#define SMB_MP_CANNOTRUNTHR     -20 /* ResumeThread failed                  */
#define SMB_MP_POOL_HAS_NO_FREE_MESSAGES   -21 /* pool has no free messages */
#define SMB_MP_POOL_INVALID_STATE          -22 /* query to destroying pool  */

#define SMB_SL_STRUCT_SIZES_MISMATCH    -32 /* struct size wrong            */
#define SMB_SL_DEFAULT_SMARTLIB_ERROR   -33 /* used only on error remapping */
#define SMB_SL_INVALID_TYPE1            -34 /* unsupported iType1 param     */
#define SMB_SL_UNSUPPORTED_COMMAND      -35 /* unsupported command          */
#define SMB_SL_REPEATED_CALL            -36 /* reinquiry of 
                                                       membership/ownership */
#define SMB_SL_NOT_LINKED               -37 /* sequence error for
                                                socket unlink */

#define SMB_UNKNOWN_MSG_TYPE            -40 /* unsupported message type     */
#define SMB_MMU_BAD_BLOCK               -41 /* memory block was damaged     */
#define SMB_CTX_LOCK_FAILED             -50 /* lock operation failed  */

#define SMB_CT_INVALID_STATE            -64 /* state machine mismatch       */
#define SMB_CX_INVALID_HUBSLOTPORT      -65 /* hub/slot/port out of range   */

/* remote connection error codes */
#define SMB_SOCKET_FAILED           -70  /* Error in the socket connection  */
                                         /* for an Ethernet Link            */
                                         /* (PC to SmartBits)               */
#define SMB_SOCKET_TIMEOUT          -71  /* Timeout on the socket connection*/
                                         /* for an Ethernet Link            */
                                         /* (PC to SmartBits)               */

/* the default TCP port to listen to for SmartBits */
#define SMARTBITS_DEFAULT_PORT_CNS 16386

/* it defines how often we check if library is shutting
    inside endless cycle (blocked working procedure)
    in order to correctly shut the module */
#define     LIB_SHUT_CHECK_PERIOD_CNS       2 /* sec */

/* typedefs */
/*
 * Typedef: CTX_HSP_STC
 *
 * Description: hub/slot/port struct
 *
 * Comments: used as single struct for hub/slot/port triplet specification
 */
typedef struct
{
	int hub, slot, port;
} CTX_HSP_STC;



/*typedef unsigned char   uchar;
typedef unsigned short  ushort;
typedef unsigned long   ulong;
typedef unsigned int    uint;
*/

/*
#ifdef  NDEBUG
#define SMB_ASSERT( exp ) \
        smbAssert(exp, #exp, moduleName, __FILE__, __LINE__)
#else
#define SMB_ASSERT(exp) \
        smbAssert(exp, #exp, moduleName, __FILE__, __LINE__); \
        assert(exp)
#endif 
*/
#define SMB_ASSERT(exp)                                         \
do{                                                             \
        smbAssert(exp, #exp, moduleName, __FILE__, __LINE__);   \
        assert(exp);                                            \
}while(0)
/*******************************************************************************
* smbAssert
*
* DESCRIPTION:
*       
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS: 
*
*******************************************************************************/
void smbAssert(
    IN  int expression,
    IN  const char *exprString,
    IN  const char *moduleName,
    IN  const char *fileName,
    IN  unsigned long lineNumber
);

/*******************************************************************************
* mpCreateAllMessageBoxes
*
* DESCRIPTION:
*       Create all message boxes required for simulation.
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       SMB_OK - all message boxes have been created successfully.
*       Otherwise - all error codes from mpMessageBoxCreate.
*
* COMMENTS: This function is a base Init routine of simulation.
*       It creates all predefined message boxes (such as LOG_TASK_CNS
*       or TRANSMIT_TASK_CNS defined in prvSBCommon.h).
*
*******************************************************************************/
int mpCreateAllMessageBoxes(void);

/*******************************************************************************
* mpStartAllMessageBoxes
*
* DESCRIPTION:
*       Starts all message boxes required for simulation.
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       SMB_OK - all message boxes have been destroyed successfully.
*
* COMMENTS: This function is a base Init routine of simulation.
*       It runs all created message boxes.
*
*******************************************************************************/
int mpStartAllMessageBoxes(void);

/*******************************************************************************
* mpDestroyAllMessageBoxes
*
* DESCRIPTION:
*       Destroys all message boxes required for simulation.
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       SMB_OK - all message boxes have been destroyed successfully.
*
* COMMENTS: This function is a part of Destroy routine of simulation.
*       It disposes all predefined message boxes (such as LOG_TASK_CNS
*       or TRANSMIT_TASK_CNS defined in prvSBCommon.h).
*
*******************************************************************************/
int mpDestroyAllMessageBoxes(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __sbCommonh */

