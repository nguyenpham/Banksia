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

void Book::loadEpdFile(const std::string& epdPath)
{
    epdVec = readTextFileToArray(epdPath);
}

std::string Book::getRandomFEN() const
{
    if (!epdVec.empty()) {
        for(int atemp = 0; atemp < 5; atemp++) {
            auto k = std::rand() % epdVec.size();
            auto str = epdVec.at(k);
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
    return epdVec.empty();
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
    
    switch (type) {
        case BookType::edp:
        {
            Book book;
            book.type = type;
            book.loadEpdFile(path);
            if (!book.isEmpty()) {
                bookList.push_back(book);
            }
            return true;
        }
            
        default:
            std::cerr << "Error: not support yet book type " << typeStr << std::endl;
            break;
    }
    
    return false;
}

Json::Value BookMng::saveToJson() const
{
    Json::Value obj;
    return obj;
}

std::string BookMng::getRandomFEN() const
{
    return !mode || bookList.empty() ? "" : bookList.front().getRandomFEN();
}
