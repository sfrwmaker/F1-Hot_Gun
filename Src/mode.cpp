/*
 * mode.cpp
 *
 *  2025 OCT 09, v.1.00
 * 		Ported from UNITED controller source code, tailored to the new hardware
 */

#include <stdio.h>
#include <string.h>
#include <cstdlib>
#include <math.h>
#include "main.h"
#include "mode.h"
#include "core.h"
#include "max31855.h"

//---------------------- The Menu mode -------------------------------------------
void MODE::setup(MODE* return_mode, MODE* long_mode) {
	mode_return	= return_mode;
	mode_lpress	= long_mode;
}

MODE* MODE::returnToMain(void) {
	if (mode_return && time_to_return && HAL_GetTick() >= time_to_return)
		return mode_return;
	return this;
}

void MODE::resetTimeout(void) {
	if (timeout_secs) {
		time_to_return = HAL_GetTick() + timeout_secs * 1000;
	}
}
void MODE::setTimeout(uint16_t t) {
	timeout_secs = t;
}

//---------------------- The Main Working mode -----------------------------------
void MWORK::init(void) {
	uint16_t t_min		= pCore->cfg.tempMin();
	uint16_t t_max		= pCore->cfg.tempMax();
	uint16_t t_set		= pCore->cfg.presetTemp();
	uint8_t  step		= pCore->cfg.isBigTemsStep()?5:1;
	uint16_t fan_speed	= pCore->cfg.fanSpeed();
	pCore->enc.reset(t_set, t_min, t_max, step, (step == 1)?5:10, false);
	pUnit				= (pCore->cfg.device() == d_gun)?(UNIT *)&pCore->gun:(UNIT *)&pCore->tweezers;
	t_set = pCore->cfg.humanToTemp(t_set);
	pUnit->setTemp(t_set);
	pCore->gun.setFan(fan_speed);
	pCore->dspl.clear();
	fan_mode_end	= 0;
	use_encoder		= false;
	screen_updated 	= 0;
}

MODE* MWORK::loop(void) {
	RENC*	pEnc	= &pCore->enc;

	// The Auto or reed switch management for Hot Air Gun or Tweezers
	if (pCore->sw.status()) {								// The Reed switch is active
		if (!pUnit->isOn()) {
			pUnit->switchPower(true);
		}
	} else if (!use_encoder) {
		if (pUnit->isOn()) {
			pCore->gun.setColdTemperature(pCore->term.intTemp());
			pUnit->switchPower(false);
			pCore->cfg.save();
		}
	}

	// The encoder manages the tweezers power, the temperature or fan mode of the Hot Air Gun
	uint16_t set = pEnc->read();							// Encoder value. The preset temperature or preset fan speed
	if (pEnc->changed()) {
		if (pCore->cfg.device() == d_gun) {					// The Tool is our Hot Air Gun
			if (fan_mode_end == 0) {						// Edit Hot Air Gun temperature
				uint16_t int_temp = pCore->cfg.humanToTemp(set);
				pCore->gun.setTemp(int_temp);
				pCore->cfg.saveTemp(set);
			} else if (!pCore->gun.isCooling()) {			// Edit the Hot Air Gun fan speed if gun is not cooling
				fan_mode_end = HAL_GetTick() + fan_speed_edit_to;
				pCore->cfg.saveFanSpeed(set);				// 0..100
				uint16_t fan_speed = pCore->cfg.fanSpeed();	// 0..1999
				pCore->gun.setFan(fan_speed);
			}
		} else {											// Managing the Tweezer's power
			if (pUnit->isOn()) {
				uint16_t int_temp = pCore->cfg.humanToTemp(set);
				pCore->tweezers.setTemp(int_temp);
			}
			pCore->cfg.saveTemp(set);
		}
		screen_updated = 0;									// Force to redraw screen
	}

	// Manage the encoder button. Toggle the tweezers power
	uint8_t button = pEnc->buttonStatus();
	if (button == 1) {										// The encoder button was shortly pressed
		if (pCore->cfg.device() == d_gun) {					// Managing the Hot Air Gun
			if (fan_mode_end > 0) {							// Editing the fan speed
				fan_mode_end = HAL_GetTick();				// Force to return to the Edit temperature mode after timeout
			} else {										// Editing the preset temperature
				uint8_t fan	= pCore->cfg.fanSpeedPcnt();	// 0..100
				pCore->enc.reset(fan, 0, 100, 1, 5, false);
				fan_mode_end = HAL_GetTick() + fan_speed_edit_to;
			}
		} else if (pCore->cfg.device() == d_tweez) {
			bool is_on = pUnit->isOn();
			pUnit->switchPower(!is_on);
			if (is_on) {									// Save preset temperature when power-off tweezers
				pCore->cfg.save();
				use_encoder = false;
			} else {
				use_encoder = true;							// Use the rotary encoder to manage the power of the Tweezers
			}
		}
		screen_updated = 0;									// Force to redraw screen
	} else if (button == 2) {								// The encoder button was pressed for a long time, go to the main menu
	   	return mode_lpress;
	}

	if (fan_mode_end > 0 && HAL_GetTick() >= fan_mode_end) { // Fan speed edit mode timed out
		fan_mode_end = 0;
		uint16_t t_min		= pCore->cfg.tempMin();
		uint16_t t_max		= pCore->cfg.tempMax();
		uint16_t t_set		= pCore->cfg.presetTemp();
		uint8_t  step		= pCore->cfg.isBigTemsStep()?5:1;
		pCore->enc.reset(t_set, t_min, t_max, step, (step == 1)?5:10, false);
	}
	if (screen_updated >= pCore->t_last_ms) return this;
	screen_updated = HAL_GetTick();

	uint16_t gtim_period = pCore->gtim_period.read();
	bool gtim_ok = ((gtim_period > 78) && (gtim_period < 105)) || (pCore->cfg.device() != d_gun);
	if (!gtim_ok) {											// Failed to run Hot Air Gun without AC_ZERO signal
		pUnit->switchPower(false);
	}

	uint16_t temp	= pCore->term.temperature();			// The device temperature, internal units
	uint16_t tempH	= pCore->cfg.tempToHuman(temp);
	int16_t data[5];
	data[0] = pCore->cfg.presetTemp();
	data[1] = pCore->gun.fanSpeedPcnt();
	data[2]	= tempH;
	data[3] = pUnit->avgPowerPcnt();
	data[4] = (pCore->term.intTemp() + 5) /10;				// The internal max31855 temperature

	pCore->dspl.workShow(data, pCore->cfg.device(), pCore->cfg.isCelsius(), pUnit->isOn(), fan_mode_end > 0);
	return this;
}

