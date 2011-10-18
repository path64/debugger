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

file: dbg_elf.cc
created on: Fri Aug 13 11:07:34 PDT 2004
author: David Allison <dallison@pathscale.com>

*/

#include "dbg_elf.h"
#include <fstream>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include "utils.h"
#include "target.h"

ProgramSegment::ProgramSegment (ELF *elf, Address baseaddr)
   : elf(elf), type(0), flags(0), offset(0), 
     vaddr(0), paddr(0), filesz(0), memsz(0), align(0), baseaddr(baseaddr)
 {
}

ProgramSegment::~ProgramSegment() {
}

void ProgramSegment::read (std::istream &s) {
    if (elf->fileclass == ELFCLASS64) {
        type = elf->read_word4(s) ;
        flags = elf->read_word4(s) ;
        offset = elf->read_offset(s) ;
        vaddr = elf->read_address(s) + baseaddr ;
        paddr = elf->read_address(s) ;
        filesz = elf->read_xword (s) ;
        memsz = elf->read_xword (s) ;
        align = elf->read_xword (s) ;
    } else {
        type = elf->read_word4(s) ;
        offset = elf->read_offset(s) ;
        vaddr = elf->read_address(s) + baseaddr ;
        paddr = elf->read_address(s) ;
        filesz = elf->read_xword (s) ;
        memsz = elf->read_xword (s) ;
        flags = elf->read_word4(s) ;
        align = elf->read_xword (s)            ;
    }

    //printf ("addr: 0x%llx, filesz: %lld, memsize: %lld\n", vaddr, filesz, memsz) ;
}

void *ProgramSegment::map (int fd) {

    /* check for void size combos */
    if ( (filesz == 0 && memsz != 0) || // no space
         (filesz == 0 && memsz != 0) || // all zeroes
         (filesz != 0 && memsz == 0)    // what ?
       ) return NULL ;

    /* XXX: standard actually allows far more than
       what we are checking for, need to expand */

    /* check for invalid sizes */
    if (filesz != memsz) {
       throw Exception ("Invalid size for program segment");
    }

    /* find the current file position */
    off_t cur = lseek(fd, offset, SEEK_SET);
    if (cur == -1) return NULL;

    /* find the end file position */
    off_t end = lseek(fd, 0, SEEK_END);
    if (end == -1) return NULL;

    /* return to the current position */
    if (lseek(fd, cur, SEEK_SET)==-1) return NULL; 

    /* check the file size */
    if (end - cur + 1 < filesz) {
       printf("Warning: ELF record size exceeds file size\n");
       
       /* oh hell, just punt */
       filesz = end - cur + 1;
       memsz = filesz;
    }

    /* actually mmap the data segment */
    void *ret = mmap (0, filesz, PROT_READ, MAP_PRIVATE, fd, offset);
    if (ret == MAP_FAILED) return NULL;

    /* return memory address */
    return ret;
}


BVector ProgramSegment::get_contents(std::istream & stream) {
    stream.seekg (elf->mainoffset + offset, std::ios_base::beg) ;

    /* XXX: this leaks but we should actually mmap it */ 

    byte* addr = (byte*)malloc(filesz);
    stream.read((char*)addr, filesz);
    return BVector(addr, filesz);
}


Section::Section (ELF *elf, int index)
    : elf(elf), index(index),
    type(0),
    flags(0),
    addr(0),
    offset(0),
    size(0),
    link(0),
    info(0),
    addralign(0),
    entsize(0) {
}

Section::~Section() {
}

std::string Section::get_name() {
        return name ;
}

Address Section::get_addr() {
        return addr ;
}

int Section::get_index() {
        return index ;
}

int Section::read(std::istream & stream) {
        nameindex = elf->read_word4 (stream) ;
        type = elf->read_word4 (stream) ;
        flags = elf->read_xword (stream) ;
        addr = elf->read_address (stream) ;
        offset = elf->read_offset (stream) ;
        size = elf->read_xword (stream) ;
        link = elf->read_word4 (stream) ;
        info = elf->read_word4 (stream) ;
        addralign = elf->read_xword (stream) ;
        entsize = elf->read_xword (stream) ;
        return 0 ;
}

