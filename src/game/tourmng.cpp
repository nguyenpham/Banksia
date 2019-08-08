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


#include <fstream>
#include <iomanip> // for setprecision
#include <cmath>
#include <algorithm>
#include <random>
#include <ctime>
#include <cmath>

#include "tourmng.h"

#include "../3rdparty/json/json.h"
#include "../3rdparty/fathom/tbprobe.h"

using namespace banksia;

bool MatchRecord::isValid() const
{
    return !playernames[0].empty() && !playernames[1].empty();
}

std::string MatchRecord::toString() const
{
    std::ostringstream stringStream;
    stringStream << "names: " << playernames[0] << ", " << playernames[1]
    << ", status: " << static_cast<int>(state)
    << ", round: " << round;
    return stringStream.str();
}

bool MatchRecord::load(const Json::Value& obj)
{
    auto array = obj["players"];
    playernames[0] = array[0].asString();
    playernames[1] = array[1].asString();
    
    if (obj.isMember("startFen")) {
        startFen = obj["startFen"].asString();
    }
    
    startMoves.clear();
    if (obj.isMember("startMoves")) {
        auto array = obj["startMoves"];
        for (int i = 0; i < int(array.size()); i++){
            auto k = array[i].asInt();
            Move m(k & 0xff, k >> 8 & 0xff, static_cast<PieceType>(k >> 16 & 0xff));
            startMoves.push_back(m);
        }
    }
    
    auto s = obj["result"].asString();
    result.result = string2ResultType(s);
    s = obj["reason"].asString();
    result.reason= string2ReasonType(s);
    
    state = result.result == ResultType::noresult ? MatchState::none : MatchState::completed;
    
    gameIdx = obj["gameIdx"].asInt();
    round = obj["round"].asInt();
    pairId = obj["pairId"].asInt();
    return true;
}

Json::Value MatchRecord::saveToJson() const
{
    Json::Value obj;
    
    Json::Value players;
    players.append(playernames[0]);
    players.append(playernames[1]);
    obj["players"] = players;
    
    if (!startFen.empty()) {
        obj["startFen"] = startFen;
    }
    
    if (!startMoves.empty()) {
        Json::Value moves;
        for(auto && m : startMoves) {
            auto k = m.dest | m.from << 8 | static_cast<int>(m.promotion) << 16;
            moves.append(k);
        }
        
        obj["startMoves"] = moves;
    }
    
    obj["result"] = resultType2String(result.result);
    obj["reason"] = reasonType2String(result.reason);
    obj["gameIdx"] = gameIdx;
    obj["round"] = round;
    obj["pairId"] = pairId;
    return obj;
}

// https://www.chessprogramming.org/Match_Statistics
Elo::Elo(int wins, int draws, int losses) {
    elo_difference = los = 0.0;
    double games = wins + losses + draws;
    if (games == 0 || wins + draws == 0) {
        return;
    }
    double winning_fraction = (wins + 0.5 * draws) / games;
    if (winning_fraction == 1) {
        return;
    }
    elo_difference = -log(1.0 / winning_fraction - 1.0) * 400.0 / log(10.0);
    los = .5 + .5 * erf((wins - losses) / sqrt(2.0 * (wins + losses)));
}

//////////////////////////////
bool TourPlayer::isValid() const
{
    return !name.empty()
    && gameCnt >= 0 && winCnt >= 0 && drawCnt>= 0 && lossCnt>= 0
    && gameCnt == winCnt + drawCnt + lossCnt;
}

std::string TourPlayer::toString() const
{
    std::ostringstream stringStream;
    stringStream << name << "#games: "<< gameCnt << ", wdl: " << winCnt << ", " << drawCnt << ", " << lossCnt;
    return stringStream.str();
}

bool TourPlayer::smaller(const TourPlayer& other) const {
    return winCnt < other.winCnt
    || (winCnt == other.winCnt && (lossCnt > other.lossCnt || (lossCnt == other.lossCnt && drawCnt < other.drawCnt)));
}

double TourPlayer::getScore() const
{
    return winCnt * 1.0 + drawCnt * 0.5;
}

//////////////////////////////
TourMng::TourMng()
{
}

TourMng::~TourMng()
{
}

static const char* tourTypeNames[] = {
    "roundrobin", "knockout", "swiss", nullptr
};


bool TourMng::start(const std::string& mainJsonPath, bool yesReply, bool noReply)
{
    if (!loadFromJsonFile(mainJsonPath)) {
        return false;
    }
    
    showTournamentInfo();
    
    if ((noReply || !loadMatchRecords(yesReply))
        && !createMatchList()) {
        return false;
    }
    
    // The app will be terminated when all matches completed
    startTournament();
    return true;
}


