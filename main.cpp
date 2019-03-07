#include <stdio.h>
#include <iostream>
#include <vector>
#include <string.h>
#include <fstream>
#include <cstring>
#include "User.h"
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <w32api/processthreadsapi.h>


const bool DEBUG = true;
long long systemTime = 0;
std::mutex mu;
std::condition_variable cond;
int r;
HANDLE C;

std::mutex stopFlagaMutex;
std::mutex threadRun;

// Get time stamp in microseconds.
uint64_t micros();
void updateStatus(std::vector<User> &users, std::vector<std::thread> &activeProcesses);
void runThread(Process* process);

int main() {

    //Handle Input File
    std::ifstream inFile;   //create input object
    inFile.open("input.txt");   //open input.txt
    if (!inFile) {  //check if file was opened
        std::cerr << "Unable to open file input.txt";
        exit(1);   // call system to stop
    }
    int quantum;
    inFile >> quantum;
    std::cout << "Quantum: " << quantum << std::endl;
    std::vector<User>users;

    std::cout<< "starting\n-----------\n";
    std::cout << "\n\n\nInput Parsing\n-------------------------------------\n";
    while (!inFile.eof())
    {
        std::string userName;
        inFile >> userName;
        if ((userName.empty()) || (userName == " ")){
            break;
        }
        if (DEBUG) std::cout << "username: " << userName <<std::endl;

        int numberOfProcesses;
        inFile >> numberOfProcesses;
        if (DEBUG) std::cout << "number of processes: " << numberOfProcesses << "\n\n";

        User newUser(userName);

        for(int i=0;i<numberOfProcesses;i++){
            int readyTime;
            int processTime;
            if (DEBUG) inFile >> readyTime;
            if (DEBUG) inFile >> processTime;
            std::cout << "process:" << i << " ready time " << readyTime << std::endl;
            std::cout << "process:" << i << " process time " << processTime << std::endl;
            Process newProcess(readyTime,processTime);
            newUser.processes.push_back(newProcess);
        }
        users.push_back(newUser);
        std::cout << "-------------------------------------\n";
    }
    std::cout << "\n\n\nRead Back From Object Vector\n-------------------------------------\n";
    for(int i=0;i<users.size();i++){
        if (DEBUG) std::cout << "username: " << users[i].username <<std::endl;
        if (DEBUG) std::cout << "number of processes: " << users[i].processes.size() << "\n\n";
        for(int j=0;j<users[i].processes.size();j++) {
            //Process tempProcess = users[i].processes[j];
            std::cout << "process:" << j << " ready time " << users[i].processes[j].startTime << std::endl;
            std::cout << "process:" << j << " process time " << users[i].processes[j].duration << std::endl;
        }
        std::cout << "-------------------------------------\n";
    }

    const uint16_t userCount = 1000;// users.size();
    uint16_t counterUsersFinished = 0;

    uint64_t start = micros();
    auto quantumMicro = quantum*1000000; //convert quantum in second to milliseconds
    unsigned long long virtualClock = 0;
    std::vector<std::thread> activeProcesses;

    /*
    for(int i=0;i<users.size();i++) {
        for (int j = 0; j < users[i].processes.size(); j++) {
            //std::thread tempTH(runThread,&users[i].processes[j]);
            users[i].processes[j].stopFlag = true;
            activeProcesses.push_back(std::thread(runThread,&users[i].processes[j]));
            std::this_thread::sleep_for (std::chrono::milliseconds(2));
        }
    }
     */
    threadRun.lock();
    while(counterUsersFinished < userCount){
        while(systemTime<1000){
            systemTime++;
        }
        //run cycle;
        std::cout << "startTime: " << micros()-start << "\n";
        updateStatus(users,activeProcesses);

        for(int i=0 ; i<users.size() ; i++) {
            for (int j = 0; j < users[i].processes.size(); j++) {


                if (users[i].processes[j].status == ready){
                    stopFlagaMutex.lock();
                    users[i].processes[j].stopFlag = false;
                    stopFlagaMutex.unlock();

                    std::unique_lock<std::mutex> locker(mu);
                    threadRun.unlock();
                    if (!cond.wait_for(locker,std::chrono::microseconds(1000),[]{return r == 1;}) ){   //false == timeout
                        stopFlagaMutex.lock();
                        users[i].processes[j].stopFlag = true;
                        stopFlagaMutex.unlock();
                    }
                    threadRun.lock();
                }


            }
        }
        //counterUsersFinished++;
    }

    return 0;
}

enum States {idle,running};

void runThread(Process* process){
    //std::unique_lock<std::mutex> locker(mu);
    bool boolStopSignalReceived = false;
    States state = idle;
    States  pastState = idle;

    while(process->status==ready)
    {
        if (pastState == idle){
            stopFlagaMutex.lock();
            if(process->stopFlag == false){
                stopFlagaMutex.unlock();
                threadRun.lock();
                state = running;
            }
        }
        else if (state == running){
            stopFlagaMutex.lock();
            if(process->stopFlag == false){
                stopFlagaMutex.unlock();
                threadRun.unlock();
                state = idle;
            }
        }

        if (state == running)
        {
            process->progress++;
            systemTime++;
        }
        pastState = state;

        /*
        while(true){
            stop.lock();
            if(process->stopFlag == false){
                if (process->startSignal == true)
                {
                    process->startSignal = false;
                    threadRun.lock();
                }
                stop.unlock();
                break;
            }
            else{
                stop.unlock();
                if (boolStopSignalReceived == true){
                    threadRun.unlock();
                }
            }

        }
         */


        /*
        stop.lock();

        if (process->stopFlag == true)
        {
            boolStopSignalReceived = true;
        }
        stop.unlock();
         */
    }
    //if anything gets to this point, it is finished processing
    process->status = finished;
    threadRun.unlock();
    cond.notify_one();
}

void updateStatus(std::vector<User> &users, std::vector<std::thread> &activeProcesses){
    for(int i=0 ; i<users.size() ; i++){
        for(int j=0 ; j<users[i].processes.size() ; j++){
            if (users[i].processes[j].status != ready){
                if (systemTime > users[i].processes[j].startTime){
                    users[i].processes[j].status = ready;
                    users[i].processes[j].stopFlag = true;
                    activeProcesses.emplace_back(runThread,&users[i].processes[j]);
                    std::this_thread::sleep_for (std::chrono::milliseconds(5));
                }
            }
        }
    }
}

// Get time stamp in microseconds.
uint64_t micros()
{
    uint64_t us = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::
                                                                        now().time_since_epoch()).count();
    return us;
}