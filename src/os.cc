#include "os.h"

//From __sigset_t
typedef struct freebsd_sigset {
	uint32_t __bits[4];
} freebsd_sigset_t;

i386_linux_arch::i386_linux_arch()
{
	regset_size = sizeof (struct i386_linux_regs);
	fpregset_size = sizeof (struct i386_linux_fpregs);

    regnames["ebx"] = 0 * sizeof (long int) ;
    regnames["ecx"] = 1 * sizeof (long int) ;
    regnames["edx"] = 2 * sizeof (long int) ;
    regnames["esi"] = 3 * sizeof (long int) ;
    regnames["edi"] = 4 * sizeof (long int) ;
    regnames["ebp"] = 5 * sizeof (long int) ;
    regnames["eax"] = 6 * sizeof (long int) ;
    regnames["xds"] = 7 * sizeof (long int) ;
    regnames["es"] = 8 * sizeof (long int) ;
    regnames["fs"] = 9 * sizeof (long int) ;
    regnames["gs"] = 10 * sizeof (long int) ;
    //regnames["orig_eax"] = 11 * sizeof (long int) ;
    regnames["eip"] = 12 * sizeof (long int) ;
    regnames["cs"] = 13 * sizeof (long int) ;
    regnames["eflags"] = 14 * sizeof (long int) ;
    regnames["esp"] = 15 * sizeof (long int) ;
    regnames["ss"] = 16 * sizeof (long int) ;

    st_start = sizeof (unsigned short) * 4 + sizeof(long) * 6 ;

    // there are 8 SSE registers
    sse_start = st_start + 32 * sizeof(long);

    reset_reg ();
}

void
i386_linux_arch::register_set_from_native(char *buf, int size, RegisterSet *reg) {
	if (size == sizeof (struct i386_linux_regs)) {
		struct i386_linux_regs	*regs_buf = (struct i386_linux_regs *)buf;

		reg->set_register("ebx", (int64_t)regs_buf->ebx);
		reg->set_register("ecx", (int64_t)regs_buf->ecx);
		reg->set_register("edx", (int64_t)regs_buf->edx);
		reg->set_register("esi", (int64_t)regs_buf->esi);
		reg->set_register("edi", (int64_t)regs_buf->edi);
		reg->set_register("fp", (int64_t)regs_buf->ebp);
		reg->set_register("eax", (int64_t)regs_buf->eax);
		reg->set_register("ds", (int64_t)regs_buf->xds);
		reg->set_register("es", (int64_t)regs_buf->xes);
		reg->set_register("fs", (int64_t)regs_buf->xfs);
		reg->set_register("gs", (int64_t)regs_buf->xgs);
		//reg->set_register("orig_eax", (int64_t)regs_buf->orig_eax);
		reg->set_register("pc", (int64_t)regs_buf->eip);
		reg->set_register("cs", (int64_t)regs_buf->xcs);
		reg->set_register("eflags", (int64_t)regs_buf->eflags);
		reg->set_register("sp", (int64_t)regs_buf->esp);
		reg->set_register("ss", (int64_t)regs_buf->xss);
	}
	else if (size == sizeof (struct x86_64_linux_regs)) {
		struct x86_64_linux_regs	*regs_buf = (struct x86_64_linux_regs *)buf;

		reg->set_register("ebx", (int64_t)regs_buf->rbx & 0xffffffff);
		reg->set_register("ecx", (int64_t)regs_buf->rcx & 0xffffffff);
		reg->set_register("edx", (int64_t)regs_buf->rdx & 0xffffffff);
		reg->set_register("esi", (int64_t)regs_buf->rsi & 0xffffffff);
		reg->set_register("edi", (int64_t)regs_buf->rdi & 0xffffffff);
		reg->set_register("fp", (int64_t)regs_buf->rbp & 0xffffffff);
		reg->set_register("eax", (int64_t)regs_buf->rax & 0xffffffff);
		reg->set_register("pc", (int64_t)regs_buf->rip & 0xffffffff);
		reg->set_register("sp", (int64_t)regs_buf->rsp & 0xffffffff);
		reg->set_register("ds", (int64_t)regs_buf->ds & 0xffffffff);
		reg->set_register("es", (int64_t)regs_buf->es & 0xffffffff);
		reg->set_register("fs", (int64_t)regs_buf->fs & 0xffffffff);
		reg->set_register("gs", (int64_t)regs_buf->gs & 0xffffffff);
		reg->set_register("cs", (int64_t)regs_buf->cs & 0xffffffff);
		reg->set_register("eflags", (int64_t)regs_buf->eflags & 0xffffffff);
		reg->set_register("ss", (int64_t)regs_buf->ss & 0xffffffff);
	}
	else {
		throw Exception ("Reg size is not right.");
	}
}

