/*
 * msg.h
 *
 *  2025 OCT 11
 *  	Ported from UNITED controller source code, tailored to the new hardware
 */

#ifndef MSG_H_
#define MSG_H_

#include <stdint.h>

typedef enum e_msg {
	MSG_MENU_MAIN = 0,
	MSG_NONE = 15, MSG_ON, MSG_OFF, MSG_CW, MSG_CCW, MSG_SELECT_DEV, MSG_TUNE_PID, MSG_HOTGUN, MSG_TWEEZERS,
	MSG_SAVE, MSG_YES, MSG_NO, MSG_FAILED, MSG_OK, MSG_ERROR, MSG_EEPROM_READ, MSG_DEVICE_ERROR, MSG_NOT_CONNECTED, MSG_WRONG_CONNECTION,
	MSG_SPI_ERROR, MSG_DATA_READ_FAILURE, MSG_SENSOR_FAILED, MSG_CHECKING_DEVICE, MSG_CALIBRATE, MSG_PID_KP, MSG_PID_KI, MSG_PID_KD,
	MSG_LAST
} t_msg_id;

class MSG {
	public:
		MSG()											{ }
		const char*		msg(t_msg_id id);
		uint8_t			menuSize(t_msg_id id);
	protected:
		const char*		message[MSG_LAST] = {
				// MAIN MENU
				"Main Menu",							// Title is the first element of each menu
				"Device",
				"units",
				"buzzer",
				"encoder",
				"temp. step",
				"brightness",
				"Auto check",
				"fast chill",
				"fan voltage",
				"calibrate",
				"tune PID",
				"about",								// Change MSG_ABOUT if new item menu inserted
				"quit",
				"save",
				// SINGLE MESSAGE STRINGS
				"",
				"ON ",
				"OFF",
				"cw",
				"ccw",
				"Select Device",
				"Tune PID",
				"Hot Gun",
				"Tweezers",
				"Save?",
				"Yes",
				"No",
				"Failed",
				"OK",
				"Error",
				"EEPROM read",
				"Device error",
				"Not connected",
				"Wrong connection",
				"SPI error",
				"Data read fail",
				"Sensor failed",
				"Checking...",
				"Calib.",
				"Kp",
				"Ki",
				"Kd"
		};
};

#endif