bool TourMng::parseJsonAfterLoading(Json::Value& d)
{
    //
    // Most inportance
    //
    if (d.isMember("base")) {
        auto v = d["base"];
        
        auto s = v["type"].asString();
        for(int t = 0; tourTypeNames[t]; t++) {
            if (tourTypeNames[t] == s) {
                type = static_cast<TourType>(t);
                break;
            }
        }
        
        s = "swiss rounds";
        if (v.isMember(s)) {
            swissRounds = std::max(1, v[s].asInt());
        }
        
        s = "resumable";
        resumable = v.isMember(s) ? v[s].asBool() : true;
        
        s = "games per pair";
        if (v.isMember(s)) {
            gameperpair = std::max(1, v[s].asInt());
        }

        s = "swap pair sides";
        swapPairSides = !v.isMember(s) || v[s].asBool();

        s = "shuffle players";
        shufflePlayers = v.isMember(s) && v[s].asBool();
        
        s = "ponder";
        gameConfig.ponderMode = v.isMember(s) && v[s].asBool();
        
        s = "event";
        if (v.isMember(s)) {
            eventName = v[s].asString();
        }
        s = "site";
        if (v.isMember(s)) {
            siteName = v[s].asString();
        }
        
        s = "concurrency";
        if (v.isMember(s)) {
            gameConcurrency = std::max(1, v[s].asInt());
        }
    }
    
    // Engine configurations
    std::string enginConfigJsonPath = "./engines.json";
    bool enginConfigUpdate = false;
    auto s = "engine configurations";
    if (d.isMember(s)) {
        auto v = d[s];
        enginConfigUpdate = v["update"].isBool() && v["update"].asBool();
        enginConfigJsonPath = v["path"].asString();
    }
    
    if (enginConfigJsonPath.empty() || !ConfigMng::instance->loadFromJsonFile(enginConfigJsonPath) || ConfigMng::instance->empty()) {
        std::cerr << "Error: missing parametter \"" << s << "\" or the file is not existed" << std::endl;
        return false;
    }
    
    s = "override options";
    if (d.isMember(s)) {
        ConfigMng::instance->loadOverrideOptions(d[s]);
    }
    
    // Participants
    participantList.clear();
    if (d.isMember("players")) {
        const Json::Value& array = d["players"];
        for (int i = 0; i < int(array.size()); i++) {
            auto str = array[i].isString() ? array[i].asString() : "";
            if (!str.empty()) {
                if (ConfigMng::instance->isNameExistent(str)) {
                    participantList.push_back(str);
                } else {
                    std::cerr << "Error: player " << str << " (in \"players\") is not existent in engine configurations." << std::endl;
                }
            }
        }
    }
    
    inclusivePlayers.clear();
    s = "inclusive players";
    if (d.isMember(s)) {
        auto v = d[s];
        inclusivePlayerMode = v.isMember("mode") && v["mode"].asBool();
        if (v.isMember("side")) {
            auto str = v["side"].asString();
            inclusivePlayerSide = string2Side(str);
        }
        if (v.isMember("players")) {
            const Json::Value& array = v["players"];
            for (int i = 0; i < int(array.size()); i++) {
                auto str = array[i].isString() ? array[i].asString() : "";
                if (!str.empty()) {
                    if (ConfigMng::instance->isNameExistent(str)) {
                        inclusivePlayers.insert(str);
                    } else {
                        std::cerr << "Error: player " << str << " (in \"inclusive Players\") is not existent in engine configurations." << std::endl;
                    }
                }
            }
        }
        
        if (inclusivePlayerMode) {
            std::cout << "Warning: inclusive mode is on. Matchs are played only if they have players in inclusive list" << std::endl;
        }
    }
    
    
    // time control
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
    
    if (participantList.empty()) {
        std::cerr << "Warning: missing parametter \"players\". Will use all players in configure instead." << std::endl;
        participantList = ConfigMng::instance->nameList();
    }
    
    if (participantList.size() < 2) {
        std::cerr << "Error: number of players in parametter \"players\" is not enough for a tournament!" << std::endl;
        return false;
    }
    
    if (type == TourType::none) {
        std::cerr << "Error: missing parametter \"type\" or it is incorrect (should be \"roundrobin\", \"knockout\")!" << std::endl;
        return false;
    }
    
    //
    // Less inportance
    //
    s = "openings";
    if (d.isMember(s) || d.isMember("opening books")) {
        auto obj = d[d.isMember(s) ? s : "opening books"];
        bookMng.load(obj);
    }
    
    s = "endgames";
    if (d.isMember(s)) {
        auto obj = d[s];
        configMng.setSyzygyPath(obj["syzygypath"].asString());
    }
    
    s = "game adjudication";
    if (d.isMember(s)) {
        auto obj = d[s];
        gameConfig.adjudicationMode = obj.isMember("mode") && obj["mode"].asBool();
        gameConfig.adjudicationEgtbMode = obj.isMember("tablebase") && obj["tablebase"].asBool();
        gameConfig.adjudicationMaxGameLength = obj.isMember("draw if game length over") ? obj["draw if game length over"].asInt() : 0;
        gameConfig.adjudicationMaxPieces = obj.isMember("tablebase max pieces") ? obj["tablebase max pieces"].asInt() : 10;
    }
    
    s = "logs";
    if (d.isMember(s)) {
        auto a = d[s];
        s = "pgn";
        if (a.isMember(s)) {
            auto v = a[s];
            pgnPathMode = v["mode"].asBool();
            pgnPath = v["path"].asString();
            logPgnAllInOneMode = !v.isMember("one file") || v["one file"].asBool();
            logPgnGameTitleSurfix = v.isMember("game title surfix") && v["game title surfix"].asBool();
            logPgnRichMode = v.isMember("rich info") && v["rich info"].asBool();
        }
        
        s = "engine";
        if (a.isMember(s)) {
            auto v = a[s];
            logEngineMode = v["mode"].asBool();
            logEngineAllInOneMode = v.isMember("one file") && v["one file"].asBool();
            logEngineBySides = v.isMember("separate by sides") && v["separate by sides"].asBool();
            
            logEngineGameTitleSurfix = v.isMember("game title surfix") && v["game title surfix"].asBool();
            logEngineShowTime = v.isMember("show time") && v["show time"].asBool();
            logEnginePath = v["path"].asString();
        }
        
        s = "result";
        if (a.isMember(s)) {
            auto v = a[s];
            logResultMode = v["mode"].asBool();
            logResultPath = v["path"].asString();
        }
        
    }
    
    
    if (gameConfig.adjudicationMode) {
        auto path = configMng.getSyzygyPath();
        if (!path.empty()) {
            Tablebase::SyzygyTablebase::tb_init(path);
        }
    }
    
    // Info about books & tablebases
    std::cout << bookMng.toString();
    if (gameConfig.adjudicationMode) {
        std::cout << Tablebase::SyzygyTablebase::toString();
    }
    std::cout << std::endl;
    
    // Check cores and memory
    {
        // engine concurrency
        auto n = gameConcurrency * 2;
        
        auto threads = n * std::max(1, configMng.getEngineThreads());
        auto memory = n * configMng.getEngineMemory();
        auto cores = getNumberOfCores();
        if (threads >= cores) {
            std::cout << "Warning: concurrent engines (" << n << ") may use from " << threads << " threads, more than the number of computer cores (" << cores << ")" << std::endl;
        }
        auto sysMem = getMemorySize() / (1024 * 1024);
        if (memory >= sysMem * 3 / 4) {
            std::cout << "Warning: concurrent engines (" << n << ") may use from " << memory << " MB memory" << std::endl;
        }
    }
    return true;
}