void Section::print() {
        std::cout << "    name = "  <<  name << '\n' ;
        std::cout << "        nameindex = "  <<  nameindex << '\n' ;
        std::cout << "        type = "  <<  type << '\n' ;
        std::cout << "        flags = "  <<  flags << '\n' ;
        std::cout << "        addr = "  <<  addr << '\n' ;
        std::cout << "        offset = "  <<  offset << '\n' ;
        std::cout << "        size = "  <<  size << '\n' ;
        std::cout << "        link = "  <<  link << '\n' ;
        std::cout << "        info = "  <<  info << '\n' ;
        std::cout << "        addralign = "  <<  addralign << '\n' ;
        std::cout << "        entsize = "  <<  entsize << '\n' ;
}

int Section::get_offset() {
        return offset ;
}

int Section::get_size() {
        return size ;
}

void Section::set_name(std::istream & stream, Section *nametable) {
        name = nametable->read_string (stream, nameindex) ;
}

std::string Section::read_string(std::istream & stream, int stroffset) {
        stream.seekg (elf->mainoffset + offset + stroffset, std::ios_base::beg) ;
        std::string str = "" ;
        while (!stream.eof()) {
            byte ch = stream.get() ;
            if (ch != 0) {
                str += ch ;
            } else {
                break ;
            }
        }
        return str  ;
}

int32_t Section::read_word4(std::istream & stream, int stroffset) {
    stream.seekg (elf->mainoffset + offset + stroffset, std::ios_base::beg) ;

    return elf->read_word4(stream);
}

BVector Section::get_contents(std::istream& stream) {
    stream.seekg (elf->mainoffset + offset, std::ios_base::beg);

    /* XXX: why does this exist? same as ProgramSegment. */

    byte* addr =  (byte*)malloc(size);
    stream.read((char*)addr, size);
    return BVector(addr, size);
}

ELFSymbol::ELFSymbol (ELF *elf) : elf (elf)
 {
}

ELFSymbol::~ELFSymbol() {
}

std::string ELFSymbol::get_name() {
        return name ;
}

Address ELFSymbol::get_value() {
        return value ;
}

Section* ELFSymbol::get_section() {
        return section ;
}

int ELFSymbol::read(std::istream & stream, Address baseaddr) {
        nameindex = elf->read_word4 (stream) ;
        if (elf->fileclass == ELFCLASS64) {
            info = elf->read_byte (stream) ;
            other = elf->read_byte (stream) ;
            shndx = elf->read_word2 (stream) ;
            value = elf->read_address (stream) ;
            size = elf->read_xword (stream) ;
        } else {
            value = elf->read_address (stream) ;
            size = elf->read_word4 (stream) ;
            info = elf->read_byte (stream) ;
            other = elf->read_byte (stream) ;
            shndx = elf->read_word2 (stream) ;
        }
        value += baseaddr ;
        section = elf->find_section_by_index (shndx) ;
        return 0 ;
}

void ELFSymbol::set_name(std::istream & stream, Section *nametable) {
        name = nametable->read_string (stream, nameindex) ;
        std::string::size_type postfix = name.find ("@@") ;
        if (postfix != std::string::npos) {
            name = name.substr (0, postfix) ;
        }
}

int ELFSymbol::get_size() {
        return elf->fileclass == ELFCLASS64 ? 24 : 16 ;
}

void ELFSymbol::print() {
        std::cout << "name = "  <<  name << '\n' ;
        std::cout << "nameindex = "  <<  nameindex << '\n' ;
        std::cout << "value = "  <<  value << '\n' ;
        std::cout << "size = "  <<  size << '\n' ;
        std::cout << "info = "  <<  info << '\n' ;
        std::cout << "other = "  <<  other << '\n' ;
        std::cout << "shndx = "  <<  shndx << '\n' ;
        if (section == NULL) {
            std::cout << "no section found" << '\n' ;
        } else {
            std::cout << "section: "  <<  section->get_name() << '\n' ;
        }
}

ELF::ELF (std::string name, Offset mainoffset)
    : name(name),
    mainoffset(mainoffset),
    fileclass(0),
    dataencoding(0),
    elfversion(0),
    abi(0),
    abiversion(0),
    type(0),
    machine(0),
    version(0),
    entry(0),
    phoff(0),
    shoff(0),
    flags(0),
    ehsize(0),
    phentsize(0),
    phnum(0),
    shentsize(0),
    shnum(0),
    shstrndx(0), caseblind_ok(false), base(0) {
}

