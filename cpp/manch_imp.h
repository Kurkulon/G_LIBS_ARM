#ifndef MANCH_IMP_H__10_05_2022__16_55
#define MANCH_IMP_H__10_05_2022__16_55

#include "types.h"
#include "core.h"
#include "SEGGER_RTT.h"


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifndef WIN32

inline void ManDisable()	{ PIO_MANCH->CLR(L1|L2);	PIO_MANCH->SET(H1|H2);							} 
inline void ManZero()		{ PIO_MANCH->CLR(L2);		PIO_MANCH->SET(L1|H1);		PIO_MANCH->CLR(H2);	} 
inline void ManOne()		{ PIO_MANCH->CLR(L1);		PIO_MANCH->SET(L2|H2);		PIO_MANCH->CLR(H1);	} 
inline void ManDischarge()	{ PIO_MANCH->CLR(L1|L2);	PIO_MANCH->CLR(H1|H2);							} 

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static const u16 manbaud[5] = { BAUD2CLK(20833), BAUD2CLK(41666), BAUD2CLK(62500), BAUD2CLK(83333), BAUD2CLK(104166) };//0:20833Hz, 1:41666Hz,2:62500Hz,3:83333Hz

static u16 trmHalfPeriod = (manbaud[0]+1)/2;
static u16 trmHalfPeriod2 = manbaud[0];
static u16 trmHalfPeriod3 = (manbaud[0]*3+1)/2;
static u16 trmHalfPeriod4 = manbaud[0] * 2;
static u16 trmHalfPeriod6 = manbaud[0] * 3;
static u16 trmHalfPeriod7 = (manbaud[0] * 7 + 1) / 2;

#endif 

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static byte stateManTrans = 0;
static MTB *manTB = 0;
static bool trmBusy = false;
static bool trmTurbo = false;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifndef WIN32

static u16 GetTrmBaudRate(byte i)
{
	if (i >= ArraySize(manbaud)) { i = ArraySize(manbaud) - 1; };

	return (manbaud[i]+1)/2;
}
#endif
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//static u16 rcvCount = 0;
static bool rcvBusy = false;
byte stateManRcvr = 0;

const u16 rcvPeriod = BAUD2CLK(20833);

static u16* rcvManPtr = 0;
static u16 rcvManCount = 0;

static u16 rcvManLen = 0;

static MRB *manRB = 0;

//static u16 rcvManLen72 = 0;
//static u16 rcvManLen96 = 0;
//static u16 rcvManLen24 = 0;
//static u16 rcvManLen48 = 0;
//
//static u32 rcvManSum72 = 0;
//static u32 rcvManSum96 = 0;
//static u32 rcvManSum24 = 0;
//static u32 rcvManSum48 = 0;
//
//static u16 rcvManCount72 = 0;
//static u16 rcvManCount96 = 0;
//static u16 rcvManCount24 = 0;
//static u16 rcvManCount48 = 0;

