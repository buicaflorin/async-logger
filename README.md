# async-logger
Simple C++11 asynchronous cross-platform lock-free logger.

 - It uses a fixed size ring buffer which is allocated by the constructor for storing the messages.
 
 - Any number of threads can log at the same.
 
 - The logging function has very small execution time and is non-blocking.
 
 - The worker thread does the actual writing into the file.

# building test app on linux

$cd async-logger

$cmake .

$make

$./async_logger

