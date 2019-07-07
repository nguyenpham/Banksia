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
    if (state == PlayerState::stopped) {
        return;
    }
    
    tick_idle++;
    
    if (isIdleCrash()) {
        std::string str = name + " is stopped because of being stalled too long!";
        (messageLogger)(getAppName(), str, LogType::system);
        setState(PlayerState::stopped);
        return;
    }
    
    if (tick_deattach > 0) tick_deattach--;

    if (tick_stopping > 0) {
        tick_stopping--;
        
        int exit_status;
        if (tick_stopping == 0
            || (process && process->try_get_exit_status(exit_status)))
            setState(PlayerState::stopped);
        return;
    }
    
    // Ping
    tick_ping++;
    if (tick_ping >= tick_period_ping) {
        resetPing();
        sendPing();
    }
}

bool Engine::isIdleCrash() const
{
    return tick_idle > tick_period_idle_dead;
}

bool Engine::isExited() const
{
    return process == nullptr;
}

void Engine::resetPing()
{
    tick_ping = 0;
}

void Engine::resetIdle()
{
    tick_idle = 0;
}

void Engine::engineSentCorrectCmds()
{
    resetIdle();
    resetPing();
}

void Engine::setMessageLogger(std::function<void(const std::string&, const std::string&, LogType)> logger)
{
    messageLogger = logger;
}

void Engine::log(const std::string& line, LogType logType) const
{
    if (line.empty() || messageLogger == nullptr)
        return;
    (messageLogger)(getName(), line, logType);
}

void Engine::read_stderr(const char *bytes, size_t n)
{
    assert(false); // don't use
}

void Engine::read_stdout(const char *bytes, size_t n)
{
    // check before use since it may be being deleted
    if (!isAttached() || n <= 0) {
        return;
    }
    
    std::vector<std::string> vec;
    auto k = 0;
    for(int i = 0; i < n; i++) {
        if (bytes[i] == '\n') {
            std::string str;
            if (i == 0) {
                str += lastIncompletedStdout;
                lastIncompletedStdout = "";
            }
            
            if (i > k) {
                str += std::string(bytes + k, i + 1 - k);
            }
            
            std::replace(str.begin(), str.end(), '\t', ' ');
            trim(str);
            if (!str.empty()) {
                vec.push_back(str);
            }
            
            k = i + 1;
        }
    }
    
    if (k < n) {
        lastIncompletedStdout = std::string(bytes + k, n - k);
    }
    
    // something wrong, try to do
    if (vec.empty() && lastIncompletedStdout.length() > 1024) {
        vec.push_back(lastIncompletedStdout);
    }
    
    for(auto && line : vec) {
        parseLine(line);
    }
}

void Engine::parseLine(const std::string& line)
{
    log(line, LogType::fromEngine);
    
    auto p = line.find(' ');
    auto cmdString = p == std::string::npos ? line : line.substr(0, p);
    
    auto engineCmdMap = getEngineCmdMap();
    auto it = engineCmdMap.find(cmdString);
    if (it == engineCmdMap.end()) { // bad cmd
        parseLine(-1, cmdString, line);
        return;
    }
    
    engineSentCorrectCmds();
    parseLine(it->second, cmdString, line);
}

bool Engine::kickStart()
{
    assert(config.isValid());
    
    resetPing();
    
    assert(isValid());
    
    if (process == nullptr) {
        setState(PlayerState::none);
        
#if (defined _WIN32) && (defined UNICODE)
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        auto command = converter.from_bytes(config.command);
        auto workingFolder = converter.from_bytes(config.workingFolder);
#else
        auto command = config.command;
        auto workingFolder = config.workingFolder;
#endif
        
        std::thread processThread([=]() {
            TinyProcessLib::Config config;
            config.buffer_size = process_buffer_size;
            TinyProcessLib::Process engineProcess (
                                                   command,
                                                   workingFolder,
                                                   [=](const char *bytes, size_t n) {
                                                       read_stdout(bytes, n);
                                                   }, [=](const char *bytes, size_t n) {
                                                       read_stdout(bytes, n);
                                                   },
                                                   true, config);
            
            process = &engineProcess;
            
            setState(PlayerState::starting);
            write(protocolString());
            
            engineProcess.get_exit_status();
            
            // engine has just exited
            process = nullptr;
            setState(PlayerState::stopped);
        });
        
        processThread.detach();
        return true;
    }
    
    write(protocolString());
    return true;
}

void Engine::attach(ChessBoard* board, const GameTimeController* timeController, std::function<void(const Move&, const std::string&, const Move&, double, EngineComputingState)> moveFunc, std::function<void()> resignFunc)
{
    Player::attach(board, timeController, moveFunc, resignFunc);
    tick_deattach = -1;
    tick_idle = 0;
}

bool Engine::isSafeToDeattach() const
{
    return computingState == EngineComputingState::idle || !isAttached() || tick_deattach == 0;
}

bool Engine::stopThinking()
{
    return stop();
}

bool Engine::isWritable() const
{
    return state > PlayerState::starting && state < PlayerState::stopped;
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

