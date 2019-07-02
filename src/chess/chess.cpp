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

#include <random>

#include "chess.h"

namespace banksia {
    extern const char* startingFen;
    const char* originalFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq";
}

using namespace banksia;


ChessBoard::ChessBoard()
{
    Piece empty(PieceType::empty, Side::none);
    for(int i = 0; i < 64; i++) {
        pieces.push_back(empty);
    }
    
    if (hashTable.empty()) {
        std::mt19937_64 gen (std::random_device{}());
        hashForSide = gen();
        
        for(int i = 0; i < 7 * 2 * 64; i++) { // 7 pieces (including empty), 2 sides, 64 cells
            hashTable.push_back(gen());
        }
    }
    
}

ChessBoard::~ChessBoard()
{
}

int ChessBoard::columnCount() const
{
    return 8;
}

int ChessBoard::getColumn(int pos) const
{
    return pos & 7;
}

int ChessBoard::getRow(int pos) const
{
    return pos >> 3;
}


bool ChessBoard::isValid() const {
    int pieceCout[2][7] = { { 0, 0, 0, 0, 0, 0, 0}, { 0, 0, 0, 0, 0, 0, 0} };
    
    for (int i = 0; i < 64; i++) {
        auto piece = getPiece(i);
        if (piece.isEmpty()) {
            continue;
        }
        
        pieceCout[static_cast<int>(piece.side)][static_cast<int>(piece.type)] += 1;
        if (piece.type == PieceType::pawn) {
            if (i < 8 || i >= 56) {
                return false;
            }
        }
    }
    
    if (castleRights[0] + castleRights[1]) {
        if (castleRights[B]) {
            if (!isPiece(4, PieceType::king, Side::black)) {
                return false;
            }
            if (((castleRights[B] & CASTLERIGHT_LONG) && !isPiece(0, PieceType::rook, Side::black)) ||
                ((castleRights[B] & CASTLERIGHT_SHORT) && !isPiece(7, PieceType::rook, Side::black))) {
                return false;
            }
        }
        
        if (castleRights[W]) {
            if (!isPiece(60, PieceType::king, Side::white)) {
                return false;
            }
            if (((castleRights[W] & CASTLERIGHT_LONG) && !isPiece(56, PieceType::rook, Side::white)) ||
                ((castleRights[W] & CASTLERIGHT_SHORT) && !isPiece(63, PieceType::rook, Side::white))) {
                return false;
            }
        }
    }
    
    if (enpassant > 0) {
        auto row = getRow(enpassant);
        if (row != 2 && row != 5) {
            return false;
        }
        auto pawnPos = row == 2 ? (enpassant + 8) : (enpassant - 8);
        if (!isPiece(pawnPos, PieceType::pawn, row == 2 ? Side::black : Side::white)) {
            return false;
        }
    }
    
    bool b =
    pieceCout[0][1] == 1 && pieceCout[1][1] == 1 &&     // king
    pieceCout[0][2] <= 9 && pieceCout[1][2] <= 9 &&     // queen
    pieceCout[0][3] <= 10 && pieceCout[1][3] <= 10 &&     // rook
    pieceCout[0][4] <= 10 && pieceCout[1][4] <= 10 &&     // bishop
    pieceCout[0][5] <= 10 && pieceCout[1][5] <= 10 &&     // knight
    pieceCout[0][6] <= 8 && pieceCout[1][6] <= 8 &&       // pawn
    pieceCout[0][2]+pieceCout[0][3]+pieceCout[0][4]+pieceCout[0][5] + pieceCout[0][6] <= 15 &&
    pieceCout[1][2]+pieceCout[1][3]+pieceCout[1][4]+pieceCout[1][5] + pieceCout[1][6] <= 15;
    
    assert(b);
    return b;
}

std::string ChessBoard::toString() const {
    
    std::ostringstream stringStream;
    
    stringStream << getFen() << std::endl;
    
    for (int i = 0; i<64; i++) {
        auto piece = getPiece(i);
        
        stringStream << Piece::toString(piece.type, piece.side) << " ";
        
        if (i > 0 && getColumn(i) == 7) {
            int row = 8 - getRow(i);
            stringStream << " " << row << "\n";
        }
    }
    
    stringStream << "a b c d e f g h \n"; // << (side == Side::white ? "w turns" : "b turns") << "\n";
    
    return stringStream.str();
}

void ChessBoard::checkEnpassant() {
    if ((enpassant >= 16 && enpassant < 24) || (enpassant >= 40 && enpassant < 48))  {
        int d = 8, xsd = W, r = 3;
        if (enpassant >= 40) {
            d = -8; xsd = B; r = 4;
        }
        
        // TODO: rewrite
        //        for(int i = 8; i < 16; i++) {
        //            auto p = pieceList[xsd][i];
        //            if (p.type == PieceType::pawn && (p.idx == enpassant + d - 1 || p.idx == enpassant + d + 1) && ROW(p.idx) == r) {
        //                return;
        //            }
        //        }
    }
    enpassant = -1;
}