void
i386_linux_arch::register_set_to_native(char *buf, int size, RegisterSet *reg) {
	if (size == sizeof (struct i386_linux_regs)) {
		struct i386_linux_regs	*regs_buf = (struct i386_linux_regs *)buf;

		regs_buf->ebx = reg->get_register_as_integer("ebx");
		regs_buf->ecx = reg->get_register_as_integer("ecx");
		regs_buf->edx = reg->get_register_as_integer("edx");
		regs_buf->esi = reg->get_register_as_integer("esi");
		regs_buf->edi = reg->get_register_as_integer("edi");
		regs_buf->ebp = reg->get_register_as_integer("fp");
		regs_buf->eax = reg->get_register_as_integer("eax");
		regs_buf->esp = reg->get_register_as_integer("sp");
		regs_buf->eip = reg->get_register_as_integer("pc");
#if 0
		regs_buf->xds = reg->get_register_as_integer("ds");
		regs_buf->xes = reg->get_register_as_integer("es");
		regs_buf->xfs = reg->get_register_as_integer("fs");
		regs_buf->xgs = reg->get_register_as_integer("gs");
		//regs_buf->orig_eax = reg->get_register_as_integer("orig_eax");
		regs_buf->xcs = reg->get_register_as_integer("cs");
		regs_buf->eflags = reg->get_register_as_integer("eflags");
#endif
	}
	else if (size == sizeof (struct x86_64_linux_regs)) {
		struct x86_64_linux_regs	*regs_buf = (struct x86_64_linux_regs *)buf;

		regs_buf->rbx &= 0xffffffff00000000ULL;
		regs_buf->rcx &= 0xffffffff00000000ULL;
		regs_buf->rdx &= 0xffffffff00000000ULL;
		regs_buf->rsi &= 0xffffffff00000000ULL;
		regs_buf->rdi &= 0xffffffff00000000ULL;
		regs_buf->rbp &= 0xffffffff00000000ULL;
		regs_buf->rax &= 0xffffffff00000000ULL;
		regs_buf->ds &= 0xffffffff00000000ULL;
		regs_buf->es &= 0xffffffff00000000ULL;
		regs_buf->fs &= 0xffffffff00000000ULL;
		regs_buf->gs &= 0xffffffff00000000ULL;
		regs_buf->rip &= 0xffffffff00000000ULL;
		regs_buf->cs &= 0xffffffff00000000ULL;
		regs_buf->eflags &= 0xffffffff00000000ULL;
		regs_buf->rsp &= 0xffffffff00000000ULL;

		regs_buf->rbx |= reg->get_register_as_integer("ebx");
		regs_buf->rcx |= reg->get_register_as_integer("ecx");
		regs_buf->rdx |= reg->get_register_as_integer("edx");
		regs_buf->rsi |= reg->get_register_as_integer("esi");
		regs_buf->rdi |= reg->get_register_as_integer("edi");
		regs_buf->rbp |= reg->get_register_as_integer("fp");
		regs_buf->rax |= reg->get_register_as_integer("eax");
		regs_buf->rip |= reg->get_register_as_integer("pc");
		regs_buf->rsp |= reg->get_register_as_integer("sp");
#if 0
		regs_buf->ds |= reg->get_register_as_integer("ds");
		regs_buf->es |= reg->get_register_as_integer("es");
		regs_buf->fs |= reg->get_register_as_integer("fs");
		regs_buf->gs |= reg->get_register_as_integer("gs");
		regs_buf->cs |= reg->get_register_as_integer("cs");
		regs_buf->eflags |= reg->get_register_as_integer("eflags");
#endif
	}
	else {
		throw Exception ("Reg size is not right.");
	}
}