void MWORK::clean(void) {
	pCore->dspl.blinkOn(false);
}

//---------------------- The Checking device mode ---------------------------------
void MCHECK::init(void) {
	pCore->dspl.clear();
	pCore->dspl.drawTitle(MSG_CHECKING_DEVICE);
	screen_updated 	= 0;
	dev_is_on 	= false;
	device_cold	= pCore->term.intTemp() + 50;				// The MAX31855 internal temperature + 5.0 Celsius
	if (device_cold < temp_cold)							// See vars.h
		device_cold = temp_cold;
}

MODE* MCHECK::loop(void) {
	bool recently_updated = HAL_GetTick() < pCore->t_last_ms + 1000;

	if (!pCore->cfg.isAutoCheck()) {						// Do Not check the connected device at startup
		return mode_return;
	}

	if (recently_updated) {									// Do not turn the device on until read the temperature correctly
		if (!dev_is_on) {
			check_tweez_ms = HAL_GetTick() + check_tweez_to;// Check the temperature at this time
			expected_temp = pCore->term.temperature() + 7; 	// The expected temperature should be greater
			if (expected_temp > device_cold) {				// Do not check the connected device if not cold
				return mode_return;
			}
			dev_is_on = true;
			pCore->configureDevTimer(d_tweez);
			pCore->cfg.setDevice(d_tweez);
			pCore->tweezers.fixPower(dev_power);
		}

		if (check_tweez_ms && HAL_GetTick() > check_tweez_ms) {	// It is time to check the device temperature
			pCore->tweezers.fixPower(0);
			if (pCore->term.temperature() < expected_temp) {
				pCore->configureDevTimer(d_gun);
				pCore->cfg.setDevice(d_gun);
			}
			return mode_return;
		}
	}

	if (HAL_GetTick() < screen_updated) return this;
	screen_updated = HAL_GetTick() + 500;					// The screen update period
	pCore->dspl.rotateStick(15, 1);
	return this;
}

//---------------------- The Menu mode -------------------------------------------
MMENU::MMENU(HW* pCore, MODE *m_calib, MODE *m_pid_tune, MODE *m_about) : MODE(pCore) {
	mode_calibrate		= m_calib;
	mode_pid_tune		= m_pid_tune;
	mode_about			= m_about;
}

