#include "Chamber.h"

Chamber::Chamber(){
	airSensor = 0;
	cooler = 0;
	heater = 0;
	light = 0;
	door = 0;
	ambientSensor = 0;
}
	
Chamber::~Chamber(){
	if(airSensor){
		delete airSensor;
	}
}


void Chamber::resetPeakDetect(void){
	doPosPeakDetect=false;
	doNegPeakDetect=false;
}



void Chamber::init(void){ //TODO
	state=STARTUP;

	airSensor->init();
	// init beers
	
	// Request temperature setting from beers
	// updateTemperatures();
	resetPeakDetect();
}

void Chamber::updateTemperatures(void){
	if(ambientSensor){
		ambientSensor->updateAndReInit();
	}
	if(airSensor){
		airSensor->updateAndReInit();
	}
}

void getTempSettingFromCarboys(void){ //TODO
	
	fixed7_9 airTemp = airSensor->readFastFiltered();
	// loop over assigned carboys
	Carboy * carboyPtr = 0; // TODO
	
	carboyPtr.updatePID(airTemp, timeSinceIdle());
	
	/* if(cs.beerSetting == INT_MIN){
		// beer setting is not updated yet
		// set fridge to unknown too
		cs.fridgeSetting = INT_MIN;
		return;
	}*/
	
	switch(mode){
		case CHAMBER_BEER_AVERAGE:
		
		break;
		
		case CHAMBER_BEER_MINIMUM:
		
		break;
		
		case CHAMBER_CONSTANT:
		
		break;
	}
}

uint16_t Chamber::timeSinceCooling(void){
	return ticks.timeSince(lastCoolTime);
}

uint16_t Chamber::timeSinceHeating(void){
	return ticks.timeSince(lastHeatTime);
}

uint16_t Chamber::timeSinceIdle(void){
	return ticks.timeSince(lastIdleTime);
}

// Increase estimator at least 20%, max 50%s
void Chamber::increaseEstimator(fixed7_9 * estimator, fixed7_9 error){
	fixed23_9 factor = 614 + constrain(abs(error)>>5, 0, 154); // 1.2 + 3.1% of error, limit between 1.2 and 1.5
	fixed23_9 newEstimator = (fixed23_9) *estimator * factor;
	byte max = byte((INT_MAX*512L)>>24);
	byte upper = byte(newEstimator>>24);
	*estimator = upper>max ? INT_MAX : newEstimator>>8; // shift back to normal precision
	eepromManager.storeTempSettings(); //TODO
}

// Decrease estimator at least 16.7% (1/1.2), max 33.3% (1/1.5)
void Chamber::decreaseEstimator(fixed7_9 * estimator, fixed7_9 error){
	fixed23_9 factor = 426 - constrain(abs(error)>>5, 0, 85); // 0.833 - 3.1% of error, limit between 0.667 and 0.833
	fixed23_9 newEstimator = (fixed23_9) *estimator * factor;
	*estimator = newEstimator>>8; // shift back to normal precision
	eepromManager.storeTempSettings(); //TODO
}

void Chamber::loadDefaultSettings(){
	mode = CHAMBER_OFF;	// the default should be off since this is fail-safe.
	tempSetting = 20<<9;;
	cs.heatEstimator = 102; // 0.2*2^9
	cs.coolEstimator=5<<9;
}


void Chamber::detectPeaks(void){
	//detect peaks in fridge temperature to tune overshoot estimators
	if(doPosPeakDetect && state!=HEATING){
		bool detected = false;
		fixed7_9 posPeakDetected = airSensor->detectPosPeak();
		fixed7_9 error = posPeak-posPeakEstimate;
		if(posPeakDetected != INT_MIN){
			// positive peak detected
			if(error > globalSettings.heatingTargetUpper){ // positive error, peak was higher than estimate
				// estimated overshoot was too low, so adjust overshoot estimator
				increaseEstimator(&(heatEstimator), error);
			}
			if(error < cc.heatingTargetLower){ // negative error, peak was lower than estimate
				// estimated overshoot was too high, so adjust overshoot estimator
				decreaseEstimator(&(heatEstimator), error);
			}
			ESTIMATOR_MSG("Positive peak detected");
			detected = true;
		}
		else if(timeSinceHeating() + 10 > HEAT_PEAK_DETECT_TIME &&
			airSensor->readFastFiltered() < (posPeakEstimate+globalSettings.heatingTargetLower)){
			// Idle period almost reaches maximum allowed time for peak detection
			// This is the heat, then drift up too slow (but in the right direction).
			// estimator is too high
			posPeakDetected=airSensor->readFastFiltered();
			decreaseEstimator(&(heatEstimator), error);
			ESTIMATOR_MSG("Drifting up after heating too short.");
			detected = true;
		}
		if(detected){
			char tempString1[9]; char tempString2[9]; char tempString3[9];
			ESTIMATOR_MSG("Peak: %s Estimated: %s. New estimator: %s",
			tempToString(tempString1, posPeakDetected, 3, 9),
			tempToString(tempString2, posPeakEstimate, 3, 9),
			fixedPointToString(tempString3, heatEstimator, 3, 9));
			doPosPeakDetect=false;
			posPeak = posPeakDetected;
		}
		if(timeSinceHeating() > HEAT_PEAK_DETECT_TIME){
			doPosPeakDetect = false;
		}
	}
	if(doNegPeakDetect && state!=COOLING){
		bool detected = false;
		fixed7_9 negPeakDetected = airSensor->detectNegPeak();
		fixed7_9 error = negPeakDetected-negPeakEstimate;
		if(negPeakDetected != INT_MIN){
			// negative peak detected
			if(error < globalSettings.coolingTargetLower){ // negative error, overshoot was higher than estimate
				// estimated overshoot was too low, so adjust overshoot estimator
				increaseEstimator(&(coolEstimator), error);
			}
			if(error > cc.coolingTargetUpper){ // positive error, overshoot was lower than estimate
				// estimated overshoot was too high, so adjust overshoot estimator
				decreaseEstimator(&(coolEstimator), error);
			}
			ESTIMATOR_MSG("Negative peak detected ");
			detected = true;
		}
		else if(timeSinceCooling() + 10 > COOL_PEAK_DETECT_TIME &&
				airSensor->readFastFiltered() > (cv.negPeakEstimate+cc.coolingTargetUpper)){
			// Idle period almost reaches maximum allowed time for peak detection
			// This is the cooling, then drift down too slow (but in the right direction).
			// estimator is too high
			decreaseEstimator(&(coolEstimator), error);
			ESTIMATOR_MSG("Drifting down after cooling too short.");
			detected = true;
		}
		if(detected){
			char tempString1[9]; char tempString2[9]; char tempString3[9];
			ESTIMATOR_MSG("Peak: %s. Estimated: %s. New estimator: %s",
				tempToString(tempString1, negPeakDetected, 3, 9),
				tempToString(tempString2, negPeakEstimate, 3, 9),
				fixedPointToString(tempString3, coolEstimator, 3, 9));
			doNegPeakDetect=false;
			negPeak = negPeakDetected;
		}
		if(timeSinceCooling() > COOL_PEAK_DETECT_TIME){
			doNegPeakDetect = false;
		}
	}
}

