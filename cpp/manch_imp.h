#ifndef MANCH_IMP_H__10_05_2022__16_55
#define MANCH_IMP_H__10_05_2022__16_55

#include "types.h"
#include "core.h"
#include "SEGGER_RTT.h"


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// ++++++++++++++	MANCH TRANSMIT	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#ifdef CPU_SAME53	//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

	#if defined(MANT_TC) && defined(MAN_TRANSMIT_V1)

		#define MNTTC						HW::MANT_TC
		#define MANT_GEN					CONCAT2(GEN_,MANT_TC)
		#define MANT_GEN_CLK				CONCAT2(CLK_,MANT_TC) 
		#define MANT_IRQ					CONCAT2(MANT_TC,_IRQ)
		#define GCLK_MANT					CONCAT2(GCLK_,MANT_TC)
		#define PID_MANT					CONCAT2(PID_,MANT_TC)

		#if (MANT_GEN_CLK > 100000000)
				#define MANT_PRESC_NUM		64
		#elif (MANT_GEN_CLK > 50000000)
				#define MANT_PRESC_NUM		16
		#elif (MANT_GEN_CLK > 20000000)
				#define MANT_PRESC_NUM		8
		#elif (MANT_GEN_CLK > 10000000)
				#define MANT_PRESC_NUM		4
		#elif (MANT_GEN_CLK > 5000000)
				#define MANT_PRESC_NUM		2
		#else
				#define MANT_PRESC_NUM		1
		#endif

		#define MANT_PRESC_DIV				CONCAT2(TC_PRESCALER_DIV,MANT_PRESC_NUM)

		#define ManResetTransmit()			{ MNTTC->CTRLA = TC_SWRST; while(MNTTC->SYNCBUSY); }
		#define ManDisableTransmit()		{ MNTTC->CTRLA = 0; MNTTC->INTENCLR = ~0; }
		#define ManEndIRQ()					{ MNTTC->INTFLAG = ~0; }

	#elif defined(MANT_TCC)

		#define MNTTCC						HW::MANT_TCC
		#define MANT_GEN					CONCAT2(GEN_,MANT_TCC)
		#define MANT_GEN_CLK				CONCAT2(CLK_,MANT_TCC) 
		#define MANT_IRQ					CONCAT2(MANT_TCC,_0_IRQ)
		#define GCLK_MANT					CONCAT2(GCLK_,MANT_TCC)
		#define PID_MANT					CONCAT2(PID_,MANT_TCC)
	
		#if (MANT_GEN_CLK > 100000000)
				#define MANT_PRESC_NUM		64
		#elif (MANT_GEN_CLK > 50000000)
				#define MANT_PRESC_NUM		16
		#elif (MANT_GEN_CLK > 20000000)
				#define MANT_PRESC_NUM		8
		#elif (MANT_GEN_CLK > 10000000)
				#define MANT_PRESC_NUM		4
		#elif (MANT_GEN_CLK > 5000000)
				#define MANT_PRESC_NUM		2
		#else
				#define MANT_PRESC_NUM		1
		#endif

		#define MANT_PRESC_DIV				CONCAT2(TCC_PRESCALER_DIV,MANT_PRESC_NUM)
		#define US2MT(v)					(((v)*(MANT_GEN_CLK/MANT_PRESC_NUM)+500000)/1000000)

		#define ManResetTransmit()			{ MNTTCC->CTRLA = TC_SWRST; while(MNTTCC->SYNCBUSY); }
		#define ManDisableTransmit()		{ MNTTCC->CTRLA = 0; MNTTCC->INTENCLR = ~0; }
		#define ManEndIRQ()					{ MNTTCC->INTFLAG = ~0; }

		#define MANT_CC_NUM					CONCAT2(MANT_TCC,_CC_NUM)

		#define L1_CC_NUM					(L1_WO_NUM % MANT_CC_NUM)
		#define L2_CC_NUM					(L2_WO_NUM % MANT_CC_NUM)

	#else
		#error  Must defined MANT_TC or MANT_TCC
	#endif
	
	#define BAUD2CLK(x)						((u32)(MANT_GEN_CLK/MANT_PRESC_NUM/(x)+0.5))

	inline void MANTT_ClockEnable()			{ HW::GCLK->PCHCTRL[GCLK_MANT] = MANT_GEN|GCLK_CHEN; HW::MCLK->ClockEnable(PID_MANT); }

