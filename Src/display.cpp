/*
 * display.cpp
 *
 *  2025 SEP 30
 *  	Ported from Lab power supply source code, tailored to the new hardware
 */

#include <string.h>
#include "display.h"
#include "msg.h"

bool BRGT::adjust(void) {
	const uint8_t b_step = 4;
	if (brightness > LCD_TIM.Instance->CCR3) {
		if (brightness - LCD_TIM.Instance->CCR3 > b_step) {
			LCD_TIM.Instance->CCR3 += b_step;
		} else {
			++LCD_TIM.Instance->CCR3;
		}
		return true;
	} else if (brightness < LCD_TIM.Instance->CCR3) {
		if (LCD_TIM.Instance->CCR3 - brightness > b_step) {
			LCD_TIM.Instance->CCR3 -= b_step;
		} else {
			--LCD_TIM.Instance->CCR3;
		}
		return true;
	}
	return false;
}

void DSPL::init(void) {
	LCD::init();
	clear();
	BRGT::stop();											// To Fix bug full backlight at startup; stm32F103ret6 only
	BRGT::start();
	BRGT::set(0);											// Minimum brightness
	BRGT::on();												// Apply brightness value to the timer counter
    for (uint8_t i = 0; i < 7; ++i)
    	createChar(i+1, (uint8_t *)custom_symbols[i]);
}

void DSPL::drawTitle(t_msg_id title_id) {
	const char *t = MSG::msg(title_id);
	if (t) puts(0, 0, t);
}

/*
 * Data is a 4 digits array:
 * The preset temperature
 * The fan speed, %
 * The device temperature
 * The power supplied, %
 * The internal max31855 temperature
 */
void DSPL::workShow(int16_t data[], tDevice dev, bool isCelsius, bool is_on, bool fan_mode) {
	blinkOn(false);
	char buff[6];
	putCustom(0, 0, SYM_TEMP);								// Draw the preset temperature
	valueStr(buff, data[0], 3, false);
	write(buff);
	put(SYM_DEGREE);
	put(isCelsius?'C':'F');
	putCustom(15, 0, is_on?SYM_ON: ' ');					// Draw ON/OFF sign
	valueStr(buff, data[2], 3, false);						// Draw device temperature
	puts(0, 1, buff);
	put(SYM_DEGREE);
	putCustom(7, 1, SYM_POWER);								// Draw power supplied
	if (data[3] > 99) data[3] = 99;							// Make sure the power value fits into 2 digits
	valueStr(buff, data[3], 2, false);
	write(buff);
	put('%');
	valueStr(buff, data[4], 2, false);						// Draw ambient temperature
	puts(13, 1, buff);
	put(SYM_DEGREE);
	if (dev == d_gun) {										// Draw fan speed
		putCustom(7, 0, SYM_FAN);
		if (data[1] > 99) data[1] = 99;						// Make sure the fan speed fits into 2 digits
		valueStr(buff, data[1], 2, false);
		write(buff);
		put('%');
		if (fan_mode) {
			setCursor(9, 0);
			blinkOn(true);
		}
	}
}

void DSPL::menuShow(t_msg_id menu, uint8_t item, const char *item_value, bool modify) {
	blinkOn(false);
	char line[17] = {0};									// The buffer for the second display line
	const char *s = MSG::msg((t_msg_id)(menu + item + 1));
	strncpy(line, s, 16);
	uint8_t value_x = strlen(s);
	if (item_value[0] != 0)
		line[value_x++] = ':';
	line[value_x]	= ' ';
	strncpy(&line[value_x+1], item_value, 15-value_x);
	uint8_t len = strlen(line);
	for (uint8_t i = len; i < 16; ++i)
		line[i] = ' ';
	puts(0, 1, line);
	if (modify) {
		setCursor(value_x + 2, 1);
		blinkOn(true);
	}
}

void DSPL::pidShow(uint8_t index, uint16_t value, int16_t t_diff, uint8_t pwr, uint16_t p_disp, bool on) {
	char buff[6];
	const char *s = MSG::msg((t_msg_id)(MSG_PID_KP+index));	// The PID coefficient name
	puts(0, 0, s);
	put('=');												// The PID coefficient value
	valueStr(buff, value, 4, false);
	write(buff);
	putCustom(9, 0, SYM_POWER);
	if (pwr > 99) pwr = 99;									// Ensure the power value requires 2 digits only
	valueStr(buff, pwr, 2, false);
	write(buff);
	put('%');
	putCustom(15, 0, on?SYM_ON: ' ');						// Draw ON/OFF sign
	putCustom(0, 1, SYM_DELTA);
	write("T=");
	if (t_diff < -9999)
		t_diff = -9999;
	else if (t_diff > 9999)
		t_diff = 9999;
	valueStr(buff, t_diff, 5, false);						// The temperature difference
	write(buff);
	setCursor(11, 1);
	write("D=");
	if (p_disp > 999) p_disp = 999;
	valueStr(buff, p_disp, 3, false);						// The power dispersion
	write(buff);
}

void DSPL::pidParam(uint16_t pid_k[], uint8_t data_index) {
	char buff[6];
	const char *s = MSG::msg(MSG_TUNE_PID);
	puts(0, 0, s);											// The title, 'Tune PID'
	s = MSG::msg((t_msg_id)(MSG_PID_KP+data_index));		// The PID coefficient name
	puts(0, 1, s);
	write(" = ");											// The PID coefficient value
	valueStr(buff, pid_k[data_index], 4, false);
	write(buff);
}

