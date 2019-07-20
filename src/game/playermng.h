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


#ifndef playermng_hpp
#define playermng_hpp

#include "uciengine.h"
#include "configmng.h"

namespace banksia {
    
    class PlayerMng : public Obj, public Tickable
    {
    public:
        static PlayerMng* instance;
        
        PlayerMng();
        virtual ~PlayerMng();
        
        virtual const char* className() const override { return "PlayerMng"; }
        virtual bool isValid() const override;
        virtual std::string toString() const override;
        
        virtual void tickWork() override;
        
        Engine* createEngine(const std::string& name);
        Engine* createEngine(const Config& config);
        
        bool add(const Config&);
        bool add(Player* player);
        bool returnPlayer(Player* player);
        
        void shutdown();
        
    private:
        bool removePlayer(Player* player);
        
    private:
        std::mutex thelock;
        std::vector<Player*> playerList;
    };
    
} // namespace banksia

#endif /* playermng_hpp */

