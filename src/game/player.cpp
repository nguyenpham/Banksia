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


#include <sstream>

#include "player.h"

using namespace banksia;

Player::Player()
: type(PlayerType::none)
{
    state = PlayerState::none;
}

Player::Player(const std::string& name, PlayerType type)
: idNumber(std::rand()), name(name), type(type)
{
}

bool Player::isValid() const
{
    return !name.empty() && (type == PlayerType::human || type == PlayerType::engine) && board != nullptr && timeController != nullptr;
}

std::string Player::toString() const
{
    std::ostringstream stringStream;
    stringStream << name << ", idNumber: " << idNumber << std::endl;
    return stringStream.str();
}

std::string Player::getName() const
{
    return name;
}

void Player::setState(PlayerState st)
{
    state = st;
    tick_state = 0;
}

void Player::attach(ChessBoard* _board, const GameTimeController* _timeController,
                    std::function<void(const Move&, const std::string&, const Move&, double, EngineComputingState)> _moveReceiver,
                    std::function<void()> _resignFunc)
{
    assert(isSafeToDeattach());
    board = _board;
    timeController = _timeController;
    moveReceiver = _moveReceiver;
    resignFunc = _resignFunc;
}

void Player::deattach()
{
    attach(nullptr, nullptr, nullptr, nullptr);
}

bool Player::isAttached() const
{
    return board != nullptr && timeController != nullptr && moveReceiver != nullptr;
}

bool Player::goPonder(const Move& pondermove)
{
    return go();
}

bool Player::go()
{
    setState(PlayerState::playing);
    score = depth = 0;
    return true;
}

bool Player::oppositeMadeMove(const Move&, const std::string&)
{
    return false;
}

