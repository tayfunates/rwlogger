//
//  main.cpp
//  rwlogger
//
//  Created by Tayfun Ateş on 8.02.2019.
//  Copyright © 2019 Tayfun Ateş. All rights reserved.
//

#include <iostream>
#include "Logger.h"

using namespace rw;

int main(int argc, const char * argv[]) {
    // insert code here...
    LOGC(Logger::LOG_LEVEL_WARNING) << " Hello World!" << " and a integer " <<  17 <<std::endl;
    LOGD(Logger::LOG_LEVEL_WARNING) << " Hello World!" << " and a integer " <<  17 <<std::endl;
    LOGF(Logger::LOG_LEVEL_WARNING, "Deneme.txt") << " Hello World!" << " and a integer " <<  17 <<std::endl;
    return 0;
}