void
i386_linux_arch::fpregister_set_from_native(char *buf, int size, RegisterSet *reg) {
	unsigned char			*p;
	std::vector<unsigned char>	val;

	struct i386_linux_fpregs *freg_buf = (struct i386_linux_fpregs *)buf;
	p = (unsigned char *)freg_buf->st_space;

	val.insert(val.begin(), p, p + 10);
	reg->set_register("st0", val);
	p += 10;
	val.insert(val.begin(), p, p + 10);
	reg->set_register("st1", val);
	p += 10;
	val.insert(val.begin(), p, p + 10);
	reg->set_register("st2", val);
	p += 10;
	val.insert(val.begin(), p, p + 10);
	reg->set_register("st3", val);
	p += 10;
	val.insert(val.begin(), p, p + 10);
	reg->set_register("st4", val);
	p += 10;
	val.insert(val.begin(), p, p + 10);
	reg->set_register("st5", val);
	p += 10;
	val.insert(val.begin(), p, p + 10);
	reg->set_register("st6", val);
	p += 10;
	val.insert(val.begin(), p, p + 10);
	reg->set_register("st7", val);
}

void
i386_linux_arch::fpregister_set_to_native(char *buf, int size, RegisterSet *reg) {
	unsigned char			*p;
	std::vector<unsigned char>	val;

	struct i386_linux_fpregs *freg_buf = (struct i386_linux_fpregs *)buf;
	p = (unsigned char *)freg_buf->st_space;

	val = reg->get_register_as_bytes(reg->get_properties()->register_number_for_name("st0"));
	assert(val.size() == 10);
	std::copy(val.begin(), val.end(), p);
	p += 10;
	val = reg->get_register_as_bytes(reg->get_properties()->register_number_for_name("st1"));
	assert(val.size() == 10);
	std::copy(val.begin(), val.end(), p);
	p += 10;
	val = reg->get_register_as_bytes(reg->get_properties()->register_number_for_name("st2"));
	assert(val.size() == 10);
	std::copy(val.begin(), val.end(), p);
	p += 10;
	val = reg->get_register_as_bytes(reg->get_properties()->register_number_for_name("st3"));
	assert(val.size() == 10);
	std::copy(val.begin(), val.end(), p);
	p += 10;
	val = reg->get_register_as_bytes(reg->get_properties()->register_number_for_name("st4"));
	assert(val.size() == 10);
	std::copy(val.begin(), val.end(), p);
	p += 10;
	val = reg->get_register_as_bytes(reg->get_properties()->register_number_for_name("st5"));
	assert(val.size() == 10);
	std::copy(val.begin(), val.end(), p);
	p += 10;
	val = reg->get_register_as_bytes(reg->get_properties()->register_number_for_name("st6"));
	assert(val.size() == 10);
	std::copy(val.begin(), val.end(), p);
	p += 10;
	val = reg->get_register_as_bytes(reg->get_properties()->register_number_for_name("st7"));
	assert(val.size() == 10);
	std::copy(val.begin(), val.end(), p);
	p += 10;
}

