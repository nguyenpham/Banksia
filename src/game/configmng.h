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

#ifndef configmng_h
#define configmng_h

#include <vector>
#include <set>

#include "player.h"

namespace banksia {
    
    // TODO: support winboard protocol
    enum class Protocol
    {
        uci = 0, none
    };
    
    enum class OptionType {
        check, spin, combo, button, string, none
    };
    
    class Option : public Jsonable {
    public:
        Option();
        Option(OptionType type, const std::string& name);
        Option(const Json::Value& obj);
        
        static OptionType stringToOptionType(const std::string& name);
        static const char* getName(OptionType type);
        
        bool operator == (const Option&) const;
        bool operator != (const Option& other) const {
            auto b = *this == other;
            return !b;
        }
        void update(const Option& other);
        
        void setValue(int val);
        void setDefaultValue(int defaultVal, int minVal, int maxVal);
        void setValue(bool val);
        void setDefaultValue(bool val);
        void setValue(const std::string& val);
        void setDefaultValue(const std::string& val);
        
        void setDefaultValue(const std::string& val, const std::vector<std::string> choices);
        void setDefaultValue(const std::vector<std::string> choices) {
            std::string val = choices.empty() ? "" : choices.front();
            setDefaultValue(val, choices);
        }
        
        virtual const char* className() const override { return "Option"; }
        virtual std::string toString() const override;
        virtual bool isValid() const override;
        
        virtual bool load(const Json::Value& obj) override;
        virtual Json::Value saveToJson() const override;
        
        bool isDefaultValue() const;
        
    public:
        OptionType type;
        std::string name;
        std::string string, defaultString;                           // for string
        bool checked = false, defaultChecked = false;                // for checkbox option
        int value = 0, defaultValue = 0, minValue = 0, maxValue = 0; // for spin
        std::vector<std::string> choiceList;                         // for combo
        
    };
    
    class Config : public Jsonable
    {
    public:
        Config();
        virtual bool load(const Json::Value& obj) override;
        virtual Json::Value saveToJson() const override;
        
        Option* getOption(const std::string& name);
        const Option* getOption(const std::string& name) const;
        void updateOption(const Option&);
        void appendOption(const Option&);
        
        bool updateOptionValue(const std::string& name, int val);
        bool updateOptionValue(const std::string& name, bool val);
        bool updateOptionValue(const std::string& naßme, const std::string& val);
        
        virtual const char* className() const override { return "Config"; }
        virtual bool isValid() const override;
        virtual std::string toString() const override;
        
    public:
        Protocol protocol;
        std::string name, command, workingFolder;
        
        std::vector<std::string> argumentList, initStringList;
        std::set<std::string> variantSet;
        std::vector<Option> optionList;
        
        bool ponderable = false;
    };
    
    class ConfigMng : public Obj, public JsonSavable
    {
    public:
        ConfigMng();
        ~ConfigMng();
        
        static ConfigMng* instance;
        
        virtual const char* className() const override { return "ConfigMng"; }
        
        virtual bool isValid() const override;
        virtual std::string toString() const override;
        
        Config get(const std::string& name) const;
        Config get(int idx) const;
        
        bool update(const std::string& oldname, const Config&);
        bool update(const Config&);
        bool insert(const Config&);
        
        bool empty() const;
        
        bool isNameExistent(const std::string& name) const;
        
    protected:
        virtual Json::Value createJsonForSaving() override;
        
    private:
        virtual bool parseJsonAfterLoading(Json::Value&) override;
        
        std::map<std::string, Config> configMap;
    };
    
    
} // namespace banksia

#endif /* configmng_hpp */

