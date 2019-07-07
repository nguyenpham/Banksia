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


#include "configmng.h"

using namespace banksia;

static const char* protocalNames[] = {
    "uci", "wb",
    nullptr
};

Protocol protocolFromString(const std::string& string)
{
    for(int i = 0; protocalNames[i]; i++) {
        if (strcmp(protocalNames[i], string.c_str()) == 0) {
            return static_cast<Protocol>(i);
        }
    }
    return Protocol::none;
}

const char* nameFromProtocol(Protocol protocol)
{
    auto k = static_cast<int>(protocol);
    if (k >= 0 && protocol < Protocol::none) {
        return protocalNames[k];
    }
    return nullptr;
}

std::vector<std::string> protocolList()
{
    std::vector<std::string> list;
    for(int i = 0; protocalNames[i]; i++) {
        list.push_back(protocalNames[i]);
    }
    return list;
}


static const char* OptionNames[] = {
    "check", "spin", "combo", "button", "string",
    nullptr, nullptr
};

OptionType Option::stringToOptionType(const std::string& name)
{
    for(int i = 0; OptionNames[i]; i++) {
        if (std::strcmp(OptionNames[i], name.c_str()) == 0) {
            return static_cast<OptionType>(i);
        }
    }
    
    return OptionType::none;
}

const char* Option::getName(OptionType type)
{
    return OptionNames[static_cast<int>(type)];
}


//////////////////////////////////////
Option::Option()
:type(OptionType::none)
{
    
}

Option::Option(OptionType type, const std::string& name)
:type(type), name(name)
{
    
}

Option::Option(const Json::Value& obj)
{
    load(obj);
}

bool Option::load(const Json::Value& obj)
{
    if (!obj.isMember("name") || !obj.isMember("type")) {
        return false;
    }
    name = obj["name"].asString();
    auto s = obj["type"].asString();
    
    type = stringToOptionType(s);
    
    if (name.empty() || type == OptionType::none) {
        return false;
    }
    
    if (type == OptionType::button) {
        return true;
    }
    
    if (!obj.isMember("value") || !obj.isMember("default")) {
        return false;
    }
    
    if (type == OptionType::check) {
        checked = obj["value"].asBool();
        defaultChecked = obj["default"].asBool();
    }
    else if (type == OptionType::spin) {
        value = obj["value"].asInt();
        defaultValue = obj["default"].asInt();
        if (obj.isMember("min") && obj.isMember("max")) {
            minValue = obj["min"].asInt();
            maxValue = obj["max"].asInt();
        }
    }
    else {
        string = obj["value"].asString();
        defaultString = obj["default"].asString();
        if (type == OptionType::combo) {
            choiceList.clear();
            if (obj.isMember("choices")) {
                const Json::Value& array = obj["choices"];
                for (int i = 0; i < int(array.size()); i++){
                    auto str = array[i].asString();
                    if (!str.empty()) {
                        choiceList.push_back(str);
                    }
                }
            }
        }
    }
    return true;
}

Json::Value Option::saveToJson() const
{
    Json::Value obj;
    obj["name"] = name;
    obj["type"] = getName(type);
    
    switch (type) {
        case OptionType::check:
            obj["value"] = checked;
            obj["default"] = defaultChecked;
            break;
            
        case OptionType::spin:
            obj["value"] = value;
            obj["default"] = defaultValue;
            obj["min"] = minValue;
            obj["max"] = maxValue;
            break;
            
        case OptionType::string:
            obj["value"] = string;
            obj["default"] = defaultString;
            break;
            
        case OptionType::combo:
        {
            obj["value"] = string;
            obj["default"] = defaultString;
            
            Json::Value array;
            for(auto && s : choiceList) {
                if (s.empty()) continue;
                array.append(s);
            }
            obj["choices"] = array;
            break;
        }
        default:
            break;
    }
    
    return obj;
}

void Option::setValue(int val)
{
    value = val;
}

void Option::setDefaultValue(int defaultVal, int minVal, int maxVal)
{
    value = defaultValue = defaultVal;
    minValue = minVal;
    maxValue = maxVal;
}

void Option::setValue(bool val)
{
    checked = val;
}

void Option::setDefaultValue(bool val)
{
    checked = defaultChecked = val;
}

void Option::setValue(const std::string& val)
{
    string = val;
}

void Option::setDefaultValue(const std::string& val)
{
    string = defaultString = val;
}

void Option::setDefaultValue(const std::string& val, const std::vector<std::string> choices)
{
    defaultString = val;
    choiceList = choices;
}

bool Option::isDefaultValue() const
{
    switch (type) {
        case OptionType::spin:
            return defaultValue == value;
            
        case OptionType::combo:
        case OptionType::string:
            return string == defaultString;
            
        case OptionType::check:
            return defaultChecked == checked;
            
        default:
            break;
    }
    
    return true;
}

