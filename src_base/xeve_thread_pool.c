/* Copyright (c) 2020, Samsung Electronics Co., Ltd.
   All Rights Reserved. */
/*
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:
   
   - Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
   
   - Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
   
   - Neither the name of the copyright owner, nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.
   
   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdio.h>
#include <stdlib.h>
#include "xeve_thread_pool.h"
#if defined(WIN32) || defined(WIN64)
#include <Windows.h>
#include <process.h>
#else
#include <pthread.h>
#endif

#define WINDOWS_MUTEX_SYNC 0

#if !defined(WIN32) && !defined(WIN64) 

typedef struct _THREAD_CTX
{
    //synchronization members
    pthread_t t_handle; //worker thread handle
    pthread_attr_t tAttribute;//worker thread attribute
    pthread_cond_t w_event; //wait event for worker thread
    pthread_cond_t r_event; //wait event for main thread
    pthread_mutex_t c_section; //for synchronization

    //member field to run  a task                   
    THREAD_ENTRY task;
    void * t_arg;
    THREAD_STATUS t_status;
    THREAD_RESULT task_result;
    int thread_id;
}THREAD_CTX;

typedef struct _syncobject
{
    pthread_mutex_t lmutex;
}THREAD_MUTEX;

void * xeve_run_worker_thread(void * arg)
{
    /********************* main routine for thread pool worker thread *************************
    ********************** worker thread can remain in suspended or running state *************
    ********************* control the synchronization with help of thread context members *****/
    
    //member Initialization section
    THREAD_CTX * t_context = (THREAD_CTX *)arg;
    if (!t_context)
    {
        return 0; //error handling, more like a fail safe mechanism
    }

    while (1)
    {
        //worker thread loop
        //remains suspended/sleep waiting for an event

        //get the mutex and check the state
        pthread_mutex_lock(&t_context->c_section);
        while (t_context->t_status == THREAD_SUSPENDED)
        {
            //wait for the event
            pthread_cond_wait(&t_context->w_event, &t_context->c_section);
        }
        
        if (t_context->t_status == THREAD_TERMINATED)
        {
            t_context->task_result = THREAD_SUCCESS;
            pthread_mutex_unlock(&t_context->c_section);
            break;//exit the routine
        }

        t_context->t_status = THREAD_RUNNING;
        pthread_mutex_unlock(&t_context->c_section);

        //run the routine
        //worker thread state is running with entry function and arg set
        t_context->task(t_context->t_arg);

        //signal the thread waiting on the result
        pthread_mutex_lock(&t_context->c_section);
        t_context->t_status = THREAD_SUSPENDED;
        pthread_cond_signal(&t_context->r_event);
        pthread_mutex_unlock(&t_context->c_section);
    }

    return 0;
}


POOL_THREAD  xeve_create_worker_thread(THREAD_CONTROLLER * tc, int thread_id)
{
    if (!tc)
    {
        return NULL; //error management 
    }

    THREAD_CTX * thread_context = NULL;


    thread_context = (THREAD_CTX *)malloc(sizeof(THREAD_CTX));

    if (!thread_context)
    {
        return NULL; //error management, bad alloc
    }

    int result = 1;

    //intialize conditional variable and mutexes
    result = pthread_mutex_init(&thread_context->c_section, NULL);
    if (result)
    {
        goto TERROR; //error handling
    }
    result = pthread_cond_init(&thread_context->w_event, NULL);
    if (result)
    {
        goto TERROR;
    }
    result = pthread_cond_init(&thread_context->r_event, NULL);
    if (result)
    {
        goto TERROR;
    }

    //initialize the worker thread attribute and set the type to joinable
    result = pthread_attr_init(&thread_context->tAttribute);
    if (result)
    {
        goto TERROR;
    }

    result = pthread_attr_setdetachstate(&thread_context->tAttribute, PTHREAD_CREATE_JOINABLE);
    if (result)
    {
        goto TERROR;
    }

    thread_context->task = NULL;
    thread_context->t_arg = NULL;
    thread_context->t_status = THREAD_SUSPENDED;
    thread_context->task_result = THREAD_INVALID_STATE;
    thread_context->thread_id = thread_id;
    
    //create the worker thread
    result = pthread_create(&thread_context->t_handle, &thread_context->tAttribute, xeve_run_worker_thread, (void*)(thread_context));
    if (result)
    {
        goto TERROR;
    }

    //dinit the attribue
    pthread_attr_destroy(&thread_context->tAttribute);
    return (POOL_THREAD)thread_context;

TERROR:
    pthread_mutex_destroy(&thread_context->c_section);
    pthread_cond_destroy(&thread_context->w_event);
    pthread_cond_destroy(&thread_context->r_event);
    pthread_attr_destroy(&thread_context->tAttribute);
    free(thread_context);

    return NULL; //error handling, can't create a worker thread with proper initialization
}


