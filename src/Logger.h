//
//  Logger.h
//  rwlogger
//
//  Created by Tayfun Ateş on 8.02.2019.
//  Copyright © 2019 Tayfun Ateş. All rights reserved.
//

#ifndef Logger_h
#define Logger_h

#include <string>
#include <ostream>
#include <sstream>
#include <mutex>
#include <iostream>

namespace rw
{
    enum LogLevel
    {
        LOG_LEVEL_ERROR        = -2,
        LOG_LEVEL_WARNING    = -1,
        LOG_LEVEL_NORMAL    = 0,
        LOG_LEVEL_DEBUG        = 1,
        LOG_LEVEL_INSANE    = 2
    };
    
    enum OverflowAction {
        ACTION_NONE = 0,
        ACTION_TRUNCATE = 1,
        ACTION_ROTATE = 2
    };
    
    /**
     * @brief    Thread safe Logger class.
     */
    class Logger final
    {
    private:
        std::string             m_path;                         ///< Output file path. In case of empty string, Logger do not write to a file
        void                    *m_pFile;                       ///< Output file. It's opened only when required.
        bool                    m_reflectToConsole;             ///< If true, logger also logs to std::cout or std::cerr (error level messages).
        bool                    m_enabled;                      ///< Enables/disables logging
        size_t                  m_maxLogSize;                   ///< Approximate max length of log file in bytes.
        std::mutex              m_mutex;                        ///< For locking logging operation in a multi threaded environment
        LogLevel                m_logLevel;                     ///< Defines the level of importance of the messages, only this and lower level messages are logged.
        OverflowAction          m_overflowAction;               ///< Decides what to do when the log size exceeds max log sizes
        
    public:
        virtual ~Logger();
        
    private:
        //Cannot instantiate object outside Logger class
        Logger( const std::string& logFilePath );
        Logger();
        Logger(const Logger& other);
        
        /**
         * @brief    Closes m_pFile (does not delete, only closes).
         */
        void            close();
    };
}

#endif /* Logger_h */

