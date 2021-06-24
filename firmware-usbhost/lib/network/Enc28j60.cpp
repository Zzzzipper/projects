#include "Enc28j60.h"

#include "common/logger/include/Logger.h"
#include "common/timer/stm32/include/SystemTimer.h"
#include "config.h"

#include "stm32f4xx_conf.h"

#include <stdio.h>
#include <string.h>

// ENC28J60 Control Registers
// Control register definitions are a combination of address,
// bank number, and Ethernet/MAC/PHY indicator bits.
// - Register address        (bits 0-4)
// - Bank number        (bits 5-6)
// - MAC/PHY indicator        (bit 7)
#define ADDR_MASK        0x1F
#define BANK_MASK        0x60
#define SPRD_MASK        0x80
// All-bank registers
#define EIE              0x1B
#define EIR              0x1C
#define ESTAT            0x1D
#define ECON2            0x1E
#define ECON1            0x1F
// Bank 0 registers
#define ERDPTL           (0x00|0x00)
#define ERDPTH           (0x01|0x00)
#define EWRPTL           (0x02|0x00)
#define EWRPTH           (0x03|0x00)
#define ETXSTL           (0x04|0x00)
#define ETXSTH           (0x05|0x00)
#define ETXNDL           (0x06|0x00)
#define ETXNDH           (0x07|0x00)
#define ERXSTL           (0x08|0x00)
#define ERXSTH           (0x09|0x00)
#define ERXNDL           (0x0A|0x00)
#define ERXNDH           (0x0B|0x00)
#define ERXRDPTL         (0x0C|0x00)
#define ERXRDPTH         (0x0D|0x00)
#define ERXWRPTL         (0x0E|0x00)
#define ERXWRPTH         (0x0F|0x00)
#define EDMASTL          (0x10|0x00)
#define EDMASTH          (0x11|0x00)
#define EDMANDL          (0x12|0x00)
#define EDMANDH          (0x13|0x00)
#define EDMADSTL         (0x14|0x00)
#define EDMADSTH         (0x15|0x00)
#define EDMACSL          (0x16|0x00)
#define EDMACSH          (0x17|0x00)
// Bank 1 registers
#define EHT0             (0x00|0x20)
#define EHT1             (0x01|0x20)
#define EHT2             (0x02|0x20)
#define EHT3             (0x03|0x20)
#define EHT4             (0x04|0x20)
#define EHT5             (0x05|0x20)
#define EHT6             (0x06|0x20)
#define EHT7             (0x07|0x20)
#define EPMM0            (0x08|0x20)
#define EPMM1            (0x09|0x20)
#define EPMM2            (0x0A|0x20)
#define EPMM3            (0x0B|0x20)
#define EPMM4            (0x0C|0x20)
#define EPMM5            (0x0D|0x20)
#define EPMM6            (0x0E|0x20)
#define EPMM7            (0x0F|0x20)
#define EPMCSL           (0x10|0x20)
#define EPMCSH           (0x11|0x20)
#define EPMOL            (0x14|0x20)
#define EPMOH            (0x15|0x20)
#define EWOLIE           (0x16|0x20)
#define EWOLIR           (0x17|0x20)
#define ERXFCON          (0x18|0x20)
#define EPKTCNT          (0x19|0x20)
// Bank 2 registers
#define MACON1           (0x00|0x40|0x80)
#define MACON2           (0x01|0x40|0x80)
#define MACON3           (0x02|0x40|0x80)
#define MACON4           (0x03|0x40|0x80)
#define MABBIPG          (0x04|0x40|0x80)
#define MAIPGL           (0x06|0x40|0x80)
#define MAIPGH           (0x07|0x40|0x80)
#define MACLCON1         (0x08|0x40|0x80)
#define MACLCON2         (0x09|0x40|0x80)
#define MAMXFLL          (0x0A|0x40|0x80)
#define MAMXFLH          (0x0B|0x40|0x80)
#define MAPHSUP          (0x0D|0x40|0x80)
#define MICON            (0x11|0x40|0x80)
#define MICMD            (0x12|0x40|0x80)
#define MIREGADR         (0x14|0x40|0x80)
#define MIWRL            (0x16|0x40|0x80)
#define MIWRH            (0x17|0x40|0x80)
#define MIRDL            (0x18|0x40|0x80)
#define MIRDH            (0x19|0x40|0x80)
// Bank 3 registers
#define MAADR1           (0x00|0x60|0x80)
#define MAADR0           (0x01|0x60|0x80)
#define MAADR3           (0x02|0x60|0x80)
#define MAADR2           (0x03|0x60|0x80)
#define MAADR5           (0x04|0x60|0x80)
#define MAADR4           (0x05|0x60|0x80)
#define EBSTSD           (0x06|0x60)
#define EBSTCON          (0x07|0x60)
#define EBSTCSL          (0x08|0x60)
#define EBSTCSH          (0x09|0x60)
#define MISTAT           (0x0A|0x60|0x80)
#define EREVID           (0x12|0x60)
#define ECOCON           (0x15|0x60)
#define EFLOCON          (0x17|0x60)
#define EPAUSL           (0x18|0x60)
#define EPAUSH           (0x19|0x60)
// PHY registers
#define PHCON1           0x00
#define PHSTAT1          0x01
#define PHHID1           0x02
#define PHHID2           0x03
#define PHCON2           0x10
#define PHSTAT2          0x11
#define PHIE             0x12
#define PHIR             0x13
#define PHLCON           0x14

