#ifndef XTRAP_H__12_03_20015__11_14
#define XTRAP_H__12_03_20015__11_14

#include "types.h"
#include "emac.h"
#include "trap_def.h"
#include "EMAC_DEF.h"

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//__packed struct SmallTx : public EthUdpBuf
//{
//	TrapHdr th;		// 9
//
//	byte data[75];
//};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

extern bool RequestTrap(EthBuf *mb);
//extern void InitTraps();
extern void UpdateTraps();

//extern SmallTx*	GetSmallTxBuffer();
//extern EthBuf*	GetHugeTxBuffer();
extern void SendTrap(EthBuf *p);
extern void SendFragTrap(EthBuf *p);
inline bool IsComputerFind() {	extern bool ComputerFind; return ComputerFind; }


#endif // TRAP_H__12_03_20015__11_14
