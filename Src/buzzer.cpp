/*
 * buzzer.cpp
 *
 */

#include "buzzer.h"
#include "main.h"

void BUZZER::activate(bool e) {
	enabled = e;
	if (enabled) {
		HAL_TIM_PWM_Start(&BUZZER_TIM, TIM_CHANNEL_2);
		BUZZER_TIM.Instance->CCR2 	= 0;
	} else {
		HAL_TIM_PWM_Stop(&BUZZER_TIM, TIM_CHANNEL_2);
	}
}

void BUZZER::playTone(uint16_t period_mks, uint16_t duration_ms) {
	BUZZER_TIM.Instance->ARR 	= period_mks-1;
	BUZZER_TIM.Instance->CCR2 	= period_mks >> 1;
	HAL_Delay(duration_ms);
	BUZZER_TIM.Instance->CCR2 	= 0;
}

void BUZZER::shortBeep(void) {
	if (!enabled) return;
	playTone(284, 160);
}

void BUZZER::doubleBeep(void) {
	if (!enabled) return;
	playTone(284, 160);
	HAL_Delay(100);
	playTone(284, 160);
}

void BUZZER::lowBeep(void) {
	if (!enabled) return;
	playTone(2840, 160);
}

void BUZZER::failedBeep(void) {
	if (!enabled) return;
	playTone(284, 160);
	HAL_Delay(50);
	playTone(2840, 60);
	HAL_Delay(50);
    playTone(1420, 160);
}

