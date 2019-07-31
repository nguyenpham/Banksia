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


#ifndef tourmng_hpp
#define tourmng_hpp

#include "game.h"
#include "configmng.h"
#include "uciengine.h"
#include "playermng.h"
#include "book.h"

#include "../3rdparty/cpptime/cpptime.h"

namespace banksia {
    enum class TourType {
        roundrobin, knockout, swiss, none
    };
    
    class EngineStats {
    public:
        i64 nodes = 0, depths = 0, moves = 0, games = 0;
        double elapsed = 0.0;
        
        void add(const EngineStats& o) {
            nodes += o.nodes; depths += o.depths; moves += o.moves; games += o.games; elapsed += o.elapsed;
        }
    };
    
    enum class MatchState {
        none, playing, completed, error
    };
    
    class MatchRecord : public Jsonable
    {
    public:
        MatchRecord() {}
        MatchRecord(const std::string& name0, const std::string& name1, bool swap) {
            auto sd = swap ? B : W;
            playernames[sd] = name0; playernames[1 - sd] = name1;
        }
        virtual const char* className() const override { return "MatchRecord"; }
        virtual bool isValid() const override;
        virtual std::string toString() const override;
        
        virtual bool load(const Json::Value& obj) override;
        virtual Json::Value saveToJson() const override;

        void swapPlayers() {
            std::swap(playernames[0], playernames[1]);
        }
        
    public:
        MatchState state = MatchState::none;
        
        std::string playernames[2];
        
        std::string startFen;
        std::vector<Move> startMoves;
        
        Result result;
        int gameIdx = 0, round = 0, pairId;
    };
    
    enum class TourState {
        none, playing, done
    };

    class TourPlayer : public Obj
    {
    public:
        std::string name;
        int gameCnt = 0, winCnt = 0, drawCnt = 0, lossCnt = 0, abnormalCnt = 0, elo = 0, flag = 0;
        int byeCnt = 0, whiteCnt = 0; // for swiss and knockdown
        virtual const char* className() const override { return "TourPlayer"; }
        
        virtual bool isValid() const override;
        virtual std::string toString() const override;
        
        double getScore() const;
        bool smaller(const TourPlayer& other) const;
    };
    
    class TourPlayerPair {
    public:
        TourPlayer pair[2];
    };
    
    class Elo {
    public:
        // https://www.chessprogramming.org/Match_Statistics
        Elo(int wins, int draws, int losses) {
            elo_difference = los = 0.0;
            double games = wins + losses + draws;
            if (games == 0 || wins + draws == 0) {
                return;
            }
            double winning_fraction = (wins + 0.5 * draws) / games;
            if (winning_fraction == 1) {
                return;
            }
            elo_difference = -std::log(1.0 / winning_fraction - 1.0) * 400.0 / std::log(10.0);
            los = .5 + .5 * std::erf((wins - losses) / std::sqrt(2.0 * (wins + losses)));
        }
        double elo_difference, los;
    };
    
    class TourMng : public Obj, public Tickable, public JsonSavable
    {
    public:
        
        TourMng();
        ~TourMng();
        
        virtual const char* className() const override { return "TourMng"; }
        
        bool createMatchList();
        bool createMatchList(std::vector<std::string> nameList, TourType type);
        void createMatch(MatchRecord&);
        bool createMatch(int gameIdx, const std::string& whiteName, const std::string& blackName, const std::string& startFen, const std::vector<Move>& startMoves);
        
        bool start(const std::string& mainJsonPath, bool yesReply, bool noReply);
        
        void playMatches();
        void setupTimeController(TimeControlMode mode, int val = 0, double t0 = 0, double t1 = 0, double t2 = 0);
        
        void setEngineLogMode(bool enabled);
        void setEngineLogPath(const std::string&);
        
        std::string createTournamentStats();
        
        void showEgineInOutToScreen(bool enabled);
        void shutdown();

		static void append2TextFile(const std::string& path, const std::string& str);
//        static void fixJson(Json::Value& d, const std::string& path);
        
        bool loadMatchRecords(bool autoYesReply);

    protected:
        void startTournament();
        std::vector<TourPlayer> collectStats() const;
        
        void reset();
        
        virtual bool parseJsonAfterLoading(Json::Value&) override;
        
        void addMatchRecord(MatchRecord& record);
        void addMatchRecord_simple(MatchRecord& record);

        void finishTournament();
        
        void engineLog(const Game* game, const std::string& name, const std::string& line, LogType logType, Side bySide = Side::none);
        
        bool createNextRoundMatches();
        int getLastRound() const;
        void checkToExtendMatches(int gIdx);
        

        // for all
        bool pairingMatchList(const std::vector<std::string>& nameList);
        bool pairingMatchList(std::vector<TourPlayer> playerVec, int round);
        bool pairingMatchListRecusive(std::vector<TourPlayer>& playerVec, int round, const std::set<std::string>& pairedSet);

        // Knockout
        std::vector<TourPlayer> getKnockoutWinnerList();
        bool createNextKnockoutMatchList();

        // Swiss
        bool createNextSwisstMatchList();

        //
        void matchCompleted(Game* game);
        bool addGame(Game* game);
        
        virtual void tickWork() override;
        
        void matchLog(const std::string& line, bool verbose);
        int uncompletedMatches();
        
    protected:
        std::string eventName = "Chess Tournament", siteName;
        
        CppTime::Timer timer;
        CppTime::timer_id mainTimerId;
        
        TourType type = TourType::none;
        TourState state = TourState::none;
        
        TimeController timeController;
        bool shufflePlayers = false;

        std::vector<std::string> participantList;
        std::vector<MatchRecord> matchRecordList;
        std::vector<Game*> gameList;
        PlayerMng playerMng;
        BookMng bookMng;

        void saveMatchRecords();
        void removeMatchRecordFile();
        
        void showTournamentInfo();
        int calcMatchNumber() const;
        
        static std::string createLogPath(std::string opath, bool onefile, bool usesurfix, bool includeGameResult, const Game* game, Side forSide = Side::none);
        
    private:
        int gameConcurrency = 1, gameperpair = 1, swissRounds = 6;
        bool resumable = true, swapPairSides = true;

        static void showPathInfo(const std::string& name, const std::string& path, bool mode);
        
        std::map<std::string, Profile> profileMap;
        std::map<std::string, EngineStats> engineStatsMap;

    private:
        
        // endgame
        std::string syzygyPath;
        
        GameConfig gameConfig;

        // inclusive players
        bool inclusivePlayerMode = false;
        std::set<std::string> inclusivePlayers;
        Side inclusivePlayerSide = Side::none;
        
        int previousElapsed = 0;
        time_t startTime;
        
        // for logging
        std::mutex matchMutex, logMutex;

        std::string pgnPath;
        bool pgnPathMode = true, logPgnAllInOneMode = false;
        bool logPgnRichMode = false, logPgnGameTitleSurfix = false;
        
        std::string logResultPath;
        bool logResultMode = false;
        
        std::string logEnginePath;
        bool logEngineAllInOneMode = false, logEngineMode = false;
        bool logEngineShowTime = false, logEngineGameTitleSurfix = false;
        bool logEngineBySides = false;

        bool logScreenEngineInOutMode = false;
    };
    
} // namespace banksia

#endif /* tourmng_hpp */

