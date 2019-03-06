#include <iostream>
#include <vector>
#include <string.h>
#include <fstream>
#include <cstring>
#include "User.h"
#include <chrono>
#include <thread>
#include <w32api/processthreadsapi.h>

const bool DEBUG = true;

// Get time stamp in microseconds.
uint64_t micros();
void updateStatus(std::vector<User>users, uint64_t startTime);
void runThread(User &user);

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
            Process tempProcess = users[i].processes[j];
            std::cout << "process:" << j << " ready time " << tempProcess.startTime << std::endl;
            std::cout << "process:" << j << " process time " << tempProcess.duration << std::endl;
        }
        std::cout << "-------------------------------------\n";
    }

    const uint16_t userCount = 1000;// users.size();
    uint16_t counterUsersFinished = 0;

    uint64_t start = micros();
    auto quantumMicro = quantum*1000000; //convert quantum in second to milliseconds
    std::vector<std::thread> processes;

    for(int i=0;i<users.size();i++) {
        for (int j = 0; j < users[i].processes.size(); j++) {
            std::thread th(runThread,users[i].processes[j]);
            processes.push_back(std::move(th));
        }
    }
    while(counterUsersFinished < userCount){
        std::cout << "startTime: " << micros()-start << "\n";
        int activeUsers = 0;
        updateStatus(users,start);
        for(int i=0 ; i<users.size() ; i++){

        }
        counterUsersFinished++;
    }

    return 0;
}

void runThread(User &user){

}

void updateStatus(std::vector<User>users, uint64_t startTime){
    for(int i=0 ; i<users.size() ; i++){
        User tempUser = users[i];
        for(int j=0 ; j<tempUser.processes.size() ; j++){
            Process tempProcess = tempUser.processes[j];
            if (tempProcess.status == notReady){
                if (micros() - startTime > tempProcess.startTime){
                    tempProcess.status = unfinished;
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