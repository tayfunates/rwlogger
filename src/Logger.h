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
    /**
     * @brief    Thread safe Logger class.
     */
    class Logger
    {
    public:
        //Public logger enumerations
        enum Level
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
        
        enum Result {
            RES_OK = 0,                 ///< Success
            RES_ERROR,                  ///< Generic error
            RES_BAD_ARGS,               ///< Some args are invalid
            RES_INVALID_VALUE,          ///< A value is invalid
            RES_INVALID_OPERATION,      ///< Operation is not allowed
            RES_MEMORY_ERROR,           ///< A memory problem occured (most probably out of memory)
            RES_FILE_ERROR,             ///< A file error occured (most probrably file not found or supported)
            RES_NULL_POINTER            ///< NULL pointer
        };
        
    private:
        std::string             m_path;                         ///< Output file path. In case of empty string, Logger do not write to a file
        void                    *m_pFile;                       ///< Output file. It's opened only when required.
        bool                    m_reflectToConsole;             ///< If true, logger also logs to std::cout or std::cerr (error level messages).
        bool                    m_enabled;                      ///< Enables/disables logging
        size_t                  m_maxLogSize;                   ///< Approximate max length of log file in bytes.
        std::mutex              m_mutex;                        ///< For locking logging operation in a multi threaded environment
        Level                   m_logLevel;                     ///< Defines the level of importance of the messages, only this and lower level messages are logged.
        OverflowAction          m_overflowAction;               ///< Decides what to do when the log size exceeds max log sizes
        const size_t            m_minLogSize = 512;             ///< Minimum value of maximum log size and minimum size after truncation/rotation
        
    public:
        
        //Destructor
        virtual ~Logger();
        
        // Getters and setters.
        void            setEnabled( bool enabled );
        bool            isEnabled( ) const;
        void            setReflectToConsole( bool b);
        bool            isReflectToConsole( ) const;
        
        /**
         * @brief              Sets maximum log size. It does not truncate/rotate the file immediately if the current file size is greater,
         *                     but new log operation will truncate/rotate the file.
         * @param    maxLen    The maximum size of log file. If the max log file size is lower than a
         *                     minimum value, maxLen is set to the minimum allowed size.
         */
        void            setMaxLogSize( size_t maxLen );
        
        /**
         * @brief               Gets maximum log size.
         * @return              The maximum log size.
         */
        size_t          getMaxLogSize() const;
        
        /**
         * @brief               Sets the log level.
         * @param    level      The log level. Only this and lower level messages are logged.
         */
        void            setLogLevel( Level level );
        
        /**
         * @brief               Gets log level.
         * @return              The log level.
         */
        Level        getLogLevel() const;
        
        /**
         * @brief               Gets the path to log file. Logger does not keep track any information about truncated or rotated logs.
         Therefore, this path is the initialized path that the object is logging
         * @return              The log file path.
         */
        std::string        getPath() const;
        
        /**
         * @brief               Gets the current size of the log file. Logger does not keep track any information about truncated or rotated logs.
                                Therefore, this size is the size remaining of the file after any number truncations or rotations
         * @return              The current size of the log file.
         */
        size_t            getLogSize();
        
    private:
        //Cannot instantiate object outside Logger class
        Logger(const std::string& logFilePath, const OverflowAction& action);
        Logger();
        Logger(const Logger& other);
        
        /**
         * @brief               Opens m_pFile (does not create, only opens).
         * @return              RESULT_OK if successful.
         */
        Result          open();
        
        /**
         * @brief               Closes m_pFile (does not delete, only closes).
         */
        void            close();
    };
}

#endif /* Logger_h */

