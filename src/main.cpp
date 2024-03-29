//
//  main.cpp
//  rwlogger
//
//  Created by Tayfun Ateş on 8.02.2019.
//  Copyright © 2019 Tayfun Ateş. All rights reserved.
//

#include <iostream>
#include "Logger.h"
#include <fstream>
#include <thread>
#include <chrono>
#include <iomanip>
#include <assert.h>

using namespace rw;

void TEST_init()
{
    auto res = Logger::init();
    assert (res == Logger::RES_OK);
}

void TEST_getAndDestroyLoggers()
{
    auto consoleLogger = Logger::getConsoleLogger();
    assert(consoleLogger);
    auto defaultLogger = Logger::getDefaultLogger();
    assert(defaultLogger);
    auto customLogger = Logger::getFileLogger("customLogger.log");
    assert(customLogger);
    
    //Removing default logger should be impossible
    assert(Logger::destroy("rw_default_log.txt") == Logger::RES_BAD_ARGS);
    
    //Removing console logger should be impossibe
    assert(Logger::destroy("") == Logger::RES_BAD_ARGS);
    
    //Custom file loggers can be removed from our container
    assert(Logger::destroy("customLogger.log") == Logger::RES_OK);
    //Retrieving again should not be a problem
    customLogger = Logger::getFileLogger("customLogger.log");
    assert(customLogger);
    //Keep it clean
    assert(Logger::destroy("customLogger.log") == Logger::RES_OK);
    
    assert(Logger::destroy("thisLoggerIsNotInTheContainer.log") == Logger::RES_ERROR);
    
    remove("customLogger.log");
    remove("rw_default_log.txt");
}

size_t getFileSize(const std::string& filePath)
{
    std::fstream inFile;
    inFile.open( filePath.c_str(), std::ios::in);
    if( inFile.is_open() ) {
        inFile.seekg(0, std::ios::end );
        size_t size = (size_t) inFile.tellg();
        inFile.close();
        return size;
    }
    return 0;
}

void TEST_enableDisable()
{
    const std::string testFile = "TEST_enableDisable.log";
    const std::string errorMessage = "Error Occured";
    
    auto customLogger = Logger::getFileLogger(testFile);
    customLogger->setEnabled(false);
    customLogger->operator()(Logger::LOG_LEVEL_ERROR) << errorMessage;
    assert(getFileSize(testFile) == 0);
    customLogger->setEnabled(true);
    customLogger->operator()(Logger::LOG_LEVEL_ERROR) << errorMessage;
    assert(getFileSize(testFile) > 0);
    
    Logger::destroy(testFile);
    remove(testFile.c_str());
}

void TEST_logLevel()
{
    const std::string testFile = "TEST_logLevel.log";
    const std::string warningMessage = "Warning Occured";
    
    auto customLogger = Logger::getFileLogger(testFile);
    customLogger->setLogLevel(Logger::LOG_LEVEL_ERROR);
    customLogger->operator()(Logger::LOG_LEVEL_WARNING) << warningMessage;
    assert(getFileSize(testFile) == 0);
    customLogger->setLogLevel(Logger::LOG_LEVEL_WARNING);
    customLogger->operator()(Logger::LOG_LEVEL_WARNING) << warningMessage;
    assert(getFileSize(testFile) > 0);
    
    Logger::destroy(testFile);
    remove(testFile.c_str());
}

class GoodPoint
{
private:
    double m_x, m_y, m_z;
    
public:
    GoodPoint(double x=0.0, double y=0.0, double z=0.0): m_x(x), m_y(y), m_z(z)
    {
    }
    
    friend std::ostream& operator<< (std::ostream &out, const GoodPoint &point)
    {
            out << "Point(" << point.m_x << ", " << point.m_y << ", " << point.m_z << ")"; // actual output done here
            return out;
    }
};

void TEST_userDefinedTypes()
{
    const std::string testFile = "TEST_userDefinedTypes.log";
    LOGF(Logger::LOG_LEVEL_WARNING, testFile) << GoodPoint(1.0f, 2.0f, 3.0f);
    assert(getFileSize(testFile) > 0);
    
    Logger::destroy(testFile);
    remove(testFile.c_str());
}

void TEST_truncation()
{
    const std::string testFile = "TEST_truncation";
    const size_t longStringSize = 200;
    const std::string longString = std::string(longStringSize, 'a');
    const size_t maxSize = 2048;
    
    auto customLogger = Logger::getFileLogger(testFile, Logger::ACTION_TRUNCATE);
    customLogger->setMaxLogSize(maxSize);
    
    const size_t numberOfTrials = 100;
    
    for(size_t i=0; i < numberOfTrials; i++) {
        //For each trial file size should not exceed maxSize + longStringSize + 45-> We truncate in the next round
        customLogger->operator()(Logger::LOG_LEVEL_WARNING) << longString;
        assert(getFileSize(testFile) <= maxSize+longStringSize+45); //approximate header size (45)
    }
    
    Logger::destroy(testFile);
    remove(testFile.c_str());
}

