# rwlogger
A naive multi-threaded logger class in C++.

rwlogger folder consists of two folders and a readme file. **src** folder contains implementation and include files for both Logger class and the unit tests conducted. **build** folder contains project files for Windows and macOS operating systems. An executable which runs unit tests compiled in Windows is also provided in *x64/Release* folder.

I had implemented and debugged in macOS using Xcode 10.1. Logger class and unit test executable can therefore be built using provided Xcode project files. I also provide a solution file which I created using Visual Studio 2015 community edition. The class and the unit test executable can also be built in Windows.

Project does not depend on any third party dependencies. I implemented unit tests using assertions instead of using a framework in order not to complicate things.

I would like to share a little bit from the design choices and the implementation details. More detailed information can be found in the comments for the method definitions and the variables of the class. Logger class encapsulates three public enum types which are **Level**, **OverflowAction** and **Result**. Level is used to specify the importance of log messages. A logger object holds a level state which filters log messages arriving according to their level of importance. OverflowAction is used to define the action to be taken when the current size of the file exceeds maximum size set for the logger. **NoAction**, **Truncate** and **Rotate** are possible actions. These actions are set during creation of the logger and cannot be updated. Therefore, different threads are not able to truncate or rotate the same log file simultaneously. As a summary; truncation is the process of discarding old messages from the file while maintaining some size of new messages. Therefore, it may not be possible to check very old log messages in truncation mode. Moreover, rotation is the process of flushing the content of the file to a new file. Finally, Result is used to control flow of execution between the functions of the class and the outside world.

Creation of Logger objects using class constructors is prohibited to outside world to follow a manager design pattern. Here, I implemented manager inside the Logger class. This pattern is used to control multiple entities of the same type. For example, If I would allow object creation, different threads may create Logger objects of the same file with different OverflowAction which possibly leads garbage log files to be created. We use an unoredered_map as the container for the loggers with the keys of file paths. Our manager allows three different Logger types to be created and cached for the use of outside world. These are the console logger with key "", default file logger with key "rw_default_log.txt" and custom file logger with key provided by the user. File loggers can also output to the console using a setter. Users are adviced to ensure that at least the console logger is created without an error via Logger interface, so that in case of an error during creation of file loggers, all messages are directed to the console. In order not to leave dangling, users are adviced to destroy custom Logger objects which they retrieved. Console and default file Logger objects cannot be destroyed once they are created. Removing of loggers from the container is thread safe for different threads logging to same file because of the usage of shared_ptr.

## List of shortcomings, know issues and future works

- OverflowAction::ACTION_NONE allows users to create log files with size which is much greater than maximum log size. It is users responsibility currently to handle enormous sizes.

- Logger manager does not keep track any information about the rotated files. Users have to manually check these files.

- Truncation operation (most probably) also truncate first message of the logs which will be kept. I do not give much importance to this, but I am aware of it.

- Rotation files are created with names including current date and time. The precision for these file names is up to milliseconds. It is not much likely to occur, but if two rotations are executed in a millisecond, then the former file would be overwritten.

- My implementation directly logs to the output streams (file or console). In order to achieve thread safety, mutexes are used to protect critical sections which are write operations. Therefore, I/O operation of one thread can make other thread wait for a different I/O operation as well. A possibly more efficient implementation could cache the messages in RAM and flush after a time interval. However, this implementation could lack from real-time log updates.

- In the future, Logger class may have the interface for querying log messages, for example within a time interval or for an importance level.

- In the future, Logger class may support different types of output streams for well defined use cases.
