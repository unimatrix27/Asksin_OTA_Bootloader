#include "cc.h"

const PROGMEM const uint8_t initVal[] = {									// define init settings for TRX868
	0x00, 0x2E,			// IOCFG2: tristate									// non inverted GDO2, high impedance tri state
	0x01, 0x2E,			// IOCFG1: tristate									// low output drive strength, non inverted GD=1, high impedance tri state
	0x02, 0x06,			// IOCFG0: packet CRC ok							// disable temperature sensor, non inverted GDO0, asserts when a sync word has been sent/received, and de-asserts at the end of the packet. in RX, the pin will also de-assert when a package is discarded due to address or maximum length filtering
	0x03, 0x0D,			// FIFOTHR: TX:9 / RX:56							// 0 ADC retention, 0 close in RX, TX FIFO = 9 / RX FIFO = 56 byte
	0x04, 0xE9,			// SYNC1											// Sync word
	0x05, 0xCA,			// SYNC0
	0x06, 0x3D,			// PKTLEN(x): 61									// packet length 61
	0x07, 0x0C,			// PKTCTRL1:										// PQT = 0, CRC auto flush = 1, append status = 1, no address check
	0x0B, 0x06,			// FSCTRL1:											// frequency synthesizer control

	// 868.299866 MHz
//	0x0D, 0x21,			// FREQ2
//	0x0E, 0x65,			// FREQ1
//	0x0F, 0x6A,			// FREQ0

	// 868.2895508  (better wireles connection)
	0x0D, 0x21,			// FREQ2
	0x0E, 0x65,			// FREQ1
	0x0F, 0x50,			// FREQ0

	0x10, 0xC8,			// MDMCFG4
	0x11, 0x93,			// MDMCFG3
	0x12, 0x03,			// MDMCFG2
	0x15, 0x34,			// DEVIATN
	0x16, 0x01,			// MCSM2
	0x17, 0x30,			// MCSM1: always go into IDLE
	0x18, 0x18,			// MCSM0
	0x19, 0x16,			// FOCCFG
	0x1B, 0x43,			// AGCTRL2
	//0x1E, 0x28,		// ..WOREVT1: tEVENT0 = 50 ms, RX timeout = 390 us
	//0x1F, 0xA0,		// ..WOREVT0:
	//0x20, 0xFB,		// ..WORCTRL: EVENT1 = 3, WOR_RES = 0
	0x21, 0x56,			// FREND1
	0x25, 0x00,
	0x26, 0x11,			// FSCAL0
	0x2D, 0x35,			// TEST1
	0x3E, 0xC3,			// ?
};

const uint8_t PROGMEM initValUpdate[] = {
	0x0B, 0x08,
	0x10, 0x5B,
	0x11, 0xF8,
	0x15, 0x47,
	0x19, 0x1D,
	0x1A, 0x1C,
	0x1B, 0xC7,
	0x1C, 0x00,
	0x1D, 0xB2,
	0x21, 0xB6,
	0x23, 0xEA,
};

/*
 * initialize CC1101
 */