static u16 rcvManLen12 = 0;
static u32 rcvManSum12 = 0;
static u16 rcvManCount12 = 0;
u16 rcvManQuality = 0;

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
				ManTT->CTRLA = 0;
				ManTT->INTENCLR = ~0;
			#elif defined(CPU_XMC48)
				ManTT->TCCLR = CC4_TRBC;
			#endif

			manTB->ready = true;
			trmBusy = false;

			break;


	}; // 	switch (stateManTrans)


	#ifdef CPU_SAME53	
		ManTT->INTFLAG = ~0;//TCC_OVF;
	#endif

	Pin_ManTrmIRQ_Clr();
}

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


		ManTT->CTRLA = TC_MODE_COUNT8;
		ManTT->WAVE = TC_WAVEGEN_NPWM;
		ManTT->PER8 = GetTrmBaudRate(mtb->baud) - 1; //trmHalfPeriod-1;

		ManTT->INTENCLR = ~TC_OVF;
		ManTT->INTENSET = TC_OVF;

		ManTT->INTFLAG = ~0;

		ManTT->CTRLA = TC_MODE_COUNT8|TC_ENABLE;
		ManTT->CTRLBSET = TC_CMD_RETRIGGER;

		//ManTT->CTRLA = 0;

		//ManTT->PER = trmHalfPeriod-1;

		//ManTT->INTENCLR = ~TCC_OVF;
		//ManTT->INTENSET = TCC_OVF;

		//ManTT->INTFLAG = ~0;

		//ManTT->CTRLA = TCC_ENABLE;

	#elif defined(CPU_XMC48)

		ManTT->PRS = GetTrmBaudRate(mtb->baud)-1; //trmHalfPeriod - 1;
		ManTT->PSC = 3; //0.08us

		ManCCU->GCSS = CCU4_S0SE;  

		ManTT->SWR = ~0;
		ManTT->INTE = CC4_PME;

		ManTT->TCSET = CC4_TRBS;

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

	HW::GCLK->PCHCTRL[GCLK_TC0_TC1] = GCLK_GEN(GEN_1M)|GCLK_CHEN;

	HW::MCLK->APBAMASK |= APBA_TC0;

	PIO_MANCH->DIRSET = L1|H1|L2|H2;

	ManTT->CTRLA = TCC_SWRST;

	while(ManTT->SYNCBUSY);

	//SetTrmBoudRate(0);

	ManTT->CTRLA = TC_MODE_COUNT8;
	ManTT->WAVE = TC_WAVEGEN_NPWM;
	ManTT->PER8 = GetTrmBaudRate(0) - 1;

	ManTT->INTENCLR = ~TC_OVF;
	ManTT->INTENSET = TC_OVF;

	ManTT->INTFLAG = ~0;

#elif defined(CPU_XMC48)

	HW::CCU_Enable(ManCCU_PID);

	ManCCU->GCTRL = 0;

	ManCCU->GIDLC = CCU4_S0I|CCU4_PRB;

	ManTT->PRS = GetTrmBaudRate(0)-1;
	ManTT->PSC = 3; //0.08us

	ManCCU->GCSS = CCU4_S0SE;  

	ManTT->SRS = 0;

	ManTT->SWR = ~0;
	ManTT->INTE = CC4_PME;

#endif

	ManDisable();
}

#endif

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#else // #ifdef MAN_TRANSMIT_V1

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifndef WIN32

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

			HW::SCU_GENERAL->CCUCON &= ~ManT_CCUCON;


			ManT1->TCCLR = CC8_TRBC;
			ManT1->INTE = 0;
			ManT2->TCCLR = CC8_TRBC;
			ManT3->TCCLR = CC8_TRBC;

			ManT_CCU8->GCSS = ManT_OUT_GCSS;
			ManT_CCU8->GCSC = ManT_OUT_GCSC;
			//ManT_CCU8->GIDLS = ManT_CCU8_GIDLS;

			manTB->ready = true;
			trmBusy = false;

			break;


	}; // 	switch (stateManTrans)


	Pin_ManTrmIRQ_Clr();
}

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


	ManTT->CTRLA = TC_MODE_COUNT8;
	ManTT->WAVE = TC_WAVEGEN_NPWM;
	ManTT->PER8 = GetTrmBaudRate(mtb->baud) - 1; //trmHalfPeriod-1;

	ManTT->INTENCLR = ~TC_OVF;
	ManTT->INTENSET = TC_OVF;

	ManTT->INTFLAG = ~0;

	ManTT->CTRLA = TC_MODE_COUNT8 | TC_ENABLE;
	ManTT->CTRLBSET = TC_CMD_RETRIGGER;

	//ManTT->CTRLA = 0;

	//ManTT->PER = trmHalfPeriod-1;

	//ManTT->INTENCLR = ~TCC_OVF;
	//ManTT->INTENSET = TCC_OVF;

	//ManTT->INTFLAG = ~0;

	//ManTT->CTRLA = TCC_ENABLE;

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

	SEGGER_RTT_WriteString(0, RTT_CTRL_TEXT_BRIGHT_WHITE "Manchester transmit2 Init ... ");

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

	SEGGER_RTT_WriteString(0, "OK\n");
}

