/*******************************************************************************
*              (c), Copyright 2006, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* prvOsDependent.h
*
* DESCRIPTION:
*       Public header which must be included to all .c files
*       which uses OS-dependent calls.
*       Implements wrappers for OS-dependent types/calls.
*
* FILE REVISION NUMBER:
*       $Revision: 2 $
*
*******************************************************************************/
#ifndef __msgOsDepenenth
#define __msgOsDepenenth

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef void* (*THREAD_PROC_T)(void*);

/******************************************************************************/
#ifdef WIN32
#   define _WIN32_WINNT 0x0400 /* for SignalObjectAndWait */
#   include <windows.h>
#   include <winsock2.h>
#   include <process.h>



    typedef unsigned (__stdcall *BEGIN_THREAD_EX_PROC_T)(void *);

    /* HANDLE is synonym for event, thread, mutex in Win32 */
    typedef HANDLE THREAD_T;
    typedef HANDLE THREAD_REF_T;
#   define  OS_GET_THREAD_REF_MAC(threadId) (threadId)

    typedef HANDLE SEMAPHORE_T;
    typedef HANDLE SEMAPHORE_REF_T;
#   define  OS_GET_SEMAPHORE_REF_MAC(semId) (semId)

    typedef HANDLE COND_T;
    typedef HANDLE COND_REF_T;
#   define  OS_GET_COND_REF_MAC(condId) (condId)

    typedef HANDLE MUTEX_T;
    typedef LONG   INTERLOCKED_LONG_T;

    /*typedef SOCKET_T SOCKET;*/
#   define SOCKET_T SOCKET

    /* Macros for fast OS-dependant calls */
#   define OS_INIT_MUTEX_MAC(mutex)     mutex=CreateMutex(NULL,FALSE,NULL)
#   define OS_MUTEX_LOCK_MAC(mutex)     WaitForSingleObject(mutex,INFINITE)
#   define OS_MUTEX_UNLOCK_MAC(mutex)   ReleaseMutex(mutex)
#   define OS_DESTROY_MUTEX_MAC(mutex)  CloseHandle(mutex)

    /* Macros for Interlocked emulation */
#   define OS_INIT_INTERLOCKED_COUNTER_CNS 0
#   define OS_INIT_INTERLOCKED_COUNTER_MAC(counter, val) (counter=val)
#   define OS_INTERLOCKED_INCREMENT_MAC(counterPtr) \
        InterlockedIncrement(counterPtr)
#   define OS_INTERLOCKED_DECREMENT_MAC(counterPtr) \
        InterlockedDecrement(counterPtr)
#   define OS_GET_INTERLOCKED_COUNTER_MAC(counter) (counter)

    /* Threads manipulation routines */
#   define OS_CREATE_THREAD_MAC(priority,stackSize,isSuspended, \
        threadProc,threadParam,threadIdPtr)                     \
            osCreateThreadWin32(priority,stackSize,isSuspended, \
            threadProc,threadParam,threadIdPtr)
#   define OS_RUN_THREAD_MAC(threadID)                          \
        ((ResumeThread(threadID)==((DWORD)-1))?SMB_MP_CANNOTRUNTHR:SMB_OK)
#   define OS_WAIT_FOR_SINGLE_THREAD_MAC(threadID)  \
        WaitForSingleObject(threadID,INFINITE)
#   define OS_WAIT_FOR_MULTIPLE_THREADS_MAC(threadsPtr,threadsNum)  \
    osWaitForMultipleThreadsWin32(threadsPtr,threadsNum);
/*        WaitForMultipleObjects(threadsNum,threadsPtr,TRUE,INFINITE);*/
#   define OS_CLEANUP_THREAD_HANDLE_MAC(threadID) CloseHandle(threadID)

    /* Semaphore manipulation routines */
#   define OS_CREATE_SEMAPHORE_MAC(semId,initValue)                         \
        (((semId=CreateSemaphore(NULL,initValue,0x7FFFFFFF,NULL))!=NULL)?   \
            SMB_OK:SMB_FAIL)