void cc1101Init(uint8_t mode100k) {
	cli();

	bitSet(DDR_SPI, PIN_SPI_SS);												// set B2(SS) as Output
	bitSet(DDR_SPI, PIN_SPI_MOSI);												// set B3(MOSI) as Output
	bitClear(DDR_SPI, PIN_SPI_MISO);											// set B4(MISO) as Input
	bitSet(DDR_SPI, PIN_SPI_SCK);												// set B5(SCK) as Output

	bitClear(DDR_GDO0, PIN_GDO0);												// set B2(SS) as Input

	bitSet(PORT_SPI, PIN_SPI_SS);												// set SS high
	bitSet(PORT_SPI, PIN_SPI_SCK);												// set SCK high
	bitClear(PORT_SPI, PIN_SPI_MOSI);											// set MOSI high

	SPCR = _BV(SPE) | _BV(MSTR);												// SPI speed = CLK/4

	cc1101_Deselect();															// some deselect and selects to init the TRX868modul
	_delay_us(30);

	cc1101_Select();	
	_delay_us(30);

	cc1101_Deselect();
	_delay_us(45);

	cmdStrobe(CC1101_SRES);														// send reset
	_delay_us(100);

	for (uint8_t i=0; i<sizeof(initVal); i += 2) {								// write init value to TRX868
		writeReg(pgm_read_byte(&initVal[i]), pgm_read_byte(&initVal[i+1]));	
	}

	if (mode100k) {																// switch to 100k mode
		for (uint8_t i=0; i<sizeof(initValUpdate); i += 2) {					// write init value to TRX868
			writeReg(pgm_read_byte(&initValUpdate[i]), pgm_read_byte(&initValUpdate[i+1]));	
		}
	}

	cmdStrobe(CC1101_SCAL);														// calibrate frequency synthesizer and turn it off
	_delay_ms(4);

	do {
		cmdStrobe(CC1101_SRX);
	} while (readReg(CC1101_MARcurStatTE, CC1101_STATUS) != 0x0D);
	
	writeReg(CC1101_PATABLE, PA_MaxPower);										// configure PATABLE
	cmdStrobe(CC1101_SRX);														// flush the RX buffer
	cmdStrobe(CC1101_SWORRST);													// reset real time clock

	_delay_ms(3);

	sei();
}

void sendData(uint8_t *buf, uint8_t burst) {									// send data packet via RF

	/**
	 * Going from RX to TX does not work if there was a reception less than 0.5
	 * sec ago. Due to CCA? Using IDLE helps to shorten this period(?)
	 * ccStrobe(CC1100_SIDLE);
	 * uint8_t cnt = 0xff;
	 * while(cnt-- && (ccStrobe( CC1100_STX ) & 0x70) != 2)
	 * my_delay_us(10);
	 */

 	cmdStrobe(CC1101_SIDLE);													// go to idle mode
	cmdStrobe(CC1101_SFRX );													// flush RX buffer
	cmdStrobe(CC1101_SFTX );													// flush TX buffer
	
	if (burst) {																// BURST-bit set?
		cmdStrobe(CC1101_STX  );												// send a burst
		_delay_ms(200);														// according to ELV, devices get activated every 300ms, so send burst for 360ms
		_delay_ms(160);
	} else {
		_delay_ms(1);															// wait a short time to set TX mode
	}

	writeBurst(CC1101_TXFIFO, buf, buf[0]+1);									// write in TX FIFO

	cmdStrobe(CC1101_SFRX);														// flush the RX buffer
	cmdStrobe(CC1101_STX);														// send a burst

	for(uint8_t i=0; i< 200;++i) {												// after sending out all bytes the chip should go automatically in RX mode
		if( readReg(CC1101_MARcurStatTE, CC1101_STATUS) == MARcurStatTE_RX)
			break;																//now in RX mode, good
		if( readReg(CC1101_MARcurStatTE, CC1101_STATUS) != MARcurStatTE_TX) {
			break;																//neither in RX nor TX, probably some error
		}

		_delay_us(10);
	}
}

uint8_t receiveData(uint8_t *buf) {												// read data packet from RX FIFO
	uint8_t rxBytes = readReg(CC1101_RXBYTES, CC1101_STATUS);					// how many bytes are in the buffer

	if ((rxBytes & 0x7F) && !(rxBytes & 0x80)) {								// any byte waiting to be read and no overflow?
		buf[0] = readReg(CC1101_RXFIFO, CC1101_CONFIG);							// read data length
		
		if (buf[0] > CC1101_DATA_LEN)											// if packet is too long
			buf[0] = 0;															// discard packet
		else {
			readBurst(&buf[1], CC1101_RXFIFO, buf[0]);							// read data packet
			readReg(CC1101_RXFIFO, CC1101_CONFIG);								// read RSSI
			
			uint8_t val = readReg(CC1101_RXFIFO, CC1101_CONFIG);				// read LQI and CRC_OK
			trx868.lqi = val & 0x7F;
			trx868.crc_ok = bitRead(val, 7);
		}
	} else buf[0] = 0;															// nothing to do, or overflow

	cmdStrobe(CC1101_SFRX);														// flush Rx FIFO
	cmdStrobe(CC1101_SIDLE);													// enter IDLE state
	cmdStrobe(CC1101_SRX);														// back to RX state
	cmdStrobe(CC1101_SWORRST);													// reset real time clock
//	trx868.rfState = RFSTATE_RX;												// declare to be in Rx state
	
	return buf[0];																// return the data buffer
}

