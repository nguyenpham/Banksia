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

#ifndef chess_h
#define chess_h

#include <stdio.h>

#include "../base/base.h"

namespace banksia {
    
    extern const char* originalFen;
    
    class ChessBoard : public BoardCore {
        
        const int CASTLERIGHT_LONG  = (1<<0);
        const int CASTLERIGHT_SHORT = (1<<1);
        const int CASTLERIGHT_MASK  = (CASTLERIGHT_LONG|CASTLERIGHT_SHORT);
        
    protected:
        int enpassant;
        int8_t castleRights[2];
        
    public:
        ChessBoard();
        ~ChessBoard();
        
        virtual std::string toString() const override;
        virtual bool isValid() const override;
        
        virtual int columnCount() const override;
        virtual int getColumn(int pos) const override;
        virtual int getRow(int pos) const override;
        
        virtual void setFen(const std::string& fen) override;
        virtual std::string getFen(int halfCount = 0, int fullMoveCount = 1) const override;
        
        void checkEnpassant();
        
        virtual void clearCastleRights(int rookPos, Side rookSide);
        int findKing(Side side) const;
        
        bool isLegalMove(int from, int dest, PieceType promotion = PieceType::empty);
        
        virtual void gen(MoveList& moveList, Side attackerSide) const;
        virtual void genLegalOnly(MoveList& moveList, Side attackerSide);
        virtual bool isIncheck(Side beingAttackedSide) const;
        virtual bool beAttacked(int pos, Side attackerSide) const;
        void genLegal(MoveList& moves, Side side, int from, int dest, PieceType promotion);
        
        virtual void make(const MoveFull& move, Hist& hist);
        virtual void takeBack(const Hist& hist);
        
        virtual void make(const MoveFull& move);
        virtual void takeBack();
        
        virtual Result rule() override;
        
        bool checkMake(int from, int dest, PieceType promotion);
        
        std::string toMoveListString(MoveNotation notation, int itemPerLine, bool moveCounter) const;
        
        Move fromSanString(const std::string&);
        bool fromSanMoveList(const std::string&);
        
        PieceType charToPieceType(char ch) const;
        
        u64 perft(int depth);
        
        virtual u64 initHashKey() const override;
        virtual u64 xorHashKey(int pos) const override;
        u64 hashKeyEnpassant(int enpassant) const;
        
    private:
        bool createStringForLastMove(const MoveList& moveList);
        
        void gen_addMove(MoveList& moveList, int from, int dest, bool capOnly) const;
        void gen_addPawnMove(MoveList& moveList, int from, int dest, bool capOnly) const;
        
    };
    
} // namespace banksia

#endif /* board_hpp */