#elif defined(CPU_XMC48) //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

	#if (SYSCLK_MHz > 100)
			#define MANRT_PSC		5
	#elif (SYSCLK_MHz > 50)
			#define MANRT_PSC		4
	#elif (SYSCLK_MHz > 20)
			#define MANRT_PSC		3
	#elif (SYSCLK_MHz > 10)
			#define MANRT_PSC		2
	#elif (SYSCLK_MHz > 5)
			#define MANRT_PSC		1
	#else
			#define MANRT_PSC		0
	#endif

	#define US2MR(v)			(((v)*(SYSCLK/(1UL<<MANRT_PSC))+500000)/1000000)
	#define US2MI(v)			(((v)*(SYSCLK/(1UL<<MANRT_PSC))+500000)/1000000)
	#define BAUD2CLK(x)			((u32)((SYSCLK*1.0/(1UL<<MANRT_PSC))/x+0.5))

	#define MANCCU_PID			CONCAT2(PID_,MAN_CCU)

	#define MANCCU				HW::MAN_CCU
	#define __MANTCC			CONCAT3(MAN_CCU,_CC4,MANT_CC)
	#define __MANICC			CONCAT3(MAN_CCU,_CC4,MANI_CC)
	#define __MANRCC			CONCAT3(MAN_CCU,_CC4,MANR_CC)

	#define MANTCC				HW::__MANTCC			
	#define MANICC				HW::__MANICC			
	#define MANRCC				HW::__MANRCC			
	
	#define MANT_GCSS			CONCAT3(CCU4_S,MANT_CC,SE)
	#define MANT_GIDLC			CONCAT3(CCU4_S,MANT_CC,I)
	#define MANR_GCSS			CONCAT3(CCU4_S,MANR_CC,SE)
	#define MANR_GIDLC			CONCAT3(CCU4_S,MANR_CC,I)
	#define MANI_GCSS			CONCAT3(CCU4_S,MANI_CC,SE)
	#define MANI_GIDLC			CONCAT3(CCU4_S,MANI_CC,I)
	
	#define MANT_IRQ			CONCAT4(MAN_CCU,_,MANT_CC,_IRQn)
	#define MANR_IRQ			CONCAT4(MAN_CCU,_,MANR_CC,_IRQn)

	#define MANICC_SRS			CC4_POSR(MANI_CC);
	#define MANRCC_SRS			CC4_E0SR(MANR_CC);

								//P1_10_CCU41_IN2
	#define MANICC_EV0IS		CONCAT7(PORT_MANRXD,_,PIN_MANRXD,_,MAN_CCU,_IN,MANI_CC)
	#define MANRCC_INS			(CC4_EV0IS(12+MANI_CC) | CC4_EV0EM_RISING_EDGE)	
	#define MANICC_INS			(CC4_EV0IS(MANICC_EV0IS) | CC4_EV0EM_BOTH_EDGES | CC4_LPF0M_7CLK)

	#define ManResetTransmit()		
	#define ManEndIRQ()	

	inline void ManDisableTransmit()
	{ 
		HW::SCU_GENERAL->CCUCON &= ~ManT_CCUCON; 
		ManT1->INTE = 0; 
		ManT1->TCCLR = ManT2->TCCLR = ManT3->TCCLR = CC8_TRBC; 
		ManT_CCU8->GCSS = ManT_OUT_GCSS;	
		ManT_CCU8->GCSC = ManT_OUT_GCSC; 
	}

#endif	// #elif defined(CPU_XMC48)

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#ifdef MAN_TRANSMIT_V1

	inline void ManDisable()	{ PIO_MANCH->CLR(L1|L2);	PIO_MANCH->SET(H1|H2);							} 
	inline void ManZero()		{ PIO_MANCH->CLR(L2);		PIO_MANCH->SET(L1|H1);		PIO_MANCH->CLR(H2);	} 
	inline void ManOne()		{ PIO_MANCH->CLR(L1);		PIO_MANCH->SET(L2|H2);		PIO_MANCH->CLR(H1);	} 
	inline void ManDischarge()	{ PIO_MANCH->CLR(L1|L2);	PIO_MANCH->CLR(H1|H2);							} 

#else	// #ifdef MAN_TRANSMIT_V1

	#ifdef CPU_SAME53	//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

		#define ManT_SET_PR(v)			{ MNTTCC->PERBUF = (v); }
		#define ManT_SET_CR(v)			{ MNTTCC->CCBUF[L1_CC_NUM] = (v); MNTTCC->CCBUF[L2_CC_NUM] = (v); }
		#define ManT_SHADOW_SYNC()			

	#elif defined(CPU_XMC48) //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

		#define ManT_SET_PR(v)			{ ManT1->PRS = (v); ManT2->PRS = (v); ManT3->PRS = (v); }
		#define ManT_SET_CR(v)			{ ManT1->CR2S = (v); ManT2->CR1S = (v); ManT2->CR2S = (v); ManT3->CR1S = (v);}
		#define ManT_SHADOW_SYNC()		{ ManT_CCU8->GCSS = ManT_CCU8_GCSS; }	

	#endif

	static u16 trmHalfPeriod = (BAUD2CLK(20833)+1)/2;
	static u16 trmHalfPeriod2 = BAUD2CLK(20833);
	static u16 trmHalfPeriod3 = (BAUD2CLK(20833)*3+1)/2;
	static u16 trmHalfPeriod4 = BAUD2CLK(20833) * 2;
	static u16 trmHalfPeriod6 = BAUD2CLK(20833) * 3;
	static u16 trmHalfPeriod7 = (BAUD2CLK(20833) * 7 + 1) / 2;

#endif	// #else	// #ifdef MAN_TRANSMIT_V1

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifndef WIN32


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static const u16 manbaud[5] = { BAUD2CLK(20833), BAUD2CLK(41666), BAUD2CLK(62500), BAUD2CLK(83333), BAUD2CLK(104166) };//0:20833Hz, 1:41666Hz,2:62500Hz,3:83333Hz


#endif 

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static byte stateManTrans = 0;
static MTB *manTB = 0;
static bool trmBusy = false;
static bool trmTurbo = false;
static bool rcvBusy = false;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifndef WIN32

static u16 GetTrmBaudRate(byte i)
{
	if (i >= ArraySize(manbaud)) { i = ArraySize(manbaud) - 1; };

	return (manbaud[i]+1)/2;
}
#endif
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifndef WIN32