void MMENU::init(void) {
	CONFIG *pCFG	= &pCore->cfg;
	bright			= pCFG->lcdBrightness() >> 3;
	is_buzzer		= pCFG->isBuzzerEnabled();
	is_celsius		= pCFG->isCelsius();
	is_enc_cw		= pCFG->isEncoderClockWise();
	is_big_step		= pCFG->isBigTemsStep();
	is_fast_chill	= pCFG->isFastChill();
	is_fan_24v		= pCFG->isFan24Volts();
	is_gun			= (pCFG->device() == d_gun);
	auto_check		= pCFG->isAutoCheck();
	uint8_t menu_len = pCore->dspl.menuSize(MSG_MENU_MAIN);
	pCore->enc.reset(mode_menu_item, 0, menu_len-1, 1, 1, true);
	pCore->dspl.clear();
	pCore->dspl.drawTitle(MSG_MENU_MAIN);						// "Main menu"
	set_param		= 0;
	screen_updated 	= 0;
}

MODE* MMENU::loop(void) {
	DSPL*	pD		= &pCore->dspl;
	uint8_t item 	= pCore->enc.read();
	uint8_t  button	= pCore->enc.buttonStatus();

	// Change the configuration parameters value in place
	if (mode_menu_item != item) {								// The encoder has been rotated
		mode_menu_item = item;
		switch (set_param) {									// Setup new value of the parameter in place
			case MM_BRIGHT:
				bright = constrain(item, 1, 249);				// 249 * 8 = 1992, 250 * 8 = 2000, the brightness is in [0..1999]
				pCore->dspl.BRGT::set(bright<<3);
				break;
			default:
				break;
		}
		screen_updated = 0;										// Force to redraw the screen
	}

	// Select setup menu Item
	if (!set_param) {											// Going through the menu
		if (button > 0) {										// The button was pressed, current menu item can be selected for modification
			switch (item) {										// item is a menu item
				case MM_DEV:									// Select active device: Hot Gun or Tweezers
					is_gun = !is_gun;
					break;
				case MM_UNITS:									// units C/F
					is_celsius	= !is_celsius;
					break;
				case MM_BUZZER:
					is_buzzer 	= !is_buzzer;
					break;
				case MM_ENCODER:
					is_enc_cw	= !is_enc_cw;
					break;
				case MM_TEMP_STEP:
					is_big_step = !is_big_step;
					break;
				case MM_FAST_CHILL:
					is_fast_chill = !is_fast_chill;
					break;
				case MM_FAN_VOLTAGE:
					is_fan_24v = !is_fan_24v;
					break;
				case MM_BRIGHT:
					set_param = item;
					pCore->enc.reset(bright, 1, 249, 1, 5, false);
					break;
				case MM_AUTOCHECK:
					auto_check = !auto_check;
					break;
				case MM_SAVE:									// save
				{
					pD->clear();
					pD->blinkOn(false);
					pCore->cfg.setup(bright<<3, is_celsius, is_buzzer, is_enc_cw, is_big_step, is_fast_chill, is_fan_24v, auto_check);
					tDevice dev = is_gun?d_gun:d_tweez;
					pCore->cfg.setDevice(dev);
					pCore->configureDevTimer(dev);
					pCore->cfg.save();
					pCore->enc.setClockWise(is_enc_cw);
					uint16_t min_speed = pCore->cfg.minFanSpeed();
					uint16_t max_speed = pCore->cfg.maxFanSpeed();
					pCore->gun.setFanLimits(min_speed, max_speed);
					pCore->gun.setFastGunCooling(is_fast_chill);
					mode_menu_item = 0;
					return mode_return;
				}
				case MM_CALIBRATE:
					return mode_calibrate;
				case MM_TUNE_PID:
					return mode_pid_tune;
				case MM_ABOUT:
					return mode_about;
				case MM_QUIT:
				{
					uint16_t br = pCore->cfg.lcdBrightness();
					pCore->dspl.BRGT::set(br);
					return mode_return;
				}
				default:
					break;
			}
		}
	} else {													// Finish modifying  parameter, return to menu mode
		if (button == 1) {
			item 			= set_param;
			mode_menu_item 	= set_param;
			set_param = 0;
			uint8_t menu_len = pD->menuSize(MSG_MENU_MAIN);
			pCore->enc.reset(mode_menu_item, 0, menu_len-1, 1, 1, true);
		}
	}

	// Prepare to modify menu item in-place using built-in editor
	bool modify = false;
	if (set_param >= in_place_start && set_param <= in_place_end) {
		item = set_param;
		modify 	= true;
	}

	if (button > 0) {											// Either short or long press
		screen_updated 	= 0;									// Force to redraw the screen
	}
	if (HAL_GetTick() < screen_updated) return this;
	screen_updated = HAL_GetTick() + 10000;

	// Build current menu item value
	const uint8_t value_length = 8;
	char item_value[value_length+1];
	item_value[1] = '\0';
	switch (item) {
		case MM_DEV:
			strncpy(item_value, pD->msg(is_gun?MSG_HOTGUN:MSG_TWEEZERS), value_length);
			break;
		case MM_UNITS:											// units: C/F
			item_value[0] = is_celsius?'C':'F';
			break;
		case MM_BUZZER:
			strncpy(item_value, pD->msg(is_buzzer?MSG_ON:MSG_OFF), value_length);
			break;
		case MM_ENCODER:										// Encoder mode: CW or CCW
			strncpy(item_value, pD->msg(is_enc_cw?MSG_CW:MSG_CCW), value_length);
			break;
		case MM_TEMP_STEP:										// Preset temperature step (1/5)
			item_value[0]	= is_big_step?'5':'1';
			break;
		case MM_FAST_CHILL:
			strncpy(item_value, pD->msg(is_fast_chill?MSG_ON:MSG_OFF), value_length);
			break;
		case MM_FAN_VOLTAGE:
			if (is_fan_24v) {
				item_value[0] = '2';
				item_value[1] = '4';
			} else {
				item_value[0] = '1';
				item_value[1] = '2';
			}
			item_value[2] =	'v';
			item_value[3] = '\0';
			break;
		case MM_BRIGHT:
			{
			uint8_t pcnt = map(bright, 0, 249, 0, 100);
			sprintf(item_value, "%3d%c", pcnt, '%');
			}
			break;
		case MM_AUTOCHECK:
			strncpy(item_value, pD->msg(auto_check?MSG_ON:MSG_OFF), value_length);
			break;
		default:
			item_value[0] = '\0';
			break;
	}
	item_value[value_length] = '\0';

	pD->menuShow(MSG_MENU_MAIN, item, item_value, modify);
	return this;
}

