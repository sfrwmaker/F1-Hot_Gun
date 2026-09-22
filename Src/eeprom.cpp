/*
 * eeprom.cpp
 *
 *  Created on: 2025 JUN 26
 *      Author: Alex
 */

#include <string.h>
#include "eeprom.h"

// Select minimal data size of 2^N bytes to fit the RECORD data
EEPROM::EEPROM() {
	uint16_t c_size = sizeof(CALIBRATION) << 1;					// Reserve space for two copies of calibration data
	uint16_t size = sizeof(RECORD);
	eeprom_chunk_size = 1;
	for (uint8_t i = 0; i < 12; ++i) {							// Calculate the configuration record chunk size (2^N)
		if (size <= eeprom_chunk_size)
			break;
		eeprom_chunk_size <<= 1;
	}
	uint16_t chunks_per_calib = 1;								// How many configurations chunks required for calibration data
	if (eeprom_chunk_size <= c_size)
		chunks_per_calib = (c_size + eeprom_chunk_size - 1) / eeprom_chunk_size;
	first_chunk = chunks_per_calib;
	eeprom_chunks = eeprom_size / eeprom_chunk_size - first_chunk;
}

bool EEPROM::init(void) {
	// Read all the records in the EEPROM and find min and max record IDs
	uint32_t 	min_rec_ID 	= 0xffffffff;
	uint16_t 	min_rec_ch 	= 0;
	uint32_t 	max_rec_ID 	= 0;
	uint16_t 	max_rec_ch 	= 0;
	uint16_t 	records 	= 0;

	if (HAL_OK != HAL_I2C_IsDeviceReady(&I2C_PORT, eeprom_address<<1, 2, 100)) {
		can_write = false;
		return can_write;
	}

	can_write = true;
	uint8_t data[eeprom_chunk_size];
	for (uint16_t chunk = first_chunk; chunk < eeprom_chunks; ++chunk) { // The calibration data (2 copies) are in the beginning of EEPROM
		if (readChunk(data, chunk)) {
			RECORD *cfg = (RECORD *)data;
			if (checkSum(cfg, false)) {
				++records;
				if (min_rec_ID 	> cfg->ID) {
					min_rec_ID 	= cfg->ID;
					min_rec_ch	= chunk;
				}
				if (max_rec_ID < cfg->ID) {
					max_rec_ID 	= cfg->ID;
					max_rec_ch 	= chunk;
				}
			} else {
				break;
			}
		} else {
			can_write	= false;
			break;
		}
	}

	if (records == 0) {
		w_chunk		= r_chunk = first_chunk;
	    return can_write;
	}

	r_chunk = max_rec_ch;
	if (records < eeprom_chunks) {								// The EEPROM is not full
	    w_chunk = r_chunk + 1;
	    if (w_chunk >= eeprom_chunks) w_chunk = first_chunk;
	} else {
		w_chunk = min_rec_ch;
	}
	return can_write;
}

bool EEPROM::loadRecord(RECORD* config_record) {
	if (readChunk((uint8_t *)config_record, r_chunk)) {
		return checkSum(config_record, false);
	}
	return false;
}

bool EEPROM::saveRecord(RECORD* config_record) {
	if (!can_write)
		return false;

	config_record->ID ++;
	checkSum(config_record, true);
	uint8_t data[eeprom_chunk_size];
	memcpy(data, (uint8_t *)config_record, sizeof(RECORD));
	if (writeChunk(data, w_chunk)) {
		r_chunk = w_chunk;
		if (++w_chunk >= eeprom_chunks) w_chunk = first_chunk;
		return true;
	}
	return false;
}

bool EEPROM::loadCalibrationData(CALIBRATION *calib) {
	int8_t index = calibDataIndex();
	if (index >= 0) {
		readCalibration((uint8_t *)calib, index);
		return true;
	}
	return false;
}

bool EEPROM::saveCalibrationData(CALIBRATION *calib) {
	int16_t index = calibDataIndex();						// -1, 0, 1
	if (index == 1 || index == -1) index = 0; else index = 1;
	calib->ID ++;
	checkSumCalibration(calib, true);
	if (writeCalibration((uint8_t *)calib, index)) {
		return true;
	}
	return false;
}