ELF::~ELF() {
    for (unsigned int i = 0 ; i < segments.size(); i++) {
        delete segments[i] ;
    }

    for (unsigned int i = 0 ; i < sections.size(); i++) {
        delete sections[i] ;
    }

    Map_Range<Address,ELFSymbol*>::iterator i;
    for (i=symmap.begin(); i!=symmap.end(); ++i) {
        delete i->val;
    }
}

bool ELF::is_elf64() {
    return fileclass == ELFCLASS64 ;
}

byte ELF::read_byte(std::istream & stream) {
    return (byte)(stream.get()) ;
}

int64_t ELF::read_word8(std::istream & stream) {
    int64_t w = 0 ;
    if (dataencoding == ELFDATA2LSB) {
        for (int i = 0 ; i < 8 ; i++) {
            w |= (int64_t)(stream.get() & 0xff) << (i * 8) ;
        }
    } else {
        for (int i = 0 ; i < 8 ; i++) {
            w = (w << 8) | (stream.get() & 0xff) ;
        }
    }
    return w ;
}

int32_t ELF::read_word4(std::istream & stream) {
    int32_t w = 0 ;
    if (dataencoding == ELFDATA2LSB) {                // little endian?
        for (int i = 0 ; i < 4 ; i++) {
            w |= (stream.get() & 0xff) << (i * 8) ;
        }
    } else {
        for (int i = 0 ; i < 4 ; i++) {
            w = (w << 8) | (stream.get() & 0xff) ;
        }
    }
    return w ;
}

int16_t ELF::read_word2(std::istream & stream) {
    int16_t w = 0 ;
    if (dataencoding == ELFDATA2LSB) {                // little endian
        for (int i = 0 ; i < 2 ; i++) {
            w |= (stream.get() & 0xff) << (i * 8) ;
        }
    } else {
        for (int i = 0 ; i < 2 ; i++) {
            w = (w << 8) | (stream.get() & 0xff) ;
        }
    }
    return w ;
}

Address ELF::read_address(std::istream & stream) {
    if (fileclass == ELFCLASS64) {           // ELF64?
        return read_word8 (stream) ;
    } else {
        return (Address)read_word4 (stream) & 0xffffffff ;
    }
}

Offset ELF::read_offset(std::istream & stream) {
    return read_address (stream) ;
}

int64_t ELF::read_xword(std::istream & stream) {
    return read_address (stream) ;
}

bool ELF::is_little_endian() {
    return dataencoding == ELFDATA2LSB ;
}

static bool
check_section_header(std::istream *s, Section *sec, std::string name,
		     int size, int tag)
{
	ssize_t	header_size;

	header_size = name.size() + 1;
	header_size = ((header_size + 3) & ~3);
	header_size += size;
	header_size = ((header_size + 3) & ~3);

	if (header_size > sec->get_size())
		return false;

	if (sec->read_word4 (*s, 0) != (int)name.size() + 1)
		return false;

	if (sec->read_word4 (*s, 4) != size)
		return false;

	if (sec->read_word4 (*s, 8) != tag)
		return false;

	if (sec->read_string (*s, 12) != name)
		return false;

	return true;
}

