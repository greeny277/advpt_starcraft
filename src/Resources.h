// vim: ts=4:sw=4 expandtab
#pragma once

class Resources {
public:
    int gas;
    int minerals;

    Resources(int gas, int minerals) :
        gas(gas),
        minerals(minerals) {
    }
};
