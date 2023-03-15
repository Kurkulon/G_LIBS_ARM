#ifndef SYSTEM_IMP_H__11_10_2022__18_02
#define SYSTEM_IMP_H__11_10_2022__18_02

#include "types.h"
#include "core.h"
#include "SEGGER_RTT.h"

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/*----------------------------------------------------------------------------
  Initialize the system
 *----------------------------------------------------------------------------*/
#ifndef WIN32

extern "C" void SystemInit()
{
	//u32 i;
	using namespace HW;

//	__breakpoint(0);

	#ifndef BOOTLOADER

		SEGGER_RTT_Init();
		SEGGER_RTT_WriteString(0, RTT_CTRL_TEXT_BRIGHT_YELLOW "SystemInit ... ");

	#endif

	#ifdef CPU_SAME53	

		using namespace CM4;

		HW::PIOA->DIR =	PIOA_INIT_DIR;
		HW::PIOA->CLR(	PIOA_INIT_CLR);
		HW::PIOA->SET(	PIOA_INIT_SET);

		HW::PIOB->DIR =	PIOB_INIT_DIR;
		HW::PIOB->CLR(	PIOB_INIT_CLR);
		HW::PIOB->SET(	PIOB_INIT_SET);

		HW::PIOC->DIR =	PIOC_INIT_DIR;
		HW::PIOC->CLR(	PIOC_INIT_CLR);
		HW::PIOC->SET(	PIOC_INIT_SET);

		Pin_MainLoop_Set();

		if ((HW::SUPC->BOD33 & BOD33_ENABLE) == 0)
		{
			HW::SUPC->BOD33 = 0;
			HW::SUPC->BOD33 = BOD33_HYST(15) | BOD33_ACTION_INT| BOD33_LEVEL(200); // Vbod = 1.5 + 6mV * BOD33_LEVEL
			HW::SUPC->BOD33 |= BOD33_ENABLE;

			Pin_MainLoop_Clr();

			while ((HW::SUPC->STATUS & (SUPC_BOD33RDY|SUPC_B33SRDY)) != (SUPC_BOD33RDY|SUPC_B33SRDY));
			while ((HW::SUPC->STATUS & SUPC_BOD33DET) != 0);

			Pin_MainLoop_Set();

			HW::SUPC->BOD33 = 0;
			HW::SUPC->BOD33 = BOD33_HYST(15) | BOD33_ACTION_RESET | BOD33_LEVEL(200); // Vbod = 1.5 + 6mV * BOD33_LEVEL
			HW::SUPC->BOD33 |= BOD33_ENABLE;
		};

		Pin_MainLoop_Clr();

		OSCCTRL->XOSC[1] = XOSC_ENABLE|XOSC_ONDEMAND; // RUNSTDBY|ENABLE

		Pin_MainLoop_Set();

		OSCCTRL->DPLL[0].CTRLA = 0; while ((OSCCTRL->DPLL[0].SYNCBUSY & DPLLSYNCBUSY_ENABLE) != 0);

		OSCCTRL->DPLL[0].CTRLB = DPLL_REFCLK_XOSC1|DPLL_DIV(24);	// 0x70010; // XOSC clock source division factor 50 = 2*(DIV+1), XOSC clock reference
		OSCCTRL->DPLL[0].RATIO = DPLL_LDR((MCK*2+500000)/1000000-1)|DPLL_LDRFRAC(0);	// 47; // Loop Divider Ratio = 200, Loop Divider Ratio Fractional Part = 0

		OSCCTRL->DPLL[0].CTRLA = DPLL_ONDEMAND|DPLL_ENABLE; 

		Pin_MainLoop_Clr();

		HW::GCLK->GENCTRL[GEN_MCK] = GCLK_DIV(0)|GCLK_SRC_DPLL0|GCLK_GENEN;

		Pin_MainLoop_Set();

		HW::MCLK->AHBMASK |= AHB_DMAC;
		HW::DMAC->CTRL = 0;
		HW::DMAC->CTRL = DMAC_SWRST;
		HW::DMAC->DBGCTRL = DMAC_DBGRUN;
		HW::DMAC->BASEADDR	= DmaTable;
		HW::DMAC->WRBADDR	= DmaWRB;
		HW::DMAC->CTRL = DMAC_DMAENABLE|DMAC_LVLEN0|DMAC_LVLEN1|DMAC_LVLEN2|DMAC_LVLEN3;

		Pin_MainLoop_Clr();

		if ((CMCC->SR & CMCC_CSTS) == 0)
		{
			CMCC->CTRL = CMCC_CEN;
		};

		Pin_MainLoop_Set();
		Pin_MainLoop_Clr();
		Pin_MainLoop_Set();
		Pin_MainLoop_Clr();

	#elif defined(CPU_XMC48)

		using namespace CM4;

		__disable_irq();

//		__DSB();
		__enable_irq();

		HW::FLASH0->FCON = FLASH_FCON_IDLE_Msk | PMU_FLASH_WS;

		/* enable PLL */
		SCU_PLL->PLLCON0 &= ~(SCU_PLL_PLLCON0_VCOPWD_Msk | SCU_PLL_PLLCON0_PLLPWD_Msk);

		SCU_OSC->OSCHPCTRL = OSC_MODE(2) | OSC_OSCVAL(OSCHP_FREQUENCY / FOSCREF - 1UL);

			/* select OSC_HP clock as PLL input */
			SCU_PLL->PLLCON2 = 0;

			/* restart OSC Watchdog */
			SCU_PLL->PLLCON0 &= ~SCU_PLL_PLLCON0_OSCRES_Msk;

			while ((SCU_PLL->PLLSTAT & SCU_PLL_PLLSTAT_OSC_USABLE) != SCU_PLL_PLLSTAT_OSC_USABLE);
		//};

		/* Go to bypass the Main PLL */
		SCU_PLL->PLLCON0 |= SCU_PLL_PLLCON0_VCOBYP_Msk;

		/* disconnect Oscillator from PLL */
		SCU_PLL->PLLCON0 |= SCU_PLL_PLLCON0_FINDIS_Msk;

		/* Setup divider settings for main PLL */
		SCU_PLL->PLLCON1 =  PLL_CON1(PLL_NDIV, PLL_K2DIV_24MHZ, PLL_PDIV);

		/* Set OSCDISCDIS */
		SCU_PLL->PLLCON0 |= SCU_PLL_PLLCON0_OSCDISCDIS_Msk;

		/* connect Oscillator to PLL */
		SCU_PLL->PLLCON0 &= ~SCU_PLL_PLLCON0_FINDIS_Msk;

		/* restart PLL Lock detection */
		SCU_PLL->PLLCON0 |= SCU_PLL_PLLCON0_RESLD_Msk;	while ((SCU_PLL->PLLSTAT & SCU_PLL_PLLSTAT_VCOLOCK_Msk) == 0U);

		/* Disable bypass- put PLL clock back */
		SCU_PLL->PLLCON0 &= ~SCU_PLL_PLLCON0_VCOBYP_Msk;	while ((SCU_PLL->PLLSTAT & SCU_PLL_PLLSTAT_VCOBYST_Msk) != 0U);

		/* Before scaling to final frequency we need to setup the clock dividers */
		SCU_CLK->SYSCLKCR = __SYSCLKCR;
		SCU_CLK->PBCLKCR = __PBCLKCR;
		SCU_CLK->CPUCLKCR = __CPUCLKCR;
		SCU_CLK->CCUCLKCR = __CCUCLKCR;
		SCU_CLK->WDTCLKCR = __WDTCLKCR;
		SCU_CLK->EBUCLKCR = __EBUCLKCR;
		SCU_CLK->USBCLKCR = __USBCLKCR;
		SCU_CLK->ECATCLKCR = __ECATCLKCR;
		SCU_CLK->EXTCLKCR = __EXTCLKCR;

		/* PLL frequency stepping...*/
		/* Reset OSCDISCDIS */
		SCU_PLL->PLLCON0 &= ~SCU_PLL_PLLCON0_OSCDISCDIS_Msk;

		SCU_PLL->PLLCON1 = PLL_CON1(PLL_NDIV, PLL_K2DIV_48MHZ, PLL_PDIV);	delay(DELAY_CNT_50US_48MHZ);

		SCU_PLL->PLLCON1 = PLL_CON1(PLL_NDIV, PLL_K2DIV_72MHZ, PLL_PDIV);	delay(DELAY_CNT_50US_72MHZ);

		SCU_PLL->PLLCON1 = PLL_CON1(PLL_NDIV, PLL_K2DIV_96MHZ, PLL_PDIV);	delay(DELAY_CNT_50US_96MHZ);

		SCU_PLL->PLLCON1 = PLL_CON1(PLL_NDIV, PLL_K2DIV_120MHZ, PLL_PDIV);	delay(DELAY_CNT_50US_120MHZ);

		SCU_PLL->PLLCON1 = PLL_CON1(PLL_NDIV, PLL_K2DIV, PLL_PDIV);			delay(DELAY_CNT_50US_144MHZ);

		/* Enable selected clocks */
		SCU_CLK->CLKSET = __CLKSET;

		SCU_POWER->PWRSET = SCU_POWER_PWRSET_HIB_Msk;	while((SCU_POWER->PWRSTAT & SCU_POWER_PWRSTAT_HIBEN_Msk) == 0);
		SCU_RESET->RSTCLR = SCU_RESET_RSTCLR_HIBRS_Msk;	while((SCU_RESET->RSTSTAT & SCU_RESET_RSTSTAT_HIBRS_Msk) != 0);

		if (SCU_HIBERNATE->OSCULCTRL != OSCULCTRL_MODE_BYPASS)
		{
			while (SCU_GENERAL->MIRRSTS & SCU_GENERAL_MIRRSTS_OSCULCTRL_Msk);	SCU_HIBERNATE->OSCULCTRL = OSCULCTRL_MODE_BYPASS;
			while (SCU_GENERAL->MIRRSTS & SCU_GENERAL_MIRRSTS_HDCR_Msk);		SCU_HIBERNATE->HDCR |= SCU_HIBERNATE_HDCR_ULPWDGEN_Msk;
			while (SCU_GENERAL->MIRRSTS & SCU_GENERAL_MIRRSTS_HDCLR_Msk);		SCU_HIBERNATE->HDCLR = SCU_HIBERNATE_HDCLR_ULPWDG_Msk;
		};

		while (SCU_GENERAL->MIRRSTS & SCU_GENERAL_MIRRSTS_HDCR_Msk);	SCU_HIBERNATE->HDCR |= SCU_HIBERNATE_HDCR_RCS_Msk | SCU_HIBERNATE_HDCR_STDBYSEL_Msk;

		P0->OUT = P0_INIT_OUT;
		P1->OUT = P1_INIT_OUT;
		P2->OUT = P2_INIT_OUT;
		P3->OUT = P3_INIT_OUT;
		P4->OUT = P4_INIT_OUT;
		P5->OUT = P5_INIT_OUT;
		P6->OUT = P6_INIT_OUT;

		P0->DIR(P0_INIT_DIR);
		P1->DIR(P1_INIT_DIR);
		P2->DIR(P2_INIT_DIR);
		P3->DIR(P3_INIT_DIR);
		P4->DIR(P4_INIT_DIR);
		P5->DIR(P5_INIT_DIR);
		P6->DIR(P6_INIT_DIR);

		P0->PPS = 0;
		P1->PPS = 0;
		P2->PPS = 0;
		P3->PPS = 0;
		P4->PPS = 0;
		P5->PPS = 0;
		P6->PPS = 0;
		P14->PPS = 0;
		P15->PPS = 0;

		P14->PDISC = P14_INIT_PDISC;
		P15->PDISC = P15_INIT_PDISC;

		HW::Peripheral_Enable(PID_DMA0);
		HW::Peripheral_Enable(PID_DMA1);

		HW::DLR->DRL0 	= DRL_RS0;
		HW::DLR->DRL1 	= DRL_RS1;
		HW::DLR->DRL2 	= DRL_RS2;
		HW::DLR->DRL3 	= DRL_RS3;
		HW::DLR->DRL4 	= DRL_RS4;
		HW::DLR->DRL5 	= DRL_RS5;
		HW::DLR->DRL6 	= DRL_RS6;
		HW::DLR->DRL7 	= DRL_RS7;
		HW::DLR->DRL8 	= DRL_RS8;	
		HW::DLR->DRL9 	= DRL_RS9;	
		HW::DLR->DRL10	= DRL_RS10;
		HW::DLR->DRL11	= DRL_RS11;

		HW::DLR->LNEN = 0;

	#elif defined(CPU_LPC824) || defined(CPU_LPC8XX)

		u32 i;
		using namespace CM0;
		using namespace HW;

		SYSCON->SYSAHBCLKCTRL |= CLK::SWM_M | CLK::IOCON_M | CLK::GPIO_M | HW::CLK::MRT_M | HW::CLK::UART0_M | HW::CLK::CRC_M | HW::CLK::DMA_M;

		GPIO->DIR0	= GPIO_INIT_DIR0; //(1<<27)|(1<<14)|(1<<17)|(1<<18)|(1<<19)|(1<<20)|(1<<21)|(1<<22)|(1<<12)|(1<<15);
		GPIO->PIN0	= GPIO_INIT_PIN0;
		GPIO->MASK0	= 0;

		//GPIO->CLR0 = (1<<27)|(1<<14)|(1<<20)|(1<<21)|(1<<22)|(1<<4);
		//GPIO->SET0 = (1<<17)|(1<<18)|(1<<19);

		IOCON->PIO0_1.B.MODE = 0;

		HW::GPIO->NOT0 = 1<<12;

		#ifdef CPU_LPC824

			SWM->PINENABLE0.B.CLKIN = 0;

		#elif defined(CPU_LPC8XX)

			SWM->PINENABLE0.D &= ((SYSCON->DEVICE_ID & 0xFF00) == 0x8100) ? (~(1<<7)) : (~(1<<9)); // CLKIN

		#endif

		for (i = 0; i < 200; i++) __nop(); 

		#ifdef PLL_MHz
			SYSCON->PDRUNCFG &= ~PDRUNCFG_SYSPLL_PD;
		#endif

		SYSCON->SYSPLLCLKSEL  = 3;					/* Select PLL Input         */
		SYSCON->SYSPLLCLKUEN  = 0;					/* Update Clock Source      */
		SYSCON->SYSPLLCLKUEN  = 1;					/* Update Clock Source      */
		while (!(SYSCON->SYSPLLCLKUEN & 1));		/* Wait Until Updated       */

		HW::GPIO->NOT0 = 1<<12;

		#ifdef PLL_MHz

			#if (PLL_MSEL < 1) || (PLL_MSEL > 32)
			#error PLL_MSEL must be 1...32
			#endif

			#if (PLL_PSEL > 3)
			#error PLL_PSEL must be 0...3
			#endif
			
			SYSCON->SYSPLLCTRL = SYSPLLCTRL_MSEL(PLL_MSEL-1)|SYSPLLCTRL_PSEL(PLL_PSEL);
			while (!(SYSCON->SYSPLLSTAT & SYSPLLSTAT_LOCK));
			HW::GPIO->NOT0 = 1<<12;

			SYSCON->MAINCLKSEL = MAINCLKSEL_PLL_OUT;	

		#else
		
			SYSCON->MAINCLKSEL = MAINCLKSEL_PLL_IN;				

		#endif

		SYSCON->MAINCLKUEN    = 0;					/* Update MCLK Clock Source */
		SYSCON->MAINCLKUEN    = 1;					/* Update MCLK Clock Source */
		while (!(SYSCON->MAINCLKUEN & 1));			/* Wait Until Updated       */

		HW::GPIO->NOT0 = 1<<12;

		if ((u32)MCK_MHz <= 30) HW::FLASHCTRL->FLASHCFG &= ~3;

		#ifdef MCK_DIV
		SYSCON->SYSAHBCLKDIV  = MCK_DIV;
		#endif

		#ifdef UARTCLK_DIV
			SYSCON->UARTCLKDIV = UARTCLK_DIV;
		#else
			SYSCON->UARTCLKDIV = 1;
		#endif

		//SWM->U0_RXD = 26;
		//SWM->U0_TXD = 16;

		#ifdef CPU_LPC824
			DMA->SRAMBASE = _DmaTable;
			DMA->CTRL = 1;
		#endif

		HW::GPIO->NOT0 = 1<<12;

	#endif


	#ifdef CORTEX_M4

		#if ((__FPU_PRESENT == 1) && (__FPU_USED == 1))
			CM4::SCB->CPACR |= ((3UL << 10*2) |                 /* set CP10 Full Access */
								(3UL << 11*2)  );               /* set CP11 Full Access */
		#else
			CM4::SCB->CPACR = 0;
		#endif

	  /* Enable unaligned memory access - SCB_CCR.UNALIGN_TRP = 0 */
		CM4::SCB->CCR &= ~(SCB_CCR_UNALIGN_TRP_Msk);

	#endif

	#ifndef BOOTLOADER
		SEGGER_RTT_WriteString(0, RTT_CTRL_TEXT_BRIGHT_GREEN "OK\n");
	#endif
}

#endif
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#endif // SYSTEM_IMP_H__11_10_2022__18_02
