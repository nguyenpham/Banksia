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

#include "base.h"

using namespace banksia;

Move Move::illegalMove(-1, -1);
MoveFull MoveFull::illegalMove(-1, -1);

u64 BoardCore::hashForSide;
std::vector<u64> BoardCore::hashTable;

BoardCore::BoardCore() {
}

bool BoardCore::fromOriginPosition() const
{
    return startFen.empty();
}

std::string BoardCore::getStartingFen() const
{
    return startFen;
}

MoveFull BoardCore::createFullMove(int from, int dest, PieceType promotion) const
{
    MoveFull move(from, dest, promotion);
    if (isPositionValid(from)) {
        move.piece = getPiece(from);
    }
    return move;
}

PieceType BoardCore::charactorToPieceType(char ch)
{
    if (ch >= 'A' && ch < 'Z') {
        ch += 'a' - 'A';
    }
    auto pieceType = PieceType::empty;
    const char* p = strchr(pieceTypeName, ch);
    int k = int(p - pieceTypeName);
    pieceType = static_cast<PieceType>(k);
    return pieceType;
}

Move BoardCore::moveFromCoordiateString(const std::string& moveString)
{
    const char* str = moveString.c_str();
    int from = coordinateStringToPos(str);
    int dest = coordinateStringToPos(str + 2);
    
    auto promotion = PieceType::empty;
    if (moveString.length() > 4) {
        char ch = moveString.at(4);
        if (ch == '=' && moveString.length() > 5) ch = moveString.at(5);
        if (ch>='A' && ch <='Z') {
            ch += 'a' - 'A';
        }
        auto t = charactorToPieceType(ch);
        if (isValidPromotion(t)) promotion = t;
    }
    
    return Move(from, dest, promotion);
}

bool BoardCore::isValidPromotion(PieceType promotion)
{
    return promotion == PieceType::empty || (promotion > PieceType::king && promotion < PieceType::pawn);
}

void BoardCore::newGame(std::string fen)
{
    setFen(fen);
}

u64 BoardCore::initHashKey() const
{
    u64 key = 0;
    for(int i = 0; i < pieces.size(); i++) {
        if (!pieces[i].isEmpty()) {
            key ^= xorHashKey(i);
        }
    }
    
    if (side == Side::black) {
        key ^= hashForSide;
    }
    return key;
}

u64 BoardCore::xorHashKey(int pos) const
{
    assert(isPositionValid(pos ));
    
    assert(!pieces[pos].isEmpty());
    int sd = static_cast<int>(pieces[pos].side);
    int p = static_cast<int>(pieces[pos].type);
    
    auto sz = int(pieces.size()); assert(sz == 64);
    int h = sd * 7 * sz + p * sz + pos; assert(h < hashTable.size());
    return hashTable[h];
}

bool BoardCore::istHashKeyValid() const
{
    return hashKey == initHashKey();
}


