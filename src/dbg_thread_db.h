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

file: dbg_thread_db.h
created on: Fri Aug 13 11:02:28 PDT 2004
author: David Allison <dallison@pathscale.com>

*/

#ifndef dbg_thread_db_h_included
#define dbg_thread_db_h_included

#include "dbg_types.h"
#include "dbg_types.h"
#include "register_set.h"
#include "os.h"

#include <stdint.h>
#include <thread_db.h>


class ProcessController ;

namespace thread_db {

void load_thread_db (ProcessController *p) ;
void unload_thread_db() ;

void reinit (int pid) ;
void new_td_handle (int pid, td_thragent_t *&agent, Address &creation, Address &death) ;
void get_event_addresses (td_thragent_t *agent, Address &creation, Address &death) ;
void list_threads (const td_thragent_t *agent, std::vector<void*> &vec) ;
void get_event_message (const td_thragent_t *agent, int &event_number, void *&thread_handle, void *&data) ;
void enable_thread_events (td_thragent_t *agent, void *threadhandle, int v) ;

void read_thread_registers(td_thragent_t *agent, void *threadhandle, RegisterSet *regs, Architecture *);
void write_thread_registers(td_thragent_t *agent, void *threadhandle, RegisterSet *regs, Architecture *);
void read_thread_fpregisters(td_thragent_t *agent, void *threadhandle, RegisterSet *regs);
void read_thread_fpxregisters(td_thragent_t *agent, void *threadhandle, RegisterSet *regs);
void write_thread_fpregisters(td_thragent_t *agent, void *threadhandle, RegisterSet *regs) ;
void write_thread_fpxregisters(td_thragent_t *agent, void *threadhandle, RegisterSet *regs) ;

void get_thread_info (td_thragent_t *agent, void *threadhandle, td_thrinfo_t &info) ;
void suspend_thread (td_thragent_t *agent, void *threadhandle) ;
void resume_thread (td_thragent_t *agent, void *threadhandle) ;

}

#endif
