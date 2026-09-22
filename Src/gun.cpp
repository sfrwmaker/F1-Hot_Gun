/*
 * gun.cpp
 *
 *  2026 MAY 29
 *  	Ported from UNITED controller source code, tailored to the new hardware
 */

#include "gun.h"

#define FAN_TIM		htim2
extern TIM_HandleTypeDef FAN_TIM;

void HOTGUN::init(int16_t temp, int16_t temp_i) {
	mode			= POWER_OFF;							// Completely stopped, no power on fan also
	fan_speed		= 0;
	fix_power		= 0;
	relay_activated	= false;
	chill			= false;
	c_temp			= temp;
    temp_gun_off 	= temp_cold;							// See vars.h
    if (temp_i > temp_gun_off) {
    	temp_gun_off = temp_i;								// The internal MAX31855 temperature
    }
	safetyRelay(false);										// Completely turn-off the power of Hot Air Gun
	h_power.length(ec);
    h_power.reset();
    h_temp.length(ec);										// The Hot Air Gun long-term average temperature
	h_temp.reset();
	d_power.length(ec);
	d_temp.length(ec);
	PID::init(13, false);									// Initialize PID for Hot Air Gun, 1Hz. Do not forcible heat!
    resetPID();
}

uint8_t HOTGUN::avgPowerPcnt(void) {
	uint8_t pcnt = 0;
	if (mode == POWER_FIXED) {
		pcnt = map(fix_power, 0, max_fix_power, 0, 100);
	} else {
		pcnt = map(h_power.read(), 0, max_power, 0, 100);
	}
	if (pcnt > 100) pcnt = 100;
	return pcnt;
}

uint16_t HOTGUN::fanSpeed(void) {
	return constrain(FAN_TIM.Instance->CCR1, 0, 1999);
}

uint8_t HOTGUN::fanSpeedPcnt(void) {
	uint16_t fan = fanSpeed();
	if (fan == 0) fan = fan_speed;
	return map(fan, 0, max_fan_speed, 0, 100);
}

void HOTGUN::fanFixed(uint16_t fan) {
	FAN_TIM.Instance->CCR1 = constrain(fan, 0, max_fan_speed);
}

void HOTGUN::fanControl(bool on) {
	if (mode == POWER_OFF) {
		FAN_TIM.Instance->CCR1	= (on)?fan_speed:0;
	}
}

void HOTGUN::updateTemp(uint16_t value) {
	if (is_connected) {
			c_temp = value;
			int32_t at = h_temp.average(value);
			int32_t diff	= at - value;
			d_temp.update(diff*diff);
		}
}

