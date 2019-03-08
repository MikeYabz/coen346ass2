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


    //Enables debug mode with termninal outputs
const bool DEBUG = true;

    //System Clock
std::mutex systemTimeMutex; //allows access to checking the system Time and it's associated variables
long long systemTime = 0; //starts at 0 and only gets incremented by processes "doing work"
bool processTimeStopFlag = false;   //tell the clock interrupt thread to end itself if needed

    //Mutex and condition variable for the scheduler to be signaled to wake up and take back control of the CPU
std::mutex schedulerSleepMutex;
std::condition_variable cond;

    //Allows reading/writing of process parameter variables
std::mutex modifyProcessMutex;

    //mutex for whichever thread has control of the CPU
std::mutex threadRun;


void updateStatus(std::vector<User> &users, std::vector<std::thread> &activeProcesses);
void runThread(Process* process);
void clockTimeout(uint32_t timeout);
bool checkForActiveProcesses(std::vector<User> users);
uint16_t returnHowManyActiveUsers(std::vector<User> users);
uint16_t returnHowManyActiveProcessesInAUser(User user);

int main() {
    //********************************************************************************************************
    //**************************************Parse Input File**************************************************
    //********************************************************************************************************
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
    //********************************************************************************************************
    //****************************************End of Parsing**************************************************
    //********************************************************************************************************

    /*
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
    }*/


    uint32_t  quantumThousand = quantum*1000; //convert quantum in seconds to milliseconds, makes splitting the quantum for fairsharing easier
    std::vector<std::thread> activeProcesses;   //create array which will hold all the process threads
    threadRun.lock();   //scheduler takes control of the CPU
    while(true){

        std::cout << "startTime: " << systemTime << "\n";

            //Check if new processes are ready for processing
        updateStatus(users,activeProcesses);    //check if new processes are ready


            //Check For Active Processes and if not, increment system clock
        if (checkForActiveProcesses(users) == false){
            systemTime++;
            continue;
        }

        uint32_t numberOfUsers = returnHowManyActiveUsers(users);  //used to calculate how much of the quantum a user will get (divided equally among users)

            //Perform a quantum cycle, looping through all processes
        for(int i=0 ; i<users.size() ; i++) {
            uint32_t numberOfUserProcesses = returnHowManyActiveProcessesInAUser(users[i]); //used to calculate how much of the quantum a process will get (divided equally among processes)
            for (int j = 0; j < users[i].processes.size(); j++) {
                if (users[i].processes[j].status == ready){ //check if process is ready to be processes

                        //prepare take process out of stop state
                    modifyProcessMutex.lock(); //take lock to allow modifying fo the Process object
                    uint32_t progressDoneBefore = users[i].processes[j].progress;   //record progress before allowing it to run
                    users[i].processes[j].stopFlag = false; //when set low, the process is allowed to run assuming is also has threadRun
                    modifyProcessMutex.unlock();

                        //prepare clock timeout so the schedule can take back control of the thread
                    systemTimeMutex.lock();
                    processTimeStopFlag = false;
                    systemTimeMutex.unlock();
                    uint32_t allotedTime; //stores how long a process will be allowed to run for
                    allotedTime = quantumThousand/numberOfUsers/numberOfUserProcesses;
                    std::thread processTimer(clockTimeout,allotedTime); //setting up a scheduler timeout, 1000 is temporary, need to calculate actual runtimes

                        //release threadRun lock which now allows the thread to finally begin processing
                    threadRun.unlock();
                    //the master now gives up control of threadRun and waits for either a timeout from the clock ot the process to finish itself
                    std::unique_lock<std::mutex> schedulerSleep(schedulerSleepMutex);
                    cond.wait(schedulerSleep);

                        //Signal thread to suspend itself if it isn't already suspended
                    modifyProcessMutex.lock();
                    users[i].processes[j].stopFlag = true;
                    uint32_t progressDoneAfter = users[i].processes[j].progress;
                    uint32_t timeUsedByProcess =  progressDoneAfter-progressDoneBefore;
                    modifyProcessMutex.unlock();

                        //Take back control of threadRun
                    threadRun.lock();

                        //get rid of clock timeout thread
                    systemTimeMutex.lock(); //take lock to allow read/writing of processTimeStopFlag
                    processTimeStopFlag = true; //when flag is set high, the clock timeout will end itself
                    systemTimeMutex.unlock();   //release the lock
                    processTimer.join();    //allow the clock thread to end

                }
            }
        }
            //End of this quantum cycle

            //Check if all processes are done and breaks the while loop if so
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
            //End
    }

        //End all process threads
    for(int i=0;i<activeProcesses.size();i++){
        activeProcesses[i].join();
    }
    if (DEBUG){
        std::string temp;   //pause terminal for debugging
        std::cin >> temp;   //pause terminal for debugging
    }
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


        //Checks for break condition, if the processes has consumed enough system clock time to complete itself
        if (process->progress >= process->duration){
            process->status = finished;
            break;
        }
    }
    //if anything gets to this point, it is finished processing, release the threadRun mutex and notify the scheduler to move on
    threadRun.unlock();
    cond.notify_one();
}


/**
 * checks users to see if they are ready to be processed
 *
 * @param users - an array of the users
 * @param activeProcesses - the array of active processes
 * @return void
 */
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


/**
 * Checks if there are any active users
 *
 * @param users - an array of the users
 * @return true if there are active users, false if not
 */
bool checkForActiveProcesses(std::vector<User> users){
    for (int i=0; i<users.size(); i++) {
        for (int j = 0; j < users[i].processes.size(); j++) {
            if (users[i].processes[j].status == ready) {
                return true;
            }
        }
    }
    return false;
}


/**
 * Checks how many users have active processes
 *
 * @param users - an array of the users
 * @return the number of of active users
 */
uint16_t returnHowManyActiveUsers(std::vector<User> users){
    uint16_t activeUserCounter = 0;
    for (int i=0; i<users.size(); i++) {
        for (int j = 0; j < users[i].processes.size(); j++) {
            if (users[i].processes[j].status == ready) {
                activeUserCounter++;
                break;
            }
        }
    }
    return activeUserCounter;
}



/**
 * Checks how many ready processes are in a user
 *
 * @param user - the user to be checked
 * @return the number of of active  processes
 */
uint16_t returnHowManyActiveProcessesInAUser(User user){
    uint16_t activeProcessCounter = 0;
    for (int j = 0; j < user.processes.size(); j++) {
        if (user.processes[j].status==ready){
            activeProcessCounter++;
        }
    }
    return  activeProcessCounter;
}


/**
 * handles the system clock, will wakeup the main scheduler thread when the timeout is reached.
 *
 * @param timeout - in how many clock cycles to wakeup the scheduler
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