/*
 * tweezers.h
 *
 *  2025 OCT 11
 *  	Ported from UNITED controller source code, tailored to the new hardware
 *
 */

#ifndef IRON_H_
#define IRON_H_

#include "unit.h"

class TWEEZERS : public UNIT {
	public:
	typedef enum { POWER_OFF, POWER_HEATING, POWER_ON, POWER_FIXED, POWER_COOLING } PowerMode;
		TWEEZERS(void) 											{ }
		void				init(int16_t temp);
		virtual void		switchPower(bool On);
		virtual bool		isOn(void)						{ return (mode == POWER_ON || mode == POWER_HEATING);	}
		uint16_t 			temp(void)						{ return temp_curr; 							}
		virtual uint16_t	presetTemp(void)				{ return temp_set;								}
		virtual uint16_t	averageTemp(void)				{ return h_temp.read(); 						}
		virtual uint16_t 	tmpDispersion(void)				{ return d_temp.read(); 						}
		virtual uint16_t	pwrDispersion(void)				{ return d_power.read(); 						}
		virtual uint16_t    getMaxFixedPower(void)			{ return max_fix_power; 						}
		virtual bool		isCold(void)					{ return (mode == POWER_OFF); 					}
		virtual void  		setTemp(uint16_t t);			// Set the temperature to be kept (internal units)
		virtual uint16_t    avgPower(void);					// Average applied power
		virtual uint8_t     avgPowerPcnt(void);				// Power applied to the IRON in percents
		virtual void		fixPower(uint16_t Power);		// Set the specified power to the the soldering IRON
		uint16_t			power(int32_t t);				// Required power to keep preset temperature
		void				reset(void);					// Iron is disconnected, clear the temp history
	private:
		uint16_t 	temp_set				= 0;			// The temperature that should be kept
		uint16_t    fix_power				= 0;			// Fixed power value of the IRON (or zero if off)
		volatile 	PowerMode	mode		= POWER_COOLING;// Working mode of the IRON
		volatile 	bool		chill		= false;		// Whether the IRON should be cooled (preset temp is lower than current)
		volatile	uint16_t	temp_curr 	= 0;			// The actual IRON temperature
		EXPA 		h_power;								// Exponential average of applied power
		EXPA		h_temp;									// Exponential average of temperature
		EXPA 		d_power;								// Exponential average of power math dispersion
		EXPA 		d_temp;									// Exponential temperature math dispersion
		bool		t_reset					= false;		// The temperature value was reset
		const uint16_t	max_power      		= 1800;			// Maximum power of the tweezers
		const uint16_t	max_fix_power  		= 800;			// Maximum power in fixed power mode
		const uint8_t	ec	   				= 5;			// Exponential average coefficient
		const uint16_t	iron_cold			= 300;			// The internal temperature when the IRON is cold
		const int32_t	stable				= 20000;		// The power value when the Iron reaches the preset temperature. Used in PID::pidStable()
};

#endif