void TourMng::tickWork()
{
    playerMng.tick();
    
    std::vector<Game*> stoppedGameList;
    
    for(auto && game : gameList) {
        game->tick();
        auto st = game->getState();
        switch (st) {
            case GameState::stopped:
                game->setState(GameState::ending);
                matchCompleted(game);
                break;
                
            case GameState::ended:
            {
                for(int sd = 0; sd < 2; sd++) {
                    auto side = static_cast<Side>(sd);
                    auto player = game->getPlayer(side);
                    if (player) {
                        player->quit();
                    }
                }
                
                stoppedGameList.push_back(game);
                break;
            }
            default:
                break;
        }
    }
    
    for(auto && game : stoppedGameList) {
        for(int sd = 0; sd < 2; sd++) {
            auto side = static_cast<Side>(sd);
            auto player = game->getPlayer(side);
            if (player) {
                auto player2 = game->deattachPlayer(side); assert(player == player2);
                playerMng.returnPlayer(player2);
            }
        }
        
        auto it = std::find(gameList.begin(), gameList.end(), game);
        if (it == gameList.end()) {
            std::cerr << "Error: somethings wrong, cannot find to delete the game " << game->getGameTitleString() << std::endl;
        } else {
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
    std::cout << " " << name << ": " << (path.empty() ? "<empty>" : path) << ", " << bool2OnOffString(mode) << std::endl;
}

int TourMng::calcMatchNumber() const
{
    auto matchCnt = 0;
    switch (type) {
        case TourType::roundrobin:
        {
            for(int i = 0; i < participantList.size(); i++) {
                for(int j = i + 1; j < participantList.size(); j++) {
                    matchCnt++;
                }
            }
            break;
        }
        case TourType::knockout:
        {
            auto n = (int)participantList.size();
            while(n >= 2) {
                n >>= 1;
                matchCnt += n;
            }
            break;
        }
        case TourType::swiss:
        {
            auto n = int(participantList.size());
            matchCnt = (n >> 1) * swissRounds;
            break;
        }
        default:
            break;
    }
    
    return matchCnt * gameperpair;
}

void TourMng::showTournamentInfo()
{
    std::string info =
    "type: " + std::string(tourTypeNames[static_cast<int>(type)])
    + ", timer: " + timeController.toString()
    + ", players: " + std::to_string(participantList.size())
    + ", games per pair: " + std::to_string(gameperpair);
    
    if (type == TourType::swiss) {
        info += ", round: " + std::to_string(swissRounds);
    }
    info +=
    + ", matches: " + std::to_string(calcMatchNumber())
    + ", concurrency: " + std::to_string(gameConcurrency)
    + ", ponder: " + bool2OnOffString(gameConfig.ponderMode)
    + ", book: " + bool2OnOffString(!bookMng.isEmpty());
    
    matchLog(info, true);
    
    showPathInfo("pgn", pgnPath, pgnPathMode);
    showPathInfo("result", logResultPath, logResultMode);
    showPathInfo("engines", logEnginePath, logEngineMode);
    std::cout << std::endl;
}

void TourMng::startTournament()
{
    startTime = time(nullptr);
    
    // tickWork will start the matches
    state = TourState::playing;
    
    mainTimerId = timer.add(std::chrono::milliseconds(500), [=](CppTime::timer_id) { tick(); }, std::chrono::milliseconds(500));
}

void TourMng::finishTournament()
{
    state = TourState::done;
    auto elapsed_secs = previousElapsed + static_cast<int>(time(nullptr) - startTime);
    
    if (!matchRecordList.empty()) {
        auto str = createTournamentStats();
        matchLog(str, true);
    }
    
    auto str = "Tournamemt finished! Elapsed: " + formatPeriod(elapsed_secs);
    matchLog(str, true);
    
    removeMatchRecordFile();
    
    // WARNING: exit the app here after completed the tournament
    shutdown();
    exit(0);
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
    
    if (gameList.empty() && !createNextRoundMatches()) {
        return finishTournament();
    }
}

void TourMng::addMatchRecord(MatchRecord& record)
{
    record.pairId = std::rand();
    for(int i = 0; i < gameperpair; i++) {
        addMatchRecord_simple(record);
        if (swapPairSides) {
            record.swapPlayers();
        }
    }
}

void TourMng::addMatchRecord_simple(MatchRecord& record)
{
    if (inclusivePlayerMode) {
        auto ok = inclusivePlayerSide != Side::black && inclusivePlayers.find(record.playernames[W]) != inclusivePlayers.end();
        if (!ok) {
            ok = inclusivePlayerSide != Side::white && inclusivePlayers.find(record.playernames[B]) != inclusivePlayers.end();
            
            if (!ok) {
                return;
            }
        }
    }
    record.gameIdx = int(matchRecordList.size());
    bookMng.getRandomBook(record.pairId, record.startFen, record.startMoves);
    matchRecordList.push_back(record);
}

bool TourMng::createNextRoundMatches()
{
    switch (type) {
        case TourType::roundrobin:
            return false;
            
        case TourType::knockout:
            return createNextKnockoutMatchList();
            
        case TourType::swiss:
            return createNextSwisstMatchList();
            
        default:
            break;
    }
    return false;
}

// This function used to break the tie between a pair of players in knockout
// It is not a tie if one has more win or more white games
void TourMng::checkToExtendMatches(int gIdx)
{
    if (type != TourType::knockout || gIdx < 0) {
        return;
    }
    
    for(auto && r : matchRecordList) {
        if (r.gameIdx == gIdx) {
            TourPlayerPair playerPair;
            playerPair.pair[0].name = r.playernames[0];
            playerPair.pair[1].name = r.playernames[1];
            auto pairId = r.pairId;
            
            for(auto && rcd : matchRecordList) {
                if (rcd.pairId != pairId) {
                    continue;
                }
                
                // some matches are not completed -> no extend
                if (rcd.state != MatchState::completed) {
                    return;
                }
                if (rcd.result.result != ResultType::win && rcd.result.result  != ResultType::loss) {
                    continue;
                }
                auto winnerName = rcd.playernames[(rcd.result.result  == ResultType::win ? W : B)];
                playerPair.pair[playerPair.pair[W].name == winnerName ? W : B].winCnt++;
                
                auto whiteIdx = playerPair.pair[W].name == rcd.playernames[W] ? W : B;
                playerPair.pair[whiteIdx].whiteCnt++;
            }
            
            // It is a tie if two players have same wins and same times to play white
            if (playerPair.pair[0].winCnt == playerPair.pair[1].winCnt && playerPair.pair[0].whiteCnt == playerPair.pair[1].whiteCnt) {
                MatchRecord record = r;
                record.result.result  = ResultType::noresult;
                record.state = MatchState::none;
                addMatchRecord_simple(record);
                
                auto str = "* Tied! Add one more game for " + record.playernames[W] + " vs " + record.playernames[B];
                matchLog(str, banksiaVerbose);
            }
            break;
        }
    }
}

int TourMng::getLastRound() const
{
    int lastRound = 0;
    for(auto && r : matchRecordList) {
        lastRound = std::max(lastRound, r.round);
    }
    return lastRound;
}

void TourMng::reset()
{
    matchRecordList.clear();
    previousElapsed = 0;
}

bool TourMng::createMatchList()
{
    return createMatchList(participantList, type);
}

bool TourMng::createMatchList(std::vector<std::string> nameList, TourType tourType)
{
    reset();
    
    if (nameList.size() < 2) {
        std::cerr << "Error: not enough players (" << nameList.size() << ") and/or unknown tournament type" << std::endl;
        return false;
    }
    
    if (shufflePlayers) {
        auto rng = std::default_random_engine {};
        rng.seed((unsigned int)std::chrono::system_clock::now().time_since_epoch().count());
        std::shuffle(std::begin(nameList), std::end(nameList), rng);
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
                        err = true; missingName = name1;
                        break;
                    }
                    
                    // random swap to avoid name0 player plays all white side
                    MatchRecord record(name0, name1, swapPairSides && (rand() & 1));
                    record.round = 1;
                    addMatchRecord(record);
                }
            }
            
            if (err) {
                
            }
            break;
        }
        case TourType::knockout:
        case TourType::swiss:
        {
            pairingMatchList(nameList);
            break;
        }
            
        default:
            break;
    }
    
    if (err) {
        std::cerr << "Error: missing engine configuration for name (case sensitive): " << missingName << std::endl;
        return false;
    }
    
    saveMatchRecords();
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
                          const std::string& startFen, const std::vector<Move>& startMoves)
{
    Engine* engines[2];
    engines[W] = playerMng.createEngine(whiteName);
    engines[B] = playerMng.createEngine(blackName);
    
    if (engines[0] && engines[1]) {
        auto game = new Game(engines[W], engines[B], timeController, gameConfig);
        game->setStartup(gameIdx, startFen, startMoves);
        
        if (addGame(game)) {
            game->setMessageLogger([=](const std::string& name, const std::string& line, LogType logType) {
                auto white = game->getPlayer(Side::white);
                auto fromSide = white && white->getName() == name ? Side::white : Side::black;
                engineLog(game, name, line, logType, fromSide);
            });
            game->kickStart();
            
            std::string infoString = std::to_string(gameIdx + 1) + ". " + game->getGameTitleString();
            
            if (banksiaVerbose) {
                printText(infoString);
            }
            
            if (!logEngineBySides) {
                engineLog(game, getAppName(), "\n" + infoString + "\n", LogType::system);
            }
            
            return true;
        }
        delete game;
    }
    
    for(int sd = 0; sd < 2; sd++) {
        playerMng.returnPlayer(engines[sd]);
    }
    return false;
}