#   define OS_SEMAPHORE_WAIT_MAC(semId) \
    ((WaitForSingleObject(semId,INFINITE)==WAIT_OBJECT_0)?SMB_OK:SMB_FAIL)
/*        WaitForSingleObject(semId,INFINITE) */
#   define OS_POST_SEMAPHORE_MAC(semId) ReleaseSemaphore(semId,1,NULL) 
#   define OS_POST_SEMAPHORE_BY_REF_MAC(semId) \
        OS_POST_SEMAPHORE_MAC(semId)
#   define OS_DESTROY_SEMAPHORE_MAC(semId) CloseHandle(semId)

    /* Condition manipulation routines */
#   define OS_CREATE_COND_MAC(condID) \
    (((condID=CreateEvent(NULL,TRUE,FALSE,NULL))!=NULL)?SMB_OK:SMB_FAIL)
#   define OS_SIGNAL_COND_MAC(condID) SetEvent(condID)
#   define OS_BROADCAST_COND_MAC(condID) SetEvent(condID)
#   define OS_DESTROY_COND_MAC(condID) CloseHandle(condID)

    /* Sleep in milli-seconds */
#   define OS_SLEEP_MS_MAC(milli_seconds) Sleep(milli_seconds)

    /* Timer in milli-seconds */
#   define OS_GET_TIMER_MS_MAC() timeGetTime()

    /* Socket routines */
#   define OS_GET_SOCK_LAST_ERROR_MAC() WSAGetLastError()
#   define OS_SOCK_EINTR WSAEINTR
#   define OS_SOCK_EINPROGRESS WSAEINPROGRESS

#   define OS_TASK_CALL __stdcall

#   define OS_LOG_MESSAGE(message) OutputDebugStringA(message)


/*******************************************************************************
* osWaitForMultipleThreadsWin32
*
* DESCRIPTION:
*       Waits for termination of multiple threads.
*
* INPUTS:
*       threadsPtr - pointer to array of threads' descriptors
*       threadsNum - number of threads to wait
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       SMB_OK - on success
*       SMB_FAIL - strange behavior (invalid descriptor?)
*
* COMMENTS: In Win32 we uses single system call to accomplish this request.
*       OS-dependent errors from WaitForMultipleObjects
*       are mapped to SMB_ errors.
*
*******************************************************************************/
int osWaitForMultipleThreadsWin32
(
    IN  THREAD_T *threadsPtr,
    IN  int      threadsNum
);

/*******************************************************************************
* osWaitForMultipleThreadsTimedWin32
*
* DESCRIPTION:
*       Waits for termination of multiple threads with timeout.
*
* INPUTS:
*       threadsPtr - pointer to array of threads' descriptors
*       threadsNum - number of threads to wait
*       dwMilliseconds - tmeout in milliseconds
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       SMB_OK - on success
*       SMB_TIMEOUT - timeout
*       SMB_FAIL - strange behavior (invalid descriptor?)
*
* COMMENTS: In Win32 we uses single system call to accomplish this request.
*       OS-dependent errors from WaitForMultipleObjects
*       are mapped to SMB_ errors.
*
*******************************************************************************/
int osWaitForMultipleThreadsTimedWin32
(
    IN  THREAD_T *threadsPtr,
    IN  int      threadsNum,
    IN  DWORD    dwMilliseconds
);

int osCreateThreadWin32
(
    IN  int              priority,
    IN  unsigned int     stackSize,
    IN  int              isSuspended,
    IN  THREAD_PROC_T    threadProc,
    IN  void             *threadParamPtr,
    OUT THREAD_T         *threadIDPtr
);

#endif /* WIN32 */

/******************************************************************************/
#ifdef POSIX

#   include <pthread.h>
#   include <semaphore.h>
#   include <stdlib.h>
#   include <sys/types.h>
#   include <sys/socket.h>
#   include <netinet/in.h>
#   include <arpa/inet.h>
//#   include <errno.h>

    /* Wrappers' types for event, thread, mutex */
    typedef struct{
	    pthread_t           thread;
	    pthread_attr_t      attr;
	    void*               pParameter;
	    struct sched_param  priority;
        void*  (*threadProc)(void*);
    } THREAD_T, *THREAD_REF_T;
