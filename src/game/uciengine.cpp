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

#include "uciengine.h"

using namespace banksia;


std::string UciEngine::protocolString() const
{
    return "uci";
}

bool UciEngine::sendOptions()
{
    if (!isReady()) {
        return false;
    }
    
    for(auto && o : config.optionList) {
        if (o.isDefaultValue()) {
            continue;
        }
        
        std::string str = "setoption name " + o.name + " value ";
        switch (o.type) {
            case OptionType::spin:
                str += std::to_string(o.value);
                break;
            case OptionType::check:
                str += o.checked ? "true" : "false";
                break;
            default: // combo, string
                str += o.string;
                break;
        }
        write(str);
    }
    return true;
}

void UciEngine::newGame()
{
    ponderingMove = Move::illegalMove;
    expectingBestmove = false;
    computingState = EngineComputingState::idle;
    write("ucinewgame");
}

bool UciEngine::sendQuit()
{
    std::string str = "quit";
    return write(str);
}

bool UciEngine::stop()
{
    if (expectingBestmove) {
        std::string str = "stop";
        return write(str);
    }
    return false;
}


bool UciEngine::goPonder(const Move& pondermove)
{
    assert(!expectingBestmove && computingState == EngineComputingState::idle);
    
    Engine::go(); // just for setting flags
    ponderingMove = Move::illegalMove;
    
    if (config.ponderable && pondermove.isValid()) {
        ponderingMove = pondermove;
        
        auto goString = getGoString(pondermove);
        assert(goString.find("ponder") != std::string::npos);
        expectingBestmove = true;
        computingState = EngineComputingState::pondering;
        return write(goString);
    }
    return false;
}

bool UciEngine::go()
{
    Engine::go();
    ponderingMove = Move::illegalMove;
    
    // Check for ponderhit
    if (computingState == EngineComputingState::pondering) {
        assert(expectingBestmove);
        if (!board->histList.empty() && board->histList.back().move == ponderingMove) {
            computingState = EngineComputingState::thinking;
            write("ponderhit");
            return true;
        }
        return stop();
    }

    assert(!expectingBestmove && computingState == EngineComputingState::idle);
    auto goString = getGoString(Move::illegalMove);
    expectingBestmove = true;
    computingState = EngineComputingState::thinking;
    return write(goString);
}

std::string UciEngine::getPositionString(const Move& pondermove) const
{
    assert(board);
    
    std::string str = "position " + (board->fromOriginPosition() ? "startpos" : ("fen " + board->getStartingFen()));
    
    if (!board->histList.empty()) {
        str += " moves";
        for(auto && hist : board->histList) {
            str += " " + hist.move.toCoordinateString();
        }
    }
    
    if (pondermove.isValid()) {
        if (board->histList.empty()) {
            str += " moves";
        }
        str += " " + pondermove.toCoordinateString();
    }
    return str;
}

std::string UciEngine::getGoString(Move pondermove)
{
    std::string str = getPositionString(pondermove) + "\ngo ";
    if (pondermove.isValid()) {
        str += "ponder ";
    }
    
    str += timeControlString();
    return str;
}

std::string UciEngine::timeControlString() const
{
    switch (timeController->mode) {
        case TimeControlMode::infinite:
            return "infinite";
            
        case TimeControlMode::depth:
            return "depth " + std::to_string(timeController->depth);
            
        case TimeControlMode::movetime:
            return "movetime " + std::to_string(timeController->time);
            
        case TimeControlMode::standard:
        {
            // timeController unit: second, here need ms
            int wtime = int(timeController->timeLeft[W] * 1000);
            int btime = int(timeController->timeLeft[B] * 1000);
            
            int inc = int(timeController->increment * 1000);
            
            std::string str = "wtime " + std::to_string(wtime) + " btime " + std::to_string(btime)
            + " winc " + std::to_string(inc) + " binc " + std::to_string(inc);
            
            auto n = timeController->moves;
            auto halfCnt = int(board->histList.size());
            int fullCnt = halfCnt / 2;
            int curMoveCnt = fullCnt % n;
            
            int movestogo = n - curMoveCnt;
            if (movestogo > 0) {
                str += " movestogo " + std::to_string(movestogo);
            }
            return str;
        }
        default:
            break;
    }
    return "";
}

bool UciEngine::sendPing()
{
    return write("isready");
}

bool UciEngine::sendPong()
{
    return write("readyok");
}

void UciEngine::tickWork()
{
    Engine::tickWork();
}