inline u16 CheckParity(u16 x)
{
	u16 y = x ^ (x >> 1);

	y = y ^ (y >> 2);
	y = y ^ (y >> 4);
	
	return (y ^ (y >> 8))^1;
}

#endif

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#ifdef MAN_TRANSMIT_V1

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#ifndef WIN32

#pragma push
#pragma O3
#pragma Otime

static __irq void ManTrmIRQ()
{
	static u32 tw = 0;
	static u16 count = 0;
	static byte i = 0;
	static const u16 *data = 0;
	static u16 len = 0;
	//const bool cmd = false;

	Pin_ManTrmIRQ_Set();

	switch (stateManTrans)
	{
		case 0:	// Idle; 

			ManDisable();
		
			data = manTB->data1;
			len = manTB->len1;
			stateManTrans = 1;

			break;

		case 1: // Start data

			i = 3;
			tw = ((u32)(*data) << 1) | (CheckParity(*data) & 1);

			data++;
			len--;

			ManOne();

			stateManTrans++;

			break;

		case 2:	// Wait data 1-st sync imp

			i--;

			if (i == 0)
			{
				stateManTrans++;
				ManZero();
				i = 3;
			};

			break;

		case 3: // Wait 2-nd sync imp

			i--; 

			if (i == 0)
			{
				stateManTrans++;
				count = 17;

				if (tw & 0x10000) { ManZero(); } else { ManOne(); };
			};

			break;

		case 4: // 1-st half bit wait

//			HW::PIOE->SODR = 1;

			if (tw & 0x10000) { ManOne(); } else { ManZero(); };

			count--;

			if (count == 0)
			{
				if (len > 0)
				{
					stateManTrans = 1;
				}
				else if (manTB->len2 != 0 && manTB->data2 != 0)
				{
					len = manTB->len2;
					data = manTB->data2;

					manTB->len2 = 0;
					manTB->data2 = 0;

					stateManTrans = 1;
				}
				else
				{
					stateManTrans = 6;
				};
			}
			else
			{
				stateManTrans++;
			};

			break;

		case 5: // 2-nd half bit wait

			tw <<= 1;
			stateManTrans = 4;
			if (tw & 0x10000) { ManZero(); } else { ManOne(); };

			break;

		case 6:

			if (tw & 0x10000) { ManZero(); } else { ManOne(); };

			stateManTrans++;

			break;

		case 7:

			ManDisable();
			stateManTrans = 0;

			#ifdef CPU_SAME53	
				ManDisableTransmit();
			#elif defined(CPU_XMC48)
				MANTCC->TCCLR = CC4_TRBC;
			#endif

			manTB->ready = true;
			trmBusy = false;

			break;


	}; // 	switch (stateManTrans)


	ManEndIRQ();

	Pin_ManTrmIRQ_Clr();
}

#pragma pop

#endif

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

