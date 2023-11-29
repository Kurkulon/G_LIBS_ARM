#ifndef CLOCK_IMP_H__20_11_2023__16_46
#define CLOCK_IMP_H__20_11_2023__16_46

#pragma once

#include "core.h"
#include "clock.h"
#include "SEGGER_RTT.h"
#include "i2c.h"

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#define ADR_DS3232	0x68
#define ADR_RV3129	0x56

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

class Clock
{
	public:

		virtual	void	SetClock(const RTC &t, DSCI2C &dsc, byte buf[8])	= 0;
		virtual bool	ReqTemp(DSCI2C &dsc, byte buf[8])					= 0;
		virtual i16		GetTemp(DSCI2C &dsc)								= 0;
		virtual bool	StartConvTemp(DSCI2C &dsc, byte buf[8])				= 0;
		virtual bool	Init()						= 0;
};

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

class Clock_DS3232 : public Clock
{
	protected:

		enum { ADR = ADR_DS3232 };

	public:

		virtual	void	SetClock(const RTC &t, DSCI2C &dsc, byte buf[8]);
		virtual bool	ReqTemp(DSCI2C &dsc, byte buf[8]);
		virtual i16		GetTemp(DSCI2C &dsc);
		virtual bool	StartConvTemp(DSCI2C &dsc, byte buf[8]);
		virtual bool	Init();
};

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

class Clock_RV3129 : public Clock
{
	protected:

		enum { ADR = ADR_RV3129 };

	public:

