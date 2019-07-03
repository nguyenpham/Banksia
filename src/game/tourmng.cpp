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


#include <fstream>
#include <iomanip> // for setprecision
#include <cmath>

#include "tourmng.h"

using namespace banksia;

TourMng* TourMng::instance = nullptr;


bool MatchRecord::isValid() const
{
    return !playernames[0].empty() && !playernames[1].empty() && gameCount >= 0;
}

std::string MatchRecord::toString() const
{
    std::ostringstream stringStream;
    stringStream << "names: " << playernames[0] << ", " << playernames[1]
    << ", status: " << static_cast<int>(state)
    << ", round: " << round
    << ", score: " << score
    << ", gameCount: " << gameCount;
    return stringStream.str();
}

//////////////////////////////
bool TourResult::isValid() const
{
    return !name.empty()
    && gameCnt >= 0 && winCnt >= 0 && drawCnt>= 0 && lossCnt>= 0
    && gameCnt == winCnt + drawCnt + lossCnt;
}

std::string TourResult::toString() const
{
    std::ostringstream stringStream;
    stringStream << name << "#games: "<< gameCnt << ", wdl: " << winCnt << ", " << drawCnt << ", " << lossCnt;
    return stringStream.str();
}

bool TourResult::smaller(const TourResult& other) const {
    return winCnt < other.winCnt
    || (winCnt == other.winCnt && (lossCnt > other.lossCnt || (lossCnt == other.lossCnt && drawCnt < other.drawCnt)));
}


//////////////////////////////
TourMng::TourMng()
{
    instance = this;
    timer.add(std::chrono::milliseconds(500), [=](CppTime::timer_id) { tick(); }, std::chrono::milliseconds(500));
}

TourMng::TourMng(const std::string& jsonPath)
{
    loadFromJsonFile(jsonPath);
}

TourMng::~TourMng()
{
}

static const char* tourTypeNames[] = {
    "roundrobin", "knockout", nullptr
};

bool TourMng::parseJsonAfterLoading(Json::Value& d)
{
    //
    // Most inportance
    //
    if (d.isMember("type")) {
        auto s = d["type"].asString();
        for(int t = 0; tourTypeNames[t]; t++) {
            if (tourTypeNames[t] == s) {
                type = static_cast<TourType>(t);
                break;
            }
        }
    }
    
    if (type == TourType::none) {
        std::cerr << "Error: missing parametter \"type\" or it is incorrect (should be \"roundrobin\", \"knockout\")!" << std::endl;
        return false;
    }
    
    std::vector<std::string> nameList;
    if (d.isMember("players")) {
        const Json::Value& array = d["players"];
        for (int i = 0; i < int(array.size()); i++){
            auto str = array[i].asString();
            if (!str.empty()) {
                nameList.push_back(str);
            }
        }
    }
    if (nameList.size() < 2) {
        std::cerr << "Error: missing parametter \"players\" or number < 2 (that is an array of player's names)" << std::endl;
        return false;
    }
    
    participantList = nameList;
    
    
    std::string enginConfigJsonPath = "./engines.json";
    auto s = "engine config path";
    if (d.isMember(s)) {
        enginConfigJsonPath = d[s].asString();
    }
    
    if (enginConfigJsonPath.empty() || !ConfigMng::instance->loadFromJsonFile(enginConfigJsonPath) || ConfigMng::instance->empty()) {
        std::cerr << "Error: missing parametter \"" << s << "\" or the file is not existed" << std::endl;
        return false;
    }
    
    auto ok = false;
    s = "time control";
    if (d.isMember(s)) {
        auto obj = d[s];
        ok = timeController.load(obj) && timeController.isValid();
    }
    if (!ok) {
        std::cerr << "Error: missing parametter \"" << s << "\" or corrupted data" << std::endl;
        return false;
    }
    
    //
    // Less inportance
    //
    s = "game per pair";
    if (d.isMember(s)) {
        gameperpair = std::max(1, d[s].asInt());
    }
    
    if (d.isMember("ponder")) {
        ponderMode = d["ponder"].asBool();
    }
    
    s = "opening book";
    if (d.isMember(s)) {
        auto obj = d[s];
        bookMng.load(obj);
    }
    
    if (d.isMember("event")) {
        eventName = d["event"].asString();
    }
    if (d.isMember("site")) {
        siteName = d["site"].asString();
    }
    
    if (d.isMember("concurrency")) {
        gameConcurrency = std::max(1, d["concurrency"].asInt());
    }
    
    s = "pgn path";
    if (d.isMember(s)) {
        if (d[s].isString()) {
            pgnPath = d[s].asString();
            pgnPathMode = true;
        } else {
            auto v = d[s];
            pgnPathMode = v["mode"].asBool();
            pgnPath = v["path"].asString();
        }
    }
    
    s = "result log";
    if (d.isMember(s)) {
        auto v = d[s];
        logResultMode = v["mode"].asBool();
        logResultPath = v["path"].asString();
    }
    
    s = "engine log";
    if (d.isMember(s)) {
        auto v = d[s];
        logEngineInOutMode = v["mode"].asBool();
        logEngineInOutShowTime = v.isMember("show time") && v["show time"].asBool();
        logEngineInOutPath = v["path"].asString();
    }
    
    return true;
}


