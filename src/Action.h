#pragma once
#include "State.h"

class Action {
    int startPoint;
    int timeToFinish;
    bool isReady();
    virtual void printJSON() = 0;
    virtual void finish(State &) = 0;
};

/*class MuleAction : Action {
    ConcreteWorker *mule;
};*/
