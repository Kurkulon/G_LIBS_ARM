#ifndef EMAC_DEF_H__13_06_2013__09_47
#define EMAC_DEF_H__13_06_2013__09_47

#include "core.h"


/* EMAC Memory Buffer configuration. */
#define NUM_RX_BUF          8          /* 0x2000 for Rx (64*128=8K)         */
#define ETH_RX_DRBS			8
#define ETH_RX_BUF_SIZE     (ETH_RX_DRBS * 64)       /* EMAC Receive buffer size.         */


#define NUM_TX_DSC          16         /* 0x0600 for Tx                     */
//#define ETH_TX_BUF_SIZE     1536        /* EMAC Transmit buffer size         */

#ifndef WIN32
#define IP_MTU				1480
#else
#define IP_MTU				0x4000
#endif

//#define AT91C_PHY_ADDR      0


#define ARP_REQUEST			0x0001
#define ARP_REPLY			0x0002
#define PROT_ARP			0x0806
#define PROT_IP				0x0800
#define PROT_ICMP			0x01
#define PROT_UDP			17
#define PROT_TCP			6
#define ICMP_ECHO_REQUEST	0x08
#define ICMP_ECHO_REPLY		0x00

#define SWAP16(x)	((u16)((x) << 8) | ((x) >> 8))
#define SWAP32(x)	(((x)>>24) | (((x)&0x00ff0000) >> 8) | (((x)&0x0000ff00) << 8) | (((u32)(x)<<24)))


#define BOOTPS SWAP16(67)	// server DHCP
#define BOOTPC SWAP16(68)	// client DHCP

#define	DHCPDISCOVER	1  
#define	DHCPOFFER		2  
#define	DHCPREQUEST		3  
#define	DHCPDECLINE		4  
#define	DHCPACK			5  
#define	DHCPNAK			6  
#define	DHCPRELEASE		7  
#define DHCPCOOKIE		0x63538263

///* Absolute IO access macros */
//#define pEMAC   AT91C_BASE_EMACB
//#define pPIOA   AT91C_BASE_PIOA
//#define pRSTC   AT91C_BASE_RSTC
//#define pAIC    AT91C_BASE_AIC
//#define pPMC    AT91C_BASE_PMC

//#define PHYA 0

#define IP32(b1, b2, b3, b4) (((u32)b1&0xFF)|(((u32)b2&0xFF)<<8)|(((u32)b3&0xFF)<<16)|(((u32)b4&0xFF)<<24))

#ifdef CPU_SAME53	

	#define OWNERSHIP_BIT		1
	#define WRAP_BIT			2


#elif defined(CPU_XMC48)


#endif









/* DP83848C PHY Registers */
#define PHY_REG_BMCR        0x00        /* Basic Mode Control Register       */
#define PHY_REG_BMSR        0x01        /* Basic Mode Status Register        */
#define PHY_REG_IDR1        0x02        /* PHY Identifier 1                  */
#define PHY_REG_IDR2        0x03        /* PHY Identifier 2                  */
#define PHY_REG_ANAR        0x04        /* Auto-Negotiation Advertisement    */
#define PHY_REG_ANLPAR      0x05        /* Auto-Neg. Link Partner Abitily    */
#define PHY_REG_ANER        0x06        /* Auto-Neg. Expansion Register      */
#define PHY_REG_ANNPTR      0x07        /* Auto-Neg. Next Page TX            */

/* PHY Extended Registers */
//#define PHY_REG_PHYSTS      0x10        /* PHY Status Register               */
//#define PHY_REG_MICR        0x11        /* MII Interrupt Control Register    */
//#define PHY_REG_MISR        0x12        /* MII Interrupt Status Register     */
//#define PHY_REG_FCSCR       0x14        /* False Carrier Sense Counter       */
//#define PHY_REG_RECR        0x15        /* Receive Error Counter             */
//#define PHY_REG_PCSR        0x16        /* PCS Sublayer Config. and Status   */
//#define PHY_REG_RBR         0x17        /* RMII and Bypass Register          */
//#define PHY_REG_LEDCR       0x18        /* LED Direct Control Register       */
//#define PHY_REG_PHYCR       0x19        /* PHY Control Register              */
//#define PHY_REG_10BTSCR     0x1A        /* 10Base-T Status/Control Register  */
//#define PHY_REG_CDCTRL1     0x1B        /* CD Test Control and BIST Extens.  */
//#define PHY_REG_EDCR        0x1D        /* Energy Detect Control Register    */