void ChessBoard::setFen(const std::string& fen) {
    reset();
    
    std::string thefen = fen;
    startFen = fen;
    if (fen.empty()) {
        thefen = originalFen;
    } else {
        if (memcmp(fen.c_str(), originalFen, strlen(originalFen)) == 0) {
            startFen = "";
        }
    }
    
    bool last = false;
    side = Side::none;
    enpassant = -1;
    status = 0;
    castleRights[0] = castleRights[1] = 0;
    
    for (int i = 0, pos = 0; i < thefen.length(); i++) {
        char ch = thefen.at(i);
        
        if (ch == ' ') {
            last = true;
            continue;
        }
        
        if (last) {
            // enpassant
            if (ch >= 'a' && ch <= 'h' && i + 1 < thefen.length()) {
                char ch2 = thefen.at(i + 1);
                if (ch2 >= '1' && ch2 <= '8') {
                    enpassant = (7 - (ch2 - '1')) * 8 + (ch - 'a');
                    continue;
                }
            }
            
            switch (ch) {
                case 'w':
                case 'W':
                    side = Side::white;
                    break;
                case 'b':
                case 'B':
                    side = Side::black;
                    break;
                case 'K':
                    castleRights[W] |= CASTLERIGHT_SHORT;
                    break;
                case 'k':
                    castleRights[B] |= CASTLERIGHT_SHORT;
                    break;
                case 'Q':
                    castleRights[W] |= CASTLERIGHT_LONG;
                    break;
                case 'q':
                    castleRights[B] |= CASTLERIGHT_LONG;
                    break;
                    
                default:
                    break;
            }
            
            continue;
        }
        
        if (ch=='/') {
            std::string str = fen.substr();
            continue;
        }
        
        if (ch>='0' && ch <= '8') {
            int num = ch - '0';
            pos += num;
            continue;
        }
        
        Side side = Side::black;
        if (ch >= 'A' && ch < 'Z') {
            side = Side::white;
            ch += 'a' - 'A';
        }
        
//        auto pieceType = PieceType::empty;
//        const char* p = strchr(pieceTypeName, ch);
//        if (p != NULL) {
//            int k = (int)(p - pieceTypeName);
//            pieceType = static_cast<PieceType>(k);
//
//        }
        
        auto pieceType = charToPieceType(ch);

        if (pieceType != PieceType::empty) {
            setPiece(pos, Piece(pieceType, side));
        }
        pos++;
    }
    
    checkEnpassant();
    
    quietCnt = 0;
    hashKey = initHashKey();
}

std::string ChessBoard::getFen(int halfCount, int fullMoveCount) const {
    std::ostringstream stringStream;
    
    int e = 0;
    for (int i=0; i < 64; i++) {
        auto piece = getPiece(i);
        if (piece.isEmpty()) {
            e += 1;
        } else {
            if (e) {
                stringStream << e;
                e = 0;
            }
            stringStream << piece.toString();
        }
        
        if (i % 8 == 7) {
            if (e) {
                stringStream << e;
            }
            if (i < 63) {
                stringStream << "/";
            }
            e = 0;
        }
    }
    
    stringStream << (side == Side::white ? " w " : " b ");
    
    if (castleRights[W] + castleRights[B]) {
        if (castleRights[W] & CASTLERIGHT_SHORT) {
            stringStream << "K";
        }
        if (castleRights[W] & CASTLERIGHT_LONG) {
            stringStream << "Q";
        }
        if (castleRights[B] & CASTLERIGHT_SHORT) {
            stringStream << "k";
        }
        if (castleRights[B] & CASTLERIGHT_LONG) {
            stringStream << "q";
        }
    } else {
        stringStream << "-";
    }
    
    stringStream << " ";
    if (enpassant > 0) {
        stringStream << posToCoordinateString(enpassant);
    } else {
        stringStream << "-";
    }
    
    stringStream << " " << halfCount << " " << fullMoveCount;
    
    return stringStream.str();
}

void ChessBoard::gen_addMove(MoveList& moveList, int from, int dest, bool captureOnly) const
{
    auto toSide = getPiece(dest).side;
    auto movingPiece = getPiece(from);
    auto fromSide = movingPiece.side;
    
    if (fromSide != toSide && (!captureOnly || toSide != Side::none)) {
        moveList.add(Move(movingPiece, from, dest));
    }
}

void ChessBoard::gen_addPawnMove(MoveList& moveList, int from, int dest, bool captureOnly) const
{
    auto toSide = getPiece(dest).side;
    auto movingPiece = getPiece(from);
    auto fromSide = movingPiece.side;
    
    assert(movingPiece.type == PieceType::pawn);
    if (fromSide != toSide && (!captureOnly || toSide != Side::none)) {
        if (dest >= 8 && dest < 56) {
            moveList.add(Move(movingPiece, from, dest));
        } else {
            moveList.add(Move(movingPiece, from, dest, PieceType::queen));
            moveList.add(Move(movingPiece, from, dest, PieceType::rook));
            moveList.add(Move(movingPiece, from, dest, PieceType::bishop));
            moveList.add(Move(movingPiece, from, dest, PieceType::knight));
        }
    }
}

