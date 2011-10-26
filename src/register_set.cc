#include "register_set.h"
#include "pstream.h"
#include <string.h>
#include <stdlib.h>

using namespace std;

RegisterSetProperties::RegisterSetProperties()
{
// 	int count = number_of_registers();
// 	const char **names = register_names();
// 	for (int i=0 ; i<count ; i++)
// 	{
// 		registerNumbers[names[i]] = i;
// 	}
}
RegisterSet* RegisterSetProperties::new_empty_register_set(void)
{
	if (is_integer(0))
	{
		return new IntegerRegisterSet(this);
	}
	return new FloatRegisterSet(this);
}

RegisterSet::RegisterSet(RegisterSetProperties *p) : properties(p) {}
RegisterSet::~RegisterSet() {}

void RegisterSet::print(PStream &os)
{
	int count = properties->number_of_registers();
	for (int i=0 ; i<count ; i++)
	{
		os.print("%%%s\t", properties->name_for_register_number(i));
		if (properties->is_integer(i))
		{
			int64_t v = get_register_as_integer(i);
			os.print("0x%016llx %12lld ", v, v);
			if (v == 0)
			{
				os.print("0");
			}
			for (int j = 63 ; j >= 0 ; j--)
			{
				int64_t bit = (v & (1LL << j)) ;
				os.print ("%c", bit ? '1' : '0') ;
			}
		}
		else
		{
			os.print("%f", get_register_as_double(i));
		}
		os.print ("\n") ;
	}
}



int64_t IntegerRegisterSet::get_register_as_integer(const int num)
{
	assert(num < properties->number_of_registers());
	return values[num];
}
double IntegerRegisterSet::get_register_as_double(const int num)
{
	assert(num < properties->number_of_registers());
	return (double)values[num];
}
std::vector<unsigned char> IntegerRegisterSet::get_register_as_bytes(const int num)
{
	assert(num < properties->number_of_registers());
	vector<unsigned char> b;
	unsigned char *data = (unsigned char*)&values[num];
	b.insert(b.begin(), data, data + properties->size_of_register());
	return b;
}
void IntegerRegisterSet::set_register(const int num, int64_t v)
{
	assert(num < properties->number_of_registers());
	values[num] = v;
	isDirty = true;
}
void IntegerRegisterSet::set_register(const int num, double v)
{
	assert(num < properties->number_of_registers());
	values[num] = (int64_t)v;
	isDirty = true;
}
void IntegerRegisterSet::set_register(const int num, 
                                      const std::vector<unsigned char> &v)
{
	assert(num < properties->number_of_registers());
	assert(v.size() == properties->size_of_register());
	std::copy(v.begin(), v.end(), (unsigned char *)&values[num]);
	isDirty = true;
}
IntegerRegisterSet::IntegerRegisterSet( RegisterSetProperties *r)
	: RegisterSet(r)
{
	values = (int64_t *)calloc(sizeof(int64_t), r->number_of_registers());
}
IntegerRegisterSet::~IntegerRegisterSet()
{
	free(values);
}
void IntegerRegisterSet::take_values_from(RegisterSet* set)
{
	assert(properties == set->get_properties());
	memcpy(values, ((IntegerRegisterSet*)set)->values,
	       properties->number_of_registers() * sizeof(int64_t));
	isDirty = true;
}


int64_t FloatRegisterSet::get_register_as_integer(const int num)
{
	assert(num < properties->number_of_registers());
	return (int64_t)values[num];
}
double FloatRegisterSet::get_register_as_double(const int num)
{
	assert(num < properties->number_of_registers());
	return values[num];
}
std::vector<unsigned char> FloatRegisterSet::get_register_as_bytes(const int num)
{
	assert(num < properties->number_of_registers());
	vector<unsigned char> b;
	unsigned char *data = (unsigned char*)&values[num];
	b.insert(b.begin(), data, data + properties->size_of_register());
	return b;
}
void FloatRegisterSet::set_register(const int num, int64_t v)
{
	assert(num < properties->number_of_registers());
	values[num] = (double)v;
	isDirty = true;
}
void FloatRegisterSet::set_register(const int num, double v)
{
	assert(num < properties->number_of_registers());
	values[num] = v;
	isDirty = true;
}
void FloatRegisterSet::set_register(const int num, 
                                      const std::vector<unsigned char> &v)
{
	assert(num < properties->number_of_registers());
	assert(v.size() == properties->size_of_register());
	std::copy(v.begin(), v.end(), (unsigned char *)&values[num]);
	isDirty = true;
}
FloatRegisterSet::FloatRegisterSet(RegisterSetProperties *r)
	: RegisterSet(r)
{
	values = (double *)calloc(sizeof(double), r->number_of_registers());
}
FloatRegisterSet::~FloatRegisterSet()
{
	free(values);
}
void FloatRegisterSet::take_values_from(RegisterSet* set)
{
	assert(properties == set->get_properties());
	memcpy(values, ((FloatRegisterSet*)set)->values,
	       properties->number_of_registers() * sizeof(double));
	isDirty = true;
}