// ENC28J60 ERXFCON Register Bit Definitions
#define ERXFCON_UCEN     0x80
#define ERXFCON_ANDOR    0x40
#define ERXFCON_CRCEN    0x20
#define ERXFCON_PMEN     0x10
#define ERXFCON_MPEN     0x08
#define ERXFCON_HTEN     0x04
#define ERXFCON_MCEN     0x02
#define ERXFCON_BCEN     0x01
// ENC28J60 EIE Register Bit Definitions
#define EIE_INTIE        0x80
#define EIE_PKTIE        0x40
#define EIE_DMAIE        0x20
#define EIE_LINKIE       0x10
#define EIE_TXIE         0x08
#define EIE_WOLIE        0x04
#define EIE_TXERIE       0x02
#define EIE_RXERIE       0x01
// ENC28J60 EIR Register Bit Definitions
#define EIR_PKTIF        0x40
#define EIR_DMAIF        0x20
#define EIR_LINKIF       0x10
#define EIR_TXIF         0x08
#define EIR_WOLIF        0x04
#define EIR_TXERIF       0x02
#define EIR_RXERIF       0x01
// ENC28J60 ESTAT Register Bit Definitions
#define ESTAT_INT        0x80
#define ESTAT_LATECOL    0x10
#define ESTAT_RXBUSY     0x04
#define ESTAT_TXABRT     0x02
#define ESTAT_CLKRDY     0x01
// ENC28J60 ECON2 Register Bit Definitions
#define ECON2_AUTOINC    0x80
#define ECON2_PKTDEC     0x40
#define ECON2_PWRSV      0x20
#define ECON2_VRPS       0x08
// ENC28J60 ECON1 Register Bit Definitions
#define ECON1_TXRST      0x80
#define ECON1_RXRST      0x40
#define ECON1_DMAST      0x20
#define ECON1_CSUMEN     0x10
#define ECON1_TXRTS      0x08
#define ECON1_RXEN       0x04
#define ECON1_BSEL1      0x02
#define ECON1_BSEL0      0x01
// ENC28J60 MACON1 Register Bit Definitions
#define MACON1_LOOPBK    0x10
#define MACON1_TXPAUS    0x08
#define MACON1_RXPAUS    0x04
#define MACON1_PASSALL   0x02
#define MACON1_MARXEN    0x01
// ENC28J60 MACON2 Register Bit Definitions
#define MACON2_MARST     0x80
#define MACON2_RNDRST    0x40
#define MACON2_MARXRST   0x08
#define MACON2_RFUNRST   0x04
#define MACON2_MATXRST   0x02
#define MACON2_TFUNRST   0x01
// ENC28J60 MACON3 Register Bit Definitions
#define MACON3_PADCFG2   0x80
#define MACON3_PADCFG1   0x40
#define MACON3_PADCFG0   0x20
#define MACON3_TXCRCEN   0x10
#define MACON3_PHDRLEN   0x08
#define MACON3_HFRMLEN   0x04
#define MACON3_FRMLNEN   0x02
#define MACON3_FULDPX    0x01
// ENC28J60 MICMD Register Bit Definitions
#define MICMD_MIISCAN    0x02
#define MICMD_MIIRD      0x01
// ENC28J60 MISTAT Register Bit Definitions
#define MISTAT_NVALID    0x04
#define MISTAT_SCAN      0x02
#define MISTAT_BUSY      0x01
// ENC28J60 PHY PHCON1 Register Bit Definitions
#define PHCON1_PRST      0x8000
#define PHCON1_PLOOPBK   0x4000
#define PHCON1_PPWRSV    0x0800
#define PHCON1_PDPXMD    0x0100
// ENC28J60 PHY PHSTAT1 Register Bit Definitions
#define PHSTAT1_PFDPX    0x1000
#define PHSTAT1_PHDPX    0x0800
#define PHSTAT1_LLSTAT   0x0004
#define PHSTAT1_JBSTAT   0x0002
// ENC28J60 PHY PHCON2 Register Bit Definitions
#define PHCON2_FRCLINK   0x4000
#define PHCON2_TXDIS     0x2000
#define PHCON2_JABBER    0x0400
#define PHCON2_HDLDIS    0x0100

