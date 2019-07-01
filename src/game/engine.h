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


#ifndef engineplayer_hpp
#define engineplayer_hpp

#include <vector>
#include <set>

#include "../3rdparty/process/process.hpp"
#include "../chess/chess.h"

#include "player.h"
#include "configmng.h"

namespace banksia {
    const int tick_ping = 60; // 30s
    
    enum class LogType {
        toEngine, fromEngine, system
    };
    
    class Engine : public Player
    {
    public:
        Engine() : Player("", PlayerType::engine) {}
        Engine(const Config& config) : Player(config.name, PlayerType::engine), config(config) {}
        
        virtual const char* className() const override { return "Engine"; }
        
        bool isReady() const;
        
        virtual void tickWork() override;
        
        void addMessageLogger(std::function<void(const std::string& string, LogType logType)> messageLogger);
        
    public:
        virtual bool kickStart() override;
        virtual bool stopThinking() override;

        virtual bool stop() = 0;
        virtual bool quit() override;
        virtual bool kill() override;

        virtual bool isHuman() const override {
            return false;
        }
        
    public:
        void read_stdout(const std::string&);
        void read_stderr(const std::string&);
        
    public:
        virtual void parseLine(const std::string&) = 0;
        
    protected:
        virtual void log(const std::string& line, LogType engineLog) const;
        
        virtual std::string protocolString() const = 0;
        
        virtual bool sendQuit() = 0;
        
        virtual void resetPing();
        virtual void resetIdle();
        
        virtual bool sendPing() = 0;
        virtual bool sendPong() = 0;
        
    protected:
        bool write(const std::string&);
        
    public:
        EngineComputingState computingState = EngineComputingState::idle;
        Config config;
        
    private:
        TinyProcessLib::Process* process = nullptr;
        std::function<void(const std::string& string, LogType engineLog)> messageLogger = nullptr;
        
        int tick_for_ping, tick_idle = 0, tick_stopping = 0;
    };
    
    
} // namespace banksia

#endif /* engineplayer_hpp */

