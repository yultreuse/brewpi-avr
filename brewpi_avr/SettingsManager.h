/*
 * SettingsManager.h
 *
 * Created: 22/04/2013 16:13:19
 *  Author: mat
 */ 

#ifndef SETTINGS_MANAGER_H
#define SETTINGS_MANAGER_H

#include "brewpi_avr.h"
#include <assert.h>
#include "EepromManager.h"
#include "DeviceManager.h"


// Set minimum off time to prevent short cycling the compressor in seconds
#define MIN_COOL_OFF_TIME 300u
// Use a minimum off time for the heater as well, so it heats in cycles, not lots of short bursts
#define MIN_HEAT_OFF_TIME 300u
// Minimum on time for the cooler.
#define MIN_COOL_ON_TIME 300u
// Minimum on time for the heater.
#define MIN_HEAT_ON_TIME 300u
// Use a large minimum off time in fridge constant mode. No need for very fast cycling.
#define MIN_COOL_OFF_TIME_FRIDGE_CONSTANT 900u
// Set a minimum off time between switching between heating and cooling
#define MIN_SWITCH_TIME 600u
// Time allowed for peak detection
#define COOL_PEAK_DETECT_TIME 1800u
#define HEAT_PEAK_DETECT_TIME 900u

/*
 * Manages the settings and devices for multiple carboys and multiple chambers.
 * This is the soul of brewpi.
 *
 * The manager hides the persistence of the settings, and uses the code closest to the settings to provide
 * useful defaults. 
 */
class SettingsManager
{
public:	
	/**
	 * Initialize settings. This attempts to read from persisted settings and apply settings from there.
	 * If that's not possible, defaults are used.
	 */
	static void loadSettings();
	
};

struct GlobalSettings
{
	char tempFormat;
	fixed7_9 tempSettingMin;
	fixed7_9 tempSettingMax;
	fixed7_9 Kp;
	fixed7_9 Ki;
	fixed7_9 Kd;
	fixed7_9 iMaxError;
	fixed7_9 idleRangeHigh;
	fixed7_9 idleRangeLow;
	fixed7_9 heatingTargetUpper;
	fixed7_9 heatingTargetLower;
	fixed7_9 coolingTargetUpper;
	fixed7_9 coolingTargetLower;
	uint16_t maxHeatTimeForEstimate; // max time for heat estimate in seconds
	uint16_t maxCoolTimeForEstimate; // max time for heat estimate in seconds
	// for the filter coefficients the b value is stored. a is calculated from b.
	uint8_t fridgeFastFilter;	// for display, logging and on-off control
	uint8_t fridgeSlowFilter;	// for peak detection
	uint8_t fridgeSlopeFilter;	// not used in current control algorithm
	uint8_t beerFastFilter;	// for display and logging
	uint8_t beerSlowFilter;	// for on/off control algorithm
	uint8_t beerSlopeFilter;	// for PID calculation	
};

extern SettingsManager settingsManager;
extern GlobalSettings globalSettings;

#endif