void ChessBoard::clearCastleRights(int rookPos, Side rookSide) {
    switch (rookPos) {
        case 0:
            if (rookSide == Side::black) {
                castleRights[B] &= ~CASTLERIGHT_LONG;
            }
            break;
        case 7:
            if (rookSide == Side::black) {
                castleRights[B] &= ~CASTLERIGHT_SHORT;
            }
            break;
        case 56:
            if (rookSide == Side::white) {
                castleRights[W] &= ~CASTLERIGHT_LONG;
            }
            break;
        case 63:
            if (rookSide == Side::white) {
                castleRights[W] &= ~CASTLERIGHT_SHORT;
            }
            break;
    }
}


int ChessBoard::findKing(Side side) const
{
    for (int pos = 0; pos < pieces.size(); ++pos) {
        if (isPiece(pos, PieceType::king, side)) {
            return pos;
        }
    }
    return -1;
}


void ChessBoard::genLegalOnly(MoveList& moveList, Side attackerSide) {
    gen(moveList, attackerSide);
    
    Hist hist;
    int j = 0;
    for (int i = 0; i < moveList.end; i++) {
        make(moveList.list[i], hist);
        if (!isIncheck(attackerSide)) {
            moveList.list[j] = moveList.list[i];
            j++;
        }
        takeBack(hist);
    }
    moveList.end = j;
}

bool ChessBoard::isIncheck(Side beingAttackedSide) const {
    int kingPos = findKing(beingAttackedSide);
    Side attackerSide = getXSide(beingAttackedSide);
    return beAttacked(kingPos, attackerSide);
}

bool ChessBoard::isLegalMove(int from, int dest, PieceType promotion)
{
    if (!Move::isValid(from, dest)) {
        return false;
    }
    auto piece = getPiece(from);
    if (piece.isEmpty()
        || piece.side == getPiece(dest).side
        || !isValidPromotion(promotion, piece.side)) {
        return false;
    }
    
    MoveList moveList;
    genLegal(moveList, piece.side, from, dest, promotion);
    return moveList.end > 0;
}

void ChessBoard::genLegal(MoveList& moves, Side side, int from, int dest, PieceType promotion)
{
    MoveList moveList;
    gen(moveList, side);
    
    Hist hist;
    
    for (int i = 0; i < moveList.end; i++) {
        auto move = moveList.list[i];
        
        if ((from >= 0 && move.from != from) || (dest >= 0 && move.dest != dest)) {
            continue;
        }
        
        make(move, hist);
        if (!isIncheck(side)) {
            moves.add(move);
        }
        takeBack(hist);
    }
}

////////////////////////////////////////////////////////////////////////

