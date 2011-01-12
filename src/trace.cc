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

file: trace.cc
created on: Thu Sep  9 22:52:18 BST 2010
author: Jay Cornwall <jay@jcornwall.me.uk>

*/

#include "trace.h"

#if defined (__linux__)
#include <endian.h>
#include <sys/user.h>
#elif defined (__FreeBSD__)
#include <sys/endian.h>
#include <machine/reg.h>
#include <machine/sysarch.h>
#endif

/* find the offset of X into struct user (from sys/user.h) */
#define STRUCT_USER_OFFSET(X) (&(((struct user*)0)->X))

int Trace::trace_me () {
    return (int)ptrace (PT_TRACE_ME, 0, 0, 0) ;
}

int Trace::attach (pid_t pid) {
    return (int)ptrace (PT_ATTACH, pid, 0, 0) ;
}

int Trace::detach (pid_t pid) {
    return (int)ptrace (PT_DETACH, pid, 0, 0) ;
}

int Trace::kill (pid_t pid) {
    return (int)ptrace (PT_KILL, pid, 0, 0) ;
}

int Trace::cont (pid_t pid, int signal) {
    return (int)ptrace (PT_CONTINUE, pid, (caddr_t)1, signal) ;
}

int Trace::single_step (pid_t pid) {
    return (int)ptrace (PT_STEP, pid, caddr_t (1), 0) ;
}

int Trace::get_regs (pid_t pid, void *regs) {
#if defined (__linux__)
    return int (ptrace (PTRACE_GETREGS, pid, 0, regs)) ;
#elif defined (__FreeBSD__)
    return ptrace (PT_GETREGS, pid, (caddr_t)regs, 0) ;
#else
#error Trace::get_regs unimplemented on this platform
#endif
}

int Trace::get_fpregs (pid_t pid, void *fpregs) {
#if defined (__linux__)
    return int (ptrace (PTRACE_GETFPREGS, pid, 0, fpregs)) ;
#elif defined (__FreeBSD__)
    return ptrace (PT_GETFPREGS, pid, (caddr_t)fpregs, 0) ;
#else
#error Trace::get_fpregs unimplemented on this platform
#endif
}

int Trace::get_fpxregs (pid_t pid, void *fpxregs) {
#if defined (__linux__)
    return int (ptrace (PTRACE_GETFPXREGS, pid, 0, fpxregs)) ;
#elif defined (__FreeBSD__)
    return get_fpregs (pid, fpxregs) ; // XXX: this right?
#else
#error Trace::get_fpxregs unimplemented on this platform
#endif
}

unsigned long Trace::get_dbgreg (pid_t pid, int idx) {
#if defined (__linux__)
    /* find offset and access struct user */
    void *addr = STRUCT_USER_OFFSET (u_debugreg[idx]) ;
    return ptrace (PTRACE_PEEKUSER, pid, addr, 0) ;
#elif defined (__FreeBSD__)
    struct dbreg regs ;
    ptrace (PT_GETDBREGS, pid, (caddr_t)&regs, 0) ;
    return regs.dr[idx] ;
#else
#error Trace::get_dbgreg unimplemented on this platform
#endif
}

int Trace::set_regs (pid_t pid, void *regs) {
#if defined (__linux__)
    return int (ptrace (PTRACE_SETREGS, pid, 0, regs)) ;
#elif defined (__FreeBSD__)
    return ptrace (PT_SETREGS, pid, (caddr_t)regs, 0) ;
#else
#error Trace::set_regs unimplemented on this platform
#endif
}

int Trace::set_fpregs (pid_t pid, void *fpregs) {
#if defined (__linux__)
    return int (ptrace (PTRACE_SETFPREGS, pid, 0, fpregs)) ;
#elif defined (__FreeBSD__)
    return ptrace (PT_SETFPREGS, pid, (caddr_t)fpregs, 0) ;
#else
#error Trace::set_fpregs unimplemented on this platform
#endif
}

int Trace::set_fpxregs (pid_t pid, void *fpxregs) {
#if defined (__linux__)
    return int (ptrace (PTRACE_SETFPXREGS, pid, 0, fpxregs)) ;
#elif defined (__FreeBSD__)
    return set_fpregs (pid, fpxregs) ; // XXX: this right?
#else
#error Trace::set_fpxregs unimplemented on this platform
#endif
}

