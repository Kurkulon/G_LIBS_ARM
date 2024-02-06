#ifndef TYPES_H__15_05_2009__14_37
#define TYPES_H__15_05_2009__14_37

//#ifdef _WIN32
//#ifndef WIN32
//#define WIN32
//#endif
//#endif

#ifdef _MSC_VER //__CC_ARM //WIN32

	#define WINDOWS_IGNORE_PACKING_MISMATCH

	#include <intrin.h>

	#define __packed __declspec(align(1))
	#define __softfp /**/
	#define __irq __declspec(naked)
	#define __align(v)
	#define __attribute__(v)
	#define __func__ __FUNCTION__

	__forceinline void __breakpoint(int v) { __debugbreak(); }
	__forceinline void __disable_irq() {}
	__forceinline void __enable_irq() {}
	//inline void __nop() {}

	#define __CC_ARM
	#define __TARGET_FPU_VFP

	#if _MSC_VER > 1500
		#pragma comment(lib, "legacy_stdio_definitions.lib")
	#endif

#endif 

#ifdef _DEBUG
#define DEBUG_ASSERT(v) if (!(v)) __breakpoint(0)
#else
#define DEBUG_ASSERT(v)
#endif

typedef unsigned char byte, u8;
typedef unsigned short int word, u16;
typedef unsigned long int dword, u32;
typedef signed char i8;
typedef signed short int i16;
typedef signed long int i32;
typedef signed long long int i64;
typedef unsigned long long int u64;

#define ArraySize(x) (sizeof(x)/sizeof(x[0]))

#ifndef NAN
static const dword __NAN_dword = 0xFFFFFFFF;
#define NAN (*((const float*)(&__NAN_dword)))
#endif

inline float ABS(float v) { *((u32*)&v) &= 0x7FFFFFFF; return v; }

//inline bool fIsValid(float v) { return (((u16*)&v)[1] & 0x7F80) != 0x7F80; }
//inline bool dIsValid(double v) { return (((u32*)&v)[1] & 0x7FF0) != 0x7FF0; }

//__forceinline u32 LIM(u32 v, u32 min, u32 max) { return (v < min) ? min : ((v > max) ? max : v); }
//__forceinline u32 MIN(u32 a, u32 b) { return (a < b) ? a : b; }
//__forceinline u32 MAX(u32 a, u32 b) { return (a >= b) ? a : b; }

#define LIM(v, min, max)	(((v) < (min)) ? (min) : (((v) > (max)) ? (max) : (v)))
#define MIN(a, b)			(((a) < (b)) ? (a) : (b))
#define MAX(a, b)			(((a) >= (b)) ? (a) : (b))

#define GD(adr, t, i) (*(((t*)adr)+i))
#define GB(adr,i) (*(((byte*)adr)+i))

__forceinline void COPY(const void *src, void *dst, u32 size) { const byte *s = (const byte*)src; byte *d = (byte*)dst;  while(size--) *d++ = *s++; }

__forceinline void delay(u32 cycles) { while(cycles--) __nop();}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#ifdef WIN32

__forceinline void Read32(u32 v) {  }
__forceinline word ReverseWord(word v) { return ((v&0x00FF)<<8 | (v&0xFF00)>>8); }
__forceinline dword ReverseDword(dword v) { v = (v&0x00FF00FF)<<8 | (v&0xFF00FF00)>>8;	return (v&0x0000FFFF)<<16 | (v&0xFFFF0000)>>16; }
__forceinline dword SwapDword(dword v) { return (v&0x00FF00FF)<<8 | (v&0xFF00FF00)>>8; }

//#elif defined(__TARGET_ARCH_ARM)
//
//inline void Read32(u32 v) { u32 t; __asm { add t, v }; }
//inline u16 ReverseWord(u16 v) { __asm	{ rev16 v, v };	return v; }
//inline u32 ReverseDword(u32 v) { return __rev(v); }
//inline dword SwapDword(dword v) { __asm { rev16 v, v }; return v; }

