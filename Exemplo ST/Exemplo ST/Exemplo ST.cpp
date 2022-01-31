#include <iostream>
#include <fstream>
#include "CEvent.h"
#include <stdlib.h>
#include <list>

CEventManager eventManager;


std::ofstream dadosSistema("dadosSistema.txt");
struct CallData{
    long callNumber;
    int entrySwitch;
    int route[4];

    double startTime;
    double serviceTime;
};

class CallCenter{

    CList<CallData*, CallData*> Buffer;
    int nOper;
    int espera;

public:

    CallCenter(){
        nOper = 0;
        espera = 0;
        while (!Buffer.IsEmpty()) {
            delete Buffer.RemoveHead();
        }
    }
    void bPush(CallData* data){
        Buffer.AddTail(data);
    }

    CallData* bPull(){

        CallData* data = Buffer.GetHead();
        Buffer.RemoveHead();
        
        return data;
    }

    int getBufferSize() {
        return Buffer.GetSize();
    }

    int getEspera() {
        return espera;
    }

    int GetOper() {
        return nOper;
    }

    void plusOper() {
        nOper++;
    }
    void lessOper() {
        nOper--;
    }

    void plusEspera() {
        espera++;
    }
    void lessEspera() {
        espera--;
    }
};

class CSwitch;

struct SwitchLink{
    int numLines;
    int occupiedLines;
    double carriedCallsTime;

    CSwitch* pNextSwitch;
};

class CSwitch{
    char name;
    long blockedCalls;
    long totalCalls;

    int nLinks;
    SwitchLink outLinks[2];

    std::ofstream outFile;
public:

    CSwitch(){
        blockedCalls = totalCalls = 0;

        memset(&(outLinks), 0, sizeof(outLinks));
        blockedCalls = totalCalls = 0;
    };

    CSwitch(char cName, int nlines1, CSwitch* pSwitch1, int nlines2 = 0, CSwitch* pSwitch2 = NULL){
        name = cName;

        nLinks = (nlines2 == 0 ? 1 : 2);

        memset(&(outLinks), 0, sizeof(outLinks));
        blockedCalls = totalCalls = 0;

        outLinks[0].numLines = nlines1;
        outLinks[0].pNextSwitch = pSwitch1;
        outLinks[0].occupiedLines = 0;

        outLinks[1].numLines = nlines2;
        outLinks[1].pNextSwitch = pSwitch2;
        outLinks[1].occupiedLines = 0;

        char fileName[100];
        sprintf_s(fileName, "switch_%c_.txt", cName);
        outFile.open(fileName, std::ofstream::out);
    }

     boolean RequestLine(int hop, int route[4]){
        totalCalls++;

        for (int i = 0; i < nLinks; i++){
            if (outLinks[i].occupiedLines < outLinks[i].numLines){

                if (outLinks[i].pNextSwitch != NULL) {
                    if (!(outLinks[i].pNextSwitch->RequestLine(hop + 1, route))) return false;
                }
     
                outLinks[i].occupiedLines++;
                route[hop] = i;
                return  true;
            }
        }
        
        blockedCalls++; 
        return false;
    };

    void ReleaseLine(int hop, CallData* pCall, double currentTime){

        if (outLinks[pCall->route[hop]].pNextSwitch != NULL) outLinks[pCall->route[hop]].pNextSwitch->ReleaseLine(hop + 1, pCall, currentTime);
        
        outLinks[pCall->route[hop]].occupiedLines--;
        
        outLinks[pCall->route[hop]].carriedCallsTime += (currentTime - pCall->startTime);
    };

    void WriteResults(double time){

        outFile << time << '\t' <<
        totalCalls << '\t' <<
        blockedCalls << '\t' <<
        ((double)blockedCalls) / totalCalls << '\t';

        for (int i = 0; i < nLinks; i++){
            outFile << outLinks[i].occupiedLines << '\t' <<
            outLinks[i].carriedCallsTime / time << '\t';
        }

        outFile << '\n';
    }
};

struct Config{
 
    double bhca;
    double holdTime;

    int nLines_A_C = 2;
    int nLines_A_D = 31;
    int nLines_B_D = 37;
    int nLines_D_E = 2;
    int nLines_D_F = 58;
    int nLines_C_E = 37;
    int nLines_E_F = 37;
    int nLines_F_CC = 107;


    double simulationTime;

    int nOperadores;
} config;

CallCenter callcenter;

CSwitch network[6];

struct StateData{
    long TodasChamadas;
    long allBlockedCall;

    double carriedServiceTime;
    double reqServiceTime;

    int ocuppiedLines;
    

    int AreaOcupadasCh =0;   
    double lastQueueChangeTime =0;   
    double totalServiceTime=0;   
    double totalWaitTime =0;       
    int MedioOper =0;     

 
} stateData;



float urand(){

    float u;
    do {
        u = ((float)rand()) / (float)RAND_MAX;
    } while (u == 0.0 || u >= 1.0);
    return u;
}

float expon(float media){
    return (float)(-media * log(urand()));
}

int intRand(int min, int max){
    float u = urand();

    return min + (u * (max - min + 1));
}

