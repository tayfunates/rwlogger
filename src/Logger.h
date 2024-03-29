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
#include <memory>
#include <unordered_map>

namespace rw
{
// Logger convenience macros.
#define LOGC(level) Logger::getConsoleLogger()->operator()((level))
#define LOGD(level) Logger::getDefaultLogger()->operator()((level))
#define LOGF(level, file) Logger::getFileLogger((file))->operator()((level))
    
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
            ACTION_TRUNCATE = 1,        ///< Truncation operation of the file to the half of the maximum log size. New size after truncation cannot be smaller than minimum log size.
            ACTION_ROTATE = 2
        };
        
        enum Result {
            RES_OK = 0,                 ///< Success
            RES_ERROR,                  ///< Generic error
            RES_BAD_ARGS,               ///< Some args are invalid
            RES_MEMORY_ERROR,           ///< A memory problem occured (most probably out of memory)
            RES_FILE_ERROR,             ///< A file error occured (most probrably file not found or supported)
        };
        
    private:
        
        //Child ostringstream class enabling << operator to be used by users
        //It enables user defined types to be logged
        //Created for each user log call and flushes its message using Logger just before its destruction
        //http://www.vilipetek.com/2014/04/17/thread-safe-simple-logger-in-c11/
        class logstream : public std::ostringstream
        {
        public:
            logstream(Logger& oLogger, const Level& level) :
            m_logger(oLogger), m_logLevel(level)
            {
            }
            
            logstream(const logstream& ls) :
            m_logger(ls.m_logger), m_logLevel(ls.m_logLevel)
            {
            }
            
            virtual ~logstream()
            {
                m_logger.doLog(m_logLevel, str());
            }
            
        private:
            Logger& m_logger;
            Level m_logLevel;
        };
        
        std::string             m_path;                         ///< Output file path. In case of empty string, Logger do not write to a file
        void                    *m_pFile;                       ///< Output file. It's opened only when required.
        bool                    m_reflectToConsole;             ///< If true, logger also logs to std::cout or std::cerr (error level messages).
        bool                    m_enabled;                      ///< Enables/disables logging
        size_t                  m_maxLogSize;                   ///< Approximate max length of log file in bytes.
        std::recursive_mutex    m_logMutex;                     ///< For locking logging operation in a multi threaded environment
        Level                   m_logLevel;                     ///< Defines the level of importance of the messages, only this and lower level messages are logged.
        OverflowAction          m_overflowAction;               ///< Decides what to do when the log size exceeds max log sizes
        const size_t            m_minLogSize = 512;             ///< Minimum value of maximum log size and minimum size after truncation/rotation
        
    public:
        
        //Destructor
        virtual ~Logger();
        
        // Getters and setters.
        void setEnabled( bool enabled );
        bool isEnabled( ) const;
        void setReflectToConsole( bool b);
        bool isReflectToConsole( ) const;
        
        /**
         * @brief                       Sets maximum log size. It does not truncate/rotate the file immediately if the current file size is greater,
         *                              but new log operation will truncate/rotate the file.
         * @param    maxLen             The maximum size of log file. If the max log file size is lower than a minimum value, maxLen is set to the minimum allowed size.
         */
        void setMaxLogSize( size_t maxLen );
        
        /**
         * @brief                       Gets maximum log size.
         * @return                      The maximum log size.
         */
        size_t getMaxLogSize() const;
        
        /**
         * @brief                       Sets the log level.
         * @param    level              The log level. Only this and lower level messages are logged.
         */
        void setLogLevel( Level level );
        
        /**
         * @brief                       Gets log level.
         * @return                      The log level.
         */
        Level getLogLevel() const;
        
        /**
         * @brief                       Gets the path to log file. Logger does not keep track any information about truncated or rotated logs.
                                        Therefore, this path is the initialized path that the object is logging
         * @return                      The log file path.
         */
        std::string getPath() const;
        
        /**
         * @brief                       Gets the current size of the log file. Logger does not keep track any information about truncated or rotated logs.
                                        Therefore, this size is the size remaining of the file after any number truncations or rotations
         * @return                      The current size of the log file.
         */
        size_t getLogSize();
        
        /**
         * @brief                       Overloaded () operator for logging
                                        Returns an ostringstream object which accumulates messages from the user with C++ style with << operator
         * @param    level              The log level.
         * @return                      custom ostringstream object
         */
        logstream operator()(const Level& level)
        {
            return logstream(*this, level);
        }
    
    private:
        //Cannot instantiate object outside Logger class
        Logger(const std::string& logFilePath, const OverflowAction& action);
        Logger();
        Logger(const Logger& other);
        
        /**
         * @brief                       Opens m_pFile (does not create, only opens).
         * @return                      RESULT_OK if successful.
         */
        Result open();
        
        /**
         * @brief                       Closes m_pFile (does not delete, only closes).
         */
        void close();
        
        /**
         * @brief                       Truncates the  log file if the log size exceeds maximum size to the given new length. Length is approximate.
         * @param   newLen              New length of the log file. If new length is smaller then min length is assigned as new length
         *
         * @return                      RES_OK if successful, RES_ERROR otherwise
         */
        Result truncate( size_t newLen );
        
        /**
         * @brief                       Rotates the file to a new file inside the current directory if the log size exceeds maximum size. New file name is decided using current time and date.
         *
         * @return                      RES_OK if successful, RES_ERROR otherwise
         */
        Result rotate();
        
        /**
         * @brief    Does the actual logging with given level and message.
         */
        void doLog(const Level& level, const std::string& message);
        
    public:
        //Logger manager related types, methods and variables
        
        typedef std::shared_ptr<Logger> LogPtr;
        
        /**
         * @brief                       Inits logging system and the console logger. If creation of file loggers, manager returns the console logger.
                                        This method is to be sure there is at least one logger object alive before proceeding any other logging operation.
                                        Applications are expected to call this method before any logging even though directly calling logger getters most probably will work.
         * @return                      RESULT_OK if successful, RES_MEMORY_ERROR if console logger cannot be created.
         */
        static Result init();
        
        /**
         * @brief                       Return console logger of the application.
                                        All applications are expected to hold a console logger, although it is not used for directing other loggers in case of failures.
         * @return                      Pointer to console logger, null if creation fails.
         */
        static LogPtr getConsoleLogger();
        
        /**
         * @brief                       Returns default file logger of the application..
                                        Path for default file location is defined in defaultLoggerFilePath.
         * @param    overflowAction     Action to be taken when file size exceeds maximum log size. Default action is truncation is to avoid inflation.
         * @return                      Pointer to default file logger, console logger if cannot find and create logger. NOTE: Please make sure console logger is awake via init().
         */
        static LogPtr getDefaultLogger(const OverflowAction& overflowAction = ACTION_TRUNCATE);
        
        /**
         * @brief                       Returns a file logger for the application.
         * @param    filePath           Path to the file for logger to be initialized.
         * @param    overflowAction     Action to be taken when file size exceeds maximum log size. Default action is truncation is to avoid inflation.
         * @return                      Pointer to the file logger, console logger if cannot find and create logger. NOTE: Please make sure console logger is awake via init().
         */
        static LogPtr getFileLogger(const std::string& filePath, const OverflowAction& overflowAction  = ACTION_TRUNCATE);
        
        /**
         * @brief                       Removes a logger object from the container if exists.
                                        Console logger and default file logger cannot be removed.
                                        This method is to allow applications to get rid of unused logger objects.
                                        This is thread safe because of the usage of shared_ptr.
         * @param    filePath           Path to the file for logger to be removed from the container.
         * @return                      RES_BAD_ARGS if trying to remove console or default logger, RES_ERROR if the logger specified with filePath is not be found in the container. RES_OK if logger is removed.
         */
        static Result destroy(const std::string& filePath);
        
        /**
         * @brief                       Gets number of loggers in the container.
         * @return                      The logger count.
         */
        static size_t getLoggerCount();
        
    private:
        typedef std::unordered_map<std::string, LogPtr> LoggerContainer;
        
        static std::recursive_mutex m_managerMutex;                                     ///< For locking logger creation/deletion in a multi threaded environment
        static LoggerContainer      m_loggers;                                          ///< Holds logger objects distributed to the applications with a key corresponding to file names opened
                                                                                        ///< Console logger has a file name of "", default logger has a file name of "rw_default_log.txt".
        static const std::string    defaultLoggerFilePath;                              ///< Default path to logger object
        static const std::string    consoleLoggerFilePath;                              ///< Path to console logger object
    };
}

#endif /* Logger_h */

