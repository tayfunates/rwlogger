//
//  Logger.cpp
//  rwlogger
//
//  Created by Tayfun Ateş on 8.02.2019.
//  Copyright © 2019 Tayfun Ateş. All rights reserved.
//

#include "Logger.h"
#include <mutex>
#include <fstream>

//Logger defines
#define RW_DEFAULT_MAX_LOG_LENGTH    (1024*1024)

namespace rw
{
    //Lock class which will be responsible from thread sync
    
    
    Logger::Logger()
    {
        m_path = "";
        m_enabled = true;
        m_logLevel = LOG_LEVEL_NORMAL;
        m_reflectToConsole = false;
        m_maxLogSize = RW_DEFAULT_MAX_LOG_LENGTH;
        m_pFile = (std::fstream*) new std::fstream();
        m_overflowAction = ACTION_TRUNCATE;
    }
    
    Logger::Logger( const std::string& fileName )
    {
        m_path = fileName;
        m_enabled = true;
        m_logLevel = LOG_LEVEL_NORMAL;
        m_reflectToConsole = false;
        m_maxLogSize = RW_DEFAULT_MAX_LOG_LENGTH;
        m_pFile = (std::fstream*) new std::fstream();
        m_overflowAction = ACTION_TRUNCATE;
    }
    
    Logger::~Logger()
    {
        close();
        if(m_pFile) {
            delete (std::fstream*) m_pFile;
            m_pFile = nullptr;
        }
    }
    
    void Logger::close()
    {
        if (((std::fstream*)m_pFile)->is_open()) ((std::fstream*)m_pFile)->close();
    }
}