int channelSelect(int A, int B, int C, int PA, int PB, int PC) {

    int randNumber = rand() % 100 + 1;
    if (randNumber <= PA)
        return A;
    if (randNumber <= (PA + PB))
        return B;
    else
        return C;
}




void Initialize(){
    //Network initialization
    network[5] = CSwitch('F', config.nLines_F_CC, NULL);
    network[4] = CSwitch('E', config.nLines_E_F, &(network[5]));
    network[3] = CSwitch('D', config.nLines_D_F, &(network[5]), config.nLines_D_E, &(network[4]));
    network[2] = CSwitch('C', config.nLines_C_E, &(network[4]));
    network[1] = CSwitch('B', config.nLines_B_D, &(network[3]));
    network[0] = CSwitch('A', config.nLines_A_D, &(network[3]), config.nLines_A_C, &(network[2]));

    
    config.bhca = 1080;
    config.holdTime = 190; 
    config.nOperadores = 63;

    config.simulationTime = 24 * 60 * 60;

    
    ZeroMemory(&stateData, sizeof(stateData));

    
    eventManager.AddEvent(new CEvent(expon(3600.0 / config.bhca), SETUP));
}


void Setup(CEvent* pEvent){
   
    eventManager.AddEvent(new CEvent(pEvent->m_time + expon(3600.0 / config.bhca), SETUP));

  
    CallData* pNewCall = new CallData;

    pNewCall->serviceTime = expon(config.holdTime);
    pNewCall->startTime = pEvent->m_time;
    pNewCall->entrySwitch = channelSelect(0, 1, 2, 35, 30, 35);
    pNewCall->callNumber = stateData.TodasChamadas;

    stateData.TodasChamadas++;

    if (network[pNewCall->entrySwitch].RequestLine(0, pNewCall->route)){

        if (config.nOperadores == callcenter.GetOper()) {
            stateData.AreaOcupadasCh += callcenter.getBufferSize() * (pEvent->m_time - stateData.lastQueueChangeTime);
            stateData.MedioOper += callcenter.GetOper() * (pEvent->m_time - stateData.lastQueueChangeTime);
            stateData.lastQueueChangeTime = pEvent->m_time;
            callcenter.bPush(pNewCall);
            callcenter.plusEspera();
        }

        else{
            callcenter.plusOper();
            eventManager.AddEvent(new CEvent(pEvent->m_time + pNewCall->serviceTime, RELEASE, pNewCall));
            stateData.reqServiceTime += pNewCall->serviceTime;
        }
    }
    else stateData.allBlockedCall++;

}

void Release(CEvent* pEvent){
    CallData* pCall = (CallData*)(pEvent->GetData());

    network[pCall->entrySwitch].ReleaseLine(0, pCall, pEvent->m_time);

    if(callcenter.getBufferSize() > 0) {
        stateData.AreaOcupadasCh += callcenter.getBufferSize() /(pEvent->m_time - stateData.lastQueueChangeTime);
        stateData.MedioOper += callcenter.GetOper() * (pEvent->m_time - stateData.lastQueueChangeTime);
        CallData* call = callcenter.bPull();
        stateData.totalWaitTime += (pEvent->m_time - call->startTime);
        eventManager.AddEvent(new CEvent(pEvent->m_time + call->serviceTime, RELEASE, call));
        stateData.reqServiceTime += call->serviceTime;
    }
    callcenter.lessOper();
    
    for (int i = 0; i < 6; i++) network[i].WriteResults(pEvent->m_time);

    stateData.carriedServiceTime += (pEvent->m_time - pCall->startTime);
}

void Run(){
    CEvent* pEvent = NULL;
    pEvent = eventManager.NextEvent();

    while (pEvent->m_time < config.simulationTime){
        switch (pEvent->m_type){

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

void resutl() {
        double probabilidadeBloqueioCh = (double)stateData.allBlockedCall / stateData.TodasChamadas; //
        double Trafico_Ao = ((config.bhca / 3600) * config.holdTime); //
        dadosSistema << "Trafego oferecido (A) = " << ((config.bhca / 3600) * config.holdTime) << '\n' <<
        "Probabilidade de Bloqueio (B) = " << (double)stateData.allBlockedCall / stateData.TodasChamadas << '\n' <<
        "Trafego Transportado (Ao) =" << (1 - probabilidadeBloqueioCh) * Trafico_Ao << '\n' <<
        "Probabilidade de Espera =" << (double)callcenter.getEspera() / stateData.TodasChamadas << '\n' <<
        "Numero medio de chamadas em espera =" << stateData.AreaOcupadasCh << '\n' <<
        "Tempo de espera =" << stateData.totalWaitTime << '\n' <<
        "Duração media de chamadas =" << stateData.reqServiceTime / (stateData.TodasChamadas - stateData.allBlockedCall) << '\n' <<
        "Numeros medido de operadores ocupados =" << stateData.MedioOper / config.simulationTime << '\n';

}

int main(){
    std::cout << "Starting Simulation!\n";
    Initialize();
    std::cout << "Running Simulation...\n";
    Run();
    std::cout << "Simulation stopped!\n";

    resutl();
    
}