void TourMng::tickWork()
{
    playerMng.tick();
    
    std::vector<Game*> stoppedGameList;
    
    for(auto && game : gameList) {
        if (game->getState() >= GameState::stopped) {
            
            if (game->getStateTick() > 10 && game->getStateTick() < 15) {
                std::cout << "Trouble: game stucked in stopped state " << game->getStateTick() << std::endl;
            }
            if (game->getState() == GameState::stopped) {
                game->setState(GameState::destroyed);
                matchCompleted(game);
            }
            
            auto undeattachCnt = 0;
            for(int sd = 0; sd < 2; sd++) {
                auto side = static_cast<Side>(sd);
                auto player = game->getPlayer(side);
                if (player) {
                    if (player->isSafeToDeattach()) {
                        auto player2 = game->deattachPlayer(side); assert(player == player2);
                        playerMng.returnPlayer(player2);
                    } else {
                        undeattachCnt++;
                        player->prepareToDeattach();
                    }
                }
            }
            
            if (!undeattachCnt)
                stoppedGameList.push_back(game);
        } else {
            game->tick();
        }
    }
    
    for(auto && game : stoppedGameList) {
        auto it = std::find(gameList.begin(), gameList.end(), game);
        if (it != gameList.end()) {
            gameList.erase(it);
        }
        delete game;
    }
    
    if (state == TourState::playing) {
        playMatches();
    }
}

static std::string bool2OnOffString(bool b)
{
    return b ? "on" : "off";
}

void TourMng::showPathInfo(const std::string& name, const std::string& path, bool mode)
{
    std::cout << " path of " << name << ": " << (path.empty() ? "<empty>" : path) << ", " << bool2OnOffString(mode) << std::endl;
}

void TourMng::startTournament()
{
    std::string info =
    "type: " + std::string(tourTypeNames[static_cast<int>(type)])
    + ", timer: " + timeController.toString()
    + ", players: " + std::to_string(participantList.size())
    + ", matches: " + std::to_string(matchRecordList.size())
    + ", concurrency: " + std::to_string(gameConcurrency)
    + ", ponder: " + bool2OnOffString(ponderMode)
    + ", book: " + bool2OnOffString(!bookMng.isEmpty());
    
    matchLog(info);
    
    showPathInfo("pgn", pgnPath, pgnPathMode);
    showPathInfo("log result", logResultPath, logResultMode);
    showPathInfo("log engines' in-output", logEngineInOutPath, logEngineInOutMode);
    std::cout << std::endl;
    
    // tickWork will start the matches
    state = TourState::playing;
}

void TourMng::finishTournament()
{
    state = TourState::done;
    
    if (matchRecordList.empty()) {
        return;
    }
    
    static const char* separateLine = "----------------------------------";
    matchLog(separateLine);
    
    auto str = createTournamentStats();
    matchLog(str);
    
    matchLog(separateLine);
    matchLog("\n");
    std::cout << "Tournamemt finished!\n";
    
    // WARNING: exit the app here after completed the tournament
    exit(0);
}

