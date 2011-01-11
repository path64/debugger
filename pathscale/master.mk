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
# accompanying license agreement. PathScale, Inc. 477 N. Mathilda Ave;
# Sunnyvale, CA 94085.


# Query the state of the machine with buildmeister
#####################################################################
hg_root :=       $(shell pwd)/..
hg_parent :=     $(hg_root)
bm :=            $(hg_root)/../buildmeister
hg_rev :=        .
hg_key :=        .
pkg_host :=      $(shell hostname -f)
pkg_date :=      $(shell date +'%Y-%m-%d %H:%M:%S %z')
pathdb_name :=   $(shell basename $(hg_root))

ifeq ($(findstring pending, $(hg_parent)),)
 compiler_dir := $(dir $(hg_root))ekopath
else
 compiler_dir := $(dir $(hg_root))ekopath-pending
endif

# This grabs the pathdb version number
pkg_ver =       $(shell $(bm)/bin/get-package-version $(compiler_dir))
pkg_rel =       $(shell $(bm)/bin/get-package-release $(pkg_ver))
distro_name :=  $(shell uname -r)
distro_type :=  $(shell uname)

# This grabs the compiler version number.
#include $(compiler_dir)/pathscale/defs.mk
compiler := $(CXX)

srcdir := $(hg_root)/src
topdir := $(hg_root)/pathscale

# Get the date of the build for perpetual licenses
psc_build_date := $(shell date '+"%F/%T"')

# Setup build commands and environment
#####################################################################
#CXX = pathCC
CXXFLAGS = -Wall -Werror -ansi -Wno-long-long 

CXXFLAGS += -DPSC_BUILD_DATE='$(psc_build_date)'

ifeq ($(ha_ha_ha),1)
 CXXFLAGS += -DNO_LICENSE
endif

ifneq ($(doomsday),)
 CXXFLAGS += -DTIMEBOMB=$(doomsday)
endif

ifeq ($(profile), 1)
 CXXFLAGS += -pg
endif

ifeq ($(debug), 1)
 CXXFLAGS += -g -O0
else
 CXXFLAGS += -O2
endif

ifeq ($(wipa),1)
 CXXFLAGS += -ipa
else
 CXXFLAGS += -fPIC
endif

ifeq ($(arch), x86_64)
 LIBDIR = /usr/lib64
endif
ifeq ($(arch), i386)
 CXXFLAGS += -m32
 LIBDIR = /usr/lib
endif
ifeq ($(arch), i686)
 CXXFLAGS += -m32 -march=i686
 LIBDIR = /usr/lib
endif

CXXCOMPILE := $(CXX) $(CXXFLAGS)
CXXLINK := $(CXX) $(CXXFLAGS)


# Handles for calling other rules
#####################################################################
all: pathdb doctool funclookup
clean : dbg-clean doc-clean


# Rules for updating the version info
#####################################################################
$(srcdir)/version_hg.cc : FORCE
	@echo 'const char* cset_rev = "$(hg_rev)";' > $@
	@echo 'const char* cset_key = "$(hg_key)";' >> $@
	@echo 'const char* build_root = "$(pkg_root)";' >>$@
	@echo 'const char* build_host = "$(pkg_host)";' >>$@
	@echo 'const char* build_date = "$(pkg_date)";' >>$@
	@echo 'const char* psc_full_ver = "$(compiler)";' >>$@

FORCE :

# Rules for building the debugger
#####################################################################
LNCURSES = -L$(LIBDIR)/ncurses -lncurses
DBGOBJS = dbg_thread_db.o dbg_proc_service.o pcm.o breakpoint.o \
process.o thread.o arch.o symtab.o expr.o dbg_dwarf.o dbg_elf.o \
dis.o opcodes.o cli.o pstream.o target.o readline.o dbg_except.o \
utils.o dbg_stl.o type_struct.o type_union.o type_base.o cli_param.o \
cli_history.o dwf_cfa.o bstream.o version_hg.o xml.o type_array.o \
type_class.o type_enum.o type_pointer.o type_nspace.o dwf_spec.o \
dwf_info.o dwf_locs.o dwf_names.o dwf_cunit.o file_info.o type_qual.o \
dwf_entry.o dwf_abbrv.o gen_loc.o junk_stream.o

DBGLIBS = libpathdb.a $(LNCURSES) -Wl,--export-dynamic -liberty
DBGTMPS = $(srcdir)/opcodes.cc $(srcdir)/dwf_spec.h $(srcdir)/dwf_spec.cc