void MMENU::clean(void) {
	pCore->dspl.blinkOn(false);
}

//---------------------- The manual calibration tip mode -------------------------
// Here the operator should 'guess' the internal temperature readings for desired temperature.
// Rotate the encoder to change temperature preset in the internal units
// and controller would keep that temperature.
// This method is more accurate one, but it requires more time.
void MCALIB_MANUAL::init(void) {
	if (pCore->cfg.device() == d_gun) {
		uint16_t fan_speed = pCore->cfg.fanSpeed();
		pCore->gun.setFan(fan_speed);
	}
	ref_temp_index 		= 1;									// Start at second reference temperature
	ready				= false;
	tuning				= false;
	for (uint8_t i = 0; i < 4; ++i)								// The reference temperatures are not calibrated yet
		calib_flag[i] = false;
	temp_setready_ms	= 0;
	screen_updated		= 0;
	pCore->enc.reset(ref_temp_index, 0, 3, 1, 1, true);			// Select reference temperature point using Encoder
	pCore->cfg.getCalibtarion(calib_temp);						// Load current calibration data
	pCore->dspl.clear();
}

// Make sure the tip[0] < tip[1] < tip[2] < tip[3];
// And the difference between next points is greater than req_diff
// Change neighborhood temperature data to keep this difference
void MCALIB_MANUAL::buildCalibration(uint16_t tip[], uint8_t ref_point) {
	if (tip[3] > int_temp_max) tip[3] = int_temp_max;			// int_temp_max is a maximum possible temperature (vars.cpp)

	const int req_diff = 200;
	if (ref_point <= 3) {										// tip[0-3] - internal temperature readings for the tip at reference points (200-400)
		for (uint8_t i = ref_point; i <= 2; ++i) {				// ref_point is 0 for 200 degrees and 3 for 400 degrees
			int diff = (int)tip[i+1] - (int)tip[i];
			if (diff < req_diff) {
				tip[i+1] = tip[i] + req_diff;					// Increase right neighborhood temperature to keep the difference
			}
		}
		if (tip[3] > int_temp_max)								// The high temperature limit is exceeded, temp_max. Lower all calibration
			tip[3] = int_temp_max;

		for (int8_t i = 3; i > 0; --i) {
			int diff = (int)tip[i] - (int)tip[i-1];
			if (diff < req_diff) {
				int t = (int)tip[i] - req_diff;					// Decrease left neighborhood temperature to keep the difference
				if (t < 0) t = 0;
				tip[i-1] = t;
			}
		}
	}
}

