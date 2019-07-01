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


#ifndef tourmng_hpp
#define tourmng_hpp

#include "game.h"
#include "configmng.h"
#include "uciengine.h"
#include "playermng.h"
#include "book.h"

#include "../3rdparty/cpptime/cpptime.h"

namespace banksia {
    // swiss
    enum class TourType {
        roundrobin, knockout, none
    };
    
    enum class MatchState {
        none, playing, completed, error
    };
    
    class MatchRecord : public Obj
    {
    public:
        MatchRecord() {}
        MatchRecord(const std::string& name0, const std::string& name1) {
            playernames[W] = name0; playernames[B] = name1;
        }
        virtual const char* className() const override { return "MatchRecord"; }
        virtual bool isValid() const override;
        virtual std::string toString() const override;
        
        void swapPlayers() {
            std::swap(playernames[0], playernames[1]);
        }
        
    public:
        std::string playernames[2];
        
        std::string startFen;
        std::vector<Move> startMoves;
        
        ResultType resultType = ResultType::noresult;
        MatchState state = MatchState::none;
        int gameIdx = 0, round = 0, score = 0;
        int gameCount = 0;
    };
    
    enum class TourState {
        none, playing, done
    };
    
    class TourResult : public Obj
    {
    public:
        std::string name;
        int gameCnt = 0, winCnt = 0, drawCnt = 0, lossCnt = 0;
        virtual const char* className() const override { return "TourResult"; }
        
        virtual bool isValid() const override;
        virtual std::string toString() const override;
        
        bool smaller(const TourResult& other) const;
    };
    
    class TourMng : public Obj, public Tickable, public JsonSavable
    {
    public:
        
        TourMng();
        TourMng(const std::string& jsonPath);
        ~TourMng();
        
        static TourMng* instance;
        
        virtual const char* className() const override { return "TourMng"; }
        
        bool createMatchList();
        bool createMatchList(const std::vector<std::string>& nameList, TourType type);
        void createMatch(MatchRecord&);
        bool createMatch(int gameIdx, const std::string& whiteName, const std::string& blackName, const std::string& startFen, const std::vector<Move>& startMoves);
        
        void startTournament();
        
        void playMatches();
        void setupTimeController(TimeControlMode mode, int val = 0, double t0 = 0, double t1 = 0, double t2 = 0);
        
        void setEngineLogMode(bool enabled);
        void setEngineLogPath(const std::string&);
        
        std::string createTournamentStats();
        
        void showEgineInOutToScreen(bool enabled);
        
    protected:
        virtual bool parseJsonAfterLoading(Json::Value&) override;
        
        void addMatchRecord(MatchRecord& record, bool returnMatch);
        
        int calcErrorMargins(int w, int d, int l);
        
        void finishTournament();
        
        void append2TextFile(const std::string& path, const std::string& str);
        void engineLog(const std::string& name, const std::string& line, LogType logType);
        
        void createKnockoutMatchList(const std::vector<std::string>& nameList, int round);
        void createNextKnockoutMatchList();
        
        std::string resultToString(const Result& result);
        void matchCompleted(Game* game);
        bool addGame(Game* game);
        
        virtual void tickWork() override;
        
        void matchLog(const std::string& line);
        
    protected:
        std::string eventName = "Chess Tournament", siteName;
        
        CppTime::Timer timer;
        
        TourType type = TourType::none;
        TourState state = TourState::none;
        
        TimeController timeController;
        bool ponderMode = false;

        std::vector<std::string> participantList;
        std::vector<MatchRecord> matchRecordList;
        std::vector<Game*> gameList;
        PlayerMng playerMng;
        BookMng bookMng;

    protected:
        bool recoverCrashEngines = true;
        int gameConcurrency = 1;
        
    protected:
        // for logging
        std::mutex matchMutex, logMutex;
        
        std::string pgnPath;
        
        std::string logResultPath;
        bool logResultMode = false;
        
        std::string logEngineInOutPath;
        bool logEngineInOutMode = false;
        bool logScreenEngineInOutMode = false;
    };
    
} // namespace banksia

#endif /* tourmng_hpp */

