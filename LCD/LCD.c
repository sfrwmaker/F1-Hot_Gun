
#include "LCD.h"
#include "LCD_Config.h"
#if _LCD_USE_FREERTOS==1
#include "cmsis_os.h"
#endif

typedef struct {
	uint8_t display_control;			// LCD_DISPLAYCONTROL_08; Display On/Off, Cursor On/Off, Blink On/Off
	uint8_t display_mode;				// LCD_ENTRYMODESET_04; Increment/Decrement, Shift left/right
	uint8_t x;
	uint8_t y;
  
} LCD_Options_t;

/* Private functions */
static void LCD_Cmd(uint8_t cmd);
static void LCD_Cmd4bit(uint8_t cmd);
static void LCD_Data(uint8_t data);

volatile static LCD_Options_t LCD_Opts;

typedef enum {
	LCD_CLEARDISPLAY_01			= 0x01,
	LCD_RETURNHOME_02			= 0x02,
	LCD_ENTRYMODESET_04			= 0x04,
	LCD_DISPLAYCONTROL_08		= 0x08,
	LCD_CURSORSHIFT_10			= 0x10,
	LCD_FUNCTIONSET_20			= 0x20,
	LCD_SETCGRAMADDR_40			= 0x40,
	LCD_SETDDRAMADDR_80			= 0x80,
	// Attributes for EntryModeSet
	EMS_ENTRYRIGHT_00			= 0x00,
	EMS_ENTRYLEFT_02			= 0x02,
	EMS_ENTRYSHIFTDECREMENT_00	= 0x00,
	EMS_ENTRYSHIFTINCREMENT_01	= 0x01,
	// Attributes for DisplayModeControl
	DMC_DISPLAY_OFF_00			= 0x00,
	DMC_DISPLAYON_04			= 0x04,
	DMC_CURSOROFF_00			= 0x00,
	DMC_CURSORON_02				= 0x02,
	DMC_BLINKOFF_00				= 0x00,
	DMC_BLINKON_01				= 0x01,
	// Attributes for CursorDisplayShift
	CDS_DISPLAYMOVE_08			= 0x08,
	CDS_MOVELEFT_00				= 0x00,
	CDS_MOVERIGHT_04			= 0x04,
	// Attributes for FunctionSet
	FS_4BITMODE_00				= 0x00,
	FS_8BITMODE_10				= 0x10,
	FS_1LINE_00					= 0x00,
	FS_2LINE_08					= 0x08,
	FS_5x8DOTS_00				= 0x00,
	FS_5x10DOTS_04				= 0x04,
} LCD_CMD;


static void LCD_Delay_us(uint16_t us) {
	uint32_t div = (SysTick->LOAD+1)/1000;
	uint32_t start_micros = HAL_GetTick()*1000 + (1000 - SysTick->VAL/div);
	while ((HAL_GetTick()*1000 + (1000-SysTick->VAL/div)-start_micros < us));
}

static void LCD_Delay_ms(uint8_t  ms) {
	#if _LCD_USE_FREERTOS==1
	osDelay(ms);
  	#else
	HAL_Delay(ms);
	#endif
}

void LCD_Init(void) {
	while (HAL_GetTick() < 200)
		LCD_Delay_ms(1);
	/* Set cursor pointer to beginning for LCD */
	LCD_Opts.x = 0;
	LCD_Opts.y = 0;
	/* Try to set 4bit mode */
	LCD_Cmd4bit(0x03);
	LCD_Delay_ms(5);
	/* Second try */
	LCD_Cmd4bit(0x03);
	LCD_Delay_ms(5);
	/* Third goo! */
	LCD_Cmd4bit(0x03);
	LCD_Delay_ms(5);
	/* Set 4-bit interface */
	LCD_Cmd4bit(0x02);
	LCD_Delay_ms(5);
	/* Set # lines, font size, etc. */
	uint8_t df = FS_4BITMODE_00 | FS_5x8DOTS_00 | FS_1LINE_00;
	if (_LCD_ROWS > 1)
		df |= FS_2LINE_08;
	LCD_Cmd(LCD_FUNCTIONSET_20 | df);
	/* Turn the display on with no cursor or blinking default */
	LCD_Opts.display_control = DMC_DISPLAYON_04 | DMC_CURSOROFF_00 | DMC_BLINKOFF_00;
	LCD_DisplayOn(true);
	LCD_Clear();
	/* Default font directions */
	LCD_Opts.display_mode = EMS_ENTRYLEFT_02 | EMS_ENTRYSHIFTDECREMENT_00;
	LCD_Cmd(LCD_ENTRYMODESET_04 | LCD_Opts.display_mode);
	LCD_Delay_ms(5);
}

void LCD_Clear(void) {
	LCD_Cmd(LCD_CLEARDISPLAY_01);
	LCD_Delay_ms(5);
}

void LCD_Home(void) {
	LCD_Cmd(LCD_RETURNHOME_02);
	LCD_Delay_ms(5);
}

void LCD_DisplayOn(bool on) {
	if (on) {
		LCD_Opts.display_control |= DMC_DISPLAYON_04;
	} else {
		LCD_Opts.display_control &= ~DMC_DISPLAYON_04;
	}
	LCD_Cmd(LCD_DISPLAYCONTROL_08 | LCD_Opts.display_control);
}

