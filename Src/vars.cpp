/*
 * vars.cpp
 *
 *  2025 OCT 10
 *  	Ported from UNITED controller source code, tailored to the new hardwares
 */

#include "vars.h"

const uint16_t	int_temp_max				= 6800;			// Maximum possible temperature in internal units
const uint16_t	tweez_temp_minC				= 180;			// Minimum TWEEZERS calibration temperature in degrees of Celsius
const uint16_t 	tweez_temp_maxC 			= 450;			// Maximum TWEEZERS calibration temperature in degrees of Celsius
const uint16_t	gun_temp_minC				= 120;			// Minimum Hot Air Gun calibration temperature in degrees of Celsius
const uint16_t 	gun_temp_maxC 				= 550;			// Maximum Hot Air Gun calibration temperature in degrees of Celsius
const uint16_t	temp_cold					= 400;			// The temperature of the Hot Air Gun when it is cold, Celsius * 10