THREAD_RESULT xeve_assign_task_thread(POOL_THREAD thread_id, THREAD_ENTRY entry, void * arg)
{
    //assign the task function and argument
    //worker thread may be in running state or suspended state
    //if worker thread is in suspended state, it can be waiting for first run or it has finished one task and is waiting again
    //if worker thread is in running state, it will come to waiting state
    //in any case, waiting on read event will always work

    THREAD_CTX * t_context = (THREAD_CTX*)(thread_id);
    if (!t_context)
    {
        return THREAD_INVALID_ARG;
    }

    //lock the mutex and wait on read event
    pthread_mutex_lock(&t_context->c_section);
    while (t_context->t_status == THREAD_RUNNING)
    {
        pthread_cond_wait(&t_context->r_event, &t_context->c_section);
    }

    //thread is in suspended state
    t_context->t_status = THREAD_RUNNING;
    t_context->task = entry;
    t_context->t_arg = arg;
    //signal the worker thread to wake up and run the task
    pthread_cond_signal(&t_context->w_event);
    pthread_mutex_unlock(&t_context->c_section); //release the lock

    return THREAD_SUCCESS;
}

THREAD_RESULT  xeve_retrieve_thread_result(POOL_THREAD thread_id, int * res)
{
    //whatever task has been assigned to worker thread
    //wait for it to finish get the result

    THREAD_CTX * t_context = (THREAD_CTX*)(thread_id);
    if (!t_context)
    {
        return THREAD_INVALID_ARG;
    }

    THREAD_RESULT  result = THREAD_SUCCESS;

    pthread_mutex_lock(&t_context->c_section);
    while (THREAD_RUNNING == t_context->t_status)
    {
        pthread_cond_wait(&t_context->r_event, &t_context->c_section);
    }

    result = t_context->task_result;
    pthread_mutex_unlock(&t_context->c_section);
    *res = result;
    return result;
}


THREAD_RESULT xeve_terminate_worker_thread(POOL_THREAD * thread_id)
{
    //handler to close the thread
    //close the thread handle
    //release all the resource
    // delete the thread context object

    THREAD_CTX * t_context = (THREAD_CTX*)(*thread_id);
    if (!t_context)
    {
        return THREAD_INVALID_ARG;
    }

    //The worker thread might be in suspended state or may be processing a task
    pthread_mutex_lock(&t_context->c_section);
    while (THREAD_RUNNING == t_context->t_status)
    {
        pthread_cond_wait(&t_context->r_event, &t_context->c_section);
    }
    
    t_context->t_status = THREAD_TERMINATED;
    pthread_cond_signal(&t_context->w_event);

    pthread_mutex_unlock(&t_context->c_section);

    //join the worker thread
    pthread_join(t_context->t_handle, NULL);

    //clean all the synchronization memebers
    pthread_mutex_destroy(&t_context->c_section);
    pthread_cond_destroy(&t_context->w_event);
    pthread_cond_destroy(&t_context->r_event);
    
    //delete the thread context memory
    free(t_context);
    (*thread_id) = NULL;
    return THREAD_SUCCESS;
}

