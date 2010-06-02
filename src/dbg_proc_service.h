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

