#ifndef MANCH_H__10_05_2022__16_54
#define MANCH_H__10_05_2022__16_54

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

struct MRB
{
	bool	ready;
	bool	OK;
	u16		len;
	u16		maxLen;
	u16		*data;
};

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

struct MTB
{
	bool		ready;
	u16			baud;
	u16			len1;
	const u16	*data1;
	u16			len2;
	const u16	*data2;
};

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

extern void InitManRecieve();
extern void InitManTransmit();
extern void InitManTransmit2();
extern bool RcvManData(MRB *mrb);
extern bool SendManData(MTB *mtb);
extern bool SendManData2(MTB* mtb);
//extern void SetTrmBoudRate(byte i);
extern void ManRcvUpdate();
//extern void ManRcvStop();

extern bool SendMLT3(MTB *mtb);

inline u16 GetRcvManQuality() { extern u16 rcvManQuality; return rcvManQuality; }

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++





#endif // MANCH_H__10_05_2022__16_54