x86_64_linux_arch::x86_64_linux_arch()
{
	regset_size = sizeof (struct x86_64_linux_regs);
	fpregset_size = sizeof (struct x86_64_linux_fpregs);

    regnames["r15"] = 0 * sizeof (long int) ;
    regnames["r14"] = 1 * sizeof (long int) ;
    regnames["r13"] = 2 * sizeof (long int) ;
    regnames["r12"] = 3 * sizeof (long int) ;
    regnames["rbp"] = 4 * sizeof (long int) ;
    regnames["rbx"] = 5 * sizeof (long int) ;
    regnames["r11"] = 6 * sizeof (long int) ;
    regnames["r10"] = 7 * sizeof (long int) ;
    regnames["r9"] = 8 * sizeof (long int) ;
    regnames["r8"] = 9 * sizeof (long int) ;
    regnames["rax"] = 10 * sizeof (long int) ;
    regnames["rcx"] = 11 * sizeof (long int) ;
    regnames["rdx"] = 12 * sizeof (long int) ;
    regnames["rsi"] = 13 * sizeof (long int) ;
    regnames["rdi"] = 14 * sizeof (long int) ;
    /*regnames["orig_rax"] = 15 * sizeof (long int) ; */       // orig_rax
    regnames["rip"] = 16 * sizeof (long int) ;
    regnames["cs"] = 17 * sizeof (long int) ;
    regnames["eflags"] = 18 * sizeof (long int) ;
    regnames["rsp"] = 19 * sizeof (long int) ;
    regnames["ss"] = 20 * sizeof (long int) ;
    /*regnames["fs_base"] = 21 * sizeof (long int) ;*/        // fs_base
    /*regnames["gs_base"] = 22 * sizeof (long int) ;*/     // gs_base
    regnames["ds"] = 23 * sizeof (long int) ;
    regnames["es"] = 24 * sizeof (long int) ;
    regnames["fs"] = 25 * sizeof (long int) ;
    regnames["gs"] = 26 * sizeof (long int) ;

    st_start = sizeof (unsigned short) * 4 + sizeof(long) * 2 + sizeof(int) * 2 ;

    // there are 8 SSE registers
    sse_start = st_start + sizeof(int) * 32 ;

    reset_reg ();

static int x86_64_linux_sigcontext_regs[] = {
        translate_regname ("r8"),
        translate_regname ("r9"),
        translate_regname ("r10"),
        translate_regname ("r11"),
        translate_regname ("r12"),
        translate_regname ("r13"),
        translate_regname ("r14"),
        translate_regname ("r15"),
        translate_regname ("rdi"),
        translate_regname ("rsi"),
        translate_regname ("rbp"),
        translate_regname ("rbx"),
        translate_regname ("rdx"),
        translate_regname ("rax"),
        translate_regname ("rcx"),
        translate_regname ("rsp"),
        translate_regname ("rip"),
        translate_regname ("eflags"),
        translate_regname ("cs"), // XXX: these are unsigned short
        translate_regname ("gs"),
        translate_regname ("fs"),
        -1
} ;

    x86_64_sigcontext_regs = x86_64_linux_sigcontext_regs;
}

void
x86_64_linux_arch::register_set_from_native(char *buf, int size, RegisterSet *reg) {
	struct x86_64_linux_regs	*regs_buf = (struct x86_64_linux_regs *)buf;

	if (size != sizeof (struct x86_64_linux_regs))
		throw Exception ("Reg size is not right.");

	reg->set_register("r15", (int64_t)regs_buf->r15);
	reg->set_register("r14", (int64_t)regs_buf->r14);
	reg->set_register("r13", (int64_t)regs_buf->r13);
	reg->set_register("r12", (int64_t)regs_buf->r12);
	reg->set_register("fp", (int64_t)regs_buf->rbp);
	reg->set_register("rbx", (int64_t)regs_buf->rbx);
	reg->set_register("r11", (int64_t)regs_buf->r11);
	reg->set_register("r10", (int64_t)regs_buf->r10);
	reg->set_register("r9", (int64_t)regs_buf->r9);
	reg->set_register("r8", (int64_t)regs_buf->r8);
	reg->set_register("rax", (int64_t)regs_buf->rax);
	reg->set_register("rcx", (int64_t)regs_buf->rcx);
	reg->set_register("rdx", (int64_t)regs_buf->rdx);
	reg->set_register("rsi", (int64_t)regs_buf->rsi);
	reg->set_register("rdi", (int64_t)regs_buf->rdi);
	reg->set_register("pc", (int64_t)regs_buf->rip);
	reg->set_register("cs", (int64_t)regs_buf->cs);
	reg->set_register("eflags", (int64_t)regs_buf->eflags);
	reg->set_register("sp", (int64_t)regs_buf->rsp);
	reg->set_register("ss", (int64_t)regs_buf->ss);
	reg->set_register("ds", (int64_t)regs_buf->ds);
	reg->set_register("es", (int64_t)regs_buf->es);
	reg->set_register("fs", (int64_t)regs_buf->fs);
	reg->set_register("gs", (int64_t)regs_buf->gs);
}

