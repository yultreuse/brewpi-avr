/*
 * Carboy.h
 *
 * Created: 10-5-2013 12:58:45
 *  Author: Elco
 */ 


#ifndef CARBOY_H_
#define CARBOY_H_

enum carboyModes{
	CARBOY_BEER_CONSTANT,
	CARBOY_BEER_PROFILE,
	CARBOY_FREERIDE,
	NUM_CARBOY_MODES
};

enum carboyRoles{
	CARBOY_ROLE_MASTER, // temp setting for chamber comes from this carboy
	CARBOY_ROLE_SLAVE, // temp setting for chamber comes from other carboy
	NUM_CARBOY_ROLES	
};

class Carboy
{
	uint8_t mode;
	
	fixed7_9 tempSetting;
	fixed7_9 suggestedChamberSetting;
	
	fixed7_9 tempDiff;
	fixed23_9 tempDiffIntegral; // also uses 9 fraction bits, but more integer bits to prevent overflow
	fixed7_9 tempSlope;
	fixed23_9 p;
	fixed23_9 i;
	fixed23_9 d;
	
	fixed7_9 storedTempSetting;
	
	TempSensor * tempSensor;
			
	Actuator * cooler;
	Actuator * heater;
	
	Carboy();
	~Carboy();
	void loadDefaultSettings();
	void updatePID(fixed7_9 chamberTemp, fixed7_9 chamberSetting, uint16_t timeSinceIdle);
	void updateTemperature();
	void init();
};



#endif /* CARBOY_H_ */