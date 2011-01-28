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

unsigned long Trace::read_text (pid_t pid, void *addr) {
    return read_data (pid, addr) ;
}

int Trace::write_text (pid_t pid, void *addr, unsigned long data) {
    return write_data (pid, addr, data) ;
}
