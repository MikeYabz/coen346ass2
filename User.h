//
// Created by MikeY on 3/6/2019.
//

#ifndef UNTITLED1_USER_H
#define UNTITLED1_USER_H

#include "Process.h"
#include "vector"

class User {
public:
    std::vector<Process>processes;
    std::string username;
    User(std::string inputUsername){
        this->username = inputUsername;
    }
};


#endif //UNTITLED1_USER_H