void ELF::read_header(std::istream & stream, Address baseaddr) {
    base = baseaddr ;
    stream.seekg (mainoffset + 0, std::ios_base::beg) ;
    for (int i = 0 ; i < 4; i++) {
        ident[i] = (byte)(stream.get()) ;
    }
    if (ident[0] != 0x7f && ident[1] != 'E' && ident[2] != 'L' && ident[3] != 'F') {
        throw Exception ("invalid elf file (bad magic)") ;
    }
    fileclass = read_byte (stream) ;
    dataencoding = read_byte(stream) ;
    elfversion = read_byte(stream) ;
    abi = read_byte(stream) ;
    abiversion = read_byte(stream) ;
    for (int i = 0 ; i < 16-9; i++) {
        stream.get() ;
    }
    type = read_word2 (stream) ;
    machine = read_word2 (stream) ;
    version = read_word4 (stream) ;
    entry = read_address (stream) ;
    phoff = read_offset (stream) ;
    shoff = read_offset (stream) ;
    flags = read_word4 (stream) ;
    ehsize = read_word2 (stream) ;
    phentsize = read_word2 (stream) ;
    phnum = read_word2 (stream) ;
    shentsize = read_word2 (stream) ;
    shnum = read_word2 (stream) ;
    shstrndx = read_word2 (stream) ;

    //println ("seekg " + format ("%x", mainoffset + shoff))
    Offset offset = mainoffset + shoff ;
    stream.seekg (offset, std::ios_base::beg) ;
    for (int i = 0 ; i < shnum; i++) {
        Section *section = new Section(this, i) ;
        sections.push_back (section) ;
        section->read (stream) ;
    }

    offset = mainoffset + phoff ;
    stream.seekg (offset, std::ios_base::beg) ;
    for (int i = 0 ; i < phnum; i++) {
        ProgramSegment *segment = new ProgramSegment(this, baseaddr) ;
        segments.push_back (segment) ;
        segment->read (stream) ;
    }


    if (sections.size() > 0) {
        Section *sectionnames = sections[shstrndx] ;
        for (unsigned int i = 0 ; i < sections.size(); i++) {
            Section *section = sections[i] ;
            section->set_name (stream, sectionnames) ;
        }
    }

    //for (int i = 0 ; i < sections.size(); i++) {
        //Section *section = sections[i] ;
        //section->print () ;
    //}
	if (abi == ELFOSABI_NONE) {
		//need get abi from section
		Section *sec = find_section(".note.ABI-tag");
		if (sec) {
			if (check_section_header(&stream, sec, "GNU", 16, 1)) {
				switch (sec->read_word4 (stream, 16)) {
				case 0:
					abi = ELFOSABI_LINUX;
					break;
				case 3:
					abi = ELFOSABI_FREEBSD;
					break;
				}
			}

			if (check_section_header(&stream, sec, "FreeBSD", 4, 1)) {
				abi = ELFOSABI_FREEBSD;
			}
		}
	}
}

void ELF::print_header() {
    std::cout << "fileclass = "  <<  fileclass << '\n' ;
    std::cout << "dataencoding = "  <<  dataencoding << '\n' ;
    std::cout << "elfversion = "  <<  elfversion << '\n' ;
    std::cout << "abi = "  <<  abi << '\n' ;
    std::cout << "abiversion = "  <<  abiversion << '\n' ;
    std::cout << "type = "  <<  type << '\n' ;
    std::cout << "machine = "  <<  machine << '\n' ;
    std::cout << "version = "  <<  version << '\n' ;
    std::cout << "entry = "  <<  entry << '\n' ;
    std::cout << "phoff = "  <<  phoff << '\n' ;
    std::cout << "shoff = "  <<  shoff << '\n' ;
    std::cout << "flags = "  <<  flags << '\n' ;
    std::cout << "ehsize = "  <<  ehsize << '\n' ;
    std::cout << "phentsize = "  <<  phentsize << '\n' ;
    std::cout << "phnum = "  <<  phnum << '\n' ;
    std::cout << "shentsize = "  <<  shentsize << '\n' ;
    std::cout << "shnum = "  <<  shnum << '\n' ;
    std::cout << "shstrndx = "  <<  shstrndx << '\n' ;
}

std::istream *ELF::open(Address baseaddr) {
     // printf ("opening elf file %s\n", name.c_str()) ;
    std::ifstream *s = new std::ifstream (
       name.c_str(), std::ios::binary) ;
    if (s == NULL || !s->good()) {
        throw Exception ("Unable to open ELF file") ;
    }
    read_header (*s, baseaddr) ;
    //print_header (stdout)
    return s ;
}

BVector ELF::get_section(std::istream & stream, std::string name) {
    for (unsigned int i = 0 ; i < sections.size(); i++) {
        Section *section = sections[i] ;
        if (section->get_name() == name) {
            return section->get_contents(stream) ;
        }
    }
    throw Exception("No such section: %s", name.c_str()) ;
}

Section *ELF::find_section(std::string name) {
    for (unsigned int i = 0 ; i < sections.size(); i++) {
        Section *section = sections[i] ;
        if (section->get_name() == name) {
            return section ;
        }
    }
    return NULL;
}

Section *ELF::find_section_by_index(int index) {
    if (index < 0 || index >= (int)sections.size()) {
        return NULL ;
    }
    return sections[index] ;
}

class Compare_symbols {
public:
    bool operator() (ELFSymbol *s1, ELFSymbol *s2) {
        return *s1 < *s2 ;
    }
} ;

