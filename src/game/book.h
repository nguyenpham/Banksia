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


#ifndef book_h
#define book_h

#include <stdio.h>

#include "../chess/chess.h"

namespace banksia {
    
    const int PologlotDefaultMaxPly = 20;
    enum class BookType {
        edp, pgn, polygot, none
    };

    class Book : public Obj
    {
    public:
        Book(BookType t) : type(t) {}
        virtual ~Book() {}
        
        virtual bool isEmpty() const = 0;
        virtual size_t size() const = 0;

        virtual bool getRandomBook(std::string& fenString, std::vector<Move>& moves) const = 0;

    public:
        virtual void load(const std::string& path, int maxPly, int top100) = 0;
        BookType type;
        
    protected:
        std::string path;
        int maxPly = PologlotDefaultMaxPly, top100 = 0;
    };
    
    class BookEdp : public Book
    {
    public:
        BookEdp() : Book(BookType::edp) {}
        virtual ~BookEdp() {}
        
        virtual const char* className() const override { return "BookEdp"; }
        bool isEmpty() const override;
        size_t size() const override;
        
        bool getRandomBook(std::string& fenString, std::vector<Move>& moves) const override;
        void load(const std::string& path, int maxPly, int top100) override;
        
    private:
        std::string getRandomFEN() const;
        std::vector<std::string> stringVec;
    };

    class BookPgn : public Book
    {
    public:
        BookPgn() : Book(BookType::pgn) {}
        virtual ~BookPgn() {}
        
        virtual const char* className() const override { return "BookPgn"; }
        
        bool isEmpty() const override;
        size_t size() const override;
        bool getRandomBook(std::string& fenString, std::vector<Move>& moves) const override;
        void load(const std::string& path, int maxPly, int top100) override;
        
    private:
        void loadPgnBook(const std::string& path);
        bool addPgnMoves(const std::string& s);
        
        std::vector<std::vector<Move>> moves;
    };
    
    class BookPolyglotItem {
    public:
        Move getMove() const;
        void convertToLittleEndian();
        std::string toString() const;
    public:
        u64 key;
        u16 move;
        u16 weight;
        u32 learn;
    };
    
    class BookPolyglot : public Book
    {
    public:
        BookPolyglot() : Book(BookType::polygot) {}
        virtual ~BookPolyglot();
        
        virtual const char* className() const override { return "BookPolyglot"; }
        virtual bool isValid() const override;

        bool isEmpty() const override;
        size_t size() const override;
        bool getRandomBook(std::string& fenString, std::vector<Move>& moves) const override;
        void load(const std::string& path, int maxPly, int top100) override;
        
        std::vector<BookPolyglotItem> search(u64 key) const;
        
    private:
        i64 binarySearch(u64 key) const;
        
        u64 itemCnt = 0;
        BookPolyglotItem* items = nullptr;
    };
    

    class BookMng : public Jsonable
    {
    public:
        BookMng();
        virtual ~BookMng();
        
        static BookMng* instance;
        
        virtual bool isValid() const override;
        virtual std::string toString() const override;
        virtual const char* className() const override { return "Book"; }
        
        virtual bool load(const Json::Value& obj) override;
        virtual Json::Value saveToJson() const override;
        bool getRandomBook(std::string& fenString, std::vector<Move>& moves) const;

        static BookType string2BookType(const std::string& name);
        
        bool isEmpty() const;
        size_t size() const;

    private:
        bool loadSingle(const Json::Value& obj);

        bool mode = true;
        std::vector<Book*> bookList;
    };
    
    
} // namespace banksia


#endif /* book_h */