std::vector<TourPlayer> TourMng::getKnockoutWinnerList()
{
    std::vector<TourPlayer> winList;
    auto lastRound = getLastRound();
    
    std::set<std::string> lostSet;
    std::vector<std::string> nameList;
    std::map<int, TourPlayerPair> pairMap;
    
    for(auto && r : matchRecordList) {
        if (r.round != lastRound) {
            continue;
        }
        
        assert(r.state == MatchState::completed);
        TourPlayerPair thePair;
        auto it = pairMap.find(r.pairId);
        if (it != pairMap.end()) thePair = it->second;
        else {
            thePair.pair[0].name = r.playernames[0];
            thePair.pair[1].name = r.playernames[1];
        }
        
        if (r.result.result == ResultType::win || r.result.result == ResultType::loss) {
            auto idxW = thePair.pair[W].name == r.playernames[W] ? W : B;
            auto winIdx = r.result.result == ResultType::win ? idxW : (1 - idxW);
            thePair.pair[winIdx].winCnt++;
        }
        auto whiteSd = thePair.pair[W].name == r.playernames[W] ? W : B;
        thePair.pair[whiteSd].whiteCnt++;
        pairMap[r.pairId] = thePair;
    }
    
    for(auto && p : pairMap) {
        auto thePair = p.second;
        assert(thePair.pair[0].winCnt != thePair.pair[1].winCnt || thePair.pair[0].whiteCnt != thePair.pair[1].whiteCnt);
        auto winIdx = W;
        if (thePair.pair[B].winCnt > thePair.pair[W].winCnt ||
            (thePair.pair[B].winCnt == thePair.pair[W].winCnt && thePair.pair[B].whiteCnt < thePair.pair[W].whiteCnt)) {
            winIdx = B;
        }
        winList.push_back(thePair.pair[winIdx]);
    }
    
    return winList;
}