void HOTGUN::switchPower(bool On) {
	fan_off_time = 0;										// Disable fan offline by timeout
	switch (mode) {
		case POWER_OFF:
			if (fanSpeed() == 0) {							// No power supplied to the Fan
				if (On)	{									// !FAN && On
					mode = POWER_HEATING;					// Do not activate the safety relay yet, check the fan is blowing first (see HOTGUN::power())
					min_cool_tm		= 0;
				}
			} else {
				if (On) {
					if (is_connected) {						// FAN && On && connected
						safetyRelay(true);
						if (c_temp < temp_set && c_temp + 200 < temp_set) {
							mode = POWER_HEATING;
						} else {
							mode = POWER_ON;
						}
						min_cool_tm		= 0;
					} else {								// FAN && On && !connected
						shutdown();
					}
				} else {
					if (is_connected) {					// FAN && !On && connected
						if (avg_sync_temp < temp_gun_off) { // FAN && !On && connected && cold
							shutdown();
						} else {							// FAN && !On && connected && !cold
							mode = POWER_COOLING;
							fan_off_time 	= HAL_GetTick() + fan_off_timeout;
							reach_cold_temp	= false;
							regMinCoolingTemp();
						}
					}
				}
			}
			break;
		case POWER_ON:
		case POWER_HEATING:
		case POWER_STBY:
			if (!On) {										// Start cooling the hot air gun
				mode = POWER_COOLING;
				fan_off_time = HAL_GetTick() + fan_off_timeout;
				reach_cold_temp = false;
				regMinCoolingTemp();
				if (fast_cooling) {							// Set maximum fan speed in case of fast cooling
					FAN_TIM.Instance->CCR1 = max_fan_speed;
				}
			}
			break;
		case POWER_FIXED:
			if (fanSpeed()) {
				if (On) {									// FAN && On
					mode = POWER_ON;
					min_cool_tm		= 0;
				} else {									// FAN && !On
					if (is_connected) {						// FAN && !On && connected
						if (avg_sync_temp < temp_gun_off) { // FAN && !On && connected && cold
							shutdown();
						} else {							// FAN && !On && connected && !cold
							mode = POWER_COOLING;
							fan_off_time = HAL_GetTick() + fan_off_timeout;
							reach_cold_temp = false;
							regMinCoolingTemp();
							if (fast_cooling) {				// Set maximum fan speed in case of fast cooling
								FAN_TIM.Instance->CCR1 = max_fan_speed;
							}
						}
					} else {
						shutdown();
					}
				}
			} else {										// !FAN
				if (!On) {									// !FAN && !On
					shutdown();
				}
			}
			break;
		case POWER_COOLING:
			if (fanSpeed()) {
				if (On) {									// FAN && On
					if (is_connected) {						// FAN && On && connected
						safetyRelay(true);					// Supply AC power to the hot air gun socket
						if (c_temp < temp_set && c_temp + 200 < temp_set) {
							mode = POWER_HEATING;
						} else {
							mode = POWER_ON;
						}
						min_cool_tm		= 0;
					} else {								// FAN && On && !connected
						shutdown();
					}
				} else {									// FAN && !On
					if (is_connected) {
						if (avg_sync_temp < temp_gun_off) { // FAN && !On && connected && cold
							fan_off_time = HAL_GetTick() + fan_extra_time;
							reach_cold_temp = true;
						}
					} else {								// FAN && !On && !connected
						shutdown();
					}
				}
			} else {
				if (On) {									// !FAN && On
					safetyRelay(true);						// Supply AC power to the hot air gun socket
					mode = POWER_HEATING;
				}
			}
			break;
		default:
			break;
	}
	h_power.reset();
	d_power.reset();
}

void HOTGUN::fixPower(uint16_t Power) {
    if (Power == 0) {										// To switch off the hot gun, set the Power to 0
        switchPower(false);
        return;
    }

    if (Power > max_power) Power = max_power;
    mode = POWER_FIXED;
    safetyRelay(true);										// Supply AC power to the hot air gun socket
    fix_power	= Power;
}