SYNC_OBJ get_synchronized_object()
{

    THREAD_MUTEX * imutex = (THREAD_MUTEX *)malloc(sizeof(THREAD_MUTEX));
    if (0 == imutex)
    {
        return 0; //failure case
    }

    //intialize the mutex
    int result = pthread_mutex_init(&imutex->lmutex, NULL);
    if (result)
    {
        if (imutex)
        {
            free(imutex);
        }
        imutex = 0;
    }

    return imutex;
}

THREAD_RESULT release_synchornized_object(SYNC_OBJ * sobj)
{

    THREAD_MUTEX * imutex = (THREAD_MUTEX *)(*sobj);

    //delete the mutex
    pthread_mutex_destroy(&imutex->lmutex);

    //free the memory
    free(imutex);
    *sobj = NULL;

    return THREAD_SUCCESS;
}

int threadsafe_decrement(SYNC_OBJ sobj, volatile int * pcnt)
{
    THREAD_MUTEX * imutex = (THREAD_MUTEX*)(sobj);
    int temp = 0;

    //lock the mutex, decrement the count and release the mutex
    pthread_mutex_lock(&imutex->lmutex);
    temp = *pcnt;
    *pcnt = --temp;
    pthread_mutex_unlock(&imutex->lmutex);

    return temp;
}

#else
typedef struct _THREAD_CTX
{
    //synchronization members
    HANDLE t_handle; //worker thread handle
    HANDLE w_event; //worker thread waiting event handle
    HANDLE r_event; //signalling thread read event handle
    CRITICAL_SECTION c_section; //critical section for fast synchronization
    
    //member field to run  a task
    THREAD_ENTRY task;
    void * t_arg;
    THREAD_STATUS t_status;
    THREAD_RESULT task_result;
    int thread_id;

}THREAD_CTX;

typedef struct _THREAD_MUTEX
{
#if WINDOWS_MUTEX_SYNC
    HANDLE lmutex;
#else
    CRITICAL_SECTION c_section; //critical section for fast synchronization
#endif

}THREAD_MUTEX;

unsigned int __stdcall xeve_run_worker_thread(void * arg)
{
    /********************* main routine for thread pool worker thread *************************
    ********************** worker thread can remain in suspended or running state *************
    ********************* control the synchronization with help of thread context members *****/
    
    //member Initialization section
    THREAD_CTX * t_context = (THREAD_CTX *)arg;
    if (!t_context)
    {
        return 0; //error handling, more like a fail safe mechanism
    }

    while (1)
    {
        //worker thread loop
        //remains suspended/sleep waiting for an event
        WaitForSingleObject(t_context->w_event, INFINITE);

        //worker thread has received the event to wake up and perform operation
        EnterCriticalSection(&t_context->c_section);
        if (t_context->t_status == THREAD_TERMINATED)
        {
            //received signal to terminate
            t_context->task_result = THREAD_SUCCESS;
            LeaveCriticalSection(&t_context->c_section);
            break;
        }
        LeaveCriticalSection(&t_context->c_section);

        //worker thread state is running with entry function and arg set
        t_context->task(t_context->t_arg);

        //change the state to suspended/waiting
        EnterCriticalSection(&t_context->c_section);
        t_context->t_status = THREAD_SUSPENDED;
        t_context->task_result = THREAD_SUCCESS;
        LeaveCriticalSection(&t_context->c_section);

        //send an event to thread, waiting for it to finish it's task
        SetEvent(t_context->r_event);
    }

    return 0;
}

