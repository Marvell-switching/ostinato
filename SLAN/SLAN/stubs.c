#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

// GREGORY
typedef unsigned long       DWORD;
#define NULL 0

#include <private/prvSBCommon.h>
//#include <private/prvOsDependent.h>

#include "slanLib.h"
#define SLANS_MAX 64

#include <gtEnvDep.h>
#include <sagent.h>

/* next SIM_OS_XXX defines are from cpss_enabler/../simulation/mainOs/h/linuxStubs/linuxSimOsSlan.h */

/* this msg has no data in it - just notify user */
#define SIM_OS_SLAN_NORMAL_MSG_RNS_CNS 0

/* call the client for a buffer space */
#define SIM_OS_SLAN_GET_BUFF_RSN_CNS          1

/* success read */
#define SIM_OS_SLAN_GIVE_BUFF_SUCCS_RSN_CNS   2

/* buffer allocation or read failed, empty buffer */
#define SIM_OS_SLAN_GIVE_BUFF_ERR_RSN_CNS     3

/* SLAN LinkUp / LinkDown events */
#define SAGNTP_SLANUP_CNS            (0x400 + 3)
#define SAGNTP_SLANDN_CNS            (0x400 + 4)

typedef struct slanParams_STC
{
    void *usr_info ;
    SAGNTG_rcv_FUN func ;
} slanParams_STC;

#define osPrintf printf

/*******************************************************************************
* slanLibEventHandlerFunc
*
* DESCRIPTION:
*       SLAN Event process function
*
* INPUTS:
*       slanId      - SLAN id
*       userData    - The data pointer passed to slanLibBind()
*       eventType   - event type
*       pktData     - pointer to packet
*       pktLen      - packet length
*
* OUTPUTS:
*       None
*
* RETURNS:
*       None
*
* COMMENTS:
*
*******************************************************************************/
static void slanLibEventHandlerFunc
(
    IN  SLAN_ID             slanId,
    IN  void*               userData,
    IN  SLAN_LIB_EVENT_TYPE eventType,
    IN  char*               pktData,
    IN  int                 pktLen
)
{
    slanParams_STC    *prm = (slanParams_STC*)userData;
    char    * buff = 0;
    int     res;

    if (prm->func == NULL)
        return;
#if 0
    fprintf(stderr, "Got event for slan %d, userData=%p, evType=%d pktLen=%d\n",
            slanId, userData, eventType, pktLen);
#endif
    switch (eventType)
    {
        case SLAN_LIB_EV_PACKET:
            /* call the client for a buffer space */
            if (pktLen == 0)
                break;
            buff = (prm->func)(1/* should not be 0 */,
                                        SIM_OS_SLAN_GET_BUFF_RSN_CNS,
                                        0/*don't care */,
                                        pktLen,
                                        prm->usr_info,
                                        pktData) ;

            if(!buff) /* check we have a buffer to copy to */
            {
                /* we don't have a buffer to copy to */
                return;
            }
            if (pktLen > 0)
            {
                res = SIM_OS_SLAN_GIVE_BUFF_SUCCS_RSN_CNS ;
            }
            else
            {
                res = SIM_OS_SLAN_GIVE_BUFF_ERR_RSN_CNS ;
osPrintf("file: %s(%d), res = SIM_OS_SLAN_GIVE_BUFF_ERR_RSN_CNS\n",	__FILE__, __LINE__);
            }
            /* copy data to buffer */
            memcpy(buff, pktData, pktLen);

            *(prm->func)(1/* should not be 0 */,
                                    res,
                                    0 /*don't care */,
                                    pktLen,
                                    prm->usr_info,
                                    buff);
            break;
        case SLAN_LIB_EV_LINKUP:
            (prm->func)(SAGNTP_SLANUP_CNS, 0, 
                                        0/*don't care */,
                                        0,
                                        prm->usr_info,
                                        NULL) ;
            break;
        case SLAN_LIB_EV_LINKDOWN:
            (prm->func)(SAGNTP_SLANDN_CNS, 0, 
                                        0/*don't care */,
                                        0,
                                        prm->usr_info,
                                        NULL) ;
            break;
        case SLAN_LIB_EV_CLOSED:
            fprintf(stderr, "The slan '%s' has closed, exiting\n",
                    slanLibGetSlanName(slanId));
            exit (0);
        default:
            break;
    }
}
/*******************************************************************************
* packetRxThread
*
* DESCRIPTION:
*       This is the task charged on receiving packet to "ASIC ports" or to 
*       CPU. The task is waiting using select call for any packet to be received
*       over one of the 5 interfaces, 4 "ASIC port" and 1 ASIC-CPU port.
*
* INPUTS:
*      None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*      None
*
* COMMENTS:
*       None
*
*******************************************************************************/
//static DWORD __stdcall  packetRxThread(PVOID pPtr)
int packetRxThread(long timeoutSec)
{
    if (slanLibMainLoop(timeoutSec) == 0)
        return SMB_TIMEOUT;
    return SMB_OK;
}


