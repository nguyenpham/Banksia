/*
 This file is part of Banksia, distributed under MIT license.
 
 Copyright (c) 2019 Nguyen Hong Pham
 
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

#include <locale>
#include <codecvt>

#include "engine.h"

using namespace banksia;

////////////////////////////////////
void Engine::tickWork()
{
    if (tick_stopping > 0) {
        tick_stopping--;
        
        int exit_status;
        if (tick_stopping == 0
            || (process && process->try_get_exit_status(exit_status)))
            setState(PlayerState::stopped);
        return;
    }
    
    if (!isReady()) {
        return;
    }
    
    tick_for_ping--;
    if (tick_for_ping < 0) {
        resetPing();
        sendPing();
    }
}

void Engine::resetPing()
{
    tick_for_ping = tick_ping;
}

void Engine::resetIdle()
{
    tick_idle = 0;
}

bool Engine::isReady() const
{
    return state > PlayerState::starting && state < PlayerState::stopped;
}

void Engine::addMessageLogger(std::function<void(const std::string& string, LogType logType)> logger)
{
    messageLogger = logger;
}

void Engine::log(const std::string& line, LogType logType) const
{
    if (line.empty() || messageLogger == nullptr)
        return;
    (messageLogger)(line, logType);
}

void Engine::read_stderr(const std::string& str)
{
    std::cout << "read_stderr: " << str << std::endl;
}

void Engine::read_stdout(const std::string& str)
{
    resetPing();
    resetIdle();

    auto vec = splitString(str, '\n');
    for(auto && line : vec) {
        if (!line.empty())
            parseLine(line);
    }
}

bool Engine::kickStart()
{
    assert(config.isValid());

    setState(PlayerState::starting);
    resetPing();
    resetIdle();
    
    if (process == nullptr) {
        
#if (defined _WIN32) && (defined UNICODE)
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        auto command = converter.from_bytes(config.command);
        auto workingFolder = converter.from_bytes(config.workingFolder);
#else
        auto command = config.command;
        auto workingFolder = config.workingFolder;
#endif

        std::thread processThread([=]() {
            TinyProcessLib::Process engineProcess (
                                                   command,
                                                   workingFolder,
                                                   [=](const char *bytes, size_t n) {
                                                       read_stdout(std::string(bytes, n));
                                                   }, [=](const char *bytes, size_t n) {
                                                       read_stdout(std::string(bytes, n));
                                                   },
                                                   true);
            
            process = &engineProcess;
            
            write(protocolString());
            
            auto exit_status = engineProcess.get_exit_status();
            
            // engine exited
            process = nullptr;
            setState(PlayerState::stopped);
        });

        processThread.detach();
        return true;
    }
    
    write(protocolString());
    return true;
}

bool Engine::stopThinking()
{
    return stop();
}

bool Engine::write(const std::string& str)
{
    if (state >= PlayerState::starting && state < PlayerState::stopped && process) {
        process->write(str + "\n");
        log(str, LogType::toEngine);
        return true;
    }
    return false;
}

bool Engine::quit()
{
    sendQuit();
    setState(PlayerState::stopping);
    tick_stopping = 6; // 3 second
    return true;
}

bool Engine::kill()
{
    if (process) {
        process->kill();
        delete process;
        process = nullptr;
    }
    
    setState(PlayerState::stopped);
    return true;
}