bool Option::operator == (const Option& other) const
{
    if (name != other.name || type != other.type) {
        return false;
    }
    
    switch (type) {
        case OptionType::spin:
            return minValue == other.minValue && maxValue == other.maxValue
            && defaultValue == other.defaultValue
            && value == other.value;
            
        case OptionType::combo:
            return string == other.string
            && choiceList == other.choiceList;
            
        case OptionType::string:
            return string == other.string;
            
        case OptionType::check:
            return defaultChecked == other.defaultChecked && checked == other.checked;
            
        default:
            break;
    }
    
    return true;
}

void Option::update(const Option& other)
{
    auto s = string;
    auto c = checked;
    auto v = value;
    *this = other;
    
    string = s;
    checked = c;
    value = v;
}

std::string Option::toString() const
{
    std::ostringstream stringStream;
    switch (type) {
        case OptionType::check:
            stringStream << "checked: " << checked;
            break;
            
        case OptionType::spin:
            stringStream << "spin: " << value << ", default: " << defaultValue << ", minmax: " << minValue << "->" << maxValue;
            break;
            
        case OptionType::combo:
            stringStream << "combo: ";
            for(auto && s : choiceList) stringStream << s << "; ";
            break;
            
        case OptionType::button:
            stringStream << "button";
            break;
            
        case OptionType::string:
            stringStream << "string: " << string;
            break;
            
        default:
            break;
    }
    return stringStream.str();
}


bool Option::isValid() const
{
    if (name.empty()) {
        return false;
    }
    
    switch (type) {
        case OptionType::spin:
            return minValue < maxValue
            && defaultValue >= minValue && defaultValue <= maxValue
            && value >= minValue && value <= maxValue;
            
        case OptionType::combo:
            if (!choiceList.empty()) {
                if (defaultString.empty()) {
                    return true;
                }
                for(auto && s : choiceList) {
                    if (s == defaultString) {
                        return true;
                    }
                }
            }
            return false;
            
        case OptionType::check:
        case OptionType::button:
        case OptionType::string:
            return true;
            
        default:
            return false;
    }
}


/////////////////////////////////

Config::Config()
{
}

bool Config::load(const Json::Value& obj)
{
    if (!obj.isMember("command")) {
        return false;
    }
    
    protocol = Protocol::none;
    if (obj.isMember("protocol")) {
        protocol = protocolFromString(obj["protocol"].asString());
    }
    if (protocol == Protocol::none) protocol = Protocol::uci;
    
    command = obj["command"].asString();
    
    if (obj.isMember("name") && obj["name"].isString()) {
        name = obj["name"].asString();
    } else {
        name = getFileName(command);
    }

    if (protocol == Protocol::none || name.empty() || command.empty()) {
        protocol = Protocol::none;
        return false;
    }
    
    if (obj.isMember("working folder")) workingFolder = obj["working folder"].asString();
    if (obj.isMember("ponderable")) ponderable = obj["ponderable"].asBool();
    
    variantSet.clear();
    if (obj.isMember("variants")) {
        const Json::Value& array = obj["variants"];
        for (int i = 0; i < int(array.size()); i++){
            auto str = array[i].asString();
            if (!str.empty()) {
                variantSet.insert(str);
            }
        }
    } else {
        variantSet.insert("standard");
    }
    
    argumentList.clear();
    if (obj.isMember("arguments")) {
        const Json::Value& array = obj["arguments"];
        for (int i = 0; i < int(array.size()); i++){
            auto str = array[i].asString();
            if (!str.empty()) {
                argumentList.push_back(str);
            }
        }
    }
    
    initStringList.clear();
    if (obj.isMember("initStrings")) {
        const Json::Value& array = obj["initStrings"];
        for (int i = 0; i < int(array.size()); i++){
            auto str = array[i].asString();
            if (!str.empty()) {
                initStringList.push_back(str);
            }
        }
    }
    
    optionList.clear();
    if (obj.isMember("options")) {
        const Json::Value& array = obj["options"];
        for (Json::Value::const_iterator it = array.begin(); it != array.end(); ++it) {
            Option engineOption(*it);
            if (engineOption.isValid()) {
                optionList.push_back(engineOption);
            }
        }
    }
    
    return true;
}

Json::Value Config::saveToJson() const
{
    Json::Value obj;

    obj["protocol"] = nameFromProtocol(protocol);
    obj["name"] = name;
    obj["command"] = command;
    obj["working folder"] = workingFolder;
    obj["ponderable"] = ponderable;

    if (!variantSet.empty()) {
        Json::Value array;
        for(auto && s : variantSet) {
            if (!s.empty()) {
                array.append(s);
            }
        }
        obj["variants"] = array;
    }

    if (!argumentList.empty()) {
        Json::Value array;
        for(auto && s : argumentList) {
            if (!s.empty()) {
                array.append(s);
            }
        }
        obj["arguments"] = array;
    }
    
    if (!initStringList.empty()) {
        Json::Value array;
        for(auto && s : initStringList) {
            if (!s.empty()) {
                array.append(s);
            }
        }
        obj["initStrings"] = array;
    }
    
    if (!optionList.empty()) {
        Json::Value array;
        for(auto && option : optionList) {
            auto o = option.saveToJson();
            array.append(o);
        }
        obj["options"] = array;
    }
    
    return obj;
}

