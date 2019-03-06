#include <iostream>
#include <vector>
#include <string.h>
#include <fstream>
#include <cstring>
#include "User.h"


int main() {

    const bool DEBUG = true;
    std::cout << "Hello, World!" << std::endl;

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
    std::vector<User>Users;
    std::string temp;

    /*
    inFile >> temp;
    std::cout << "1 " << temp << std::endl;
    inFile >> temp;
    std::cout << "2 " << temp << std::endl;
    inFile >> temp;
    std::cout << "3 " << temp << std::endl;
     */

    std::cout<< "starting\n\n";
    while (!inFile.eof())
    {
        if (DEBUG) std::cout << "num1\n";
        std::string userName;
        inFile >> userName;

        int numberOfProcesses;
        inFile >> numberOfProcesses;

        for(int i=0;i<numberOfProcesses;i++){
            inFile
        }

        /*
        if (DEBUG) std::cout << "num2\n";
        std::cout << input;
        if (DEBUG) std::cout << "num3\n";
        std::string userName = input.substr(0,1);
        std::string numberOfProcessesString = input.substr(1,1);
        if (DEBUG) std::cout << "num4 " << numberOfProcessesString << std::endl;
        int numberOfProcesses = std::stoi(numberOfProcessesString);
        if (DEBUG) std::cout << "num5\n";
        std::cout<< "username: " << userName << "   processes: " << numberOfProcesses << std::endl;
        if (DEBUG) std::cout << "num6\n";
         */
    }


    //system("PAUSE");
    return 0;
}