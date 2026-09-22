
#ifndef _LCD_H
#define _LCD_H

/*
+++   Nima Askari
+++   www.github.com/NimaLTD
+++   www.instagram.com/github.NimaLTD
+++   Version: 1.1.0
*/


#include <stdbool.h>
#include "main.h"
#ifdef __cplusplus
extern "C" {
#endif

void	LCD_Init(void);
void	LCD_Clear(void);
void 	LCD_Home(void);
void	LCD_DisplayOn(bool on);
void	LCD_SetCursor(uint8_t col, uint8_t row);
void	LCD_CursorOn(bool on);
void	LCD_BlinkOn(bool on);
void	LCD_ScrollLeft(void);
void	LCD_ScrollRight(void);
void 	LCD_LeftToRight(void);
void 	LCD_RightToLeft(void);
void 	LCD_AutoScroll(bool on);
void	LCD_CreateChar(uint8_t location, uint8_t data[]);
void	LCD_Puts(uint8_t x, uint8_t y, const char* str);
void	LCD_PutCustom(uint8_t x, uint8_t y, uint8_t location);
void	LCD_Put(uint8_t Data);
void	LCD_Write(const char* str);

#ifdef __cplusplus
}

class LCD {
	public:
		LCD(void)						{ }
		void	init(void)				{ LCD_Init();						}
		void	on(bool on)				{ LCD_DisplayOn(on);				}
		void	clear(void)				{ LCD_Clear();						}
		void	home(void)				{ LCD_Home();						}
		void	displayOn(bool on)		{ LCD_DisplayOn(on);				}
		void	setCursor(uint8_t x, uint8_t y)
										{ LCD_SetCursor(x, y);				}
		void	setCursor(bool on)		{ LCD_CursorOn(on);					}
		void	blinkOn(bool on)		{ LCD_BlinkOn(on);					}
		void	scrollLeft(void)		{ LCD_ScrollLeft();					}
		void	scrollRight(void)		{ LCD_ScrollRight();				}
		void 	leftToRight(void)		{ LCD_LeftToRight();				}
		void 	rightToLeft(void)		{ LCD_RightToLeft();				}
		void 	autoScroll(bool on)		{ LCD_AutoScroll(on);				}
		void	createChar(uint8_t location, uint8_t data[])
										{ LCD_CreateChar(location, data);	}
		void	puts(uint8_t x, uint8_t y, const char* str)
										{ LCD_Puts(x, y, str);				}
		void	putCustom(uint8_t x, uint8_t y, uint8_t location)
										{ LCD_PutCustom(x, y, location);	}
		void	put(uint8_t Data)		{ LCD_Put(Data);					}
		void	write(const char* str)	{ LCD_Write(str);					}
};

#endif

#endif

