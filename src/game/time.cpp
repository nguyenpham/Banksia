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

#include "time.h"

using namespace banksia;

TimeController::TimeController()
{
}

void TimeController::setup(TimeControlMode _mode, int val, double t0, double t1, double t2)
{
    mode = _mode;
    
    switch (mode) {
        case TimeControlMode::infinite:
            break;
        case TimeControlMode::depth:
            depth = val;
            break;
        case TimeControlMode::movetime:
            time = t0;
            break;
        case TimeControlMode::standard:
            moves = val;
            time = t0;
            increment = t1;
            margin = t2;
            break;
        default:
            break;
    }
}

TimeController::~TimeController()
{
}

static const char* timeControllerNams[] = {
    "infinite", "depth", "movetime", "standard", nullptr
};

TimeControlMode TimeController::string2TimeControlMode(const std::string& name)
{
    for(int i = 0; timeControllerNams[i]; i++) {
        if (name == timeControllerNams[i]) {
            return static_cast<TimeControlMode>(i);
        }
    }
    return TimeControlMode::none;
}

std::string TimeController::toString() const
{
    std::ostringstream stringStream;
    
    auto t = static_cast<int>(mode);
    switch (mode) {
        case TimeControlMode::infinite:
            stringStream << timeControllerNams[t];
            break;
        case TimeControlMode::depth:
            stringStream << timeControllerNams[t] << ":" << depth;
            break;
        case TimeControlMode::movetime:
            stringStream << timeControllerNams[t] << ":" << time;
            break;
        case TimeControlMode::standard:
            stringStream << moves << "/" << time << ":" << increment;
            break;
            
        default:
            break;
    }
    return stringStream.str();
}

bool TimeController::isValid() const
{
    if (static_cast<int>(mode) >= 0 && mode < TimeControlMode::none) {
        switch (mode) {
            case TimeControlMode::infinite:
                return true;
            case TimeControlMode::depth:
                return depth > 0;;
            case TimeControlMode::movetime:
                return time > 0;
            case TimeControlMode::standard:
                return moves >= 0 && time > 0 && increment >= 0 && margin >= 0;
                
            default:
                break;
        }
    }
    return false;
}


bool TimeController::load(const Json::Value& obj)
{
    if (!obj.isMember("mode")
        ) {
        return false;
    }
    auto str = obj["mode"].asString();
    mode = string2TimeControlMode(str);
    
    switch (mode) {
        case TimeControlMode::infinite:
            return true;
            
        case TimeControlMode::depth:
            if (!obj.isMember("depth")) return false;
            depth = obj["depth"].asInt();
            return depth > 0;
            
        case TimeControlMode::movetime:
            if (!obj.isMember("time")) return false;
            time = obj["time"].asDouble();
            return time > 0;
            
        case TimeControlMode::standard:
            if (!obj.isMember("time")
                ||!obj.isMember("moves")
                ||!obj.isMember("increment")
                )
                return false;
            moves = obj["moves"].asInt();
            time = obj["time"].asDouble();
            increment = obj["increment"].asDouble();
            margin = obj.isMember("margin") ? obj["margin"].asDouble() : 0;
            return time > 0 && increment >= 0 && margin >= 0 && moves >= 0;
            
        default:
            return false;
    }
    
    return true;
}

Json::Value TimeController::saveToJson() const
{
    Json::Value obj;
    obj["mode"] = timeControllerNams[static_cast<int>(mode)];
    
    switch (mode) {
        case TimeControlMode::infinite:
            return true;
        case TimeControlMode::depth:
            obj["depth"] = depth;
            break;
        case TimeControlMode::movetime:
            obj["time"] = time;
            break;
        case TimeControlMode::standard:
            obj["moves"] = moves;
            obj["time"] = time;
            obj["increment"] = increment;
            obj["margin"] = margin;
            break;
            
        default:
            break;
    }
    
    return obj;
}

void TimeController::cloneFrom(const TimeController& other)
{
    mode = other.mode;
    depth = other.depth;
    moves = other.moves;
    time = other.time;
    increment = other.increment;
    margin = other.margin;
}

////////////////////////
GameTimeController::GameTimeController()
{
}

GameTimeController::~GameTimeController()
{
}

void GameTimeController::startMoveTimeClock()
{
    moveStartClock = std::chrono::system_clock::now();
}

// unit: second
double GameTimeController::moveTimeConsumed() const
{
    auto diff = std::chrono::system_clock::now() - moveStartClock;
    auto ms = std::chrono::duration <double, std::milli> (diff).count();
	assert(ms >= 0);
    return double(ms) / 1000; // convert into second
}

double GameTimeController::getTimeLeft(int sd) const
{
    assert(sd == 0 || sd == 1);
    return timeLeft[sd];
}

bool GameTimeController::isTimeOver(Side side)
{
    if (mode != TimeControlMode::movetime && mode != TimeControlMode::standard) {
        return false;
    }
    
    auto sd = static_cast<int>(side);
    
    lastQueryConsumed = moveTimeConsumed();
    if (lastQueryConsumed >= timeLeft[sd] + margin) {
        return true;
    }
    return false;
}


void GameTimeController::setupClocksBeforeThinking(int halfMoveCnt)
{
    if (mode == TimeControlMode::movetime || halfMoveCnt == 0) {
        timeLeft[0] = timeLeft[1] = time;
    }
    
    startMoveTimeClock();
}

void GameTimeController::udateClockAfterMove(double moveElapse, Side side, int halfMoveCnt)
{
    assert(moveElapse > 0 && halfMoveCnt > 0);
    
    if (mode != TimeControlMode::standard) {
        return;
    }
    
    auto sd = static_cast<int>(side);
    timeLeft[sd] += increment - moveElapse;
    
    if (moves == 0) {
        return;
    }
    
    int fullCnt = (halfMoveCnt + 1) / 2;
    if (fullCnt % moves == 0) {
        timeLeft[sd] += time;
    }
}

bool GameTimeController::isValid() const
{
    return TimeController::isValid() && timeLeft[0] >= 0 && timeLeft[1] >= 0;
}