bool TourMng::createNextKnockoutMatchList()
{
    auto list = getKnockoutWinnerList();
    return pairingMatchList(list, getLastRound() + 1);
}

bool TourMng::createNextSwisstMatchList()
{
    auto round = getLastRound() + 1;
    if (round >= swissRounds) {
        return false;
    }
    auto list = collectStats();
    return pairingMatchList(list, round);
}

bool TourMng::pairingMatchList(const std::vector<std::string>& nameList)
{
    std::vector<TourPlayer> vec;
    for(auto && name : nameList) {
        TourPlayer tourPlayer;
        tourPlayer.name = name;
        vec.push_back(tourPlayer);
    }
    
    return pairingMatchList(vec, 0);
}

bool TourMng::pairingMatchListRecusive(std::vector<TourPlayer>& playerVec, int round, const std::set<std::string>& pairedSet)
{
    auto n = playerVec.size(); assert((n & 1) == 0); // must be even
    
    auto r = true;
    for(int i = 0; i < n; ++i) {
        auto& player0 = playerVec.at(i);
        if (player0.flag & 1) continue;
        r = false;
        auto name0 = player0.name;
        player0.flag |= 1;
        
        for(int j = i + 1; j < n; ++j) {
            auto& player1 = playerVec.at(j);
            if (player1.flag & 1) continue;
            auto name1 = player1.name;
            
            if (pairedSet.find(name0 + "*" + name1) != pairedSet.end()) {
                continue;
            }
            
            player1.flag |= 1;
            
            if (!pairingMatchListRecusive(playerVec, round, pairedSet)) {
                player1.flag &= ~1;
                continue;
            }
            
            // random swap to avoid name0 player plays all white side
            auto swap = rand() & 1;
            
            if (type == TourType::swiss) {
                swap = player0.whiteCnt > player1.whiteCnt;
            }
            MatchRecord record(name0, name1, swapPairSides && swap);
            record.round = round;
            addMatchRecord(record);
            return true;
        }
        player0.flag &= ~1;
        
    }
    return r;
}

bool TourMng::pairingMatchList(std::vector<TourPlayer> playerVec, int round)
{
    if (playerVec.size() < 2) {
        if (playerVec.size() == 1) {
            auto str = "\n* The winner is " + playerVec.front().name;
            matchLog(str, true);
        }
        return false;
    }
    
    // odd/bye players, one won't have opponent and he is lucky to set win
    if (playerVec.size() & 1) {
        auto luckyIdx = -1;
        for (int i = 0; i < 10; i++) {
            auto k = std::rand() % playerVec.size();
            if (playerVec.at(k).byeCnt == 0) {
                luckyIdx = int(k);
                break;
            }
        }
        
        if (luckyIdx < 0) {
            for (int i = 0; i <  playerVec.size(); i++) {
                if (playerVec.at(i).byeCnt == 0) {
                    luckyIdx = i;
                    break;
                }
            }
        }
        
        if (luckyIdx < 0) {
            luckyIdx = std::rand() % playerVec.size();
            std::cerr << "Warning: all players have been bye (odd players who got free wins)." << std::endl;
        }
        
        auto luckPlayer = playerVec.at(luckyIdx);
        
        {
            auto it = playerVec.begin();
            std::advance(it, luckyIdx);
            playerVec.erase(it);
        }
        
        
        // the odd player wins all games in the round
        MatchRecord record(luckPlayer.name, "", false);
        record.round = round;
        record.state = MatchState::completed;
        record.result.result = ResultType::win; // win
        record.pairId = std::rand();
        addMatchRecord_simple(record);
        
        auto str = "\n* Player " + luckPlayer.name + " is an odd one (no opponent to pair with) and receives a bye (a win) for round " + std::to_string(round + 1);
        matchLog(str, banksiaVerbose);
    }
    
    std::sort(playerVec.begin(), playerVec.end(), [](const TourPlayer& lhs, const TourPlayer& rhs)
              {
                  return lhs.getScore() > rhs.getScore();
              });
    
    std::set<std::string> pairedSet;
    for(auto && m : matchRecordList) {
        if (m.playernames[0].empty() || m.playernames[1].empty()) continue;
        pairedSet.insert(m.playernames[0] + "*" + m.playernames[1]);
    }
    
    if (!pairingMatchListRecusive(playerVec, round, pairedSet)) {
        std::cout << "Warning: All players have played together already." << std::endl;
        pairedSet.clear();
        if (!pairingMatchListRecusive(playerVec, round, pairedSet)) {
            std::cerr << "Error: cannot pair players." << std::endl;
            return false;
        }
    }
    
    std::string str = "\n" + std::string(tourTypeNames[static_cast<int>(type)]) + " round: " + std::to_string(round + 1);
    if (type == TourType::swiss) {
        str += "/" + std::to_string(swissRounds);
    }
    str += ", pairs: " + std::to_string(playerVec.size() / 2) + ", matches: " + std::to_string(uncompletedMatches());
    
    matchLog(str, true);
    return true;
}

