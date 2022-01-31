// Exemplo ST.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include <iostream>
#include <fstream>
#include "CEvent.h"
#include "afxtempl.h"
#include <list>

CEventManager eventManager;

std::ofstream outGlobalFile("globalResults.txt");

class CSwitch;

struct CallData
{
    long callNumber;
    int entrySwitch;
    int route[4];

    double startTime;   //Quando começou a chamada
    double serviceTime; //Duraçao da chamada
};

struct SwitchLink
{
    int numLines;   //n de linhas da ligaçao de cada switch
    int occupiedLines;  //n de linhas ocupadas por ligaçao 
    double carriedCallsTime;
    double MediaDeOcup;

    CSwitch* pNextSwitch;
};

class CSwitch
{
    char name;
    long blockedCalls;
    long totalCalls;

    int nLinks;
    SwitchLink outLinks[2];

    std::ofstream outFile;
public:
    CSwitch() {
        blockedCalls = totalCalls = 0;

        memset(&(outLinks), 0, sizeof(outLinks));
        blockedCalls = totalCalls = 0;
    };

    CSwitch(char cName, int nlines1, CSwitch* pSwitch1, int nlines2 = 0, CSwitch* pSwitch2 = NULL) {
        name = cName;

        nLinks = (nlines2 == 0 ? 1 : 2);

        memset(&(outLinks), 0, sizeof(outLinks));
        blockedCalls = totalCalls = 0;

        outLinks[0].numLines = nlines1;
        outLinks[0].pNextSwitch = pSwitch1;
        outLinks[0].occupiedLines = 0;
        outLinks[0].MediaDeOcup = 0;

        outLinks[1].numLines = nlines2;
        outLinks[1].pNextSwitch = pSwitch2;
        outLinks[1].occupiedLines = 0;
        outLinks[1].MediaDeOcup = 0;

        char fileName[100];
        sprintf_s(fileName, "switch_%c_.txt", cName);
        outFile.open(fileName, std::ofstream::out);
    }

    bool  RequestLine(int hop, int route[4]) {
        totalCalls++;
        for (int i = 0; i < nLinks; i++)
        {
            if (outLinks[i].occupiedLines < outLinks[i].numLines)
            {
                if (outLinks[i].pNextSwitch != NULL)
                    if (!outLinks[i].pNextSwitch->RequestLine(hop + 1, route)) {
                        return false;
                    }
                outLinks[i].MediaDeOcup += outLinks[i].occupiedLines;
                outLinks[i].occupiedLines++;
                route[hop] = i;
                return true;
            }
        }
        blockedCalls++;
        return false;
    };

    void  ReleaseLine(int hop, CallData* pCall, double currentTime)
    {
        if (outLinks[pCall->route[hop]].pNextSwitch != NULL)
            outLinks[pCall->route[hop]].pNextSwitch->ReleaseLine(hop + 1, pCall, currentTime);
        outLinks[pCall->route[hop]].occupiedLines--;
        outLinks[pCall->route[hop]].carriedCallsTime += (currentTime - pCall->startTime);
    };

    void WriteResults(double time)
    {
        outFile << time << '\t' <<
            totalCalls << '\t' << blockedCalls << '\t' <<
            ((double)blockedCalls) / totalCalls << '\t';
        for (int i = 0; i < nLinks; i++)
        {
           
            
            outFile << outLinks[i].MediaDeOcup / totalCalls << '\t' <<
                outLinks[i].carriedCallsTime / time << '\t' <<
                outLinks[i].occupiedLines << '\t';
                
        }
        outFile << '\n';
    }
};

CSwitch network[6];

struct Config
{
    //system data
    double simulationTime;  //Sim time
    double bhca;    //call busy hour rate
    double holdTime;    //call average holding time
    //int nLines;     //number of lines
    int nOperators; //number of operators

    //network links capacity
    int nLines_A_C = 2;
    int nLines_A_D = 13;
    int nLines_B_D = 68;
    int nLines_D_E = 2;
    int nLines_D_F = 67;
    int nLines_C_E = 8;
    int nLines_E_F = 8;
    int nLines_F_CC = 73;

} config;

struct StateData
{
    int waitingCalls;      //n de chamadas em fila de espera (!)
    int totalCalls;        //n total de chamadas
    int blockedCalls;      //n total de chamadas perdidas/bloqueadas
    CList<CallData*, CallData*> queue;    //fila de espera (!)

