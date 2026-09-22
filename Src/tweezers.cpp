/*
 * tweezers.cpp
 *
 *  2025 OCT 11
 *  	Ported from UNITED controller source code, tailored to the new hardware
 */

#include "tweezers.h"
#include "tools.h"
#include "vars.h"

#define TWEEZ_TIM		htim2
extern TIM_HandleTypeDef TWEEZ_TIM;

void TWEEZERS::init(int16_t temp) {
	mode		= POWER_COOLING;
	fix_power	= 0;
	chill		= false;
	t_reset		= true;										// This flag indicating the temperature value was reset
	h_power.length(ec);
	h_temp.length(ec);
	h_temp.reset(temp);
	d_power.length(ec);
	d_temp.length(ec);

	PID::init(9, true);									// Initialize PID for the IRON.
	resetPID();
}

void TWEEZERS::switchPower(bool On) {
	if (!On) {
		fix_power	= 0;
		if (mode != POWER_OFF) {
			mode = POWER_COOLING;							// Start the cooling process
			TWEEZ_TIM.Instance->CCR1	= 0;
		}
	} else {
		resetPID();
		uint16_t t = h_temp.read();
		if (t < temp_set && t + 20 < temp_set) {
			mode		= POWER_HEATING;
		} else {
			mode		= POWER_ON;
		}
	}
	h_power.reset();
	d_power.reset();
}

void TWEEZERS::setTemp(uint16_t t) {
	if (mode == POWER_ON) resetPID();
	if (t > int_temp_max) t = int_temp_max;					// Do not allow over heating. int_temp_max is defined in vars.cpp
	temp_set = t;
	uint16_t ta = h_temp.read();
	chill = (ta > t + 20);                         			// The IRON must be cooled
}

uint16_t TWEEZERS::avgPower(void) {
	uint16_t p = h_power.read();
	if (mode == POWER_FIXED)
		p = fix_power;
	if (p > max_power) p = max_power;
	return p;
}

uint8_t TWEEZERS::avgPowerPcnt(void) {
	uint16_t p 		= h_power.read();
	uint16_t max_p 	= max_power;
	if (mode == POWER_FIXED) {
		p	  = fix_power;
		max_p = max_fix_power;
	}
	p = constrain(p, 0, max_p);
	return map(p, 0, max_p, 0, 100);
}

void TWEEZERS::fixPower(uint16_t Power) {
	h_power.reset();
	d_power.reset();
	if (Power == 0) {										// To switch off the IRON, set the Power to 0
		TWEEZ_TIM.Instance->CCR1	= 0;					// Switch-off the tweezers immediately
		fix_power 	= 0;
		mode		= POWER_COOLING;
		return;
	}

	if (Power > max_fix_power)
		fix_power 	= max_fix_power;

	fix_power 	= Power;
	mode		= POWER_FIXED;
}

// Called from HAL_TIM_PWM_PulseFinishedHalfCpltCallback() and HAL_TIM_PWM_PulseFinishedCallback() event handlers. See core.cpp for details.
uint16_t TWEEZERS::power(int32_t t) {
	if (t_reset) {
		h_temp.reset(t);
		t_reset = false;
	}
	temp_curr		= t;
	int32_t at 		= h_temp.average(temp_curr);
	int32_t diff	= at - temp_curr;
	d_temp.update(diff*diff);
	bool overheat = false;
	if (t >= int_temp_max + 100) {							// Prevent global over heating
		int32_t	ap		= h_power.average(0);
		diff 			= ap - 0;
		d_power.update(diff*diff);
		overheat = true;
	}

	int32_t p = 0;
	switch (mode) {
		case POWER_COOLING:
			if (at < iron_cold)
				mode = POWER_OFF;							// no break in this case because of check current through the IRON
			break;
		case POWER_HEATING:
			if (t >= temp_set + 20) {
				mode = POWER_ON;
				PID::pidStable(stable);
			}
			p = PID::reqPower(temp_set, t);
			p = constrain(p, 0, max_power);
			break;
		case POWER_ON:
			if (!overheat) {
				uint16_t t_set = temp_set;
				if (t > (temp_set + 100)) {					// Prevent over heating
					chill = true;
				}
				if (chill) {
					if (t < (t_set - 10)) {
						chill = false;
						resetPID(t);
					} else {								// Do not supply power, wait the IRON get colder
						break;
					}
				}
				p = PID::reqPower(t_set, t);
				p = constrain(p, 0, max_power);
			}
			break;
		case POWER_FIXED:
			if (!overheat) {
				p = fix_power;
			}
			break;
		case POWER_OFF:
		default:
			break;
	}

	int32_t	ap		= h_power.average(p);
	diff 			= ap - p;
	d_power.update(diff*diff);
	return p;
}

void TWEEZERS::reset(void) {
	t_reset		= true;										// This flag indicating the temperature value was reset
	h_power.reset();
	h_temp.reset();
	d_power.reset();
	d_temp.reset();
	mode = POWER_COOLING;
}