void UciEngine::parseLine(const std::string& str)
{
    log(str, LogType::fromEngine);
    
    static const char* optionName = "option name";
    auto len = strlen(optionName);
    if (memcmp(str.c_str(), optionName, len) == 0) {
        if (!parseOption(str)) {
            write("Unknown " + str);
        }
        return;
    }
    
    static const char* info = "info ";
    len = strlen(info);
    if (memcmp(str.c_str(), info, len) == 0) {
        // info is useless at the moment - completely ignore
        return;
    }
    
    auto timeCtrl = timeController;
    auto moveRecv = moveReceiver;
    if (timeCtrl == nullptr || moveRecv == nullptr) {
        return;
    }
    
    static const char* bestmove = "bestmove ";
    len = strlen(bestmove);
    if (memcmp(str.c_str(), bestmove, len) == 0) {
        
        assert(expectingBestmove);
        assert(computingState != EngineComputingState::idle);
        
        expectingBestmove = false;
        auto oldComputingState = computingState;
        computingState = EngineComputingState::idle;
        
        auto period = timeCtrl->moveTimeConsumed(); // moveTimeConsumed();
        
        auto ss = str.substr(len);
        auto vec = splitString(ss, ' '); assert(vec.size() > 0);
        auto moveString = vec.at(0);
        
        std::string ponderMoveString = "";
        if (vec.size() > 2 && vec.at(1) == "ponder") {
            ponderMoveString = vec.at(2);
        }
        
        if (!moveString.empty() && moveReceiver != nullptr) {
            (moveRecv)(moveString, ponderMoveString, period, oldComputingState);
        }
        return;
    }
    
    if (str == "uciok") {
        setState(PlayerState::ready);
        expectingBestmove = false;
        sendOptions();
        sendPing();
        return;
    }
    
    if (str == "isready") {
        sendPong();
        return;
    }
}

bool UciEngine::parseOption(const std::string& s)
{
    assert(!s.empty());
    
    std::regex re("option name (.*) type (combo|spin|button|check|string)(.*)");
    
    try {
        std::smatch match;
        if (std::regex_search(s, match, re) && match.size() > 1) {
            Option option;
            option.name = match.str(1);
            auto type = match.str(2);
            
            if (type == "button") {
                option.type = OptionType::button;
                config.updateOption(option);
                return true;
            }
            if (match.size() <= 3) {
                return false;
            }
            std::string rest = match.str(3);
            if (rest.empty()) {
                return false;
            }
            
            auto p = rest.find("default");
            if (p == std::string::npos) {
                return false;
            }
            
            if (type == "string" || type == "check") {
                auto str = p + 8 >= rest.size() ? "" : rest.substr(p + 8);
                if (type == "check") {
                    option.type = OptionType::check;
                    bool b = memcmp(str.c_str(), "true", 4) == 0;
                    option.setDefaultValue(b);
                } else {
                    option.type = OptionType::string;
                    if (str == "<empty>") {
                        str = "";
                    }
                    option.setDefaultValue(str);
                }
                if (option.isValid()) {
                    config.updateOption(option);
                    return true;
                }
                return false;
            }
            
            if (type == "spin") {
                std::regex re("(.*)default (.+) min (.+) max (.+)");
                if (std::regex_search(rest, match, re) && match.size() > 4) {
                    auto dString = match.str(2);
                    auto minString = match.str(3);
                    auto maxString = match.str(4);
                    auto dInt = std::atoi(dString.c_str()), minInt = std::atoi(minString.c_str()), maxInt = std::atoi(maxString.c_str());
                    
                    option.type = OptionType::spin;
                    option.setDefaultValue(dInt, minInt, maxInt);
                    if (option.isValid()) {
                        config.updateOption(option);
                        return true;
                    }
                }
                return false;
            }
            
            if (type == "combo") {
                std::regex re("(.*)default (.+) (var (.+))+");
                if (std::regex_search(rest, match, re) && match.size() > 3) {
                    auto dString = match.str(2);
                    std::vector<std::string> list;
                    for(int i = 3; i < match.size(); i++) {
                        auto str = match.str(i);
                        if (memcmp(str.c_str(), "var ", 4) == 0) {
                            str = str.substr(4);
                            if (!str.empty()) list.push_back(str);
                            //                            std::cout << str << std::endl;
                        }
                    }
                    
                    option.type = OptionType::combo;
                    option.setDefaultValue(dString);
                    option.setDefaultValue(list);
                    
                    if (option.isValid()) {
                        config.updateOption(option);
                        return true;
                    }
                    return false;
                }
            }
        } else {
        }
    } catch (std::regex_error& e) {
        // Syntax error in the regular expression
        std::cout << "Error: " << e.what() << std::endl;
    }
    
    return false;
}