void ELF::read_symtab (std::istream &stream, Section *symtab, Address baseaddr, Section *strtab) {
    int symtabsize = symtab->get_size() ;
    stream.seekg (mainoffset + symtab->get_offset(), std::ios_base::beg) ;

    while (symtabsize > 0) {
        ELFSymbol *sym = new ELFSymbol(this) ;
        sym->read (stream, baseaddr) ;
        symtabsize -= sym->get_size() ;
        // only insert into symbol table if the symbol is not hidden and it has a valid section index
        if (ELF32_ST_VISIBILITY (sym->get_other()) != STV_HIDDEN && sym->get_section_index() != 0) {
            int type = ELF32_ST_TYPE(sym->get_info()) ;
            if (type == STT_OBJECT || type == STT_FUNC || type == STT_NOTYPE) {
                symmap.raw(sym->value, sym->value + sym->get_size() -1, sym);

                std::ios::pos_type pos = stream.tellg();
                stream.seekg(symtabsize, std::ios_base::cur);
                sym->set_name(stream, strtab);
                symbols[sym->get_name()] = sym;
                stream.seekg(pos, std::ios_base::beg);
            } else {
                delete sym ;
            }
        } else {
            delete sym ;
        }
    }
}

void ELF::make_cb_symbol_table() {
    if (caseblind_ok) {
        return ;
    }
    for (ELFSymbolMap::iterator i = symbols.begin() ; i != symbols.end() ; i++) {
        cbsymbols[Utils::toUpper(i->first)] = i->second ;
    }
    caseblind_ok = true ;
}

void ELF::read_symbol_table(std::istream & stream, Address baseaddr) {
    Section *symtab = NULL ;
    Section *strtab = NULL ;

    symtab = find_section (".symtab") ;// Section object
    strtab = find_section (".strtab") ;// section object
    if (symtab != NULL && strtab != NULL) {
       read_symtab (stream, symtab, baseaddr, strtab) ;
    }

    symtab = find_section (".dynsym") ;// Section object
    strtab = find_section (".dynstr") ;// section object
    if (symtab != NULL && strtab != NULL) {
       read_symtab (stream, symtab, baseaddr, strtab) ;
    }
}

Address ELF::find_symbol(std::string name, bool caseblind) {
    if (caseblind) {
        if (!caseblind_ok) {
            make_cb_symbol_table() ;
        }
        ELFSymbolMap::iterator symi = cbsymbols.find (Utils::toUpper(name)) ;
        if (symi == cbsymbols.end()) {
            return 0 ;
        }
        return symi->second->get_value() ;
    } else {
        ELFSymbolMap::iterator symi = symbols.find (name) ;
        if (symi == symbols.end()) {
            return 0 ;
        }
        return symi->second->get_value() ;
    }
}

Section *ELF::find_symbol_section(std::string name, bool caseblind) {
    if (caseblind) {
        if (!caseblind_ok) {
            make_cb_symbol_table() ;
        }
        ELFSymbolMap::iterator symi = cbsymbols.find (Utils::toUpper(name)) ;
        if (symi == cbsymbols.end()) {
            return NULL ;
        }
        return symi->second->get_section() ;
    } else {
        ELFSymbolMap::iterator symi = symbols.find (name) ;
        if (symi == symbols.end()) {
            return NULL ;
        }
        return symi->second->get_section() ;
    }
}

void ELF::find_symbol_at_address(Address addr, std::string &name, int &offset) {
   ELFSymbol* sym;

   /* lookup of the address */
   if ( symmap.get(addr, &sym) ) {
      name = "";
      offset = 0;
      return;
   }

   name = sym->get_name();
   offset = addr - sym->value;
}

Section *ELF::find_section_at_addr(Address addr) {
    for (unsigned int i = 0 ; i < sections.size(); i++) {
        Section *section = sections[i] ;
        Address sectaddr = section->get_addr() ;
        if (addr >= sectaddr && addr < sectaddr + section->get_size()) {
            return section ;
        }
    }
    return NULL ;
}

void ELF::list_symbols(PStream &os) {
    Map_Range<Address,ELFSymbol*>::iterator i;
    for (i=symmap.begin(); i!=symmap.end(); ++i) {
       ELFSymbol* sym = i->val;
       std::string name = sym->get_name() ;
       Address value = sym->get_value() ;
       os.print ("\t%-30s 0x%llx\n", name.c_str(), value) ;
    }
}


