#ifndef AT25DF021_IMP_H__21_06_2022__17_19
#define AT25DF021_IMP_H__21_06_2022__17_19

#include "at25df021.h"

#include "types.h"
#include "core.h"
#include "CRC16.h"

#define NUM_SECTORS 	64			/* number of sectors in the flash device */
#define SECTOR_SIZE		4096

#undef TIMEOUT
#undef DELAY

/* flash commands */
#define SPI_WREN            (0x06)  //Set Write Enable Latch
#define SPI_WRDI            (0x04)  //Reset Write Enable Latch
#define SPI_RDID            (0x9F)  //Read Identification
#define SPI_RDSR            (0x05)  //Read Status Register
#define SPI_WRSR            (0x01)  //Write Status Register
#define SPI_READ            (0x03)  //Read data from memory
#define SPI_FAST_READ       (0x0B)  //Read data from memory
#define SPI_PP              (0x02)  //Program Data into memory
#define SPI_SE              (0x20)  //Erase one sector in memory
#define SPI_BE              (0xC7)  //Erase all memory
#define RDY_BSY 			(0x1)	//Check the write in progress bit of the SPI status register
#define WEL					(0x2)	//Check the write enable bit of the SPI status register
#define SWP					(0xC)	//Software Protection Status
#define WPP					(0x10)	//Write Protect (WP) Pin Status
#define EPE					(0x20)	//Erase/Program Error
#define SPRL				(0x80)	//Sector Protection Registers Locked

#define SPI_UPS				(0x39)  //Unprotect Sector 
#define SPI_PRS				(0x36)  //Protect Sector 


#define SPI_PAGE_SIZE		(256)
//#define SPI_SECTORS		    (512)
//#define SPI_SECTOR_SIZE		(4224)
//#define SPI_SECTOR_DIFF		(3968)
//#define PAGE_BITS			(10)
//#define PAGE_SIZE_DIFF		(496)

#define DELAY				15
#define TIMEOUT				35000

#define BFLAG_FINAL         0x00008000   /* final block in stream */
#define BFLAG_FIRST         0x00004000   /* first block in stream */
#define BFLAG_INDIRECT      0x00002000   /* load data via intermediate buffer */
#define BFLAG_IGNORE        0x00001000   /* ignore block payload */
#define BFLAG_INIT          0x00000800   /* call initcode routine */
#define BFLAG_CALLBACK      0x00000400   /* call callback routine */
#define BFLAG_QUICKBOOT     0x00000200   /* boot block only when BFLAG_WAKEUP=0 */
#define BFLAG_FILL          0x00000100   /* fill memory with 32-bit argument value */
#define BFLAG_AUX           0x00000020   /* load auxiliary header -- reserved */
#define BFLAG_SAVE          0x00000010   /* save block on power down -- reserved */

struct BOOT_HEADER
{
	u32 blockCode;
	u32	targetAddress;
	u32 byteCount;
	u32 arg;
}; 

/* application definitions */
#define COMMON_SPI_SETTINGS (SPE|MSTR|CPOL|CPHA)  /* settings to the SPI_CTL */
#define TIMOD01 (0x01)                  /* sets the SPI to work with core instructions */

#define COMMON_SPI_DMA_SETTINGS (MSTR|CPOL|CPHA)  /* settings to the SPI_CTL */

#define BAUD_RATE_DIVISOR 	8
#define PE4 0x0010

//char			SPI_Page_Buffer[SPI_PAGE_SIZE];
//int 			SPI_Page_Index = 0;
//char            SPI_Status;


/* function prototypes */
static ERROR_CODE SetupForFlash();
static ERROR_CODE Wait_For_nStatus(void);
//ERROR_CODE Wait_For_Status( char Statusbit );
static ERROR_CODE Wait_For_WEL(void);
extern void SetupSPI();
extern void SPI_OFF(void);
void SendSingleCommand( const int iCommand );
//unsigned long DataFlashAddress (unsigned long address);

static ERROR_CODE PollToggleBit(void);
static byte ReadFlash();
static void WriteFlash(byte usValue);
static unsigned long GetFlashStartAddress( unsigned long ulAddr);
static void GlobalUnProtect();


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#endif // AT25DF021_IMP_H__21_06_2022__17_19