void
x86_64_linux_arch::register_set_to_native(char *buf, int size, RegisterSet *reg) {
	struct x86_64_linux_regs	*regs_buf = (struct x86_64_linux_regs *)buf;

	if (size != sizeof (struct x86_64_linux_regs))
		throw Exception ("Reg size is not right.");

	regs_buf->r15 = reg->get_register_as_integer("r15");
	regs_buf->r14 = reg->get_register_as_integer("r14");
	regs_buf->r13 = reg->get_register_as_integer("r13");
	regs_buf->r12 = reg->get_register_as_integer("r12");
	regs_buf->rbp = reg->get_register_as_integer("fp");
	regs_buf->rbx = reg->get_register_as_integer("rbx");
	regs_buf->r11 = reg->get_register_as_integer("r11");
	regs_buf->r10 = reg->get_register_as_integer("r10");
	regs_buf->r9 = reg->get_register_as_integer("r9");
	regs_buf->r8 = reg->get_register_as_integer("r8");
	regs_buf->rax = reg->get_register_as_integer("rax");
	regs_buf->rcx = reg->get_register_as_integer("rcx");
	regs_buf->rdx = reg->get_register_as_integer("rdx");
	regs_buf->rsi = reg->get_register_as_integer("rsi");
	regs_buf->rdi = reg->get_register_as_integer("rdi");
	regs_buf->rip = reg->get_register_as_integer("pc");
#if 0
	regs_buf->cs = reg->get_register_as_integer("cs");
	regs_buf->eflags = reg->get_register_as_integer("eflags");
#endif
	regs_buf->rsp = reg->get_register_as_integer("sp");
#if 0
	regs_buf->ss = reg->get_register_as_integer("ss");
	regs_buf->ds = reg->get_register_as_integer("ds");
	regs_buf->es = reg->get_register_as_integer("es");
	regs_buf->fs = reg->get_register_as_integer("fs");
	regs_buf->gs = reg->get_register_as_integer("gs");
#endif
}

void
x86_64_linux_arch::fpregister_set_from_native(char *buf, int size, RegisterSet *reg) {
	unsigned char			*p;
	std::vector<unsigned char>	val;

	struct x86_64_linux_fpregs *freg_buf = (struct x86_64_linux_fpregs *)buf;
	p = (unsigned char *)freg_buf->st_space;

	val.insert(val.begin(), p, p + 10);
	reg->set_register("st0", val);
	p += 10;
	val.insert(val.begin(), p, p + 10);
	reg->set_register("st1", val);
	p += 10;
	val.insert(val.begin(), p, p + 10);
	reg->set_register("st2", val);
	p += 10;
	val.insert(val.begin(), p, p + 10);
	reg->set_register("st3", val);
	p += 10;
	val.insert(val.begin(), p, p + 10);
	reg->set_register("st4", val);
	p += 10;
	val.insert(val.begin(), p, p + 10);
	reg->set_register("st5", val);
	p += 10;
	val.insert(val.begin(), p, p + 10);
	reg->set_register("st6", val);
	p += 10;
	val.insert(val.begin(), p, p + 10);
	reg->set_register("st7", val);
}

void
x86_64_linux_arch::fpregister_set_to_native(char *buf, int size, RegisterSet *reg) {
	unsigned char			*p;
	std::vector<unsigned char>	val;

	struct x86_64_linux_fpregs *freg_buf = (struct x86_64_linux_fpregs *)buf;
	p = (unsigned char *)freg_buf->st_space;

	val = reg->get_register_as_bytes(reg->get_properties()->register_number_for_name("st0"));
	assert(val.size() == 10);
	std::copy(val.begin(), val.end(), p);
	p += 10;
	val = reg->get_register_as_bytes(reg->get_properties()->register_number_for_name("st1"));
	assert(val.size() == 10);
	std::copy(val.begin(), val.end(), p);
	p += 10;
	val = reg->get_register_as_bytes(reg->get_properties()->register_number_for_name("st2"));
	assert(val.size() == 10);
	std::copy(val.begin(), val.end(), p);
	p += 10;
	val = reg->get_register_as_bytes(reg->get_properties()->register_number_for_name("st3"));
	assert(val.size() == 10);
	std::copy(val.begin(), val.end(), p);
	p += 10;
	val = reg->get_register_as_bytes(reg->get_properties()->register_number_for_name("st4"));
	assert(val.size() == 10);
	std::copy(val.begin(), val.end(), p);
	p += 10;
	val = reg->get_register_as_bytes(reg->get_properties()->register_number_for_name("st5"));
	assert(val.size() == 10);
	std::copy(val.begin(), val.end(), p);
	p += 10;
	val = reg->get_register_as_bytes(reg->get_properties()->register_number_for_name("st6"));
	assert(val.size() == 10);
	std::copy(val.begin(), val.end(), p);
	p += 10;
	val = reg->get_register_as_bytes(reg->get_properties()->register_number_for_name("st7"));
	assert(val.size() == 10);
	std::copy(val.begin(), val.end(), p);
	p += 10;
}

