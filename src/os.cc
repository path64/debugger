#include "os.h"

struct i386_linux_regs
{
  int32_t ebx;
  int32_t ecx;
  int32_t edx;
  int32_t esi;
  int32_t edi;
  int32_t ebp;
  int32_t eax;
  int32_t xds;
  int32_t xes;
  int32_t xfs;
  int32_t xgs;
  int32_t orig_eax;
  int32_t eip;
  int32_t xcs;
  int32_t eflags;
  int32_t esp;
  int32_t xss;
};

struct x86_64_linux_regs
{
  uint64_t r15;
  uint64_t r14;
  uint64_t r13;
  uint64_t r12;
  uint64_t rbp;
  uint64_t rbx;
  uint64_t r11;
  uint64_t r10;
  uint64_t r9;
  uint64_t r8;
  uint64_t rax;
  uint64_t rcx;
  uint64_t rdx;
  uint64_t rsi;
  uint64_t rdi;
  uint64_t orig_rax;
  uint64_t rip;
  uint64_t cs;
  uint64_t eflags;
  uint64_t rsp;
  uint64_t ss;
  uint64_t fs_base;
  uint64_t gs_base;
  uint64_t ds;
  uint64_t es;
  uint64_t fs;
  uint64_t gs;
};

x86_linux_os::x86_linux_os(int bit) : bit(bit) {}

void
x86_linux_os::char2regset(char *buf, RegisterSet *reg) {
	if (bit == 32) {
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
	}
	else {
		struct x86_64_linux_regs	*regs_buf = (struct x86_64_linux_regs *)buf;

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
}

void
x86_linux_os::char2fpregset(char *p, RegisterSet *reg) {
	std::vector<unsigned char>	val;

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
