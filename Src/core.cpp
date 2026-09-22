/*
 * core.cpp
 *
 *  Hardware configuration:
 *  TIM1:
 *  A8	- TIM1_CH1, AC zero signal read - Timer reset signal
 *  A9	- TIM1_CH2, Hot Air Gun power [0-max_gun_pwr]
 *  TIM2:
 *  A0	- TIM2_CH1, FAN power or Tweezers power [0-1999]
 *  B10	- TIM2_CH3,	LCD backlight [0-1999]
 *  TIM3:
 *  B4	- TIM3_CH1, ENC_L
 *  B5	- TIM3_CH2, ENC_R
 *
 *
 *  How the Hot Air Gun powered.
 *  The AC power outlet frequency is 50 Hz in Europe and 60 Hz in USA. The AC signal after full diode rectifier has a positive half-period shapes
 *  its frequency is 100 Hz or 120 Hz respectively. This signal is coming from the power board as a AC_ZERO interrupts. The TIM1 timer is clocked
 *  by the internal clock and has period of 25.600 mS (the timer counter ticks from 0 to 255). The period between two AC_ZERO signals is 10 mS (8.33 ms in USA).
 *  The AC_ZERO signal resets the TIM1 counter, so the TIM1 counter never reached its period for sure.
 *  To avoid the AC source distortion, the TRIAC in the power board are managed to propagate whole halh-period shape or to completely block it.
 *  To manage the power of the Hot Air Gun, the controller uses a means of bundle of 120 half-period shapes (1.2 secs in Europe or 1 sec is USA).
 *  The number 120 has a multiple dividers: 2,3,4,5,6, etc. so it easy to distribute many active half-period shapes from 0 to 120 in 120-element array evenly.
 *  120 half-period shapes define a period of checking the Hot Air Gun temperature and calculate the required power to be applied.
 *  As soon as required power calculated (0-120 half-period shapes) this number of half-period shapes is distributed into 120 elements DMA buffer (gun_pwr)
 *  by the calculateGunPowerData() routine. This DMA buffer then sent to the TIM1->CHANNEL2 to manage the PWM signal to activate the TRIAC on the Power board.
 *  Active pulse encoded as 70 (TIM1 ticks) and inactive pulse is encoded as 0.
 *  As mentioned before, the TIM1 timer counts from 0 to 100 (or 83 in USA) before AC_ZERO interrupt reset the timer and the 70-ticks long active pulse activates
 *  the TRIAC at the AV wave beginning and goes down before the sine pulse ends, but the TRIAC keeps open until the AC sine wave goes through the zero,
 *  so the half-period shape will propagate to the Hot Air Gun completely.
 *  From the other side, the TIM1 PWM channel goes down at 70-th timer tick and power-off the Hot Air Gun at the beginning of the next half-period shape for sure.
 */

#include "main.h"
#include "hw.h"
#include "mode.h"

extern TIM_HandleTypeDef	htim1;
extern TIM_HandleTypeDef 	htim2;

#define MAX_GUN_POWER		(120)

volatile static uint8_t		gun_pwr[MAX_GUN_POWER*2] = {0};	// The HOT GUN power PWM buffer
volatile static uint32_t	gun_half_period_ms		 = 0;	// The time when the Hot Air Gun DMA buffer was sent
volatile static uint32_t	gtim_last_ms	= 0;			// Time when the last AC_ZERO interrupt received

static uint32_t t_check_ms		= 0;						// Time when to check the temperature of Hot Air Gun or Tweezers
static uint32_t	check_sw		= 0;						// Time when check the Auto switch (reed switch) status (ms)
const static	uint32_t	check_sw_period = 100;			// IRON switches check period, ms

static HW				core;
static MWORK			work(&core);
static MCHECK			check(&core);
static MCALIB_MANUAL	calib(&core);
static MTPID			pid_tune(&core);
static MDEBUG			debug(&core);
static MFAIL			fail(&core);
static MABOUT			about(&core);
static MMENU			menu(&core, &calib, &pid_tune, &about);
static MODE*			pMode = &check;						// Check the device connected first

