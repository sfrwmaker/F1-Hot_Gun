/*
 * pid.cpp
 *
 *  2025 OCT 10, v.1.00
 * 		Ported from UNITED controller source code, tailored to the new hardware
 */

#include "pid.h"
#include "tools.h"

PIDparam::PIDparam(int32_t Kp, int32_t Ki, int32_t Kd) {
	this->Kp	= Kp;
	this->Ki	= Ki;
	this->Kd	= Kd;
}

PIDparam::PIDparam(uint16_t pid[3]) {
	Kp = pid[0];
	Ki= pid[1];
	Kd = pid[2];
}

PIDparam::PIDparam(const PIDparam &p) {
	Kp	= p.Kp;
	Ki	= p.Ki;
	Kd	= p.Kd;
}

// When load the PID parameters, calculate aggressive heating mode parameter values also:
// Increase the Kp in the aggressive mode in several times,
// Decrease the Ki in the aggressive mode. The Kd is not used in the aggressive mode
void PID::load(const PIDparam &p) {
	Kp	= p.Kp;
	Ki	= p.Ki;
	Kd	= p.Kd;
	Kp_force = Kp * 5;
	Ki_force = Ki / 10;
	if (Ki_force < 5) Ki_force = 5;
}

void PID::init(uint8_t denominator_p, bool heat_force) { 	// PID parameters are initialized from EEPROM by  call
	Kp	= 10;
	Ki	= 10;
	Kd  = 0;
	Kp_force	= 10;
	Ki_force	= 5;
	this->denominator_p = denominator_p;
	use_force	= heat_force;
}

void PID::resetPID(uint16_t t) {
	temp_h0 		= 0;
	temp_h1 		= t;
	power  			= 0;
}

int32_t PID::changePID(uint8_t p, int32_t k) {
	switch(p) {
    	case 1:
    		if (k >= 0) Kp = k;
    		return Kp;
    	case 2:
    		if (k >= 0) Ki = k;
    		return Ki;
    	case 3:
    		if (k >= 0) Kd = k;
    		return Kd;
    	default:
    		break;
	}
	return 0;
}

int32_t PID::reqPower(int16_t temp_set, int16_t temp_curr) {
	if (use_force && temp_curr + 100 < temp_set) {			// Aggressive heat-up mode, use Kp_force and Ki_forse only
		if (temp_h0 == 0) {									// Use direct formulae because do not know previous temperature
			power 		= 0;
			int32_t	i_summ 	= temp_set - temp_curr;
			power = Kp_force*(temp_set - temp_curr) + Ki_force * i_summ;
		} else {
			int32_t kp = Kp_force * (temp_h1 	- temp_curr);
			int32_t ki = Ki_force * (temp_set	- temp_curr);
			int32_t delta_p = kp + ki;
			power += delta_p;								// Power is stored multiplied by denominator!
		}
	} else {												// Use regular PID parameters near preset temperature
		if (temp_h0 == 0) {									// Use direct formulae because do not know previous temperature
			power 		= 0;
			int32_t	i_summ 	= temp_set - temp_curr;
			power = Kp*(temp_set - temp_curr) + Ki * i_summ;
		} else {
			int32_t kp = Kp * (temp_h1 	- temp_curr);
			int32_t ki = Ki * (temp_set	- temp_curr);
			int32_t kd = Kd * (temp_h0 	+ temp_curr - 2 * temp_h1);
			int32_t delta_p = kp + ki + kd;
			power += delta_p;								// Power is stored multiplied by denominator!
		}
	}
	temp_h0 = temp_h1;
	temp_h1 = temp_curr;
	int32_t pwr = power + (1 << (denominator_p-1));			// prepare the power to divide by denominator, round the result
	pwr >>= denominator_p;									// divide by the denominator
	return pwr;
}
