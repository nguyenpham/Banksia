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
    
    enum class BookType {
        edp, pgn, polygot, none
    };
    
    class Book : public Obj
    {
    public:
        Book();
        ~Book();
        
        virtual const char* className() const override { return "Book"; }
        virtual bool isValid() const override;
        virtual std::string toString() const override;
        
        virtual std::string getRandomFEN() const;
        bool isEmpty() const;
        
    public:
        void loadEpdFile(const std::string& epdPath);

        BookType type;
        std::string path;

        std::vector<std::string> epdVec;
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

        static BookType string2BookType(const std::string& name);
        
        virtual std::string getRandomFEN() const;

        bool isEmpty() const;
        
    private:
        bool mode = true;
        std::vector<Book> bookList;
    };
    
    
} // namespace banksia


#endif /* book_h */

