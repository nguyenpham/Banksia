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


#include <iostream>
#include <iomanip>

#include "game.h"
#include "engine.h"

using namespace banksia;

Game::Game()
{
    state = GameState::begin;
    players[0] = players[1] = nullptr;
}

Game::Game(Player* player0, Player* player1, const TimeController& timeController, bool ponderMode)
{
    state = GameState::begin;
    players[0] = players[1] = nullptr;
    set(player0, player1, timeController, ponderMode);
}

Game::~Game()
{
}

bool Game::isValid() const
{
    return players[0] && players[0]->isValid() && players[1] && players[1]->isValid();
}

std::string Game::toString() const
{
    return "";
}

void Game::setState(GameState st)
{
    if (state != st) {
        stateTick = 0;
    }
    state = st;
}

void Game::setStartup(int _idx, const std::string& _startFen, const std::vector<MoveCore>& _startMoves)
{
    idx = _idx;
    startFen = _startFen;
    startMoves = _startMoves;
}

int Game::getIdx() const
{
    return idx;
}

void Game::set(Player* player0, Player* player1, const TimeController& _timeController, bool _ponderMode)
{
    attachPlayer(player0, Side::white);
    attachPlayer(player1, Side::black);
    timeController.cloneFrom(_timeController);
    ponderMode = _ponderMode;
}

void Game::setMessageLogger(std::function<void(const std::string&, const std::string&, LogType)> logger)
{
    messageLogger = logger;
    
    for(int sd = 0; sd < 2; sd++) {
        if (players[sd] && !players[sd]->isHuman()) {
            ((Engine*)players[sd])->setMessageLogger(logger);
        }
    }
}

void Game::attachPlayer(Player* player, Side side)
{
    if (player == nullptr || (side != Side::white && side != Side::black)) return;
    auto sd = static_cast<int>(side);
    
    players[sd] = player;
    
    player->attach(&board, &timeController,
                   [=](const std::string& moveString, const std::string& ponderMoveString, double timeConsumed, EngineComputingState state) {
        moveFromPlayer(moveString, ponderMoveString, timeConsumed, side, state);
    });
}

Player* Game::deattachPlayer(Side side)
{
    auto sd = static_cast<int>(side);
    auto player = players[sd];
    players[sd] = nullptr;
    if (player) {
        player->deattach();
    }
    
    return player;
}

void Game::newGame()
{
    // Include opening
    board.newGame(startFen);
    
    timeController.setupClocksBeforeThinking(0);
    assert(timeController.isValid());
    
    // opening
    if (!startMoves.empty()) {
        for(auto && m : startMoves) {
            if (!board.checkMake(m.from, m.dest, m.promotion)) {
                break;
            }
        }
        board.histList.back().comment = "End of opening";
    }
    
    for(int i = 0; i < 2; i++) {
        if (players[i]) {
            players[i]->newGame();
        }
    }
    
    setState(GameState::begin);
}

void Game::startPlaying()
{
    assert(state == GameState::ready);
    
    newGame();
    
    setState(GameState::playing);
    
    startThinking();
}

void Game::startThinking(Move pondermove)
{
    assert(board.isValid());
    
    timeController.setupClocksBeforeThinking(int(board.histList.size()));
    
    auto sd = static_cast<int>(board.side);
    
    players[1 - sd]->goPonder(pondermove);
    players[sd]->go();
}

void Game::start()
{
    for(int sd = 0; sd < 2; sd++) {
        players[sd]->kickStart();
    }
}

void Game::pause()
{
}

void Game::stop()
{
}

void Game::moveFromPlayer(const std::string& moveString, const std::string& ponderMoveString, double timeConsumed, Side side, EngineComputingState oldState)
{
    // avoid conflicting between this function and one called by tickWork
    std::lock_guard<std::mutex> dolock(criticalMutex);
    
    if (state != GameState::playing || checkTimeOver() || board.side != side) {
        return;
    }
    
    auto move = ChessBoard::moveFromCoordiateString(moveString);
    Move pondermove = ponderMode ? ChessBoard::moveFromCoordiateString(ponderMoveString) : Move::illegalMove;
    
    assert(board.side == side);
    if (oldState == EngineComputingState::thinking) {
        if (make(move)) {
            assert(board.side != side);
            
            auto lastHist = board.histList.back();
            lastHist.elapsed = timeConsumed;
            timeController.udateClockAfterMove(timeConsumed, lastHist.move.piece.side, int(board.histList.size()));
            
            startThinking(pondermove);
        }
    } else if (oldState == EngineComputingState::pondering) { // missed ponderhit, stop called
        auto sd = static_cast<int>(board.side);
        players[sd]->go();
    }
}

bool Game::make(const Move& move)
{
    if (board.checkMake(move.from, move.dest, move.promotion)) {
        auto result = board.rule();
        if (result.result != ResultType::noresult) {
            gameOver(result);
            return false;
        }
        
        assert(board.isValid());
        return true;
    } else {
        Result result(board.side == Side::white ? ResultType::loss : ResultType::win, ReasonType::illegalmove);
        gameOver(result);
        return false;
    }
    return false;
}

