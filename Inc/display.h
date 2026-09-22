/*
 * display.h
 *
 *  2025 SEP 30
 *  	Ported from Lab power supply source code, tailored to the new hardware
 */

#ifndef DISPLAY_H_
#define DISPLAY_H_

#include "LCD.h"
#include "eeprom.h"
#include "msg.h"

// TFT brightness control class
#define LCD_TIM		htim2
extern TIM_HandleTypeDef LCD_TIM;

class BRGT {
	public:
					BRGT(void)								{ }
		void		start(void)								{ HAL_TIM_PWM_Start(&LCD_TIM, TIM_CHANNEL_3);	}
		void		stop(void)								{ HAL_TIM_PWM_Stop(&LCD_TIM,  TIM_CHANNEL_3);	}
		uint16_t	get(void)								{ return LCD_TIM.Instance->CCR3;				}
		void		off(void)								{ LCD_TIM.Instance->CCR4 = 0;					}
		void		dim(uint16_t br)						{ LCD_TIM.Instance->CCR4 = br;					}
		void		on(void)								{ LCD_TIM.Instance->CCR4 = brightness;			}
		void		set(uint16_t brightness)				{ this->brightness = brightness; 				}
		bool		adjust(void);
	private:
		uint16_t	brightness			= 0;				// Setup display brightness
};

class DSPL : public LCD, public BRGT, public MSG {
	public:
					DSPL(void)								{ }
		virtual		~DSPL()									{ }
		void		init(void);
		void		drawTitle(t_msg_id title_id);
		void		workShow(int16_t data[], tDevice dev, bool isCelsius, bool is_on, bool fan_mode);
		void		menuShow(t_msg_id menu, uint8_t item, const char *item_value, bool modify);
		void		pidShow(uint8_t index, uint16_t value, int16_t t_diff, uint8_t pwr, uint16_t p_disp, bool on);
		void		pidParam(uint16_t pid_k[], uint8_t data_index);
		void		calibManualTuning(uint16_t ref_temp, int16_t delta, uint16_t int_temp, bool is_celsius, uint8_t power, bool ready, bool recently_updated);
		void		calibManualSelect(tDevice dev, uint16_t ref_temp, uint16_t calib_temp, bool is_celsius, bool is_calibrated);
		void		showCalibrated(tDevice dev, bool ok);
		void		debugShow(int16_t data[], bool gtim_ok, bool recently_updated, tDevice dev, bool is_on, bool fan_is_on);
		void		showDialog(t_msg_id msg, uint16_t pid_k[], uint8_t answer);
		void		errorMessage(const char *line1, const char *line2);
		void		rotateStick(uint8_t x, uint8_t y);
		void		showVersion(void);
		void		drawSecondLine(const char *line2);
	private:
		void		valueStr(char *str, int16_t value, uint8_t dgts, bool dot);
		enum e_sym {SYM_DEGREE = 1, SYM_FAN, SYM_POWER, SYM_TEMP, SYM_ON, SYM_BACKSLASH, SYM_DELTA};
		uint8_t		stick_indx = 0;
		const char stick[4] = {'|', '/', '-', (char)SYM_BACKSLASH};
        const   uint8_t custom_symbols[7][8] = {
			  { 0b00110,									// Degree
				0b01001,
				0b01001,
				0b00110,
				0b00000,
				0b00000,
				0b00000,
				0b00000
			  },
			  { 0b00000,
				0b00000,
				0b01000,
				0b01011,
				0b00100,
				0b00100,
				0b01000,
				0b00000
			  },
			  { 0b00011,									// Power sign
				0b00110,
				0b01100,
				0b11111,
				0b00110,
				0b01100,
				0b01000,
				0b10000
			  },
			  { 0b00100,									// Temperature sign
				0b01010,
				0b01010,
				0b01110,
				0b01110,
				0b11111,
				0b11111,
				0b01110
			  },
			  { 0b00100,									// 'ON' sign
				0b10101,
				0b10101,
				0b10001,
				0b01110,
				0b00000,
				0b00000,
				0b00000
			  },
			  { 0b00000,									// backslash sign
				0b00000,
				0b10000,
				0b01000,
				0b00100,
				0b00010,
				0b00001,
				0b00000
			  },
			  { 0b00000,									// delta sign
				0b00000,
				0b00000,
				0b00100,
				0b01010,
				0b01010,
				0b10001,
				0b11111
			  }
			};
};


#endif
