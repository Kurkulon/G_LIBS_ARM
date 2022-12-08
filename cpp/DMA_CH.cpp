#include "DMA.h"

#ifdef CPU_SAME53


DMA_CH		DMA_CH0		(0	);
DMA_CH		DMA_CH1		(1	);
DMA_CH		DMA_CH2		(2	);
DMA_CH		DMA_CH3		(3	);
DMA_CH		DMA_CH4		(4	);
DMA_CH		DMA_CH5		(5	);
DMA_CH		DMA_CH6		(6	);
DMA_CH		DMA_CH7		(7	);
DMA_CH		DMA_CH8		(8	);
DMA_CH		DMA_CH9		(9	);
DMA_CH		DMA_CH10	(10	);
DMA_CH		DMA_CH11	(11	);
DMA_CH		DMA_CH12	(12	);
DMA_CH		DMA_CH13	(13	);
DMA_CH		DMA_CH14	(14	);
DMA_CH		DMA_CH15	(15	);
DMA_CH		DMA_CH16	(16	);
DMA_CH		DMA_CH17	(17	);
DMA_CH		DMA_CH18	(18	);
DMA_CH		DMA_CH19	(19	);
DMA_CH		DMA_CH20	(20	);
DMA_CH		DMA_CH21	(21	);
DMA_CH		DMA_CH22	(22	);
DMA_CH		DMA_CH23	(23	);
DMA_CH		DMA_CH24	(24	);
DMA_CH		DMA_CH25	(25	);
DMA_CH		DMA_CH26	(26	);
DMA_CH		DMA_CH27	(27	);
DMA_CH		DMA_CH28	(28	);
DMA_CH		DMA_CH29	(29	);
DMA_CH		DMA_CH30	(30	);
DMA_CH		DMA_CH31	(31	);
	
#elif defined(CPU_XMC48)

DMA_CH		DMA_CH0		(HW::DMA0,	0	);
DMA_CH		DMA_CH1		(HW::DMA0,	1	);
DMA_CH		DMA_CH2		(HW::DMA0,	2	);
DMA_CH		DMA_CH3		(HW::DMA0,	3	);
DMA_CH		DMA_CH4		(HW::DMA0,	4	);
DMA_CH		DMA_CH5		(HW::DMA0,	5	);
DMA_CH		DMA_CH6		(HW::DMA0,	6	);
DMA_CH		DMA_CH7		(HW::DMA0,	7	);
																
DMA_CH		DMA_CH8		(HW::DMA1,	8	);
DMA_CH		DMA_CH9		(HW::DMA1,	9	);
DMA_CH		DMA_CH10	(HW::DMA1,	10	);
DMA_CH		DMA_CH11	(HW::DMA1,	11	);

#elif defined(CPU_LPC824)


#endif

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

