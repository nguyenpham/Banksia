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


#ifndef wbengine_h
#define wbengine_h

#include <stdio.h>

#include "engine.h"

namespace banksia {
    
    class WbEngine : public Engine
    {
    private:
        enum class WbEngineCmd {
            feature, move, resign, offer,
            illegal, error, ping, pong,
            tellopponent, tellothers, tellall, telluser, tellusererror, tellicsnoalias,
        };
        
    public:
        WbEngine() : Engine() {}
        WbEngine(const Config& config) : Engine(config) {}
        
        virtual const char* className() const override { return "WbEngine"; }
        
        virtual std::string protocolString() const override;
        
        virtual void newGame() override;
        
        virtual void prepareToDeattach() override;
        
        virtual bool sendQuit() override;
        
        virtual bool sendPing() override;
        virtual bool sendPong(const std::string&);
        
        virtual bool goPonder(const Move& pondermove) override;
        virtual bool go() override;
        
        virtual bool stop() override;
        virtual void tickWork() override;
        
        virtual bool oppositeMadeMove(const Move& move, const std::string& sanMoveString) override;
        
    protected:
        virtual bool sendOptions();
        virtual const std::unordered_map<std::string, int>& getEngineCmdMap() const override;
        virtual void parseLine(int, const std::string&, const std::string&) override;
        
        void parseFeatures(const std::string& line);
        bool parseFeature(const std::string& featureName, const std::string& content, bool quote);
        
        bool engineMove(const std::string& moveString, bool mustSend);
        bool isFeatureOn(const std::string& featureName, bool defaultValue = false);
        bool sendMemoryAndScoreOptions();
        
        virtual bool isIdleCrash() const override;
        
    private:
        std::string timeControlString() const;
        
        std::map<std::string, std::string> featureMap;
        
        int pingCnt = 0;
        static const std::unordered_map<std::string, int> wbEngineCmd;
        int tick_delay_2_ready = -1;
        
        bool feature_san = false, feature_usermove = false, feature_setboard = false;
        
        bool feature_done_finished = true;
        
        std::set<std::string> variantSet;
    };
    
} // namespace banksia

#endif /* WbEngineplayer_hpp */


