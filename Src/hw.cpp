/*
 * hw.cpp
 *
 */

#include <math.h>
#include "hw.h"

HW_ERROR HW::init(void) {
	sw.init(sw_avg_len,	sw_off_value,  sw_on_value);
	dspl.init();
	enc.start();
	enc.addButton(ENC_B_GPIO_Port, ENC_B_Pin);
	sensor_status = term.status();						// Read the device temperature
	if (MAX31855_OK == sensor_status) {
		t_last_ms = HAL_GetTick();
	}
	int16_t temp	= term.temperature();
	int16_t	temp_i	= term.intTemp();
	bool fast_cool	= cfg.isFastChill();
	gun.init(temp, temp_i);
	gun.setFastGunCooling(fast_cool);
	tweezers.init(temp);
	bool cfg_ok = cfg.init();
	PIDparam pp   = cfg.pidParams(d_tweez);
	tweezers.load(pp);
	pp   	= cfg.pidParams(d_gun);
	gun.load(pp);
	uint16_t min_fan	= cfg.minFanSpeed();
	uint16_t max_fan	= cfg.maxFanSpeed();
	gun.setFanLimits(min_fan, max_fan);
	enc.setClockWise(cfg.isEncoderClockWise());
	if (cfg_ok) {
		tDevice dev = cfg.device();						// Configure the controller hardware for specific device
		configureDevTimer(dev);
		return HW_ERR_OK;
	}
	return HW_ERR_EEPROM;
}

void HW::configureDevTimer(tDevice dev) {
	if (dev == d_gun) {									// Increase the TIM2 frequency to power the Fan of the Hot Air Gun
		htim2.Instance->PSC = 2;
	} else {											// Decrease the TIM2 frequency to power the Tweezers power
		htim2.Instance->PSC = 99;
	}
}
