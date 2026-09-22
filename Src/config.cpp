/*
 * config.cpp
 *
 *  Created on: Oct 14, 2025
 *      Author: Alex
 */

#include <string.h>
#include "config.h"
#include "tools.h"
#include "vars.h"

bool CONFIG::init(void) {
	bool ret = EEPROM::init();
	if (ret) {
		if (loadRecord(&a_cfg)) {
			if (!loadCalibrationData(&calib)) {
				defaultCalibration();
			}
			return true;
		}
	}
	defaultConfig();
	defaultCalibration();
	return ret;
}

void CONFIG::saveFanSpeed(uint8_t s_pcnt) {
	if (s_pcnt > 100) s_pcnt = 100;
	a_cfg.fan_speed = s_pcnt;
}

uint16_t CONFIG::minFanSpeed(void) {
	if (a_cfg.bit_params & p_fan_24v)
		return fan_24_v[0];
	return fan_12_v[0];
}

uint16_t CONFIG::maxFanSpeed(void) {
	if (a_cfg.bit_params & p_fan_24v)
		return fan_24_v[1];
	return fan_12_v[1];
}

uint16_t CONFIG::presetTemp(void) {
	if (a_cfg.bit_params & p_gun)
		return a_cfg.gun_temp;
	return a_cfg.twez_temp;
}

uint16_t CONFIG::fanSpeed(void) {
	const uint16_t *fan_limit = (a_cfg.bit_params & p_fan_24v)?fan_24_v:fan_12_v;
	return map(a_cfg.fan_speed, 0, 100, fan_limit[0], fan_limit[1]);
}

void CONFIG::saveTemp(uint16_t t) {
	if (a_cfg.bit_params & p_gun) {
		uint16_t t_min = gun_temp_minC;
		uint16_t t_max = gun_temp_maxC;
		if (!isCelsius()) {
			t_min = celsiusToFahrenheit(t_min);
			t_max = celsiusToFahrenheit(t_max);
		}
		t = constrain(t, t_min, t_max);
		a_cfg.gun_temp  = t;
	} else {
		uint16_t t_min = tweez_temp_minC;
		uint16_t t_max = tweez_temp_maxC;
		if (!isCelsius()) {
			t_min = celsiusToFahrenheit(t_min);
			t_max = celsiusToFahrenheit(t_max);
		}
		t = constrain(t, t_min, t_max);
		a_cfg.twez_temp = t;
	}
}

/*
 * Translate the internal temperature of the TWEEZERS or Hot Air Gun to the human readable units (Celsius or Fahrenheit)
 * Parameters:
 * temp 		- Device temperature in internal units
 * dev			- Device: Tweezers or Hot Gun
 */
uint16_t CONFIG::tempToHuman(uint16_t temp) {
	uint16_t tempH = tempCelsius(temp);
	if (!isCelsius())
		tempH = celsiusToFahrenheit(tempH);
	return tempH;
}

// Translate the temperature from human readable units (Celsius or Fahrenheit) to the internal units
uint16_t CONFIG::humanToTemp(uint16_t t) {
	uint16_t t200	= referenceTemp(0);
	uint16_t t400	= referenceTemp(3);
	uint16_t tmin	= tempMin(true);						// The minimal temperature, Celsius
	uint16_t tmax	= tempMax(true);						// The maximal temperature, Celsius
	if (!isCelsius()) {
		t200 = celsiusToFahrenheit(t200);
		t400 = celsiusToFahrenheit(t400);
		tmin = celsiusToFahrenheit(tmin);
		tmax = celsiusToFahrenheit(tmax);
	}
	t = constrain(t, tmin, tmax);
	uint16_t *calibration = (a_cfg.bit_params & p_gun)?calib.gun_calib:calib.tweezers_calib;

	uint16_t left 	= 0;
	uint16_t right 	= int_temp_max;
	uint16_t temp = emap(t, t200, t400, calibration[0], calibration[3]);

	if (temp > (left+right)/ 2) {
		temp -= (right-left) / 4;
	} else {
		temp += (right-left) / 4;
	}

	for (uint8_t i = 0; i < 20; ++i) {
		uint16_t tempH = tempToHuman(temp);
		if (tempH == t) {
			return temp;
		}
		uint16_t new_temp;
		if (tempH < t) {
			left = temp;
			 new_temp = (left+right)/2;
			if (new_temp == temp)
				new_temp = temp + 1;
		} else {
			right = temp;
			new_temp = (left+right)/2;
			if (new_temp == temp)
				new_temp = temp - 1;
		}
		temp = new_temp;
	}
	return temp;
}