std::string TourMng::createTournamentStats()
{
    std::map<std::string, TourResult> resultMap;
    
    for(auto && m : matchRecordList) {
        if (m.resultType == ResultType::noresult) {
            continue;
        }
        
        for(int sd = 0; sd < 2; sd++) {
            auto name = m.playernames[sd];
            assert(!name.empty());
            TourResult r;
            auto it = resultMap.find(name);
            if (it == resultMap.end()) {
                r.name = name;
            } else {
                r = it->second; assert(r.name == name);
            }
            
            r.gameCnt++;
            switch (m.resultType) {
                case ResultType::win:
                    if (sd == W) r.winCnt++; else r.lossCnt++;
                    break;
                case ResultType::draw:
                    r.drawCnt++;
                    break;
                case ResultType::loss:
                    if (sd == B) r.winCnt++; else r.lossCnt++;
                    break;
                default:
                    assert(false);
                    break;
            }
            resultMap[name] = r;
        }
    }
    
    std::vector<TourResult> resultList;
    for (auto && s : resultMap) {
        resultList.push_back(s.second);
    }
    
    std::sort(resultList.begin(), resultList.end(), [](const TourResult& lhs, const TourResult& rhs)
              {
                  return lhs.smaller(rhs);
              });
    
    
    std::stringstream stringStream;
    
    stringStream << "rank name games wins draws" << std::endl;
    
    for(int i = 0; i < resultList.size(); i++) {
        auto r = resultList.at(i);
        
        auto d = double(std::max(1, r.gameCnt));
        double win = double(r.winCnt * 100) / d, draw = double(r.drawCnt * 100) / d;
        //        auto errorMagins = calcErrorMargins(r.winCnt, r.drawCnt, r.lossCnt);
        stringStream
        << (i + 1) << ". "
        << r.name << " \t" << r.gameCnt << " \t"
        // << (errorMagins > 0 ? "+" : "") << errorMagins << " "
        << std::fixed << std::setprecision(1)
        << win << "% \t" << draw << "%"
        << std::endl;
    }
    
    return stringStream.str();
}

void TourMng::playMatches()
{
    if (matchRecordList.empty()) {
        return finishTournament();
    }
    
    if (gameList.size() >= gameConcurrency) {
        return;
    }
    
    for(auto && m : matchRecordList) {
        if (m.state != MatchState::none) {
            continue;
        }
        
        createMatch(m);
        assert(m.state != MatchState::none);
        
        if (gameList.size() >= gameConcurrency) {
            break;
        }
    }
    
    if (gameList.empty()) {
        return finishTournament();
    }
}

void TourMng::createNextKnockoutMatchList()
{
    auto lastRound = 0;
    for(auto && r : matchRecordList) {
        lastRound = std::max(lastRound, r.round);
    }
    
    std::vector<std::string> nameList;
    for(auto && r : matchRecordList) {
        if (r.round == lastRound) {
            if (r.score > 0) {
                nameList.push_back(r.playernames[0]);
            } else {
                nameList.push_back(r.playernames[1]);
            }
        }
    }
    
    if (nameList.size() > 1) {
        createKnockoutMatchList(nameList, lastRound + 1);
    }
}

void TourMng::addMatchRecord(MatchRecord& record, bool returnMatch)
{
    addMatchRecord(record);
    
    if (returnMatch) {
        record.swapPlayers();
        addMatchRecord(record);
    }
}

void TourMng::addMatchRecord(MatchRecord& record)
{
    for(int i = 0; i < gameperpair; i++) {
        record.gameIdx = int(matchRecordList.size());
        bookMng.getRandomBook(record.startFen, record.startMoves);
        matchRecordList.push_back(record);
    }
}

void TourMng::createKnockoutMatchList(const std::vector<std::string>& nameList, int round)
{
    auto n = nameList.size() / 2;
    
    // the odd and the lucky last player win all game, put him to begin
    if (2 * n < nameList.size()) {
        auto name0 = nameList.back();
        
        MatchRecord record(name0, "");
        record.round = round;
        record.state = MatchState::completed;
        record.resultType = ResultType::win; // win
        addMatchRecord(record, false);
        
        record.swapPlayers();
        record.resultType = ResultType::loss;
        addMatchRecord(record, false);
    }
    
    for(int i = 0; i < n; i++) {
        auto name0 = nameList.at(i);
        auto name1 = nameList.at(i + n);
        
        MatchRecord record(name0, name1);
        record.round = round;
        addMatchRecord(record, true);
    }
}