void Game::gameOver(const Result& result)
{
    for(int sd = 0; sd < 2; sd++) {
        if (players[sd]) {
            players[sd]->stopThinking();
        }
    }
    
    board.result = result;
    setState(GameState::stopped);
}

Player* Game::getPlayer(Side side)
{
    auto sd = static_cast<int>(side);
    return players[sd];
}

std::string Game::getGameTitleString() const
{
    std::string str;
    str += players[W] ? players[W]->getName() : "*";
    str += " vs " + (players[B] ? players[B]->getName() : "*");
    return str;
}

bool Game::checkTimeOver()
{
    if (timeController.isTimeOver(board.side)) {
        // Report more detail
        if (messageLogger) {
            std::ostringstream stringStream;
            stringStream
            << std::fixed << std::setprecision(2)
            << "Timeleft for ";
            
            for(int sd = 1; sd >= 0; sd--) {
                stringStream << players[sd]->getName() << ": " << timeController.getTimeLeft(sd);
                if (static_cast<int>(board.side) == sd) {
                    stringStream << ", used: " << timeController.lastQueryConsumed;
                }
                if (sd == 1) {
                    stringStream << ", ";
                }
            }

            auto str = stringStream.str();
            // printText("\t" + str);
            if (messageLogger) {
                (messageLogger)(getAppName(), str, LogType::system);
            }
        }
        
        Result result;
        result.result = board.side == Side::white ? ResultType::loss : ResultType::win;
        result.reason = ReasonType::timeout;
        gameOver(result);
        return true;
    }
    return false;
}

void Game::tickWork()
{
    stateTick++;
    
    switch (state) {
        case GameState::begin:
        {
            auto readyCnt = 0, stoppedCnt = 0;
            for(int sd = 0; sd < 2; sd++) {
                if (!players[sd]) {
                    continue;
                }
                auto st = players[sd]->getState();
                if (st == PlayerState::ready) {
                    readyCnt++;
                } else if (st == PlayerState::stopped) {
                    stoppedCnt++;
                }
            }
            
            if (readyCnt + stoppedCnt < 2) {
                break;
            }
            
            if (readyCnt == 2) {
                setState(GameState::ready);
            } else {
                Result result;
                result.reason = ReasonType::crash;
                setState(GameState::stopped);
                if (stoppedCnt == 2) { // both crash
                    result.result = ResultType::draw;
                } else {
                    result.result =  players[W]->getState() == PlayerState::stopped ? ResultType::loss : ResultType::win;
                }
                
                gameOver(result);
            }
            break;
        }
            
        case GameState::ready:
            startPlaying();
            break;
            
        case GameState::playing:
        {
            // check time over
            auto sd = static_cast<int>(board.side);
            if (!players[sd] || players[sd]->isHuman()) {
                break;
            }

            std::lock_guard<std::mutex> dolock(criticalMutex); // avoid conflicting with moveFromPlayer
            if (state == GameState::playing) {
                checkTimeOver();
            }
            break;
        }
        default:
            break;
    }
}


std::string Game::toPgn(std::string event, std::string site, int round) const
{
    std::ostringstream stringStream;
    
    if (!event.empty()) {
        stringStream << "[Event \t\"" << event << "\"]" << std::endl;
    }
    if (!site.empty()) {
        stringStream << "[Site \t\"" << site << "\"]" << std::endl;
    }
    
    auto tm = localtime_xp(std::time(0));
    
    stringStream << "[Date \t\"" << std::put_time(&tm, "%Y.%m.%d") << "\"]" << std::endl;
    
    if (round >= 0) {
        stringStream << "[Round \t\"" << round << "\"]" << std::endl;
    }
    
    for(int sd = 1; sd >= 0; sd--) {
        if (players[sd]) {
            stringStream << "[" << (sd == W ? "White" : "Black") << " \t\"" << players[sd]->getName() << "\"]" << std::endl;
        }
    }
    stringStream << "[Result \t\"" << board.result.toShortString() << "\"]" << std::endl;
    
    stringStream << "[TimeControl \t\"" << timeController.toString() << "\"]" << std::endl;
    stringStream << "[Time \t\"" << std::put_time(&tm, "%H:%M:%S") << "\"]" << std::endl;
    
    auto str = board.result.reasonString();
    if (!str.empty()) {
        stringStream << "[Termination \t\"" << str << "\"]" << std::endl;
    }
    
    if (!board.fromOriginPosition()) {
        stringStream << "[FEN \t\"" << board.getStartingFen() << "\"]" << std::endl;
        stringStream << "[SetUp \t\"1\"]" << std::endl;
    }
    
    // Move text
    stringStream << board.toMoveListString(MoveNotation::san, 8, true);
    
    if (board.result.result != ResultType::noresult) {
        if (board.histList.size() % 8 != 0) stringStream << " ";
        stringStream << board.result.toShortString() << std::endl;
    }
    stringStream << std::endl;
    
    return stringStream.str();
}


