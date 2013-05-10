/*
 * Copyright 2012 BrewPi/Elco Jacobs.
 *
 * This file is part of BrewPi.
 * 
 * BrewPi is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * BrewPi is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with BrewPi.  If not, see <http://www.gnu.org/licenses/>.
 */ 


#ifndef CONTROLLER_H_
#define CONTROLLER_H_

#include "brewpi_avr.h"
#include "TempSensor.h"
#include "pins.h"
#include "temperatureFormats.h"
#include "Actuator.h"
#include "Sensor.h"
#include "EepromManager.h"


#define EEPROM_TC_SETTINGS_BASE_ADDRESS 0
#define EEPROM_CONTROL_SETTINGS_ADDRESS (EEPROM_TC_SETTINGS_BASE_ADDRESS+sizeof(uint8_t))
#define EEPROM_CONTROL_CONSTANTS_ADDRESS (EEPROM_CONTROL_SETTINGS_ADDRESS+sizeof(ControlSettings))

#define	MODE_FRIDGE_CONSTANT 'f'
#define MODE_BEER_CONSTANT 'b'
#define MODE_BEER_PROFILE 'p'
#define MODE_OFF 'o'
#define MODE_TEST 't'


#define TC_STATE_MASK 0x7;	// 3 bits

#ifndef TEMP_CONTROL_STATIC
#define TEMP_CONTROL_STATIC 0
#endif

#if TEMP_CONTROL_STATIC
#define TEMP_CONTROL_METHOD static
#define TEMP_CONTROL_FIELD static
#else
#define TEMP_CONTROL_METHOD 
#define TEMP_CONTROL_FIELD
#endif

// Making all functions and variables static reduces code size.
// There will only be one TempControl object, so it makes sense that they are static.
/*
 * MDM: To support multi-chamber, I could have made TempControl non-static, and had a reference to
 * the current instance. But this means each lookup of a field must be done indirectly, which adds to the code size.
 * Instead, we swap in/out the sensors and control data so that the bulk of the code can work against compile-time resolvable
 * memory references. While the design goes against the grain of typical OO practices, the savings 
 */


class TempControl{
	public:
	
	TempControl(){};
	~TempControl(){};
	
	TEMP_CONTROL_METHOD void init(void);
		
	TEMP_CONTROL_METHOD void updatePID(void);
	TEMP_CONTROL_METHOD void updateState(void);
	TEMP_CONTROL_METHOD void updateOutputs(void);
	TEMP_CONTROL_METHOD void detectPeaks(void);
	
	TEMP_CONTROL_METHOD uint8_t loadSettings(eptr_t offset);
	TEMP_CONTROL_METHOD uint8_t storeSettings(eptr_t offset);
	TEMP_CONTROL_METHOD void loadDefaultSettings(void);
	
	TEMP_CONTROL_METHOD uint8_t loadConstants(eptr_t offset);
	TEMP_CONTROL_METHOD uint8_t storeConstants(eptr_t offset);
	TEMP_CONTROL_METHOD void loadDefaultConstants(void);
	
	//TEMP_CONTROL_METHOD void loadSettingsAndConstants(void);
		
	TEMP_CONTROL_METHOD uint16_t timeSinceCooling(void);
 	TEMP_CONTROL_METHOD uint16_t timeSinceHeating(void);
  	TEMP_CONTROL_METHOD uint16_t timeSinceIdle(void);
	  
	TEMP_CONTROL_METHOD fixed7_9 getBeerTemp(void);
	TEMP_CONTROL_METHOD fixed7_9 getBeerSetting(void);
	TEMP_CONTROL_METHOD void setBeerTemp(int newTemp);
	
	TEMP_CONTROL_METHOD fixed7_9 getFridgeTemp(void);
	TEMP_CONTROL_METHOD fixed7_9 getFridgeSetting(void);
	TEMP_CONTROL_METHOD void setFridgeTemp(int newTemp);
	
	TEMP_CONTROL_METHOD fixed7_9 getRoomTemp(void) {
		return ambientSensor->read();
	}
		
	TEMP_CONTROL_METHOD void setMode(char newMode);
	TEMP_CONTROL_METHOD char getMode(void) {
		return cs.mode;
	}

	TEMP_CONTROL_METHOD void setState(unsigned char newState){
		state = newState;
	}

	TEMP_CONTROL_METHOD unsigned char getState(void){
		return state;
	}
		