uint8_t detectBurst(void) {														// wake up CC1101 from power down state
	/*
	 * 10 7/10 5 in front of the received string; 33 after received string
	 * 10 - 00001010 - sync word found
	 * 7  - 00000111 - GDO0 = 1, GDO2 = 1
	 * 5  - 00000101 - GDO0 = 1, GDO2 = 1
	 * 33 - 00100001 - GDO0 = 1, preamble quality reached
	 * 96 - 01100000 - burst sent
	 * 48 - 00110000 - in receive mode
	 *
	 * Status byte table:
	 *  0 current GDO0 value
	 *  1 reserved
	 *  2 GDO2
	 *  3 sync word found
	 *  4 channel is clear
	 *  5 preamble quality reached
	 *  6 carrier sense
	 *  7 CRC ok
	 *
	 * possible solution for finding a burst is to check for bit 6, carrier sense

	 * set RXTX module in receive mode
	 */
	cc1101_Select();															// select CC1101
	wait_Miso();																// wait until MISO goes low
	cc1101_Deselect();															// deselect CC1101
	cmdStrobe(CC1101_SRX);														// set RX mode again
	_delay_ms(3);																// wait a short time to set RX mode
	
	// todo: check carrier sense for 5ms to avoid wakeup due to normal transmition
	return bitRead(monitorStatus(),6);											// return the detected signal
}

void setPowerDownxtStatte() {													// put CC1101 into power-down state
	cmdStrobe(CC1101_SIDLE);													// coming from RX state, we need to enter the IDLE state first
	cmdStrobe(CC1101_SFRX);
	cmdStrobe(CC1101_SPWD);														// enter power down state
	//Serial << "pd\n";
}

uint8_t monitorStatus() {
	return readReg(CC1101_PKTSTATUS, CC1101_STATUS);
}

uint8_t sendSPI(uint8_t val) {													// send byte via SPI
	SPDR = val;																	// transfer byte via SPI
	while(!(SPSR & _BV(SPIF)));													// wait until SPI operation is terminated
	return SPDR;
}

void cmdStrobe(uint8_t cmd) {													// send command strobe to the CC1101 IC via SPI
	cc1101_Select();															// select CC1101
	wait_Miso();																// wait until MISO goes low
	sendSPI(cmd);																// send strobe command
	cc1101_Deselect();															// deselect CC1101
}

void writeBurst(uint8_t regAddr, uint8_t *buf, uint8_t len) {					// write multiple registers into the CC1101 IC via SPI
	cc1101_Select();															// select CC1101
	wait_Miso();																// wait until MISO goes low
	sendSPI(regAddr | WRITE_BURST);												// send register address
	for(uint8_t i=0 ; i<len ; i++) sendSPI(buf[i]);								// send value
	cc1101_Deselect();															// deselect CC1101
}

void readBurst(uint8_t *buf, uint8_t regAddr, uint8_t len) {					// read burst data from CC1101 via SPI
	cc1101_Select();															// select CC1101
	wait_Miso();																// wait until MISO goes low
	sendSPI(regAddr | READ_BURST);												// send register address
	for(uint8_t i=0 ; i<len ; i++) buf[i] = sendSPI(0x00);						// read result byte by byte
	cc1101_Deselect();															// deselect CC1101
}

uint8_t readReg(uint8_t regAddr, uint8_t regType) {								// read CC1101 register via SPI
	cc1101_Select();															// select CC1101
	wait_Miso();																// wait until MISO goes low
	sendSPI(regAddr | regType);													// send register address
	uint8_t val = sendSPI(0x00);												// read result
	cc1101_Deselect();															// deselect CC1101
	return val;
}

void writeReg(uint8_t regAddr, uint8_t val) {									// write single register into the CC1101 IC via SPI
	cc1101_Select();															// select CC1101
	wait_Miso();																// wait until MISO goes low
	sendSPI(regAddr);															// send register address
	sendSPI(val);																// send value
	cc1101_Deselect();															// deselect CC1101
}