    int     ocuppiedOperators;  //n de operadores ocupados
    int     ocuppiedLines;      //n linhas ocupadas   
    double averageDurationTime;    //tempo de atendimento nos operadores (!)

    int totalQueueOccupiedArea;   //n medio de chamadas espera (!)
    double lastQueueChangeTime;    //(!)
    double totalServiceTime;    // tempo que os operadores estiveram ocupados (!)
    double totalWaitTime;       // tempo que as chamadas estiveram em espera (!)
    int mediaOperatorsBusy;     // média de operadores ocupados (!)

    double carriedServiceTime;     //tempo da chamada ***

} stateData;


/////////////////////////////////////////////////////////////////////////////////
//Random variable generation

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
double expon(double media)
{
    double x = urand();
    double y = log(x);
    double ret = (double)(-media * y);
    return ret;
}

//integer number generator U(Min,Max)
int intRand(int min, int max)
{
    float u = urand();

    return min + (u * (max - min + 1));
}

int randomABC(int A, int B, int C, int PA, int PB, int PC) {
    int randNumber = rand() % 100 + 1;
    if (randNumber <= PA)
        return A;
    if (randNumber <= (PA + PB))
        return B;
    else
        return C;
}

void Initialize()
{
    //Network initialization
    network[5] = CSwitch('F', config.nLines_F_CC, NULL);
    network[4] = CSwitch('E', config.nLines_E_F, &(network[5]));
    network[3] = CSwitch('D', config.nLines_D_F, &(network[5]), config.nLines_D_E, &(network[4]));
    network[2] = CSwitch('C', config.nLines_C_E, &(network[4]));
    network[1] = CSwitch('B', config.nLines_B_D, &(network[3]));
    network[0] = CSwitch('A', config.nLines_A_D, &(network[3]), config.nLines_A_C, &(network[2]));

    //configuration initialization
    config.simulationTime = 24 * 60 * 60; //24 hours
    config.bhca = 1260;
    config.holdTime = 170;   //duração media de chamadas 
    config.nOperators = 60;    //n op (!)



    //state data initialization
    stateData.averageDurationTime = 0;
    stateData.blockedCalls = 0;
    stateData.carriedServiceTime = 0;
    stateData.lastQueueChangeTime = 0;
    stateData.mediaOperatorsBusy = 0;
    stateData.ocuppiedLines = 0;
    stateData.ocuppiedOperators = 0;
    stateData.totalServiceTime = 0;
    stateData.totalCalls = 0;
    stateData.totalQueueOccupiedArea = 0;
    stateData.totalWaitTime = 0;
    stateData.waitingCalls = 0;
    while (!stateData.queue.IsEmpty()) {
        delete stateData.queue.RemoveHead();
    }

    //schedule first setup event
    eventManager.AddEvent(new CEvent(expon(3600.0 / config.bhca), SETUP));
}


void Setup(CEvent* pEvent)
{
    //schedule next setup event
    double nextCallTime = pEvent->m_time + expon(3600.0 / config.bhca);
    eventManager.AddEvent(new CEvent(nextCallTime, SETUP));

    //current call
    CallData* pNewCall = new CallData;
    pNewCall->serviceTime = expon(config.holdTime);//generate call duration
    pNewCall->startTime = pEvent->m_time;
    //pNewCall->entrySwitch = intRand(0, 2);
    pNewCall->entrySwitch = randomABC(0, 1, 2, 10, 85, 5);
    pNewCall->callNumber = stateData.totalCalls;

    //printf("Setup call: %d\n", pNewCall->callNumber); 

    stateData.totalCalls++;

    if (network[pNewCall->entrySwitch].RequestLine(0, pNewCall->route))
    {
        if (config.nOperators == stateData.ocuppiedOperators) { //Operadores ocupados     
            //printf("Ocupados\n");
            //printf("Queue call: %d with time=%lf\n", pNewCall->callNumber, pNewCall->serviceTime);
            stateData.totalQueueOccupiedArea += stateData.queue.GetSize() * (pEvent->m_time - stateData.lastQueueChangeTime);
            stateData.mediaOperatorsBusy += stateData.ocuppiedOperators * (pEvent->m_time - stateData.lastQueueChangeTime);
            stateData.lastQueueChangeTime = pEvent->m_time;
            stateData.queue.AddTail(pNewCall);
            //printf("Queue call: %d with time=%lf\n", stateData.queue.GetTail()->callNumber, stateData.queue.GetTail()->serviceTime);
            stateData.waitingCalls++;

        }
        else {  //Operadores livres
            stateData.ocuppiedOperators++;
            //printf("Adding 1 call / Oprtr = %d\n", stateData.ocuppiedOperators);
            //printf("Adding call: %d with time=%lf\n", pNewCall->callNumber, pNewCall->serviceTime);
            eventManager.AddEvent(new CEvent(pEvent->m_time + pNewCall->serviceTime, RELEASE, pNewCall));
            stateData.totalServiceTime += pNewCall->serviceTime;
        }
    }
    else {
        stateData.blockedCalls++;
    }
    /*
    outGlobalFile << pEvent->m_time << '\t' <<
        stateData.totalServiceTime / pEvent->m_time << '\t' <<
        stateData.carriedServiceTime / pEvent->m_time << '\t' <<
        (float)(stateData.blockedCalls) / stateData.totalCalls << '\n';
    */
    outGlobalFile << (float)(stateData.blockedCalls) / stateData.totalCalls << '\n';


}


