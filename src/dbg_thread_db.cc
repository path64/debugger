/*

   Copyright (c) 2004-2005 PathScale, Inc.  All rights reserved.
   PathDB is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation version 3

   PathDB is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with PathDB; see the file COPYING.  If not, write to the Free
   Software Foundation, 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

file: dbg_thread_db.cc
created on: Fri Aug 13 11:07:38 PDT 2004
author: David Allison <dallison@pathscale.com>

*/

#define __STDC_CONSTANT_MACROS
#include "dbg_thread_db.h"
#include "dbg_proc_service.h"
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/user.h>
                                                                                                                                                   
#include <dlfcn.h>


class ProcessController ;
ProcessController *pcm ;               // hack, the pcm object for symbol lookup

// thread_db stuff

// static function pointers for the thread_db functions

namespace thread_db {

struct Thread_db_interface {
    td_err_e (*td_ta_new)(struct ps_prochandle *__ps, td_thragent_t **__ta);
    td_err_e (*td_init)(void) ;
    td_err_e (*td_ta_thr_iter) (const td_thragent_t *__ta,
                                td_thr_iter_f *__callback, void *__cbdata_p,
                                td_thr_state_e __state, int __ti_pri,
                                sigset_t *__ti_sigmask_p,
                                unsigned int __ti_user_flags);
    td_err_e (*td_thr_get_info) (const td_thrhandle_t *__th,
                                 td_thrinfo_t *__infop);

    td_err_e (*td_thr_getfpregs) (const td_thrhandle_t *__th,
                                  prfpregset_t *__regset);
                                                                                                                     
    td_err_e (*td_thr_getgregs) (const td_thrhandle_t *__th,
                                 prgregset_t __gregs);
                                                                                                                     
    td_err_e (*td_thr_getxregs) (const td_thrhandle_t *__th, void *__xregs);
    td_err_e (*td_thr_setxregs) (const td_thrhandle_t *__th, const void *__xregs);
                                                                                                                     
    td_err_e (*td_thr_getxregsize) (const td_thrhandle_t *__th, int *__sizep);
                                                                                                                     
    td_err_e (*td_thr_setfpregs) (const td_thrhandle_t *__th,
                                  const prfpregset_t *__fpregs);
                                                                                                                     
    td_err_e (*td_thr_setgregs) (const td_thrhandle_t *__th,
                                 prgregset_t __gregs);
    td_err_e (*td_thr_event_enable) (const td_thrhandle_t *__th, int __event);
                                                                                                                     
    td_err_e (*td_thr_set_event) (const td_thrhandle_t *__th,
                                  td_thr_events_t *__event);
                                                                                                                     
    td_err_e (*td_thr_clear_event) (const td_thrhandle_t *__th,
                                    td_thr_events_t *__event);
                                                                                                                     
    td_err_e (*td_thr_event_getmsg) (const td_thrhandle_t *__th,
                                     td_event_msg_t *__msg);

    td_err_e (*td_ta_event_addr) (const td_thragent_t *__ta,
                                  td_event_e __event, td_notify_t *__ptr);
                                                                                                                     
    td_err_e (*td_ta_set_event) (const td_thragent_t *__ta,
                                 td_thr_events_t *__event);
                                                                                                                     
    td_err_e (*td_ta_clear_event) (const td_thragent_t *__ta,
                                   td_thr_events_t *__event);
                                                                                                                     
    td_err_e (*td_ta_event_getmsg) (const td_thragent_t *__ta,
                                    td_event_msg_t *__msg);
                                                                                                                     
    // suspend and resume
    td_err_e (*td_thr_dbsuspend) (const td_thrhandle_t *__th) ;
    td_err_e (*td_thr_dbresume) (const td_thrhandle_t *__th) ;