bool SendManData(MTB *mtb)
{
#ifndef WIN32

	if (trmBusy || rcvBusy || mtb == 0 || mtb->data1 == 0 || mtb->len1 == 0)
	{
		return false;
	};

	mtb->ready = false;

	manTB = mtb;

	stateManTrans = 0;

	#ifdef CPU_SAME53	


#ifdef MNTTC
		MNTTC->CTRLA = TC_MODE_COUNT8|MANT_PRESC_DIV;
		MNTTC->WAVE = TC_WAVEGEN_NPWM;
		MNTTC->PER8 = GetTrmBaudRate(mtb->baud) - 1; //trmHalfPeriod-1;

		MNTTC->INTENCLR = ~TC_OVF;
		MNTTC->INTENSET = TC_OVF;

		MNTTC->INTFLAG = ~0;

		MNTTC->CTRLA |= TC_ENABLE;
		MNTTC->CTRLBSET = TC_CMD_RETRIGGER;
#else

		MNTTCC->CTRLA = MANT_PRESC_DIV;
		MNTTCC->WAVE = TCC_WAVEGEN_NPWM;

		MNTTCC->PER = GetTrmBaudRate(mtb->baud) - 1;

		MNTTCC->INTENCLR = ~TCC_OVF;
		MNTTCC->INTENSET = TCC_OVF;

		MNTTCC->INTFLAG = ~0;

		MNTTCC->CTRLA |= TCC_ENABLE;
#endif

	#elif defined(CPU_XMC48)

		MANTCC->PRS = GetTrmBaudRate(mtb->baud)-1; //trmHalfPeriod - 1;
		MANTCC->PSC = MANRT_PSC; //0.08us

		MANCCU->GCSS = MANT_GCSS;  

		MANTCC->SWR = ~0;
		MANTCC->INTE = CC4_PME;

		MANTCC->TCSET = CC4_TRBS;

	#endif

	return trmBusy = true;

#else

	mtb->ready = true;

	return true;

#endif
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#ifndef WIN32

void InitManTransmit()
{
	using namespace HW;

	VectorTableExt[MANT_IRQ] = ManTrmIRQ;
	CM4::NVIC->CLR_PR(MANT_IRQ);
	CM4::NVIC->SET_ER(MANT_IRQ);

#ifdef CPU_SAME53	

	MANTT_ClockEnable();

	PIO_MANCH->DIRSET = L1|H1|L2|H2;

	ManResetTransmit();

#elif defined(CPU_XMC48)

	HW::CCU_Enable(MANCCU_PID);

	MANCCU->GCTRL = 0;

	MANCCU->GIDLC = MANT_GIDLC|CCU4_PRB;

	MANTCC->PRS = GetTrmBaudRate(0)-1;
	MANTCC->PSC = MANRT_PSC; //0.08us

	MANCCU->GCSS = MANT_GCSS;  

	MANTCC->SRS = 0;

	MANTCC->SWR = ~0;
	MANTCC->INTE = CC4_PME;

#endif

	ManDisable();
}

#endif

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#else // #ifdef MAN_TRANSMIT_V1

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifndef WIN32

#pragma push
#pragma O3
#pragma Otime

static __irq void ManTrmIRQ2()
{
	static u32 tw = 0;
	static u16 count = 0;
	//static byte i = 0;
	static const u16* data = 0;
	static u16 len = 0;
	static bool cmd = false;

	Pin_ManTrmIRQ_Set();

	switch (stateManTrans)
	{
		case 0:	// 1-st sync imp 

			data = manTB->data1;
			len = manTB->len1;
			cmd = false;
			stateManTrans++;

		case 1:

			if (len == 0)
			{
				data = manTB->data2;
				len = manTB->len2;
				manTB->len2 = 0;
			};

			if (len != 0)
			{
				tw = ((u32)(*data) << 1) | (CheckParity(*data) & 1);

				data++;
				len--;

				count = 17;

				u32 tadd = (cmd) ? trmHalfPeriod : 0;

				ManT_SET_CR(trmHalfPeriod3+tadd);

				if (tw & 0x10000)
				{
					ManT_SET_PR(trmHalfPeriod7+tadd); //US2MT(96);
					stateManTrans += 2;
				}
				else
				{
					ManT_SET_PR(trmHalfPeriod6+tadd); //US2MT(72);
					stateManTrans++;
				};

				ManT_SHADOW_SYNC();

				tw <<= 1;
				count--;
			}
			else
			{
				stateManTrans = 4;
			};

			break;

		case 2:	

			ManT_SET_CR(trmHalfPeriod);

			if (count == 0)
			{
				ManT_SET_PR(trmHalfPeriod2);
				cmd = false;
				stateManTrans = 1;
			}
			else
			{
				if (tw & 0x10000)
				{
					ManT_SET_PR(trmHalfPeriod3);

					if (count == 1)
					{
						cmd = true;
						stateManTrans = 1;
					}
					else
					{
						stateManTrans++;
					};
				}
				else
				{
					ManT_SET_PR(trmHalfPeriod2);
				};

				tw <<= 1;
				count--;
			};

			ManT_SHADOW_SYNC();

			break;

		case 3: 

			if (tw & 0x10000)
			{
				ManT_SET_CR(trmHalfPeriod);
				ManT_SET_PR(trmHalfPeriod2);

				tw <<= 1;
				count--;

				if (count == 0)
				{
					cmd = true;
					stateManTrans = 1;
				};
			}
			else
			{
				tw <<= 1;
				count--;

				ManT_SET_CR(trmHalfPeriod2);

				if (tw & 0x10000)
				{
					ManT_SET_PR(trmHalfPeriod4);
					
					if (count == 1)
					{
						cmd = true;
						stateManTrans = 1;
					};
				}
				else
				{
					ManT_SET_PR(trmHalfPeriod3);

					if (count == 0)
					{
						cmd = false;
						stateManTrans = 1;
					}
					else
					{
						stateManTrans--;
					}
				};

//				if (count != 0)
				{
					tw <<= 1;
					count--;
				};
			};

			ManT_SHADOW_SYNC();

			break;

		case 4:

			stateManTrans++;
			break;

		case 5:

			stateManTrans = 0;

			ManDisableTransmit();

			manTB->ready = true;
			trmBusy = false;

			break;


	}; // 	switch (stateManTrans)

	ManEndIRQ();

	Pin_ManTrmIRQ_Clr();
}

#pragma pop

#endif
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

bool SendManData(MTB* mtb)
{
	#ifndef WIN32

		if (trmBusy || rcvBusy || mtb == 0 || mtb->data1 == 0 || mtb->len1 == 0)
		{
			return false;
		};

		mtb->ready = false;

		manTB = mtb;

		trmHalfPeriod = GetTrmBaudRate(mtb->baud);
		trmHalfPeriod2 = trmHalfPeriod * 2 - 1;
		trmHalfPeriod3 = trmHalfPeriod * 3 - 1;
		trmHalfPeriod4 = trmHalfPeriod * 4 - 1;
		trmHalfPeriod6 = trmHalfPeriod * 6 - 1;
		trmHalfPeriod7 = trmHalfPeriod * 7 - 1;

		stateManTrans = 0;

		#ifdef CPU_SAME53	

			MNTTCC->CTRLA = MANT_PRESC_DIV;
			MNTTCC->WAVE = TCC_WAVEGEN_NPWM;//|TCC_POL0;
			MNTTCC->DRVCTRL = TCC_INVEN0 << L2_WO_NUM;

			MNTTCC->PER = US2MT(50)-1;
			MNTTCC->CC[L1_CC_NUM] = 0; 
			MNTTCC->CC[L2_CC_NUM] = ~0; 

			MNTTCC->EVCTRL = 0;

			MNTTCC->INTENCLR = ~0;
			MNTTCC->INTENSET = TCC_OVF;
			MNTTCC->INTFLAG = TCC_OVF;

			MNTTCC->CTRLA |= TCC_ENABLE;
			MNTTCC->CTRLBSET = TCC_CMD_RETRIGGER;

			PIO_MANCH->SetWRCONFIG(L1|L2, MANCH_PMUX|PORT_WRPMUX|PORT_WRPINCFG|PORT_PMUXEN);
			PIO_MANCH->CLR(L1|L2);
			PIO_MANCH->DIRSET = L1|L2;

		#elif defined(CPU_XMC48)

			ManT_SET_PR(US2MT(50)-1); //trmHalfPeriod - 1;
			ManT1->CR2S = (~0); ManT2->CR1S = (0); ManT2->CR2S = (0); ManT3->CR1S = (~0);

			ManT1->PSC = ManRT_PSC; //0.08us
			ManT2->PSC = ManRT_PSC; //0.08us
			ManT3->PSC = ManRT_PSC; //0.08us

			//ManT1->PSL = ManT1_PSL;
			//ManT2->PSL = ManT2_PSL;
			//ManT3->PSL = ManT3_PSL;

			ManT1->INS = CC8_EV0IS(7) | CC4_EV0EM_RISING_EDGE;
			ManT2->INS = CC8_EV0IS(7) | CC4_EV0EM_RISING_EDGE;
			ManT3->INS = CC8_EV0IS(7) | CC4_EV0EM_RISING_EDGE;

			ManT1->CMC = CC4_STRTS_EVENT0;
			ManT2->CMC = CC4_STRTS_EVENT0;
			ManT3->CMC = CC4_STRTS_EVENT0;
			
			ManT1->TC = CC8_STRM;
			ManT2->TC = CC8_STRM;
			ManT3->TC = CC8_STRM;

			ManT1->SWR = ~0;
			ManT2->SWR = ~0;
			ManT3->SWR = ~0;

			ManT1->CHC = ManT1_CHC;
			ManT2->CHC = ManT2_CHC;
			ManT3->CHC = ManT3_CHC;

			ManT1->INTE = CC8_PME;
			ManT2->INTE = 0;
			ManT3->INTE = 0;

			ManT_CCU8->GCSS = ManT_CCU8_GCSS;
			ManT_CCU8->GIDLC = ManT_CCU8_GIDLC;

			HW::SCU_GENERAL->CCUCON |= ManT_CCUCON;

		#endif

		return trmBusy = true;

	#else

		mtb->ready = true;

		return true;

	#endif
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifndef WIN32

void InitManTransmit()
{
	using namespace HW;

	SEGGER_RTT_WriteString(0, RTT_CTRL_TEXT_BRIGHT_WHITE "Manchester Transmiter Init ... ");

#ifdef CPU_SAME53	

	VectorTableExt[MANT_IRQ] = ManTrmIRQ2;
	CM4::NVIC->CLR_PR(MANT_IRQ);
	CM4::NVIC->SET_ER(MANT_IRQ);

	MANTT_ClockEnable();

	PIO_MANCH->DIRSET = L1|H1|L2|H2;
	PIO_MANCH->CLR(L1|L2);
	PIO_MANCH->SET(H1|H2);
	PIO_MANCH->SetWRCONFIG(L1|L2, PORT_WRPINCFG);

	ManResetTransmit();

#elif defined(CPU_XMC48)

	VectorTableExt[MANT_CCU8_IRQ] = ManTrmIRQ2;
	CM4::NVIC->CLR_PR(MANT_CCU8_IRQ);
	CM4::NVIC->SET_ER(MANT_CCU8_IRQ);

	HW::CCU_Enable(ManT_CCU8_PID);

	ManT_CCU8->GCTRL = 0;
	ManT_CCU8->GIDLC = ManT_CCU8_GIDLC;
	ManT_CCU8->GCSS = ManT_OUT_GCSS;
	ManT_CCU8->GCSC = ManT_OUT_GCSC;

	PIO_MANCH->ModePin(PIN_L1, A3PP);
	PIO_MANCH->ModePin(PIN_H1, A3PP);
	PIO_MANCH->ModePin(PIN_L2, A3PP);
	PIO_MANCH->ModePin(PIN_H2, A3PP);

	//ManT_CCU8->GIDLS = ManT_CCU8_GIDLS;

	//ManT1->PRS = US2MT(100) - 1;
	//ManT1->CR1S = US2MT(50) - 1;
	//ManT1->CR2S = US2MT(50) - 1;

	//ManT1->PSC = ManT_PSC; 
	//ManT2->PSC = ManT_PSC;
	//ManT3->PSC = ManT_PSC; 

	////ManT->CHC = CC8_;

	////ManT->TCSET = CC8_TRBS;

	////ManT1->PSL = ManT1_PSL;
	////ManT2->PSL = ManT2_PSL;
	////ManT3->PSL = ManT3_PSL;

	//ManT1->INS = CC8_EV0IS(7) | CC4_EV0EM_RISING_EDGE;
	//ManT2->INS = CC8_EV0IS(7) | CC4_EV0EM_RISING_EDGE;

	//ManT1->CMC = CC4_STRTS_EVENT0;
	//ManT2->CMC = CC4_STRTS_EVENT0;
	//ManT3->CMC = CC4_STRTS_EVENT0;

	ManT1->CHC = ManT1_CHC;
	ManT2->CHC = ManT2_CHC;
	ManT3->CHC = ManT3_CHC;

	//ManT_CCU8->GCSS = ManT_CCU8_GCSS;

	//ManT1->SRS = 0;

	//ManT1->SWR = ~0;
	//ManT_CCU8->GIDLC = ManT_CCU8_GIDLC;
	//ManT->INTE = CC8_PME;
#endif

	SEGGER_RTT_WriteString(0, "OK\n");
}

#endif

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#endif // #else // #ifdef MAN_TRANSMIT_V1

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// ++++++++++++++	MANCH RECIEVER	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#ifdef CPU_SAME53	//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

	#define MANR_GEN			CONCAT2(GEN_,MANR_TCC)
	#define MANR_GEN_CLK		CONCAT2(CLK_,MANR_TCC) 
	#define MANR_IRQ			CONCAT2(MANR_TCC,_1_IRQ)

	#if (MANR_GEN_CLK > 100000000)
			#define MANR_PRESC_NUM		64
	#elif (MANR_GEN_CLK > 50000000)
			#define MANR_PRESC_NUM		16
	#elif (MANR_GEN_CLK > 20000000)
			#define MANR_PRESC_NUM		8
	#elif (MANR_GEN_CLK > 10000000)
			#define MANR_PRESC_NUM		4
	#elif (MANR_GEN_CLK > 5000000)
			#define MANR_PRESC_NUM		2
	#else
			#define MANR_PRESC_NUM		1
	#endif

	#define MANR_PRESC_DIV		CONCAT2(TCC_PRESCALER_DIV,MANR_PRESC_NUM)
	#define US2MR(v)			(((v)*(MANR_GEN_CLK/MANR_PRESC_NUM)+500000)/1000000)

	#define MANI_GEN			CONCAT2(GEN_,MANI_TC)
	#define MANI_GEN_CLK		CONCAT2(CLK_,MANI_TC) 

	#if (MANI_GEN_CLK > 100000000)
			#define MANI_PRESC_NUM		64
	#elif (MANI_GEN_CLK > 50000000)
			#define MANI_PRESC_NUM		16
	#elif (MANI_GEN_CLK > 20000000)
			#define MANI_PRESC_NUM		8
	#elif (MANI_GEN_CLK > 10000000)
			#define MANI_PRESC_NUM		4
	#elif (MANI_GEN_CLK > 5000000)
			#define MANI_PRESC_NUM		2
	#else
			#define MANI_PRESC_NUM		1
	#endif

	#define MANI_PRESC_DIV		CONCAT2(TC_PRESCALER_DIV,MANI_PRESC_NUM)
	#define US2MI(v)			(((v)*(MANI_GEN_CLK/MANI_PRESC_NUM)+500000)/1000000)

	#define MANIT_EVSYS_USER	CONCAT3(EVSYS_USER_, MANI_TC, _EVU)
	#define MANRT_EVENT_GEN		CONCAT3(EVGEN_, MANI_TC, _OVF)
	#define MANRT_EVSYS_USER	CONCAT3(EVSYS_USER_,MANR_TCC,_MC_0)

	#define MNITC				HW::MANI_TC
	#define MNRTCC				HW::MANR_TCC

	#define MANR_EXTINT			(PIN_RXD&15)

	inline void MANRT_ClockEnable()  { HW::GCLK->PCHCTRL[CONCAT2(GCLK_,MANR_TCC)]	= MANR_GEN|GCLK_CHEN; HW::MCLK->ClockEnable(CONCAT2(PID_,MANR_TCC)); }
	inline void MANIT_ClockEnable()  { HW::GCLK->PCHCTRL[CONCAT2(GCLK_,MANI_TC)]	= MANI_GEN|GCLK_CHEN; HW::MCLK->ClockEnable(CONCAT2(PID_,MANI_TC)); }

#elif defined(CPU_XMC48)	//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


	//#define MANR_EXTINT			(PIN_RXD&15)

//	inline void MANRT_ClockEnable()  { HW::GCLK->PCHCTRL[CONCAT2(GCLK_,MANR_TCC)]	= MANR_GEN|GCLK_CHEN; HW::MCLK->ClockEnable(CONCAT2(PID_,MANR_TCC)); }
//	inline void MANIT_ClockEnable()  { HW::GCLK->PCHCTRL[CONCAT2(GCLK_,MANI_TC)]	= MANI_GEN|GCLK_CHEN; HW::MCLK->ClockEnable(CONCAT2(PID_,MANI_TC)); }

#elif defined(WIN32)	//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

	#define US2MR(v)			(v)

#endif	//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//static u16 rcvCount = 0;
byte stateManRcvr = 0;

const u16 rcvPeriod = BAUD2CLK(20833);

static u16* rcvManPtr = 0;
static u16 rcvManCount = 0;

static u16 rcvManLen = 0;

static MRB *manRB = 0;

static u16 rcvManLen12 = 0;
static u32 rcvManSum12 = 0;
static u16 rcvManCount12 = 0;
//u32 rcvManMax12 = 0;
u16 rcvManQuality = 0;
static CTM32 manRcvTime;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void ManRcvEnd(bool ok)
{
#ifdef CPU_SAME53	
	MNRTCC->INTENCLR = ~0;
#elif defined(CPU_XMC48)
	MANRCC->INTE = 0;
#endif

	manRB->OK = ok;
	manRB->ready = true;
	manRB->len = rcvManLen;

	rcvManLen12 = (rcvManCount12 != 0) ? (rcvManSum12 / rcvManCount12) : 0;
	rcvManQuality = (rcvManLen12 > US2MR(12)) ? 0 : (((US2MR(12) - rcvManLen12) * 100 + US2MR(6))/US2MR(12));
	
	//rcvManQuality = (rcvManMax12 > US2MR(12)) ? 0 : (((US2MR(12) - rcvManMax12) * 100 + US2MR(6))/US2MR(12));

	rcvBusy = false;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifndef WIN32

#pragma push
#pragma O3
#pragma Otime

static __irq void ManRcvIRQ2()
{
	using namespace HW;

	static u32 _number = 0;
	static u32 _length = 0;
	static u32 _data = 0;
	static bool _command = false;
	static bool _parity_temp = false;
	const bool _parity = true;
	static bool _sync = false;
	static bool _state = false;

	Pin_ManRcvIRQ_Set();

	#ifdef CPU_SAME53	

		u32 len = MNRTCC->CC[0];

		MNRTCC->CTRLBSET = TCC_CMD_RETRIGGER;

	#elif defined(CPU_XMC48)

		u16 len = MANRCC->CV[1];

		//ManTmr->TCCLR = CC4_TCC;
		//ManTmr->TCSET = CC4_TRB;

	#endif

	_state = !_state;

	if (len <= US2MR(60))
	{
		i16 dl;

		if (len <= US2MR(36))
		{
			_length += 1; dl = len - US2MR(24); 
		}
		else
		{
			_length += 2; dl = len - US2MR(48);
		};

		if (dl < 0) dl = -dl; rcvManSum12 += dl; rcvManCount12++;

		if(_length >= 3)
		{
			_sync = false;
		};
	}
	else
	{
		if(len > US2MR(108))
		{
			_sync = false;
		}
		else
		{
			manRcvTime.Reset();

			_sync = true;
			_data = 0;
			_parity_temp = _parity;
			_number = 0;
			_command = !_state; 

			i16 dl;

			if (len <= US2MR(84))
			{
				_length = 1; dl = len - US2MR(72); 
			}
			else
			{
				_length = 2; dl = len - US2MR(96); 
			};

			if (dl < 0) dl = -dl; rcvManSum12 += dl; rcvManCount12++;
		};
	};

	if(_sync && _length == 2)
	{
		manRcvTime.Reset();

		if(_number < 16)
		{
			_data <<= 1;
			_data |= _state;
			_parity_temp ^= _state;
			_number++;
			_length = 0;
		}
	 	else
		{
			Pin_ManRcvSync_Set();

			_sync = false;

			if(_state != _parity_temp)
			{
				_state = !_state;
				_data = (~_data);
				_command = !_command;
			};

			if (rcvManLen == 0)
			{
				if(_command)
				{
					*rcvManPtr++ = _data;
					rcvManLen = 1;
				};
			}
			else 
			{
				if(rcvManLen < rcvManCount)
				{
					*rcvManPtr++ = _data;
				};

				rcvManLen += 1;	
			};

			Pin_ManRcvSync_Clr();
		};
	};

	#ifdef CPU_SAME53	
		MNRTCC->INTFLAG = ~0;
	#elif defined(CPU_XMC48)
	#endif

	Pin_ManRcvIRQ_Clr();
}

#pragma pop

#endif

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void ManRcvUpdate()
{
	if (rcvBusy)
	{
		u32 irq = Push_IRQ();

		if (rcvManLen > 0 && manRcvTime.Timeout(US2CLK(200)))
		{
			ManRcvEnd(true);
		}
		else
		{
			manRB->len = rcvManLen;
		};

		Pop_IRQ(irq);
	};
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifndef WIN32

void InitManRecieve()
{
	using namespace HW;

	SEGGER_RTT_WriteString(0, RTT_CTRL_TEXT_BRIGHT_GREEN "Manchester Reciever Init ... ");

	VectorTableExt[MANR_IRQ] = ManRcvIRQ2;
	CM4::NVIC->CLR_PR(MANR_IRQ);
	CM4::NVIC->SET_ER(MANR_IRQ);	

#ifdef CPU_SAME53	

	MANRT_ClockEnable();
	MANIT_ClockEnable();

	HW::GCLK->PCHCTRL[EVENT_MANR_1+GCLK_EVSYS0] = GCLK_GEN(GEN_MCK)|GCLK_CHEN;
	HW::GCLK->PCHCTRL[EVENT_MANR_2+GCLK_EVSYS0] = GCLK_GEN(GEN_MCK)|GCLK_CHEN;

	EIC->CTRLA = 0;
	while(EIC->SYNCBUSY);

	EIC->EVCTRL |= EIC_EXTINT0<<MANR_EXTINT;
	EIC->SetConfig(MANR_EXTINT, 1, EIC_SENSE_BOTH);
	EIC->INTENCLR = EIC_EXTINT0<<MANR_EXTINT;
	EIC->CTRLA = EIC_ENABLE;

	EVSYS->CH[EVENT_MANR_1].CHANNEL = (EVGEN_EIC_EXTINT_0+MANR_EXTINT)|EVSYS_PATH_ASYNCHRONOUS;
	EVSYS->USER[MANIT_EVSYS_USER] = EVENT_MANR_1+1;

	EVSYS->CH[EVENT_MANR_2].CHANNEL = MANRT_EVENT_GEN|EVSYS_PATH_ASYNCHRONOUS|EVSYS_EDGSEL_RISING_EDGE;;
	EVSYS->USER[MANRT_EVSYS_USER] = EVENT_MANR_2+1;

	//PIO_MANCHRX->DIRCLR = MANCHRX;
	//PIO_MANCHRX->CTRL |= MANCHRX;

	PIO_RXD->DIRCLR = RXD;
	PIO_RXD->CTRL |= RXD;

	//PIO_MANCHRX->SetWRCONFIG(	MANCHRX,	PORT_PMUX_A|PORT_WRPINCFG|PORT_PMUXEN|PORT_WRPMUX|PORT_INEN);
	PIO_RXD->SetWRCONFIG(		RXD,		PORT_PMUX_A|PORT_WRPINCFG|PORT_PMUXEN|PORT_WRPMUX|PORT_INEN);

	MNRTCC->CTRLA = TCC_SWRST;
	while(MNRTCC->SYNCBUSY);

	MNRTCC->CTRLA = TCC_CPTEN0|MANR_PRESC_DIV;
	MNRTCC->EVCTRL = TCC_MCEI0;

	MNRTCC->PER = ~0;
	MNRTCC->CC[1] = US2MR(250);

	MNRTCC->INTENCLR = ~0;
	//ManRT->INTENSET = TCC_MC0;

	MNRTCC->CTRLA |= TCC_ENABLE;

	MNITC->CTRLA = TCC_SWRST;

	while(MNITC->SYNCBUSY);

	MNITC->CTRLA = TC_MODE_COUNT8|MANI_PRESC_DIV;
	MNITC->WAVE = TC_WAVEGEN_NPWM;

	MNITC->EVCTRL = TC_TCEI|TC_EVACT_RETRIGGER|TC_OVFEO;

	MNITC->PER8 = US2MI(12)-1;
	MNITC->CC8[0] = ~0;
	MNITC->CC8[1] = ~0;

	MNITC->INTENCLR = ~0;

	MNITC->INTFLAG = ~0;

	MNITC->CTRLA |= TC_ENABLE;

	MNITC->CTRLBSET = TC_ONESHOT;

#elif defined(CPU_XMC48)

	HW::CCU_Enable(MANCCU_PID);

	PIO_MANRXD->ModePin(PIN_MANRXD, I2DPU);

	MANCCU->GCTRL = 0;

	MANCCU->GIDLC = MANR_GIDLC|MANI_GIDLC|CCU4_SPRB; //ManCCU_GIDLC;//CCU4_CS1I|CCU4_CS2I|CCU4_SPRB;

	MANICC->PRS = US2MR(12) - 1;
	MANICC->CRS = US2MR(12) - 1;
	MANICC->PSC = MANRT_PSC; //1.28us

	MANRCC->PRS = US2MR(250);
	MANRCC->PSC = MANRT_PSC; //1.28us

	MANCCU->GCSS = MANR_GCSS|MANI_GCSS; //ManCCU_GCSS;//CCU4_S1SE|CCU4_S2SE;  

	MANICC->INS = MANICC_INS;//CC4_EV0IS(2)|CC4_EV0EM_BOTH_EDGES|CC4_LPF0M_7CLK;
	MANICC->CMC = CC4_STRTS_EVENT0;
	MANICC->TC = CC4_STRM|CC4_TSSM;

	MANICC->INTE = 0;//CC4_PME;
	MANICC->SRS = MANICC_SRS;//CC4_POSR(2);

	MANRCC->INS = MANRCC_INS; //CC4_EV0IS(15) | CC4_EV0EM_RISING_EDGE;
	MANRCC->CMC = CC4_CAP0S_EVENT0|CC4_STRTS_EVENT0;
	MANRCC->SRS = MANRCC_SRS; //CC4_E0SR(2);
	MANRCC->TC = CC4_TSSM|CC4_CAPC_ALWAYS;
	MANRCC->TCSET = CC4_TRB;

	MANRCC->INTE = 0;//CC4_PME;

#endif

	SEGGER_RTT_WriteString(0, "OK\n");
}

#endif

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

bool RcvManData(MRB *mrb)
{
#ifndef WIN32

	if (rcvBusy /*|| trmBusy*/ || mrb == 0 || mrb->data == 0 || mrb->maxLen == 0)
	{
		return false;
	};

	//ManDisable();

	mrb->ready = mrb->OK = false;
	mrb->len = 0;

	manRB = mrb;
	
	rcvManLen = 0;

	rcvManPtr = manRB->data;
	rcvManCount = manRB->maxLen;

	rcvManSum12 = 0; rcvManCount12 = 0;

	#ifdef CPU_SAME53	

		MNRTCC->INTFLAG = ~0;
		MNRTCC->INTENSET = TCC_MC0;

	#elif defined(CPU_XMC48)

		MANRCC->SWR = CC4_RE0A;
		MANRCC->INTE = CC4_E0AE;

	#endif

	return rcvBusy = true;

#else

	mrb->ready = mrb->OK = false;
	mrb->len = 0;

	return true;

#endif
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#endif // MANCH_IMP_H__10_05_2022__16_55
