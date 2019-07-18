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


#ifndef engineplayer_hpp
#define engineplayer_hpp

#include <vector>
#include <set>

#include "../3rdparty/process/process.hpp"
#include "../chess/chess.h"

#include "player.h"
#include "configmng.h"

namespace banksia {
    enum class LogType {
        toEngine, fromEngine, system
    };
    
    class Engine : public Player
    {
    protected:
        const int tick_period_ping = 30; // 20s
        const int tick_period_deattach = 6; // 3s
        const int tick_period_idle_dead = 60; // 30s

    public:
        Engine() : Player("", PlayerType::engine) {}
        Engine(const Config& config) : Player(config.name, PlayerType::engine), config(config) {}
        virtual ~Engine();
        
        virtual const char* className() const override { return "Engine"; }
        
        bool isWritable() const;
        
        virtual void tickWork() override;
        
        void setMessageLogger(std::function<void(const std::string&, const std::string&, LogType logType)> messageLogger);
        
    public:
        virtual bool kickStart() override;
        virtual bool stopThinking() override;

        virtual bool stop() = 0;
        virtual bool quit() override;
        virtual bool kill() override;

    public:
        virtual void attach(ChessBoard*, const GameTimeController*, std::function<void(const Move&, const std::string&, const Move&, double, EngineComputingState)>, std::function<void()>) override;

        virtual bool isSafeToDeattach() const override;
        virtual bool isSafeToDelete() const;

        virtual std::string protocolString() const = 0;
        virtual void parseLine(int, const std::string&, const std::string&) {}
        virtual const std::unordered_map<std::string, int>& getEngineCmdMap() const = 0;

    protected:
        virtual void parseLine(const std::string&);

    protected:
        virtual void log(const std::string& line, LogType engineLog) const;

        virtual bool sendQuit();
        
        virtual void resetPing();
        virtual void resetIdle();
        virtual void engineSentCorrectCmds();

        virtual bool sendPing() = 0;
        
        void read_stdout(const char *bytes, size_t n);
        void read_stderr(const char *bytes, size_t n);
        
        bool exited() const;
        
        virtual bool isIdleCrash() const;

        virtual void finished() {}
        virtual void tickPing();
        

    public:
        EngineComputingState computingState = EngineComputingState::idle;
        Config config;
        
    protected:
        bool write(const std::string&);
        int tick_deattach = -1;
        int tick_ping, tick_idle, tick_being_kill = -1; //, tick_stopping = 0;
        std::function<void(const std::string&, const std::string&, LogType)> messageLogger = nullptr;

        void deleteThread();
        int correctCmdCnt = 0;
        
    private:
        const int process_buffer_size = 16 * 1024;
        std::string lastIncompletedStdout;
        TinyProcessLib::Process::id_type processId = 0;
        TinyProcessLib::Process* process = nullptr;
        std::thread* pThread = nullptr;
    };
    
    
} // namespace banksia

#endif /* engineplayer_hpp */

