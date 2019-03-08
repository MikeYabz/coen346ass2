//
// Created by MikeY on 3/6/2019.
//

#ifndef UNTITLED1_PROCESS_H
#define UNTITLED1_PROCESS_H

#include <iostream>
#include <mutex>

enum Status {ready, finished, notReady};

class Process {
private:
    //std::mutex memberMutex;
public:
    bool stopFlag;
    int startTime;
    int duration;
    int progress;
    Status status;


    Process(int inputStartTime, int inputDuration){
        this->startTime = inputStartTime;
        this->duration = inputDuration;
        progress = 0;
        stopFlag = true;
        status = notReady;
    }

};


#endif //UNTITLED1_PROCESS_H
