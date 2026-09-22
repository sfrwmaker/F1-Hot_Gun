/*
 * buzzer.h
 *
 */

#ifndef BUZZER_H_
#define BUZZER_H_

#ifndef __BUZZ_H
#define __BUZZ_H
#include "main.h"

// The Buzzer control class
#define BUZZER_TIM		htim4
extern TIM_HandleTypeDef BUZZER_TIM;

class BUZZER {
	public:
		BUZZER(void)										{ }
		void		activate(bool e);
		void		lowBeep(void);
		void		shortBeep(void);
		void		doubleBeep(void);
		void		failedBeep(void);
	private:
		void		playTone(uint16_t period_mks, uint16_t duration_ms);
		bool		enabled = true;
};

#endif

#endif
