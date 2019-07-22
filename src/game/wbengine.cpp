/*
 This file is part of Banksia.
 
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

bool WbEngine::candoSyncTaskNow(SyncTask task)
{
    if (feature_ping && expectingPongCnt) {
        std::lock_guard<std::mutex> dolock(syncMutex);
        if (expectingPongCnt) {
            syncTasks.push_back(task);
            return false;
        }
    }
    return true;
}

bool WbEngine::doSyncTask()
{
    assert(feature_ping);
    if (expectingPongCnt || syncTasks.empty()) {
        return false;
    }
    
    std::lock_guard<std::mutex> dolock(syncMutex);
    
    if (expectingPongCnt || syncTasks.empty()) {
        return false;
    }


    auto task = syncTasks.front();
    syncTasks.erase(syncTasks.begin());
    switch (task) {
        case SyncTask::newgame:
            newGame_straight();
            break;
        case SyncTask::go:
            go_straight();
            break;
        default:
            break;
    }
    return true;
}

void WbEngine::newGame()
{
    if (!candoSyncTaskNow(SyncTask::newgame)) {
        return;
    }
    newGame_straight();
}

void WbEngine::newGame_straight()
{
    assert(getState() == PlayerState::ready);
    computingState = EngineComputingState::idle;
    
    sendMemoryAndCoreOptions();
    
    write(ponderMode ? "hard" : "easy");
    write("post");

    if (isFeatureOn("reuse", true)) {
        write("new");
    }
    
    if (!board->fromOriginPosition()) {
        write("setboard " + board->getStartingFen());
    }
    
    if (!board->histList.empty()) {
        // TODO: check logic again. No ping here
        // force to avoid some engines such as Crafty auto computing
        write("force");
        for (auto && hist : board->histList) {
            std::string str = move2String(hist.move, hist.moveString);
            write(str);
        }
    }
    
    write(timeControlString());
    
    if (feature_ping) {
        // fake ping to avoid other cmd be run
        expectingPongCnt++;
        
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        sendPing();
    } else {
        setState(PlayerState::playing);
    }

}

void WbEngine::prepareToDeattach()
{
    if (tick_deattach >= 0) return;
    stop();
    tick_deattach = tick_period_deattach;
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
    if (!candoSyncTaskNow(SyncTask::go)) {
        return false;
    }
    
    return go_straight();
}

bool WbEngine::go_straight()
{
    Engine::go();
    computingState = EngineComputingState::thinking;
    
    return write(timeLeftString()) && write("go");
}

std::string WbEngine::timeLeftString() const
{
    switch (timeController->mode) {
        case TimeControlMode::movetime:
        {
            auto centiSeconds = static_cast<int>(timeController->time * 100);
            auto s = std::to_string(centiSeconds);
            return "time " + s + "\notim " + s;
        }
            
        case TimeControlMode::standard:
        {
            // timeController unit: second -> centi second
            auto sd = static_cast<int>(board->side);
            auto time = int(timeController->getTimeLeft(sd) * 100);
            auto otime = int(timeController->getTimeLeft(1 - sd) * 100);
            return "time " + std::to_string(time) + "\notim " + std::to_string(otime);
        }
        default:
            break;
    }
    return "";
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
    assert(feature_ping);
    expectingPongCnt++;
    return write("ping " + std::to_string(++pingCnt));
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
            write("force");
            if (feature_ping) {
                sendPing();
            }
            setState(PlayerState::ready);
        }
    }
}

void WbEngine::tickPing()
{
    if (computingState == EngineComputingState::thinking || !feature_ping) {
        return;
    }
    
    tick_ping++;
    if (tick_ping >= tick_period_ping) {
        resetPing();
        sendPing();
    } else {
        doSyncTask();
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

bool WbEngine::sendMemoryAndCoreOptions()
{
    // cores N, memory N
    std::string str;
    
    if (isFeatureOn("memory")) {
        Option option = ConfigMng::instance->getOverrideOption("memory");
        if (!option.isValid()) {
            for(auto && o : config.optionList) {
                if (o.name == "memory") {
                    option = o;
                    break;
                }
            }
        }
        if (option.isValid()) {
            str += option.name + " " + option.getValueAsString();
        }
    }
    if (isFeatureOn("smp")) {
        Option option = ConfigMng::instance->getOverrideOption("cores");
        if (!option.isValid()) {
            for(auto && o : config.optionList) {
                if (o.name == "cores") {
                    option = o;
                    break;
                }
            }
        }
        if (option.isValid()) {
            if (!str.empty()) str += "\n";
            str += option.name + " " + option.getValueAsString();
        }
    }

    return !str.empty() && write(str);
}

bool WbEngine::parseFeature(const std::string& name, const std::string& content, bool quote)
{
    if (name.empty() || content.empty()) {
        return false;
    }
    
    if (name == "option") {
        auto vec = splitString(content, ' ');
        if (vec.size() < 2) return true;
        auto optionName = vec.front();
        
        for(auto && o : config.optionList) {
            if (o.name != optionName) {
                continue;
            }
            
            if (!isWritable() || o.isDefaultValue() || o.name == "memory" || o.name == "cores") {
                break;
            }
            
            std::string str = "option " + o.name + "=" + o.getValueAsString();
            write(str);
            break;
        }
        
        return true;
    }
    
    if (name == "san") {
        feature_san = content == "1";
    } else if (name == "usermove") {
        feature_usermove = content == "1";
    } else if (name == "ping") {
        feature_ping = content == "1";
    } else if (name == "variants") {
        config.variantSet.clear();
        auto varList = splitString(content, ',');
        for(auto && s : varList) {
            trim(s);
            if (!s.empty()) {
                config.variantSet.insert(s);
            }
        }
    } else if (name == "done") {
        if (content == "0") {
            tick_delay_2_ready = 60 * 60 * 2; // 1h
            feature_done_finished = false;
        } else {
            setState(PlayerState::ready);
            feature_done_finished = true;
        }
        return true;
    } else if (name == "smp" || name == "memory") { // changed into option
        if (content == "1") {
            int dInt = name == "smp" ? 1 : 16, minInt = 1, maxInt = 256;
            Option option;
            option.name = name == "smp" ? "cores" : "memory";
            option.type = OptionType::spin;
            option.setDefaultValue(dInt, minInt, maxInt);
            config.updateOption(option);
        }
    } else if (name == "myname") {
        config.idName = content;
    }
    
    write("accepted " + name);
    
    featureMap[name] = content;
    
    return true;
}

void WbEngine::parseFeatures(const std::string& line)
{
    // "feature " length = 8
    std::string featureName;
    for(int i = 8, k = -1, quote = 0; i < line.size(); i++) {
        auto ch = line[i];
        if (ch == '=') {
            if (k < 0 || i <= k) break; // somethings wrong
            featureName = line.substr(k, i - k);
            k = i + 1;
            continue;
        }
        if (ch == '"') {
            if (quote == 0) {
                quote++;
                k = i + 1;
            } else {
                if (!featureName.empty() && k > 0 && i > k + 1) {
                    auto content = line.substr(k, i - k);
                    parseFeature(featureName, content, true);
                }
                featureName = "";
                quote = 0;
                k = -1;
                continue;
            }
        }
        if ((ch == ' ' || i + 1 == line.size()) && quote == 0) {
            if (!featureName.empty() && k > 0) {
                auto len = i - k;
                if (i + 1 == line.size()) len++;
                auto content = line.substr(k, len);
                parseFeature(featureName, content, false);
            }
            featureName = "";
            quote = 0;
            k = -1;
            continue;
        }
        if (k < 0) k = i;
    }
}

std::string WbEngine::move2String(const Move& move, const std::string& sanMoveString) const
{
    std::string str;
    if (feature_usermove) {
        str += "usermove ";
    }
    
    if (feature_san) {
        str += sanMoveString;
    } else {
        str += move.toCoordinateString();
    }
    return str;
}



bool WbEngine::oppositeMadeMove(const Move& move, const std::string& sanMoveString)
{
    write("force"); // we don't want this engine starts calculating after this move
    
    std::string str = move2String(move, sanMoveString);
    return write(str);
}

bool WbEngine::engineMove(const std::string& moveString, bool mustSend)
{
    if (moveString.length() < 2 || !isalpha(moveString.at(0))
        || computingState != EngineComputingState::thinking) { // some engines don't orbey 'force' and create moves too fast -> ignore some cases
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
        
        if (computingState != EngineComputingState::thinking) {
            return;
        }
        auto ch = line.front();
        if (isdigit(ch)) { // it may be thinking output -> ignore
            // 9 156 1084 48000 Nf3 Nc6 Nc3 Nf6
            // ply score time nodes pv
            auto vec = splitString(line, ' ');
            if (vec.size() >= 4) {
                depth = std::atoi(vec[0].c_str());
                score = std::atoi(vec[1].c_str());
                nodes = std::atoi(vec[3].c_str());
                
                if (depth > 0 && nodes > 0) {
                    engineSentCorrectCmds();
                }
            }
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
            tick_delay_2_ready = std::max(3, tick_delay_2_ready); // extent init time a bit
            parseFeatures(line);
            break;
        }

        case WbEngineCmd::ping:
        {
            auto vec = splitString(line, ' ');
            sendPong(vec.size() >= 2 ? vec.at(1) : "");
            break;
        }

        case WbEngineCmd::pong:
        {
            expectingPongCnt = 0;
            pongCnt++;

            if (getState() == PlayerState::ready) {
                setState(PlayerState::playing);
            }
            doSyncTask();
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




