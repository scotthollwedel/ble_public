#ifndef SPI_H
#define SPI_H
#include "serial.h"
/**
 *  SPI master operating frequency
 */
typedef enum
{
    Freq_125Kbps = 0,        /*!< drive SClk with frequency 125Kbps */
    Freq_250Kbps,            /*!< drive SClk with frequency 250Kbps */
    Freq_500Kbps,            /*!< drive SClk with frequency 500Kbps */
    Freq_1Mbps,              /*!< drive SClk with frequency 1Mbps */
    Freq_2Mbps,              /*!< drive SClk with frequency 2Mbps */
    Freq_4Mbps,              /*!< drive SClk with frequency 4Mbps */
    Freq_8Mbps               /*!< drive SClk with frequency 8Mbps */
} SPIFrequency_t;

/**
 *  SPI mode
 */
typedef enum
{
    //------------------------Clock polarity 0, Clock starts with level 0-------------------------------------------
    SPI_MODE0 = 0,          /*!< Sample data at rising edge of clock and shift serial data at falling edge */
    SPI_MODE1,              /*!< sample data at falling edge of clock and shift serial data at rising edge */
    //------------------------Clock polarity 1, Clock starts with level 1-------------------------------------------
    SPI_MODE2,              /*!< sample data at falling edge of clock and shift serial data at rising edge */
    SPI_MODE3               /*!< Sample data at rising edge of clock and shift serial data at falling edge */
} SPIMode;

#ifdef  __cplusplus
extern "C" {
#endif
typedef void (*gcSpiHandleRx)(void *p);
typedef void (*gcSpiHandleTx)(void);

extern unsigned char wlan_tx_buffer[];

//*****************************************************************************
//
// Prototypes for the APIs.
//
//*****************************************************************************
extern void SpiOpen(gcSpiHandleRx pfRxHandler);
extern void SpiClose(void);
extern long SpiWrite(unsigned char *pUserBuffer, unsigned short usLength);
extern void SpiResumeSpi(void);
extern void SpiConfigureHwMapping(	unsigned long ulPioPortAddress,
									unsigned long ulPort, 
									unsigned long ulSpiCs, 
									unsigned long ulPortInt, 
									unsigned long uluDmaPort,
									unsigned long ulSsiPortAddress,
									unsigned long ulSsiTx,
									unsigned long ulSsiRx,
									unsigned long ulSsiClck);
extern void SpiCleanGPIOISR(void);
extern int init_spi(void);
extern void ReadWriteFrame(unsigned char *pTxBuffer, unsigned char *pRxBuffer, unsigned short size);
extern long TXBufferIsEmpty(void);
extern long RXBufferIsEmpty(void);
#ifdef  __cplusplus
}
#endif // __cplusplus

void IntSpiGPIOHandler(void);
#endif //SPI_H