// ENC28J60 Packet Control Byte Bit Definitions
#define PKTCTRL_PHUGEEN  0x08
#define PKTCTRL_PPADEN   0x04
#define PKTCTRL_PCRCEN   0x02
#define PKTCTRL_POVERRIDE 0x01

// SPI operation codes
#define ENC28J60_READ_CTRL_REG       0x00
#define ENC28J60_READ_BUF_MEM        0x3A
#define ENC28J60_WRITE_CTRL_REG      0x40
#define ENC28J60_WRITE_BUF_MEM       0x7A
#define ENC28J60_BIT_FIELD_SET       0x80
#define ENC28J60_BIT_FIELD_CLR       0xA0
#define ENC28J60_SOFT_RESET          0xFF


// The RXSTART_INIT should be zero. See Rev. B4 Silicon Errata
// buffer boundaries applied to internal 8K ram
// the entire available packet buffer space is allocated
//
// start with recbuf at 0/
#define RXSTART_INIT     0x0
// receive buffer end
#define RXSTOP_INIT      (0x1FFF-0x0600-1)
// start TX buffer at 0x1FFF-0x0600, pace for one full ethernet frame (~1500 bytes)
#define TXSTART_INIT     (0x1FFF-0x0600)
// stp TX buffer at end of mem
#define TXSTOP_INIT      0x1FFF
//
// max frame length which the conroller will accept:
#define        MAX_FRAMELEN        1500        // (note: maximum ethernet frame length would be 1518)
//#define MAX_FRAMELEN     600

#define PHLCON_VALUE	0x476 	// LEDA = Display link status (0b0100), LEDB = Display transmit and receive activity (0b0111), LFRQ = Stretch LED events by TMSTRCH (0b01), STRCH =  Stretchable LED events will cause lengthened LED pulses based on LFRQ<1:0> configuration, (1)

ENC28J60::ENC28J60(SPI *spi) : spi(spi) {
//	LOG_DEBUG(LOG_ENC28J60, "");

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

#if (HW_VERSION == HW_2_0_0 || HW_VERSION >= HW_3_5_x)
#elif (HW_VERSION >= HW_3_0_0)
	GPIO_InitTypeDef gpio;
    GPIO_StructInit(&gpio);
    gpio.GPIO_OType = GPIO_OType_PP;
    gpio.GPIO_Mode = GPIO_Mode_OUT;
    gpio.GPIO_Pin = ETH_RESET_PIN;
    GPIO_Init(ETH_RESET_PORT, &gpio);

    resetChip(true);
    SystemTimer::get()->delay_ms(50);
    resetChip(false);
#else
#error "HW_VERSION must be defined in project settings"
#endif
}

