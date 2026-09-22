/*
 * mode.h
 *
 *  2025 OCT 09, v.1.00
 * 		Ported from UNITED controller source code, tailored to the new hardware
 */

#include <string.h>
#include "hw.h"
#include "msg.h"

#ifndef _MODE_H_
#define _MODE_H_

class MODE {
	public:
		MODE(HW *pCore)										{ this->pCore = pCore; 	}
		void			setup(MODE* return_mode, MODE* long_mode);
		virtual void	init(void)							{ }
		virtual MODE*	loop(void)							{ return 0; }
		virtual void	clean(void)							{ }
		virtual			~MODE(void)							{ }
		MODE*			returnToMain(void);
	protected:
		void 			resetTimeout(void);
		void 			setTimeout(uint16_t t);
		HW*				pCore			= 0;
		uint16_t		timeout_secs	= 0;				// Timeout to return to main mode, seconds
		uint32_t		time_to_return 	= 0;				// Time in ms when to return to the main mode
		uint32_t		screen_updated	= 0;				// Time when the screen has been be updated last time, ms
		MODE*			mode_return		= 0;				// Previous working mode
		MODE*			mode_lpress		= 0;				// When encoder button long  pressed

};

//---------------------- The Main Working mode -----------------------------------
class MWORK : public MODE {
	public:
		MWORK(HW* pCore) : MODE(pCore) 						{ }
		virtual void	init(void);
		virtual MODE*	loop(void);
		virtual void	clean(void);
	private:
		UNIT			*pUnit			= 0;
		bool			use_encoder		= false;			// Use encoder to manage the power of the Tweezers
		uint32_t		fan_mode_end	= 0;				// When the fan speed mode ends or zero in case of temperature editing mode, ms
		const uint16_t 	fan_speed_edit_to = 5000;			// Timeout for Fan speed edit mode
};

//---------------------- The Checking device mode ---------------------------------
class MCHECK : public MODE {
	public:
		MCHECK(HW* pCore) : MODE(pCore)						{ }
		virtual void	init(void);
		virtual MODE*	loop(void);
	private:
		bool			dev_is_on		= false;
		uint32_t		check_tweez_ms	= 0;				// The time when to check the temperature raised
		uint16_t		expected_temp	= 0;				// The expected temperature of the tweezers
		uint16_t		device_cold		= 0;				// The device is cold, internal temperature, initialized in init()
		const uint16_t	dev_power 		= 50;				// Apply this power ro check the connected device
		const uint32_t	check_tweez_to	= 1500;				// The period to check the tweezers temperature
};

//---------------------- The Menu mode -------------------------------------------
class MMENU : public MODE {
	public:
		MMENU(HW* pCore, MODE *m_calib, MODE *m_pid_tune, MODE *m_about);
		virtual void	init(void);
		virtual MODE*	loop(void);
		virtual void	clean(void);
	private:
		MODE*		mode_calibrate;
		MODE*		mode_pid_tune;
		MODE*		mode_about;
		uint8_t		bright			= 127;					// The display brightness (divided by 8)
		bool		is_gun			= true;					// Is the device connected is Hot Gun
		bool		is_celsius		= true;					// Celsius or Fahrenheit
		bool		is_enc_cw		= true;					// The rotary encoder Clockwise or ConterClockwise
		bool		is_big_step		= false;				// Is the temperature step is 5 degrees Celsius or 1
		bool		is_fast_chill	= false;				// Is the Hot Air Gun fast chill
		bool		is_buzzer		= false;				// Is the buzzer enabled
		bool		is_fan_24v		= false;				// Is the Fan of the Hot Air Gun is 24 volts capable
		bool		auto_check		= true;					// Is automatically check the device at startup
		uint8_t		mode_menu_item 	= 0;					// Save active menu element index to return back later
		uint8_t		set_param		= 0;					// The index of the modifying parameter
		const uint16_t	min_standby_C	= 120;				// Minimum standby temperature, Celsius
		enum { MM_DEV = 0, MM_UNITS, MM_BUZZER, MM_ENCODER, MM_TEMP_STEP, MM_BRIGHT, MM_AUTOCHECK, MM_FAST_CHILL,
			MM_FAN_VOLTAGE, MM_CALIBRATE, MM_TUNE_PID, MM_ABOUT, MM_QUIT, MM_SAVE
		};
		const uint8_t	in_place_start	= MM_BRIGHT;		// See the menu names. Index of the first parameter that can be changed inside menu (see msg.h)
		const uint8_t	in_place_end	= MM_BRIGHT;		// See the menu names. Index of the last parameter that can be changed inside menu
};

