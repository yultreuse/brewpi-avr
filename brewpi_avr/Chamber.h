/*
 * chamber.h
 *
 * Created: 15/03/2013 02:19:45
 *  Author: mat
 */ 

#ifndef CHAMBER_H_
#define CHAMBER_H_

#include "brewpi_avr.h"
#include "Sensor.h"
#include "TempSensor.h"
#include "Actuator.h"

/*
#ifndef MULTICHAMBER
#define MULTICHAMBER 1
#endif

#if MULTICHAMBER
#include "TempControl.h"
#include "TempSensor.h"
#include "piLink.h"

	extern ChamberManager chamberManager;
	#define CURRENT_CHAMBER (chamberManager.currentChamber())
	#define CHAMBER_COUNT (chamberManager.chamberCount())
	#else  // !MULTICHAMBER
	#define CURRENT_CHAMBER (0)
	#define CHAMBER_COUNT (1)
	#endif
	
*/

/*
* The chamber class implements an air space, which can contain multiple carboys.
* The chamber can have a heater, a cooler a fan and a light and a door switch.
*/

enum states{
	IDLE,
	STARTUP,
	STATE_OFF,
	DOOR_OPEN,
	HEATING,
	COOLING,
	NUM_STATES
};

enum chamberModes{
	CHAMBER_OFF,
	CHAMBER_CONSTANT,
	CHAMBER_BEER_MINIMUM,
	CHAMBER_BEER_AVERAGE,
	CHAMBER_BEER_MAXIMUM
};

class Chamber
{
	public:
	Chamber();
	~Chamber();

	uint8_t chamberId;		
	fixed7_9 tempSetting;
	fixed7_9 heatEstimator; // updated automatically by self learning algorithm
	fixed7_9 coolEstimator; // updated automatically by self learning algorithm
	
	fixed7_9 estimatedPeak;
	fixed7_9 negPeakEstimate; // last estimate
	fixed7_9 posPeakEstimate;
	fixed7_9 negPeak; // last detected peak
	fixed7_9 posPeak;
	bool doPosPeakDetect;
	bool doNegPeakDetect;
	
	unsigned int lastIdleTime;
	unsigned int lastHeatTime;
	unsigned int lastCoolTime;
	uint8_t mode;
	uint8_t state;
	
	private:
	TempSensor * airSensor;
	BasicTempSensor * ambientSensor;
	
	Actuator * cooler;
	Actuator * heater;
	Actuator * light;
	SwitchSensor * door;
	
	public:
	void init();
	void resetPeakDetect(void);
	void updateTemperatures(void);
	void getTempSettingFromBeers(); //TODO
	
	void increaseEstimator(fixed7_9 * estimator, fixed7_9 error);
	void decreaseEstimator(fixed7_9 * estimator, fixed7_9 error);
	void updateEstimatedPeak(uint16_t estimate, fixed7_9 estimator, uint16_t sinceIdle);
};

/*
class ChamberManager
{
public: 
	ChamberManager(Chamber** _chambers, int _count) 
		: chambers(_chambers), current(0), chamber_count(_count) 
		{			
		}
	
	void init()
	{
		
			DEBUG_MSG(PSTR("Initialized with %d chambers"), chamberCount());
			for (chamber_id i=0; i<chamberCount(); i++) {
				DEBUG_MSG(PSTR("Chamber %d sensor pins %d and %d"), i, getChamber(i).getState().beerSensor.pinNr, 
					getChamber(i).getState().fridgeSensor.pinNr);
			}
		
		current = 0;
	}		
		
	chamber_id switchChamber(chamber_id id);
	
	void initChamber(chamber_id id);
	
	chamber_id chamberCount() const
	{
		return chamber_count;
	}
	
	chamber_id currentChamber() const
	{
		return current;
	}
	
private:
	Chamber& getChamber(chamber_id id) const {
		return *chambers[id];
	}
	
private:
	Chamber** chambers;
	chamber_id current;
	chamber_id chamber_count;
};

*/

#endif /* CHAMBER_H_ */