pathdb: libpathdb.a driver.o $(hg_root)/etc/help.xml
	$(CXXLINK) -o pathdb driver.o $(DBGLIBS)

dbg-clean :
	rm -f $(DBGTMPS)

driver.o : $(srcdir)/driver.cc
	$(CXXCOMPILE) -o driver.o -c $(srcdir)/driver.cc

#### SNIP ####
trial: libpathdb.a trial.o 
	$(CXXLINK) -o trial trial.o $(DBGLIBS)

trial.o : $(srcdir)/trial.cc
	$(CXXCOMPILE) -o trial.o -c $(srcdir)/trial.cc
#### SNIP ####

$(DBGOBJS) : %.o : $(srcdir)/%.cc
	$(CXXCOMPILE) -o $@ -c $<

libpathdb.a : $(DBGOBJS)
	$(AR) ruv libpathdb.a $(DBGOBJS)

$(srcdir)/opcodes.cc: $(srcdir)/opcodes.txt $(srcdir)/mkopcodes.py
	$(srcdir)/mkopcodes.py $(srcdir)/opcodes.txt > $(srcdir)/opcodes.cc

$(srcdir)/dwf_spec.h : dwf_spec.txt mkdwfspec.py
	$(srcdir)/mkdwfspec.py $(srcdir)/dwf_spec.txt > $(srcdir)/dwf_spec.h

$(srcdir)/dwf_spec.cc : dwf_spec.txt mkdwfspec.py
	$(srcdir)/mkdwfspec.py -d $(srcdir)/dwf_spec.txt > $(srcdir)/dwf_spec.cc

$(hg_root)/etc/help.xml: $(srcdir)/help.xml
	mkdir -p $(hg_root)/etc
	rm -f $(hg_root)/etc/pathdb-help.xml
	cp $(srcdir)/help.xml $(hg_root)/etc/pathdb-help.xml


# Rules for building the doctools
#####################################################################
DOCOBJS = doctool.o doctext.o
DOCTMPS = $(srcdir)/doctext.cc

doctool : $(DOCOBJS)
	$(CXXLINK) -o doctool $(DOCOBJS)

doc-clean : FORCE
	rm -f $(DOCTMPS)

$(DOCOBJS) : %.o : $(srcdir)/%.cc
	$(CXXCOMPILE) -o $@ -c $<

$(srcdir)/doctext.cc : $(srcdir)/mkdoctext.py $(srcdir)/docinfo.xml
	$(srcdir)/mkdoctext.py $(srcdir)/docinfo.xml > $(srcdir)/doctext.cc


# Rules for building pathcore
#####################################################################
#pathcore: pathcore.o libpathdb.a 
#	$(CXXLINK) -o pathcore pathcore.o libpathdb.a $(DBGLIBS)


# Rules for building funclookup
#####################################################################
funclookup : funclookup.o libpathdb.a
	$(CXX) $(CXXFLAGS) -o $@ funclookup.o libpathdb.a $(DBGLIBS)


# RPM building rules (stolen from the subscription client rules)
#####################################################################

# Automatic determination of required libraries is broken on SLES.
ifeq ($(distro_type),suse)
autoreq := 0
else
autoreq := 1
endif

make-spec = \
        sed -e 's!%%pkg_ver%%!$(pkg_ver)!g' \
            -e 's!%%pkg_rel%%!$(pkg_rel)!g' \
            -e 's!%%hg_root%%!$(hg_root)!g' \
            -e 's!%%prefix%%!$(PSC_ROOT_PREFIX)!g' \
            -e 's!%%target%%!$(PSC_TARGET)!g' \
            -e 's!%%arch%%!$(arch)!g' \
            -e 's!%%psc_name%%!$(PSC_NAME_PREFIX)!g' \
            -e 's!%%distro_name%%!$(distro_name)!g' \
            -e 's!%%distro_type%%!$(distro_type)!g' \
	    -e 's!%%autoreq_enable%%!$(autoreq)!g'

pathdb-$(distro_name)-$(arch).spec: $(topdir)/pathdb.spec.in
	$(make-spec) < $< > $@

rpmbuild := $(shell which rpmbuild 2>/dev/null || which rpm)

ifneq ($(arch),x86_64)
RPMBUILD_FLAGS += --define "_lib lib"
endif

rpm qrpm : pathdb-$(distro_name)-$(arch).spec pathdb
	$(rpmbuild) $(RPMBUILD_FLAGS) --target $(arch) -bb $<


# Pull in the header file dependencies
#####################################################################
VPATH := $(srcdir)
include $(topdir)/depend.mk