#define PHY_REG_DRC         0x10
#define PHY_REG_AFECON1     0x11
#define PHY_REG_RXERCTR     0x15
#define PHY_REG_OMSO        0x16
#define PHY_REG_OMSS        0x17
#define PHY_REG_EXCON       0x18
#define PHY_REG_ICSR        0x1B
#define PHY_REG_LINKMDCS    0x1D
#define PHY_REG_PHYCON1     0x1E
#define PHY_REG_PHYCON2     0x1F

#define PHY_FULLD_100M      0x2100      /* Full Duplex 100Mbit               */
#define PHY_HALFD_100M      0x2000      /* Half Duplex 100Mbit               */
#define PHY_FULLD_10M       0x0100      /* Full Duplex 10Mbit                */
#define PHY_HALFD_10M       0x0000      /* Half Duplex 10MBit                */
#define PHY_AUTO_NEG        0x3000      /* Select Auto Negotiation           */

#define PHYSTS_LINKST		0x0001		/* Link status                       */
#define PHYSTS_SPEEDST		0x0002		/* Speed status: 1=10Mb, 0=100Mb     */
#define PHYSTS_DUPLEXST		0x0004		/* Duplex: 1=Full, 0=Half			 */


#define DP83848C_DEF_ADR    0x0100      /* Default PHY device address        */
#define DP83848C_ID         0x20005C90  /* PHY Identifier                    */


/* Basic mode control register. */
#define BMCR_RESV           0x007f      /* Unused...                         */
#define BMCR_CTST           0x0080      /* Collision test                    */
#define BMCR_FULLDPLX       0x0100      /* Full duplex                       */
#define BMCR_ANRESTART      0x0200      /* Auto negotiation restart          */
#define BMCR_ISOLATE        0x0400      /* Disconnect DM9161 from MII        */
#define BMCR_PDOWN          0x0800      /* Powerdown the DM9161              */
#define BMCR_ANENABLE       0x1000      /* Enable auto negotiation           */
#define BMCR_SPEED100       0x2000      /* Select 100Mbps                    */
#define BMCR_LOOPBACK       0x4000      /* TXD loopback bits                 */
#define BMCR_RESET          0x8000      /* Reset the DM9161                  */

/* Basic mode status register. */
#define BMSR_ERCAP          0x0001      /* Ext-reg capability                */
#define BMSR_JCD            0x0002      /* Jabber detected                   */
#define BMSR_LINKST         0x0004      /* Link status                       */
#define BMSR_ANEGCAPABLE    0x0008      /* Able to do auto-negotiation       */
#define BMSR_RFAULT         0x0010      /* Remote fault detected             */
#define BMSR_ANEGCOMPLETE   0x0020      /* Auto-negotiation complete         */
#define BMSR_MIIPRESUP      0x0040      /* MII Frame Preamble Suppression    */
#define BMSR_RESV           0x0780      /* Unused...                         */
#define BMSR_10HALF         0x0800      /* Can do 10mbps, half-duplex        */
#define BMSR_10FULL         0x1000      /* Can do 10mbps, full-duplex        */
#define BMSR_100HALF        0x2000      /* Can do 100mbps, half-duplex       */
#define BMSR_100FULL        0x4000      /* Can do 100mbps, full-duplex       */
#define BMSR_100BASE4       0x8000      /* Can do 100mbps, 4k packets        */