void ENC28J60::init(uint8_t *macaddr) {
	LOG_DEBUG(LOG_ETH, "init");
	SystemTimer::get()->delay_ms(12);
	enc28j60Init(macaddr);
	enc28j60clkout(2); 					// change clkout from 6.25MHz to 12.5MHz
	SystemTimer::get()->delay_ms(12);
	enc28j60PhyWrite(PHLCON, PHLCON_VALUE);
	SystemTimer::get()->delay_ms(12); 	// 12ms
#if 0
	setFullDuplexMode();
	SystemTimer::get()->delay_ms(25);
#endif
}

void ENC28J60::setHalfDuplexMode()
{
	// MACON3.FULDPX = 0 and PHCON1.PDPXMD = 0

	uint16_t reg = enc28j60PhyRead(PHCON1);
	reg &= ~PHCON1_PDPXMD;
	enc28j60PhyWrite(PHCON1, reg);

	enc28j60WriteOp(ENC28J60_BIT_FIELD_CLR, MACON3, MACON3_FULDPX);
}

void ENC28J60::setFullDuplexMode()
{
	// MACON3.FULDPX = 1 and PHCON1.PDPXMD = 1
	uint16_t reg = enc28j60PhyRead(PHCON1);
	reg |= PHCON1_PDPXMD;
	enc28j60PhyWrite(PHCON1, reg);

	enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, MACON3, MACON3_FULDPX);
}

ENC28J60::DuplexMode ENC28J60::getDuplexMode()
{
//	uint16_t reg = enc28j60PhyReadH(PHCON1);
//	enc28j60ReadOp(ENC28J60_BIT_FIELD_SET, MACON3, MACON3_FULDPX);

	return DuplexMode::Error;
}

uint16_t ENC28J60::recvFrame(uint8_t* frame, uint16_t maxlen) {
	uint16_t rxstat;
	uint16_t len;
	// check if a packet has been received and buffered
	//if( !(enc28j60Read(EIR) & EIR_PKTIF) ){
	// The above does not work. See Rev. B4 Silicon Errata point 6.
	if(enc28j60Read(EPKTCNT) == 0){
		return(0);
	}

	// Set the read pointer to the start of the received packet
	enc28j60WriteWord(ERDPTL, gNextPacketPtr);
	//enc28j60Write(ERDPTL, (gNextPacketPtr &0xFF));
	//enc28j60Write(ERDPTH, (gNextPacketPtr)>>8);
	// read the next packet pointer
	gNextPacketPtr  = enc28j60ReadBufferWord();
	//gNextPacketPtr  = enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0);
	//gNextPacketPtr |= enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0)<<8;
	// read the packet length (see datasheet page 43)
	len = enc28j60ReadBufferWord() - 4;
	//len = enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0);
	//len |= enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0)<<8;
	//len-=4; //remove the CRC count
	// read the receive status (see datasheet page 43)
	rxstat  = enc28j60ReadBufferWord();
	//rxstat  = enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0);
	//rxstat |= ((uint16_t)enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0))<<8;
	// limit retrieve length
	if (len>maxlen-1){
        len=maxlen-1;
	}
	// check CRC and symbol errors (see datasheet page 44, table 7-3):
	// The ERXFCON.CRCEN is set by default. Normally we should not
	// need to check this.
	if ((rxstat & 0x80)==0){
        // invalid
        len=0;
	}else{
        // copy the packet from the receive buffer
        enc28j60ReadBuffer(len, frame);
	}
	// Move the RX read pointer to the start of the next received packet
	// This frees the memory we just read out
	enc28j60WriteWord(ERXRDPTL, gNextPacketPtr );
	//enc28j60Write(ERXRDPTL, (gNextPacketPtr &0xFF));
	//enc28j60Write(ERXRDPTH, (gNextPacketPtr)>>8);
	// However, compensate for the errata point 13, rev B4: enver write an even address!
	if ((gNextPacketPtr - 1 < RXSTART_INIT)
        || (gNextPacketPtr -1 > RXSTOP_INIT)) {
        enc28j60WriteWord(ERXRDPTL, RXSTOP_INIT);
        //enc28j60Write(ERXRDPTL, (RXSTOP_INIT)&0xFF);
        //enc28j60Write(ERXRDPTH, (RXSTOP_INIT)>>8);
	} else {
        enc28j60WriteWord(ERXRDPTL, (gNextPacketPtr-1));
        //enc28j60Write(ERXRDPTL, (gNextPacketPtr-1)&0xFF);
        //enc28j60Write(ERXRDPTH, (gNextPacketPtr-1)>>8);
	}
	// decrement the packet counter indicate we are done with this packet
	enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON2, ECON2_PKTDEC);
	return(len);
}

