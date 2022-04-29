#ifndef TYPES_H__15_05_2009__14_37
#define TYPES_H__15_05_2009__14_37

//#ifdef _WIN32
//#ifndef WIN32
//#define WIN32
//#endif
//#endif

#ifndef __CC_ARM //WIN32

#define __packed /*__declspec(align(1))*/
#define __softfp /**/
#define __irq __declspec(naked)
#define __align(v)
#define __attribute__(v)
#define __func__ __FUNCTION__

inline void __disable_irq() {}
inline void __enable_irq() {}
//inline void __nop() {}

#define __CC_ARM
#define __TARGET_FPU_VFP

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

inline bool fIsValid(float v) { return (((u16*)&v)[2] & 0x7F80) != 0x7F80; }
inline bool dIsValid(float v) { return (((u32*)&v)[2] & 0x7FF0) != 0x7FF0; }

#define GD(adr, t, i) (*(((t*)adr)+i))
#define GB(adr,i) (*(((byte*)adr)+i))

inline void COPY(const void *src, void *dst, u32 size) { const byte *s = (const byte*)src; byte *d = (byte*)dst;  while(size--) *d++ = *s++; }

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#ifdef WIN32

inline void Read32(u32 v) {  }
inline word ReverseWord(word v) { return ((v&0x00FF)<<8 | (v&0xFF00)>>8); }
inline dword ReverseDword(dword v) { v = (v&0x00FF00FF)<<8 | (v&0xFF00FF00)>>8;	return (v&0x0000FFFF)<<16 | (v&0xFFFF0000)>>16; }
inline dword SwapDword(dword v) { return (v&0x00FF00FF)<<8 | (v&0xFF00FF00)>>8; }

#else

inline void Read32(u32 v) { u32 t; __asm { add t, v }; }
inline u16 ReverseWord(u16 v) { __asm	{ rev16 v, v };	return v; }
inline u32 ReverseDword(u32 v) { return __rev(v); }
inline dword SwapDword(dword v) { __asm { rev16 v, v }; return v; }

#endif

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


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


#endif // TYPES_H__15_05_2009__14_37