MODE* MCALIB_MANUAL::loop(void) {
	UNIT *pUnit 		= (pCore->cfg.device() == d_gun)?(UNIT *)&pCore->gun:(UNIT *)&pCore->tweezers;
	uint16_t encoder	= pCore->enc.read();
    uint8_t  button		= pCore->enc.buttonStatus();

    int16_t enc_change = pCore->enc.changed();
    if (enc_change) {
    	if (tuning) {											// Preset temperature (internal units)
    		pUnit->setTemp(encoder);
    		ready = false;
    		if (enc_change < 0) {								// The preset temperature was decreased
    			if (restore_power_ms == 0)
    				pUnit->switchPower(false);
    			restore_power_ms = HAL_GetTick() + 500;
    		}
    		temp_setready_ms = HAL_GetTick() + 5000;    		// Prevent beep just right the new temperature setup
    	} else {
    		ref_temp_index = encoder;							// Update reference temperature index
    	}
    	screen_updated = 0;
    }

	if (button == 1) {											// The button pressed
		if (tuning) {											// New reference temperature was confirmed
			pUnit->switchPower(false);
		    if (ready) {										// The temperature has been stabilized
		    	ready = false;
		    	uint16_t temp	= pUnit->averageTemp();			// The temperature of the IRON of Hot Air Gun in internal units
			    uint8_t ref 	= ref_temp_index;
			    calib_temp[ref] = temp;
			    calib_flag[ref] = true;							// Mark this point as a calibrated
			    calibrationOLS();								// Approximate the calibration by OLS method
			    uint16_t tip[4];
			    for (uint8_t i = 0; i < 4; ++i) {
			    	tip[i] = calib_temp[i];
			    }
			    buildCalibration(tip, ref);						// ref is 0 for 200 degrees and 3 for 400 degrees
			    pCore->cfg.setCalibtarion(tip, false);			// Do not save the calibration yet
		    }
		    pCore->dspl.clear();
		    tuning	= false;
			encoder = ref_temp_index;
		    pCore->enc.reset(encoder, 0, 3, 1, 1, true);		// Turn back to the reference temperature point selection mode
		} else {												// Reference temperature index was selected from the list
			pCore->dspl.clear();
			tuning 			= true;
			uint16_t temp 	= calib_temp[encoder];				// The reference temperature
			encoder 		= temp;
			pCore->enc.reset(temp, 100, int_temp_max, (temp>1500)?5:1, 50, false); // int_temp_max declared in the vars.cpp
			pUnit->setTemp(temp);
			pUnit->switchPower(true);
			temp_setready_ms = HAL_GetTick() + 10000;
		}
		screen_updated		= 0;
		restore_power_ms	= 0;
	} else if (button == 2) {									// The button was pressed for a long time, save the calibration
		pUnit->switchPower(false);
		if (pCore->cfg.isCalibrationValid(calib_temp)) {
			pCore->cfg.setCalibtarion(calib_temp, true);
			bool calibrated = pCore->cfg.setCalibtarion(calib_temp, true);
			showResult(calibrated);
			if (calibrated) {									// Try to save calibration data to the EEPROM
				pCore->buzz.shortBeep();
			} else {
				pCore->buzz.failedBeep();
			}
			return mode_lpress;
		} else {												// Calibration is not correct
			pCore->buzz.failedBeep();
			return this;
		}
	}

	if (screen_updated >= pCore->t_last_ms) return this;
	screen_updated = HAL_GetTick();

	if (restore_power_ms > 0 && HAL_GetTick() > restore_power_ms) {
		restore_power_ms = 0;
		pUnit->switchPower(true);
	}

	uint16_t temp_set		= pUnit->presetTemp();				// Prepare the parameters to be displayed
	uint16_t temp			= pUnit->averageTemp();
	uint8_t  power			= pUnit->avgPowerPcnt();
	uint16_t tmp_disp		= pUnit->tmpDispersion();
	if (tuning && (abs(temp_set - temp) <= 16) && (tmp_disp < 20) && power > 0)  {
		if (!ready && temp_setready_ms && (HAL_GetTick() > temp_setready_ms)) {
			pCore->buzz.shortBeep();
			ready 				= true;
			temp_setready_ms	= 0;
	    }
	}

	uint16_t temp_setup = temp_set;
	if (!tuning) {
		temp_setup 		= calib_temp[ref_temp_index];
	}
	uint16_t ref_temp	= pCore->cfg.referenceTemp(ref_temp_index);
	bool is_celsius		= pCore->cfg.isCelsius();
	if (tuning) {
		int16_t delta = (int16_t)temp - (int16_t)temp_setup;
		bool recently_updated = HAL_GetTick() < pCore->t_last_ms + 1000;
		pCore->dspl.calibManualTuning(ref_temp, delta, encoder, is_celsius, power, ready, recently_updated);
	} else {
		pCore->dspl.calibManualSelect(pCore->cfg.device(), ref_temp, calib_temp[ref_temp_index], is_celsius, calib_flag[ref_temp_index]);
	}
	return this;
}