void ENC28J60::sendFrame(uint8_t* frame, uint16_t len)
{
    // Check no transmit in progress
    while (enc28j60ReadOp(ENC28J60_READ_CTRL_REG, ECON1) & ECON1_TXRTS)
    {
            // Reset the transmit logic problem. See Rev. B4 Silicon Errata point 12.
            if( (enc28j60Read(EIR) & EIR_TXERIF) ) {
                    enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_TXRST);
                    enc28j60WriteOp(ENC28J60_BIT_FIELD_CLR, ECON1, ECON1_TXRST);
            }
    }

    // Set the write pointer to start of transmit buffer area
    enc28j60WriteWord(EWRPTL, TXSTART_INIT);
	// Set the TXND pointer to correspond to the packet size given
	enc28j60WriteWord(ETXNDL, (TXSTART_INIT+len));
	// write per-packet control byte (0x00 means use macon3 settings)
	enc28j60WriteOp(ENC28J60_WRITE_BUF_MEM, 0, 0x00);
	// copy the packet into the transmit buffer
	enc28j60WriteBuffer(len, frame);
	// send the contents of the transmit buffer onto the network
	enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_TXRTS);
    // Reset the transmit logic problem. See Rev. B4 Silicon Errata point 12.
}

void ENC28J60::enc28j60Init(uint8_t* macaddr) {
	enableChip(); // ss=0

	// perform system reset
	enc28j60WriteOp(ENC28J60_SOFT_RESET, 0, ENC28J60_SOFT_RESET);
	delay(50);
	// check CLKRDY bit to see if reset is complete
        // The CLKRDY does not work. See Rev. B4 Silicon Errata point. Just wait.
	//while(!(enc28j60Read(ESTAT) & ESTAT_CLKRDY));
	// do bank 0 stuff
	// initialize receive buffer
	// 16-bit transfers, must write low byte first
	// set receive buffer start address
	gNextPacketPtr = RXSTART_INIT;
	// Rx start
	enc28j60WriteWord(ERXSTL, RXSTART_INIT);
	// set receive pointer address
	enc28j60WriteWord(ERXRDPTL, RXSTART_INIT);
	// RX end
	enc28j60WriteWord(ERXNDL, RXSTOP_INIT);
	// TX start
	enc28j60WriteWord(ETXSTL, TXSTART_INIT);
	// TX end
	enc28j60WriteWord(ETXNDL, TXSTOP_INIT);
	// do bank 1 stuff, packet filter:
        // For broadcast packets we allow only ARP packtets
        // All other packets should be unicast only for our mac (MAADR)
        //
        // The pattern to match on is therefore
        // Type     ETH.DST
        // ARP      BROADCAST
        // 06 08 -- ff ff ff ff ff ff -> ip checksum for theses bytes=f7f9
        // in binary these poitions are:11 0000 0011 1111
        // This is hex 303F->EPMM0=0x3f,EPMM1=0x30

	//enc28j60Write(ERXFCON, ERXFCON_UCEN|ERXFCON_CRCEN|ERXFCON_PMEN);
        //Change to add ERXFCON_BCEN recommended by epam
	//enc28j60Write(ERXFCON, ERXFCON_UCEN|ERXFCON_CRCEN|ERXFCON_PMEN|ERXFCON_BCEN);
        erxfcon =  ERXFCON_UCEN|ERXFCON_CRCEN|ERXFCON_PMEN|ERXFCON_BCEN;
	enc28j60Write(ERXFCON, erxfcon );
	enc28j60WriteWord(EPMM0, 0x303f);
	enc28j60WriteWord(EPMCSL, 0xf7f9);
        //
	// do bank 2 stuff
	// enable MAC receive
	enc28j60Write(MACON1, MACON1_MARXEN|MACON1_TXPAUS|MACON1_RXPAUS);
	// bring MAC out of reset
	enc28j60Write(MACON2, 0x00);
	// enable automatic padding to 60bytes and CRC operations
	enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, MACON3, MACON3_PADCFG0|MACON3_TXCRCEN|MACON3_FRMLNEN);  //|MACON3_FULDPX);
	// set inter-frame gap (non-back-to-back)
	enc28j60WriteWord(MAIPGL, 0x0C12);
	// set inter-frame gap (back-to-back)
	enc28j60Write(MABBIPG, 0x12);
	// Set the maximum packet size which the controller will accept
        // Do not send packets longer than MAX_FRAMELEN:
	enc28j60WriteWord(MAMXFLL, MAX_FRAMELEN);
	// do bank 3 stuff
	// write MAC address
	// NOTE: MAC address in ENC28J60 is byte-backward
	enc28j60Write(MAADR5, macaddr[0]);
	enc28j60Write(MAADR4, macaddr[1]);
	enc28j60Write(MAADR3, macaddr[2]);
	enc28j60Write(MAADR2, macaddr[3]);
	enc28j60Write(MAADR1, macaddr[4]);
	enc28j60Write(MAADR0, macaddr[5]);
	// no loopback of transmitted frames
	enc28j60PhyWrite(PHCON2, PHCON2_HDLDIS);
	// switch to bank 0
	enc28j60SetBank(ECON1);
	// enable interrutps
	enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, EIE, EIE_INTIE|EIE_PKTIE);
	// enable packet reception
	enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_RXEN);
}