void ELF::list_functions(PStream &os) {
    Map_Range<Address,ELFSymbol*>::iterator i;
    for (i=symmap.begin(); i!=symmap.end(); ++i) {
       ELFSymbol* sym = i->val;
       if (ELF64_ST_TYPE(sym->get_info()) == STT_FUNC) {
           std::string name = sym->get_name() ;
           Address value = sym->get_value() ;
           os.print ("0x%016llx %s\n", value, name.c_str()) ;
        }
    }
}

void ELF::list_variables(PStream &os) {
    Map_Range<Address,ELFSymbol*>::iterator i;
    for (i=symmap.begin(); i!=symmap.end(); ++i) {
       ELFSymbol *sym = i->val;
       if (ELF64_ST_TYPE(sym->get_info()) == STT_OBJECT) {
           std::string name = sym->get_name() ;
           Address value = sym->get_value() ;
           os.print ("0x%016llx %s\n", value, name.c_str()) ;
        }
    }
}


ProgramSegment *ELF::find_segment (Address addr) {
    for (unsigned int i = 0 ; i < segments.size() ; i++) {
        ProgramSegment *seg = segments[i] ;
        Address segaddr = seg->get_start() ;
        if (segaddr == addr) {
            return seg ;
        }
    }
    return NULL ;
}

Architecture *ELF::new_arch() {
	Architecture	*arch = NULL;

	switch (abi) {
	case ELFOSABI_LINUX:
		/* Linux */
		if (machine == EM_386)
			arch = new i386_linux_arch ();
		else if (machine == EM_X86_64)
			arch = new x86_64_linux_arch ();
		break;
	case ELFOSABI_FREEBSD:
		/* FreeBSD */
		if (machine == EM_386)
			arch = new i386_freebsd_arch ();
		else if (machine == EM_X86_64)
			arch = new x86_64_freebsd_arch ();
		break;
	}

	if (arch == NULL) {
		throw Exception ("This architecture is not support.") ;
	}

	return arch;
}

void
ELF::prstatus_to_thread(BStream *stream, int size, struct CoreThread *thread)
{
	switch (machine) {
	case EM_386:		//X86
		if (size != 144)
			throw Exception ("The format of core is not right.") ;
		//Get sig.
		stream->seek(12, BSTREAM_CUR);
		thread->sig = (int)stream->read2s();
		//Get pid.
		stream->seek(10, BSTREAM_CUR);
		thread->pid = (int)stream->read4s();
		//Get reg.
		stream->seek(44, BSTREAM_CUR);
		thread->reg = (char *)malloc(68);
		if (!thread->reg)
			throw Exception ("Malloc failed.") ;
		stream->read(thread->reg, 68);
		//Seek pass all this content.
		stream->seek(4, BSTREAM_CUR);
		break;
	case EM_X86_64:	//X86_64
		if (size != 336)
			throw Exception ("The format of core is not right.") ;
		//Get sig.
		stream->seek(12, BSTREAM_CUR);
		thread->sig = (int)stream->read2s();
		//Get pid.
		stream->seek(18, BSTREAM_CUR);
		thread->pid = (int)stream->read4s();
		//Get reg.
		stream->seek(76, BSTREAM_CUR);
		thread->reg = (char *)malloc(216);
		if (!thread->reg)
			throw Exception ("Malloc failed.") ;
		stream->read(thread->reg, 216);
		//Seek pass all this content.
		stream->seek(8, BSTREAM_CUR);
		break;
	default:
		throw Exception ("The format of core is not support.") ;
		break;
	}
}

void
ELF::prstatus_to_pname(BStream *stream, int size, std::string &pname)
{
	char	buf[17];

	switch (machine) {
	case EM_386:		//X86
		if (size != 124)
			throw Exception ("The format of core is not right.") ;
		//Get pname
		stream->seek(28, BSTREAM_CUR);
		stream->read(buf, 17);
		pname += buf;
		//Seek pass all this content.
		stream->seek(79, BSTREAM_CUR);
		break;
	case EM_X86_64:	//X86_64
		if (size != 136)
			throw Exception ("The format of core is not right.") ;
		//Get pname
		stream->seek(40, BSTREAM_CUR);
		stream->read(buf, 17);
		pname += buf;
		//Seek pass all this content.
		stream->seek(79, BSTREAM_CUR);
		break;
	default:
		throw Exception ("The format of core is not support.") ;
		break;
	}
}
