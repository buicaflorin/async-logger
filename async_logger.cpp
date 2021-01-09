/*
MIT License

Copyright (c) 2021 Florin Buica

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#include <chrono>
#include <async_logger.h>

bool Logger::logMessage(enum Verbosity verbosity, const std::string& input) {
    uint32_t tmpIndex = this->pushIndex.fetch_add(1, std::memory_order_relaxed);

    //check for ring buffer overflow
    if(tmpIndex > (this->loggedEntries + this->requestedLogSlots)){
        std::cerr << "Log buffer is full! Consider increasing its size.\n";
        return false;
    }
    tmpIndex = tmpIndex % this->requestedLogSlots;
    if (this->logTimestamp) {
        this->entries->at(tmpIndex) = this->verbosityStr.at(verbosity) + std::to_string(this->getTimestamp()) + " " + input + '\n';
    }
    else {
        this->entries->at(tmpIndex) = this->verbosityStr.at(verbosity) + input + '\n';
    }
    return true;
}

uint64_t Logger::getTimestamp(void) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

void Logger::workerThread(void* self) {
    Logger* owner = (Logger*)self;
    uint32_t currentWriteIndex = 0;
    owner->loggedEntries = 0;
    while ((false == owner->exitWorkerSignal) || (owner->loggedEntries != owner->pushIndex)) {
        if (owner->pushIndex) {
            while (owner->loggedEntries < owner->pushIndex) {
                *owner->outFile << owner->entries->at(owner->loggedEntries % owner->requestedLogSlots);
                owner->loggedEntries++;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    owner->outFile->flush();
    owner->outFile->close();
}

Logger::Logger(uint32_t size, const std::string& filePath, bool timestamp) {
    if (size < MAX_LOG_SIZE) {
        this->entries = new std::vector<std::string>(size);
        this->requestedLogSlots = size;
    }
    else {
        throw std::runtime_error("Size exceeds maximum configured.");
    }
    this->logTimestamp = timestamp;
    this->outFile = new std::ofstream(filePath);
    if (this->outFile->fail()) {
        throw std::runtime_error("Failed to create log file.");
    }
    this->pushIndex = std::atomic_int(0);
    this->loggedEntries = std::atomic_int(0);
    this->exitWorkerSignal = false;
    this->worker = std::thread(&Logger::workerThread, (void*)this);
}

Logger::~Logger() {
    this->exitWorkerSignal = true;
    this->worker.join();
    delete this->entries;
}
