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
#include <fstream>

#include "book.h"

using namespace banksia;

void BookEdp::load(const std::string& _path, int _maxPly, int _top100)
{
    path = _path; maxPly = _maxPly; top100 = _top100;
    stringVec = readTextFileToArray(path);
}

std::string BookEdp::getRandomFEN() const
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

bool BookEdp::isEmpty() const
{
    return stringVec.empty();
}

size_t BookEdp::size() const
{
    return stringVec.size();
}

bool BookEdp::getRandomBook(std::string& fenString, std::vector<Move>& moveList) const
{
    fenString = getRandomFEN();
    return !fenString.empty();
}

/////////////////////////////////
bool BookPgn::isEmpty() const
{
    return moves.empty();
}

size_t BookPgn::size() const
{
    return moves.size();
}

bool BookPgn::addPgnMoves(const std::string& str)
{
    if (!str.empty()) {
        ChessBoard board;
        board.newGame();
        if (board.fromSanMoveList(str) && !board.histList.empty()) {
            std::vector<Move> list;
            for(auto hist : board.histList) {
                Move m = hist.move;
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

void BookPgn::load(const std::string& _path, int _maxPly, int _top100)
{
    path = _path; maxPly = _maxPly; top100 = _top100;

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


bool BookPgn::getRandomBook(std::string& fenString, std::vector<Move>& moveList) const
{
    if (moves.empty()) {
        return false;
    }
    auto k = std::rand() % moves.size();
    moveList = moves.at(k);
    return !moveList.empty();
}

///////////////////////////////////
Move BookPolyglotItem::getMove() const
{
    auto f = move & 0x7, r = move >> 3 & 0x7;
    auto dest = (7 - r) * 8 + f;

    f = move >> 6 & 0x7; r = move >> 9 & 0x7;
    auto from = (7 - r) * 8 + f;

    auto p = move >> 12 & 0x3;
    auto promotion = p == 0 ? PieceType::empty : static_cast<PieceType>(6 - p);
    return Move(from, dest, promotion);
}

void BookPolyglotItem::convertToLittleEndian()
{
    key = (key >> 56) |
    ((key<<40) & 0x00FF000000000000UL) |
    ((key<<24) & 0x0000FF0000000000UL) |
    ((key<<8) & 0x000000FF00000000UL) |
    ((key>>8) & 0x00000000FF000000UL) |
    ((key>>24) & 0x0000000000FF0000UL) |
    ((key>>40) & 0x000000000000FF00UL) |
    (key << 56);

    move = (move >> 8) | (move << 8);
    weight = (weight >> 8) | (weight << 8);

    learn = (learn >> 24) | ((learn<<8) & 0x00FF0000) | ((learn>>8) & 0x0000FF00) | (learn << 24);
}

std::string BookPolyglotItem::toString() const
{
    std::ostringstream stringStream;
    stringStream << key << ", " << getMove().toString() << ", " << move << ", " << weight << ", " << learn;
    return stringStream.str();
}

BookPolyglot::~BookPolyglot()
{
    if (items) {
        delete items;
        items = nullptr;
    }
}

bool BookPolyglot::isEmpty() const
{
    return items == nullptr || itemCnt == 0;
}

size_t BookPolyglot::size() const
{
    return itemCnt;
}

void BookPolyglot::load(const std::string& _path, int _maxPly, int _top100)
{
    path = _path; maxPly = _maxPly; top100 = _top100;
    
    std::ifstream ifs(path, std::ios::binary| std::ios::ate);
    
    i64 length = 0;
    if (ifs.is_open()) {
        length = std::max((i64)0, (i64)ifs.tellg());
    }

    assert(sizeof(BookPolyglotItem) == 16);
    itemCnt = length / sizeof(BookPolyglotItem);

    if (itemCnt == 0) {
        std::cerr << "Error: cannot load book " << path << std::endl;
        return;
    }
    
    items = (BookPolyglotItem *) malloc(itemCnt * sizeof(BookPolyglotItem) + 64);
    ifs.seekg(0, std::ios::beg);
    ifs.read((char*)items, length);

    for(int i = 0; i < itemCnt; i++) {
        items[i].convertToLittleEndian();
    }

    std::cout << "BookPolyglot::load " << path << ", itemCnt: " << itemCnt << std::endl;
    
}

bool BookPolyglot::isValid() const
{
    if (items == nullptr) {
        return false;
    }
    
    u64 preKey = 0;
    for(int i = 0; i < itemCnt; i++) {
        if (preKey > items[i].key) {
            return false;
        }
        preKey = items[i].key;
    }
    
    return true;
}

i64 BookPolyglot::binarySearch(u64 key) const
{
    i64 first = 0, last = itemCnt - 1;
    
    while (first <= last) {
        auto middle = (first + last) / 2;
        if (items[middle].key == key) {
            return middle;
        }
        
        if (items[middle].key > key)  {
            last = middle - 1;
        }
        else {
            first = middle + 1;
        }
    }
    return -1;
}

std::vector<BookPolyglotItem> BookPolyglot::search(u64 key) const
{
    std::vector<BookPolyglotItem> vec;
    
    auto k = binarySearch(key);
    if (k >= 0) {
        for(; k > 0 && items[k - 1].key == key; k--) {}

        for(; k < i64(itemCnt) && items[k].key == key; k++) {
            vec.push_back(items[k]);
        }
    }
    
    return vec;
}

bool BookPolyglot::getRandomBook(std::string& fenString, std::vector<Move>& moveList) const
{
    ChessBoard board;
    board.newGame();

    while (moveList.size() < maxPly) {
        auto vec = search(board.key());
        if (vec.empty()) break;
        
        auto k = int(vec.size()) * top100 / 100;
        assert(k >= 0 && k <= vec.size());
        auto idx = k == 0 ? 0 : (std::rand() % k);
        
        auto move = vec[idx].getMove();
        if (!board.checkMake(move.from, move.dest, move.promotion)) break;
        moveList.push_back(move);
    }

    board.printOut(("last position of an opening, len: " + std::to_string(board.histList.size())).c_str());
    return true;
}

/////////////////////////////////////////
static const char* bookTypeNames[] = {
    "epd", "pgn", "polyglot", "none", nullptr
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

std::string BookMng::bookType2String(BookType type)
{
    auto t = static_cast<int>(type);
    if (t < 0 || t >= 3) t = 3;
    return bookTypeNames[t];
}

BookMng* BookMng::instance = nullptr;

BookMng::BookMng()
{
    instance = this;
}

BookMng::~BookMng()
{
    for(auto && book : bookList) {
        delete book;
    }
    
    bookList.clear();
}

bool BookMng::isEmpty() const
{
    if (!bookList.empty()) {
        for(auto && book : bookList) {
            if (!book->isEmpty()) {
                return false;
            }
        }
    }
    return true;
}

size_t BookMng::size() const
{
    size_t sz = 0;
    for(auto && book : bookList) {
        sz += book->size();
    }
    return sz;
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
    auto r = false;
    
    if (obj.isArray()) {
        for (Json::Value::const_iterator it = obj.begin(); it != obj.end(); ++it) {
            r = loadSingle(*it) || r;
        }
    } else {
        r = loadSingle(obj);
    }
    
	std::cout << "opening books loaded, total items: " << size() << std::endl;
	return r;
}

bool BookMng::loadSingle(const Json::Value& obj)
{
    if (!obj.isMember("type") || !obj.isMember("path")
        ) {
        return false;
    }
    
    auto mode = obj.isMember("mode") && obj["mode"].asBool();
    
    if (!mode) {
        return false;
    }
    
    auto path = obj["path"].asString();
    auto typeStr = obj["type"].asString();
    auto type = string2BookType(typeStr);
    
    auto maxPly = obj.isMember("maxply") ? obj["maxply"].asInt() : PologlotDefaultMaxPly;
    auto top100 = obj.isMember("top100") ? obj["top100"].asInt() : 0;
    
    if (type == BookType::none) {
        std::cerr << "Error: does not support yet book type " << typeStr << std::endl;
    } else {
        Book* book;
        switch (type) {
            case BookType::edp:
                book = new BookEdp;
                break;
            case BookType::pgn:
                book = new BookPgn;
                break;

            case BookType::polygot:
                book = new BookPolyglot;
                break;
                
            default:
                return false;
        }
        book->load(path, maxPly, top100);
        if (book->isEmpty()) {
            delete book;
        } else {
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

bool BookMng::getRandomBook(std::string& fenString, std::vector<Move>& moves) const
{
    return !bookList.empty() && bookList.front()->getRandomBook(fenString, moves);
}