bool TourMng::createMatchList()
{
    return createMatchList(participantList, type);
}

bool TourMng::createMatchList(const std::vector<std::string>& nameList, TourType tourType)
{
    matchRecordList.clear();
    
    if (nameList.size() < 2) {
        std::cerr << "Error: not enough players (" << nameList.size() << ") and/or unknown tournament type" << std::endl;
        return false;
    }
    
    std::string missingName;
    auto err = false;
    switch (tourType) {
        case TourType::roundrobin:
        {
            for(int i = 0; i < nameList.size() - 1 && !err; i++) {
                auto name0 = nameList.at(i);
                if (!ConfigMng::instance->isNameExistent(name0)) {
                    err = true; missingName = name0;
                    break;
                }
                for(int j = i + 1; j < nameList.size(); j++) {
                    auto name1 = nameList.at(j);
                    if (!ConfigMng::instance->isNameExistent(name1)) {
                        err = true; missingName = name0;
                        break;
                    }
                    
                    MatchRecord record(name0, name1);
                    record.round = 1;
                    addMatchRecord(record, true);
                }
            }
            
            if (err) {
                
            }
            break;
        }
        case TourType::knockout:
        {
            createKnockoutMatchList(nameList, 0);
            break;
        }
            
        default:
            break;
    }
    
    if (err) {
        std::cerr << "Error: missing engine configuration for name (case sensitive): " << missingName << std::endl;
        return false;
    }
    
    return true;
}

void TourMng::createMatch(MatchRecord& record)
{
    if (!record.isValid() ||
        !createMatch(record.gameIdx, record.playernames[W], record.playernames[B], record.startFen, record.startMoves)) {
        std::cerr << "Error: match record invalid or missing players " << record.toString() << std::endl;
        record.state = MatchState::error;
        return;
    }
    
    record.state = MatchState::playing;
}

bool TourMng::createMatch(int gameIdx, const std::string& whiteName, const std::string& blackName,
                          const std::string& startFen, const std::vector<MoveCore>& startMoves)
{
    Engine* engines[2];
    engines[W] = playerMng.createEngine(whiteName);
    engines[B] = playerMng.createEngine(blackName);
    
    if (engines[0] && engines[1]) {
        auto game = new Game(engines[W], engines[B], timeController, ponderMode);
        game->setStartup(gameIdx, startFen, startMoves);
        
        if (addGame(game)) {
            game->setMessageLogger([=](const std::string& name, const std::string& line, LogType logType) {
                engineLog(name, line, logType);
            });
            game->start();
            
            std::string infoString = std::to_string(gameIdx + 1) + ". " + game->getGameTitleString() + ", started";
            
            printText(infoString);
            engineLog(getAppName(), infoString, LogType::system);
            
            return true;
        }
        delete game;
    }
    
    for(int sd = 0; sd < 2; sd++) {
        playerMng.returnPlayer(engines[sd]);
    }
    return false;
}

std::string TourMng::resultToString(const Result& result)
{
    auto str = result.toShortString();
    if (result.reason != ReasonType::noreason) {
        str += " (" + result.reasonString() + ")";
    }
    return str;
}

// http://www.open-aurec.com/wbforum/viewtopic.php?t=949
//1) Use number of wins, loss, and draws
//W = number of wins, L = number of lost, D = number of draws
//n = number of games (W + L + D)
//m = mean value
//
//2) Apply the following formulas to compute s
//( SQRT: square root of. )
//
//x = W*(1-m)*(1-m) + D*(0.5-m)*(0.5-m) + L*(0-m)*(0-m)
//s = SQRT( x/(n-1) )
//
//3) Compute error margin A (use 1.96  for 95% confidence)
//
//A = 1.96 * s / SQRT(n)
//
//4) State with 95% confidence:
//The 'real' result should be somewhere in between m-A to m+A
//
//5) Lookup the ELO figures with the win% from m-A and m+A to get the lower and higher values in the error margin.