void LCD_SetCursor(uint8_t col, uint8_t row) {
	uint8_t row_offsets[] = {0x00, 0x40, 0x14, 0x54};
	if (row >= _LCD_ROWS)
		row = 0;
	LCD_Opts.x = col;
	LCD_Opts.y = row;
	LCD_Cmd(LCD_SETDDRAMADDR_80 | (col + row_offsets[row]));
	LCD_Delay_ms(2);
}

void LCD_CursorOn(bool on) {
	if (on) {
		LCD_Opts.display_control |= DMC_CURSORON_02;
	} else {
		LCD_Opts.display_control &= ~DMC_CURSORON_02;
	}
	LCD_Cmd(LCD_DISPLAYCONTROL_08 | LCD_Opts.display_control);
}

void LCD_BlinkOn(bool on) {
	if (on) {
		LCD_Opts.display_control |= DMC_BLINKON_01;
	} else {
		LCD_Opts.display_control &= ~DMC_BLINKON_01;
	}
	LCD_Cmd(LCD_DISPLAYCONTROL_08 | LCD_Opts.display_control);
}

void LCD_ScrollLeft(void) {
	LCD_Cmd(LCD_CURSORSHIFT_10 | CDS_DISPLAYMOVE_08 | CDS_MOVELEFT_00);
}

void LCD_ScrollRight(void) {
	LCD_Cmd(LCD_CURSORSHIFT_10 | CDS_DISPLAYMOVE_08 | CDS_MOVERIGHT_04);
}

void LCD_LeftToRight(void) {
	LCD_Opts.display_mode |= EMS_ENTRYLEFT_02;
	LCD_Cmd(LCD_ENTRYMODESET_04 | LCD_Opts.display_mode);
}

void LCD_RightToLeft(void) {
	LCD_Opts.display_mode &= ~EMS_ENTRYLEFT_02;
	LCD_Cmd(LCD_ENTRYMODESET_04 | LCD_Opts.display_mode);
}

void LCD_AutoScroll(bool on) {
	if (on)
		LCD_Opts.display_mode |= EMS_ENTRYSHIFTINCREMENT_01;
	else
		LCD_Opts.display_mode &= ~EMS_ENTRYSHIFTINCREMENT_01;
	LCD_Cmd(LCD_ENTRYMODESET_04 | LCD_Opts.display_mode);
}

void LCD_CreateChar(uint8_t location, uint8_t data[]) {
	uint8_t i;
	location &= 0x07;											// We have 8 locations available for custom characters
	LCD_Cmd(LCD_SETCGRAMADDR_40 | (location << 3));

	for (i = 0; i < 8; i++) {
		LCD_Data(data[i]);
	}
}

void LCD_Write(const char* str) {
	while (*str) {
		if (LCD_Opts.x >= _LCD_COLS) {
			LCD_Opts.x = 0;
			LCD_Opts.y++;
			LCD_SetCursor(LCD_Opts.x, LCD_Opts.y);
		}
		if (*str == '\n') {
			LCD_Opts.y++;
			LCD_SetCursor(LCD_Opts.x, LCD_Opts.y);
		} else if (*str == '\r') {
			LCD_SetCursor(0, LCD_Opts.y);
		} else {
			LCD_Data(*str);
			LCD_Opts.x++;
		}
		str++;
	}
}

void LCD_Puts(uint8_t x, uint8_t y, const char* str) {
	LCD_SetCursor(x, y);

	LCD_Write(str);
}

void LCD_PutCustom(uint8_t x, uint8_t y, uint8_t location) {
	LCD_SetCursor(x, y);
	LCD_Data(location);
}

void LCD_Put(uint8_t Data) {
	LCD_Data(Data);
}

static void LCD_Cmd(uint8_t cmd) {
	HAL_GPIO_WritePin(LCD_RS_GPIO_Port, LCD_RS_Pin, GPIO_PIN_RESET);
	LCD_Cmd4bit(cmd >> 4);
	LCD_Cmd4bit(cmd & 0x0F);
}

static void LCD_Data(uint8_t data) {
	HAL_GPIO_WritePin(LCD_RS_GPIO_Port, LCD_RS_Pin, GPIO_PIN_SET);
	LCD_Cmd4bit(data >> 4);
	LCD_Cmd4bit(data & 0x0F);
}

static void LCD_Cmd4bit(uint8_t cmd) {
	HAL_GPIO_WritePin(LCD_D7_GPIO_Port, LCD_D7_Pin, (GPIO_PinState)(cmd & 0x08));
	HAL_GPIO_WritePin(LCD_D6_GPIO_Port, LCD_D6_Pin, (GPIO_PinState)(cmd & 0x04));
	HAL_GPIO_WritePin(LCD_D5_GPIO_Port, LCD_D5_Pin, (GPIO_PinState)(cmd & 0x02));
	HAL_GPIO_WritePin(LCD_D4_GPIO_Port, LCD_D4_Pin, (GPIO_PinState)(cmd & 0x01));
	HAL_GPIO_WritePin(LCD_EN_GPIO_Port, LCD_EN_Pin, GPIO_PIN_SET);
	LCD_Delay_us(150);
	HAL_GPIO_WritePin(LCD_EN_GPIO_Port, LCD_EN_Pin, GPIO_PIN_RESET);
	LCD_Delay_us(150);
}