bool Config::isValid() const
{
    if (protocol == Protocol::none || name.empty() || command.empty() || variantSet.empty()) {
        return false;
    }
    for(auto && option : optionList) {
        if (!option.isValid()) {
            return false;
        }
    }
    return true;
}

std::string Config::toString() const
{
    std::ostringstream stringStream;
    stringStream << "Config: " << name << ", " << command << ", " << workingFolder << std::endl;
    
    stringStream << "variantSet sz: " << variantSet.size() << ": ";
    for(auto && v : variantSet) {
        stringStream << v << "; ";
    }
    stringStream << std::endl;
    
    stringStream << "optionList sz: " << optionList.size() << ": ";
    for(auto && v : optionList) {
        stringStream << v.toString() << "; ";
    }
    stringStream << std::endl;
    
    return stringStream.str();
}

Option* Config::getOption(const std::string& name)
{
    for(auto && option : optionList) {
        if (option.name == name) {
            return &option;
        }
    }
    return nullptr;
}

const Option* Config::getOption(const std::string& name) const
{
    for(auto && option : optionList) {
        if (option.name == name) {
            return &option;
        }
    }
    return nullptr;
}

void Config::updateOption(const Option& o)
{
    for(auto && option : optionList) {
        if (option.name == name) {
            if (option != o) {
                option.update(o);
            }
            return;
        }
    }
    appendOption(o);
}

void Config::appendOption(const Option& option)
{
    if (option.name == "Ponder") {
        ponderable = true;
        return;
    }
    
    optionList.push_back(option);
}

bool Config::updateOptionValue(const std::string& name, int val)
{
    auto option = getOption(name);
    if (option == nullptr) return false;
    option->value = val;
    assert(option->type == OptionType::spin);
    return true;
}

bool Config::updateOptionValue(const std::string& name, bool val)
{
    auto option = getOption(name);
    if (option == nullptr) return false;
    option->checked = val;
    assert(option->type == OptionType::check);
    return true;
}

bool Config::updateOptionValue(const std::string& name, const std::string& val)
{
    auto option = getOption(name);
    if (option == nullptr) return false;
    option->string = val;
    assert(option->type == OptionType::string);
    return true;
}

/////////////////////////////////////////////
ConfigMng* ConfigMng::instance = nullptr;

ConfigMng::ConfigMng()
{
    assert(!instance);
    instance = this;
}

ConfigMng::~ConfigMng()
{
}

bool ConfigMng::isValid() const
{
    for(auto && p : configMap) {
        if (p.first.empty() || !p.second.isValid() || p.first != p.second.name) {
            return false;
        }
    }
    return true;
}

std::string ConfigMng::toString() const
{
    std::ostringstream stringStream;
    for(auto && p : configMap) {
        stringStream << p.second.toString() << std::endl;
    }
    stringStream << std::endl;
    return stringStream.str();
}

bool ConfigMng::empty() const
{
    return configMap.empty();
}

bool ConfigMng::isNameExistent(const std::string& name) const
{
    return configMap.find(name) != configMap.end();
}

std::vector<std::string> ConfigMng::nameList() const
{
    std::vector<std::string> list;
    for(auto && c : configMap) {
        list.push_back(c.first);
    }
    return list;
}

bool ConfigMng::parseJsonAfterLoading(Json::Value& jsonData)
{
    for (Json::Value::const_iterator it = jsonData.begin(); it != jsonData.end(); ++it) {
        Config config;
        config.load(*it);
        if (config.isValid()) {
            insert(config);
        }
    }
    return true;
}


Json::Value ConfigMng::createJsonForSaving()
{
    Json::Value jsonData;
    for(auto && s : configMap) {
        auto obj = s.second.saveToJson();
        jsonData.append(obj);
    }
    
    return jsonData;
}

Config ConfigMng::get(const std::string& name) const
{
    auto it = configMap.find(name);
    if (it != configMap.end()) return it->second;
    return Config();
}

Config ConfigMng::get(int idx) const
{
    if (idx < configMap.size()) {
        auto it = configMap.begin();
        std::advance(it, idx);
        return it->second;
    }
    return Config();
}


bool ConfigMng::update(const std::string& oldname, const Config& config)
{
    if (!oldname.empty() && oldname != config.name) {
        auto it = configMap.find(oldname);
        if (it != configMap.end()) {
            configMap.erase(it);
        }
    }
    return update(config);
}

bool ConfigMng::update(const Config& config)
{
    return insert(config);
}

bool ConfigMng::insert(const Config& config)
{
    if (config.isValid()) {
        if (configMap.find(config.name) != configMap.end()) {
            std::cerr << "Warning: configuration name's " << config.name << " is repeated. Override data.\n";
        }
        configMap[config.name] = config;
        return true;
    }
    return false;
}