tDevice CONFIG::device(void) {
	if (a_cfg.bit_params & p_gun)
		return d_gun;
	return d_tweez;
}

void CONFIG::setDevice(tDevice dev) {
	a_cfg.bit_params &= ~p_gun;
	if (dev == d_gun)
		a_cfg.bit_params |= p_gun;
}

uint16_t CONFIG::tempMin(bool force_celsius) {
	uint16_t t = (a_cfg.bit_params & p_gun)?gun_temp_minC:tweez_temp_minC;
	if (!force_celsius && !isCelsius()) {					// Convert to Fahrenheit
		t = celsiusToFahrenheit(t);
		t -= t % 10;										// Round left to be multiplied by 10
	}
	return t;
}

uint16_t CONFIG::tempMax(bool force_celsius) {
	uint16_t t = (a_cfg.bit_params & p_gun)?gun_temp_maxC:tweez_temp_maxC;
	if (!force_celsius && !isCelsius()) {					// Convert to Fahrenheit
		t = celsiusToFahrenheit(t);
		t -= t % 10;										// Round left to be multiplied by 10
	}
	return t;
}

void CONFIG::getCalibtarion(uint16_t data[]) {
	uint16_t *clb = (a_cfg.bit_params & p_gun)?calib.gun_calib:calib.tweezers_calib;
	for (uint8_t i = 0; i < 4; ++i)
		data[i] = clb[i];
}

bool CONFIG::setCalibtarion(uint16_t data[4], bool save) {
	uint16_t *clb = (a_cfg.bit_params & p_gun)?calib.gun_calib:calib.tweezers_calib;
	for (uint8_t i = 0; i < 4; ++i)
		clb[i] = data[i];
	if (save)
		return saveCalibrationData(&calib);
	return true;
}

bool CONFIG::isCalibrationValid(uint16_t data[4]) {
	for (uint8_t i = 0; i < 3; ++i) {
		if (data[i] >= data[i+1] || (data[i+1] - data[i]) < min_temp_diff) return false;
	}
	return true;
}

// PID parameters: Kp, Ki, Kd
PIDparam CONFIG::pidParams(tDevice dev) {
	if (dev == d_gun) {
		return PIDparam(calib.PID_gun);
	} else {
		return PIDparam(calib.PID_tweez);
	}
}

void CONFIG::setup(uint16_t bright, bool is_celsius, bool is_buzzer,
					bool is_enc_cw, bool is_big_step, bool is_fast_chill, bool is_fan_24v, bool auto_check) {
	a_cfg.brightness = bright>>3;
	uint8_t bp = 0;
	if (is_celsius)			bp |= p_celsius;
	if (is_buzzer)			bp |= p_buzz;
	if (is_enc_cw)			bp |= p_enc_cw;
	if (is_big_step)		bp |= p_step_5;
	if (is_fast_chill)		bp |= p_fast_chill;
	if (is_fan_24v)			bp |= p_fan_24v;
	if (auto_check)			bp |= p_autocheck;
	a_cfg.bit_params = bp;
	if ((a_cfg.bit_params & p_celsius) && (s_cfg.bit_params & p_celsius) == 0) { // Turned Fahrenheight to Celsius
		a_cfg.twez_temp = fahrenheitToCelsius(a_cfg.twez_temp);
		a_cfg.gun_temp  = fahrenheitToCelsius(a_cfg.gun_temp);
	} else if ((a_cfg.bit_params & p_celsius) == 0 && (s_cfg.bit_params & p_celsius)) { // Turned Celsius to Fahrenheight
		a_cfg.twez_temp = celsiusToFahrenheit(a_cfg.twez_temp);
		a_cfg.gun_temp  = celsiusToFahrenheit(a_cfg.gun_temp);
	}
}

