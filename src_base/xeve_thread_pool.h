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

#ifndef _XEVE_THREAD_POOL_
#define _XEVE_THREAD_POOL_

typedef void* POOL_THREAD;
typedef int (*THREAD_ENTRY) (void * arg);
typedef struct _THREAD_CONTROLLER THREAD_CONTROLLER;
typedef void* SYNC_OBJ;

/*****************************  Salient points  ****************************************************
******************************  Thread Controller object will create, run and destroy***************
******************************  threads. Thread Controller has to be initialised *******************
******************************  before invoking handler functions.  Thread controller***************
******************************  should be de-initialized to release handler functions***************
****************************************************************************************************/

typedef enum _THREAD_RESULT
{
    THREAD_SUCCESS = 0,
    THREAD_OUT_OF_MEMORY,
    THREAD_INVALID_ARG,
    THREAD_INVALID_STATE,
    THREAD_UNKNOWN_ERROR

}THREAD_RESULT;

typedef enum _THREAD_STATUS
{
    THREAD_SUSPENDED = 0,
    THREAD_RUNNING,
    THREAD_TERMINATED

}THREAD_STATUS;

struct _THREAD_CONTROLLER
{
    //Handler function to create requested thread, thread created is in suspended state
    POOL_THREAD (*create)(THREAD_CONTROLLER * tc, int thread_id);
    //Handler function to wake up suspended thread and assign task to complete
    THREAD_RESULT (*run) (POOL_THREAD thread_id, THREAD_ENTRY entry, void * arg);
    //Handler function to get result from the task assigned to the thread in consideration
    THREAD_RESULT (*join)(POOL_THREAD thread_id, int * res);
    //Handler function to terminate a thread in consideration
    THREAD_RESULT (*release)(POOL_THREAD *thread_id);
    //handle for mask number of allowed thread
    int max_task_cnt;
};

THREAD_RESULT init_thread_controller(THREAD_CONTROLLER * tc, int maxtask);
THREAD_RESULT dinit_thread_controller(THREAD_CONTROLLER * tc);

/*** Create a synchronization object which can be used to control race conditions across threads, synchronization object will be on encoding context*****/

SYNC_OBJ get_synchronized_object();
THREAD_RESULT release_synchornized_object(SYNC_OBJ * sobj); //sync object will be deleted
int spinlock_wait(volatile int * addr, int val);
void threadsafe_assign(volatile int * addr, int val);
int threadsafe_decrement(SYNC_OBJ sobj, volatile int * pcnt);

#endif