#   define OS_GET_THREAD_REF_MAC(threadId) (&threadId)

    typedef sem_t   SEMAPHORE_T;
    typedef sem_t*  SEMAPHORE_REF_T;
#   define OS_GET_SEMAPHORE_REF_MAC(semId) (&semId)

    typedef pthread_cond_t  COND_T;
    typedef pthread_cond_t* COND_REF_T;
#   define OS_GET_COND_REF_MAC(condId) (&condId)

#   define MUTEX_T  pthread_mutex_t
#   define COND_T   pthread_cond_t

#   define SOCKET_T int

#   define INVALID_HANDLE_VALUE NULL

    /* Adaptors for Win32-specific types */
#   define HANDLE 	unsigned long 
#   define DWORD	unsigned long
#   define VOID     void
#   define WINAPI
#   define LPVOID	void*
#   define BOOL     int
#   define LPLONG	long*
#   define LONG     long

    /* POSIX convenience definitions of priority */
#   define THREAD_PRIORITY_TIME_CRITICAL 	99
#   define THREAD_PRIORITY_HIGHEST			85
#   define THREAD_PRIORITY_ABOVE_NORMAL	    71
#   define THREAD_PRIORITY_NORMAL			57
#   define THREAD_PRIORITY_BELOW_NORMAL	    43
#   define THREAD_PRIORITY_LOWEST			28
#   define THREAD_PRIORITY_ABOVE_IDLE		14
#   define THREAD_PRIORITY_IDLE		        0

    /* Macros for fast OS-dependant calls */
#   define OS_INIT_MUTEX_MAC(mutex)     pthread_mutex_init(&mutex,NULL)
#   define OS_MUTEX_LOCK_MAC(mutex)     pthread_mutex_lock(&mutex)
#   define OS_MUTEX_UNLOCK_MAC(mutex)   pthread_mutex_unlock(&mutex)
#   define OS_DESTROY_MUTEX_MAC(mutex)  pthread_mutex_destroy(&mutex)

    /* POSIX has no Interlocked counters. So guard counter by mutex. */
    typedef struct{
	    pthread_mutex_t mutex;
	    int             counter;
    } INTERLOCKED_LONG_T;

    /* Macros for Interlocked emulation */
#   define OS_INIT_INTERLOCKED_COUNTER_CNS {PTHREAD_MUTEX_INITIALIZER,0}
#   define OS_INIT_INTERLOCKED_COUNTER_MAC(counterVar, val)  \
        ((counterVar).counter=val)
#   define OS_INTERLOCKED_INCREMENT_MAC(counter) \
        osInterlockedIncrementPOSIX(counter)
#   define OS_INTERLOCKED_DECREMENT_MAC(counter) \
        osInterlockedDecrementPOSIX(counter)
#   define OS_GET_INTERLOCKED_COUNTER_MAC(counterVar) ((counterVar).counter)

    /* Threads manipulation routines */
#   define OS_CREATE_THREAD_MAC(priority,stackSize,isSuspended, \
        threadProc,threadParam,threadIdPtr)                     \
            osCreateThreadPOSIX(priority,stackSize,isSuspended, \
            threadProc,threadParam,threadIdPtr)
#   define OS_RUN_THREAD_MAC(threadID)                          \
        osRunThreadPOSIX(OS_GET_THREAD_REF_MAC(threadID))
#   define OS_WAIT_FOR_SINGLE_THREAD_MAC(threadID) pthread_join((threadID).thread,NULL)
#   define OS_WAIT_FOR_MULTIPLE_THREADS_MAC(threadsPtr,threadsNum)  \
        osWaitForMultipleThreadsPOSIX(threadsPtr,threadsNum);
#   define OS_CLEANUP_THREAD_HANDLE_MAC(threadID) ;

    /* Semaphore manipulation routines */
