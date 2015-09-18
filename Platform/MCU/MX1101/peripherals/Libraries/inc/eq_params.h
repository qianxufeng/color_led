/**
 *******************************************************************************
 * @file    eq_params.h
 * @brief   EQ parameters (type, f0, Q, sqrtA) for the follow EQ styles.
 *
 * @author  Auto Generated by Parametric Equalizer Plot Software
 * @version V1.0.0
 *
 * $Created: Apr  9 2014 10:36:46$:
 *
 * &copy; Shanghai Mountain View Silicon Technology Co.,Ltd. All rights reserved.
 *******************************************************************************
 */

#ifndef __EQ_PARAMS_H__
#define __EQ_PARAMS_H__

#ifdef  __cplusplus
extern "C" {
#endif//__cplusplus

#include "type.h"

typedef enum _EqStyleID
{
	EQ_STYLE_FLAT            = 0,
	EQ_STYLE_CLASSICAL       = 1,
	EQ_STYLE_POP             = 2,
	EQ_STYLE_ROCK            = 3,
	EQ_STYLE_JAZZ            = 4,

	EQ_STYLE_SUM,

} EqStyleID;

/**
 * @brief  Select the "Id"-th EQ style
 * @param  SamplingRate sample rate
 * @param  Id the selected EQ style ID, its defination as follow:
 *    @arg 0: CLASSICAL
 *    @arg 1: VOCAL BOOSTER
 *    @arg 2: FLAT
 *    @arg 3: BASS BOOSTER
 *    @arg 4: BASS REDUCER
 *    @arg 5: TREBLE BOOSTER
 *    @arg 6: TREBLE REDUCER
 *    @arg 7: POP
 *    @arg 8: ROCK
 *    @arg 9: JAZZ
 * @return NONE
 */
void EqStyleSelect(uint8_t Id);

/**
 * @brief  set all audio channel EQ
 * @param  NONE
 * @return NONE
 */
void AudioPlayerSwitchEQ(void);

/**
 * @brief  Treb  Bass gain set
 * @param  TrebBassMode: 0 :treb mode;1:bass mode
 * @param  TrebVol: treb volume(0~10)
 * @param  BassVol: bass volume(0~10)
 * @return NONE
 */
void TrebBassSet(uint8_t TrebVol, uint8_t BassVol);

#ifdef  __cplusplus
}
#endif//__cplusplus

#endif//__EQ_PARAMS_H__
