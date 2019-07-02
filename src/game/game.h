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


#ifndef game_hpp
#define game_hpp

#include "../chess/chess.h"
#include "player.h"


namespace banksia {
    
    enum class GameState {
        begin, ready, playing, stopped, destroyed
    };
    
    class Game : public Obj, public Tickable
    {
    public:
        
        Game();
        Game(Player*, Player*, const TimeController&, bool);
        ~Game();
        
        void set(Player*, Player*, const TimeController&, bool);
        void attach(Player* player, Side side);
        Player* deattachPlayer(Side side);
        
        void newGame();
        
        void start();
        void startPlaying();
        void pause();
        void stop();
        bool make(const Move& move);
        void startThinking(Move pondermove = Move::illegalMove);
        
        virtual const char* className() const override { return "Game"; }
        virtual bool isValid() const override;
        virtual std::string toString() const override;
        
        void gameOver(const Result& result);
        void moveFromPlayer(const std::string& moveString, const std::string& ponderMoveString, double timeConsumed, Side side, EngineComputingState oldState);
        
        virtual void tickWork() override;
        
        Player* getPlayer(Side side);
        
        GameState getState() const { return state; }
        void setState(GameState st) { state = st; }
        
        void setStartup(int idx, const std::string& startFen, const std::vector<MoveCore>& startMoves);
        int getIdx() const;
        
        std::string toPgn(std::string event = "", std::string site = "", int round = -1) const;
        
    public:
        ChessBoard board;
        
    private:
        bool checkTimeOver();
        
    private:
        int idx;
        GameState state;
        
        Player* players[2];
        GameTimeController timeController;
        bool ponderMode = false;

        std::string startFen;
        std::vector<MoveCore> startMoves;
        std::mutex criticalMutex;
    };
    
} // namespace banksia

#endif /* game_hpp */