//---------------------- The calibrate tip mode: manual calibration --------------
class MCALIB_MANUAL : public MODE {
	public:
		MCALIB_MANUAL(HW *pCore) : MODE(pCore)				{ }
		virtual void	init(void);
		virtual MODE*	loop(void);
	private:
		void 		buildCalibration(uint16_t tip[], uint8_t ref_point);
		bool		calibrationOLS(void);
		void		showResult(bool ok);
		uint8_t		ref_temp_index	= 1;					// Which temperature reference to change: [0-ref_points]
		uint16_t	calib_temp[4];							// The calibration temp. in internal units in reference points
		bool		calib_flag[4];							// Flag indicating the reference temperature has been calibrated
		bool		ready			= 0;					// Whether the temperature has been established
		bool		tuning			= 0;					// Whether the reference temperature is modifying (else we select new reference point)
		uint32_t	temp_setready_ms= 0;					// The time in ms when we should check the temperature is ready
		uint32_t	restore_power_ms= 0;
};

//---------------------- The PID coefficients tune mode --------------------------
class MTPID : public MODE {
	public:
		MTPID(HW *pCore) : MODE(pCore)						{ }
		virtual void	init(void);
		virtual MODE*	loop(void);
	private:
		bool		confirm(void);							// Confirmation dialog
		UNIT*		pUnit		= 0;						// Pointer to the active unit: Hot Air Gun or Tweezers
		uint32_t	data_update	= 0;						// When read the data from the sensors (ms)
		uint8_t		data_index	= 0;						// Active coefficient
		bool        modify		= 0;						// Whether is modifying value of coefficient
		bool		on			= 0;						// Whether the IRON or Hot Air Gun is turned on
		int16_t		t_diff		= 0;						// The difference between temperature and preset temperature
};

//---------------------- The Fail mode: display error message --------------------
class MFAIL : public MODE {
	public:
		typedef enum e_msg_type { FAIL_CONNECTIVITY, FAIL_MSG } tFAIL;
		MFAIL(HW *pCore) : MODE(pCore)						{ }
		virtual void	init(void);
		virtual MODE*	loop(void);
		void			setMessage(t_msg_id line1, t_msg_id line2);
		void			setMessage(tFAIL type)				{ this->type = type;	}
	private:
		t_msg_id		line1 			= MSG_NONE;
		t_msg_id		line2 			= MSG_NONE;
		tFAIL			type			= FAIL_CONNECTIVITY;
};

//---------------------- The About dialog mode. Show about message ---------------
class MABOUT : public MODE {
	public:
		MABOUT(HW *pCore) : MODE(pCore)						{ }
		virtual void	init(void);
		virtual MODE*	loop(void);
};

//---------------------- The Tweezers Debug mode: display internal parameters ----
class MDEBUG : public MODE {
	public:
		MDEBUG(HW *pCore) : MODE(pCore)						{ }
		virtual void	init(void);
		virtual MODE*	loop(void);
	private:
		bool			dev_is_on 		= false;			// Flag indicating the device is powered on
		const uint16_t	hot_gun_power	= 2;				// The Applied Hot Air Gun power (just to check the devie)
		const uint16_t	min_tweez_power	= 10;
		const uint16_t	max_tweez_power = 200;
		const uint32_t	check_tweez_to	= 90000;			// The period to check the tweezers temperature
};

#endif
