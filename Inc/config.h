/*
 * config.h
 *
 *  Created on: Oct 14, 2025
 *      Author: Alex
 */

#ifndef _CFG_H_
#define _CFG_H_

#include "eeprom.h"

class CONFIG : public EEPROM {
	public:
		CONFIG(void) : EEPROM()							{ }
		bool		init(void);
		uint16_t	lcdBrightness(void)					{ return (uint16_t)a_cfg.brightness << 3;	}
		bool		isCelsius(void)						{ return a_cfg.bit_params & p_celsius;		}
		bool		isEncoderClockWise(void)			{ return a_cfg.bit_params & p_enc_cw;		}
		bool		isBigTemsStep(void)					{ return a_cfg.bit_params & p_step_5;		}
		bool		isFastChill(void)					{ return a_cfg.bit_params & p_fast_chill;	}
		bool		isBuzzerEnabled(void)				{ return a_cfg.bit_params & p_buzz;			}
		bool		isFan24Volts(void)					{ return a_cfg.bit_params & p_fan_24v;		}
		bool		isAutoCheck(void)					{ return a_cfg.bit_params & p_autocheck;	}
		uint8_t		fanSpeedPcnt(void)					{ return a_cfg.fan_speed;					}
		uint16_t	presetTemp(void);
		uint16_t	fanSpeed(void);
		void		saveTemp(uint16_t t);
		void		saveFanSpeed(uint8_t s_pcnt);
		uint16_t	minFanSpeed(void);
		uint16_t	maxFanSpeed(void);
		uint16_t 	tempToHuman(uint16_t temp);
		uint16_t 	humanToTemp(uint16_t t);
		tDevice		device(void);
		void		setDevice(tDevice dev);
		uint16_t	tempMin(bool force_celsius = false);
		uint16_t	tempMax(bool force_celsius = false);
		void		getCalibtarion(uint16_t data[4]);
		bool 		setCalibtarion(uint16_t data[4], bool save);
		bool		isCalibrationValid(uint16_t data[4]);
		PIDparam 	pidParams(tDevice dev);
		void		setup(uint16_t bright, bool is_celsius, bool is_buzzer, bool is_enc_cw, bool is_big_step, bool is_fast_chill, bool is_fan_24v, bool auto_check);
		bool		save(void);
		bool		savePID(PIDparam &pp);
		uint16_t 	referenceTemp(uint8_t index);
	private:
		void 		defaultConfig(void);
		void		defaultCalibration(void);
		uint16_t 	tempCelsius(uint16_t temp);
		RECORD		a_cfg					= {0};		// The Active configuration record
		RECORD		s_cfg					= {0};		// The Loaded configuration record
		CALIBRATION	calib					= {0};		// The loaded calibration data
		const uint16_t	temp_ref_tweez[4]	= { 200, 260, 330, 400};
		const uint16_t	temp_ref_gun[4]		= { 200, 300, 400, 500};
		const uint16_t	min_temp_diff		= 100;		// Minimal temperature difference between nearest reference points
        const uint16_t	fan_12_v[2]			= {70,  650};
        const uint16_t	fan_24_v[2]			= {200, 1999};
};

#endif