/*
 * Calculate tip calibration parameter using linear approximation by Ordinary Least Squares method
 * Y = a * X + b, where
 * Y - internal temperature, X - real temperature. a and b are double coefficients
 * a = (N * sum(Xi*Yi) - sum(Xi) * sum(Yi)) / ( N * sum(Xi^2) - (sum(Xi))^2)
 * b = 1/N * (sum(Yi) - a * sum(Xi))
 */
bool MCALIB_MANUAL::calibrationOLS(void) {
	long sum_XY = 0;											// sum(Xi * Yi)
	long sum_X 	= 0;											// sum(Xi)
	long sum_Y  = 0;											// sum(Yi)
	long sum_X2 = 0;											// sum(Xi^2)
	long N		= 0;

	for (uint8_t i = 0; i < 4; ++i) {
		uint16_t X 	= pCore->cfg.referenceTemp(i);
		uint16_t Y	= calib_temp[i];
		if (calib_flag[i]) {									// The reference temperature calibrated
			sum_XY 	+= X * Y;
			sum_X	+= X;
			sum_Y   += Y;
			sum_X2  += X * X;
			++N;
		}
	}

	if (N < 2)													// Not enough real temperatures have been entered
		return false;

	double	a  = (double)N * (double)sum_XY - (double)sum_X * (double)sum_Y;
			a /= (double)N * (double)sum_X2 - (double)sum_X * (double)sum_X;
	double 	b  = (double)sum_Y - a * (double)sum_X;
			b /= (double)N;

	for (uint8_t i = 0; i < 4; ++i) {
		if (!calib_flag[i]) {									// The reference temperature is not yet calibrated
			double temp = a * (double)pCore->cfg.referenceTemp(i) + b;
			calib_temp[i] = round(temp);
		}
	}
	if (calib_temp[3] > int_temp_max) calib_temp[3] = int_temp_max;	// Maximal possible temperature (vars.cpp)
	return true;
}

void MCALIB_MANUAL::showResult(bool ok) {
	pCore->dspl.clear();
	pCore->dspl.showCalibrated(pCore->cfg.device(), ok);

	while (true) {
		if (pCore->dspl.adjust())								// Adjust display brightness
			HAL_Delay(5);
		if (pCore->enc.buttonStatus() > 0)
			return;
	}
}

//---------------------- The PID coefficients tune mode --------------------------
void MTPID::init(void) {
	pCore->dspl.clear();
	pCore->enc.reset(0, 0, 2, 1, 1, true);						// Select the coefficient to be modified: Kp, Ki, Kd
	uint16_t temp = pCore->cfg.presetTemp();
	temp  = pCore->cfg.humanToTemp(temp);
	pUnit = (pCore->cfg.device() == d_gun)?(UNIT *)&pCore->gun:(UNIT *)&pCore->tweezers;
	pUnit->setTemp(temp);
	t_diff				= 0;
	data_update 		= 0;
	data_index 			= 0;
	modify				= false;
	on					= false;
	screen_updated 		= 0;
}

