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

file: dbg_thread_db.h
created on: Fri Aug 13 11:02:28 PDT 2004
author: David Allison <dallison@pathscale.com>

*/

#ifndef dbg_thread_db_h_included
#define dbg_thread_db_h_included

#include "dbg_types.h"

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

void read_thread_registers (td_thragent_t *agent, void *threadhandle, unsigned char *regs) ;
void write_thread_registers (td_thragent_t *agent, void *threadhandle, unsigned char *regs) ;
void read_thread_fpregisters (td_thragent_t *agent, void *threadhandle, unsigned char *regs) ;
void read_thread_fpxregisters (td_thragent_t *agent, void *threadhandle, unsigned char *regs) ;
void write_thread_fpregisters (td_thragent_t *agent, void *threadhandle, unsigned char *regs) ;
void write_thread_fpxregisters (td_thragent_t *agent, void *threadhandle, unsigned char *regs) ;

void get_thread_info (td_thragent_t *agent, void *threadhandle, td_thrinfo_t &info) ;
void suspend_thread (td_thragent_t *agent, void *threadhandle) ;
void resume_thread (td_thragent_t *agent, void *threadhandle) ;

}

#endif
