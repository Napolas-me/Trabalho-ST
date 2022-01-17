// Example ST.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <iostream>
#include "CEvent.h"
#include <list>
#include <iostream>
#include <fstream>

std::ofstream statFile("stat.txt");

//random number generator U(0,1)
double urand()
{
	double u;
	do {
		u = ((double)rand()) / (double)RAND_MAX;
	} while (u == 0.0 || u >= 1.0);
	return u;
}

//exponential number generation
double expon(double avg)
{
	return (double)(-avg * log(urand()));
}

//simultaion time
double simTime;
//event manager
CEventManager eventManager;


//global simulation data
struct Config
{
	double 	holdTime;	//call average holding time
	double 	bhca;		//call  busy hour rate
	int 	nLines;		//system number of lines

	double 	simTime;	//simulation time
} config;
	
struct StateData
{
	int occupiedLines;	//number of occupied lines
		
	long totalCalls;	//total arrived calls
	long bloquedCalls;	//bloqued calls

	double reqServiceTime;	//total requested time
	double carriedServiceTime;	//total carried time
	double totalHoldingTime;	//total holding time

} stateData;


void Initialize()
{
	//Simulation
	simTime = 0;
	config.simTime = 24 * 60 * 60;
	//traffic configuration
	config.bhca = 500;
	config.holdTime = 140;
	config.nLines = 10;

	//state data initialization	
	memset(&stateData, 0, sizeof(stateData));
	
	//Add first events
	eventManager.Reset();
	eventManager.AddEvent(new CEvent(expon(3600.0 / config.bhca), SETUP));
};


//SETUP Event
void Setup(CEvent* pEvent)
{
	//compute and schedule next call
	double nextCallTime = pEvent->m_time + expon(3600.0 / config.bhca);
	eventManager.AddEvent(new CEvent(nextCallTime, SETUP));

	//current call
	double serviceTime = expon(config.holdTime);

	//update system state
	stateData.reqServiceTime += serviceTime;
	stateData.totalCalls++;

	//request line	
	if (stateData.occupiedLines == config.nLines)
	{
		//blocking !! (lost call)
		stateData.bloquedCalls++;
	}
	else
	{
		stateData.occupiedLines++;
		stateData.carriedServiceTime += serviceTime;
		
		eventManager.AddEvent(new CEvent(pEvent->m_time + serviceTime, RELEASE));
	}

	statFile<<pEvent->m_time<<'\t'<<pEvent->m_type<<'\t'<< stateData.occupiedLines<<'\t'<< (double)(stateData.bloquedCalls)/stateData.totalCalls<<'\t'<<(double)(stateData.reqServiceTime)/pEvent->m_time<< '\t'<< (double)(stateData.carriedServiceTime)/pEvent->m_time <<'\n';

}


void Release(CEvent* pEvent)
{
	stateData.occupiedLines--;
}

void UpdateStat(double time)
{
	
}

void Run()
{
	CEvent* pEvent = NULL;

	pEvent = eventManager.NextEvent();

	while (pEvent->m_time < config.simTime)
	{
		switch (pEvent->m_type)
		{
		case SETUP:
			Setup(pEvent);
			break;
		case RELEASE:
			Release(pEvent);
			break;
		}
		delete pEvent;
		pEvent = eventManager.NextEvent();
	};
	delete pEvent;
}


int main()
{
    std::cout << "exemplo aplicação ST\n"; 

	Initialize();

	Run();

}