void TEST_rotate()
{
    const std::string testFile = "TEST_rotate";
    const size_t longStringSize = 200;
    const std::string longString = std::string(longStringSize, 'a');
    const size_t maxSize = 2048;
    
    auto customLogger = Logger::getFileLogger(testFile, Logger::ACTION_ROTATE);
    customLogger->setMaxLogSize(maxSize);
    
    const size_t numberOfTrials = 100;
    
    bool willRotate = false;
    for(size_t i=0; i < numberOfTrials; i++) {
        customLogger->operator()(Logger::LOG_LEVEL_WARNING) << longString;
        const size_t fileSize = getFileSize(testFile);
        if(willRotate == true)
        {
            assert(fileSize <= longStringSize+45); //If rotation decision has been made in the previous round, then the size should be smaller than the size of the newly added log message plus approximate header size (45)
            willRotate = false;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        
        if(fileSize>maxSize)
        {
            willRotate = true;
        }
    }
    
    Logger::destroy(testFile);
    remove(testFile.c_str());
    
    //TO DO: Check and remove rotated files
    //Logger does not keep track of the rotated files currently.
}

void loggerCreationDestructionThread( int loopCount )
{
    for (int i=0;i<loopCount;++i)
    {
        int id = rand() & 0xFF;
        std::stringstream ss;
        ss << id << ".log";
        const std::string logFile = ss.str();
        Logger::getFileLogger(logFile);
        Logger::destroy(logFile);
    }
}

void TEST_multithreadedCreationAndDestruction()
{
    const int threadCnt = 8;
    std::thread* threads[threadCnt];
    
    // Run creater/destoyer threads.
    for ( int i=0;i<threadCnt;++i)
    {
        threads[i] = new std::thread( loggerCreationDestructionThread, 1000 );
    }
    // Wait for threads
    for ( int i=0;i<threadCnt;++i)
    {
        threads[i]->join();
        delete threads[i];
    }
    
    //8 threads created and destroyed 1000 loggers possibly looking the same file
    //None of the action should lead cause crash
    //There should be 2 loggers remaining which is the console logger and the default file logger
    assert(Logger::getLoggerCount() == 2);
}

void loggerWriterThread( bool retainTemporarily, std::string loggerId, int loopCount )
{
    if (retainTemporarily)
    {
        for (int i=0;i<loopCount;++i)
        {
            LOGF(Logger::LOG_LEVEL_ERROR, loggerId) << std::setw(2) << std::setfill('0') << i;
        }
    }
    else
    {
        Logger::LogPtr ptr = Logger::getFileLogger( loggerId );
        for (int i=0;i<loopCount;++i)
        {
            ptr->operator()(Logger::LOG_LEVEL_ERROR) << std::setw(2) << std::setfill('0') << i;
        }
    }
}

void TEST_multithreadedDestructionWhileInUse()
{
    const std::string testFile = "TEST_multithreadedDestructionWhileInUse";
    // Create a writer thread and destruct while in use
    // Destroy a fully thread-retained logger while in use by the thread
    // It must not crash
    {
        std::thread thread( loggerWriterThread, false, testFile, 100 );
        std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) ); // wait a while
        Logger::destroy( testFile ); // and destroy while the writer thread is (most probably) still active
        thread.join(); // wait writer thread
    }
    // Destroy a temporarily thread-retained logger while in use by the thread
    // It must not crash
    {
        std::thread thread( loggerWriterThread, true, testFile, 100 );
        std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) ); // wait a while
        Logger::destroy( testFile ); // and destroy while the writer thread is (most probably) still active
        thread.join(); // wait writer thread
    }
    
    Logger::destroy(testFile);
    remove(testFile.c_str());
}

void TEST_multithreadedMultipleThreadsSingleFile()
{
    const std::string testFile = "TEST_multithreadedMultipleThreadsSingleFile";
    auto customLogger = Logger::getFileLogger(testFile, Logger::ACTION_NONE);

    // Create multiple writers to the same logger
    const int threadCnt = 8;
    std::thread* threads[threadCnt];
    for ( int i=0;i<threadCnt;++i)
    {
        threads[i] = new std::thread( loggerWriterThread, true, testFile, 100 );
    }
    for ( int i=0;i<threadCnt;++i)
    {
        threads[i]->join();
        delete threads[i];
    }
    
#ifdef _MSC_VER
	const size_t expectedLineSize = 52;
#else
	const size_t expectedLineSize = 51;
#endif
    assert(getFileSize(testFile) == threadCnt * 100 /*Thread function iteration*/ * expectedLineSize /*Size per log*/);
    
    Logger::destroy(testFile);
    remove(testFile.c_str());
}

int main(int argc, const char * argv[]) {
    
    TEST_init();
    TEST_getAndDestroyLoggers();
    TEST_enableDisable();
    TEST_logLevel();
    TEST_truncation();
    //TEST_rotate(); --> Creates multiple files, disabled for now.
    TEST_multithreadedCreationAndDestruction();
    TEST_multithreadedDestructionWhileInUse();
    TEST_multithreadedMultipleThreadsSingleFile();
    
    std::cout << "Tests are completed without an error!" << std::endl;
    
    return 0;
}
