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
#include <chrono>
#include <thread>
#include <algorithm>

//Logger defines
#define RW_DEFAULT_MAX_LOG_LENGTH    (1024*1024)

namespace rw
{
    //Logger related implementations
    
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
    
    Logger::Result Logger::open()
    {
        std::lock_guard<std::recursive_mutex> lk(m_logMutex);
        if ( ((std::fstream*)m_pFile)->is_open()) return Logger::RES_OK;
        
        ((std::fstream*)m_pFile)->open( m_path.c_str(), std::ios::out | std::ios::app  );
        return ((std::fstream*)m_pFile)->is_open() ? Logger::RES_OK : Logger::RES_FILE_ERROR;
    }
    
    void Logger::close()
    {
        if (((std::fstream*)m_pFile)->is_open()) ((std::fstream*)m_pFile)->close();
    }
    
    void Logger::setEnabled( bool enabled )
    {
        // RAII locker -> https://en.cppreference.com/w/cpp/language/raii
        // Any updates to Logger objects should be protected
        std::lock_guard<std::recursive_mutex> lk(m_logMutex);
        m_enabled = enabled;
    }
    
    bool Logger::isEnabled() const
    {
        return m_enabled;
    }
    
    void Logger::setReflectToConsole( bool b)
    {
        std::lock_guard<std::recursive_mutex> lk(m_logMutex);
        m_reflectToConsole = b;
    }
    
    bool Logger::isReflectToConsole( ) const
    {
        return m_reflectToConsole;
    }
    
    void Logger::setMaxLogSize( size_t maxLen )
    {
        std::lock_guard<std::recursive_mutex> lk(m_logMutex);
        if (maxLen<=m_minLogSize) maxLen = m_minLogSize;
        m_maxLogSize = maxLen;
    }
    
    size_t Logger::getMaxLogSize() const
    {
        return m_maxLogSize;
    }
    
    void Logger::setLogLevel( Level level )
    {
        std::lock_guard<std::recursive_mutex> lk(m_logMutex);
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
        std::lock_guard<std::recursive_mutex> lk(m_logMutex);
        if( open() == Logger::RES_OK ) {
            ((std::fstream*)m_pFile)->seekp(0, std::ios::end );
            size_t size = (size_t) ((std::fstream*)m_pFile)->tellp();
            close();
            return size;
        }
        return 0;
    }
    
    static std::string getLogLevelString(const Logger::Level& level )
    {
        if (level==Logger::LOG_LEVEL_ERROR) return std::string("ERR");
        else if (level==Logger::LOG_LEVEL_WARNING) return std::string("WRN");
        else if (level==Logger::LOG_LEVEL_DEBUG) return std::string("DBG");
        else return std::string("   ");
    }
    
    static std::string getTimeAndDateString()
    {
        auto now = std::chrono::system_clock::now();
        std::time_t time_for_now = std::chrono::system_clock::to_time_t(now);
        std::string timeStr( std::ctime(&time_for_now) );
        size_t end = timeStr.find_last_of('\n');
        timeStr = timeStr.substr(0,end);
        return timeStr;
    }
    
    void Logger::doLog(const Level& level, const std::string& message)
    {
        if(!m_enabled) {
            return;
        }
        if(level > m_logLevel)
        {
            return;
        }
        
        const std::thread::id threadId = std::this_thread::get_id();
        const std::string lvl = getLogLevelString(level);
        
        std::stringstream messageStream;
        messageStream << getTimeAndDateString() << " " << threadId << " " << lvl << "| " << message << std::endl;
        
        {
            std::lock_guard<std::recursive_mutex> lk(m_logMutex);
            
            if(m_overflowAction != ACTION_NONE)
            {
                const size_t logSize = getLogSize();
                if(logSize > m_maxLogSize)
                {
                    if(m_overflowAction == ACTION_TRUNCATE)
                    {
                        truncate(m_maxLogSize/2);
                    }
                    if(m_overflowAction == ACTION_ROTATE)
                    {
                        rotate();
                    }
                }
            }
            
            if(open() == RES_OK)
            {
                *((std::fstream*)m_pFile) << messageStream.str();
                close();
            }
            
            if(m_reflectToConsole)
            {
                std::ostream &ostr = (level == LOG_LEVEL_ERROR) ? std::cerr : std::cout;
                ostr << messageStream.str();
            }
        }
    }
    