	private:
	
	TEMP_CONTROL_METHOD void constantsChanged();

	public:
	TEMP_CONTROL_FIELD TempSensor* beerSensor;
	TEMP_CONTROL_FIELD TempSensor* fridgeSensor;
	TEMP_CONTROL_FIELD BasicTempSensor* ambientSensor;
	TEMP_CONTROL_FIELD Actuator* heater;
	TEMP_CONTROL_FIELD Actuator* cooler; 
	TEMP_CONTROL_FIELD Actuator* light;
	TEMP_CONTROL_FIELD Sensor<bool>* door;
	
	// Control parameters
	TEMP_CONTROL_FIELD ControlConstants cc;
	TEMP_CONTROL_FIELD ControlSettings cs;
	TEMP_CONTROL_FIELD ControlVariables cv;
	
	friend class TempControlState;
};
	
extern TempControl tempControl;

#if MULTICHAMBER

#if TEMP_CONTROL_STATIC
class TempControlState
{
	protected:
	TempSensor& fridgeSensor;
	TempSensor& beerSensor;
	
	Actuator& heater;
	Actuator& cooler;
	Actuator& light;
	SwitchSensor& door;
	
	// Control parameters
	ControlConstants cc;
	ControlSettings cs;
	ControlVariables cv;
	
	protected:
	// keep track of beer setting stored in EEPROM
	fixed7_9 storedBeerSetting;

	// Timers
	unsigned int lastIdleTime;
	unsigned int lastHeatTime;
	unsigned int lastCoolTime;
	
	// State variables
	uint8_t state;
	bool doPosPeakDetect;
	bool doNegPeakDetect;
	
	public:
	TempControlState(TempSensor& _fridgeSensor, TempSensor& _beerSensor, Actuator& _cooler, Actuator& _heater, Actuator& _light, SwitchSensor& _door)
	: fridgeSensor(_fridgeSensor), beerSensor(_beerSensor), heater(_heater), cooler(_cooler), light(_light), door(_door) {}

	#define applyField(field) memcpy(&TempControl::field, &field, sizeof(TempControl::field));
	#define retractField(field) memcpy(&field, &TempControl::field, sizeof(TempControl::field));
	#define applyFieldRef(field) TempControl::field = &field;
	#define retractFieldRef(field) ;

	void applyInit() {
		applyFieldRef(beerSensor);
		applyFieldRef(fridgeSensor);
		applyFieldRef(cooler);
		applyFieldRef(heater);
		applyFieldRef(door);
		applyFieldRef(light);
	}

	/*load into the global temp controller*/
	void apply() {
		applyInit();
		
		applyField(cc);
		applyField(cs);
		applyField(cv);
		applyField(storedBeerSetting);

		applyField(lastIdleTime);
		applyField(lastHeatTime);
		applyField(lastCoolTime);
		
		applyField(state);
		applyField(doPosPeakDetect);
		applyField(doNegPeakDetect);
	}

	/*save state from the global temp controller*/
	void retract() {
		retractFieldRef(beerSensor);
		retractFieldRef(fridgeSensor);
		retractFieldRef(heater);
		retractFieldRef(cooler);
		retractFieldRef(light);
		retractFieldRef(door);
		
		retractField(cc);
		retractField(cs);
		retractField(cv);
		retractField(storedBeerSetting);

		retractField(lastIdleTime);
		retractField(lastHeatTime);
		retractField(lastCoolTime);
		
		retractField(state);
		retractField(doPosPeakDetect);
		retractField(doNegPeakDetect);
	}

	
#else
	class TempControlState : public TempControl {
		public:
		TempControlState(TempSensor& _fridgeSensor, TempSensor& _beerSensor, Actuator& _cooler, Actuator& _heater, Actuator& _light, SwitchSensor& _door)
		{
			fridgeSensor = &_fridgeSensor;
			beerSensor = &_beerSensor;
			cooler = &_cooler;
			heater = &_heater;
			light = &_light;
			door = &_door;
		}
	
	void applyInit() {
		apply();
	}

	void apply() {
		memcpy(&tempControl, (TempControl*)this, sizeof(TempControl));
	}
	
	void retract() {
		memcpy((TempControl*)this, &tempControl, sizeof(TempControl));
	}
#endif	
	
	friend class Chamber;
	friend class ChamberManager;

};
#endif // MULTICHAMBER



#endif /* CONTROLLER_H_ */