void ENC28J60::enableChip()
{
	spi->setChipEnable(true);
}

void ENC28J60::disableChip()
{
	spi->setChipEnable(false);
}

#if (HW_VERSION == HW_2_0_0 || HW_VERSION >= HW_3_5_x)
#elif (HW_VERSION >= HW_3_0_0)
void ENC28J60::resetChip(bool state)
{
	if (state)	GPIO_ResetBits(ETH_RESET_PORT, ETH_RESET_PIN);
	else		GPIO_SetBits(ETH_RESET_PORT, ETH_RESET_PIN);
}
#else
#error "HW_VERSION must be defined in project settings"
#endif

void ENC28J60::delay(int ms)
{
	SystemTimer::get()->delay_ms(ms);
}

uint8_t ENC28J60::ENC28J60_SendByte(uint8_t tx)
{
	return spi->receiveData(tx);
}


uint8_t ENC28J60::enc28j60ReadOp(uint8_t op, uint8_t address)
{
	uint8_t temp;
	enableChip();
    // issue read command
    ENC28J60_SendByte(op | (address & ADDR_MASK));
    temp = ENC28J60_SendByte(0xFF);
    if (address & 0x80)
        temp = ENC28J60_SendByte(0xFF);

    // release CS
    disableChip();
    return temp;
}

void ENC28J60::enc28j60WriteOp(uint8_t op, uint8_t address, uint8_t data)
{
    enableChip();
    ENC28J60_SendByte(op | (address & ADDR_MASK));
    ENC28J60_SendByte(data);
    disableChip();
}

void ENC28J60::enc28j60ReadBuffer(uint16_t len, uint8_t* data)
{
    enableChip();
    ENC28J60_SendByte(ENC28J60_READ_BUF_MEM);
    while (len--) {
        *data++ = ENC28J60_SendByte(0x00);
    }
    disableChip();
}

