#include "Carboy.h"

Carboy::Carboy(){
	beerSensor = 0;
	cooler = 0;
	heater = 0;
}

Carboy::~Carboy(){
	if(tempSensor){
		delete tempSensor;
	}
}

void Carboy::init(){ //TODO
	mode=CARBOY_FREERIDE;

	tempSensor->init();
}

void Carboy::updateTemperature(){
	if(tempSensor){
		tempSensor->updateAndReInit();
	}
}

void Carboy::updatePID(fixed7_9 chamberTemp, fixed7_9 chamberSetting, uint16_t timeSinceIdle){
	static unsigned char integralUpdateCounter = 0;
	if(mode == CARBOY_BEER_CONSTANT || mode == CARBOY_BEER_PROFILE){
				
		// fridge setting is calculated with PID algorithm. Beer temperature error is input to PID
		tempDiff =  tempSetting - tempSensor->readSlowFiltered();
		tempSlope = tempSensor->readSlope();
		
		if(role == CARBOY_ROLE_MASTER){
			// only update integral when this carboy is the master carboy, otherwise the temperature will always be off target
			if(integralUpdateCounter++ == 60){
				integralUpdateCounter = 0;
				if(abs(tempDiff) < globalSettings.iMaxError){
					//difference is smaller than iMaxError, check 4 conditions to see if integrator should be active
				
					// Actuator is not saturated. Update integrator
					if((tempDiff >= 0) == (tempDiffIntegral >= 0)){
						// beerDiff and integrator have same sign. Integrator action is increased.
						if(timeSinceIdle > 1800){
							// more than 30 minutes since idle, chamber actuator is probably saturated. Do not increase integrator.
						}
						if(chamberTemp == globalSettings.tempSettingMax && chamberTemp == globalSettings.tempSettingMin){
							// chamber temp is already at max. Increasing actuator will only cause integrator windup.
						}
						else if(tempDiff < 0 && (chamberSetting +1024) < chamberTemp){
							// cooling and chamber temp is more than 2 degrees from setting, actuator is saturated.
						}
						else if(tempDiff > 0 && (chamberSetting -1024) > chamberTemp){
							// heating and fridge temp is more than 2 degrees from setting, actuator is saturated.
						}
						else{
							// increase integrator action
							diffIntegral = diffIntegral + tempDiff;
						}
					}
					else{
						// integrator action is decreased. Decrease faster than increase.
						diffIntegral = diffIntegral + 4*tempDiff;
					}
				}
				else{
					// decrease integral by 1/8 when not close to end value to prevent integrator windup
					diffIntegral = diffIntegral-(diffIntegral>>3);
				
				}	
			}
		}
		
		// calculate PID parts. Use fixed23_9 to prevent overflow
		p = multiplyFixed7_9(globalSettings.Kp, tempDiff);
		i = multiplyFixeda7_9b23_9(globalSettings.Ki, diffIntegral);
		d = multiplyFixed7_9(globalSettings.Kd, tempSlope);
		suggestedChamberSetting = constrain(tempSetting + cv.p + cv.i + cv.d, globalSettings.tempSettingMin, globalSettings.tempSettingMax);
	}
	else{
		// Carboy is free-riding return INT_MIN to indicate
		suggestedChamberSetting = INT_MIN;
	}
}

void Carboy::loadDefaultSettings(){
	mode = CARBOY_FREERIDE;	// the default should be free-ride since this is fail-safe.
	tempSetting = 20<<9;;
}