/*******************************************************************************
* slanTxRxInit
*
* DESCRIPTION:
*       Initialize the DB used for working with the NICs.
*       Allocates the required system resources (FD set, semaphore, task).
*
* INPUTS:
*      None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*      None
*
* COMMENTS:
*      Called from initialization of slanRxTask
*
*******************************************************************************/
void slanTxRxInit(void)
{
    slanLibInit(SLANS_MAX, "smbsim");
}

/*******************************************************************************
* slanTxRxDestroy
*
* DESCRIPTION:
*   Performs actions required on exit   
*
* INPUTS:
*      None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*      None
*
* COMMENTS:
*      Called from destroy of slanRxTask
*
*******************************************************************************/
void slanTxRxDestroy(void)
{
}

/*******************************************************************************
* SAGNTG_bind
*
* DESCRIPTION:
*       Creates a socket for each "ASIC port" it is called for and one for the
*       port used for CPU-ASIC communication. The "ASIC ports" are bind to a
*       "real" ethernet interface and the CPU-ASIC port is bind to the logical
*       loopback port.
*
* INPUTS:
*       slan_name - name of the NIC.
*       client_name - name of the client (ASIC port).
*       PoolSize - size of buffers allocated for that interface.
*       MaxClients - how many clients can bind to that interface.
*       usr_info - user information structure.
*       func - the routine to be called upon receiving packet from that
*       interface.
*
* OUTPUTS:
*       Identifier of the NIC.
*
* RETURNS:
*      None
*
* COMMENTS:
*       The name source is historical.       
*******************************************************************************/
void* SAGNTG_bind (

    /*!     INPUTS:             */

    char            *slan_name,

    char            *client_name,

    DWORD           PoolSize,

    DWORD           MaxClients,

    void            *usr_info,

    SAGNTG_rcv_FUN  func

    /*!     INPUTS / OUTPUTS:   */
    /*!     OUTPUTS:            */
)
{
    slanParams_STC * pThreadPrm;
    SLAN_ID slan;

    pThreadPrm = (slanParams_STC*)calloc(1, sizeof(*pThreadPrm));

    pThreadPrm->usr_info = usr_info ;
    pThreadPrm->func = func ;

    if (slanLibBind(slan_name, slanLibEventHandlerFunc, pThreadPrm, &slan) != 0)
    {
        osPrintf("file: %s(%d), failed to bind slan\n", __FILE__, __LINE__);
        exit(0);
    }

    return (void*)slan;
}


/*******************************************************************************
* SAGNTG_unbind
*
* DESCRIPTION:
*       
* INPUTS:
*      None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*      None
*
* COMMENTS:
*    The name source is historical.   
*******************************************************************************/
void SAGNTG_unbind (

    /*!     INPUTS:             */

    void    *slan_id

    /*!     INPUTS / OUTPUTS:   */
    /*!     OUTPUTS:            */
)
{
    SLAN_ID slan;
    slanParams_STC* pThreadPrm;

    slan = (SLAN_ID)slan_id;

    pThreadPrm = (slanParams_STC*)slanLibGetUserData(slan);
    if (pThreadPrm)
    {
        free(pThreadPrm);
    }
    slanLibUnbind(slan);
}


/*******************************************************************************
* SAGNTG_transmit
*
* DESCRIPTION:
*       
* INPUTS:
*       slan_id - the identifier given to the interface upon calling SAGNTG_bind 
*       client_name - the transmitter name.
*       msg_code - 
*       len - the message length.
*       msg - message.
*
* OUTPUTS:
*       How many bytes were sent.
*
* RETURNS:
*      None
*
* COMMENTS:
*    The name source is historical.   
*******************************************************************************/
unsigned int SAGNTG_transmit (

    /*!     INPUTS:             */

    void            *slan_id,

    char            *client_name,

    DWORD           msg_code,

    DWORD           len,

    char            *msg

    /*!     INPUTS / OUTPUTS:   */
    /*!     OUTPUTS:            */
)
{
    SLAN_ID slan;
    int sent;

    slan = (SLAN_ID)slan_id;

    sent = slanLibTransmit(slan, msg, len);
    return sent;
}