int Trace::set_dbgreg (pid_t pid, int idx, unsigned long val) {
#if defined (__linux__)
    /* find offset and access struct user */
    void *addr = STRUCT_USER_OFFSET (u_debugreg[idx]) ;
    return ptrace (PTRACE_POKEUSER, pid, addr, val) ;
#elif defined (__FreeBSD__)
    struct dbreg regs ;
    ptrace (PT_GETDBREGS, pid, (caddr_t)&regs, 0) ;
    regs.dr[idx] = val ;
    return ptrace (PT_SETDBREGS, pid, (caddr_t)&regs, 0) ;
#else
#error Trace::set_dbgreg unimplemented on this platform
#endif
}

unsigned long Trace::read_data (pid_t pid, void *addr) {
#if defined (__linux__)
    return ptrace (PTRACE_PEEKDATA, pid, addr, 0) ;
#elif defined (__FreeBSD__)
    unsigned long read_word = 0 ;
    int n_reads = int(sizeof (long) / sizeof (int)) ;

    for (int i = 0 ; i < n_reads ; i++) {
#if BYTE_ORDER == LITTLE_ENDIAN
        void *int_addr = (int *)addr + i ;
#else
        void *int_addr = (int *)addr + (n_reads - 1 - i) ;
#endif
        unsigned int read_int = (unsigned int)ptrace (PT_READ_D, pid, (caddr_t)int_addr, 0) ;
        read_word |= ((unsigned long)read_int << (i * 32)) ;
    }

    return read_word ;
#else
#error Trace::read_data unimplemented on this platform
#endif
}

unsigned long Trace::read_text (pid_t pid, void *addr) {
    return read_data (pid, addr) ;
}

int Trace::write_data (pid_t pid, void *addr, unsigned long data) {
#if defined (__linux__)
    return ptrace (PTRACE_POKEDATA, pid, addr, (void *)data) ;
#elif defined (__FreeBSD__)
    int ret_val = 0 ;
    int n_writes = int(sizeof (long) / sizeof (int)) ;

    for (int i = 0 ; i < n_writes ; i++) {
#if BYTE_ORDER == LITTLE_ENDIAN
        void *int_addr = (int *)addr + i ;
#else
        void *int_addr = (int *)addr + (n_writes - 1 - i) ;
#endif
        unsigned int write_int = (unsigned int)(data >> (i * 32)) ;
        ret_val |= ptrace (PT_WRITE_D, pid, (caddr_t)int_addr, write_int) ;
    }

    return ret_val ;
#else
#error Trace::write_data unimplemented on this platform
#endif
}

int Trace::write_text (pid_t pid, void *addr, unsigned long data) {
    return write_data (pid, addr, data) ;
}

int Trace::set_options (pid_t pid, long opts) {
#if defined (__linux__)
    return ptrace(PTRACE_SETOPTIONS, pid, (void*)0, opts) ;
#elif defined (__FreeBSD__)
    return -1 ;
#else
#error Trace::set_options unimplemented on this platform
#endif
}

int Trace::get_fork_pid (pid_t parent_pid, pid_t *fork_pid) {
#if defined (__linux__)
	long opt = 0;
	int ret = ptrace (PTRACE_GETEVENTMSG, parent_pid, (void*)0, &opt);
	*fork_pid = (pid_t)opt;
	return  ret;
#elif defined (__FreeBSD__)
    return -1 ;
#else
#error Trace::get_fork_pid unimplemented on this platform
#endif
}

int Trace::get_thread_area (pid_t pid, int idx, void *dst) {
#if defined (__linux__)
#define PTRACE_GET_THREAD_AREA (__ptrace_request)25
    return ptrace (PTRACE_GET_THREAD_AREA, pid, (void *)idx, dst) ;
#else
    return -1 ;
#endif
}

#if defined (__FreeBSD__)
int Trace::suspend (void *tid) {
    return (int)ptrace (PT_SUSPEND, (intptr_t)tid, 0, 0) ;
}
#endif
