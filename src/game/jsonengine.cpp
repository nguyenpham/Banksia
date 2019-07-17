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


#include <regex>
#include <map>

#include "jsonengine.h"

using namespace banksia;

JsonEngine::JsonEngine()
{
}

JsonEngine::~JsonEngine()
{
}

JsonEngine::JsonEngine(const Config& config)
: Engine(config)
{
    assert(!config.command.empty());
    
    originalProtocol = config.protocol;
    if (config.protocol == Protocol::none) {
        this->config.protocol = Protocol::uci;
    }
}

void JsonEngine::setupEngine()
{
    if (config.protocol == Protocol::uci) {
        engine = &uciEngine;
    } else {
        engine = &wbEngine;
    }
    
    engine->config = config;
    engine->setState(PlayerState::starting);
}

//void JsonEngine::log(const std::string& line, LogType logType) const
//{
////    std::cout << (logType == LogType::fromEngine ? "read: " : "write: ") << line << std::endl;
//}

bool JsonEngine::isIdleCrash() const
{
    return false;
}

std::string JsonEngine::protocolString() const
{
    return engine->protocolString();
}

bool JsonEngine::isAttached() const
{
    return true;
}

bool JsonEngine::sendQuit()
{
    return true;
}

bool JsonEngine::sendPing()
{
    return true;
}

void JsonEngine::kickStart(std::function<void(Config* config)> _taskComplete)
{
    jsonstate = JsonEngineState::working;
    taskComplete = _taskComplete;
    
    setupEngine();
    tick_test = tick_test_period;
    
    Engine::kickStart();
}

const std::unordered_map<std::string, int>& JsonEngine::getEngineCmdMap() const
{
    return engine->getEngineCmdMap();
}

void JsonEngine::parseLine(int cmdInt, const std::string& cmdString, const std::string& line)
{
    if (cmdInt >= 0) {
        usedCmdSet.insert(cmdString);
        engine->parseLine(cmdInt, cmdString, line);
    }
}

void JsonEngine::completed(Config* config)
{
    (taskComplete)(config);
    quit();
    kill();
    jsonstate = JsonEngineState::done;
}

void JsonEngine::tickWork()
{
    Engine::tickWork();
    
    if (jsonstate == JsonEngineState::done) {
        return;
    }
    
    auto st = engine->getState();
    if (st == PlayerState::stopped) {
        return completed(nullptr);
    }
    
    if (st == PlayerState::ready) {
        if (correctCmdCnt == 0) { // hm, wait for a pong (in case of wb)
            return;
        }
        completed(&engine->config);
        return;
    }
    
    // wb need tickWork to turn ready
    if (config.protocol == Protocol::wb) {
        engine->tickWork();
    }
    
    tick_test--;
    if (tick_test > 0) {
        return;
    }
    
    if ((correctCmdCnt > 6 && usedCmdSet.size() > 2) || (correctCmdCnt > 3 && usedCmdSet.find("feature") != usedCmdSet.end())) {
        completed(&engine->config);
        return;
    }
    
    if (tryNum > 0 && correctCmdCnt > 2) {
        tryNum--;
        tick_test = tick_test_period;
        return;
    }
    
    if (originalProtocol != Protocol::none || config.protocol == Protocol::wb) {
        completed(nullptr);
        return;
    }
    
    quit();
    
    config.protocol = Protocol::wb;
    setupEngine();
    
    usedCmdSet.clear();
    correctCmdCnt = 0;
    tick_idle = 0;
    tick_test = tick_test_period;
    tryNum = 3;
    write(engine->protocolString());
}


