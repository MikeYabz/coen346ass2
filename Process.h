//
// Created by MikeY on 3/6/2019.
//

#ifndef UNTITLED1_PROCESS_H
#define UNTITLED1_PROCESS_H

#include <iostream>

enum Status { notReady, unfinished, finished};
class Process {
public:
    int startTime;
    int duration;
    int progress;
    bool finishedStatus;
    Status status;

    Process(int inputStartTime, int inputDuration){
        this->startTime = inputStartTime;
        this->duration = inputDuration;
        progress = 0;
        finishedStatus = false;
        status = notReady;
    }

};


#endif //UNTITLED1_PROCESS_H
