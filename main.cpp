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
long long systemTime = 900;
bool processTimeStopFlag = false;
std::mutex systemTimeMutex;
std::mutex mu;
std::condition_variable cond;
int r;
HANDLE C;

std::mutex modifyProcessMutex;
std::mutex threadRun;
std::mutex timeoutEndFlagMutex;

// Get time stamp in microseconds.
uint64_t micros();
void updateStatus(std::vector<User> &users, std::vector<std::thread> &activeProcesses);
void runThread(Process* process);
void clockTimeout(uint32_t timeout);
//void clockTimeout(uint32_t timeout, bool &timeoutEndFlag);

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
            Process newProcess(readyTime*1000,processTime*1000);
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

    /*
    const uint16_t processesCount = users.size();// users.size();
    uint16_t counterProcessesFinished = 0;
    */

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
    while(true){
        std::cout << "startTime: " << systemTime << "\n";
        updateStatus(users,activeProcesses);    //check if new processes are ready

        bool noActiveProcesses = true;
        for(int i=0 ; i<users.size() ; i++) {
            for (int j = 0; j < users[i].processes.size(); j++) {
                if(users[i].processes[j].status == ready)
                {
                    noActiveProcesses = false;
                }
            }
        }
        if (noActiveProcesses == true){
            systemTime++;
            continue;
        }



        for(int i=0 ; i<users.size() ; i++) {
            for (int j = 0; j < users[i].processes.size(); j++) {


                if (users[i].processes[j].status == ready){

                    modifyProcessMutex.lock();
                    uint32_t progressDoneBefore = users[i].processes[j].progress;
                    users[i].processes[j].stopFlag = false;
                    modifyProcessMutex.unlock();


                    systemTimeMutex.lock();
                    processTimeStopFlag = false;
                    systemTimeMutex.unlock();
                    std::thread processTimer(clockTimeout,1000); //setting up a scheduler timeout, 1000 is temporary, need to calculate actual runtimes


                    threadRun.unlock();
                    //the master now gives up control of threadRun and waits for either a timeout from the clock ot the process to finish itself
                    std::unique_lock<std::mutex> locker(mu);
                    cond.wait(locker);

                        //Signal thread to suspend itself
                    modifyProcessMutex.lock();
                    users[i].processes[j].stopFlag = true;
                    uint32_t progressDoneAfter = users[i].processes[j].progress;
                    uint32_t timeUsedByProcess =  progressDoneAfter-progressDoneBefore;
                    modifyProcessMutex.unlock();

                    threadRun.lock();

                    systemTimeMutex.lock();
                    processTimeStopFlag = true;
                    systemTimeMutex.unlock();
                    processTimer.join();

                }
            }
        }


        int counterUsersNotFinished= 0;
        for(int i=0 ; i<users.size() ; i++) {
            for (int j = 0; j < users[i].processes.size(); j++) {
                if(users[i].processes[j].status != finished){
                    counterUsersNotFinished++;
                }
            }
        }
        if (counterUsersNotFinished == 0)
        {
            break;
        }



    }

    for(int i=0;i<activeProcesses.size();i++){
        activeProcesses[i].join();
    }

    std::string temp;
    std::cin >> temp;
    return 0;
}




enum States {idle,running};
void runThread(Process* process){
    bool boolStopSignalReceived = false;
    States state = idle;
    States  pastState = idle;

    while(true)
    {

        //************************STATE CHANGE LOGIC
        if (pastState == idle){
            modifyProcessMutex.lock();
            if(process->stopFlag == false){
                threadRun.lock();
                state = running;
            }
            modifyProcessMutex.unlock();
        }
        else if (pastState == running){
            modifyProcessMutex.lock();
            if(process->stopFlag == true){
                threadRun.unlock();
                state = idle;
            }
            modifyProcessMutex.unlock();
        }
        //************************END


        //************************STATE PROCESSING LOGIC
        if (state == running)
        {
            process->progress++;
            systemTimeMutex.lock();
            systemTime++;
            systemTimeMutex.unlock();
        }
        pastState = state;
        //************************END


        if (process->progress >= process->duration){
            process->status = finished;
            break;
        }
    }
    //if anything gets to this point, it is finished processing
    threadRun.unlock();
    cond.notify_one();
}

void updateStatus(std::vector<User> &users, std::vector<std::thread> &activeProcesses){
    for(int i=0 ; i<users.size() ; i++){
        for(int j=0 ; j<users[i].processes.size() ; j++){
            if (users[i].processes[j].status == notReady){
                if (systemTime >= users[i].processes[j].startTime){
                    users[i].processes[j].status = ready;
                    users[i].processes[j].stopFlag = true;
                    activeProcesses.emplace_back(runThread,&users[i].processes[j]);
                    std::this_thread::sleep_for (std::chrono::milliseconds(5));
                }
            }
        }
    }
}

/*
// Get time stamp in microseconds.
uint64_t micros()
{
    uint64_t us = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::
                                                                        now().time_since_epoch()).count();
    return us;
}
*/
void clockTimeout(uint32_t timeout){
    uint32_t currentTime = systemTime;
    while(true){

        systemTimeMutex.lock();
        if((systemTime > currentTime+timeout) || (processTimeStopFlag == true)){
            systemTimeMutex.unlock();
            cond.notify_one();
            return;
        }
        systemTimeMutex.unlock();

    }
}