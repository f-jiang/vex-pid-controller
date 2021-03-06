/** @file opcontrol.c
 * @brief File for operator control code
 *
 * This file should contain the user operatorControl() function and any functions related to it.
 *
 * Copyright (c) 2011-2014, Purdue University ACM SIG BOTS.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Purdue University ACM SIG BOTS nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL PURDUE UNIVERSITY ACM SIG BOTS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Purdue Robotics OS contains FreeRTOS (http://www.freertos.org) whose source code may be
 * obtained from http://sourceforge.net/projects/freertos/files/ or on request.
 */

#include "main.h"

#include <stdbool.h>
#include <stdint.h>
#include "togglebtn.h"

/*
 * Runs the user operator control code. This function will be started in its own task with the
 * default priority and stack size whenever the robot is enabled via the Field Management System
 * or the VEX Competition Switch in the operator control mode. If the robot is disabled or
 * communications is lost, the operator control task will be stopped by the kernel. Re-enabling
 * the robot will restart the task, not resume it from where it left off.
 *
 * If no VEX Competition Switch or Field Management system is plugged in, the VEX Cortex will
 * run the operator control task. Be warned that this will also occur if the VEX Cortex is
 * tethered directly to a computer via the USB A to A cable without any VEX Joystick attached.
 *
 * Code running in this task can take almost any action, as the VEX Joystick is available and
 * the scheduler is operational. However, proper use of delay() or taskDelayUntil() is highly
 * recommended to give other tasks (including system tasks such as updating LCDs) time to run.
 *
 * This task should never exit; it should end with some kind of infinite loop, even if empty.
 */
void operatorControl()
{
	// controls
	bool isEnabled = false;
	toggleBtnInit(1, 8, JOY_DOWN);	// pid enable/disable

	toggleBtnInit(1, 7, JOY_UP);	// kp increase
	toggleBtnInit(1, 7, JOY_DOWN);	// kp decrease

	toggleBtnInit(1, 5, JOY_UP);	// ki increase
	toggleBtnInit(1, 5, JOY_DOWN);	// ki decrease

	toggleBtnInit(1, 6, JOY_UP);	// kd increase
	toggleBtnInit(1, 6, JOY_DOWN);	// kd decrease

	toggleBtnInit(1, 8, JOY_LEFT);	// zero kp
	toggleBtnInit(1, 8, JOY_UP);	// zero ki
	toggleBtnInit(1, 8, JOY_RIGHT);	// zero kd

	float increments[] = { 0.001, 0.01, 0.1 };
	size_t i = 0;
	toggleBtnInit(1, 7, JOY_RIGHT);	// cycle through increments

	// input and output devices
	const uint8_t inputChannel = 1;
	const uint8_t outputChannel = 6;

	analogCalibrate(inputChannel);

	// pid controller
	float kp = 0.01;
	float ki = 0.01;
	float kd = 0.01;
	const float integralLimit = 100;	// with respect to input
	float setpoint = 2000;				// with respect to input

	float error = 0, lastError = 0;
	float input = 0, output = 0;
	float proportional = 0, integral = 0, derivative = 0;

	const int16_t inputMin = 7;
	const int16_t inputMax = 4095;
	const int8_t outputMin = -60;
	const int8_t outputMax = 60;

	while (true) {
		// cycle through increments
		if (toggleBtnGet(1, 7, JOY_RIGHT) == BUTTON_PRESSED) {
			if (++i >= 3) {
				i = 0;
			}
		}

		// adjust kp, ki, and kd
		if (toggleBtnGet(1, 7, JOY_UP) == BUTTON_PRESSED) {
			kp += increments[i];
		} else if (toggleBtnGet(1, 7, JOY_DOWN) == BUTTON_PRESSED) {
			kp -= increments[i];
		} else if (toggleBtnGet(1, 8, JOY_LEFT) == BUTTON_PRESSED) {
			kp = 0;
		}

		if (toggleBtnGet(1, 5, JOY_UP) == BUTTON_PRESSED) {
			ki += increments[i];
		} else if (toggleBtnGet(1, 5, JOY_DOWN) == BUTTON_PRESSED) {
			ki -= increments[i];
		} else if (toggleBtnGet(1, 8, JOY_UP) == BUTTON_PRESSED) {
			ki = 0;
		}

		if (toggleBtnGet(1, 6, JOY_UP) == BUTTON_PRESSED) {
			kd += increments[i];
		} else if (toggleBtnGet(1, 6, JOY_DOWN) == BUTTON_PRESSED) {
			kd -= increments[i];
		} else if (toggleBtnGet(1, 8, JOY_RIGHT) == BUTTON_PRESSED) {
			kd = 0;
		}

		// adjust setpoint by max of 127/cycle
		setpoint += joystickGetAnalog(1, 3);

		if (setpoint > inputMax) {
			setpoint = inputMax;
		} else if (setpoint < inputMin) {
			setpoint = inputMin;
		}

		input = analogRead(inputChannel);	// get sensor reading
		printf("enabled: %d cur pos: %8f setpoint: %8f increment: %8f p: %8f i: %8f d: %8f\r",
				isEnabled, input, setpoint, increments[i], kp, ki, kd);

		// pid loop on/off
		if (toggleBtnGet(1, 8, JOY_DOWN) == BUTTON_PRESSED) {
			isEnabled = !isEnabled;
		}

		// pid control
		if (isEnabled) {
			error = input - setpoint;

			// proportional term
			proportional = error;

			// integral term
			if (abs(error) < integralLimit) {
				integral += error;
			} else {
				integral = 0;
			}

			// derivative term
			derivative = error - lastError;

			// output calculation
			// TODO may need scaling
			output = (kp * proportional) + (ki * integral) + (kd * derivative);

			if (output < outputMin) {
				output = outputMin;
			} else if (output > outputMax) {
				output = outputMax;
			}

			lastError = error;
		} else {
			// TODO reset other pid controller variables too?
			integral = 0;
			output = 0;
		}

		motorSet(outputChannel, output);	// run motor

		toggleBtnUpdateAll();
		delay(20);
	}
}
