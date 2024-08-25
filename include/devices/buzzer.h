/*
 * buzzer.h
 *
 *  Created on: Feb 10, 2023
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _BUZZER_H_
#define _BUZZER_H_

#include <rtos/rtos.h>

#include <hal/hal.h>

#define NOTE_SILENT_FREQ 0.000F
#define NOTE_B_FREQ 493.883F
#define NOTE_BF_FREQ 466.164F
#define NOTE_AS_FREQ 466.164F
#define NOTE_A_FREQ 440.000F
#define NOTE_AF_FREQ 415.305F
#define NOTE_GS_FREQ 415.305F
#define NOTE_G_FREQ 391.995F
#define NOTE_GF_FREQ 369.994F
#define NOTE_FS_FREQ 369.994F
#define NOTE_F_FREQ 349.228F
#define NOTE_E_FREQ 329.628F
#define NOTE_EF_FREQ 311.127F
#define NOTE_DS_FREQ 311.127F
#define NOTE_D_FREQ 293.665F
#define NOTE_DF_FREQ 277.183F
#define NOTE_CS_FREQ 277.183F
#define NOTE_C_FREQ 261.626F

struct buzzer
{
	struct pwm_slice *pwm;
	osTimerId_t timer;
};

int buzzer_ini(struct buzzer *buzzer, uint32_t gpio);
void buzzer_fini(struct buzzer *buzzer);

struct buzzer *buzzer_create(uint32_t gpio);
void buzzer_destroy(struct buzzer* buzzer);

int buzzer_play_freq(struct buzzer *buzzer, float freq, unsigned int duration, unsigned int volume);
int buzzer_stop(struct buzzer *buzzer);

#endif