/* Advertisement control register. */
#define ANAR_SLCT           0x001f      /* Selector bits                     */
#define ANAR_CSMA           0x0001      /* Only selector supported           */
#define ANAR_10HALF         0x0020      /* Try for 10mbps half-duplex        */
#define ANAR_10FULL         0x0040      /* Try for 10mbps full-duplex        */
#define ANAR_100HALF        0x0080      /* Try for 100mbps half-duplex       */
#define ANAR_100FULL        0x0100      /* Try for 100mbps full-duplex       */
#define ANAR_100BASE4       0x0200      /* Try for 100mbps 4k packets        */
#define ANAR_FCS            0x0400      /* Try for Flow Control Support      */
#define ANAR_RESV           0x1800      /* Unused...                         */
#define ANAR_RFAULT         0x2000      /* Say we can detect faults          */
#define ANAR_LPACK          0x4000      /* Ack link partners response        */
#define ANAR_NPAGE          0x8000      /* Next page bit                     */

#define ANAR_FULL           (ANAR_100FULL | ANAR_10FULL | ANAR_CSMA)
#define ANAR_ALL            (ANAR_10HALF  | ANAR_10FULL | ANAR_100HALF | ANAR_100FULL)

/* Link partner ability register. */
#define ANLPAR_SLCT         0x001f      /* Same as advertise selector        */
#define ANLPAR_10HALF       0x0020      /* Can do 10mbps half-duplex         */
#define ANLPAR_10FULL       0x0040      /* Can do 10mbps full-duplex         */
#define ANLPAR_100HALF      0x0080      /* Can do 100mbps half-duplex        */
#define ANLPAR_100FULL      0x0100      /* Can do 100mbps full-duplex        */
#define ANLPAR_100BASE4     0x0200      /* Can do 100mbps 4k packets         */
#define ANLPAR_FCS          0x0400      /* Can do Flow Control Support       */
#define ANLPAR_RESV         0x1800      /* Unused...                         */
#define ANLPAR_RFAULT       0x2000      /* Link partner faulted              */
#define ANLPAR_LPACK        0x4000      /* Link partner acked us             */
#define ANLPAR_NPAGE        0x8000      /* Next page bit                     */

#define ANLPAR_DUPLEX       (ANLPAR_10FULL  | ANLPAR_100FULL)
#define ANLPAR_100          (ANLPAR_100FULL | ANLPAR_100HALF | ANLPAR_100BASE4)

/* Expansion register for auto-negotiation. */
#define ANER_LPANABLE       0x0001      /* Link Partner AutoNegotiation able */
#define ANER_PAGERX         0x0002      /* New Page received                 */
#define ANER_NPABLE         0x0004      /* Local Device next page able       */
#define ANER_LPNPABLE       0x0008      /* Link partner supports npage       */
#define ANER_PDF            0x0010      /* Parallel Detection fault          */
#define ANER_RESV           0xFFE0      /* Unused...                         */

//DRC register
#define DRC_PLL_OFF                 (1 << 4)

//AFECON1 register
#define AFECON1_SLOW_OSC_MODE_EN    (1 << 5)

//OMSO register
#define OMSO_BCAST_OFF_OVERRIDE     (1 << 9)
#define OMSO_MII_BTB_OVERRIDE       (1 << 7)
#define OMSO_RMII_BTB_OVERRIDE      (1 << 6)
#define OMSO_NAND_TREE_OVERRIDE     (1 << 5)
#define OMSO_RMII_OVERRIDE          (1 << 1)
#define OMSO_MII_OVERRIDE           (1 << 0)

//OMSS register
#define OMSS_PHYAD2                 (1 << 15)
#define OMSS_PHYAD1                 (1 << 14)
#define OMSS_PHYAD0                 (1 << 13)
#define OMSS_RMII_STATUS            (1 << 1)

//EXCON register
#define EXCON_EDPD_EN				(0 << 11)
#define EXCON_EDPD_DIS              (1 << 11)