#else

__forceinline void Read32(u32 v) { u32 t; __asm { add t, v }; }
__forceinline u16 ReverseWord(u16 v) { __asm	{ rev16 v, v };	return v; }
__forceinline u32 ReverseDword(u32 v) { return __rev(v); }
__forceinline dword SwapDword(dword v) { __asm { rev16 v, v }; return v; }

__forceinline i32 _InterlockedIncrement(volatile i32 *v) { __disable_irq(); i32 r = *v += 1; __enable_irq(); return r; }
__forceinline i32 _InterlockedDecrement(volatile i32 *v) { __disable_irq(); i32 r = *v -= 1; __enable_irq(); return r; }

#endif

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

__forceinline u32 Push_IRQ()
{
#ifndef WIN32

	register u32 t;

	register u32 primask __asm("primask");

	t = primask;

	__disable_irq();

	return t;

#else 

	return 0;

#endif
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

__forceinline void Pop_IRQ(u32 t)
{
#ifndef WIN32

	register u32 primask __asm("primask");

	primask = t;

#endif
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

union DataCRC
{
	word	w;
	byte	b[2];
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

union U16u 
{
	word w; byte b[2]; 

	U16u(word v) {w = v;}
	operator word() {return w;}
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

union U32u 
{ 
	dword d; word w[2]; byte b[4]; 

	U32u(dword v) {d = v;}
	U32u() {d = 0;}
	operator dword() {return d;}
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

union F32u
{ 
	float f; dword d; word w[2]; byte b[4]; 

	F32u (float v) {f = v;}
	operator float() {return f;}
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

union DataPointer
{
	__packed void	*v;
	__packed byte	*b;
	__packed word	*w;
	__packed dword	*d;
	__packed float	*f;

	DataPointer(__packed void *p) { v = p; } 

	void operator=(__packed void *p) { v = p; } 
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

union ConstDataPointer
{
	__packed const void		*v;
	__packed const byte		*b;
	__packed const word		*w;
	__packed const dword	*d;
	__packed const float	*f;

	ConstDataPointer(__packed const void *p) { v = p; } 

	void operator=(__packed const void *p) { v = p; } 
};

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#define __CONCAT1(s1)						s1
#define __CONCAT2(s1,s2)					s1##s2
#define __CONCAT3(s1,s2,s3)					s1##s2##s3
#define __CONCAT4(s1,s2,s3,s4)				s1##s2##s3##s4
#define __CONCAT5(s1,s2,s3,s4,s5)			s1##s2##s3##s4##s5
#define __CONCAT6(s1,s2,s3,s4,s5,s6)		s1##s2##s3##s4##s5##s6
#define __CONCAT7(s1,s2,s3,s4,s5,s6,s7)		s1##s2##s3##s4##s5##s6##s7
#define __CONCAT8(s1,s2,s3,s4,s5,s6,s7,s8)	s1##s2##s3##s4##s5##s6##s7##s8


#define CONCAT1(s1)							__CONCAT1(s1)
#define CONCAT2(s1,s2)						__CONCAT2(s1,s2)
#define CONCAT3(s1,s2,s3)					__CONCAT3(s1,s2,s3)
#define CONCAT4(s1,s2,s3,s4)				__CONCAT4(s1,s2,s3,s4)
#define CONCAT5(s1,s2,s3,s4,s5)				__CONCAT4(s1,s2,s3,s4,s5)
#define CONCAT6(s1,s2,s3,s4,s5,s6)			__CONCAT6(s1,s2,s3,s4,s5,s6)		
#define CONCAT7(s1,s2,s3,s4,s5,s6,s7)		__CONCAT7(s1,s2,s3,s4,s5,s6,s7)		
#define CONCAT8(s1,s2,s3,s4,s5,s6,s7,s8)	__CONCAT8(s1,s2,s3,s4,s5,s6,s7,s8)

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


#endif // TYPES_H__15_05_2009__14_37
