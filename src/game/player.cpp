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


#include <sstream>

#include "player.h"

using namespace banksia;

static const char* playerTypeNames[] = {
    "human", "engine", "none", nullptr
};

Player::Player()
: type(PlayerType::none)
{
    state = PlayerState::none;
}

Player::Player(const std::string& name, PlayerType type)
: name(name), type(type), idNumber(std::rand())
{
}

bool Player::isValid() const
{
    return !name.empty() && (type == PlayerType::human || type == PlayerType::engine) && board != nullptr && timeController != nullptr;
}

std::string Player::toString() const
{
    std::ostringstream stringStream;
    stringStream << name << ", " << playerTypeNames[static_cast<int>(type)] << ", idNumber: " << idNumber << std::endl;
    return stringStream.str();
}

void Player::setup(const ChessBoard* _board, const GameTimeController* _timeController)
{
    board = _board;
    timeController = _timeController;
}

bool Player::isAttachedToGame() const
{
    return board != nullptr && timeController != nullptr;
}

void Player::setMoveReceiver(void* parent, std::function<void(const std::string&, const std::string&, double, EngineComputingState)> theFunc)
{
    assert(parent);
    moveReceiver = theFunc;
}

bool Player::goPonder(const Move& pondermove)
{
    return go();
}

bool Player::go()
{
    setState(PlayerState::playing);
    return true;
}

/////////////////////////////////////

bool Human::kickStart()
{
    setState(PlayerState::ready);
    return true;
}

bool Human::stopThinking()
{
    return true;
}

bool Human::quit()
{
    setState(PlayerState::stopped);
    return true;
}

bool Human::kill()
{
    setState(PlayerState::stopped);
    return true;
}