void ChessBoard::gen(MoveList& moves, Side side) const {
    bool captureOnly = false;
    
    for (int pos = 0; pos < 64; ++pos) {
        auto piece = pieces[pos];
        
        if (piece.side != side) {
            continue;
        }
        
        switch (piece.type) {
            case PieceType::king:
            {
                int col = getColumn(pos);
                if (col) { // go left
                    gen_addMove(moves, pos, pos - 1, captureOnly);
                }
                if (col < 7) { // right
                    gen_addMove(moves, pos, pos + 1, captureOnly);
                }
                if (pos > 7) { // up
                    gen_addMove(moves, pos, pos - 8, captureOnly);
                }
                if (pos < 56) { // down
                    gen_addMove(moves, pos, pos + 8, captureOnly);
                }
                
                if (col && pos > 7) { // left up
                    gen_addMove(moves, pos, pos - 9, captureOnly);
                }
                if (col < 7 && pos > 7) { // right up
                    gen_addMove(moves, pos, pos - 7, captureOnly);
                }
                if (col && pos < 56) { // left down
                    gen_addMove(moves, pos, pos + 7, captureOnly);
                }
                if (col < 7 && pos < 56) { // right down
                    gen_addMove(moves, pos, pos + 9, captureOnly);
                }
                
                if (captureOnly) {
                    break;
                }
                if ((pos ==  4 && castleRights[B]) ||
                    (pos == 60 && castleRights[W])) {
                    if (pos == 4) {
                        if ((castleRights[B] & CASTLERIGHT_LONG) &&
                            pieces[1].isEmpty() && pieces[2].isEmpty() &&pieces[3].isEmpty() &&
                            !beAttacked(2, Side::white) && !beAttacked(3, Side::white)) {
                            assert(isPiece(0, PieceType::rook, Side::black));
                            gen_addMove(moves, 4, 2, captureOnly);
                        }
                        if ((castleRights[B] & CASTLERIGHT_SHORT) &&
                            pieces[5].isEmpty() && pieces[6].isEmpty() &&
                            !beAttacked(5, Side::white) && !beAttacked(6, Side::white)) {
                            assert(isPiece(7, PieceType::rook, Side::black));
                            gen_addMove(moves, 4, 6, captureOnly);
                        }
                    } else {
                        if ((castleRights[W] & CASTLERIGHT_LONG) &&
                            pieces[57].isEmpty() && pieces[58].isEmpty() && pieces[59].isEmpty() &&
                            !beAttacked(58, Side::black) && !beAttacked(59, Side::black)) {
                            assert(isPiece(56, PieceType::rook, Side::white));
                            gen_addMove(moves, 60, 58, captureOnly);
                        }
                        if ((castleRights[W] & CASTLERIGHT_SHORT) &&
                            pieces[61].isEmpty() && pieces[62].isEmpty() &&
                            !beAttacked(61, Side::black) && !beAttacked(62, Side::black)) {
                            assert(isPiece(63, PieceType::rook, Side::white));
                            gen_addMove(moves, 60, 62, captureOnly);
                        }
                    }
                }
                break;
            }
                
            case PieceType::queen:
            case PieceType::bishop:
            {
                for (int y = pos - 9; y >= 0 && getColumn(y) != 7; y -= 9) {        /* go left up */
                    gen_addMove(moves, pos, y, captureOnly);
                    if (!isEmpty(y)) {
                        break;
                    }
                }
                for (int y = pos - 7; y >= 0 && getColumn(y) != 0; y -= 7) {        /* go right up */
                    gen_addMove(moves, pos, y, captureOnly);
                    if (!isEmpty(y)) {
                        break;
                    }
                }
                for (int y = pos + 9; y < 64 && getColumn(y) != 0; y += 9) {        /* go right down */
                    gen_addMove(moves, pos, y, captureOnly);
                    if (!isEmpty(y)) {
                        break;
                    }
                }
                for (int y = pos + 7; y < 64 && getColumn(y) != 7; y += 7) {        /* go right down */
                    gen_addMove(moves, pos, y, captureOnly);
                    if (!isEmpty(y)) {
                        break;
                    }
                }
                /*
                 * if the piece is a bishop, stop heere, otherwise queen will be continued as a rook
                 */
                if (piece.type == PieceType::bishop) {
                    break;
                }
                // Queen continues
            }
                
            case PieceType::rook: // both queen and rook here
            {
                int col = getColumn(pos);
                for (int y=pos - 1; y >= pos - col; y--) { /* go left */
                    gen_addMove(moves, pos, y, captureOnly);
                    if (!isEmpty(y)) {
                        break;
                    }
                }
                
                for (int y=pos + 1; y < pos - col + 8; y++) { /* go right */
                    gen_addMove(moves, pos, y, captureOnly);
                    if (!isEmpty(y)) {
                        break;
                    }
                }
                
                for (int y=pos - 8; y >= 0; y -= 8) { /* go up */
                    gen_addMove(moves, pos, y, captureOnly);
                    if (!isEmpty(y)) {
                        break;
                    }
                }
                
                for (int y=pos + 8; y < 64; y += 8) { /* go down */
                    gen_addMove(moves, pos, y, captureOnly);
                    if (!isEmpty(y)) {
                        break;
                    }
                    
                }
                break;
            }
                
            case PieceType::knight:
            {
                int col = getColumn(pos);
                int y = pos - 6;
                if (y >= 0 && col < 6)
                    gen_addMove(moves, pos, y, captureOnly);
                y = pos - 10;
                if (y >= 0 && col > 1)
                    gen_addMove(moves, pos, y, captureOnly);
                y = pos - 15;
                if (y >= 0 && col < 7)
                    gen_addMove(moves, pos, y, captureOnly);
                y = pos - 17;
                if (y >= 0 && col > 0)
                    gen_addMove(moves, pos, y, captureOnly);
                y = pos + 6;
                if (y < 64 && col > 1)
                    gen_addMove(moves, pos, y, captureOnly);
                y = pos + 10;
                if (y < 64 && col < 6)
                    gen_addMove(moves, pos, y, captureOnly);
                y = pos + 15;
                if (y < 64 && col > 0)
                    gen_addMove(moves, pos, y, captureOnly);
                y = pos + 17;
                if (y < 64 && col < 7)
                    gen_addMove(moves, pos, y, captureOnly);
                break;
            }
                
            case PieceType::pawn:
            {
                int col = getColumn(pos);
                if (side == Side::black) {
                    if (!captureOnly && isEmpty(pos + 8)) {
                        gen_addPawnMove(moves, pos, pos + 8, captureOnly);
                    }
                    if (!captureOnly && pos < 16 && isEmpty(pos + 8) && isEmpty(pos + 16)) {
                        gen_addMove(moves, pos, pos + 16, captureOnly);
                    }
                    
                    if (col && (getPiece(pos + 7).side == Side::white || (pos + 7 == enpassant && getPiece(pos + 7).side == Side::none))) {
                        gen_addPawnMove(moves, pos, pos + 7, captureOnly);
                    }
                    if (col < 7 && (getPiece(pos + 9).side == Side::white || (pos + 9 == enpassant && getPiece(pos + 9).side == Side::none))) {
                        gen_addPawnMove(moves, pos, pos + 9, captureOnly);
                    }
                } else {
                    if (!captureOnly && isEmpty(pos - 8)) {
                        gen_addPawnMove(moves, pos, pos - 8, captureOnly);
                    }
                    if (!captureOnly && pos >= 48 && isEmpty(pos - 8) && isEmpty(pos - 16)) {
                        gen_addMove(moves, pos, pos - 16, captureOnly);
                    }
                    
                    if (col < 7 && (getPiece(pos - 7).side == Side::black || (pos - 7 == enpassant && getPiece(pos - 7).side == Side::none)))
                        gen_addPawnMove(moves, pos, pos - 7, captureOnly);
                    if (col && (getPiece(pos - 9).side == Side::black || (pos - 9 == enpassant && getPiece(pos - 9).side == Side::none)))
                        gen_addPawnMove(moves, pos, pos - 9, captureOnly);
                }
                break;
            }
                
            default:
                break;
        }
    }
}


