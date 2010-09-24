
#if (defined(__i386__) || defined(__x86_64__)) && !defined(NO_ASM)
static void double_to_x87(double a, char *d)
{
	__asm__ (
			"fldl %1 \n"
			"fstpt %0"
			: "=m" (*d)
			: "m" (a)
			: "st");
}

static double x87_to_double(char *d)
{
	double a;
	__asm__ (
			"fldt %1 \n"
			"fstpl %0"
			: "=m" (a)
			: "m" (*d)
			: "st");
	return a;
}
#else
static double x87_to_double(char *d)
{
	// Parse the mantissa (first 64 bits, little endian)
	unsigned long long l = 0;
	for (int i=7 ; i>=0 ; i--)
	{
		l <<= 8;
		l += d[i];
	}
	// Parse the exponent and sign - next 2 bytes
	short e = (d[9] << 8) + (d[8]);
	// Exponent is the low 15 bits of the last two bytes
	e &= 0x7fff;
	// Correct for exponent bias
	e -= 0x3fff;
	double c = l;
	c /= (1ULL<<63);
	// Apply the exponent
	c *= pow(2, e);
	// If the sign bit is set, negate the result
	if (d[9] & 0x80)
	{
		c = 0-c;
	}
	return c;
}
static void double_to_x87(double a, char *d)
{
	// Get the exponent
	short exp = (short)ilogb(a);
	// Get the fraction
	double m = a / pow(2, exp);
	// Bias the exponent
	exp += 0x3fff;
	// Get the sign bit
	int sign = m < 0;
	// Make sure the exponent is only 15 bits (should be a no-op when coming
	// from a double)
	exp &= 0x7fff;
	// Pop the sign bit in the high bit of the exponent
	if (sign)
	{
		m = 0 - m;
		exp |= 0x8000;
	}
	// Calculate the mantissa
	unsigned long long mantissa = m * (1ULL<<63);
	// Store the mantissa in the first 8 bytes.
	for (int i=0 ; i<7 ; i++)
	{
		d[i] = mantissa & 0xff;
		mantissa >>= 8;
	}
	// Store the exponent and the sign bit in the next ones
	d[8] = exp & 0xff;
	d[9] = exp >> 8;
}
#endif

X87RegisterSet::X87RegisterSet(RegisterSetProperties *p) : RegisterSet(p)
{
	assert(p == X87RegisterSetProperties);
}
X87RegisterSet::X87RegisterSet() : RegisterSet(X87RegisterSetProperties) {}

int64_t X87RegisterSet::get_register_as_integer(const int num);
{
	assert(num < properties->number_of_registers());
	return (int64_t)x87_to_double(values[num]);
}
double X87RegisterSet::get_register_as_double(const int num)
{
	assert(num < properties->number_of_registers());
	return x87_to_double(values[num]);
}
std::vector<unsigned char> X87RegisterSet::get_register_as_bytes(const int num)
{
	assert(num < properties->number_of_registers());
	vector<unsigned char> b;
	unsigned char *data = (unsigned char*)&values[num];
	b.insert(b.begin(), data, data + 10);
	return b;
}
void X87RegisterSet::set_register(const int num, int64_t v)
{
	assert(num < properties->number_of_registers());
	double_to_x87((double)v, values[num]);
}
void X87RegisterSet::set_register(const int num, double v) = 0;
{
	assert(num < properties->number_of_registers());
	double_to_x87(v, values[num]);
}
void X87RegisterSet::set_register(const int num, 
                                      const std::vector<unsigned char> &v)
{
	assert(num < properties->number_of_registers());
	assert(v.size() == 10);
	std::copy(v.begin(), v.end(), (unsigned char*)values[num]);
}

