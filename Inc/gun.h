/*
 * gun.h
 *
 * 2026 MAY 29 v1.00
 * 		Ported from F103_united_rework project, tailored to the new hardware
 */

#ifndef GUN_H_
#define GUN_H_

#include "stat.h"
#include "tools.h"
#include "unit.h"
#include "vars.h"

class HOTGUN : public UNIT {
    public:
		typedef enum { POWER_OFF, POWER_HEATING, POWER_ON, POWER_FIXED, POWER_STBY, POWER_COOLING } PowerMode;
        HOTGUN(void) 		{ }
        void        		init(int16_t temp, int16_t temp_i);
		virtual bool		isOn(void)						{ return (mode == POWER_ON || mode == POWER_HEATING || mode == POWER_FIXED); }
		virtual uint16_t	presetTemp(void)				{ return temp_set; 								}
		uint16_t			presetFan(void)					{ return fan_speed;								}
		virtual uint16_t 	averageTemp(void)				{ return avg_sync_temp; 						}
        virtual uint16_t	getMaxFixedPower(void)			{ return max_fix_power; 						}
        virtual bool		isCold(void)					{ return mode == POWER_OFF;						}
        bool				isFanWorking(void)				{ return (fanSpeed() >= min_fan_speed);			}
        virtual uint16_t	pwrDispersion(void)				{ return d_power.read(); 						}
        virtual uint16_t 	tmpDispersion(void)				{ return d_temp.read(); 						}
		virtual void		setTemp(uint16_t temp)			{ temp_set	= constrain(temp, 0, int_temp_max);	}
		void				setFan(uint16_t fan)			{ fan_speed = constrain(fan, min_fan_speed, max_fan_speed);	}
		void				setFastGunCooling(bool on)		{ fast_cooling = on;							}
		void				setConnected(bool connected)	{ is_connected = connected;						}
		bool				isCooling(void)					{ return mode == POWER_COOLING;					}
		void				fanFixed(uint16_t fan);
		void				fanControl(bool on);
		void				updateTemp(uint16_t value);		// The Hot Gun temperature, Celsius * 10 (not calibrated)
        virtual void		switchPower(bool On);
        virtual uint16_t	avgPower(void)					{ return avgPowerPcnt();						}
        virtual uint8_t		avgPowerPcnt(void);
		uint16_t			fanSpeed(void);					// Fan supplied to Fan, PWM duty
		uint8_t				fanSpeedPcnt(void);
        virtual void        fixPower(uint16_t Power);		// Set the specified power to the the hot gun
		uint8_t				presetFanPcnt(void);
		uint16_t			power(void);					// Required Hot Air Gun power to keep the preset temperature
		void				safetyRelay(bool activate);
		void        		lowPowerMode(uint16_t t);		// Activate low power mode (preset temp.) To disable, use switchPower(true)
		void				setFanLimits(uint16_t min_speed, uint16_t max_speed);
		void				setColdTemperature(int16_t temp_internal);
    private:
		void		shutdown(void);
		void		regMinCoolingTemp(void)					{ min_cool_temp	= avg_sync_temp; min_cool_tm = HAL_GetTick(); }
		PowerMode	mode				= POWER_OFF;
		uint8_t    	fix_power			= 0;				// Fixed power value of the Hot Air Gun (or zero if off)
		bool		chill				= false;			// Chill the Hot Air gun if it is over heating
		bool		reach_cold_temp		= true;				// Flag indicating the Hot Air Gun has reached the 'temp_gun_cold' temperature
		bool		fast_cooling		= false;			// Flag indicating maximum fan speed when cooling
		uint16_t	temp_set			= 0;				// The preset temperature of the hot air gun (internal units)
		uint16_t	fan_speed			= 0;				// Preset fan speed
		uint16_t	low_temp			= 0;				// The temperature in standby mode (if not zero)
		uint32_t	fan_off_time		= 0;				// Time when the fan should be powered off in cooling mode (ms)
		uint16_t	min_cool_temp		= 0;				// The minimum registered temperature in cooling mode
		uint32_t	min_cool_tm			= 0;				// The time when the minimum registered temperature in cooling mode reached
		bool		is_connected		= false;			// The temperature read successfully. Updated by setConnected()
		uint16_t	min_fan_speed		= 70;				// The minimum fan speed (depends on fan voltage)
		uint16_t	max_fan_speed		= 700;				// The maximum fan speed (depends on fan voltage)
		uint16_t	c_temp				= 0;				// Hot Air Gun current temperature. See updateTemp()
		EXPA		h_power;								// Exponential average of applied power
		EXPA		h_temp;									// Exponential average of Hot Air Gun history temperature.
		EXPA		d_power;								// Exponential average of power dispersion
		EXPA		d_temp;									// Exponential temperature math dispersion
		EXPA		zero_temp;								// Exponential average of minimum (zero) temperature
		bool		relay_activated		= false;			// The relay activated flag
        uint16_t	temp_gun_off		= 0;				// Temperature when is safe to turn-off the Fan, initialized in init()
		volatile    uint16_t	avg_sync_temp	= 0;		// Average temperature synchronized with TIM1 (used to calculate required power, see power() method)
		volatile 	uint8_t		relay_ready_cnt	= 0;		// The relay ready counter, see HOTHUN::power()
        const       uint8_t     max_fix_power 	= 50;
		const		uint8_t		max_power		= 80;
        const		uint32_t	fan_off_timeout	= 6*60*1000;// The timeout to turn the fan off in cooling mode
        const		uint32_t	fan_extra_time	= 60000;	// Extra time to wait after the Hot Air Gun reaches the 'temp_gun_cold' temperature
		const		uint8_t		fan_curr_avg_len= 13;
		const		uint8_t		ec	   			= 5;		// Exponential average coefficient
        const		uint32_t	relay_activate	= 1;		// The relay activation delay (loops of TIM1, 1 time per second)
        const		uint32_t	cooling_to		= 60000;	// If min_cool_temp min_cool_temp has not been changed during this timeout, the minimum temperature reached
};

#endif
