/*
 * cfgtypes.h
 *
 *  2025 OCT 14
 *  	Ported from UNITED controller source code, tailored to the new hardware
 */

#ifndef CFGTYPES_H_
#define CFGTYPES_H_
#include "vars.h"

typedef enum { d_tweez = 0, d_gun = 1, d_unknown } tDevice;

typedef enum { p_gun = 1, p_celsius = 2, p_enc_cw = 4, p_step_5 = 8, p_fast_chill = 16, p_buzz = 32, p_fan_24v = 64, p_autocheck = 128 } tParams;

typedef struct s_config RECORD;
struct s_config {
	uint32_t	ID;										// The configuration record ID
	uint16_t	crc;									// The checksum
	uint16_t	twez_temp;								// The Tweezers preset temperature in degrees (Celsius or Fahrenheit)
	uint16_t	gun_temp;								// The Hot Air Gun preset temperature in degrees (Celsius or Fahrenheit)
	uint8_t		fan_speed;								// The Hot Air Gun Fan speed, percent (0-100)
	uint8_t		brightness;								// The display brightness (divided by 8)
	uint16_t	bit_params;								// The Bit mapped parameters, see tParams
};

typedef enum { PID_KP = 0, PID_KI, PID_KD } tPID_PARAM;

typedef struct s_calib CALIBRATION;
struct s_calib {
	uint16_t	ID;										// The configuration record ID
	uint16_t	crc;									// The checksum
	uint16_t 	tweezers_calib[4];						// The calibration temperature in reference points of Tweezers
	uint16_t	gun_calib[4];							// The calibration temperature in reference points of Hot Air Gun
	uint16_t	PID_tweez[3];							// The PID parameters of the TWEEZERS
	uint16_t	PID_gun[3];								// The PID parameters of the Hot Air Gun
};

#endif
