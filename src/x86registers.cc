#include "register_set.h"
//XXX
// #include "x86registers.h"
// #include "x87_register_set.h"

namespace
{

class X86RegisterSetProperties : public RegisterSetProperties
{
public:
	X86RegisterSetProperties() {
		int count = number_of_registers();
		const char **names = register_names();
		for (int i=0 ; i<count ; i++)
		{
			registerNumbers[names[i]] = i;
		}
	};
	virtual const char *name() const { return "main"; };

	virtual int number_of_registers() const { return 18; };

	virtual size_t size_of_register() const { return 4; }

	virtual bool is_integer(int) const { return true; }
	virtual bool is_address(int) const { return true; }

	virtual int register_number_for_dwarf_number(int n) const
		{ return n <= 7 ? n : -1; };
	virtual int dwarf_number_for_register_number(int n) const
		{ return -1; };
	//XXX
	//virtual RegisterSet* new_empty_register_set(void) { return NULL;};
private:
	virtual const char** register_names() const
	{
		static const char *names[] = { 
			// GPRs
			"eax", "ecx", "edx", "ebx", 
			// stack pointer / base
			"sp", "fp",
			// String operation source / destination
			"esi", "edi", 
			// Instruction pointer, 
			"pc", "eflags",
			// Segment registers
			"es", "cs", "ss", "ds", "fs", "gs",
			"isp", "err", 
			};
		return names;
	}
};

class X87RegisterSetProperties : public RegisterSetProperties
{
public:
	X87RegisterSetProperties(){
		int count = number_of_registers();
		const char **names = register_names();
		for (int i=0 ; i<count ; i++)
		{
			registerNumbers[names[i]] = i;
		}
	};
	virtual const char *name() const { return "x87"; };

	virtual int number_of_registers() const { return 8; };

	virtual size_t size_of_register() const { return 10; }

	virtual bool is_integer(int) const { return true; }
	virtual bool is_address(int) const { return true; }

	virtual bool is_integer() const { return false; }

	virtual int register_number_for_dwarf_number(int n) const
		{ return (n < 33 || n > 40) ? -1 : n - 33; }
	virtual int dwarf_number_for_register_number(int n) const
		{ return -1; };
	//XXX
	//virtual RegisterSet* new_empty_register_set(void) { return NULL;};
private:
	virtual const char** register_names() const
	{
		static const char *names[] = { "st0", "st1", "st2", "st3", "st4", "st5",
			"st6", "st7" };
		return names;
	}
};

class X86_64RegisterSetProperties : public RegisterSetProperties
{
public:
	X86_64RegisterSetProperties(){
		int count = number_of_registers();
		const char **names = register_names();
		for (int i=0 ; i<count ; i++)
		{
			registerNumbers[names[i]] = i;
		}
	};
	virtual const char *name() const { return "x86-64"; };

	virtual int number_of_registers() const { return 24; };

	virtual size_t size_of_register() const { return 4; }

	virtual bool is_integer(int) const { return true; }
	virtual bool is_address(int) const { return true; }

	virtual int register_number_for_dwarf_number(int n) const
		{ return n <= 7 ? n : -1; };
	virtual int dwarf_number_for_register_number(int n) const
		{ return -1; };
	//XXX
	//virtual RegisterSet* new_empty_register_set(void) { return NULL;};

private:
	virtual const char** register_names() const
	{
		// FIXME:
		static const char *names[] = {
			"rax", "rcx", "rdx", "rbx", "rsi", "rdi", "fp",
			"sp", "pc", "cs", "ss", "ds", "es", "fs", "gs",
			"eflags", "r8", "r9", "r10", "r11", "r12", "r13",
			"r14", "r15",
		};
		return names;
	}
};

}; // end anonymous namespace


static X86RegisterSetProperties x86;
RegisterSetProperties *X86RegisterProperties = &x86;

static X87RegisterSetProperties x87;
RegisterSetProperties *X87RegisterProperties = &x87;

static X86_64RegisterSetProperties x86_64;
RegisterSetProperties *X86_64RegisterProperties = &x86_64;