    Logger::Result Logger::truncate( size_t newLen )
    {
        if(newLen<m_minLogSize) {
            newLen = m_minLogSize;
        }
        
        std::fstream inFile;
        inFile.open( m_path.c_str(), std::ios::in | std::ios::binary );
        if(inFile.is_open())
        {
            inFile.seekp(-int(newLen), std::ios::end );
            std::string timeStr = getTimeAndDateString();
            std::replace (timeStr.begin(), timeStr.end(), ':', ' ');
            timeStr.erase(std::remove(timeStr.begin(), timeStr.end(), ' '), timeStr.end());
            
            std::fstream tempFile;
            std::string tempFileName(m_path+"."+timeStr+".log");
            
            tempFile.open( tempFileName.c_str(), std::ios::out | std::ios::binary );
            if(tempFile.is_open())
            {
                std::string line;
                while(std::getline(inFile, line))
                {
                    if(line.length()>0) {
                        tempFile << line;
                    }
                }
                tempFile.close();
            }
            
            //rename
            inFile.close();
            remove(m_path.c_str());
            rename(tempFileName.c_str(), m_path.c_str());
            
            return RES_OK;
        }
        return RES_ERROR;
    }
    
    Logger::Result Logger::rotate()
    {
        std::string timeStr = getTimeAndDateString();
        std::replace (timeStr.begin(), timeStr.end(), ':', ' ');
        timeStr.erase(std::remove(timeStr.begin(), timeStr.end(), ' '), timeStr.end());
        std::string newFileName(m_path+"."+timeStr+".log");
        rename(m_path.c_str(), newFileName.c_str());
        remove(m_path.c_str());
        return RES_OK;
    }
    
    //Manager related implementations
    
    const std::string Logger::defaultLoggerFilePath = "rw_default_log.txt";
    const std::string Logger::consoleLoggerFilePath = "";
    std::recursive_mutex Logger::m_managerMutex;
    Logger::LoggerContainer Logger::m_loggers;
    
    Logger::Result Logger::init()
    {
        LogPtr consoleLogger = getConsoleLogger();
        if(consoleLogger) {
            return RES_OK;
        }
        return RES_MEMORY_ERROR;
    }
    
    Logger::LogPtr Logger::getConsoleLogger()
    {
        std::lock_guard<std::recursive_mutex> lk(m_managerMutex);
        const LoggerContainer::iterator it = m_loggers.find(consoleLoggerFilePath);
        
        LogPtr res;
        if(it != m_loggers.end()) {
            res = it->second;
        }
        else
        {
            Logger* pLogger = new (std::nothrow) Logger(consoleLoggerFilePath, ACTION_NONE);
            if(pLogger) {
                pLogger->setReflectToConsole(true);
                res = LogPtr(pLogger);
                m_loggers[consoleLoggerFilePath] = res;
            }
        }
        return res;
    }
    
    Logger::LogPtr Logger::getDefaultLogger(const Logger::OverflowAction& overflowAction)
    {
        std::lock_guard<std::recursive_mutex> lk(m_managerMutex);
        const LoggerContainer::iterator it = m_loggers.find(defaultLoggerFilePath);
        
        LogPtr res;
        if(it != m_loggers.end()) {
            res = it->second;
        }
        else
        {
            Logger* pLogger = new (std::nothrow) Logger(defaultLoggerFilePath, overflowAction);
            if(pLogger) {
                res = LogPtr(pLogger);
                m_loggers[defaultLoggerFilePath] = res;
            }
            else
            {
                res = getConsoleLogger();
            }
        }
        return res;
    }
    
    Logger::LogPtr Logger::getFileLogger(const std::string& filePath, const Logger::OverflowAction& overflowAction)
    {
        std::lock_guard<std::recursive_mutex> lk(m_managerMutex);
        const LoggerContainer::iterator it = m_loggers.find(filePath);
        
        LogPtr res;
        if(it != m_loggers.end()) {
            res = it->second;
        }
        else
        {
            Logger* pLogger = new (std::nothrow) Logger(filePath, overflowAction);
            if(pLogger) {
                res = LogPtr(pLogger);
                m_loggers[filePath] = res;
            }
            else
            {
                res = getConsoleLogger();
            }
        }
        return res;
    }
    
    Logger::Result Logger::destroy(const std::string& filePath)
    {
        if(filePath == "")
        {
            return RES_BAD_ARGS; //Cannot remove console logger
        }
        if(filePath == defaultLoggerFilePath)
        {
            return RES_BAD_ARGS; //Cannot remove default logger
        }
        
        std::lock_guard<std::recursive_mutex> lk(m_managerMutex);
        const LoggerContainer::iterator it = m_loggers.find(filePath);
        if(it != m_loggers.end()) {
            m_loggers.erase(it);
        }
        else
        {
            return RES_ERROR;
        }
        return RES_OK;
    }
}



