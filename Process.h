//
// Created by MikeY on 3/6/2019.
//

#ifndef UNTITLED1_PROCESS_H
#define UNTITLED1_PROCESS_H

#include <iostream>

class Process {
public:
    int startTime;
    int duration;
    int progress;
    bool finishedStatus;

    Process(int inputStartTime, int inputDuration){
        this->startTime = inputStartTime;
        this->duration = inputDuration;
        progress = 0;
        finishedStatus = false;
    }

};


#endif //UNTITLED1_PROCESS_H
