/*
 * hw.h
 */

#ifndef _HW_H_
#define _HW_H_

#include "config.h"
#include "display.h"
#include "tweezers.h"
#include "gun.h"
#include "max31855.h"
#include "encoder.h"
#include "buzzer.h"
#include "stat.h"

// The rotary encoder timer
extern TIM_HandleTypeDef 	htim3;
// The Fan or Tweezers timer instance
extern TIM_HandleTypeDef 	htim2;

typedef enum {
	HW_ERR_OK							= 0x0000,
	HW_ERR_EEPROM						= 0x0001
} HW_ERROR;

class HW {
	public:
		HW(void) : gtim_period(10), enc(&htim3) 		{ }
		void			configureDevTimer(tDevice dev);
		HW_ERROR		init(void);
		CONFIG			cfg;
		TWEEZERS		tweezers;
		HOTGUN			gun;
		DSPL			dspl;
		MAX31855		term;
		MAX31855_STATUS	sensor_status	= MAX_31855_NO_DATA;
		uint32_t		t_last_ms		= 0;			// Time when the temperature was received
		EXPA			gtim_period;
		RENC			enc;							// The rotary encoder
		BUZZER			buzz;
		SWITCH 			sw;								// Auto switch
	private:
		const 		uint8_t		sw_off_value	= 30;
		const 		uint8_t		sw_on_value		= 60;
		const 		uint8_t		sw_avg_len		= 7;
};

#endif
