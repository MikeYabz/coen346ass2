//
// Created by MikeY on 3/6/2019.
//

#ifndef UNTITLED1_PROCESS_H
#define UNTITLED1_PROCESS_H

#include <iostream>
#include <mutex>

enum Status {processed, ready, finished, notReady};

class Process {
private:
    //std::mutex memberMutex;
public:
    int processId;
    bool stopFlag = false;
    int startTime;
    int duration;
    int progress;
    bool finishedStatus;
    Status status;

    //std::mutex &mootex() { return memberMutex; }

    Process(int inputStartTime, int inputDuration){
        this->startTime = inputStartTime;
        this->duration = inputDuration;
        progress = 0;
        finishedStatus = false;
        status = notReady;
    }
    ~Process() {
    }

};


#endif //UNTITLED1_PROCESS_H
