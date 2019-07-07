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

const std::unordered_map<std::string, int> WbEngine::wbEngineCmd {
    { "feature",    static_cast<int>(WbEngine::WbEngineCmd::feature) },
    { "move",       static_cast<int>(WbEngine::WbEngineCmd::move) },
    { "resign",     static_cast<int>(WbEngine::WbEngineCmd::resign) },
    { "offer",      static_cast<int>(WbEngine::WbEngineCmd::offer) },
    
    { "illegal",    static_cast<int>(WbEngine::WbEngineCmd::illegal) },
    { "Illegal",    static_cast<int>(WbEngine::WbEngineCmd::illegal) },
    { "Error",      static_cast<int>(WbEngine::WbEngineCmd::error) },

    { "ping",       static_cast<int>(WbEngine::WbEngineCmd::ping) },
    { "pong",       static_cast<int>(WbEngine::WbEngineCmd::pong) },

    { "tellopponent", static_cast<int>(WbEngine::WbEngineCmd::tellopponent) },
    { "tellothers", static_cast<int>(WbEngine::WbEngineCmd::tellothers) },
    { "tellall",    static_cast<int>(WbEngine::WbEngineCmd::tellall) },
    { "telluser",    static_cast<int>(WbEngine::WbEngineCmd::telluser) },
    { "tellusererror",    static_cast<int>(WbEngine::WbEngineCmd::tellusererror) },
    { "tellicsnoalias", static_cast<int>(WbEngine::WbEngineCmd::tellicsnoalias) },
};

const std::unordered_map<std::string, int>& WbEngine::getEngineCmdMap() const
{
    return wbEngineCmd;
}


std::string WbEngine::protocolString() const
{
    return "xboard\nprotover 2";
}

bool WbEngine::sendOptions()
{
    return true;
}

void WbEngine::newGame()
{
    computingState = EngineComputingState::idle;
    
    write("force"); // we don't want engine start calculating right now
    write(ponderMode ? "hard" : "easy");
    write("post"); // write("nopost");
    write("new");
    
    if (!variantSet.empty()) {
        if (variantSet.find("normal") == variantSet.end()) {
            (resignFunc)();
            return;
        }
        write("variant normal");
    }
    
    write("force"); // we don't want engine start calculating right now
    if (!board->fromOriginPosition()) {
        write("setboard " + board->getStartingFen());
    }
    
    if (!board->histList.empty()) {
        write("force"); // we don't want engine start calculating right now
        for (auto && hist : board->histList) {
            write(hist.move.toCoordinateString());
        }
    }
}

void WbEngine::prepareToDeattach()
{
    if (tick_deattach >= 0) return;
    stop();
    tick_deattach = tick_period_deattach;
}

bool WbEngine::sendQuit()
{
    return write("quit");
}

bool WbEngine::stop()
{
    return write("force");
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
    
    if (getState() == PlayerState::starting &&
        tick_delay_2_ready > 0) {
        tick_delay_2_ready--;
        if (tick_delay_2_ready == 0) {
            setState(PlayerState::ready);
        }
    }
}

bool WbEngine::isIdleCrash() const
{
    // if engine send feature done=0 (feature_done_finished = false), it can wait longer
    return !feature_done_finished && tick_idle > tick_period_idle_dead;
}


bool WbEngine::isFeatureOn(const std::string& featureName, bool defaultValue)
{
    auto p = featureMap.find(featureName);
    return p == featureMap.end() ? defaultValue : p->second == "1";
}

std::string WbEngine::parseFeatures(const std::string& line)
{
    std::string str;
    auto vec = splitString(line, ' ');
    
    for(int i = 1; i < vec.size(); i++) {
        auto s = vec.at(i);
        auto p = s.find('=');
        if (p == std::string::npos) {
            continue;
        }
        auto s0 = s.substr(0, p), s1 = s.substr(p + 1);
        if (!s0.empty() && !s1.empty()) {
            if (s0 == "san") {
                feature_san = s1 == "1";
            } else if (s0 == "usermove") {
                feature_usermove = s1 == "1";
            } else if (s0 == "setboard") {
                feature_setboard = s1 == "1";
            } else if (s0 == "variants") {
                size_t pos;
                // Search for the substring in string in a loop untill nothing is found
                while ((pos  = s1.find("\"") )!= std::string::npos) {
                    s1.erase(pos, 1);
                }

                auto varList = splitString(s1, ',');
                for(auto && s : varList) {
                    trim(s);
                    if (!s.empty()) {
                        variantSet.insert(s);
                    }
                }
            } else if (s0 == "done") {
                if (s1 == "0") {
                    tick_delay_2_ready = 60 * 60 * 2; // 1h
                    feature_done_finished = false;
                } else {
                    setState(PlayerState::ready);
                    feature_done_finished = true;
                }
                continue;
            }
            if (!str.empty()) str += "\n";
            str += "accepted " + s0;

            featureMap[s0] = s1;
        }
    }

    return str;
}

bool WbEngine::oppositeMadeMove(const Move& move, const std::string& sanMoveString)
{
    write("force"); // we don't want this engine starts calculating after this move

    std::string str;
    if (feature_usermove) {
        str += "usermove ";
    }

    if (feature_san) {
        str += sanMoveString;
    } else {
        str += move.toCoordinateString();
    }
    return write(str);
}

bool WbEngine::engineMove(const std::string& moveString, bool mustSend)
{
    if (moveString.length() < 2 || !isalpha(moveString.at(0))
        || computingState != EngineComputingState::thinking) { // some engines don't orbey 'force' and create move too fast -> ignore some cases
        return false;
    }
    
    auto timeCtrl = timeController;
    auto moveRecv = moveReceiver;
    if (timeCtrl == nullptr || moveRecv == nullptr) {
        return false;
    }
    
    auto move = board->moveFromCoordiateString(moveString);
    if (!move.isValid()) {
        move = board->fromSanString(moveString);
    }

    if (mustSend || move.isValid()) {
        auto period = timeCtrl->moveTimeConsumed(); // moveTimeConsumed();
        
        auto oldComputingState = computingState;
        computingState = EngineComputingState::idle;

        (moveRecv)(move, moveString, Move::illegalMove, period, oldComputingState);

        return true;
    }

    return false;
}

void WbEngine::parseLine(int cmdInt, const std::string& cmdString, const std::string& line)
{
    if (cmdInt < 0) {
        if (state != PlayerState::playing) {
            return;
        }
        auto ch = line.front();
        if (isdigit(ch)) { // it may be thinking output -> ignore
            // 9 156 1084 48000 Nf3 Nc6 Nc3 Nf6
            engineSentCorrectCmds();
        }
        return;
    }
    
    if (tick_delay_2_ready < 0) tick_delay_2_ready = 3;
    
    auto cmd = static_cast<WbEngineCmd>(cmdInt);
    switch (cmd) {
        case WbEngineCmd::move:
        {
            auto vec = splitString(line, ' ');
            if (vec.size() < 2) { // something wrong
                return;
            }

            engineMove(vec.at(1), true);
            break;
        }
            
        case WbEngineCmd::feature:
        {
            tick_delay_2_ready = std::max(3, tick_delay_2_ready);
            auto str = parseFeatures(line);
            if (!str.empty()) {
                write(str);
            }
            break;
        }
            
        case WbEngineCmd::ping:
        {
            auto vec = splitString(line, ' ');
            sendPong(vec.size() >= 2 ? vec.at(1) : "");
            break;
        }

        case WbEngineCmd::resign:
        {
            if (resignFunc != nullptr) {
                (resignFunc)();
            }
            break;
        }
            
        default:
            break;
    }
}
