// vim: ts=4:sw=4 expandtab

#include "Ability.h"

Mule::Mule(int energyCosts_) :
    energyCosts(energyCosts_){
    }


Mule::MuleAction create(int startpoint, State &s, EntityInst *triggeredBy){
    WorkerInst mule = new Mule();
    s.entities.push_back(mule);
    return new MuleAction(startPoint, duration, triggeredBy.getID(), mule);
}