// Called by event handlers every 1.2 seconds (see core.cpp)
uint16_t HOTGUN::power(void) {
	avg_sync_temp	= h_temp.read();						// Save average temperature to be read as average value

	if ((c_temp >= int_temp_max + 100) || (c_temp > (temp_set + 400))) {	// Prevent global over heating
		if (mode == POWER_ON) chill = true;					// Turn off the power in main working mode only;
	}

	int32_t	p = 0;											// The Hot Air Gun power value
	switch (mode) {
		case POWER_OFF:
			break;
		case POWER_HEATING:
			if (!relay_activated && is_connected) {			// Activate the relay if Hot Gun is connected
				safetyRelay(true);
				PID::resetPID();
			}
		case POWER_ON:
			FAN_TIM.Instance->CCR1	= fan_speed;
			if (chill) {
				if (c_temp < (temp_set - 2)) {
					chill = false;
					resetPID();
				} else {
					break;
				}
			}
			if (mode == POWER_HEATING && c_temp >= temp_set) {
				mode = POWER_ON;
			}
			if (relay_activated) {							// Do supply power to the heater if the relay activated
				if (relay_ready_cnt > 0) {					// Relay is not ready yet
					--relay_ready_cnt;						// Do not apply power to the HOT GUN till AC relay is ready
					relay_ready_cnt &= 7;
				} else {
					p = PID::reqPower(temp_set, c_temp);
					p = constrain(p, 0, max_power);
				}
			}
			break;
		case POWER_FIXED:
			if (relay_ready_cnt > 0) {						// Relay is not ready yet
				--relay_ready_cnt;							// Do not apply power to the HOT GUN till AC relay is ready
			} else {
				p = fix_power;
			}
			FAN_TIM.Instance->CCR1	= fan_speed;
			break;
		case POWER_STBY:
			FAN_TIM.Instance->CCR1	= min_fan_speed;
			if (chill) {
				if (c_temp < (low_temp - 2)) {
					chill = false;
					resetPID();
				} else {
					break;
				}
			}
			p = PID::reqPower(low_temp, c_temp);
			p = constrain(p, 0, max_power);
			break;
		case POWER_COOLING:
			if (fanSpeed() < min_fan_speed) {
				shutdown();
			} else {
				if (is_connected) {
					if (avg_sync_temp < temp_gun_off) {		// FAN && connected && absolutely cold
						shutdown();
						break;
					}
					if (avg_sync_temp < min_cool_temp)
						regMinCoolingTemp();
					if (min_cool_tm && HAL_GetTick() >= min_cool_tm + cooling_to) {	// FAN && connected && min_cool_temp has not been changed during timeout
						if (!reach_cold_temp) {
							reach_cold_temp = true;
							fan_off_time = HAL_GetTick() + fan_extra_time;
							FAN_TIM.Instance->CCR1 = max_fan_speed;
						}
					} else {								// FAN && connected && !cold
						if (!fast_cooling && !reach_cold_temp) { // Use standard cooling algorithm
							uint16_t fan = map(avg_sync_temp, temp_gun_off, temp_set, max_fan_speed, min_fan_speed);
							FAN_TIM.Instance->CCR1 = fan;
						}
					}
				}  else {									// No Hot Air Gun connected
					shutdown();
				}
				// Here the FAN is working but the Hot Air Gun can be disconnected
				if (fan_off_time && HAL_GetTick() >= fan_off_time) { // The fan should be turned off in specific time
					shutdown();
				}
			}
			break;
		default:
			break;
	}

	// Only supply the power to the heater if the Hot Air Gun is connected
	if (fanSpeed() < min_fan_speed || !is_connected) p = 0;
	h_power.update(p);
	int32_t	ap	= h_power.average(p);
	int32_t	diff 	= ap - p;
	d_power.update(diff*diff);
	return p;
}

uint8_t	HOTGUN::presetFanPcnt(void) {
	return map(fan_speed, 0, max_fan_speed, 0, 100);
}

// Can be called from the event handler.
void HOTGUN::shutdown(void)	{
	mode = POWER_OFF;
	FAN_TIM.Instance->CCR1 = 0;
	safetyRelay(false);										// Stop supplying AC power to the hot air gun
	fan_off_time	= 0;
	reach_cold_temp = true;
}

// We need some time to activate the relay, so we initialize the relay_ready_cnt variable.
void HOTGUN::safetyRelay(bool activate) {
	if (activate) {
		HAL_GPIO_WritePin(AC_RELAY_GPIO_Port, AC_RELAY_Pin, GPIO_PIN_SET);
		relay_ready_cnt = relay_activate;
	} else {
		HAL_GPIO_WritePin(AC_RELAY_GPIO_Port, AC_RELAY_Pin, GPIO_PIN_RESET);
		relay_ready_cnt = 0;
	}
	relay_activated = activate;
}

void HOTGUN::lowPowerMode(uint16_t t) {
    if ((mode == POWER_ON || mode == POWER_HEATING) && t < temp_set) {
    	low_temp = t;                           			// Activate low power mode
        chill = true;										// Stop heating, when temp reaches standby one, reset PID
    	h_power.reset();
    	d_power.reset();
    	mode = POWER_STBY;
    }
}

void HOTGUN::setFanLimits(uint16_t min_speed, uint16_t max_speed) {
	min_fan_speed		= min_speed;
	max_fan_speed		= max_speed;
}

void HOTGUN::setColdTemperature(int16_t temp_internal) {
    if (temp_internal > temp_gun_off)
    	temp_gun_off = temp_internal;						// The internal MAX31855 temperature
}
