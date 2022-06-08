#ifndef EXTERN_DEF_H__20_04_2022__11_02
#define EXTERN_DEF_H__20_04_2022__11_02

extern void SetNumDevice(u16 num);
extern u16 GetNumDevice();
extern void SaveMainParams();
extern u16 GetVersionDevice();
extern void SetClock(const RTC &t);
extern u16 CRC_CCITT_PIO(const void *data, u32 len, u16 init);
//extern u16 CRC_CCITT_DMA(const void *data, u32 len, u16 init);
extern bool CRC_CCITT_DMA_Async(const void* data, u32 len, u16 init);
extern bool CRC_CCITT_DMA_CheckComplete(u16* crc);
extern bool TransmitEth(EthBuf *b);
extern bool CheckStatusUDP(u32 stat);
extern bool EmacIsConnected();

#endif // EXTERN_DEF_H__20_04_2022__11_02