//ICSR register
#define ICSR_JABBER_IE              (1 << 15)
#define ICSR_RECEIVE_ERROR_IE       (1 << 14)
#define ICSR_PAGE_RECEIVED_IE       (1 << 13)
#define ICSR_PAR_DET_FAULT_IE       (1 << 12)
#define ICSR_LP_ACK_IE              (1 << 11)
#define ICSR_LINK_DOWN_IE           (1 << 10)
#define ICSR_REMOTE_FAULT_IE        (1 << 9)
#define ICSR_LINK_UP_IE             (1 << 8)
#define ICSR_JABBER_IF              (1 << 7)
#define ICSR_RECEIVE_ERROR_IF       (1 << 6)
#define ICSR_PAGE_RECEIVED_IF       (1 << 5)
#define ICSR_PAR_DET_FAULT_IF       (1 << 4)
#define ICSR_LP_ACK_IF              (1 << 3)
#define ICSR_LINK_DOWN_IF           (1 << 2)
#define ICSR_REMOTE_FAULT_IF        (1 << 1)
#define ICSR_LINK_UP_IF             (1 << 0)

//LINKMDCS register
#define LINKMDCS_CABLE_DIAG_EN      (1 << 15)
#define LINKMDCS_CABLE_DIAG_RES1    (1 << 14)
#define LINKMDCS_CABLE_DIAG_RES0    (1 << 13)
#define LINKMDCS_SHORT_CABLE        (1 << 12)
#define LINKMDCS_CABLE_FAULT_CNT8   (1 << 8)
#define LINKMDCS_CABLE_FAULT_CNT7   (1 << 7)
#define LINKMDCS_CABLE_FAULT_CNT6   (1 << 6)
#define LINKMDCS_CABLE_FAULT_CNT5   (1 << 5)
#define LINKMDCS_CABLE_FAULT_CNT4   (1 << 4)
#define LINKMDCS_CABLE_FAULT_CNT3   (1 << 3)
#define LINKMDCS_CABLE_FAULT_CNT2   (1 << 2)
#define LINKMDCS_CABLE_FAULT_CNT1   (1 << 1)
#define LINKMDCS_CABLE_FAULT_CNT0   (1 << 0)

//PHYCON1 register
#define PHYCON1_PAUSE_EN            (1 << 9)
#define PHYCON1_LINK_STATUS         (1 << 8)
#define PHYCON1_POL_STATUS          (1 << 7)
#define PHYCON1_MDIX_STATE          (1 << 5)
#define PHYCON1_ENERGY_DETECT       (1 << 4)
#define PHYCON1_ISOLATE             (1 << 3)
#define PHYCON1_OP_MODE2            (1 << 2)
#define PHYCON1_OP_MODE1            (1 << 1)
#define PHYCON1_OP_MODE0            (1 << 0)

//Operation mode indication
#define PHYCON1_OP_MODE_MASK        (7 << 0)
#define PHYCON1_OP_MODE_AN          (0 << 0)
#define PHYCON1_OP_MODE_10BT        (1 << 0)
#define PHYCON1_OP_MODE_100BTX      (2 << 0)
#define PHYCON1_OP_MODE_10BT_FD     (5 << 0)
#define PHYCON1_OP_MODE_100BTX_FD   (6 << 0)

//PHYCON2 register
#define PHYCON2_HP_MDIX             (1 << 15)
#define PHYCON2_MDIX_SEL            (1 << 14)
#define PHYCON2_PAIR_SWAP_DIS       (1 << 13)
#define PHYCON2_FORCE_LINK          (1 << 11)
#define PHYCON2_POWER_SAVING        (1 << 10)
#define PHYCON2_INT_LEVEL           (1 << 9)
#define PHYCON2_JABBER_EN           (1 << 8)
#define PHYCON2_RMII_REF_CLK_SEL    (1 << 7)
#define PHYCON2_LED_MODE1           (1 << 5)
#define PHYCON2_LED_MODE0           (1 << 4)
#define PHYCON2_TX_DIS              (1 << 3)
#define PHYCON2_REMOTE_LOOPBACK     (1 << 2)
#define PHYCON2_SCRAMBLER_DIS       (1 << 0)


/* PHY ID */
#define MII_DM9161_ID       0x0181b8a0
#define MII_AM79C875_ID     0x00225540  /* 0x00225541                        */

/* Definitions */
#define ETH_ADRLEN      6         /* Ethernet Address Length in bytes        */
#define IP_ADRLEN       4         /* IP Address Length in bytes              */
#define OS_HEADER_LEN   4         /* TCPnet 'os_frame' header size           */
                                  /* Frame Header length common for all      */
