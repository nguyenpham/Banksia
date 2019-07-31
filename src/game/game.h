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


#ifndef game_hpp
#define game_hpp

#include "../chess/chess.h"
#include "engine.h"


namespace banksia {
    
    enum class GameState {
        none, begin,
        ready, playing, stopped, // playing
        ending, ended // last work to remove
    };
    
    class GameConfig
    {
    public:
        bool ponderMode = false;
        
        // adjudication
        bool adjudicationMode = true;
        bool adjudicationEgtbMode = true;
        int adjudicationMaxGameLength = 0;
        int adjudicationMaxPieces = 10;
    };
    
    class Game : public Obj, public Tickable
    {
    public:
        
        Game();
        Game(Player*, Player*, const TimeController&, const GameConfig&);
        virtual ~Game();
        
        void set(Player*, Player*, const TimeController&);
        void attachPlayer(Player* player, Side side);
        Player* deattachPlayer(Side side);
        void setMessageLogger(std::function<void(const std::string&, const std::string&, LogType)> logger);
        
        void newGame();
        
        void kickStart();
        void pause();
        void stop();
        bool make(const Move& move, const std::string& moveString);
        void startThinking(Move pondermove = Move::illegalMove);
        
        virtual const char* className() const override { return "Game"; }
        virtual bool isValid() const override;
        virtual std::string toString() const override;
        
        void gameOver(const Result& result);
        void gameOver(Side winner, ReasonType reasonType);
        
        void moveFromPlayer(const Move& move, const std::string& moveString, const Move& ponderMove, double timeConsumed, Side side, EngineComputingState oldState);
        
        virtual void tickWork() override;
        
        Player* getPlayer(Side side);
        const Player* getPlayer(Side side) const;

        GameState getState() const { return state; }
        void setState(GameState st);
        
        void setStartup(int idx, const std::string& startFen, const std::vector<Move>& startMoves);
        int getIdx() const;
        int getStateTick() const { return stateTick; }
        
        std::string toPgn(std::string event = "", std::string site = "", int round = -1, int gameIdx = -1, bool richMode = false);
        
        std::string getGameTitleString(bool includeResult = false) const;
        
    public:
        ChessBoard board;
        
    private:
        bool checkTimeOver();
        
    private:
        int idx, stateTick = 0;
        GameState state;
        GameConfig gameConfig;
        
        Player* players[2];
        GameTimeController timeController;
        
        std::function<void(const std::string&, const std::string&, LogType)> messageLogger = nullptr;
        
        std::string startFen;
        std::vector<Move> startMoves;
        std::mutex criticalMutex;
    };
    
} // namespace banksia

#endif /* game_hpp */