// sd = sqrt(wins * (1 - m)^2 + losses * (0 - m)^2 + draws * (0.5 - m)^2) / sqrt(n - 1)
// http://talkchess.com/forum3/viewtopic.php?topic_view=threads&p=441743&t=41773
//n = number of games
//w = number of won games
//d = number of drawn games
//l = number of lost games
//
//n = w + d + l
//W = w/n ; D = d/n ; L = l/n
//W + D + L = 1
//
//mu = (w + d/2)/n = W + D/2
//1 - mu = (d/2 + l)/n = D/2 + L
// sd = sqrt{(1/n)·[mu·(1 - mu) - D/4]}
//
// s = sqrt{[1/(n - 1)]·[W·(1 - mu)² + D·(1/2 - mu)² + L·mu²]}

int TourMng::calcErrorMargins(int w, int d, int l)
{
    auto n = w + d + l;
    assert(w >= 0 && d >= 0 && l >= 0 && n > 0);
    double W_ = double(w) / n, D_ = double(d) / n, L_ = l / double(n);
    double mu = (w + double(d) / 2) / double(n); // = W_ + D_ /2;
    
    auto sd = std::sqrt((1 / double(n)) * (mu * (1 - mu) - D_ / 4));
    return int(sd);
}

void TourMng::matchCompleted(Game* game)
{
    if (game == nullptr) return;
    
    auto gIdx = game->getIdx();
    if (gIdx >= 0 && gIdx < matchRecordList.size()) {
        auto record = &matchRecordList[gIdx];
        assert(record->state == MatchState::playing);
        record->state = MatchState::completed;
        record->resultType = game->board.result.result;
        assert(matchRecordList[gIdx].resultType == game->board.result.result);
        
        if (pgnPathMode && !pgnPath.empty()) {
            auto pgnString = game->toPgn(eventName, siteName, record->round);
            append2TextFile(pgnPath, pgnString);
        }
    }
    
    if (logResultMode || banksiaVerbose) { // log
        auto wplayer = game->getPlayer(Side::white), bplayer = game->getPlayer(Side::black);
        if (wplayer && bplayer) {
            std::string infoString = std::to_string(gIdx + 1) + ") " + game->getGameTitleString() + ": " + resultToString(game->board.result);
            
            matchLog(infoString);
            // Add extra info to help understanding log
            engineLog(getAppName(), infoString, LogType::system);
        }
    }
}

void TourMng::setupTimeController(TimeControlMode mode, int val, double t0, double t1, double t2)
{
    timeController.setup(mode, val, t0, t1, t2);
}

bool TourMng::addGame(Game* game)
{
    if (game == nullptr) return false;
    gameList.push_back(game);
    return true;
}

void TourMng::setEngineLogMode(bool enabled)
{
    logEngineInOutMode = enabled;
}

void TourMng::setEngineLogPath(const std::string& path)
{
    logEngineInOutPath = path;
}

void TourMng::matchLog(const std::string& infoString)
{
    printText(infoString);
    
    if (logResultMode && !logResultPath.empty()) {
        std::lock_guard<std::mutex> dolock(matchMutex);
        append2TextFile(logResultPath, infoString);
    }
}

void TourMng::showEgineInOutToScreen(bool enabled)
{
    logScreenEngineInOutMode = enabled;
}

void TourMng::engineLog(const std::string& name, const std::string& line, LogType logType)
{
    if (line.empty() || !logEngineInOutMode || logEngineInOutPath.empty()) return;
    
    std::ostringstream stringStream;

    if (logEngineInOutShowTime) {
        auto tm = localtime_xp(std::time(0));
        stringStream << std::put_time(&tm, "%H:%M:%S ");
    }
    stringStream << name << (logType == LogType::toEngine ? "< " : "> ") << line;
    
    auto str = stringStream.str();
    
    if (logScreenEngineInOutMode) {
        printText(str);
    }
    
    std::lock_guard<std::mutex> dolock(logMutex);
    append2TextFile(logEngineInOutPath, str);
}

void TourMng::append2TextFile(const std::string& path, const std::string& str)
{
    std::ofstream ofs(path, std::ios_base::out | std::ios_base::app);
    ofs << str << std::endl;
    ofs.close();
}



