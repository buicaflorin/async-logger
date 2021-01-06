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
    uint32_t tmp_index = this->pushIndex.fetch_add(1, std::memory_order_relaxed);
    tmp_index = tmp_index % this->maxFileEntries;

    if (this->logTimestamp) {
        this->entries->at(tmp_index) = this->verbosityStr.at(verbosity) + std::to_string(this->getTimestamp()) + " " + input + '\n';
    }
    else {
        this->entries->at(tmp_index) = this->verbosityStr.at(verbosity) + input + '\n';
    }
    return true;
}

uint64_t Logger::getTimestamp(void) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

void Logger::workerThread(void* self) {
    Logger* owner = (Logger*)self;
    uint32_t currentWriteIndex = 0;
    uint32_t loggedEntries = 0;
    while ((false == owner->exitWorker) || (loggedEntries != owner->pushIndex)) {
        if (owner->pushIndex) {
            while (loggedEntries < owner->pushIndex) {
                *owner->outFile << owner->entries->at(loggedEntries % owner->maxFileEntries);
                loggedEntries++;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    owner->outFile->flush();
    owner->outFile->close();
}

Logger::Logger(uint32_t size, const std::string& filePath, bool timestamp) {
    this->entries = new std::vector<std::string>(size);
    this->maxFileEntries = size;
    this->logTimestamp = timestamp;
    this->outFile = new std::ofstream(filePath);
    this->pushIndex = std::atomic_int(0);
    this->exitWorker = false;
    this->worker = std::thread(&Logger::workerThread, (void*)this);
}

Logger::~Logger() {
    this->exitWorker = true;
    this->worker.join();
    delete this->entries;
}

////////testing

const uint16_t TESTS_NUMBER = 10;

typedef std::pair<Logger::Verbosity, std::string> logmessage_t;
const std::vector<logmessage_t> testMessages = {
{ Logger::Verbosity::INFO, "Lorem ipsum dolor sit amet, consectetur adipiscing" },
{ Logger::Verbosity::WARNING, "elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua."},
{ Logger::Verbosity::ERROR, "Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut"},
{ Logger::Verbosity::INFO, "aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in"},
{ Logger::Verbosity::WARNING, "voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint"},
{ Logger::Verbosity::WARNING, "occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit"},
{ Logger::Verbosity::INFO, "The purpose of lorem ipsum is to create a natural looking block of text"},
{ Logger::Verbosity::INFO, "(sentence, paragraph, page, etc.) that doesn't distract from the layout."},
{ Logger::Verbosity::INFO, "A practice not without controversy, laying out pages with meaningless filler"},
{ Logger::Verbosity::WARNING, "text can be very useful when the focus is meant to be on design, not content."}
};

typedef struct {
    Logger* logger;
    const std::vector<logmessage_t>* messages;
    std::atomic_uint current_msg_index;
}workItems_t;

void testLogging(void* pWorkItems) {
    workItems_t* work_items = (workItems_t*)pWorkItems;
    uint32_t test_index = work_items->current_msg_index;

    Logger::Verbosity verbosity = work_items->messages->at(work_items->current_msg_index).first;
    const std::string& message = work_items->messages->at(work_items->current_msg_index).second;

    work_items->logger->logMessage(verbosity, message);
    work_items->current_msg_index = (work_items->current_msg_index + 1) % TESTS_NUMBER;
}

int main()
{
    std::vector<std::thread> testThreads;

    Logger* logger = new Logger(100, std::string("log.txt"), true);
    workItems_t work = { logger, &testMessages, 0};

    for (int index = 0; index < TESTS_NUMBER; index++) {
       testThreads.push_back(std::thread(testLogging, &work));
    }

    for (int index = 0; index < TESTS_NUMBER; index++) {
        testThreads.at(index).join();
    }

    delete work.logger;
    return 0;
}