POOL_THREAD  xeve_create_worker_thread(THREAD_CONTROLLER * tc, int thread_id)
{
    if (!tc)
    {
        return NULL; //error management 
    }

    THREAD_CTX * thread_context = NULL;
    thread_context = (THREAD_CTX *)malloc(sizeof(THREAD_CTX));

    if (!thread_context)
    {
        return NULL; //error management, bad alloc
    }

    //create waiting event
    //create waiting event as automatic reset, only one thread can come out of waiting state
    //done intentionally ... signally happens from different thread and only worker thread should be able to respond
    thread_context->w_event = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (!thread_context->w_event)
    {
        goto TERROR; //error handling, can't create event handler
    }

    thread_context->r_event = CreateEvent(NULL, TRUE, TRUE, NULL); //read event is enabled by default
    if (!thread_context->r_event)
    {
        goto TERROR;
    }

    InitializeCriticalSection(&(thread_context->c_section)); //This section for fast data retrieval
    
    //intialize the state variables for the thread context object
    thread_context->task = NULL;
    thread_context->t_arg = NULL;
    thread_context->t_status = THREAD_SUSPENDED;
    thread_context->task_result = THREAD_INVALID_STATE;
    thread_context->thread_id = thread_id;

    thread_context->t_handle = (HANDLE)_beginthreadex(NULL, 0, xeve_run_worker_thread, (void *)thread_context,0, NULL); //create a thread store the handle and pass the handle to context
    if (!thread_context->t_handle)
    {
        goto TERROR;
    }

    //Everything created and intialized properly
    //return the created thread_context;
    return (POOL_THREAD)thread_context;

TERROR:    
    if (thread_context->w_event)
    {
        CloseHandle(thread_context->w_event);
    }
    if (thread_context->r_event)
    {
        CloseHandle(thread_context->r_event);
    }
    DeleteCriticalSection(&thread_context->c_section);
    if (thread_context)
    {
        free(thread_context);
    }

    return NULL; //error handling, can't create a worker thread with proper initialization
}

THREAD_RESULT xeve_assign_task_thread(POOL_THREAD thread_id, THREAD_ENTRY entry, void * arg)
{
    //assign the task function and argument
    //worker thread may be in running state or suspended state
    //if worker thread is in suspended state, it can be waiting for first run or it has finished one task and is waiting again
    //if worker thread is in running state, it will come to waiting state
    //in any case, waiting on read event will always work

    THREAD_CTX * t_context = (THREAD_CTX*)(thread_id);
    if (!t_context)
    {
        return THREAD_INVALID_ARG;
    }

    WaitForSingleObject(t_context->r_event, INFINITE);

    //worker thread is in waiting state
    EnterCriticalSection(&t_context->c_section);
    t_context->t_status = THREAD_RUNNING;
    t_context->task = entry;
    t_context->t_arg = arg;
    //signal the worker thread to wake up and run the task
    ResetEvent(t_context->r_event);
    SetEvent(t_context->w_event);
    LeaveCriticalSection(&t_context->c_section);

    return THREAD_SUCCESS;
}

THREAD_RESULT  xeve_retrieve_thread_result(POOL_THREAD thread_id, int * res)
{
    //whatever task has been assigned to worker thread
    //wait for it to finish get the result
    THREAD_CTX * t_context = (THREAD_CTX*)(thread_id);
    if (!t_context)
    {
        return THREAD_INVALID_ARG;
    }

    THREAD_RESULT  result = THREAD_SUCCESS;

    WaitForSingleObject(t_context->r_event, INFINITE);

    //worker thread has finished it's job and now it is in waiting state
    EnterCriticalSection(&t_context->c_section);
    result = t_context->task_result;
    LeaveCriticalSection(&t_context->c_section);
    
    *res = result;
    return result;
}

THREAD_RESULT xeve_terminate_worker_thread(POOL_THREAD * thread_id)
{
    //handler to close the thread
    //close the thread handle
    //release all the resource
    // delete the thread context object


    //the thread may be running or it is in suspended state
    //if it is in suspended state, read event will be active
    //if it is in running state, read event will be active after sometime

    THREAD_CTX * t_context = (THREAD_CTX*)(*thread_id);
    if (!t_context)
    {
        return THREAD_INVALID_ARG;
    }

    WaitForSingleObject(t_context->r_event, INFINITE);

    //worker thread is in waiting state
    EnterCriticalSection(&t_context->c_section);
    t_context->t_status = THREAD_TERMINATED;
    LeaveCriticalSection(&t_context->c_section);

    //signal the worker thread to wake up and run the task
    SetEvent(t_context->w_event);

    //wait for worker thread to finish it's routine
    WaitForSingleObject(t_context->t_handle, INFINITE);
    CloseHandle(t_context->t_handle); //freed all the resources for the thread

    CloseHandle(t_context->w_event);
    CloseHandle(t_context->r_event);
    DeleteCriticalSection(&t_context->c_section);

    //delete the thread context memory
    free(t_context);
    (*thread_id) = NULL;

    return THREAD_SUCCESS;
}

