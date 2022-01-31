// Exemplo ST.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include <iostream>
#include <fstream>
#include "CEvent.h"
#include <stdlib.h>
CEventManager eventManager;

std::ofstream outGlobalFile("globalResults.txt");

struct CallData
{
    long callNumber;
    int entrySwitch;
    int route[4];

    double startTime;
    double serviceTime;
};

typedef struct lNode {
    struct lNode* next;
    CallData* calldata;
} LNode;

void lInsTail(LNode** hp, LNode** tp, CallData* call) {
    LNode* n = (lNode*)malloc(sizeof *n);
    n->calldata = call;
    n->next = NULL;
    if (*hp == NULL)
        *hp = n;
    else
        (*tp)->next = n;
    *tp = n;
}

int lRemHead(LNode** hp, LNode** tp, CallData** call) {
    if (*hp == NULL)
        return 0;
    LNode* p = *hp;
    *call = p->calldata;
    *hp = p->next;
    if (*hp == NULL)
        *tp = NULL;
    free(p);
    return 1;
}

class CallCenter {

    int operadores;
    int Numero_Fila_espera;
    LNode* head = NULL;
    LNode* tail = NULL;
    //int idxPull = 0;
    //int idxPush = 0;
    //CallData* Buffer = new CallData;
    //boolean callCenter = false;

public:
    CallCenter(int NumOperadores) {
        this->operadores = NumOperadores;
        this->Numero_Fila_espera = 0;
        //this->idxPull = 0;
        //this->idxPush = 0;
    }
    
    void DecrementarOperadores() {
        operadores--;
    }

    void IncrementarOperadores() {
        operadores++;
    }
    int getEspera() {
        return Numero_Fila_espera;
    }

    int getOperadores() {
        return operadores;
    }

    void bPush(CallData* data) {
      
        /*if (Numero_Fila_espera == 0)
        {
            //Buffer = (CallData*)malloc(sizeof(CallData));
            //Buffer[idxPush] = data;
        }
        else {
            Buffer = (CallData*)realloc(Buffer, sizeof(CallData) * (idxPush - idxPull));
            *Buffer[idxPush] = data;
        }*/
        //idxPush++;

        lInsTail(&head, &tail, data);
        Numero_Fila_espera++;
    }


    CallData* bPull() {

        CallData* data;
        Numero_Fila_espera--;
        lRemHead(&head, &tail, &data);
        /*CallData* ret = &(*Buffer[idxPull]);
        free(Buffer[idxPull]);
        idxPull++;*/

        return data;
    }
};

class CSwitch;

struct SwitchLink
{
    int numLines;
    int occupiedLines;
    double carriedCallsTime;

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

        outLinks[1].numLines = nlines2;
        outLinks[1].pNextSwitch = pSwitch2;
        outLinks[1].occupiedLines = 0;

        char fileName[100];
        sprintf_s(fileName, "switch_%c_.txt", cName);
        outFile.open(fileName, std::ofstream::out);
    }

     boolean RequestLine(int hop, int route[4]) {
        totalCalls++;
        int i;
        for (i = 0; i < nLinks; i++)
        {
            if (outLinks[i].occupiedLines < outLinks[i].numLines)
            {
                if (outLinks[i].pNextSwitch != NULL) {
                    if (!(outLinks[i].pNextSwitch->RequestLine(hop + 1, route))) {  
                        return false;
                    }
                }
     
                outLinks[i].occupiedLines++;
                route[hop] = i;
                return  true;
            }
        }
        
        blockedCalls++; 
        return false;
    };

    void ReleaseLine(int hop, CallData* pCall, double currentTime)
    {
        if (outLinks[pCall->route[hop]].pNextSwitch != NULL) outLinks[pCall->route[hop]].pNextSwitch->ReleaseLine(hop + 1, pCall, currentTime);
        
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
            outFile << outLinks[i].occupiedLines << '\t' <<
                outLinks[i].carriedCallsTime / time << '\t';
        }
        outFile << '\n';
    }
};


///CallCenter callcenter;