// Calculates the PWM value data for TIMER to supply power to the heater
// Each AC-outlet peak (100 Hz in Russia and 60 Hz in US) resets the timer and make the timer to supply power
// The PWM values can be in two states: supply power for the half-period (peak) or not
static void calculateGunPowerData(volatile uint8_t *data, uint8_t max_power, uint8_t pwr) {
	const uint8_t active_pulse = 70;
	uint8_t on	= active_pulse;
	uint8_t off = 0;
	if (pwr > (max_power >> 1)) {							// In case the pwr is greater than half of maximum power, calculate positions of "empty" peaks
		if (pwr > max_power) pwr = max_power;
		on	= 0;
		off = active_pulse;
		pwr = max_power - pwr;
	}
	if (pwr == 0) {											// No power supplied at all, empty all PWM slots
		for (uint8_t i = 0; i < max_power; ++i)
			data[i] = off;
		return;
	}
	uint8_t slots	= max_power / pwr;						// Number of PWM slots per each "powered" peak (0 .. max_power/2)
	uint8_t remain	= max_power % pwr;						// The division remainder
	uint8_t pos		= slots >> 1;							// Put the "powered" peak in to the center of the slot
	int8_t extra	= 0;									// Extra position remainder (extra/pwr)
	for (uint8_t i = 0; i < max_power; ++i) {
		if (i < pos) {
			data[i] = off;
		} else {
			data[i] = on;
			pos += slots;
			extra += remain;
			if (extra + (remain>>1) >= pwr) {
				++pos;
				extra -= pwr;
			}
		}
	}
}

static void powerOffGun(void) {
	for (uint16_t i = 0; i < MAX_GUN_POWER * 2; ++i) {
		gun_pwr[i] = 0;
	}
}

extern "C" void setup(void) {
	HW_ERROR err = core.init();
	work.setup(&work, &menu);								// return_mode, long_mode
	check.setup(&work, &menu);
	calib.setup(&work, &work);
	pid_tune.setup(&work, &work);
	debug.setup(&work, &work);
	fail.setup(&work, &menu);
	about.setup(&work, &debug);
	menu.setup(&work, &work);

	HAL_TIM_IC_Start_IT(&htim1, TIM_CHANNEL_1);				// The Input Capture mode on channel 1 of TIM1. The AC_ZERO signal resets the timer
	HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_2, (const uint32_t*)gun_pwr, MAX_GUN_POWER*2); // PWM signal of Hot Air Gun
	HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);				// PWM signal to manage Hot Air Gun Fan or Tweezers heater
	uint16_t br = core.cfg.lcdBrightness();
	core.dspl.set(br);

	if (HW_ERR_EEPROM == err) {
		pMode = &fail;
		fail.setMessage(MSG_ERROR, MSG_EEPROM_READ);
	} else {
		fail.setMessage(MFAIL::FAIL_CONNECTIVITY);
	}
	pMode->init();
}