#endif

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#endif // #else // #ifdef MAN_TRANSMIT_V1

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void ManRcvEnd(bool ok)
{
#ifdef CPU_SAME53	
	ManRT->INTENCLR = ~0;
#elif defined(CPU_XMC48)
	ManTmr->INTE = 0;
#endif

	manRB->OK = ok;
	manRB->ready = true;
	manRB->len = rcvManLen;

	rcvManLen12 = (rcvManCount12 != 0) ? (rcvManSum12 / rcvManCount12) : 0;

	rcvManQuality = (rcvManLen12 > US2MT(12)) ? 0 : (((US2MT(12) - rcvManLen12) * 100 + US2MT(6))/US2MT(12));

	rcvBusy = false;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static RTM manRcvTime;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifndef WIN32

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

		u32 len = ManRT->CC[0];

		ManRT->CTRLBSET = TCC_CMD_RETRIGGER;

	#elif defined(CPU_XMC48)

		u16 len = ManTmr->CV[1];

		//ManTmr->TCCLR = CC4_TCC;
		//ManTmr->TCSET = CC4_TRB;

	#endif

	_state = !_state;

	if (len <= US2MT(60))
	{
		i16 dl;

		if (len <= US2MT(36))
		{
			_length += 1; dl = len - US2MT(24); 
		}
		else
		{
			_length += 2; dl = len - US2MT(48);
		};

		if (dl < 0) dl = -dl; rcvManSum12 += dl; rcvManCount12++;

		if(_length >= 3)
		{
			_sync = false;
		};
	}
	else
	{
		if(len > US2MT(108))
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

			if (len <= US2MT(84))
			{
				_length = 1; dl = len - US2MT(72); 
			}
			else
			{
				_length = 2; dl = len - US2MT(96); 
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
		ManRT->INTFLAG = ~0;
	#elif defined(CPU_XMC48)
	#endif

	Pin_ManRcvIRQ_Clr();
}
#endif

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void ManRcvUpdate()
{
	if (rcvBusy)
	{
		u32 irq = Push_IRQ();

		if (rcvManLen > 0 && manRcvTime.Timeout(US2RT(200)))
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

	HW::GCLK->PCHCTRL[GCLK_TCC2_TCC3]	= GCLK_GEN(GEN_1M)|GCLK_CHEN;
	HW::GCLK->PCHCTRL[GCLK_TC0_TC1]		= GCLK_GEN(GEN_1M)|GCLK_CHEN;

	HW::MCLK->APBCMASK |= APBC_TCC2;
	HW::MCLK->APBAMASK |= APBA_TC1;

	HW::GCLK->PCHCTRL[EVENT_MANR_1+GCLK_EVSYS0] = GCLK_GEN(GEN_MCK)|GCLK_CHEN;
	HW::GCLK->PCHCTRL[EVENT_MANR_2+GCLK_EVSYS0] = GCLK_GEN(GEN_MCK)|GCLK_CHEN;

	EIC->CTRLA = 0;
	while(EIC->SYNCBUSY);

	EIC->EVCTRL |= EIC_EXTINT0<<MANR_EXTINT;
	EIC->SetConfig(MANR_EXTINT, 1, EIC_SENSE_BOTH);
	EIC->INTENCLR = EIC_EXTINT0<<MANR_EXTINT;
	EIC->CTRLA = EIC_ENABLE;

	EVSYS->CH[EVENT_MANR_1].CHANNEL = (EVGEN_EIC_EXTINT_0+MANR_EXTINT)|EVSYS_PATH_ASYNCHRONOUS;
	EVSYS->USER[EVSYS_USER_TC1_EVU] = EVENT_MANR_1+1;

	EVSYS->CH[EVENT_MANR_2].CHANNEL = EVGEN_TC1_OVF|EVSYS_PATH_ASYNCHRONOUS|EVSYS_EDGSEL_RISING_EDGE;;
	EVSYS->USER[EVSYS_USER_TCC2_MC_0] = EVENT_MANR_2+1;

	PIO_MANCHRX->DIRCLR = MANCHRX;
	PIO_MANCHRX->CTRL |= MANCHRX;

	PIO_RXD->DIRCLR = RXD;
	PIO_RXD->CTRL |= RXD;

	PIO_MANCHRX->SetWRCONFIG(	MANCHRX,	PORT_PMUX(0)|PORT_WRPINCFG|PORT_PMUXEN|PORT_WRPMUX|PORT_INEN);
	PIO_RXD->SetWRCONFIG(		RXD,		PORT_PMUX(0)|PORT_WRPINCFG|PORT_PMUXEN|PORT_WRPMUX|PORT_INEN);

	//PIO_MANCHRX->PINCFG[PIN_MANCHRX] = PINGFG_INEN|PINGFG_PMUXEN;
	//PIO_RXD->PINCFG[PIN_RXD] = PINGFG_INEN|PINGFG_PMUXEN;

	ManRT->CTRLA = TCC_SWRST;
	while(ManRT->SYNCBUSY);

	ManRT->CTRLA = TCC_CPTEN0;
	ManRT->EVCTRL = TCC_MCEI0;

	ManRT->PER = ~0;
	ManRT->CC[1] = 250;

	ManRT->INTENCLR = ~0;
	//ManRT->INTENSET = TCC_MC0;

	ManRT->CTRLA = TCC_CPTEN0|TCC_ENABLE;


	ManIT->CTRLA = TCC_SWRST;

	while(ManIT->SYNCBUSY);

	ManIT->CTRLA = TC_MODE_COUNT8;
	ManIT->WAVE = TC_WAVEGEN_NPWM;

	ManIT->EVCTRL = TC_TCEI|TC_EVACT_RETRIGGER|TC_OVFEO;

	ManIT->PER8 = 11;
	ManIT->CC8[0] = ~0;
	ManIT->CC8[1] = ~0;

	ManIT->INTENCLR = ~0;

	ManIT->INTFLAG = ~0;

	ManIT->CTRLA = TC_MODE_COUNT8|TC_ENABLE;

	ManIT->CTRLBSET = TC_ONESHOT;

#elif defined(CPU_XMC48)

	HW::CCU_Enable(ManCCU_PID);

	P1->ModePin10(I2DPU);
	P1->ModePin11(I2DPU);

	ManCCU->GCTRL = 0;

	ManCCU->GIDLC = ManCCU_GIDLC;//CCU4_CS1I|CCU4_CS2I|CCU4_SPRB;

	ManRT->PRS = US2MT(12) - 1;
	ManRT->CRS = US2MT(12) - 1;
	ManRT->PSC = ManRT_PSC; //1.28us

	ManTmr->PRS = US2MT(250);
	ManTmr->PSC = ManRT_PSC; //1.28us

	ManCCU->GCSS = ManCCU_GCSS;//CCU4_S1SE|CCU4_S2SE;  

	ManRT->INS = ManRT_INS;//CC4_EV0IS(2)|CC4_EV0EM_BOTH_EDGES|CC4_LPF0M_7CLK;
	ManRT->CMC = CC4_STRTS_EVENT0;
	ManRT->TC = CC4_STRM|CC4_TSSM;

	ManRT->INTE = 0;//CC4_PME;
	ManRT->SRS = 0;//ManRT_SRS;//CC4_POSR(2);

	ManTmr->INS = ManTmr_INS; //CC4_EV0IS(15) | CC4_EV0EM_RISING_EDGE;
	ManTmr->CMC = CC4_CAP0S_EVENT0|CC4_STRTS_EVENT0;
	ManTmr->SRS = CC4_E0SR(2);
	ManTmr->TC = CC4_TSSM|CC4_CAPC_ALWAYS;
	ManTmr->TCSET = CC4_TRB;

	ManTmr->INTE = 0;//CC4_PME;

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

		ManRT->INTFLAG = ~0;
		ManRT->INTENSET = TCC_MC0;

	#elif defined(CPU_XMC48)

		ManTmr->SWR = CC4_RE0A;
		ManTmr->INTE = CC4_E0AE;

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
