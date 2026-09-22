/*
 * eeprom.h
 *
 *  Created on: 2025 JUN 26
 *      Author: Alex
 *
 */

#ifndef EEPROM_H_
#define EEPROM_H_
#include "main.h"
#include "cfgtypes.h"
#include "pid.h"

#define I2C_PORT			hi2c1
extern I2C_HandleTypeDef 	I2C_PORT;

class EEPROM {
	public:
		EEPROM(void);
		bool		init();
		bool 		loadRecord(RECORD* config_record);
		bool		saveRecord(RECORD* config_record);
		bool		loadCalibrationData(CALIBRATION *calib);
		bool		saveCalibrationData(CALIBRATION *calib);
		void 		erase(void);
	private:
		bool 		readChunk(uint8_t *data, uint16_t chunk_index);
		bool 		writeChunk(uint8_t *data, uint16_t chunk_index);
		bool 		readCalibration(uint8_t *data, uint8_t index);
		bool 		writeCalibration(uint8_t *data, uint8_t index);
		bool 		checkSum(RECORD* cfg, bool update);
		bool 		checkSumCalibration(CALIBRATION* cfg, bool update);
		int8_t 		calibDataIndex(void);
		bool		can_write				= false;	// The flag indicates that data can be saved to the EEPROM
		uint16_t	r_chunk					= 0;		// Chunk number of the correct record in EEPROM to be read
		uint16_t	w_chunk					= 0;		// Chunk number in the EEPROM to start write new record
		uint16_t	first_chunk;						// The first configuration chunk index. Calculated in the constructor.
		uint16_t	eeprom_chunk_size;					// Calculated in the constructor. The size depends on size of the RECORD
		uint16_t	eeprom_chunks;						// The number of configuration chunks in my EEPROM IC (depends on eeprom_chunk_size)
		const uint16_t  eeprom_address 		= 0x50;		// AT24C32 EEPROM IC address on the I2C bus
		const uint16_t	eeprom_size			= 4096;
};

#endif