std::string TourMng::createLogPath(std::string opath, bool onefile, bool usesurfix, bool includeGameResult, const Game* game, Side forSide)
{
    if (onefile || game == nullptr) return opath;
    
    std::string s = (usesurfix ? ", " : "-") + std::to_string(game->getIdx() + 1);
    if (usesurfix) {
        if (!game->getPlayer(Side::white) || !game->getPlayer(Side::black)) {
            return "";
        }
        s += ") " + game->getGameTitleString(includeGameResult);
    }
    
    if (forSide != Side::none) {
        s += ", " + side2String(forSide, true);
    }
    
    auto p = opath.rfind(".");
    if (p == std::string::npos) {
        opath += s;
    } else {
        opath = opath.substr(0, p) + s + opath.substr(p);
    }
    return opath;
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
    logEngineMode = enabled;
}

void TourMng::setEngineLogPath(const std::string& path)
{
    logEnginePath = path;
}

void TourMng::matchLog(const std::string& infoString, bool verbose)
{
    if (verbose) {
        printText(infoString);
    }
    
    if (logResultMode && !logResultPath.empty()) {
        std::lock_guard<std::mutex> dolock(matchMutex);
        append2TextFile(logResultPath, infoString);
    }
}

void TourMng::showEgineInOutToScreen(bool enabled)
{
    logScreenEngineInOutMode = enabled;
}

void TourMng::engineLog(const Game* game, const std::string& name, const std::string& line, LogType logType, Side bySide)
{
    if (line.empty() || !logEngineMode || logEnginePath.empty()) return;
    
    std::ostringstream stringStream;
    
    auto gameIdx = game ? game->getIdx() : -1;
    if (logEngineAllInOneMode && gameIdx >= 0 && gameConcurrency > 1) {
        stringStream << (gameIdx + 1) << ".";
    }
    
    if (logEngineShowTime) {
        auto tm = localtime_xp(std::time(0));
        stringStream << std::put_time(&tm, "%H:%M:%S ");
    }
    
    if (!logEngineBySides && bySide != Side::none) {
        stringStream << name;
    }
    
    stringStream << (logType == LogType::toEngine ? "< " : "> ") << line;
    
    auto str = stringStream.str();
    
    if (logScreenEngineInOutMode) {
        printText(str);
    }
    
    auto forSide = Side::none;
    if (logEngineBySides) {
        forSide = bySide;
    }
    auto path = createLogPath(logEnginePath, logEngineAllInOneMode, logEngineGameTitleSurfix, false, game, forSide);
    
    if (!path.empty()) {
        std::lock_guard<std::mutex> dolock(logMutex);
        append2TextFile(path, str);
    }
}

void TourMng::append2TextFile(const std::string& path, const std::string& str)
{
    std::ofstream ofs(path, std::ios_base::out | std::ios_base::app);
    ofs << str << std::endl;
    ofs.close();
}


void TourMng::shutdown()
{
    timer.remove(mainTimerId);
    playerMng.shutdown();
}

int TourMng::uncompletedMatches()
{
    auto cnt = 0;
    for(auto && r : matchRecordList) {
        if (r.state == MatchState::none) {
            cnt++;
        }
    }
    return cnt;
}


#ifdef _WIN32
const std::string matchPath = "playing.json";
#else
const std::string matchPath = "./playing.json";
#endif


void TourMng::removeMatchRecordFile()
{
    std::remove(matchPath.c_str());
}

void TourMng::saveMatchRecords()
{
    if (!resumable) {
        return;
    }
    
    Json::Value d;
    
    d["type"] = tourTypeNames[static_cast<int>(type)];
    d["timeControl"] = timeController.saveToJson();
    
    Json::Value a;
    for(auto && r : matchRecordList) {
        a.append(r.saveToJson());
    }
    d["recordList"] = a;
    d["elapsed"] = static_cast<int>(time(nullptr) - startTime);
    
    JsonSavable::saveToJsonFile(matchPath, d);
}

bool TourMng::loadMatchRecords(bool autoYesReply)
{
    Json::Value d;
    if (!resumable || !loadFromJsonFile(matchPath, d, false)) {
        return false;
    }
    
    auto uncompletedCnt = 0;
    std::vector<MatchRecord> recordList;
    auto array = d["recordList"];
    for(int i = 0; i < int(array.size()); i++) {
        auto v = array[i];
        MatchRecord record;
        if (record.load(v)) {
            recordList.push_back(record);
            if (record.state == MatchState::none) {
                uncompletedCnt++;
            }
        }
    }
    
    if (uncompletedCnt == 0) {
        removeMatchRecordFile();
        return false;
    }
    
    std::cout << "\nThere are " << uncompletedCnt << " (of " << recordList.size() << ") uncompleted matches from previous tournament! Do you want to resume? (y/n)" << std::endl;
    
    while (!autoYesReply) {
        std::string line;
        std::getline(std::cin, line);
        banksia::trim(line);
        if (line.empty()) {
            continue;
        }
        
        if (line == "n" || line == "no") {
            removeMatchRecordFile();
            std::cout << "Discarded last tournament!" << std::endl;
            return false;
        }
        if (line == "y" || line == "yes") {
            break;
        }
    }
    
    std::cout << "Tournament resumed!" << std::endl;
    
    matchRecordList = recordList;
    
    auto first = matchRecordList.front();
    
    if (d.isMember("type")) {
        auto s = d["type"].asString();
        for(int t = 0; tourTypeNames[t]; t++) {
            if (tourTypeNames[t] == s) {
                type = static_cast<TourType>(t);
            }
        }
    }
    
    auto s = "timeControl";
    if (d.isMember(s)) {
        auto obj = d[s];
        auto oldTimeControl = timeController.saveToJson();
        if (!timeController.load(obj) || !timeController.isValid()) {
            timeController.load(oldTimeControl);
            std::cerr << "Error: TimeControl is incorrect. Reload default one." << std::endl;
        }
    }

    assert(timeController.isValid());
    previousElapsed += d["elapsed"].asInt();
    
    removeMatchRecordFile();
    
    startTournament();
    return true;
}

