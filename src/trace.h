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

file: trace.h
created on: Thu Sep  9 22:52:18 BST 2010
author: Jay Cornwall <jay@jcornwall.me.uk>

*/

#ifndef trace_h_included
#define trace_h_included

#include <sys/types.h>
#include "ptrace.h"

class Trace {
public:
    static int trace_me () ;
    static int attach (pid_t pid) ;
    static int detach (pid_t pid) ;
    static int kill (pid_t pid) ;
    static int cont (pid_t pid, int signal) ;
    static int single_step (pid_t pid) ;
    static int get_regs (pid_t pid, void *regs) ;
    static int get_fpregs (pid_t pid, void *fpregs) ;
    static int get_fpxregs (pid_t pid, void *fpxregs) ;
    static unsigned long get_dbgreg (pid_t pid, int idx) ;
    static int set_regs (pid_t pid, void *regs) ;
    static int set_fpregs (pid_t pid, void *fpregs) ;
    static int set_fpxregs (pid_t pid, void *fpxregs) ;
    static int set_dbgreg (pid_t pid, int idx, unsigned long val) ;
    static unsigned long read_data (pid_t pid, void *addr) ;
    static unsigned long read_text (pid_t pid, void *addr) ;
    static int write_data (pid_t pid, void *addr, unsigned long data) ;
    static int write_text (pid_t pid, void *addr, unsigned long data) ;
    static int set_options (pid_t pid, long opts) ;
    static int get_fork_pid (pid_t parent_pid, pid_t *fork_pid) ;
    static int get_thread_area (pid_t pid, int idx, void *dst) ;

#if defined (__FreeBSD__)
    static int suspend (void *tid) ;
#endif
};

#endif