		virtual	void	SetClock(const RTC &t, DSCI2C &dsc, byte buf[8]);
		virtual bool	ReqTemp(DSCI2C &dsc, byte buf[8]);
		virtual i16		GetTemp(DSCI2C &dsc);
		virtual bool	StartConvTemp(DSCI2C &dsc, byte buf[8]);
		virtual bool	Init();
};

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void Clock_DS3232::SetClock(const RTC &t, DSCI2C &dsc, byte buf[8])
{
	//static DSCI2C dsc;

	//static byte reg = 0;
	//static u16 rbuf = 0;
	//static byte buf[8];

	buf[0] = 0;
	buf[1] = ((t.sec/10) << 4)|(t.sec%10);
	buf[2] = ((t.min/10) << 4)|(t.min%10);
	buf[3] = ((t.hour/10) << 4)|(t.hour%10);
	buf[4] = 1;
	buf[5] = ((t.day/10) << 4)|(t.day%10);
	buf[6] = ((t.mon/10) << 4)|(t.mon%10);

	byte y = t.year % 100;

	buf[7] = ((y/10) << 4)|(y%10);

	dsc.adr = ADR;
	dsc.wdata = buf;
	dsc.wlen = 8;
	dsc.rdata = 0;
	dsc.rlen = 0;
	dsc.wdata2 = 0;
	dsc.wlen2 = 0;

	if (SetTime(t)) I2C_AddRequest(&dsc);
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void Clock_RV3129::SetClock(const RTC &t, DSCI2C &dsc, byte buf[8])
{
	//static DSCI2C dsc;

	//static byte reg = 0;
	//static u16 rbuf = 0;
	//static byte buf[8];

	buf[0] = 8;
	buf[1] = ((t.sec/10) << 4)|(t.sec%10);
	buf[2] = ((t.min/10) << 4)|(t.min%10);
	buf[3] = ((t.hour/10) << 4)|(t.hour%10);
	buf[4] = 1;
	buf[5] = ((t.day/10) << 4)|(t.day%10);
	buf[6] = ((t.mon/10) << 4)|(t.mon%10);

	byte y = t.year % 100;

	buf[7] = ((y/10) << 4)|(y%10);

	dsc.adr = ADR;
	dsc.wdata = buf;
	dsc.wlen = 8;
	dsc.rdata = 0;
	dsc.rlen = 0;
	dsc.wdata2 = 0;
	dsc.wlen2 = 0;

	if (SetTime(t)) I2C_AddRequest(&dsc);
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

bool Clock_DS3232::ReqTemp(DSCI2C &dsc, byte buf[8])
{
	//static byte buf[3];

	buf[0] = 0x11;

	dsc.adr = ADR_DS3232;
	dsc.wdata = buf;
	dsc.wlen = 1;
	dsc.rdata = buf+1;
	dsc.rlen = 2;
	dsc.wdata2 = 0;
	dsc.wlen2 = 0;

	return I2C_AddRequest(&dsc);
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

bool Clock_RV3129::ReqTemp(DSCI2C &dsc, byte buf[8])
{
	//static byte buf[2];

	buf[0] = 0x20;

	dsc.adr = ADR;
	dsc.wdata = buf;
	dsc.wlen = 1;
	dsc.rdata = buf+1;
	dsc.rlen = 1;
	dsc.wdata2 = 0;
	dsc.wlen2 = 0;

	return I2C_AddRequest(&dsc);
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

i16 Clock_DS3232::GetTemp(DSCI2C &dsc)
{
	byte *p = (byte*)dsc.rdata;

	i16 t = ((i16)((p[0]<<8) + p[1]) + 128) / 256;

	if (t < (-60))
	{
		t += 256;
	};

	return t;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

i16 Clock_RV3129::GetTemp(DSCI2C &dsc)
{
	byte *p = (byte*)dsc.rdata;

	return (i16)(p[0]) - 60;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

bool Clock_DS3232::StartConvTemp(DSCI2C &dsc, byte buf[8])
{
	//static byte buf[3];

	buf[0] = 0x0E;
	buf[1] = 0x20;
	buf[2] = 0xC8;

	dsc.adr = ADR_DS3232;
	dsc.wdata = buf;
	dsc.wlen = 3;
	dsc.rdata = 0;
	dsc.rlen = 0;
	dsc.wdata2 = 0;
	dsc.wlen2 = 0;

	return I2C_AddRequest(&dsc);
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

bool Clock_RV3129::StartConvTemp(DSCI2C &dsc, byte buf[8])
{
	//static byte buf[2];

	buf[0] = 0x02; // Reset Control_INT Flag 
	buf[1] = 0x00;

	dsc.adr = ADR_RV3129;
	dsc.wdata = buf;
	dsc.wlen = 2;
	dsc.rdata = 0;
	dsc.rlen = 0;
	dsc.wdata2 = 0;
	dsc.wlen2 = 0;

	return I2C_AddRequest(&dsc);
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

bool Clock_DS3232::Init()
{
	DSCI2C dsc;

	//byte reg = 0;
	byte buf[10];
	
	::RTC t;

	buf[0] = 0x0F;
	buf[1] = 0x88;
	dsc.adr = ADR;
	dsc.wdata = buf;
	dsc.wlen = 2;
	dsc.rdata = 0;
	dsc.rlen = 0;
	dsc.wdata2 = 0;
	dsc.wlen2 = 0;

	SEGGER_RTT_WriteString(0, RTT_CTRL_TEXT_BRIGHT_YELLOW "Init DS3232 ... ");

	I2C_AddRequest(&dsc);

	while (!dsc.ready) I2C_Update();

	if (!dsc.ack) { SEGGER_RTT_WriteString(0, RTT_CTRL_TEXT_BRIGHT_RED "!!! ERROR !!!\n"); return false; };

	SEGGER_RTT_WriteString(0, RTT_CTRL_TEXT_BRIGHT_GREEN "OK\n");

	buf[0] = 0;

	dsc.adr = ADR;
	dsc.wdata = buf;
	dsc.wlen = 1;
	dsc.rdata = buf;
	dsc.rlen = 7;
	dsc.wdata2 = 0;
	dsc.wlen2 = 0;

	SEGGER_RTT_WriteString(0, RTT_CTRL_TEXT_BRIGHT_YELLOW "Sync with DS3232 ... ");

	I2C_AddRequest(&dsc);

	while (!dsc.ready) I2C_Update();	// { HW::WDT->Update(); };

	if (!dsc.ack) { SEGGER_RTT_WriteString(0, RTT_CTRL_TEXT_BRIGHT_RED "!!! ERROR !!!\n"); return false; };

	t.sec	= (buf[0]&0xF) + ((buf[0]>>4)*10);
	t.min	= (buf[1]&0xF) + ((buf[1]>>4)*10);
	t.hour	= (buf[2]&0xF) + ((buf[2]>>4)*10);
	t.day	= (buf[4]&0xF) + ((buf[4]>>4)*10);
	t.mon	= (buf[5]&0xF) + ((buf[5]>>4)*10);
	t.year	= (buf[6]&0xF) + ((buf[6]>>4)*10) + 2000;

	SetTime(t);

	SEGGER_RTT_WriteString(0, RTT_CTRL_TEXT_BRIGHT_GREEN "OK\n");

	return true;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

bool Clock_RV3129::Init()
{
	DSCI2C dsc;

	//byte reg = 8;
	byte buf[10];
	
	::RTC t;

	buf[0] = 0x18;
	buf[1] = 31;	// Timer Low
	buf[2] = 0;		// Timer High

	dsc.adr = ADR;
	dsc.wdata = buf;
	dsc.wlen = 3;
	dsc.rdata = 0;
	dsc.rlen = 0;
	dsc.wdata2 = 0;
	dsc.wlen2 = 0;

	SEGGER_RTT_WriteString(0, RTT_CTRL_TEXT_BRIGHT_YELLOW "Init RV3129 ... ");

	I2C_AddRequest(&dsc);

	while (!dsc.ready) I2C_Update();

	if (!dsc.ack) { SEGGER_RTT_WriteString(0, RTT_CTRL_TEXT_BRIGHT_RED "!!! ERROR !!!\n"); return false; };

	buf[0] = 0x00;
	buf[1] = 0x9F;	// Control_1
	buf[2] = 0x02;	// Control_INT: TIE

	dsc.adr = ADR;
	dsc.wdata = buf;
	dsc.wlen = 3;
	dsc.rdata = 0;
	dsc.rlen = 0;
	dsc.wdata2 = 0;
	dsc.wlen2 = 0;

	I2C_AddRequest(&dsc);

	while (!dsc.ready) I2C_Update();

	if (!dsc.ack) { SEGGER_RTT_WriteString(0, RTT_CTRL_TEXT_BRIGHT_RED "!!! ERROR !!!\n"); return false; };

	SEGGER_RTT_WriteString(0, RTT_CTRL_TEXT_BRIGHT_GREEN "OK\n");

	buf[0] = 8;

	dsc.adr = ADR;
	dsc.wdata = buf;
	dsc.wlen = 1;
	dsc.rdata = buf;
	dsc.rlen = 7;
	dsc.wdata2 = 0;
	dsc.wlen2 = 0;

	SEGGER_RTT_WriteString(0, RTT_CTRL_TEXT_BRIGHT_YELLOW "Sync with RV3129 ... ");

	I2C_AddRequest(&dsc);

	while (!dsc.ready) I2C_Update();	// { HW::WDT->Update(); };

	if (!dsc.ack) { SEGGER_RTT_WriteString(0, RTT_CTRL_TEXT_BRIGHT_RED "!!! ERROR !!!\n"); return false; };

	t.sec	= (buf[0]&0xF) + ((buf[0]>>4)*10);
	t.min	= (buf[1]&0xF) + ((buf[1]>>4)*10);
	t.hour	= (buf[2]&0xF) + ((buf[2]>>4)*10);
	t.day	= (buf[3]&0xF) + ((buf[3]>>4)*10);
	t.mon	= (buf[5]&0xF) + ((buf[5]>>4)*10);
	t.year	= (buf[6]&0xF) + ((buf[6]>>4)*10) + 2000;

	SetTime(t);

	SEGGER_RTT_WriteString(0, RTT_CTRL_TEXT_BRIGHT_GREEN "OK\n");

	return true;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static Clock_DS3232 ds3232;
static Clock_RV3129 rv3129;

static Clock *clock = &ds3232;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void SetClock(const RTC &t)
{
	static DSCI2C dsc;
	static byte buf[10];

	clock->SetClock(t, dsc, buf);
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

bool ClockReqTemp(DSCI2C &dsc, byte buf[8])
{
	return clock->ReqTemp(dsc, buf);
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

i16	ClockGetTemp(DSCI2C &dsc)
{
	return clock->GetTemp(dsc);
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

bool ClockStartConvTemp(DSCI2C &dsc, byte buf[8])
{
	return clock->StartConvTemp(dsc, buf);
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static __irq void Clock_IRQ()
{
#ifdef CPU_SAME53

	HW::EIC->INTFLAG = 1UL<<CLOCK_EXTINT;
	
	timeBDC.msec = (timeBDC.msec < 500) ? 0 : 999;

#elif defined(CPU_XMC48)

	if (HW::SCU_HIBERNATE->HDSTAT & SCU_HIBERNATE_HDSTAT_ULPWDG_Msk)
	{
		if ((HW::SCU_GENERAL->MIRRSTS & SCU_GENERAL_MIRRSTS_HDCLR_Msk) == 0)	HW::SCU_HIBERNATE->HDCLR = SCU_HIBERNATE_HDCLR_ULPWDG_Msk;
	}
	else
	{
		timeBDC.msec = (timeBDC.msec < 500) ? 0 : 999;
	};

	HW::SCU_GCU->SRCLR = SCU_INTERRUPT_SRCLR_PI_Msk;

#endif
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifndef WIN32

static void InitClock()
{
	if (rv3129.Init())
	{
		clock = &rv3129;
	}
	else if (ds3232.Init())
	{
		clock = &ds3232;
	};

	SEGGER_RTT_WriteString(0, RTT_CTRL_TEXT_BRIGHT_WHITE "Clock Init ... ");

	VectorTableExt[CLOCK_IRQ] = Clock_IRQ;
	CM4::NVIC->CLR_PR(CLOCK_IRQ);
	CM4::NVIC->SET_ER(CLOCK_IRQ);	

#ifdef CPU_SAME53

	using namespace HW;

	EIC->CTRLA = 0;
	while(EIC->SYNCBUSY);

	EIC->EVCTRL |= EIC_EXTINT0<<CLOCK_EXTINT;
	EIC->SetConfig(CLOCK_EXTINT, 1, EIC_SENSE_FALL);
	EIC->INTENSET = EIC_EXTINT0<<CLOCK_EXTINT;
	EIC->CTRLA = EIC_ENABLE;

	PIO_RTCINT->SetWRCONFIG(RTCINT, PORT_PMUX(0)|PORT_WRPINCFG|PORT_PMUXEN|PORT_WRPMUX|PORT_INEN);

#elif defined(CPU_XMC48)

	HW::RTC->CTR = (0x7FFFUL << RTC_CTR_DIV_Pos) | RTC_CTR_ENB_Msk;

	while (HW::SCU_GCU->MIRRSTS & SCU_GENERAL_MIRRSTS_RTC_MSKSR_Msk);

	HW::RTC->MSKSR = RTC_MSKSR_MPSE_Msk;
	HW::SCU_GCU->SRMSK = SCU_INTERRUPT_SRMSK_PI_Msk;

#endif

	SEGGER_RTT_WriteString(0, "OK\n");
}

#endif


#endif // CLOCK_IMP_H__20_11_2023__16_46