void TourMng::matchCompleted(Game* game)
{
    if (game == nullptr) return;
    
    auto gIdx = game->getIdx();
    if (gIdx >= 0 && gIdx < matchRecordList.size()) {
        auto record = &matchRecordList[gIdx];
        assert(record->state == MatchState::playing);
        record->state = MatchState::completed;
        record->result = game->board.result;
        
        EngineStats engineStats[2];
        for(auto && hist : game->board.histList) {
            // not for uncomputing moves
            if (hist.nodes == 0) {
                continue;
            }
            auto sd = static_cast<int>(hist.move.piece.side);
            engineStats[sd].nodes += hist.nodes;
            engineStats[sd].depths += hist.depth;
            engineStats[sd].elapsed += hist.elapsed;
            engineStats[sd].moves++;
        }
        
        for(int sd = 0; sd < 2; sd++) {
            engineStats[sd].games++;
            auto side = static_cast<Side>(sd);
            auto name = game->getPlayer(side)->getName();
            auto it = engineStatsMap.find(name);
            if (it != engineStatsMap.end()) {
                engineStats[sd].add(it->second);
            }
            engineStatsMap[name] = engineStats[sd];
            //            std::cout << "adding, sd: " << sd << ", name: " << name << ", #n: " << game->board.histList.size()
            //            << ", games: " << engineStats[sd].games
            //            << ", nodes: " << engineStats[sd].nodes
            //            << ", depths: " << engineStats[sd].depths
            //            << ", moves: " << engineStats[sd].moves
            //            << ", elapsed: " << engineStats[sd].elapsed
            //            << std::endl;
        }
        
        if (pgnPathMode && !pgnPath.empty()) {
            auto pgnString = game->toPgn(eventName, siteName, record->round, record->gameIdx, logPgnRichMode);
            auto path = createLogPath(pgnPath, logPgnAllInOneMode, logPgnGameTitleSurfix, true, game);
            if (!path.empty()) {
                append2TextFile(path, pgnString);
            }
        }
    }
    
    auto wplayer = (EngineProfile*)game->getPlayer(Side::white), bplayer = (EngineProfile*)game->getPlayer(Side::black);
    if (wplayer && bplayer) {
        std::ostringstream stringStream;
        
        stringStream << (gIdx + 1) << ") " << game->getGameTitleString()
        << ", #" <<game->board.histList.size()
        << ", " << game->board.result.toString();
        
        if (profileMode) {
            auto w = int(std::max(wplayer->getName().length(), bplayer->getName().length()));
            stringStream
            << "\n\t" << std::setw(w) << wplayer->getName() << std::setw(0) << ": " << wplayer->profile.toString(false)
            << "\n\t" << std::setw(w) << bplayer->getName() << std::setw(0) << ": " << bplayer->profile.toString(false);
            
            Profile profile = wplayer->profile;
            auto it = profileMap.find(wplayer->getName());
            if (it != profileMap.end()) {
                profile.addFrom(it->second);
            }
            profileMap[wplayer->getName()] = profile;
            
            profile = bplayer->profile;
            it = profileMap.find(bplayer->getName());
            if (it != profileMap.end()) {
                profile.addFrom(it->second);
            }
            profileMap[bplayer->getName()] = profile;
        }
        
        auto infoString = stringStream.str();
        matchLog(infoString, banksiaVerbose);
        // Add extra info to help understanding log
        if (!logEngineBySides) {
            engineLog(game, getAppName(), infoString, LogType::system);
        }
    }
    
    checkToExtendMatches(gIdx);
    
    saveMatchRecords();
}