void CONFIG::defaultCalibration(void) {
	for (uint8_t i = 0; i < 4; ++i) {
		calib.tweezers_calib[i]	= temp_ref_tweez[i] * 10;
		calib.gun_calib[i]		= temp_ref_gun[i]   * 10;
	}
	calib.PID_tweez[PID_KP]	=  3000;
	calib.PID_tweez[PID_KI]	=   450;
	calib.PID_tweez[PID_KD]	=   300;
	calib.PID_gun[PID_KP]	=   300;
	calib.PID_gun[PID_KI]	=    20;
	calib.PID_gun[PID_KD]	=     0;
	calib.ID				=     0;
}

bool CONFIG::save(void) {
	if (memcmp((uint8_t *)&a_cfg, (uint8_t *)&s_cfg, sizeof(RECORD)) == 0)
		return true;

	if (saveRecord(&a_cfg)) {
		memcpy((uint8_t *)&s_cfg, (uint8_t *)&a_cfg, sizeof(RECORD));
		return true;
	}
	return false;
}

bool CONFIG::savePID(PIDparam &pp) {
	if (a_cfg.bit_params & p_gun) {
		calib.PID_gun[PID_KP]	= pp.Kp;
		calib.PID_gun[PID_KI]	= pp.Ki;
		calib.PID_gun[PID_KD]	= pp.Kd;
	} else {
		calib.PID_tweez[PID_KP]	= pp.Kp;
		calib.PID_tweez[PID_KI]	= pp.Ki;
		calib.PID_tweez[PID_KD]	= pp.Kd;
	}
	return saveCalibrationData(&calib);
}

void CONFIG::defaultConfig(void) {
	a_cfg.ID			= 0;
	a_cfg.brightness	= 125;
	a_cfg.twez_temp		= 240;
	a_cfg.gun_temp		= 210;
	a_cfg.fan_speed		= 10;
	a_cfg.bit_params	= p_celsius | p_enc_cw | p_autocheck;
}

// Translate the internal temperature of the Tweezers or Hot Air Gun to Celsius
uint16_t CONFIG::tempCelsius(uint16_t temp) {
	int16_t tempH 	= 0;
	uint16_t *calibration = (a_cfg.bit_params & p_gun)?calib.gun_calib:calib.tweezers_calib;

	if (temp < calibration[0]) {								// less than first calibration point
	    tempH = map(temp, 25, calibration[0], 25, referenceTemp(0));
	} else {
		if (temp <= calibration[3]) {							// Inside calibration interval
			for (uint8_t j = 1; j < 4; ++j) {
				if (temp < calibration[j]) {
					tempH = map(temp, calibration[j-1], calibration[j],
							referenceTemp(j-1), referenceTemp(j));
					break;
				}
			}
		} else {												// Greater than maximum
			if (calibration[1] < calibration[3]) { 				// If device calibrated correctly
				tempH = emap(temp, calibration[1], calibration[3],
					referenceTemp(1), referenceTemp(3));
			} else {											// Perhaps, the tip calibration process
				tempH = emap(temp, calibration[1], int_temp_max,
							referenceTemp(1), referenceTemp(3));
			}
		}
	}
	tempH = constrain(tempH, 0, 999);
	return tempH;
}

uint16_t CONFIG::referenceTemp(uint8_t index) {
	uint16_t t = 0;
	if (a_cfg.bit_params & p_gun)
		t = temp_ref_gun[index];
	else
		t = temp_ref_tweez[index];
	if (!isCelsius()) {											// Convert to Fahrenheit
		t = celsiusToFahrenheit(t);
		t -= t % 10;											// Round left to be multiplied by 10
	}
	return t;
}