x86_64_freebsd_arch::x86_64_freebsd_arch()
{
	regset_size = sizeof (struct x86_64_freebsd_regs);
	fpregset_size = sizeof (struct x86_64_freebsd_fpregs);

    regnames["r15"] = 0 * sizeof (uint64_t) ;
    regnames["r14"] = 1 * sizeof (uint64_t) ;
    regnames["r13"] = 2 * sizeof (uint64_t) ;
    regnames["r12"] = 3 * sizeof (uint64_t) ;
    regnames["rbp"] = 10 * sizeof (uint64_t) ;
    regnames["rbx"] = 11 * sizeof (uint64_t) ;
    regnames["r11"] = 4 * sizeof (uint64_t) ;
    regnames["r10"] = 5 * sizeof (uint64_t) ;
    regnames["r9"] = 6 * sizeof (uint64_t) ;
    regnames["r8"] = 7 * sizeof (uint64_t) ;
    regnames["rax"] = 14 * sizeof (uint64_t) ;
    regnames["rcx"] = 13 * sizeof (uint64_t) ;
    regnames["rdx"] = 12 * sizeof (uint64_t) ;
    regnames["rsi"] = 9 * sizeof (uint64_t) ;
    regnames["rdi"] = 8 * sizeof (uint64_t) ;
    /*regnames["orig_rax"] = 15 * sizeof (uint64_t) ; */        // orig_rax
    regnames["rip"] = 17 * sizeof (uint64_t) ;
    regnames["cs"] = 18 * sizeof (uint64_t) ;
    regnames["eflags"] = 19 * sizeof (uint64_t) ;
    regnames["rsp"] = 20 * sizeof (uint64_t) ;
    regnames["ss"] = 21 * sizeof (uint64_t) ;
    /*regnames["fs_base"] = 21 * sizeof (uint64_t) ;*/         // fs_base
    /*regnames["gs_base"] = 22 * sizeof (uint64_t) ;*/         // gs_base
    regnames["ds"] = 17 * sizeof (uint64_t) - sizeof (uint16_t) ;
    regnames["es"] = 17 * sizeof (uint64_t) - 2 * sizeof (uint16_t) ;
    regnames["fs"] = 15 * sizeof (uint64_t) + sizeof (uint32_t) ;
    regnames["gs"] = 15 * sizeof (uint64_t) + sizeof (uint32_t) + sizeof (uint16_t) ;

    // XXX Not supported
    st_start = sizeof (uint64_t) * 4 ;

    // XXX Not supported
    sse_start = st_start + sizeof(int) * 32 ;

    reset_reg ();

    ctx_offset = sizeof (freebsd_sigset_t) + sizeof (long) ; // skip sc_mask and sc_onstack members

   static int x86_64_freebsd_sigcontext_regs[] = {
        translate_regname ("rdi"),
        translate_regname ("rsi"),
        translate_regname ("rdx"),
        translate_regname ("rcx"),
        translate_regname ("r8"),
        translate_regname ("r9"),
        translate_regname ("rax"),
        translate_regname ("rbx"),
        translate_regname ("rbp"),
        translate_regname ("r10"),
        translate_regname ("r11"),
        translate_regname ("r12"),
        translate_regname ("r13"),
        translate_regname ("r14"),
        translate_regname ("r15"),
        -2,                        // sc_trapno, sc_fs, sc_gs
        -2,                        // sc_addr
        -2,                        // sc_flags, sc_es, sc_ds
        -2,                        // sc_err
        translate_regname ("rip"),
        translate_regname ("cs"),
        translate_regname ("eflags"),
        translate_regname ("rsp"),
        translate_regname ("ss"),
        -1
    } ;
    x86_64_sigcontext_regs = x86_64_freebsd_sigcontext_regs;
}