std::vector<TourPlayer> TourMng::collectStats() const
{
    std::map<std::string, TourPlayer> resultMap;
    
    for(auto && m : matchRecordList) {
        if (m.result.result == ResultType::noresult) { // hm ?
            continue;
        }
        
        for(int sd = 0; sd < 2; sd++) {
            auto name = m.playernames[sd];
            if (name.empty()) { // bye players (in knockout) won without opponents
                continue;
            }
            
            TourPlayer r;
            auto it = resultMap.find(name);
            if (it == resultMap.end()) {
                r.name = name;
            } else {
                r = it->second; assert(r.name == name);
            }
            
            if (m.playernames[1 - sd].empty()) { // bye player
                r.byeCnt++;
            }
            
            auto lossCnt = r.lossCnt;
            r.gameCnt++;
            switch (m.result.result) {
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
            
            if (lossCnt < r.lossCnt) {
                if (m.result.reason == ReasonType::illegalmove || m.result.reason == ReasonType::crash || m.result.reason == ReasonType::timeout) {
                    r.abnormalCnt++;
                }
            }
            resultMap[name] = r;
        }
    }
    
    std::vector<TourPlayer> resultList;
    for (auto && s : resultMap) {
        resultList.push_back(s.second);
    }
    return resultList;
}

std::string TourMng::createTournamentStats()
{
    
    auto resultList = collectStats();
    
    auto maxNameLen = 0, abnormalCnt = 0;
    for (auto && r : resultList) {
        maxNameLen = std::max(maxNameLen, int(r.name.length()));
        abnormalCnt += r.abnormalCnt;
    }
    
    std::sort(resultList.begin(), resultList.end(), [](const TourPlayer& lhs, const TourPlayer& rhs)
              {
                  return rhs.smaller(lhs); // return lhs.smaller(rhs);
              });
    
    
    std::stringstream stringStream;
    stringStream.precision(1);
    stringStream << std::fixed;
    
    auto separateLineSz = maxNameLen + 62;
    for(int i = 0; i < separateLineSz; i++) {
        stringStream << "-";
    }
    stringStream << std::endl;
    
    stringStream << "  #  "
    << std::left << std::setw(maxNameLen + 2) << "name"
    << "games   wins%  draws% losses%   score    los%  elo+/-"
    << std::endl;
    
    const int w = 8, pw = 7;
    
    for(int i = 0; i < resultList.size(); i++) {
        auto r = resultList.at(i);
        
        auto d = double(std::max(1, r.gameCnt));
        double win = double(r.winCnt * 100) / d, draw = double(r.drawCnt * 100) / d, loss = double(r.lossCnt * 100) / d;
        
        double score = double(r.winCnt) + double(r.drawCnt) / 2;
        Elo elo(r.winCnt, r.drawCnt, r.lossCnt);
        
        stringStream
        << std::right << std::setw(3) << (i + 1) << ". "
        << std::left << std::setw(maxNameLen + 2) << r.name
        << std::right << std::setw(5) << r.gameCnt
        //        << std::fixed << std::setprecision(1)
        << std::right << std::setw(w) << win // << std::left << std::setw(0) << "%"
        << std::right << std::setw(w) << draw // << std::left << std::setw(0) << "%"
        << std::right << std::setw(w) << loss // << std::left << std::setw(0) << "%"
        << std::right << std::setw(w) << score
        << std::right << std::setw(w) << elo.los * 100
        << std::right << std::setw(w) << elo.elo_difference;
        
        stringStream << std::left << std::setw(0) << std::endl;
    }
    
    for(int i = 0; i < separateLineSz; i++) {
        stringStream << "-";
    }
    
    /////////////
    stringStream << std::endl << "\nTech (average nodes, depths, time/m per move, others per game):\n";
    
    EngineStats allStats;
    for(auto && s : engineStatsMap) {
        allStats.add(s.second);
    }
    
    stringStream
    << std::right << "  #  "
    << std::left << std::setw(maxNameLen + 2) << "name"
    << std::right << std::setw(w) << "nodes"
    << std::right << std::setw(w) << "depths"
    << std::right << std::setw(w) << "time/m"
    << std::right << std::setw(w) << "moves"
    << std::right << std::setw(w) << "time"
    ;
    
    if (abnormalCnt) {
        stringStream << std::right << std::setw(w) << "#fails";
    }
    
    if (profileMode) {
        stringStream
        << std::right << std::setw(pw) << "cpu"
        << std::right << std::setw(pw) << "think"
        << std::right << std::setw(pw) << "mem"
        << std::right << std::setw(pw) << "max"
        << std::right << std::setw(pw + 1) << "threads"
        << std::right << std::setw(pw - 1) << "max"
        ;
    }
    
    stringStream << std::endl;
    
    for(int i = 0; i < resultList.size(); i++) {
        auto r = resultList.at(i);
        auto stats = engineStatsMap[r.name];
        
        auto games = std::max<i64>(1, stats.games);
        auto moves = std::max<i64>(1, stats.moves);
        auto nodeStr = std::to_string(int(stats.nodes / (moves * 1024))) + "K";
        
        stringStream
        << std::right << std::setw(3) << (i + 1) << ". "
        << std::left << std::setw(maxNameLen + 2) << r.name
        << std::right << std::setw(w) << nodeStr
        << std::right << std::setw(w) << double(stats.depths) / moves
        << std::right << std::setw(w) << stats.elapsed / double(std::max<i64>(1, stats.moves))
        
        << std::right << std::setw(w) << double(stats.moves) / games
        << std::right << std::setw(w) << stats.elapsed / games
        ;
        
        if (abnormalCnt) {
            stringStream << std::right << std::setw(w);
            if (r.abnormalCnt)
                stringStream << r.abnormalCnt;
            else
                stringStream << "";
        }
        
        if (profileMode) {
            auto it = profileMap.find(r.name);
            if (it != profileMap.end()) {
                stringStream << std::left << std::setw(0) << it->second.toString(true);
            }
        }
        stringStream << std::endl;
    }
    
    auto games = std::max<i64>(1, allStats.games);
    auto moves = std::max<i64>(1, allStats.moves);
    auto nodeStr = std::to_string(int(allStats.nodes / (moves * 1024))) + "K";
    
    stringStream
    << "     "
    << std::left << std::setw(maxNameLen + 2) << "all ---"
    << std::right << std::setw(w) << nodeStr
    << std::right << std::setw(w) << double(allStats.depths) / moves
    << std::right << std::setw(w) << double(allStats.moves) / games
    << std::right << std::setw(w) << allStats.elapsed / games
    << std::right << std::setw(w) << allStats.elapsed / double(std::max<i64>(1, allStats.moves));
    
    if (abnormalCnt) {
        stringStream << std::right << std::setw(w) << abnormalCnt;
    }
    
    if (profileMode) {
        Profile profile;
        for (auto && p : profileMap) {
            profile.addFrom(p.second);
        }
        
        stringStream << std::left << std::setw(0) << profile.toString(true);
    }
    
    stringStream << std::endl;
    
    if (abnormalCnt) {
        stringStream << "Failed games (timeout, crashed, illegal moves): " << abnormalCnt << " of " << matchRecordList.size();
    }
    
    return stringStream.str();
}


