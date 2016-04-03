/*
 * togglebtn.h
 *
 *  Created on: 2016-01-03
 *      Author: feilan
 */

#ifndef TOGGLEBTN_H_
#define TOGGLEBTN_H_

#include <stdint.h>

enum ButtonState { BUTTON_HELD, BUTTON_NOT_PRESSED, BUTTON_PRESSED, BUTTON_RELEASED, NO_STATE };

void toggleBtnInit(int8_t joystick, int8_t buttonGroup, int8_t button);

enum ButtonState toggleBtnGet(int8_t joystick, int8_t buttonGroup, int8_t button);

void toggleBtnUpdateAll();

#endif /* TOGGLEBTN_H_ */
