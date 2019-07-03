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

#include "playermng.h"
#include "wbengine.h"

using namespace banksia;

PlayerMng* PlayerMng::instance = nullptr;

PlayerMng::PlayerMng()
{
    assert(!instance);
    instance = this;
}

PlayerMng::~PlayerMng()
{
}

void PlayerMng::tickWork()
{
    std::vector<Player*> removingList;
    
    for(auto && player : playerList) {
        if (player->getState() == PlayerState::stopped) {
            if (!player->isAttached()) {
                removingList.push_back(player);
            }
        } else {
            player->tick();
        }
    }
    
    for(auto && player : removingList) {
        auto it = std::find(playerList.begin(), playerList.end(), player);
        if (it != playerList.end()) {
            playerList.erase(it);
        }
        killPlayer(player);
    }
}

bool PlayerMng::isValid() const
{
    for(auto && player : playerList) {
        if (!player->isValid()) {
            return false;
        }
    }
    
    return true;
}

std::string PlayerMng::toString() const
{
    std::ostringstream stringStream;
    for(auto && player : playerList) {
        stringStream << player->toString() << std::endl;
    }
    return stringStream.str();
}

bool PlayerMng::add(const Config& config)
{
    if (config.isValid()) {
        configMng.insert(config);
        return true;
    }
    return false;
}

bool PlayerMng::add(Player* player)
{
    if (player) {
        std::lock_guard<std::mutex> dolock(thelock);
        playerList.push_back(player);
        return true;
    }
    return false;
}

bool PlayerMng::returnPlayer(Player* player)
{
    if (player == nullptr) return false;
    
    if (player->getState() < PlayerState::stopping) {
        player->quit();
        return true;
    }
    
    if (player->getState() == PlayerState::stopping) {
        return true;
    }
    
    return killPlayer(player);
}


bool PlayerMng::killPlayer(Player* player)
{
    if (player == nullptr) return false;
    
    auto it = std::find(playerList.begin(), playerList.end(), player);
    
    if (it != playerList.end()) {
        std::lock_guard<std::mutex> dolock(thelock);
        playerList.erase(it);
    }
    
    delete player;
    return true;
}

Player* PlayerMng::createPlayer(const std::string& name)
{
    auto config = configMng.get(name);
    if (config.isValid()) {
        return createEngine(config);
    }
    
    auto s = name.empty() ? "Human" : name;
    auto player = new Human(name);
    add(player);
    return player;
}

Engine* PlayerMng::createEngine(const std::string& name)
{
    auto config = configMng.get(name);
    return createEngine(config);
}

Engine* PlayerMng::createEngine(const Config& config)
{
    if (!config.isValid()) {
        return nullptr;
    }
    
    Engine* ePlayer = nullptr;
    
    switch (config.protocol) {
        case Protocol::uci:
            ePlayer = new UciEngine(config);
            break;
            
        case Protocol::wb:
//            ePlayer = new WbEngine(config);
            break;

        default:
            break;
    }
    
    if (ePlayer)
        add(ePlayer);
    return ePlayer;
}