void
x86_64_freebsd_arch::register_set_from_native(char *buf, int size, RegisterSet *reg) {
	struct x86_64_freebsd_regs	*regs_buf = (struct x86_64_freebsd_regs *)buf;

	reg->set_register("r15", (int64_t)regs_buf->r_r15);
	reg->set_register("r14", (int64_t)regs_buf->r_r14);
	reg->set_register("r13", (int64_t)regs_buf->r_r13);
	reg->set_register("r12", (int64_t)regs_buf->r_r12);
	reg->set_register("fp", (int64_t)regs_buf->r_rbp);
	reg->set_register("rbx", (int64_t)regs_buf->r_rbx);
	reg->set_register("r11", (int64_t)regs_buf->r_r11);
	reg->set_register("r10", (int64_t)regs_buf->r_r10);
	reg->set_register("r9", (int64_t)regs_buf->r_r9);
	reg->set_register("r8", (int64_t)regs_buf->r_r8);
	reg->set_register("rax", (int64_t)regs_buf->r_rax);
	reg->set_register("rcx", (int64_t)regs_buf->r_rcx);
	reg->set_register("rdx", (int64_t)regs_buf->r_rdx);
	reg->set_register("rsi", (int64_t)regs_buf->r_rsi);
	reg->set_register("rdi", (int64_t)regs_buf->r_rdi);
	reg->set_register("pc", (int64_t)regs_buf->r_rip);
	reg->set_register("cs", (int64_t)regs_buf->r_cs);
	reg->set_register("eflags", (int64_t)regs_buf->r_rflags);
	reg->set_register("sp", (int64_t)regs_buf->r_rsp);
	reg->set_register("ss", (int64_t)regs_buf->r_ss);
	reg->set_register("ds", (int64_t)regs_buf->r_ds);
	reg->set_register("es", (int64_t)regs_buf->r_es);
	reg->set_register("fs", (int64_t)regs_buf->r_fs);
	reg->set_register("gs", (int64_t)regs_buf->r_gs);
}

void
x86_64_freebsd_arch::register_set_to_native(char *buf, int size, RegisterSet *reg) {
	struct x86_64_freebsd_regs	*regs_buf = (struct x86_64_freebsd_regs *)buf;

	regs_buf->r_r15 = reg->get_register_as_integer("r15");
	regs_buf->r_r14 = reg->get_register_as_integer("r14");
	regs_buf->r_r13 = reg->get_register_as_integer("r13");
	regs_buf->r_r12 = reg->get_register_as_integer("r12");
	regs_buf->r_rbp = reg->get_register_as_integer("fp");
	regs_buf->r_rbx = reg->get_register_as_integer("rbx");
	regs_buf->r_r11 = reg->get_register_as_integer("r11");
	regs_buf->r_r10 = reg->get_register_as_integer("r10");
	regs_buf->r_r9 = reg->get_register_as_integer("r9");
	regs_buf->r_r8 = reg->get_register_as_integer("r8");
	regs_buf->r_rax = reg->get_register_as_integer("rax");
	regs_buf->r_rcx = reg->get_register_as_integer("rcx");
	regs_buf->r_rdx = reg->get_register_as_integer("rdx");
	regs_buf->r_rsi = reg->get_register_as_integer("rsi");
	regs_buf->r_rdi = reg->get_register_as_integer("rdi");
	regs_buf->r_rip = reg->get_register_as_integer("pc");
	regs_buf->r_cs = reg->get_register_as_integer("cs");
	regs_buf->r_rflags = reg->get_register_as_integer("eflags");
	regs_buf->r_rsp = reg->get_register_as_integer("sp");
	regs_buf->r_ss = reg->get_register_as_integer("ss");
	regs_buf->r_ds = reg->get_register_as_integer("ds");
	regs_buf->r_es = reg->get_register_as_integer("es");
	regs_buf->r_fs = reg->get_register_as_integer("fs");
	regs_buf->r_gs = reg->get_register_as_integer("gs");
}

void
x86_64_freebsd_arch::fpregister_set_from_native(char *buf, int size, RegisterSet *reg) {
	throw Exception ("Not support");
}

void
x86_64_freebsd_arch::fpregister_set_to_native(char *buf, int size, RegisterSet *reg) {
	throw Exception ("Not support");
}