bool ChessBoard::beAttacked(int pos, Side attackerSide) const
{
    int row = getRow(pos), col = getColumn(pos);
    /* Check attacking of Knight */
    if (col > 0 && row > 1 && isPiece(pos - 17, PieceType::knight, attackerSide))
        return true;
    if (col < 7 && row > 1 && isPiece(pos - 15, PieceType::knight, attackerSide))
        return true;
    if (col > 1 && row > 0 && isPiece(pos - 10, PieceType::knight, attackerSide))
        return true;
    if (col < 6 && row > 0 && isPiece(pos - 6, PieceType::knight, attackerSide))
        return true;
    if (col > 1 && row < 7 && isPiece(pos + 6, PieceType::knight, attackerSide))
        return true;
    if (col < 6 && row < 7 && isPiece(pos + 10, PieceType::knight, attackerSide))
        return true;
    if (col > 0 && row < 6 && isPiece(pos + 15, PieceType::knight, attackerSide))
        return true;
    if (col < 7 && row < 6 && isPiece(pos + 17, PieceType::knight, attackerSide))
        return true;
    
    /* Check horizontal and vertical lines for attacking of Queen, Rook, King */
    /* go down */
    for (int y = pos + 8; y < 64; y += 8) {
        auto piece = getPiece(y);
        if (!piece.isEmpty()) {
            if (piece.side == attackerSide) {
                if (piece.type == PieceType::queen || piece.type == PieceType::rook ||
                    (piece.type == PieceType::king && y == pos + 8)) {
                    return true;
                }
            }
            break;
        }
    }
    
    /* go up */
    for (int y = pos - 8; y >= 0; y -= 8) {
        auto piece = getPiece(y);
        if (!piece.isEmpty()) {
            if (piece.side == attackerSide) {
                if (piece.type == PieceType::queen || piece.type == PieceType::rook ||
                    (piece.type == PieceType::king && y == pos - 8)) {
                    return true;
                }
            }
            break;
        }
    }
    
    /* go left */
    for (int y = pos - 1; y >= pos - col; y--) {
        auto piece = getPiece(y);
        if (!piece.isEmpty()) {
            if (piece.side == attackerSide) {
                if (piece.type == PieceType::queen || piece.type == PieceType::rook ||
                    (piece.type == PieceType::king && y == pos - 1)) {
                    return true;
                }
            }
            break;
        }
    }
    
    /* go right */
    for (int y = pos + 1; y < pos - col + 8; y++) {
        auto piece = getPiece(y);
        if (!piece.isEmpty()) {
            if (piece.side == attackerSide) {
                if (piece.type == PieceType::queen || piece.type == PieceType::rook ||
                    (piece.type == PieceType::king && y == pos + 1)) {
                    return true;
                }
            }
            break;
        }
    }
    
    /* Check diagonal lines for attacking of Queen, Bishop, King, Pawn */
    /* go right down */
    for (int y = pos + 9; y < 64 && getColumn(y) != 0; y += 9) {
        auto piece = getPiece(y);
        if (!piece.isEmpty()) {
            if (piece.side == attackerSide) {
                if (piece.type == PieceType::queen || piece.type == PieceType::bishop ||
                    (y == pos + 9 && (piece.type == PieceType::king || (piece.type == PieceType::pawn && piece.side == Side::white)))) {
                    return true;
                }
            }
            break;
        }
    }
    
    /* go left down */
    for (int y = pos + 7; y < 64 && getColumn(y) != 7; y += 7) {
        auto piece = getPiece(y);
        if (!piece.isEmpty()) {
            if (piece.side == attackerSide) {
                if (piece.type == PieceType::queen || piece.type == PieceType::bishop ||
                    (y == pos + 7 && (piece.type == PieceType::king || (piece.type == PieceType::pawn && piece.side == Side::white)))) {
                    return true;
                }
            }
            break;
        }
    }
    
    /* go left up */
    for (int y = pos - 9; y >= 0 && getColumn(y) != 7; y -= 9) {
        auto piece = getPiece(y);
        if (!piece.isEmpty()) {
            if (piece.side == attackerSide) {
                if (piece.type == PieceType::queen || piece.type == PieceType::bishop ||
                    (y == pos - 9 && (piece.type == PieceType::king || (piece.type == PieceType::pawn && piece.side == Side::black)))) {
                    return true;
                }
            }
            break;
        }
    }
    
    /* go right up */
    for (int y = pos - 7; y >= 0 && getColumn(y) != 0; y -= 7) {
        auto piece = getPiece(y);
        if (!piece.isEmpty()) {
            if (piece.side == attackerSide) {
                if (piece.type == PieceType::queen || piece.type == PieceType::bishop ||
                    (y == pos - 7 && (piece.type == PieceType::king || (piece.type == PieceType::pawn && piece.side == Side::black)))) {
                    return true;
                }
            }
            break;
        }
    }
    
    return false;
}