void DSPL::calibManualTuning(uint16_t ref_temp, int16_t delta, uint16_t int_temp, bool is_celsius, uint8_t power, bool ready, bool recently_updated) {
	char value[8];;
	valueStr(value, ref_temp, 4, false);
	puts(0, 0, value);
	put(SYM_DEGREE);
	put(is_celsius?'C':'F');
	setCursor(8, 0);
	put('[');
	valueStr(value, int_temp, 4, false);
	write(value);
	put(']');
	setCursor(15, 0); put(ready?'!':' ');
	valueStr(value, delta, 6, false);
	puts(0, 1, value);
	putCustom(8, 1, SYM_POWER);
	valueStr(value, power, 3, false);
	write(value); put('%');
	// The temperature updated flag
	if (recently_updated) {
		rotateStick(15, 1);
	}
}

void DSPL::calibManualSelect(tDevice dev, uint16_t ref_temp, uint16_t calib_temp, bool is_celsius, bool is_calibrated) {
	char value[8];
	const char *s = MSG::msg(MSG_CALIBRATE);
	puts(0, 0, s);
	put(' ');
	s = MSG::msg((dev == d_gun)?MSG_HOTGUN:MSG_TWEEZERS);
	write(s);
	valueStr(value, ref_temp, 3, false);					// Print reference temperature value
	puts(0, 1, value);
	put(SYM_DEGREE);
	put(is_celsius?'C':'F');
	valueStr(value, calib_temp, 4, false);
	setCursor(7, 1);
	put('[');
	write(value);
	put(']');
	setCursor(15, 1); put(is_calibrated?'*':' ');
}

void DSPL::showCalibrated(tDevice dev, bool ok) {
	const char *s = MSG::msg(MSG_CALIBRATE);
	puts(0, 0, s);
	put(' ');
	s = MSG::msg((dev == d_gun)?MSG_HOTGUN:MSG_TWEEZERS);
	write(s);
	const char *res = MSG::msg(ok?MSG_OK:MSG_FAILED);
	uint8_t pos = (16 - strlen(res)) >> 1;
	puts(pos, 1, res);
}

void DSPL::showDialog(t_msg_id msg, uint16_t pid_k[], uint8_t answer) {
	char buff[6];
	const char *s = MSG::msg(msg);
	puts(0, 0, s);
	write(" [");
	uint8_t len = strlen(s) + 2;
	s = MSG::msg((answer&1)?MSG_YES:MSG_NO);
	write(s);
	put(']');
	len += strlen(s) + 1;
	for (uint8_t i = 0; i < 16-len; ++i)
		put(' ');
	for (uint8_t k = 0; k < 3; ++k) {
		valueStr(buff, pid_k[k], 4, false);
		puts(k*5, 1, buff);
	}
}

/*
 * Data is a 4 digits array:
 *	Tweezers power or Fan speed
 *	Device temperature
 *	The internal max31855 temperature
 *	GUN Timer period determined by AC_ZERO signal
 *	When the temperature was updated last time, s
 */
void DSPL::debugShow(int16_t data[], bool gtim_ok, bool recently_updated, tDevice dev, bool is_on, bool fan_is_on) {
	char buff[6];
	// The device temperature
	valueStr(buff, data[1]/10, 3, false);					// Integer part of temperature
	puts(0, 0, buff);
	if (data[1] < 0)
		data[1] = ~(data[1]-1);								// data[1] *= -1
	valueStr(buff, data[1]%10, 2, true);					// One decimal character and decimal dot
	write(buff);
	put(SYM_DEGREE);
	// The internal temperature
	valueStr(buff, data[2]/10, 2, false);					// Integer part of internal temperature
	puts(8, 0, buff);
	if (data[2] < 0)
		data[2] = ~(data[2]-1);								// data[2] *= -1
	valueStr(buff, data[2]%10, 2, true);					// One decimal character and decimal dot
	write(buff);
	put(SYM_DEGREE);
	// The temperature updated flag
	if (recently_updated) {
		rotateStick(15, 1);
	}
	// Applied power
	valueStr(buff, data[0], 4, false);
	putCustom(0, 1, (dev == d_gun)?SYM_FAN:SYM_POWER);
	puts(1, 1, buff);
	put(' ');
	put(fan_is_on?'*':' ');
	// GUN Timer period
	valueStr(buff, data[3], 3, false);
	puts(8, 1, buff);
	write("ms");
	putCustom(15, 0, is_on?SYM_ON:' ');
}

void DSPL::rotateStick(uint8_t x, uint8_t y) {
	setCursor(x, y);
	put(stick[stick_indx++]);
	stick_indx &= 3;										// Stick_indx is in [0..3]
}

void DSPL::errorMessage(const char *line1, const char *line2) {
	puts(0, 0, line1);
	puts(0, 1, line2);
}

void DSPL::showVersion(void) {
	const char *msg = "Ctrl v.";
	puts(0, 0, msg);
	write((char *)FW_VERSION);
	puts(0, 1, (char *)__DATE__);
}

void DSPL::valueStr(char *str, int16_t value, uint8_t dgts, bool dot) {
	bool negative = false;
	if (value < 0) {
		dot 	= false;									// Do not write decimal dot before negative value
		negative = true;
		value = ~(value-1);									// value *= -1
	}
	uint8_t pos = dgts-1;
	str[pos+1] = '\0';
	for (uint8_t d = 0; d < dgts; ++d) {
		uint8_t v = value % 10;
		str[pos] = v + '0';
		value /= 10;
		if (value == 0 && !dot)
			break;
		if (pos > 0) --pos;
	}
	if (dot) {
		str[0] = '.';
		return;
	} else if (negative) {
		if (pos > 0) --pos;
		str[pos] = '-';
	}
	for (uint8_t i = 0; i < pos; ++i)
		str[i] = ' ';
}

void DSPL::drawSecondLine(const char *line2) {
	uint8_t len = strlen(line2);
	puts(0, 1, line2);
	for (uint8_t i = len; i < 16; ++i) {
		put(' ');
	}
}
