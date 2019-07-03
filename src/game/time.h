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


#ifndef time_hpp
#define time_hpp

#include <stdio.h>
#include <functional>
#include <map>
#include <mutex>

#include "../base/comm.h"

namespace banksia {
    
    enum class TimeControlMode {
        infinite, depth, movetime, standard, none
    };
    
    class TimeController : public Jsonable
    {
    public:
        TimeController();
        ~TimeController();
        
        void setup(TimeControlMode mode, int val = 0, double t0 = 0, double t1 = 0, double t2 = 0);
        
        virtual const char* className() const override { return "TimeController"; }
        
        virtual std::string toString() const override;
        virtual bool isValid() const override;
        
        virtual bool load(const Json::Value& obj) override;
        virtual Json::Value saveToJson() const override;
        
        static TimeControlMode string2TimeControlMode(const std::string& name);
        
        void cloneFrom(const TimeController&);
        
    public:
        TimeControlMode mode;
        int depth, moves;
        double time, increment, margin;

    private:
    };
    
    class GameTimeController : public TimeController
    {
    public:
        GameTimeController();
        ~GameTimeController();
        
        virtual const char* className() const override { return "GameTimeController"; }
        
        void setupClocksBeforeThinking(int halfMoveCnt);
        void udateClockAfterMove(double moveElapse, Side side, int halfMoveCnt);
        
        bool isTimeOver(Side side);
        virtual bool isValid() const override;
        double moveTimeConsumed() const;

        double getTimeLeft(int sd) const;

        double lastQueryConsumed = 0;
        
    private:
        double timeLeft[2];
        
        void startMoveTimeClock();
        
        std::chrono::system_clock::time_point moveStartClock;
    };
    
} // namespace banksia

#endif /* time_hpp */