extern "C" void loop(void) {
	if (HAL_GetTick() > check_sw) {
		check_sw = HAL_GetTick() + check_sw_period;
		GPIO_PinState pin = HAL_GPIO_ReadPin(REED_SW_GPIO_Port, REED_SW_Pin);
		core.sw.update((GPIO_PIN_SET == pin)?100:0);		// Switch active when the Hot Air Gun handle is off-hook or Auto switch is opened
	}

	if (t_check_ms > 0 && HAL_GetTick() >= t_check_ms) {
		t_check_ms = 0;										// Stop checking the temperature
		core.sensor_status = core.term.status();			// Read the device temperature
		if (MAX31855_OK == core.sensor_status) {
			core.t_last_ms = HAL_GetTick();
			if (core.cfg.device() == d_gun) {
				core.gun.setConnected(true);
				core.gun.updateTemp(core.term.temperature());
			}
		} else if (core.cfg.device() == d_gun) {
			core.gun.setConnected(false);
		}
	}

	if (HAL_GetTick() > gtim_last_ms + 1000) {
		core.gtim_period.reset(0);
	}

	if (core.sensor_status != MAX31855_OK && pMode != &fail && pMode != &debug && pMode != &menu) {
		pMode = &fail;
		fail.setMessage(MFAIL::FAIL_CONNECTIVITY);
		pMode->init();
	}

	MODE* new_mode = pMode->returnToMain();
	if (new_mode && new_mode != pMode) {
		if (core.cfg.device() == d_gun) {
			core.gun.switchPower(false);
		} else {
			core.tweezers.switchPower(false);
			TIM2->CCR1	= 0;								// Switch-off the Tweezers power immediately
		}
		powerOffGun();
		pMode->clean();
		pMode = new_mode;
		pMode->init();
		return;
	}
	new_mode = pMode->loop();
	if (new_mode != pMode) {
		if (new_mode == 0) new_mode = &fail;				// Mode Failed
		if (core.cfg.device() == d_gun) {
			core.gun.switchPower(false);
		} else {
			core.tweezers.switchPower(false);
			TIM2->CCR1	= 0;								// Switch-off the Tweezers power immediately
		}
		powerOffGun();
		pMode->clean();
		pMode = new_mode;
		pMode->init();
	}

	// Adjust display brightness
	if (core.dspl.BRGT::adjust()) {
		HAL_Delay(5);
	}
}

extern "C" void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim) {
	if (htim->Instance == TIM1) {
		gtim_last_ms = HAL_GetTick();
		core.gtim_period.update(htim->Instance->CCR1);
	}
}

/*
 * IRQ handler of TIM1.
 * Used to power the Hot Air Gun
 */
void HAL_TIM_PWM_PulseFinishedHalfCpltCallback(TIM_HandleTypeDef *htim) {
	if (htim->Instance != TIM1) return;
	uint32_t n = HAL_GetTick();
	uint32_t period 	= n - gun_half_period_ms;			// The period between DMA buffer sent
	gun_half_period_ms	= n;
	t_check_ms			= n + period - 5;					// Calculate the time when update the device temperature
	if (core.cfg.device() == d_gun) {
		uint16_t gun_power	= 0;							// First half of the pwr_buffer has been sent, calculate next buffer values
		if (core.gtim_period.read() > 50)
			gun_power	= core.gun.power();
		if (gun_power) {
			calculateGunPowerData(&gun_pwr[MAX_GUN_POWER], MAX_GUN_POWER, gun_power);
		} else {
			powerOffGun();
		}
	} else if (core.cfg.device() == d_tweez) {
		uint16_t tweez_power = core.tweezers.power(core.term.temperature());
		TIM2->CCR1 = tweez_power;
		powerOffGun();
	}
}

/*
 * IRQ handler of TIM1. Used to power the Hot Air Gun or Hot Tweezers
 */
void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim) {
	if (htim->Instance != TIM1) return;
	uint32_t n = HAL_GetTick();
	uint32_t period 	= n - gun_half_period_ms;			// The period between DMA buffer sent
	gun_half_period_ms	= n;
	t_check_ms			= n + period - 5;					// Calculate the time when update the device temperature
	if (core.cfg.device() == d_gun) {
		uint16_t gun_power	= 0;							// Second half of the pwr_buffer has been sent, calculate next buffer values
		if (core.gtim_period.read() > 50)
			gun_power	= core.gun.power();
		if (gun_power) {
			calculateGunPowerData(gun_pwr, MAX_GUN_POWER, gun_power);
		} else {
			powerOffGun();
		}
	} else if (core.cfg.device() == d_tweez) {
		uint16_t tweez_power = core.tweezers.power(core.term.temperature());
		TIM2->CCR1 = tweez_power;
		powerOffGun();
	}
}