MODE* MTPID::loop(void) {
	uint16_t index		= pCore->enc.read();
	uint8_t  button		= pCore->enc.buttonStatus();
	bool enc_changed	= pCore->enc.changed();
	if (enc_changed)
		screen_updated = 0;

	if (modify) {											// The Coefficient is selected, start to heat
		if (button == 1) {									// Short button press: select another PID coefficient
			modify = false;
			pCore->enc.reset(data_index, 0, 2, 1, 1, true);
			pCore->dspl.clear();
			screen_updated = 0;								// Force to redraw screen
			return this;									// Restart the procedure
		} else if (button == 2) {							// Long button press: toggle the power
			on = !on;
			pUnit->switchPower(on);
			if (on) {
				pCore->dspl.clear();
			}
			screen_updated = 0;								// Force to redraw screen
		}
		if (enc_changed) {
			pUnit->changePID(data_index+1, index);
			return this;
		}
		if (screen_updated >= pCore->t_last_ms) return this;
		screen_updated		= HAL_GetTick();
		t_diff				= (int16_t)pUnit->averageTemp() - (int16_t)pUnit->presetTemp();
		uint8_t	 pwr		= pUnit->avgPowerPcnt();
		uint16_t t_disp		= pUnit->tmpDispersion();
		pCore->dspl.pidShow(data_index, index, t_diff, pwr, t_disp, on);
	} else {												// Selecting the PID coefficient to be tuned
		if (enc_changed) {
			data_index  = index;
		}
		if (button == 1) {									// Short button press: select another PID coefficient
			modify = true;
			pCore->dspl.clear();
			screen_updated	= 0;
			data_index  	= index;
			// Prepare to change the coefficient [index]
			uint16_t k = 0;
			k = pUnit->changePID(index+1, -1);				// Read the PID coefficient from the IRON or Hot Air Gun
			uint8_t inc 	= 1;							// Calculate increments
			uint8_t inc_b	= 10;
			if (index == 0 || index == 2) {
				inc = 10;
				inc_b = 100;
			}
			on = false;
			pUnit->switchPower(on);
			pCore->enc.reset(k, 0, 30000, inc, inc_b, false);
			return this;									// Restart the procedure
		} else if (button == 2) {							// Long button press: save the parameters and return to menu
			if (confirm()) {
				PIDparam pp = pUnit->PID::dump();
				pCore->cfg.savePID(pp);
			}
			return mode_lpress;
		}
		uint16_t pid_k[3];
		for (uint8_t i = 0; i < 3; ++i) {
			pid_k[i] = 	pUnit->changePID(i+1, -1);
		}
		if (screen_updated >= pCore->t_last_ms) return this;
		screen_updated = HAL_GetTick();
		pCore->dspl.pidParam(pid_k, data_index);
	}
	return this;
}

bool MTPID::confirm(void) {
	pCore->enc.reset(0, 0, 1, 1, 1, true);
	pCore->dspl.clear();
	uint16_t pid_k[3];
	for (uint8_t i = 0; i < 3; ++i) {
		pid_k[i] = 	pUnit->changePID(i+1, -1);
	}
	pCore->dspl.showDialog(MSG_SAVE, pid_k, 0);

	while (true) {
		if (pCore->dspl.adjust())							// Adjust display brightness
			HAL_Delay(5);
		uint8_t answer = pCore->enc.read();
		if (pCore->enc.buttonStatus() > 0)
			return answer == 1;
		if (pCore->enc.changed())
			pCore->dspl.showDialog(MSG_SAVE, pid_k, answer);
	}
	return false;
}

//---------------------- The Fail mode: display error message --------------------
void MFAIL::init(void) {
	pCore->enc.reset(0, 0, 1, 1, 1, false);
	pCore->dspl.clear();
	screen_updated = 0;
}

MODE* MFAIL::loop(void) {
	if (pCore->sensor_status == MAX31855_OK)
		return mode_return;
	if (mode_lpress && pCore->enc.buttonStatus() == 2) {
		return mode_lpress;
	}

	if (HAL_GetTick() < screen_updated) return this;
	screen_updated = HAL_GetTick() + 30000;
	if (type == FAIL_CONNECTIVITY) {
		line1 = MSG_DEVICE_ERROR;
		if (pCore->sensor_status & MAX31855_OC) {
			line2 = MSG_NOT_CONNECTED;
		} else if (pCore->sensor_status & (MAX31855_SCG | MAX31855_SCV)) {
			line2 = MSG_WRONG_CONNECTION;
		} else if (pCore->sensor_status == MAX_31855_SPI) {
			line2 = MSG_SPI_ERROR;
		} else if (pCore->sensor_status == MAX_31855_NO_DATA) {
			line2 = MSG_DATA_READ_FAILURE;
		} else {
			line2 = MSG_SENSOR_FAILED;
		}
	}
	const char *str1 = pCore->dspl.msg(line1);
	const char *str2 = pCore->dspl.msg(line2);
	pCore->dspl.errorMessage(str1, str2);
	return this;
}

void MFAIL::setMessage(t_msg_id line1, t_msg_id line2) {
	this->line1 = line1;
	this->line2 = line2;
}

