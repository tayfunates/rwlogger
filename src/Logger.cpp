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
    Logger::Logger()
    {
        m_path = "";
        m_enabled = true;
        m_logLevel = Logger::LOG_LEVEL_NORMAL;
        m_reflectToConsole = false;
        m_maxLogSize = RW_DEFAULT_MAX_LOG_LENGTH;
        m_pFile = (std::fstream*) new std::fstream();
        m_overflowAction = Logger::ACTION_TRUNCATE;
    }
    
    Logger::Logger(const std::string& logFilePath, const OverflowAction& action)
    {
        m_path = logFilePath;
        m_enabled = true;
        m_logLevel = Logger::LOG_LEVEL_NORMAL;
        m_reflectToConsole = false;
        m_maxLogSize = RW_DEFAULT_MAX_LOG_LENGTH;
        m_pFile = (std::fstream*) new std::fstream();
        m_overflowAction = action;
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
    
    void Logger::setEnabled( bool enabled )
    {
        // RAII locker -> https://en.cppreference.com/w/cpp/language/raii
        // Any updates to Logger objects should be protected
        std::lock_guard<std::mutex> lk(m_mutex);
        m_enabled = enabled;
    }
    
    bool Logger::isEnabled() const
    {
        return m_enabled;
    }
    
    void Logger::setReflectToConsole( bool b)
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        m_reflectToConsole = b;
    }
    
    bool Logger::isReflectToConsole( ) const
    {
        return m_reflectToConsole;
    }
    
    void Logger::setMaxLogSize( size_t maxLen )
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        if (maxLen<=m_minLogSize) maxLen = m_minLogSize;
        m_maxLogSize = maxLen;
    }
    
    size_t Logger::getMaxLogSize() const
    {
        return m_maxLogSize;
    }
    
    void Logger::setLogLevel( Level level )
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        m_logLevel = level;
    }
    
    Logger::Level Logger::getLogLevel() const
    {
        return m_logLevel;
    }
    
    std::string Logger::getPath() const
    {
        return m_path;
    }
    
    size_t Logger::getLogSize()
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        if( open() == Logger::RES_OK ) {
            ((std::fstream*)m_pFile)->seekp(0, std::ios::end );
            size_t size = (size_t) ((std::fstream*)m_pFile)->tellp();
            close();
            return size;
        }
        return 0;
    }
    
    Logger::Result Logger::open()
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        if ( ((std::fstream*)m_pFile)->is_open()) return Logger::RES_OK;
        
        ((std::fstream*)m_pFile)->open( m_path.c_str(), std::ios::out | std::ios::app  );
        return ((std::fstream*)m_pFile)->is_open() ? Logger::RES_OK : Logger::RES_FILE_ERROR;
    }
}



