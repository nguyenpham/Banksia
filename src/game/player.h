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


#ifndef player_hpp
#define player_hpp

#include <stdio.h>

#include "../chess/chess.h"
#include "time.h"

namespace banksia {
    enum class PlayerType {
        human, engine, none
    };
    
    enum class PlayerState {
        none, starting, ready, playing, stopping, stopped
    };
    
    enum class EngineComputingState {
        idle, thinking, pondering
    };
    
    class Player : public Obj, public Tickable
    {
    public:
        Player();
        Player(const std::string& name, PlayerType type);
        virtual ~Player() {}
        
        virtual bool isValid() const override;
        virtual std::string toString() const override;
        virtual const char* className() const override { return "Player"; }
        
        std::string getName() const;
        PlayerState getState() const { return state; }
        void setState(PlayerState st);
        int getTickState() const { return tick_state; }
        void setPonderMode(bool mode) { ponderMode = mode; }

    public:
        virtual bool kickStart() = 0;
        virtual bool stopThinking() = 0;
        virtual bool quit() = 0;
        virtual bool kill() = 0;
        
        virtual void newGame() {}

        virtual void attach(ChessBoard*, const GameTimeController*, std::function<void(const Move&, const std::string&, const Move&, double, EngineComputingState)>, std::function<void()>);
        virtual void deattach();
        virtual bool isAttached() const;
        virtual bool isSafeToDeattach() const = 0;
        virtual void prepareToDeattach() = 0;

        virtual bool goPonder(const Move& pondermove);
        virtual bool go();
        virtual bool oppositeMadeMove(const Move& move, const std::string& sanMoveString);

        int getScore() const {
            return score;
        }
        
        int getDepth() const {
            return depth;
        }
        
        i64 getNodes() const {
            return nodes;
        }

    protected:
        int idNumber; // a random number, main purpose for debugging
        std::string name;
        
        PlayerType type;
        PlayerState state;
        int tick_state = 0;
        // for stats
        int score, depth;
        i64 nodes;
        
        bool ponderMode = false;
        
        std::function<void(const Move&, const std::string&, const Move&, double, EngineComputingState)> moveReceiver = nullptr;
        std::function<void()> resignFunc = nullptr;
        
        ChessBoard* board = nullptr;
        const GameTimeController* timeController = nullptr;
    };

} // namespace banksia


#endif /* player_hpp */