//---------------------- The About dialog mode. Show about message ---------------
void MABOUT::init(void) {
	pCore->enc.reset(0, 0, 1, 1, 1, false);
	setTimeout(20);											// Show version for 20 seconds
	resetTimeout();
	pCore->dspl.clear();
	screen_updated = 0;
}

MODE* MABOUT::loop(void) {
	uint8_t b_status = pCore->enc.buttonStatus();
	if (b_status == 1) {									// Short button press
		return mode_return;									// Return to the main menu
	} else if (b_status == 2) {
		return mode_lpress;									// Activate debug mode
	}

	if (HAL_GetTick() < screen_updated) return this;
	screen_updated = HAL_GetTick() + 60000;

	pCore->dspl.showVersion();
	return this;
}

//---------------------- The Tweezers Debug mode: display internal parameters ----
void MDEBUG::init(void) {
	uint16_t min_fan = pCore->cfg.minFanSpeed();
	uint16_t max_fan = pCore->cfg.maxFanSpeed();
	pCore->gun.setFan((min_fan + max_fan) >> 1);
	if (pCore->cfg.device() == d_gun) {
		pCore->enc.reset(min_fan, min_fan, max_fan, 5, 10, false);
	} else {
		pCore->enc.reset(min_tweez_power, min_tweez_power, max_tweez_power, 5, 10, false);
	}
	pCore->dspl.clear();
	screen_updated 	= 0;
}

MODE* MDEBUG::loop(void) {
	RENC*	pEnc	= &pCore->enc;
	bool recently_updated = HAL_GetTick() < pCore->t_last_ms + 1000;
	tDevice dev = pCore->cfg.device();

	// Manage the encoder, that manages the tweezers power or fan speed of Hot Air Gun
	uint16_t pwr = pEnc->read();
	if (pEnc->changed()) {
		if (dev_is_on) {
			if (dev == d_tweez) {
				pCore->tweezers.fixPower(pwr);
			} else {
				pCore->gun.fanFixed(pwr);
			}
		}
		pCore->gun.setFan(pwr);
		screen_updated = 0;									// Force to redraw screen
	}

	// The Auto or reed switch management for Hot Air Gun
	if (pCore->cfg.device() == d_gun) {
		if (pCore->sw.status()) {							// The Reed switch is active
			if (!pCore->gun.isOn()) {
				pCore->gun.setFan(pwr);
				pCore->gun.fixPower(hot_gun_power);
			}
		} else {
			if (pCore->gun.isOn()) {
				pCore->gun.fixPower(0);
			}
		}
	}

	// Manage the encoder button
	uint8_t button = pEnc->buttonStatus();
	if (button == 1) {										// The encoder button was shortly pressed, toggle the tweezers
		screen_updated = 0;									// Force to redraw screen
		dev_is_on = !dev_is_on;
		if (dev_is_on && !recently_updated)					// Do not turn the device on until read the temperature correctly
			dev_is_on = false;
		if (dev_is_on) {
			if (dev == d_tweez) {
				pCore->tweezers.fixPower(pwr);				// Turn on the tweezers power
			} else {
				pCore->gun.fanFixed(pwr);
			}
		} else {
			if (dev == d_tweez) {
				pCore->tweezers.fixPower(0);
			} else {
				pCore->gun.fanFixed(0);
			}
		}
	} else if (button == 2) {								// The Hot Air Gun button was pressed for a long time, exit debug mode
	   	return mode_lpress;
	}

	if (screen_updated >= pCore->t_last_ms)	return this;	// The screen should be updated as soon as the sensor data has been updated
	screen_updated = HAL_GetTick();

	int16_t data[4];
	data[0]	= pwr;											// Tweezers or Fan power
	data[1]	= pCore->term.temperature();					// Device temperature
	data[2] = pCore->term.intTemp();						// The internal max31855 temperature
	data[3]	= pCore->gtim_period.read();					// GUN_TIM period
	data[3] += 5; data[3] /= 10;							// The GUN TIM period, ms

	bool gtim_ok = (data[3] > 78) && (data[3] < 105);
	bool device_is_on 	= dev_is_on;
	bool fan_is_on		= false;
	if (dev == d_gun) {
		device_is_on 	= pCore->gun.isOn();
		fan_is_on		= dev_is_on;
	}
	pCore->dspl.debugShow(data, gtim_ok, recently_updated, dev, device_is_on, fan_is_on);
	return this;
}
