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


#include "configmng.h"

namespace banksia {
    
    static const char* protocalNames[] = {
        "uci", "wb", "none",
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
        if (k >= 0 && protocol <= Protocol::none) {
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
    
    ConfigMng configMng;
}

using namespace banksia;


static const char* OptionNames[] = {
    "check", "spin", "combo", "button", "reset", "save", "string", "file", "path",
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
    
    overridable = !obj.isMember("overridable") || obj["overridable"].asBool(); // default = true

    type = stringToOptionType(s);
    
    if (name.empty() || type == OptionType::none) {
        return false;
    }
    
    if (type == OptionType::button || type == OptionType::reset || type == OptionType::save) {
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
    
    if (!overridable) {
        obj["overridable"] = false;
    }
    
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
        case OptionType::file:
        case OptionType::path:
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
    if (overrideType)
        return false;
    
    switch (type) {
        case OptionType::spin:
            return defaultValue == value;
            
        case OptionType::combo:
        case OptionType::string:
        case OptionType::file:
        case OptionType::path:
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
        case OptionType::file:
        case OptionType::path:
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
        case OptionType::file:
        case OptionType::path:
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
        case OptionType::file:
        case OptionType::path:
            return true;
            
        default:
            return false;
    }
}


std::string Option::getValueAsString() const
{
    switch (type) {
        case OptionType::spin:
            return std::to_string(value);
            break;
        case OptionType::check:
            return checked ? "true" : "false";
            break;
        default: // combo, string
            break;
    }
    return string;
}

/////////////////////////////////

Config::Config()
{
}

bool Config::load(const Json::Value& obj)
{
    // root items: app, comment, options
    // "app" is used to group important information and show on top of all others
    if (!obj.isMember("app")) {
        return false;
    }
    auto app = obj["app"];

    if (!app.isMember("command")) {
        return false;
    }
    command = app["command"].asString();

    protocol = Protocol::none;
    if (app.isMember("protocol")) {
        protocol = protocolFromString(app["protocol"].asString());
    }
    
    if (app.isMember("name") && app["name"].isString()) {
        name = app["name"].asString();
    }
    
    if (name.empty()) {
        name = "<<<" + getFileName(command) + ">>>";
    }
    
    if (app.isMember("working folder")) {
        workingFolder = app["working folder"].asString();
    } else {
        workingFolder = getFolder(command);
    }
    
    if (app.isMember("ponderable")) ponderable = app["ponderable"].asBool(); // useful for Winboard only
    if (app.isMember("elo")) elo = app["elo"].asInt();
    
    variantSet.clear();
    if (app.isMember("variants")) {
        const Json::Value& array = app["variants"];
        for (int i = 0; i < int(array.size()); i++){
            auto str = array[i].asString();
            if (!str.empty()) {
                variantSet.insert(str);
            }
        }
    }
    
    argumentList.clear();
    if (app.isMember("arguments")) {
        const Json::Value& array = app["arguments"];
        for (int i = 0; i < int(array.size()); i++){
            auto str = array[i].asString();
            if (!str.empty()) {
                argumentList.push_back(str);
            }
        }
    }
    
    initStringList.clear();
    if (app.isMember("initStrings")) {
        const Json::Value& array = app["initStrings"];
        for (int i = 0; i < int(array.size()); i++){
            auto str = array[i].asString();
            if (!str.empty()) {
                initStringList.push_back(str);
            }
        }
    }
    
    if (obj.isMember("comment")) comment = obj["comment"].asString();
    
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
    Json::Value app;

    app["protocol"] = nameFromProtocol(protocol);
    app["name"] = name;
    app["command"] = command;
    app["working folder"] = workingFolder;
    
    app["elo"] = elo;
    if (protocol == Protocol::wb) { // useful for Winboard only
        app["ponderable"] = ponderable;
    }

    if (!variantSet.empty()) {
        Json::Value array;
        for(auto && s : variantSet) {
            if (!s.empty()) {
                array.append(s);
            }
        }
        app["variants"] = array;
    }
    
    if (argumentList.empty()){
        app["arguments"] = Json::arrayValue;
    } else {
        Json::Value array;
        for(auto && s : argumentList) {
            if (!s.empty()) {
                array.append(s);
            }
        }
        app["arguments"] = array;
    }
    
    if (initStringList.empty()){
        app["initStrings"] = Json::arrayValue;
    } else {
        Json::Value array;
        for(auto && s : initStringList) {
            if (!s.empty()) {
                array.append(s);
            }
        }
        app["initStrings"] = array;
    }
    
    Json::Value obj;
    
    obj["app"] = app;
    

    obj["comment"] = comment;
    
    if (optionList.empty()){
        obj["options"] = Json::arrayValue;
    } else {
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
    if (protocol == Protocol::none || name.empty() || command.empty()) { //  || variantSet.empty()
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
    stringStream << "Config: " << name << ", " << idName << ", " << nameFromProtocol(protocol) << ", "
    << command << ", " << workingFolder
    << std::endl;
    
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
        if (option.name == o.name) {
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

size_t ConfigMng::size() const
{
    return configMap.size();
}

void ConfigMng::clear()
{
    configMap.clear();
}

std::vector<Config> ConfigMng::configList() const
{
    std::vector<Config> list;
    for(auto && c : configMap) {
        list.push_back(c.second);
    }
    return list;
}

bool ConfigMng::parseJsonAfterLoading(Json::Value& jsonData)
{
    for (Json::Value::const_iterator it = jsonData.begin(); it != jsonData.end(); ++it) {
        Config config;
        config.load(*it);
        if (editingMode || config.isValid()) {
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
    if (config.isValid() || (editingMode && !config.command.empty())) {
        if (configMap.find(config.name) != configMap.end()) {
            std::cerr << "Warning: configuration name's " << config.name << " is repeated. Override data.\n";
        }
        configMap[config.name] = config;
        return true;
    }
    return false;
}


bool ConfigMng::loadOverrideOptions(const Json::Value& oo)
{
    if (oo.isMember("base")) {
        auto v = oo["base"];
        overrideOptionMode = v.isMember("mode") && v["mode"].asBool();
    }
    
    if (!overrideOptionMode || !oo.isMember("options")) {
        return false;
    }
    
    auto array = oo["options"];
    overrideOptions.clear();
    if (array.isArray()) {
        for (Json::Value::const_iterator it = array.begin(); it != array.end(); ++it) {
            Option option(*it);
            if (option.isValid()) {
                option.setOverrideType(true);
                overrideOptions[option.name] = option;
            }
        }
    }
    return true;
}

Option ConfigMng::checkOverrideOption(const Option& option)
{
    if (option.isOverridable()) {
        auto p = overrideOptions.find(option.name);
        if (p != overrideOptions.end() && p->second.type == option.type) {
            return p->second;
        }
    }
    return option;
}

Option ConfigMng::getOverrideOption(const std::string& name)
{
    auto p = overrideOptions.find(name);
    return p == overrideOptions.end() ? Option() : p->second;
}