i386_freebsd_arch::i386_freebsd_arch()
{
	regset_size = sizeof (struct i386_freebsd_regs);
	fpregset_size = sizeof (struct i386_freebsd_fpregs);

    // from i386 machine/reg.h, unavailable on x86_64 host
    regnames["fs"] = 0 * sizeof (unsigned int) ;
    regnames["es"] = 1 * sizeof (unsigned int) ;
    regnames["xds"] = 2 * sizeof (unsigned int) ; // XXX: xds or ds? as above
    regnames["edi"] = 3 * sizeof (unsigned int) ;
    regnames["esi"] = 4 * sizeof (unsigned int) ;
    regnames["ebp"] = 5 * sizeof (unsigned int) ;
    regnames["ebx"] = 7 * sizeof (unsigned int) ;
    regnames["edx"] = 8 * sizeof (unsigned int) ;
    regnames["ecx"] = 9 * sizeof (unsigned int) ;
    regnames["eax"] = 10 * sizeof (unsigned int) ;
    regnames["eip"] = 13 * sizeof (unsigned int) ;
    regnames["cs"] = 14 * sizeof (unsigned int) ;
    regnames["eflags"] = 15 * sizeof (unsigned int) ;
    regnames["esp"] = 16 * sizeof (unsigned int) ;
    regnames["ss"] = 17 * sizeof (unsigned int) ;
    regnames["gs"] = 18 * sizeof (unsigned int) ;



    // XXX Not supported
    st_start = sizeof (unsigned long) * 8 ;

    // XXX Not supported
    sse_start = st_start + 8 * 16 ;

    ctx_offset = sizeof (freebsd_sigset_t) + sizeof (int) ; // skip sc_mask and sc_onstack members

    reset_reg ();
}

void
i386_freebsd_arch::register_set_from_native(char *buf, int size, RegisterSet *reg) {
	struct i386_freebsd_regs	*regs_buf = (struct i386_freebsd_regs *)buf;

	reg->set_register("ebx", (int64_t)regs_buf->r_ebx);
	reg->set_register("ecx", (int64_t)regs_buf->r_ecx);
	reg->set_register("edx", (int64_t)regs_buf->r_edx);
	reg->set_register("esi", (int64_t)regs_buf->r_esi);
	reg->set_register("edi", (int64_t)regs_buf->r_edi);
	reg->set_register("fp", (int64_t)regs_buf->r_ebp);
	reg->set_register("eax", (int64_t)regs_buf->r_eax);
	reg->set_register("ds", (int64_t)regs_buf->r_ds);
	reg->set_register("es", (int64_t)regs_buf->r_es);
	reg->set_register("fs", (int64_t)regs_buf->r_fs);
	reg->set_register("gs", (int64_t)regs_buf->r_gs);
	//reg->set_register("orig_eax", (int64_t)regs_buf->orig_eax);
	reg->set_register("pc", (int64_t)regs_buf->r_eip);
	reg->set_register("cs", (int64_t)regs_buf->r_cs);
	reg->set_register("eflags", (int64_t)regs_buf->r_eflags);
	reg->set_register("sp", (int64_t)regs_buf->r_esp);
	reg->set_register("ss", (int64_t)regs_buf->r_ss);
}

void
i386_freebsd_arch::register_set_to_native(char *buf, int size, RegisterSet *reg) {
	struct i386_freebsd_regs	*regs_buf = (struct i386_freebsd_regs *)buf;

	regs_buf->r_ebx = reg->get_register_as_integer("ebx");
	regs_buf->r_ecx = reg->get_register_as_integer("ecx");
	regs_buf->r_edx = reg->get_register_as_integer("edx");
	regs_buf->r_esi = reg->get_register_as_integer("esi");
	regs_buf->r_edi = reg->get_register_as_integer("edi");
	regs_buf->r_ebp = reg->get_register_as_integer("fp");
	regs_buf->r_eax = reg->get_register_as_integer("eax");
	regs_buf->r_ds = reg->get_register_as_integer("ds");
	regs_buf->r_es = reg->get_register_as_integer("es");
	regs_buf->r_fs = reg->get_register_as_integer("fs");
	regs_buf->r_gs = reg->get_register_as_integer("gs");
	//regs_buf->orig_eax = reg->get_register_as_integer("orig_eax");
	regs_buf->r_eip = reg->get_register_as_integer("pc");
	regs_buf->r_cs = reg->get_register_as_integer("cs");
	regs_buf->r_eflags = reg->get_register_as_integer("eflags");
	regs_buf->r_esp = reg->get_register_as_integer("sp");
}

void
i386_freebsd_arch::fpregister_set_from_native(char *buf, int size, RegisterSet *reg) {
	throw Exception ("Not support");
}

void
i386_freebsd_arch::fpregister_set_to_native(char *buf, int size, RegisterSet *reg) {
	throw Exception ("Not support");
}

