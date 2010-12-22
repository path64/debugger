#ifndef _OS_H_
#define _OS_H_

#include "dbg_except.h"
#include "register_set.h"

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

struct i386_linux_fpregs
{
  int32_t cwd;
  int32_t swd;
  int32_t twd;
  int32_t fip;
  int32_t fcs;
  int32_t foo;
  int32_t fos;
  int32_t st_space [20];
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

struct x86_64_linux_fpregs
{
  uint16_t		cwd;
  uint16_t		swd;
  uint16_t		ftw;
  uint16_t		fop;
  uint64_t		rip;
  uint64_t		rdp;
  uint32_t		mxcsr;
  uint32_t		mxcr_mask;
  uint32_t		st_space[32];   /* 8*16 bytes for each FP-reg = 128 bytes */
  uint32_t		xmm_space[64];  /* 16*16 bytes for each XMM-reg = 256 bytes */
  uint32_t		padding[24];
};

struct i386_freebsd_regs {
	uint32_t	r_fs;
	uint32_t	r_es;
	uint32_t	r_ds;
	uint32_t	r_edi;
	uint32_t	r_esi;
	uint32_t	r_ebp;
	uint32_t	r_isp;
	uint32_t	r_ebx;
	uint32_t	r_edx;
	uint32_t	r_ecx;
	uint32_t	r_eax;
	uint32_t	r_trapno;
	uint32_t	r_err;
	uint32_t	r_eip;
	uint32_t	r_cs;
	uint32_t	r_eflags;
	uint32_t	r_esp;
	uint32_t	r_ss;
	uint32_t	r_gs;
};

struct i386_freebsd_fpregs {
	uint32_t	fpr_env[7];
	uint8_t		fpr_acc[8][10];
	uint32_t	fpr_ex_sw;
	uint8_t		fpr_pad[64];
};

struct x86_64_freebsd_regs {
	uint64_t	r_r15;
	uint64_t	r_r14;
	uint64_t	r_r13;
	uint64_t	r_r12;
	uint64_t	r_r11;
	uint64_t	r_r10;
	uint64_t	r_r9;
	uint64_t	r_r8;
	uint64_t	r_rdi;
	uint64_t	r_rsi;
	uint64_t	r_rbp;
	uint64_t	r_rbx;
	uint64_t	r_rdx;
	uint64_t	r_rcx;
	uint64_t	r_rax;
	uint32_t	r_trapno;
	uint16_t	r_fs;
	uint16_t	r_gs;
	uint32_t	r_err;
	uint16_t	r_es;
	uint16_t	r_ds;
	uint64_t	r_rip;
	uint64_t	r_cs;
	uint64_t	r_rflags;
	uint64_t	r_rsp;
	uint64_t	r_ss;
};

struct x86_64_freebsd_fpregs {
	uint64_t	fpr_env[4];
	uint8_t		fpr_acc[8][16];
	uint8_t		fpr_xacc[16][16];
	uint64_t	fpr_spare[12];
};

class OS {
public:
	int regset_size;
	int fpregset_size;
	virtual void char2regset(char *, int, RegisterSet *) {throw Exception ("Not support")  ;};
	virtual void regset2char(char *, int, RegisterSet *) {throw Exception ("Not support")  ;};
	virtual void char2fpregset(char *, int, RegisterSet *) {throw Exception ("Not support")  ;};
	virtual void fpregset2char(char *, int, RegisterSet *) {throw Exception ("Not support")  ;};
};

class x86_linux_os : public OS
{
private:
	int	bit;
public:
	x86_linux_os(int bit);
	void char2regset(char *, int, RegisterSet *);
	void regset2char(char *, int, RegisterSet *);
	void char2fpregset(char *, int, RegisterSet *);
	void fpregset2char(char *, int, RegisterSet *);
};

class x86_freebsd_os : public OS
{
private:
	int	bit;
public:
	x86_freebsd_os(int bit);
	void char2regset(char *, int, RegisterSet *);
	void regset2char(char *, int, RegisterSet *);
};

#endif
