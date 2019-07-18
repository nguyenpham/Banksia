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


#ifndef jsonengineplayer_h
#define jsonengineplayer_h

#include <stdio.h>

#include "engine.h"
#include "uciengine.h"
#include "wbengine.h"

namespace banksia {
    
    class JsonEngine : public Engine
    {
    private:
        enum class JsonEngineState {
            none, working, done
        };
        
    public:
        JsonEngine();
        JsonEngine(const Config& config);
        virtual ~JsonEngine();
        
        const char* className() const override { return "JsonEngine"; }

        void kickStart(std::function<void(Config* config)> taskComplete);
        void tickWork() override;

        bool isFinished() const {
            return jsonstate == JsonEngineState::done;
        }
    private:
        const std::unordered_map<std::string, int>& getEngineCmdMap() const override;
        void parseLine(int, const std::string&, const std::string&) override;

        bool isIdleCrash() const override;
        
        std::string protocolString() const override;
        bool sendPing() override;
        void prepareToDeattach() override {}
        bool stop() override { return true; }
        bool isAttached() const override;

//        void log(const std::string& line, LogType logType) const override;

    private:
        void completed(Config* config);
        void setupEngine();
        
        JsonEngineState jsonstate = JsonEngineState::none;
        Protocol originalProtocol;

        std::function<void(Config* config)> taskComplete = nullptr;

        const int tick_test_period = 24;    // 12s
        const int tick_test_period_wb = 50; // 25 seconds
        int tick_test = 0, tryNum = 3;      // 4 x 12 -> 48s
        
        Engine* engine = nullptr;
        
        UciEngine uciEngine;
        WbEngine wbEngine;
        
        std::set<std::string> usedCmdSet;
    };
    
} // namespace banksia

#endif /* jsonengineplayer_h */