#   define OS_CREATE_SEMAPHORE_MAC(semId,initValue)     \
    ((sem_init(&semId,0,initValue)!=-1)?SMB_OK:SMB_FAIL)
//#   define OS_SEMAPHORE_WAIT_MAC(semId)	\
//    ((sem_wait(&semId)==0)?SMB_OK:SMB_FAIL)
#   define OS_SEMAPHORE_WAIT_MAC(semId)	\
        osSemWaitPOSIX(&semId)
    
#   define OS_POST_SEMAPHORE_MAC(semId) sem_post(&semId)
#   define OS_POST_SEMAPHORE_BY_REF_MAC(semRef) sem_post(semRef)
#   define OS_DESTROY_SEMAPHORE_MAC(semId) sem_destroy(&semId)

    /* Condition manipulation routines */
#   define OS_CREATE_COND_MAC(condID)       \
        ((pthread_cond_init(&condID, NULL)==0)?SMB_OK:SMB_FAIL)
#   define OS_SIGNAL_COND_MAC(condID) pthread_cond_signal(&condID)
#   define OS_BROADCAST_COND_MAC(condID) pthread_cond_broadcast(&condID)
#   define OS_DESTROY_COND_MAC(condID)      \
        pthread_cond_destroy(&condID)

    /* Sleep in milli-seconds */
#   define OS_SLEEP_MS_MAC(milli_seconds) osNanoSleepPOSIX((milli_seconds)*1000000L)

    /* Timer in milli-seconds */
#   define OS_GET_TIMER_MS_MAC() osGetTimerPOSIX()

    /* Socket routines */
#   define OS_GET_SOCK_LAST_ERROR_MAC() errno
#   define OS_SOCK_EINTR EINTR
#   define OS_SOCK_EINPROGRESS EINPROGRESS
#   define INVALID_SOCKET  (SOCKET_T)(-1)
#   define SOCKET_ERROR              (-1)

    /* calling convention for task function */
#   define OS_TASK_CALL

    /* case insensetive comparing */
#   define strcmpi strcasecmp

    /* logging function */
#   define OS_LOG_MESSAGE(message) printf("%s", message)
#   define _snprintf snprintf
#   define _vsnprintf vsnprintf



/*******************************************************************************
* osSemWaitPOSIX
*
* DESCRIPTION:
*       Wrapper to wait semaphore for POSIX.
*
* INPUTS:
*       semIdPtr - pointer to semaphore.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       SMB_OK - on success
*       SMB_FAIL - other error
*
* COMMENTS: EINTR error during sem_wait is skipped
*
*******************************************************************************/
int osSemWaitPOSIX(SEMAPHORE_REF_T semIdPtr);

/*******************************************************************************
* osRunThreadPOSIX
*
* DESCRIPTION:
*       Wrapper to run thread for POSIX.
*
* INPUTS:
*       None.
*
* INPUTS/OUTPUTS:
*       threadIDRef - reference to thread.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       SMB_OK - on success
*       SMB_MP_EAGAIN, SMB_MP_EINVAL, SMB_MP_EPERM - errors from pthread_create.
*       SMB_FAIL - other error
*
* COMMENTS: Note that actually thread is created there, not during ctearion
*       wrapper osCreateThreadPOSIX. That's because we cannot create
*       suspended thread in POSIX, but we are using that model.
*
*******************************************************************************/
int osRunThreadPOSIX(INOUT THREAD_REF_T threadIDRef);

/*******************************************************************************
* osCreateThreadPOSIX
*
* DESCRIPTION:
*       Wrapper to create a thread for POSIX.
*
* INPUTS:
*
* OUTPUTS:
*       threadIDPtr - ponter to thread ID to be returned on successfull call.
*
* RETURNS:
*       SMB_OK - on success
*       SMB_ATTR_INIT_ERROR - cannot set attributes for thread
*       SMB_BAD_STACK_SZ - stach size is invalid
*       SMB_FAIL - other error
*
* COMMENTS: Note that actually SUSPENDED thread is not created, only inited.
*       That's because we cannot create
*       suspended thread in POSIX, but we are using that model.
*
*******************************************************************************/
int osCreateThreadPOSIX
(
    IN  int              priority,
    IN  unsigned int     stackSize,
    IN  int              isSuspended,
    IN  void*            (*threadProc)(void*),
    IN  void             *threadParamPtr,
    OUT THREAD_T         *threadIDPtr
);