void Release(CEvent* pEvent)
{
    CallData* pCall = (CallData*)(pEvent->GetData());

    network[pCall->entrySwitch].ReleaseLine(0, pCall, pEvent->m_time);
    //printf("Release call: %d with time=%lf\n", pCall->callNumber, pCall->serviceTime);

    if (stateData.queue.GetSize() > 0) {
        //printf("Adding Queue call: %d with time=%lf\n", stateData.queue.GetHead()->callNumber, stateData.queue.GetHead()->serviceTime);
        stateData.totalQueueOccupiedArea += stateData.queue.GetSize() * (pEvent->m_time - stateData.lastQueueChangeTime);
        stateData.mediaOperatorsBusy += stateData.ocuppiedOperators * (pEvent->m_time - stateData.lastQueueChangeTime);
        double arrivalTime = stateData.queue.GetHead()->startTime;
        stateData.totalWaitTime += (pEvent->m_time - arrivalTime);
        eventManager.AddEvent(new CEvent(pEvent->m_time + stateData.queue.GetHead()->serviceTime, RELEASE, stateData.queue.GetHead()));
        stateData.totalServiceTime += stateData.queue.GetHead()->serviceTime;
        stateData.queue.RemoveHead();
    }

    stateData.ocuppiedOperators--;
    //printf("Remov 1 call / Oprtr = %d\n", stateData.ocuppiedOperators);
    for (int i = 0; i < 6; i++)
        network[i].WriteResults(pEvent->m_time);
    stateData.carriedServiceTime += (pEvent->m_time - pCall->startTime);
}

void Run()
{
    CEvent* pEvent = NULL;
    pEvent = eventManager.NextEvent();

    while (pEvent->m_time < config.simulationTime)
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
    }

    delete pEvent;
}


int main()
{
    std::cout << "Starting Simulation!\n";
    Initialize();
    std::cout << "Running Simulation...\n";
    Run();
    std::cout << "Simulation stopped!\n";

    double probBloqueio = (double)stateData.blockedCalls / stateData.totalCalls; //
    double trafegoOferecido = ((config.bhca / 3600) * config.holdTime); //
    double trafegoTransportado = (1 - probBloqueio) * trafegoOferecido; //
    double probEspera = (double)stateData.waitingCalls/ stateData.totalCalls; //
    double numChamadasEspera = stateData.totalQueueOccupiedArea; //
    double tempoDeEspera = stateData.totalWaitTime;
    double duracaoChamadas = stateData.totalServiceTime / (stateData.totalCalls - stateData.blockedCalls);
    double operadoresOcupados = stateData.mediaOperatorsBusy / config.simulationTime;

    printf("Trafego oferecido (A) = %lf\n", trafegoOferecido);
    printf("Probabilidade de Bloqueio (B) = %lf\n", probBloqueio);
    printf("Trafego Transportado (Ao) = %lf\n", trafegoTransportado);
    printf("Probabilidade de Espera = %lf\n", probEspera);
    printf("Numero medio de chamadas em espera = %lf\n", numChamadasEspera);
    printf("Tempo de espera = %lf\n", tempoDeEspera);
    printf("Duracao media de chamadas = %lf\n", duracaoChamadas);
    printf("Numero medio de operadores ocupados = %lf\n", operadoresOcupados);

}