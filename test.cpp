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
#include "async_logger.h"

const uint16_t TESTS_THREADS_NUMBER = 10;

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
    std::shared_ptr<Logger> logger;
    const std::vector<logmessage_t>* messages;
    std::atomic_uint current_msg_index;
}workItems_t;

void testLogging(void* pWorkItems) {
    workItems_t* work_items = (workItems_t*)pWorkItems;
    uint32_t test_index = work_items->current_msg_index;

    Logger::Verbosity verbosity = work_items->messages->at(work_items->current_msg_index).first;
    const std::string& message = work_items->messages->at(work_items->current_msg_index).second;

    work_items->logger->logMessage(verbosity, message);
    work_items->current_msg_index = (work_items->current_msg_index + 1) % TESTS_THREADS_NUMBER;
}

int main()
{
    std::vector<std::thread> testThreads;

    std::shared_ptr<Logger> logger(new Logger(100, std::string("log.txt"), true));
    workItems_t work = { logger, &testMessages};

    int index;
    for (index = 0; index < TESTS_THREADS_NUMBER; index++) {
       testThreads.push_back(std::thread(testLogging, &work));
    }

    //add some more entries from the current thread
    for (index = 0; index < 100; index++) {
        testLogging(&work);
    }

    for (index = 0; index < TESTS_THREADS_NUMBER; index++) {
        testThreads.at(index).join();
    }
    return 0;
}