SYNC_OBJ get_synchronized_object()
{
    THREAD_MUTEX * imutex = (THREAD_MUTEX *)malloc(sizeof(THREAD_MUTEX));
    if (0 == imutex)
    {
        return 0; //failure case
    }

#if WINDOWS_MUTEX_SYNC
    //initialize the created mutex instance
    imutex->lmutex = CreateMutex(NULL,FALSE,NULL);
    if (0 == imutex->lmutex)
    {
        if (imutex)
        {
            free(imutex);
        }
        return 0;
    }
#else
    //initialize the critical section
    InitializeCriticalSection(&(imutex->c_section));
#endif
    return imutex;
}

THREAD_RESULT release_synchornized_object(SYNC_OBJ *  sobj)
{
    THREAD_MUTEX * imutex = (THREAD_MUTEX *)(*sobj);
#if WINDOWS_MUTEX_SYNC
    //release the mutex
    CloseHandle(imutex->lmutex);
#else
    //delete critical section
    DeleteCriticalSection(&imutex->c_section);
#endif

    //free the memory
    free(imutex);
    *sobj = NULL;

    return THREAD_SUCCESS;
}

int threadsafe_decrement(SYNC_OBJ sobj, volatile int * pcnt)
{
    THREAD_MUTEX * imutex = (THREAD_MUTEX*)(sobj);
    int temp = 0;

#if WINDOWS_MUTEX_SYNC
    //let's lock the mutex
    DWORD dw_wait_result = WaitForSingleObject(imutex->lmutex,INFINITE); //wait for infinite time

    switch (dw_wait_result)
    {
        // The thread got ownership of the mutex
    case WAIT_OBJECT_0:
        temp = *pcnt;
        *pcnt = --temp;
        // Release ownership of the mutex object
        ReleaseMutex(imutex->lmutex);
        break;
        // The thread got ownership of an abandoned mutex
        // The database is in an indeterminate state
    case WAIT_ABANDONED:
        temp = *pcnt;
        temp--;
        *pcnt = temp;
        break;
    }
#else
    EnterCriticalSection(&imutex->c_section);
    temp = *pcnt;
    *pcnt = --temp;
    LeaveCriticalSection(&imutex->c_section);
#endif
    return temp;
}
#endif

THREAD_RESULT init_thread_controller(THREAD_CONTROLLER * tc, int maxtask)
{
    //assign handles to threadcontroller object
    //handles for create, run, join and terminate will be given to controller  object

    tc->create = xeve_create_worker_thread;
    tc->run = xeve_assign_task_thread;
    tc->join = xeve_retrieve_thread_result;
    tc->release = xeve_terminate_worker_thread;
    tc->max_task_cnt = maxtask;

    return THREAD_SUCCESS;
}

THREAD_RESULT dinit_thread_controller(THREAD_CONTROLLER * tc)
{
    //reset all the handler to NULL
    tc->create = NULL;
    tc->run = NULL;
    tc->join = NULL;
    tc->release = NULL;
    tc->max_task_cnt = 0;

    return THREAD_SUCCESS;
}

int spinlock_wait(volatile int * addr, int val)
{
    int temp;

    while (1)
    {
        temp = *addr; //thread safe volatile read
        if (temp == val || temp == -1)
        {
            break;
        }
    }
    return temp;
}

void threadsafe_assign(volatile int * addr, int val)
{
    //thread safe volatile assign
    *addr = val;
}