void ChessBoard::make(const Move& move, Hist& hist) {
    assert(istHashKeyValid());
    
    hist.enpassant = enpassant;
    hist.status = status;
    hist.castleRights[0] = castleRights[0];
    hist.castleRights[1] = castleRights[1];
    hist.move = move;
    hist.cap = pieces[move.dest];
    hist.hashKey = hashKey;
    hist.quietCnt = quietCnt;
    
    hashKey ^= xorHashKey(move.from);
    if (!hist.cap.isEmpty()) {
        hashKey ^= xorHashKey(move.dest);
    }
    
    auto p = pieces[move.from];
    pieces[move.dest] = p;
    pieces[move.from].setEmpty();
    
    hashKey ^= xorHashKey(move.dest);
    
    quietCnt++;
    enpassant = -1;
    
    if ((castleRights[0] + castleRights[1]) && hist.cap.type == PieceType::rook) {
        clearCastleRights(move.dest, hist.cap.side);
    }
    
    switch (p.type) {
        case PieceType::king: {
            castleRights[static_cast<int>(p.side)] &= ~(CASTLERIGHT_LONG|CASTLERIGHT_SHORT);
            if (abs(move.from - move.dest) == 2) { // castle
                int rookPos = move.from + (move.from < move.dest ? 3 : -4);
                int newRookPos = (move.from + move.dest) / 2;
                
                hashKey ^= xorHashKey(rookPos);
                pieces[newRookPos] = pieces[rookPos];
                pieces[rookPos].setEmpty();
                hashKey ^= xorHashKey(newRookPos);
                quietCnt = 0;
            }
            break;
        }
            
        case PieceType::rook: {
            if (castleRights[0] + castleRights[1]) {
                clearCastleRights(move.from, p.side);
            }
            break;
        }
            
        case PieceType::pawn: {
            int d = abs(move.from - move.dest);
            
            if (d == 16) {
                enpassant = (move.from + move.dest) / 2;
            } else if (move.dest == hist.enpassant) {
                int ep = move.dest + (p.side == Side::white ? +8 : -8);
                hist.cap = pieces[ep];
                
                hashKey ^= xorHashKey(ep);
                pieces[ep].setEmpty();
            } else {
                if (move.promotion != PieceType::empty) {
                    hashKey ^= xorHashKey(move.dest);
                    pieces[move.dest].type = move.promotion;
                    hashKey ^= xorHashKey(move.dest);
                    quietCnt = 0;
                }
            }
            break;
        }
        default:
            break;
    }
    
    if (!hist.cap.isEmpty()) {
        quietCnt = 0;
    }
    //    assert(hashKey == initHashKey());
}

void ChessBoard::make(const Move& move) {
    Hist hist;
    make(move, hist);
    histList.push_back(hist);
    side = getXSide(side);
    
    //    if (side == Side::black) {
    //    }
    assert(istHashKeyValid());
}

void ChessBoard::takeBack(const Hist& hist) {
    auto movep = getPiece(hist.move.dest);
    setPiece(hist.move.from, movep);
    
    int capPos = hist.move.dest;
    
    if (movep.type == PieceType::pawn && hist.enpassant == hist.move.dest) {
        capPos = hist.move.dest + (movep.side == Side::white ? +8 : -8);
        setEmpty(hist.move.dest);
    }
    setPiece(capPos, hist.cap);
    
    if (movep.type == PieceType::king) {
        if (abs(hist.move.from - hist.move.dest) == 2) {
            int rookPos = hist.move.from + (hist.move.from < hist.move.dest ? 3 : -4);
            assert(isEmpty(rookPos));
            int newRookPos = (hist.move.from + hist.move.dest) / 2;
            setPiece(rookPos, Piece(PieceType::rook, hist.move.dest < 8 ? Side::black : Side::white));
            setEmpty(newRookPos);
        }
    }
    
    if (hist.move.promotion != PieceType::empty) {
        setPiece(hist.move.from, Piece(PieceType::pawn, hist.move.dest < 8 ? Side::white : Side::black));
    }
    
    status = hist.status;
    castleRights[0] = hist.castleRights[0];
    castleRights[1] = hist.castleRights[1];
    enpassant = hist.enpassant;
    quietCnt = hist.quietCnt;
    
    //    pieceList_takeback(hist);
    hashKey = hist.hashKey;
}