    struct ps_prochandle phandle ;
    void *handle ;
} ;

Thread_db_interface thread_db ;


static void *needsym (void *handle, const char *name) {
    void *sym = dlsym (handle, name) ;
    if (sym == NULL) {
       throw Exception ("Unable to find thread_db symbol") ;
    }
    return sym ;
}

// load the thread database
void load_thread_db (ProcessController *p) {
    pcm = p ;
    thread_db.handle = dlopen ("libthread_db.so.1", RTLD_NOW) ;
    if (thread_db.handle == NULL) {
        //std::cerr << dlerror() << "\n" ;
        throw Exception ("Unable to open thread debug library") ;
    }
    void *handle = thread_db.handle ;
    thread_db.td_ta_new = (td_err_e (*)(struct ps_prochandle*, td_thragent_t **))needsym (handle, "td_ta_new") ;
    thread_db.td_init = (td_err_e (*)())needsym (handle, "td_init") ;
    thread_db.td_ta_thr_iter = (td_err_e (*)(const td_thragent_t *, td_thr_iter_f *, void *, td_thr_state_e, int,
                                sigset_t *, unsigned int))needsym (handle, "td_ta_thr_iter") ;
    thread_db.td_thr_get_info = (td_err_e (*)(const td_thrhandle_t *, td_thrinfo_t *))needsym (handle, "td_thr_get_info") ;
    thread_db.td_thr_getfpregs = (td_err_e (*)(const td_thrhandle_t *, prfpregset_t *))needsym (handle, "td_thr_getfpregs") ;
    thread_db.td_thr_getgregs = (td_err_e (*)(const td_thrhandle_t *, prgregset_t))needsym (handle, "td_thr_getgregs") ;
    thread_db.td_thr_getxregs = (td_err_e (*)(const td_thrhandle_t *, void *))needsym (handle, "td_thr_getxregs") ;
    thread_db.td_thr_setxregs = (td_err_e (*)(const td_thrhandle_t *, const void *))needsym (handle, "td_thr_setxregs") ;
    thread_db.td_thr_getxregsize = (td_err_e (*)(const td_thrhandle_t *, int *))needsym (handle, "td_thr_getxregsize") ;
    thread_db.td_thr_setfpregs = (td_err_e (*)(const td_thrhandle_t *, const prfpregset_t *))needsym (handle, "td_thr_setfpregs") ;
    thread_db.td_thr_setgregs = (td_err_e (*)(const td_thrhandle_t *, prgregset_t ))needsym (handle, "td_thr_setgregs") ;
    thread_db.td_ta_event_addr = (td_err_e (*)(const td_thragent_t *, td_event_e, td_notify_t *))needsym (handle, "td_ta_event_addr") ;
    thread_db.td_ta_set_event = (td_err_e (*)(const td_thragent_t *, td_thr_events_t *))needsym (handle, "td_ta_set_event") ;
    thread_db.td_ta_clear_event = (td_err_e (*)(const td_thragent_t *, td_thr_events_t *))needsym (handle, "td_ta_clear_event") ;
    thread_db.td_ta_event_getmsg = (td_err_e (*)(const td_thragent_t *, td_event_msg_t *))needsym (handle, "td_ta_event_getmsg") ;

    thread_db.td_thr_event_enable = (td_err_e (*)(const td_thrhandle_t *, int ))needsym (handle, "td_thr_event_enable") ;
    thread_db.td_thr_set_event = (td_err_e (*)(const td_thrhandle_t *, td_thr_events_t *))needsym (handle, "td_thr_set_event") ;
    thread_db.td_thr_clear_event = (td_err_e (*)(const td_thrhandle_t *, td_thr_events_t *))needsym (handle, "td_thr_clear_event") ;
    thread_db.td_thr_event_getmsg = (td_err_e (*)(const td_thrhandle_t *, td_event_msg_t *))needsym (handle, "td_thr_event_getmsg") ;

    thread_db.td_thr_dbsuspend = (td_err_e (*)(const td_thrhandle_t *))needsym (handle, "td_thr_dbsuspend") ;
    thread_db.td_thr_dbresume = (td_err_e (*)(const td_thrhandle_t *))needsym (handle, "td_thr_dbresume") ;

    if (thread_db.td_init() != TD_OK) {
        throw Exception ("Failed to initialize thread library") ;
    }
    
}

void unload_thread_db() {
    int e = dlclose (thread_db.handle) ;
    if (e != 0) {
        perror ("dlclose") ;
    }
}

// the function returns 3 things:
//   the agent
//   the address of the creation breakpoint
//   the address for the death breakpoint

// allocate and return a new debug library handle
void new_td_handle (int pid, td_thragent_t *&agent, Address &creation, Address &death) {
    thread_db.phandle.pid = pid ;
    td_err_e e = thread_db.td_ta_new(&thread_db.phandle, &agent) ;
    if (e != TD_OK) {
        //std::cout << "error: " << e << "\n" ;
        throw Exception ("Failed to allocate thread debug handle");
    }

    // enable the thread creation and death events

    td_thr_events_t events ;
    td_event_emptyset (&events) ;

    td_event_addset (&events, TD_CREATE) ;
    //td_event_addset (&events, TD_DEATH) ;                       // gdb says this is broken in glibc 2.1.3
    e = thread_db.td_ta_set_event (agent, &events) ;
    if (e != TD_OK) {
        throw Exception ("Failed to set global event mask for thread_db") ;
    }

    get_event_addresses (agent, creation, death) ;
}

void reinit (int pid) {
    thread_db.phandle.pid = pid ;
}

void get_event_addresses (td_thragent_t *agent, Address &creation, Address &death) {
    td_notify_t bpaddr ;                        // address for event breakpoint
    // get the creation event address
    td_err_e e = thread_db.td_ta_event_addr (agent, TD_CREATE, &bpaddr) ;
    if (e != TD_OK) {
        throw Exception ("Failed to enable thread death event") ;
    }
    creation = reinterpret_cast<Address>(bpaddr.u.bptaddr) ;

    // get the death event address
    e = thread_db.td_ta_event_addr (agent, TD_DEATH, &bpaddr) ;
    if (e != TD_OK) {
        throw Exception ("Failed to enable thread creation event") ;
    }
    death = reinterpret_cast<Address>(bpaddr.u.bptaddr) ;
}


// get thread info and put handle into vector passed in 'data'
static int thread_iterator_callback (const td_thrhandle_t *th_p, void *data) {
    std::vector<void*> *vec = (std::vector<void*> *)data ;
    td_thrinfo_t info ;
    td_err_e e = thread_db.td_thr_get_info (th_p, &info) ;
    if (e != TD_OK) {
        throw Exception ("Failed to read info for thread") ;
    }
    if (info.ti_state == TD_THR_ZOMBIE) {
        return 0 ;
    }
    vec->push_back ((void*)th_p->th_unique) ;                   // XXX: hack: it's supposed to be opaque
    return 0 ;
}

void list_threads (const td_thragent_t *agent, std::vector<void*> &vec) {
    td_err_e e = thread_db.td_ta_thr_iter (agent, thread_iterator_callback, &vec, TD_THR_ANY_STATE,
                                                TD_THR_LOWEST_PRIORITY, TD_SIGNO_MASK, TD_THR_ANY_USER_FLAGS) ;
    if (e != TD_OK) {
       throw Exception ("failed to list threads") ;
    }
}

// parameters:
// 0: thread agent
// 1: reference to event number
// 2: reference to thread handle 
// 3: reference to event specific data

void get_event_message (const td_thragent_t *agent, int &event_number, void *&thread_handle, void *&data) {
    td_event_msg_t msg ;
    td_err_e e = thread_db.td_ta_event_getmsg (agent, &msg) ;
    if (e != TD_OK) {
       throw Exception ("Unable to get event message") ;
    }
    event_number = msg.event ;
    thread_handle = (void *)msg.th_p->th_unique ;
    //std::cout << "thread handle = " << (void*)msg.th_p->th_unique << "\n" ;
    data = (void*)msg.msg.data ;
}

void get_thread_info (td_thragent_t *agent, void *threadhandle, td_thrinfo_t &info) {
    td_thrhandle_t handle ;
    handle.th_ta_p = agent ;
    handle.th_unique = threadhandle ;
    td_err_e e = thread_db.td_thr_get_info (&handle, &info) ;
    if (e != TD_OK) {
       throw Exception ("Unable to read thread info") ;
    }
}

void enable_thread_events (td_thragent_t *agent, void *threadhandle, int v) {
    td_thrhandle_t handle ;
    handle.th_ta_p = agent ;
    handle.th_unique = threadhandle ;
    td_err_e e = thread_db.td_thr_event_enable (&handle, v) ;
    if (e != TD_OK) {
        throw Exception ("Unable to enable events for thread") ;
    }
}

void suspend_thread (td_thragent_t *agent, void *threadhandle) {
    td_thrhandle_t handle ;
    handle.th_ta_p = agent ;
    handle.th_unique = threadhandle ;
    td_err_e e = thread_db.td_thr_dbsuspend (&handle) ;
    if (e != TD_OK) {
        printf ("suspend failed %d\n", e) ;
        throw Exception ("Unable to suspend thread") ;
    }
}


void resume_thread (td_thragent_t *agent, void *threadhandle) {
    td_thrhandle_t handle ;
    handle.th_ta_p = agent ;
    handle.th_unique = threadhandle ;
    td_err_e e = thread_db.td_thr_dbresume (&handle) ;
    if (e != TD_OK) {
        printf ("resume failed %d\n", e) ;
        throw Exception ("Unable to resume thread") ;
    }
}


// the type elf_greg_t is the native register set type (the same as ptrace uses)
void read_thread_registers (td_thragent_t *agent, void *threadhandle, unsigned char *regs) {
    td_thrhandle_t handle ;
    handle.th_ta_p = agent ;
    handle.th_unique = threadhandle ;
    td_err_e e = thread_db.td_thr_getgregs (&handle, (elf_greg_t*)regs) ;
    if (e != TD_OK) {
        throw Exception ("Unable to read thread registers") ;
    }
}

// the type elf_greg_t is the native register set type (the same as ptrace uses)
void write_thread_registers (td_thragent_t *agent, void *threadhandle, unsigned char *regs) {
    td_thrhandle_t handle ;
    handle.th_ta_p = agent ;
    handle.th_unique = threadhandle ;
    td_err_e e = thread_db.td_thr_setgregs (&handle, (elf_greg_t*)regs) ;
    if (e != TD_OK) {
        throw Exception ("Unable to write thread registers") ;
    }
}

void read_thread_fpregisters (td_thragent_t *agent, void *threadhandle, unsigned char *regs) {
    td_thrhandle_t handle ;
    handle.th_ta_p = agent ;
    handle.th_unique = threadhandle ;
    td_err_e e = thread_db.td_thr_getfpregs (&handle, (prfpregset_t*)regs) ;
    if (e != TD_OK) {
        throw Exception ("Unable to read thread floating point registers") ;
    }
}

void write_thread_fpregisters (td_thragent_t *agent, void *threadhandle, unsigned char *regs) {
    td_thrhandle_t handle ;
    handle.th_ta_p = agent ;
    handle.th_unique = threadhandle ;
    td_err_e e = thread_db.td_thr_setfpregs (&handle, (prfpregset_t*)regs) ;
    if (e != TD_OK) {
        throw Exception ("Unable to write thread floating point registers") ;
    }
}


void read_thread_fpxregisters (td_thragent_t *agent, void *threadhandle, unsigned char *regs) {
    td_thrhandle_t handle ;
    handle.th_ta_p = agent ;
    handle.th_unique = threadhandle ;
    // td_err_e e = thread_db.td_thr_getxregs (&handle, (void*)regs) ;
    // ignore errors
}

void write_thread_fpxregisters (td_thragent_t *agent, void *threadhandle, unsigned char *regs) {
    td_thrhandle_t handle ;
    handle.th_ta_p = agent ;
    handle.th_unique = threadhandle ;
    // td_err_e e = thread_db.td_thr_setxregs (&handle, (void*)regs) ;
    // ignore errors
}


}