struct Config
{
    //system data
    double bhca;
    double holdTime;
    int nLines;

    //network links capacity
    int nLines_A_C = 40;
    int nLines_A_D = 40;
    int nLines_B_D = 40;
    int nLines_D_E = 40;
    int nLines_D_F = 40;
    int nLines_C_E = 40;
    int nLines_E_F = 40;
    int nLines_F_CC = 810;
    //simulation
    double simulationTime;

    int nOperadores = 70;
} config;

CallCenter callcenter = CallCenter(config.nOperadores);
CSwitch network[6];

struct StateData
{
    long totalCalls;
    long blockedCalls;

    double carriedServiceTime;
    double reqServiceTime;

    int ocuppiedLines;
} stateData;


/////////////////////////////////////////////////////////////////////////////////
//Random variable generation

//random number generator U(0,1)
float urand()
{
    float u;
    do {
        u = ((float)rand()) / (float)RAND_MAX;
    } while (u == 0.0 || u >= 1.0);
    return u;
}

//exponential number generation
float expon(float media)
{
    return (float)(-media * log(urand()));
}

//integer number generator U(Min,Max)
int intRand(int min, int max)
{
    float u = urand();

    return min + (u * (max - min + 1));
}

//returns the starting channel based on a percentage given by the professor(c1:35 c2:30 c3:35)
int channelSelect() {

    int perc = intRand(0, 100);
    int channel2 = 30;

    if (perc <= channel2) return 1;
    else {
        int fifthyf = intRand(0, 1);
        if (fifthyf == 0) return 0;
        else return 2;
    }
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
    config.bhca = 1080; //2000      (1 (35%) -> 378   2 (30%) -> 324     3 (35%) -> 378)
    config.holdTime = 190;  //500
    config.nLines = 70;     //55

    config.simulationTime = 24 * 60 * 60; //24 hours

    //state data initialization
    ZeroMemory(&stateData, sizeof(stateData));

    //schedule first setup event
    eventManager.AddEvent(new CEvent(expon(3600.0 / config.bhca), SETUP));
}


void Setup(CEvent* pEvent)
{
    //schedule next setup event
    eventManager.AddEvent(new CEvent(pEvent->m_time + expon(3600.0 / config.bhca), SETUP));

    //current call
    CallData* pNewCall = new CallData;

    pNewCall->serviceTime = expon(config.holdTime);//generate call duration
    pNewCall->startTime = pEvent->m_time;
    pNewCall->entrySwitch = channelSelect();//intRand(0, 2);//geraçao de chamadas
    pNewCall->callNumber = stateData.totalCalls;

    stateData.totalCalls++;
    stateData.reqServiceTime += pNewCall->serviceTime;


    if (network[pNewCall->entrySwitch].RequestLine(0, pNewCall->route))
    {
        if (callcenter.getOperadores() > 0) {
            if (callcenter.getEspera() > 0) {

                CallData *call = callcenter.bPull();
                callcenter.DecrementarOperadores();
                eventManager.AddEvent(new CEvent(pEvent->m_time + call->serviceTime, RELEASE, call));
            }
            else {
                callcenter.DecrementarOperadores();
                eventManager.AddEvent(new CEvent(pEvent->m_time + pNewCall->serviceTime, RELEASE, pNewCall));
            }
        }
        else callcenter.bPush(pNewCall);
    }
    else {
        stateData.blockedCalls++;
    }
    outGlobalFile << pEvent->m_time << '\t' <<
        stateData.reqServiceTime / pEvent->m_time << '\t' <<
        stateData.carriedServiceTime / pEvent->m_time << '\t' <<
        (float)(stateData.blockedCalls) / stateData.totalCalls << '\t' <<
        callcenter.getEspera() << '\t' <<
        callcenter.getOperadores() << '\n';
}


void Release(CEvent* pEvent)
{
    callcenter.IncrementarOperadores();

    CallData* pCall = (CallData*)(pEvent->GetData()); 
    network[pCall->entrySwitch].ReleaseLine(0, pCall, pEvent->m_time);
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
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file