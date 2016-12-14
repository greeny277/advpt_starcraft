#pragma once

class State;

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

class BuildingStarted : public Action {
    inline virtual void printJSON() {
        // TODO Malte
    }
    inline virtual void finish(State &) {
        // TODO Christian
    }
};
