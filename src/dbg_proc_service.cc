/*

 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at 
 * http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at src/CDDL.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END

 * Copyright (c) 2004-2005 PathScale, Inc.  All rights reserved.
 * Use is subject to license terms.

file: dbg_proc_service.cc
created on: Fri Aug 13 11:07:37 PDT 2004
author: David Allison <dallison@pathscale.com>

*/


/* provide the additional functions that libthread_db needs
*/

#ifdef __linux__

#include "dbg_proc_service.h"
#include <sys/ptrace.h>
#include <dlfcn.h>
#include <iostream>
#include <string>

#include "pcm.h"

extern ProcessController *pcm ;

extern "C" {

/* lets define only the functions needed by libthread_db.so:
extern [dallison@tourmaline arch]$ nm /usr/lib/libthread_db.so | grep ps_
         U ps_getpid
         U ps_lgetfpregs
         U ps_lgetregs
         U ps_lsetfpregs
         U ps_lsetregs
         U ps_pdread
         U ps_pdwrite
         U ps_pglobal_lookup
*/

int ps_getpid (ps_prochandle *handle) {
    return handle->pid ;
}

ps_err_e ps_lgetregs (struct ps_prochandle *handle, int lid, prgregset_t *gregset) {
    int e = ptrace (PTRACE_GETREGS, lid, 0, gregset) ;
    if (e < 0) {
        //std::cout << "ps_lgetregs " << lid << "\n" ;
        throw Exception ("Failed to read registers");
    }
    return PS_OK ;
}

ps_err_e ps_lgetfpregs (struct ps_prochandle *handle, int lid, prfpregset_t *fpregset) {
    int e = ptrace (PTRACE_GETFPREGS, lid, 0, fpregset) ;
    if (e < 0) {
        throw Exception ("Failed to read fp registers") ;
    }
    return PS_OK ;
}

ps_err_e ps_lsetregs (struct ps_prochandle *handle, int lid, prgregset_t *gregset) {
    //std::cout << "ps_lsetregs " << lid << "\n" ;
    int e = ptrace (PTRACE_SETREGS, lid, 0, gregset) ;
    if (e < 0) {
        throw Exception ("Failed to write registers for thread %d", lid) ;
    }
    return PS_OK ;
}

ps_err_e ps_lsetfpregs (struct ps_prochandle *handle, int lid, prfpregset_t *fpregset) {
    int e = ptrace (PTRACE_SETFPREGS, lid, 0, fpregset) ;
    if (e < 0) {
        throw Exception ("Failed to write fp registers") ;
    }
    return PS_OK ;
}

ps_err_e ps_pdread(struct ps_prochandle *ph, psaddr_t  addr, void *buf, size_t size) {
    //std::cout << "ps_pdread " << ph->pid << " " << addr << " " << size << "\n" ;
    int nwords = size / 4 ;
    int remainder = size - nwords * 4 ;
    int *ibuf = (int*)buf ;
    char *iaddr = reinterpret_cast<char*>(addr) ;
    while (nwords > 0) {
       int v = ptrace (PTRACE_PEEKDATA, ph->pid, iaddr, 0) ;
       *ibuf++ = v ;
       iaddr += 4 ;
       nwords-- ;
    }
    // any left to read?
    if (remainder != 0) {               
        Address v = ptrace (PTRACE_PEEKDATA, ph->pid, iaddr, 0) ;
        memcpy (ibuf, &v, remainder) ;                  // XXX: won't work big endian
    }
    return PS_OK ;
}

// although on 64 bit machines, ptrace writes 64 bits, we will only write 32 bits at
// a time using read/modify/write.  This is because we don't know what the
// size of the target is here
ps_err_e ps_pdwrite(struct ps_prochandle *ph, psaddr_t addr, const void *buf, size_t size) {
    //std::cout << "ps_pdwrite " << ph->pid << " " << addr << " " << size << "\n" ;
    int nwords = size / 4 ;
    int remainder = size - nwords * 4 ;
    int *ibuf = (int*)buf ;
    char *iaddr = reinterpret_cast<char*>(addr) ;
    while (nwords > 0) {
        Address v = ptrace (PTRACE_PEEKDATA, ph->pid, iaddr, 0) ;
        v = (v & 0xffffffff00000000LL) | ((Address)*ibuf++ & 0xffffffffLL) ;
        ptrace (PTRACE_POKEDATA, ph->pid, iaddr, v) ;
        iaddr += 4 ;
        nwords-- ;
    }
    // any left to write?
    // if so, we need to read-modify-write the last word
    if (remainder != 0) {               
        Address v = ptrace (PTRACE_PEEKDATA, ph->pid, iaddr, 0) ;
        memcpy (&v, ibuf, remainder) ;                  // XXX: won't work big endian
        ptrace (PTRACE_POKEDATA, ph->pid, iaddr, v) ;
    }
    return PS_OK ;
}

ps_err_e ps_pglobal_lookup(struct ps_prochandle  *ph,  const char    *object_name,   const   char   *sym_name,   psaddr_t *sym_addr) {
    *sym_addr = reinterpret_cast<psaddr_t>(pcm->lookup_symbol (sym_name)) ;
    return *sym_addr != 0 ? PS_OK : PS_NOSYM ;
}

ps_err_e ps_get_thread_area(const struct ps_prochandle *ph, lwpid_t lwpid, int idx, void **base) {
    unsigned int desc[4];
#define PTRACE_GET_THREAD_AREA (__ptrace_request)25

    if  (ptrace (PTRACE_GET_THREAD_AREA, lwpid, reinterpret_cast<void *>(idx), (void*)&desc) < 0) {
        return PS_ERR;
    }

    *(int *)base = desc[1];
    return PS_OK;
}

}

#endif
