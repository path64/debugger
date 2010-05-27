# Copyright (c) 2004, 2005, 2006 PathScale, Inc.  All rights reserved.
# Unpublished -- rights reserved under the copyright laws of the United
# States. USE OF A COPYRIGHT NOTICE DOES NOT IMPLY PUBLICATION OR
# DISCLOSURE. THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND TRADE
# SECRETS OF PATHSCALE, INC. USE, DISCLOSURE, OR REPRODUCTION IS
# PROHIBITED WITHOUT THE PRIOR EXPRESS WRITTEN PERMISSION OF PATHSCALE,
# INC.

# U.S. Government Restricted Rights:
# The Software is a "commercial item," as that term is defined at 48
# C.F.R. 2.101 (OCT 1995), consisting of "commercial computer software"
# and "commercial computer software documentation," as such terms are used
# in 48 C.F.R. 12.212 (SEPT 1995).  Consistent with 48 C.F.R. 12.212 and
# 48 C.F.R. 227-7202-1 through 227-7202-4 (JUNE 1995), all U.S. Government
# End Users acquire the Software with only those rights set forth in the
# accompanying license agreement. PathScale, Inc. 2071 Stierlin Court,
# Suite 200, Mountain View, CA 94043.

# This is not a "real" Makefile - all the good stuff happens over in
# master.mk.
#
# This makefile is responsible only for making sure that everything
# happens in an architecture-specific subdirectory.
#


# Query the build environment
#####################################################################
ifeq ($(arch),)
   arch := $(shell uname -m | sed -e 's,i.86,i386,')
endif

hg_root := $(shell hg root)
topdir := $(hg_root)/pathscale
srcdir := $(hg_root)/src
objdir := $(topdir)/obj-$(arch)

submake := $(MAKE) -C $(objdir) -f $(topdir)/master.mk arch=$(arch)
depmake := cd $(srcdir) && pathCC -MM *.cc > $(topdir)/depend.mk

docs_dir := $(hg_root)/docs

# Hand down basic stuff like all and clean
#####################################################################
all rpm qrpm trial doctool : $(objdir) FORCE
	+$(submake) $@

rpm: FORCE

all_rpms : FORCE
	$(MAKE) arch=i386 rpm
	$(MAKE) arch=x86_64 rpm

$(objdir) :
	mkdir -p $@

deps : FORCE
	$(depmake)

clean : sub-clean obj-clean rpm-clean

sub-clean : $(objdir) FORCE
	+$(submake) clean

obj-clean : FORCE
	@for i in `ls . | grep '^obj-'`; do        \
	   echo "rm -rf $$i";                      \
	   rm -rf $$i;                             \
	done

rpm-clean : FORCE
	rm -rf rpms

FORCE :