/*******************************************************************************
* osWaitForMultipleThreadsPOSIX
*
* DESCRIPTION:
*       Waits for termination of multiple threads.
*
* INPUTS:
*       threadsPtr - pointer to array of threads' descriptors
*       threadsNum - number of threads to wait
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       SMB_OK - on success
*
* COMMENTS: In POSIX we uses one system call per thread
*       to accomplish this request (we havn't pthread_join for multiple!). 
*
*******************************************************************************/
int osWaitForMultipleThreadsPOSIX
(
    IN  THREAD_T *threadsPtr,
    IN  int      threadsNum
);

/*******************************************************************************
* osInterlockedIncrementPOSIX
*
* DESCRIPTION:
*       Thread-safe increment for counter in POSIX.
*
* INPUTS:
*       None.
*
* INPUTS/OUTPUTS:
*       counterStructPtr - pointer to interlocked struct
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       New (incremented) value of a counter.
*
* COMMENTS: In POSIX we haven't system call InterclockedIncrement,
*       so we emulate it by mutex+counter. 
*
*******************************************************************************/
int osInterlockedIncrementPOSIX(INOUT INTERLOCKED_LONG_T *counterStructPtr);

/*******************************************************************************
* osInterlockedDecrementPOSIX
*
* DESCRIPTION:
*       Thread-safe decrement for counter in POSIX.
*
* INPUTS:
*       None.
*
* INPUTS/OUTPUTS:
*       counterStructPtr - pointer to interlocked struct
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       New (decremented) value of a counter.
*
* COMMENTS: In POSIX we haven't system call InterclockedDecrement,
*       so we emulate it by mutex+counter. 
*
*******************************************************************************/
int osInterlockedDecrementPOSIX(INOUT INTERLOCKED_LONG_T *counterStructPtr);

/*******************************************************************************
* osNanoSleepPOSIX
*
* DESCRIPTION:
*       Wrapper for nanosleep call.
*
* INPUTS:
*       time_ns - time to wait, in nanoseconds.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       None.
*
* COMMENTS: nanosleep call can be interrupted by signal. So we should handle
*       this case to continue to wait.
*
*******************************************************************************/
void osNanoSleepPOSIX(IN unsigned long time_ns);


/*******************************************************************************
* osGetTimerPOSIX
*
* DESCRIPTION:
*       Wrapper for timer call; return timer in milli-seconds.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       Timer value.
*
* COMMENTS: we can overflow in result beyond 32 bits,
*       but that doesn't matter 'cause we use always difference of time,
*       not absolute value.
*
*******************************************************************************/
unsigned long osGetTimerPOSIX(void);
#endif /* POSIX */

/******************************************************************************/

#ifdef __linux__
/*******************************************************************************
* osWaitForMultipleThreadsTimedLinux
*
* DESCRIPTION:
*       Waits for termination of multiple threads with timeout.
*
* INPUTS:
*       threadsPtr - pointer to array of threads' descriptors
*       threadsNum - number of threads to wait
*       dwMilliseconds - tmeout in milliseconds
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       SMB_OK - on success
*       SMB_TIMEOUT - timeout
*
* COMMENTS: In Linux we uses one NON-PORTABLE system call per thread
*       to accomplish this request (we havn't pthread_join for multiple!). 
*       OS-dependent errors from pthread_timedjoin_np are mapped to SMB_ errors.
*
*******************************************************************************/
int osWaitForMultipleThreadsTimedLinux
(
    IN  THREAD_T *threadsPtr,
    IN  int      threadsNum,
    IN  DWORD    dwMilliseconds
);
#endif /* __linux__ */

/******************************************************************************/
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __msgOsDepenenth */
