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

file: dbg_proc_service.h
created on: Fri Aug 13 11:02:27 PDT 2004
author: David Allison <dallison@pathscale.com>

*/

/* this has to be provided by the user of libthread_db.so.  This
implementation  uses ptrace.

The types and names come from the Solaris man pages because that is what
is required by libthread_db.so (it originally came from Solaris).
*/

#ifndef _proc_service_h_included
#define _proc_service_h_included

#include <sys/procfs.h>

extern "C" {

typedef enum {
    PS_OK,
    PS_ERR,
    PS_BADPID,
    PS_BADLID,
    PS_BADADDR,
    PS_NOSYM,
    PS_NOFREGS
} ps_err_e ;


struct ps_prochandle {
    int pid ;
} ;

int ps_getpid (struct ps_prochandle *handle) ;
ps_err_e ps_lgetregs (struct ps_prochandle *handle, int lid, prgregset_t *gregset) ;
ps_err_e ps_lgetfpregs (struct ps_prochandle *handle, int lid, prfpregset_t *fpegset) ;
ps_err_e ps_lsetregs (struct ps_prochandle *handle, int lid, prgregset_t *gregset) ;
ps_err_e ps_lsetfpregs (struct ps_prochandle *handle, int lid, prfpregset_t *fpegset) ;
ps_err_e ps_pdread(struct ps_prochandle *ph, psaddr_t  addr, void *buf, size_t size) ;
ps_err_e ps_pdwrite(struct ps_prochandle *ph, psaddr_t addr, const void *buf, size_t size) ;
ps_err_e ps_pglobal_lookup(struct ps_prochandle  *ph,  const char    *object_name,   const   char   *sym_name,   psaddr_t *sym_addr) ;
}

#endif

