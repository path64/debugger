#include "register_set.h"

namespace
{

class X86RegisterSetProperties : public RegisterSetProperties
{
	public:
	virtual const char *name() const { return "main"; };

	virtual int numberOfRegisters() const { return 19; };
	virtual const char** registerNames() const
	{
		static const char *names[] = { 
			// GPRs
			"eax", "ecx", "edx", "ebx", 
			// stack pointer / base
			"esp", "ebp", 
			// String operation source / destination
			"esi", "edi", 
			// Instruction pointer, 
			"eip", "eflags",
			// Segment registers
			"es", "cs", "ss", "ds", "fs", "gs",
			"isp", "err", 
			};
		return names;
	}

	virtual size_t size_of_register() const { return 4; }

	virtual bool is_integer() const { return true; }

	virtual int register_number_for_dwarf_number(int n) const
		{ return n <= 7 ? n : -1; };
};

class X87RegisterSetProperties : public RegisterSetProperties
{
	public:
	virtual const char *name() const { return "x87"; };

	virtual int number_of_registers() const { return 8; };
	virtual const char** register_names() const
	{
		static const char *names[] = { "st0", "st1", "st2", "st3", "st4", "st5",
			"st6", "st7" };
		return names;
	}

	virtual size_t size_of_register() const { return 10; }

	virtual bool is_integer() const { return false; }

	virtual int register_number_for_dwarf_number(int n) const
		{ return (n < 33 || n > 40) ? -1 : n - 33; }
};

class X86_64RegisterSetProperties : public RegisterSetProperties
{
	public:
	virtual const char *name() const { return "x86-64"; };

	virtual int get_num_registers() const { return 19; };
	virtual const char** get_register_names() const
	{
		// FIXME:
		static const char *names[] = {
			"rax", "rdx", "rcx", "rbx", "rsi", "rdi", "rbp", "rsp", "r8", "r9", "r10", 
		};
		return names;
	}

	virtual size_t size_of_register() const { return 4; }

	virtual bool is_integer() const { return true; }

	virtual int register_number_for_dwarf_number(int n) const
		{ return n <= 7 ? n : -1; };
};

}; // end anonymous namespace


static X86RegisterSetProperties x86;
RegisterSetProperties *X86RegisterProperties = &x86;

static X87RegisterSetProperties x87;
RegisterSetProperties *X87RegisterProperties = &x87;

static X86_64RegisterSetProperties x86_64;
RegisterSetProperties *X86_64RegisterProperties = &x86_64;