#define PHY_HEADER_LEN  (2*ETH_ADRLEN + 2) /* network interfaces.            */
#define ETH_MTU         1514      /* Ethernet Frame Max Transfer Unit        */
#define PPP_PROT_IP     0x0021    /* PPP Protocol type: IP                   */
#define TCP_DEF_WINSIZE 4380      /* TCP default window size                 */
#define PASSW_SZ        20        /* Authentication Password Buffer size     */

/* Network Interfaces */
#define NETIF_ETH       0         /* Network interface: Ethernet             */
#define NETIF_PPP       1         /* Network interface: PPP                  */
#define NETIF_SLIP      2         /* Network interface: Slip                 */
#define NETIF_LOCAL     3         /* Network interface: Localhost (loopback) */
#define NETIF_NULL      4         /* Network interface: Null (none)          */

/* Telnet Definitions */
#define TNET_LBUFSZ     96        /* Command Line buffer size (bytes)        */
#define TNET_HISTSZ     128       /* Command History buffer size (bytes)     */
#define TNET_FIFOSZ     128       /* Input character Fifo buffer (bytes)     */

/* SNMP-MIB Definitions */
#define MIB_INTEGER     0x02      /* MIB entry type INTEGER                  */
#define MIB_OCTET_STR   0x04      /* MIB entry type OCTET_STRING             */
#define MIB_OBJECT_ID   0x06      /* MIB entry type OBJECT_IDENTIFIER        */
#define MIB_IP_ADDR     0x40      /* MIB entry type IP ADDRESS (U8[4])       */
#define MIB_COUNTER     0x41      /* MIB entry type COUNTER (U32)            */
#define MIB_GAUGE       0x42      /* MIB entry type GAUGE (U32)              */
#define MIB_TIME_TICKS  0x43      /* MIB entry type TIME_TICKS               */
#define MIB_ATR_RO      0x80      /* MIB entry attribute READ_ONLY           */
#define MIB_OIDSZ       17        /* Max.size of Object ID value             */
#define MIB_STRSZ       110       /* Max.size of Octet String variable       */
#define MIB_READ        0         /* MIB entry Read access                   */
#define MIB_WRITE       1         /* MIB entry Write access                  */

/* SNMP-MIB Macros */
#define MIB_STR(s)      sizeof(s)-1, s
#define MIB_INT(o)      sizeof(o), (void *)&o
#define MIB_IP(ip)      4, (void *)&ip 
#define OID0(f,s)       (f*40 + s) 

/* Debug Module Definitions */
#define MODULE_MEM      0         /* Dynamic Memory Module ID                */
#define MODULE_ETH      1         /* Ethernet Module ID                      */
#define MODULE_PPP      2         /* PPP Module ID                           */
#define MODULE_SLIP     3         /* SLIP Module ID                          */
#define MODULE_ARP      4         /* ARP Module ID                           */
#define MODULE_IP       5         /* IP Module ID                            */
#define MODULE_ICMP     6         /* ICMP Module ID                          */
#define MODULE_IGMP     7         /* IGMP Module ID                          */
#define MODULE_UDP      8         /* UDP Module ID                           */
#define MODULE_TCP      9         /* TCP Module ID                           */
#define MODULE_NBNS     10        /* NBNS Module ID                          */
#define MODULE_DHCP     11        /* DHCP Module ID                          */
#define MODULE_DNS      12        /* DNS Module ID                           */
#define MODULE_SNMP     13        /* SNMP Module ID                          */
#define MODULE_BSD      14        /* BSD Socket Module ID                    */
#define MODULE_HTTP     15        /* HTTP Server Module ID                   */
#define MODULE_FTP      16        /* FTP Server Module ID                    */
#define MODULE_FTPC     17        /* FTP Client Module ID                    */
#define MODULE_TNET     18        /* Telnet Server Module ID                 */
#define MODULE_TFTP     19        /* TFTP Server Module ID                   */
#define MODULE_TFTPC    20        /* TFTP Client Module ID                   */
#define MODULE_SMTP     21        /* SMTP Client Module ID                   */



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++







#endif // EMAC_DEF_H__13_06_2013__09_47