// Clear whole EEPROM
void EEPROM::erase(void) {
	uint8_t data[eeprom_chunk_size];
	for (uint8_t i = 0; i < eeprom_chunk_size; ++i)
		data[i] = 0xFF;
	for (uint16_t i = 0; i < eeprom_chunks; ++i) {
		uint32_t addr = i * eeprom_chunk_size;
		if (HAL_OK != HAL_I2C_Mem_Write(&I2C_PORT, eeprom_address<<1, addr, I2C_MEMADD_SIZE_16BIT, data, eeprom_chunk_size, 100)) {
			break;											// Stop writing immediately in case of error
		}
		HAL_Delay(10);										// Let the data to be saved in the EEPROM
	}
	init();
}

// Read the EEPROM whole chunk
bool EEPROM::readChunk(uint8_t *data, uint16_t chunk_index) {
	if (chunk_index >= eeprom_chunks) return false;

	uint16_t addr = chunk_index * eeprom_chunk_size;
	if (HAL_OK == HAL_I2C_Mem_Read(&I2C_PORT, eeprom_address<<1, addr, I2C_MEMADD_SIZE_16BIT, data, eeprom_chunk_size, 100)) {
		return true;
	}
	return false;
}

// Write the EEPROM whole chunk
bool EEPROM::writeChunk(uint8_t *data, uint16_t chunk_index) {
	if (chunk_index >= eeprom_chunks) return false;

	uint16_t addr = chunk_index * eeprom_chunk_size;
	if (HAL_OK == HAL_I2C_Mem_Write(&I2C_PORT, eeprom_address<<1, addr, I2C_MEMADD_SIZE_16BIT, data, eeprom_chunk_size, 100)) {
		HAL_Delay(20);
		return true;
	}
	HAL_Delay(20);
	return false;
}

// Checks the CRC of the RECORD structure. Returns true if OK. Replace the CRC with the correct value if update is true
bool EEPROM::checkSum(RECORD* cfg, bool update) {
	uint16_t 	summ 		= 117;							// To avoid good check sum with all-zero, start with 117
	uint16_t    rec_summ 	= cfg->crc;
	cfg->crc				= 0;
	uint8_t*	d 			= (uint8_t*)cfg;
	for (uint8_t i = 0; i < sizeof(RECORD); ++i) {
		summ <<= 1; summ += d[i];
	}
	bool res = (rec_summ == summ);
	if (update) cfg->crc = summ;
	return res;
}

// Read the Calibration chunk
bool EEPROM::readCalibration(uint8_t *data, uint8_t index) {
	index &= 1;												// Two copies only
	uint16_t addr = (index > 0)?sizeof(CALIBRATION):0;
	if (HAL_OK == HAL_I2C_Mem_Read(&I2C_PORT, eeprom_address<<1, addr, I2C_MEMADD_SIZE_16BIT, data, sizeof(CALIBRATION), 100)) {
		return true;
	}
	return false;
}

// Write the Calibration chunk
bool EEPROM::writeCalibration(uint8_t *data, uint8_t index) {
	index &= 1;												// Two copies only
	uint16_t addr = (index > 0)?sizeof(CALIBRATION):0;
	if (HAL_OK == HAL_I2C_Mem_Write(&I2C_PORT, eeprom_address<<1, addr, I2C_MEMADD_SIZE_16BIT, data, sizeof(CALIBRATION), 100)) {
		HAL_Delay(20);
		return true;
	}
	HAL_Delay(20);
	return false;
}

// Checks the CRC of the CALIBRATION structure. Returns true if OK. Replace the CRC with the correct value if update is true
bool EEPROM::checkSumCalibration(CALIBRATION* cfg, bool update) {
	uint16_t 	summ 		= 113;							// To avoid good check sum with all-zero, start with 117
	uint16_t    rec_summ 	= cfg->crc;
	cfg->crc				= 0;
	uint8_t*	d 			= (uint8_t*)cfg;
	for (uint8_t i = 0; i < sizeof(CALIBRATION); ++i) {
		summ <<= 1; summ += d[i];
	}
	bool res = (rec_summ == summ);
	if (update) cfg->crc = summ;
	return res;
}

int8_t EEPROM::calibDataIndex(void) {
	uint8_t data[sizeof(CALIBRATION)];
	uint32_t max_id		= 0;
	uint8_t  chunk_id	= 0;
	for (uint8_t i = 0; i < 2; ++i) {
		if (readCalibration(data, i)) {
			if (checkSumCalibration((CALIBRATION *)data, false)) {
				CALIBRATION *pC = (CALIBRATION *)&data;
				if (pC->ID > max_id) {
					max_id = pC->ID;
					chunk_id = i;
				}
			}
		}
	}
	if (max_id)
		return chunk_id;
	return -1;
}