void ENC28J60::enc28j60WriteBuffer(uint16_t len, uint8_t* data)
{
    enableChip();
    ENC28J60_SendByte(ENC28J60_WRITE_BUF_MEM);
    while (len--)
        ENC28J60_SendByte(*data++);

    disableChip();
}

uint16_t ENC28J60::enc28j60ReadBufferWord()
{
    uint16_t result;
    enc28j60ReadBuffer(2, (uint8_t*) &result);
    return result;
}

void ENC28J60::enc28j60SetBank(uint8_t address)
{
    if ((address & BANK_MASK) != bank) {
        enc28j60WriteOp(ENC28J60_BIT_FIELD_CLR, ECON1, ECON1_BSEL1|ECON1_BSEL0);
        bank = address & BANK_MASK;
        enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, bank>>5);
    }
}

uint8_t ENC28J60::enc28j60Read(uint8_t address)
{
    // set the bank
    enc28j60SetBank(address);
    // do the read
    return enc28j60ReadOp(ENC28J60_READ_CTRL_REG, address);
}

void ENC28J60::enc28j60WriteWord(uint8_t address, uint16_t data)
{
    enc28j60Write(address, data & 0xff);
    enc28j60Write(address + 1, data >> 8);
}

// read upper 8 bits
uint16_t ENC28J60::enc28j60PhyRead(uint8_t address)
{
	// Set the right address and start the register read operation
	enc28j60Write(MIREGADR, address);
	enc28j60Write(MICMD, MICMD_MIIRD);
	delay(15);

	// wait until the PHY read completes
	while(enc28j60Read(MISTAT) & MISTAT_BUSY);

	// reset reading bit
	enc28j60Write(MICMD, 0x00);

	// get data value
	uint16_t data  = enc28j60Read(MIRDL);
	data |= (enc28j60Read(MIRDH) << 8);

	return data;
}

void ENC28J60::enc28j60Write(uint8_t address, uint8_t data)
{
    // set the bank
    enc28j60SetBank(address);
    // do the write
    enc28j60WriteOp(ENC28J60_WRITE_CTRL_REG, address, data);
}

void ENC28J60::enc28j60PhyWrite(uint8_t address, uint16_t data)
{
    // set the PHY register address
    enc28j60Write(MIREGADR, address);
    // write the PHY data
    enc28j60Write(MIWRL, data);
    enc28j60Write(MIWRH, data>>8);
    // wait until the PHY write completes
    while(enc28j60Read(MISTAT) & MISTAT_BUSY) {
            delay(15);
    }
}

void ENC28J60::enc28j60clkout(uint8_t clk)
{
    //setup clkout: 2 is 12.5MHz:
	enc28j60Write(ECOCON, clk & 0x7);
}

uint8_t ENC28J60::enc28j60hasRxPkt(void)
{
	return enc28j60Read(EPKTCNT) > 0;
}

uint8_t ENC28J60::enc28j60getrev(void)
{
	return(enc28j60Read(EREVID));
}

uint8_t ENC28J60::enc28j60linkup(void)
{
    // bit 10 (= bit 3 in upper reg)
	return(enc28j60PhyRead(PHSTAT2) && 4);
}

// A number of utility functions to enable/disable broadcast and multicast bits
void ENC28J60::enc28j60EnableBroadcast( void )
{
	erxfcon |= ERXFCON_BCEN;
	enc28j60Write(ERXFCON, erxfcon);
}

void ENC28J60::enc28j60DisableBroadcast( void )
{
	erxfcon &= (0xff ^ ERXFCON_BCEN);
	enc28j60Write(ERXFCON, erxfcon);
}

void ENC28J60::enc28j60EnableMulticast( void )
{
	erxfcon |= ERXFCON_MCEN;
	enc28j60Write(ERXFCON, erxfcon);
}

void ENC28J60::enc28j60DisableMulticast( void )
{
	erxfcon &= (0xff ^ ERXFCON_MCEN);
	enc28j60Write(ERXFCON, erxfcon);
}

bool ENC28J60::checkAnswer()
{
	return (enc28j60PhyRead(PHLCON) == PHLCON_VALUE);
}