void ChessBoard::takeBack() {
    auto hist = histList.back();
    histList.pop_back();
    side = getXSide(side);
    takeBack(hist);
    //    hashKey = hist.hashKey;
    assert(hashKey == initHashKey());
}


Result ChessBoard::rule()
{
    assert(istHashKeyValid());
    Result result;
    
    // Mated or stalemate
    auto haveLegalMove = false;
    MoveList moveList;
    gen(moveList, side);
    for(int i = 0; i < moveList.end; i++) {
        auto move = moveList.list[i];
        Hist hist;
        make(move, hist);
        if (!isIncheck(side)) {
            haveLegalMove = true;
        }
        takeBack(hist);
        if (haveLegalMove) break;
    }
    
    if (!haveLegalMove) {
        if (isIncheck(side)) {
            result.result = side == Side::white ? ResultType::loss : ResultType::win;
            result.reason = ReasonType::mate;
        } else {
            result.result = ResultType::draw;
            result.reason = ReasonType::stalemate;
        }
        return result;
        
    }
    
    // draw by insufficientmaterial
    int pieceCnt[2][7];
    memset(pieceCnt, 0, sizeof(pieceCnt));
    
    int total = 0;
    for(int i = 0; i < 64; i++) {
        auto p = pieces[i];
        if (p.type == PieceType::empty) continue;
        pieceCnt[static_cast<int>(p.side)][static_cast<int>(p.type)]++;
        total++;
    }
    
    if (total <= 3) {
        auto draw = false;
        if (total <= 2) {
            draw = true;
        } else {
            auto b = static_cast<int>(PieceType::bishop);
            draw = pieceCnt[0][b] + pieceCnt[1][b];
        }
        
        if (draw) {
            result.result = ResultType::draw;
            result.reason = ReasonType::insufficientmaterial;
            return result;
        }
    }
    
    // 50 moves
    if (quietCnt >= 50 * 2) {
        result.result = ResultType::draw;
        result.reason = ReasonType::fiftymoves;
        return result;
    }
    
    if (quietCnt >= 3 * 4) {
        auto cnt = 0;
        auto i = int(histList.size()), k = i - quietCnt;
        for(i -= 2; i >= 0 && i >= k; i -= 2) {
            auto hist = histList.at(i);
            if (hist.hashKey == hashKey) {
                cnt++;
            }
        }
        if (cnt >= 3) {
            result.result = ResultType::draw;
            result.reason = ReasonType::repetition;
            return result;
        }
        
    }
    
    return result;
}

// Check and make the move if it is legal
bool ChessBoard::checkMake(int from, int dest, PieceType promotion)
{
    if (!Move::isValid(from, dest)) {
        return false;
    }
    
    auto piece = getPiece(from);
    if (piece.isEmpty()
        || piece.side != side
        || piece.side == getPiece(dest).side
        || !isValidPromotion(promotion, piece.side)) {
        return false;
    }
    
    MoveList moveList;
    gen(moveList, side);
    
    for (int i = 0; i < moveList.end; i++) {
        auto move = moveList.list[i];
        
        if (move.from != from || move.dest != dest || move.promotion != promotion) {
            continue;
        }
        
        auto theSide = side;
        auto fullmove = createMove(from, dest, promotion);
        make(fullmove); assert(side != theSide);
        
        if (isIncheck(theSide)) {
            takeBack();
            return false;
        }
        
        createStringForLastMove(moveList);
        assert(isValid());
        return true;
    }
    
    return false;
}

bool ChessBoard::createStringForLastMove(const MoveList& moveList)
{
    if (histList.empty()) {
        return false;
    }
    
    auto hist = &histList.back();
    
    auto movePiece = hist->move.piece;
    if (movePiece.isEmpty()) {
        return false; // something wrong
    }
    
    // special cases - castling moves
    if (movePiece.type == PieceType::king && std::abs(hist->move.from - hist->move.dest) == 2) {
        auto col = hist->move.dest % 8;
        hist->moveString = col < 4 ? "O-O-O" : "O-O";
        return true;
    }
    
    auto ambi = false, sameCol = false, sameRow = false;
    
    if (movePiece.type != PieceType::king) {
        for(int i = 0; i < moveList.end; i++) {
            auto m = moveList.list[i];
            if (m.dest == hist->move.dest
                && m.from != hist->move.from
                && pieces.at(m.from).type == movePiece.type) {
                ambi = true;
                if (m.from / 8 == hist->move.from / 8) {
                    sameRow = true;
                }
                if (m.from % 8 == hist->move.from % 8) {
                    sameCol = true;
                }
            }
        }
    }
    
    std::string str;
    if (movePiece.type != PieceType::pawn) {
        str = char(pieceTypeName[static_cast<int>(movePiece.type)] - 'a' + 'A');
    }
    if (ambi) {
        if (sameCol && sameRow) {
            str += posToCoordinateString(hist->move.from);
        } else if (sameCol) {
            str += std::to_string(8 - hist->move.from / 8);
        } else {
            str += char('a' + hist->move.from % 8);
        }
    }
    
    if (!hist->cap.isEmpty()) {
        str += "x";
    }
    
    str += posToCoordinateString(hist->move.dest);
    
    // promotion
    if (hist->move.promotion != PieceType::empty) {
        str += "=";
        str += char(pieceTypeName[static_cast<int>(hist->move.promotion)] - 'a' + 'A');
    }
    
    // incheck
    if (isIncheck(side)) {
        str += "+";
    }
    
    hist->moveString = str;
    return true;
}

