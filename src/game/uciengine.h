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


#ifndef uciengineplayer_hpp
#define uciengineplayer_hpp

#include <stdio.h>

#include "engine.h"

namespace banksia {
    
    class UciEngine : public Engine
    {
    private:
        enum class UciEngineCmd {
            uciok, readyok, option, info, bestmove, theId, copyprotection, registration
        };
        
    public:
        UciEngine() : Engine() {}
        UciEngine(const Config& config) : Engine(config) {}
        virtual ~UciEngine() {}
        
        virtual const char* className() const override { return "UciEngine"; }
        
        virtual std::string protocolString() const override;
        
        virtual void newGame() override;
        
        virtual bool sendPing() override;
        virtual bool sendPong();
        
        virtual bool goPonder(const Move& pondermove) override;
        virtual bool go() override;
        
        virtual bool stop() override;
        
        virtual void prepareToDeattach() override;
        
    protected:
        virtual const std::unordered_map<std::string, int>& getEngineCmdMap() const override;
        virtual void parseLine(int, const std::string&, const std::string&) override;
        
        std::string getPositionString(const Move& ponderMove) const;
        std::string getGoString(const Move& pondermove);
        
        virtual bool sendOptions();
        
    private:
        std::string timeControlString() const;
        bool parseOption(const std::string& str);
        bool parseInfo(const std::string& line);
        
        bool expectingBestmove = false;
        Move ponderingMove;
        static const std::unordered_map<std::string, int> uciEngineCmd;
    };
    
} // namespace banksia

#endif /* uciengineplayer_hpp */


