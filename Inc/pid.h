/*
 * pid.h
 *
 *  2025 OCT 10
 *  	Ported from UNITED controller source code, tailored to the new hardware
 */

#ifndef _PID_H
#define _PID_H

#include "main.h"
#include "stat.h"

class PIDparam {
	public:
		PIDparam(int32_t Kp, int32_t Ki, int32_t Kd);
		PIDparam(uint16_t pid[3]);
		PIDparam(const PIDparam &p);
		int32_t	Kp					= 0;
		int32_t	Ki					= 0;
		int32_t	Kd					= 0;
};

/*  The PID algorithm 
 *  Un = Kp*(Xs - Xn) + Ki*summ{j=0; j<=n}(Xs - Xj) + Kd(Xn - Xn-1),
 *  Where Xs - is the setup temperature, Xn - the temperature on n-iteration step
 *  In this program the interactive formula is used:
 *    Un = Un-1 + Kp*(Xn-1 - Xn) + Ki*(Xs - Xn) + Kd*(Xn-2 + Xn - 2*Xn-1)
 *  With the first step:
 *  U0 = Kp*(Xs - X0) + Ki*(Xs - X0); Xn-1 = Xn;
 *  
 *  The default values of PID coefficients can be found in config.cpp
 */
class PID {
	public:
		PID(void) 											{ }
		void		load(const PIDparam &p);
		PIDparam	dump(void)								{ return PIDparam(Kp, Ki, Kd);	}
		void		init(uint8_t denominator_p = 11, bool heat_force = true);
		void 		resetPID(uint16_t t = 0);        					// reset PID algorithm history parameters
		int32_t 	reqPower(int16_t temp_set, int16_t temp_curr);
		int32_t  	changePID(uint8_t p, int32_t k);    	// set or get (if parameter < 0) PID parameter
		void		pidStable(int32_t power)				{ this->power = power; }
	private:
		void  		debugPID(int t_set, int t_curr, long kp, long ki, long kd, long delta_p);
		int16_t   	temp_h0			= 0;					// previously measured temperatures
		int16_t	  	temp_h1			= 0;
		int32_t  	power			= 0;					// The power iterative multiplied by denominator
		int32_t  	Kp 				= 10;					// The PID coefficients multiplied by denominator.
		int32_t     Ki 				= 10;
		int32_t		Kd				= 0;
		int32_t		Kp_force		= 10;
		int32_t		Ki_force		= 5;
		int16_t  	denominator_p	= 11;              		// The common coefficient denominator power of 2 (11 means 2048)
		bool		use_force		= true;					// Flag indicating to use forcibly heating mode
};

#endif