std::string ChessBoard::toMoveListString(MoveNotation notation, int itemPerLine, bool moveCounter) const
{
    std::ostringstream stringStream;
    
    auto c = 0;
    for(int i = 0; i < histList.size(); i++) {
        if (c) stringStream << " ";
        if (moveCounter && (i & 1) == 0) {
            stringStream << (1 + i / 2) << ". ";
        }
        
        auto hist = histList.at(i);
        switch (notation) {
            case MoveNotation::san:
                stringStream << hist.moveString;
                break;
                
            case MoveNotation::coordinate:
            default:
                stringStream << hist.move.toCoordinateString();
                break;
        }
        
        // Comment
        if (!hist.comment.empty() && moveCounter) {
            stringStream << " {" << hist.comment << "} ";
        }
        
        c++;
        if (itemPerLine > 0 && c >= itemPerLine) {
            c = 0;
            stringStream << std::endl;
        }
    }
    
    return stringStream.str();
}

PieceType ChessBoard::charToPieceType(char ch) const
{
    if (ch >= 'A' && ch <= 'Z') {
        ch += 'a' - 'A';
    }
    const char* p = strchr(pieceTypeName, ch);
    if (p != nullptr) {
        int k = (int)(p - pieceTypeName);
        return static_cast<PieceType>(k);
    }
    
    return PieceType::empty;
}

bool ChessBoard::fromSanMoveList(const std::string& str)
{
    std::string str2;
    for(auto ch : str) {
        if (ch != '+' && ch != 'x' && ch != '*' && ch != '#') {
            if (ch < ' ' || ch == '.') ch = ' ';
            str2 += ch;
        }
    }
    
    auto vec = splitString(str2, ' ');
    
    for(auto && s : vec) {
        if (s.length() < 2 || isdigit(s.at(0))) {
            continue;
        }
        
        int from = -1, dest = -1, fromCol = -1, fromRow = -1;
        auto pieceType = PieceType::pawn, promotion = PieceType::empty;
        
        if (s == "O-O" || s == "O-O-O") {
            from = side == Side::black ? 4 : 60;
            dest = from + (s == "O-O" ? 2 : -2);
        } else {
            auto p = s.find("=");
            if (p != std::string::npos) {
                if (s.length() == p + 1) {
                    return false; // something wrong
                }
                char ch = s.at(p + 1);
                promotion = charToPieceType(ch);
                
                s = s.substr(0, p);
                if (s.size() < 2 || promotion == PieceType::empty) {
                    return false;
                }
            }
            
            auto destString = s.substr(s.length() - 2, 2);
            dest = coordinateStringToPos(destString.c_str());
            
            if (!isPositionValid(dest)) {
                return false;
            }
            
            if (s.length() > 2) {
                auto k = 0;
                char ch = s.at(0);
                if (ch >= 'A' && ch <= 'Z') {
                    k++;
                    pieceType = charToPieceType(ch);
                    
                    if (pieceType == PieceType::empty) {
                        return false;
                    }
                }
                
                auto left = s.length() - k - 2;
                if (left > 0) {
                    s = s.substr(k, left);
                    if (left == 2) {
                        from = coordinateStringToPos(s.c_str());
                    } else {
                        char ch = s.at(0);
                        if (isdigit(ch)) {
                            fromCol = ch - '0';
                        } else if (ch >= 'a' && ch <= 'z') {
                            fromRow = ch - 'a';
                        }
                    }
                }
            }
            
            if (from < 0) {
                MoveList moveList;
                gen(moveList, side);
                
                for(int i = 0; i < moveList.end; i++) {
                    auto m = moveList.list[i];
                    if (m.dest != dest || m.promotion != promotion ||
                        getPiece(m.from).type != pieceType) {
                        continue;
                    }
                    
                    if ((fromRow < 0 || getRow(m.from) == fromRow) && (fromCol < 0 && getColumn(m.from) != fromCol)) {
                        from = m.from;
                        break;
                    }
                }
            }
        }
        
        if (!checkMake(from, dest, promotion)) {
            return false;
        }
    }
    return true;
}


