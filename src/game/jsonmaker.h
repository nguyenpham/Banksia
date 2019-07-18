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


#ifndef jsonmaker_h
#define jsonmaker_h

#include <stdio.h>

#include "tourmng.h"
#include "jsonengine.h"

namespace banksia {
    
    class JsonMaker : public Obj, public Tickable
    {
    private:
        enum class JsonMakerState {
            begin, working, done
        };
        
        const std::string defaultJsonTourName = "tour.json";
        const std::string defaultJsonEngineName = "engines.json";

    public:
        JsonMaker();
        virtual ~JsonMaker() {}
        
        virtual bool isValid() const override;
        virtual std::string toString() const override;
        virtual const char* className() const override { return "JsonMaker"; }

        void build(const std::string& mainJsonPath, const std::string& mainEnginesPath, int concurrency);
        void shutdown();

        static std::vector<std::string> listExcecutablePaths(const std::string& dirname);
        static bool isRunable(const std::string& path);
        
    private:
        virtual void tickWork() override;
        void completed();
        
    private:
        JsonMakerState state = JsonMakerState::begin;
        
        const int tick_idle_max = 2 * 5 * 60; // 5 min
        int concurrency = 4, tick_idle = 0;
        std::vector<Config> configVec;
        
        std::vector<JsonEngine*> workingEngineVec;
        std::vector<Config> goodConfigVec;

        CppTime::Timer timer;
        CppTime::timer_id mainTimerId;
        time_t startTime;

        std::string jsonTourMngPath, jsonEngineConfigPath, motherEngineFolder;
    };
    
} // namespace banksia


#endif /* player_hpp */

