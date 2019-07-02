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

#include "book.h"

using namespace banksia;

Book::Book()
{
}

Book::~Book()
{
}

bool Book::isValid() const
{
    return true;
}

std::string Book::toString() const
{
    std::ostringstream stringStream;
    return stringStream.str();
}

bool Book::addPgnMoves(const std::string& str)
{
    if (!str.empty()) {
        ChessBoard board;
        board.newGame();
        if (board.fromSanMoveList(str) && !board.histList.empty()) {
            std::vector<MoveCore> list;
            for(auto hist : board.histList) {
                MoveCore m = hist.move;
                list.push_back(m);
            }
            
            if (!list.empty()) {
                moves.push_back(list);
            }

            return true;
        }
    }
    return false;
}

void Book::load(const std::string& _path)
{
    path = _path;
    if (type == BookType::pgn) {
        loadPgnBook(path);
    } else {
        loadEdpBook(path);
    }
}

void Book::loadEdpBook(const std::string& path)
{
    stringVec = readTextFileToArray(path);
}

void Book::loadPgnBook(const std::string& path)
{
    moves.clear();
    auto vec = readTextFileToArray(path);
    
    std::string moveText;
    for(auto && s : vec) {
        if (s.find("[") != std::string::npos) {
            if (s.find("[Event") != std::string::npos) {
                addPgnMoves(moveText);
                moveText = "";
                continue;
            }
            continue;
        }
        moveText += " " + s;
    }
    addPgnMoves(moveText);
}

std::string Book::getRandomFEN() const
{
    if (!stringVec.empty()) {
        for(int atemp = 0; atemp < 5; atemp++) {
            auto k = std::rand() % stringVec.size();
            auto str = stringVec.at(k);
            if (!str.empty()) {
                ChessBoard board;
                board.setFen(str);
                if (board.isValid()) {
                    return board.getFen();
                } else {
                    std::cerr << "Warning: epd position is invalid: " << str << std::endl;
                }
            }
        }
    }
    
    return "";
}

bool Book::isEmpty() const
{
    return stringVec.empty() && moves.empty();
}


bool Book::setupOpening(ChessBoard& board, int randomIdx) const
{
    if (type == BookType::pgn) {
        if (moves.empty()) {
            return false;
        }
        
        auto k = randomIdx % moves.size();
        auto vec = moves.at(k);
        if (vec.empty()) {
            return false;
        }
        
        for(auto && m : vec) {
            board.checkMake(m.from, m.dest, m.promotion);
        }
        return board.isValid();
    }
    
    if (stringVec.empty()) {
        return false;
    }
    
    auto k = randomIdx % stringVec.size();
    auto fen = stringVec.at(k);
    if (!fen.empty()) {
        board.newGame(fen);
        return board.isValid();
    }
    return false;
}

bool Book::getRandomBook(std::string& fenString, std::vector<MoveCore>& moveList) const
{
    if (type == BookType::pgn) {
        if (moves.empty()) {
            return false;
        }
        auto k = std::rand() % moves.size();
        moveList = moves.at(k);
        return !moveList.empty();
    }
    
    fenString = getRandomFEN();
    return !fenString.empty();
}

static const char* bookTypeNames[] = {
    "epd", "pgn", "polyglot", nullptr
};

BookType BookMng::string2BookType(const std::string& name)
{
    for(int i = 0; bookTypeNames[i]; i++) {
        if (name == bookTypeNames[i]) {
            return static_cast<BookType>(i);
        }
    }
    return BookType::none;
}

BookMng* BookMng::instance = nullptr;

BookMng::BookMng()
{
    instance = this;
}

BookMng::~BookMng()
{
}

bool BookMng::isEmpty() const
{
    if (mode && !bookList.empty()) {
        for(auto && book : bookList) {
            if (!book.isEmpty()) {
                return false;
            }
        }
    }
    return true;
}

bool BookMng::isValid() const
{
    return true;
}

std::string BookMng::toString() const
{
    std::ostringstream stringStream;
    return stringStream.str();
}

bool BookMng::load(const Json::Value& obj)
{
    if (!obj.isMember("type") || !obj.isMember("path")
        ) {
        return false;
    }
    
    mode = obj.isMember("mode") && obj["mode"].asBool();
    
    if (!mode) {
        return false;
    }
    
    auto path = obj["path"].asString();
    auto typeStr = obj["type"].asString();
    auto type = string2BookType(typeStr);
    
    if (type == BookType::none) {
        std::cerr << "Error: does not support yet book type " << typeStr << std::endl;
    } else {
        Book book;
        book.type = type;
        book.load(path);
        if (!book.isEmpty()) {
            bookList.push_back(book);
        }
        return true;
    }
    
    return false;
}

Json::Value BookMng::saveToJson() const
{
    Json::Value obj;
    return obj;
}

bool BookMng::getRandomBook(std::string& fenString, std::vector<MoveCore>& moves) const
{
    return mode && !bookList.empty() && bookList.front().getRandomBook(fenString, moves);
}
