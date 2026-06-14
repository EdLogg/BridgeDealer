#include "stdafx.h"


//	KISS
//
//	The idea is to use simple, fast, individually promising
//	generators to get a composite that will be fast, easy to code
//	have a very long period and pass all the tests put to it.
//	The three components of KISS are:
//	x(n)=a*x(n-1)+1 mod 2^32
//	y(n)=y(n-1)(I+L^13)(I+R^17)(I+L^5),
//	z(n)=2*z(n-1)+z(n-2) +carry mod 2^32
//	The y's are a shift register sequence on 32bit binary vectors period 2^32-1;
//	The z's are a simple multiply-with-carry sequence with period 2^63+2^32-1.  
//	The period of KISS is 2^32*(2^32-1)*(2^63+2^32-1) > 2^127
//

static U32 kiss_x = 1;
static U32 kiss_y = 2;
static U32 kiss_z = 4;
static U32 kiss_w = 8;
static U32 kiss_carry = 0;
static U32 kiss_k;
static U32 kiss_m;


void	RandomSeed(U32 seed)
{
	kiss_x = seed | 1;
	kiss_y = seed | 2;
	kiss_z = seed | 4;
	kiss_w = seed | 8;
	kiss_carry = 0;
}


void	RandomInit(void)
{
	SYSTEMTIME C_time;
	S64 C_z, C_aux;
	U32	seed;

	// Make it depend on the date and tick count

	GetSystemTime(&C_time);

	C_aux = C_time.wYear;
	C_aux *= 12 * 31 * 24 * 60 * 60;
	C_z = C_aux;
	C_aux = C_time.wMonth;
	C_aux *= 31 * 24 * 60 * 60;
	C_z += C_aux;

	C_aux = C_time.wDay;
	C_aux *= 24 * 60 * 60;
	C_z += C_aux;

	C_aux = C_time.wHour;
	C_aux *= 60 * 60;
	C_z += C_aux;

	C_aux = C_time.wMinute;
	C_aux *= 60;
	C_z += C_aux;

	C_aux = C_time.wSecond;
	C_z += C_aux;

	C_z += GetTickCount();
	seed = (U32)C_z;
	RandomSeed(seed);
}


U32 Random(U32 in)
{
	kiss_x = kiss_x * 69069 + 1;
	kiss_y ^= kiss_y << 13;
	kiss_y ^= kiss_y >> 17;
	kiss_y ^= kiss_y << 5;
	kiss_k = (kiss_z >> 2) + (kiss_w >> 3) + (kiss_carry >> 2);
	kiss_m = kiss_w + kiss_w + kiss_z + kiss_carry;
	kiss_z = kiss_w;
	kiss_w = kiss_m;
	kiss_carry = kiss_k >> 30;
	U32	rand = kiss_x + kiss_y + kiss_w;
	if (in == 0)
		return rand;
	else
		return rand % in;
}

