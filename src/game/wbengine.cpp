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

#include "wbengine.h"

using namespace banksia;

std::string WbEngine::protocolString() const
{
    return "xboard\nprotover 2";
}

bool WbEngine::sendOptions()
{
    if (!isReady()) {
        return false;
    }
    
    return true;
}

void WbEngine::newGame()
{
    computingState = EngineComputingState::idle;
    write("new");
    
    if (!board->fromOriginPosition()) {
        write("setboard " + board->getStartingFen());
    }
    
    if (!board->histList.empty()) {
        for (auto && hist : board->histList) {
            write(hist.move.toCoordinateString());
        }
    }
}

void WbEngine::prepareToDeattach()
{
    stop();
}

bool WbEngine::sendQuit()
{
    return write("quit");
}

bool WbEngine::stop()
{
    return write("stop");
}


bool WbEngine::goPonder(const Move& pondermove)
{
    Engine::go(); // just for setting flags
    return false;
}

bool WbEngine::go()
{
    Engine::go();
    computingState = EngineComputingState::thinking;

    return write(timeControlString()) && write("go");
}

std::string WbEngine::timeControlString() const
{
    switch (timeController->mode) {
        case TimeControlMode::infinite:
            return "analyze";
            
        case TimeControlMode::depth:
            return "sd " + std::to_string(timeController->depth);
            
        case TimeControlMode::movetime:
            return "st " + std::to_string(timeController->time);
            
        case TimeControlMode::standard:
        {
            // timeController unit: second
            auto sd = static_cast<int>(board->side);
            auto time = int(timeController->getTimeLeft(sd)), m = time / 60, s = time % 60;
            auto timeString = std::to_string(m) + (s > 0 ? ":" + std::to_string(s) : "");
            
            int inc = int(timeController->increment);
            
            auto n = timeController->moves;
            auto halfCnt = int(board->histList.size());
            int fullCnt = halfCnt / 2;
            int curMoveCnt = fullCnt % n;
            int movestogo = n - curMoveCnt;
            
            // level 40 0:30 0
            std::string str = "level " + std::to_string(movestogo)
            + " " + timeString
            + " " + std::to_string(inc)
            ;
            return str;
        }
        default:
            break;
    }
    return "";
}

bool WbEngine::sendPing()
{
    return write("ping " + std::to_string(pingCnt++));
}

bool WbEngine::sendPong(const std::string& str)
{
    return write("pong " + str);
}

void WbEngine::tickWork()
{
    Engine::tickWork();
}

void WbEngine::parseLine(const std::string& str)
{
    log(str, LogType::fromEngine);
    
    auto vec = splitString(str, ' ');
    if (vec.empty()) {
        return;
    }
    auto cmd = vec.front();
    
    auto ch = cmd.front();
    
    if (isdigit(ch)) { // it may be thinking output -> ignore
        // 9 156 1084 48000 Nf3 Nc6 Nc3 Nf6
        return;
    }
    
    if (cmd == "move") {
        if (vec.size() < 2) { // something wrong
            return;
        }
        
        auto timeCtrl = timeController;
        auto moveRecv = moveReceiver;
        if (timeCtrl == nullptr || moveRecv == nullptr) {
            return;
        }

        auto oldComputingState = computingState;
        computingState = EngineComputingState::idle;
        auto period = timeCtrl->moveTimeConsumed(); // moveTimeConsumed();

        auto moveString = vec.at(1);
        auto ponderMoveString = "";
        
        if (!moveString.empty() && moveReceiver != nullptr) {
            (moveRecv)(moveString, ponderMoveString, period, oldComputingState);
        }
        return;
    }
    
    if (cmd == "ping") {
        sendPong(vec.size() >= 2 ? vec.at(1) : "");
        return;
    }
    
}

