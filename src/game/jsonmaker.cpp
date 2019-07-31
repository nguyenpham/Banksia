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

#include <algorithm>
#include <assert.h>
#include <fstream>
#include <sstream>

#include "jsonmaker.h"

using namespace banksia;

static const std::string jsonTourString =
"{\n"
"    \"base\" :\n"
"    {\n"
"        \"concurrency\" : 2,\n"
"        \"event\" : \"Computer event\",\n"
"        \"games per pair\" : 2,\n"
"        \"swap pair sides\" : true,\n"
"        \"guide\" : \"type: roundrobin, knockout, swiss; event, site for PGN tags; shuffle: random players for roundrobin or swiss\",\n"
"        \"ponder\" : false,\n"
"        \"resumable\" : true,\n"
"        \"shuffle players\" : false,\n"
"        \"site\" : \"Somewhere on Earth\",\n"
"        \"swiss rounds\" : 6,\n"
"        \"type\" : \"roundrobin\"\n"
"    },\n"
"    \"engine configurations\" :\n"
"    {\n"
"        \"path\" : \"\",\n"
"        \"update\" : false\n"
"    },\n"
"    \"inclusive players\" :\n"
"    {\n"
"        \"guide\" : \"matches are counted with players in this list only; side: white, black, any\",\n"
"        \"mode\" : false,\n"
"        \"players\" : [ ],\n"
"        \"side\" : \"black\"\n"
"    },\n"
"    \"logs\" :\n"
"    {\n"
"        \"engine\" :\n"
"        {\n"
"            \"game title surfix\" : true,\n"
"            \"guide\" : \"one file: if false, games are stored in multi files using game indexes as surfix; game title surfix: use players names, results for file name surfix, affective only when 'one file' is false; separate by sides: each side has different logs\",\n"
"            \"mode\" : true,\n"
"            \"one file\" : false,\n"
"            \"path\" : \"logengine.txt\",\n"
"            \"separate by sides\" : false,\n"
"            \"show time\" : true\n"
"        },\n"
"        \"pgn\" :\n"
"        {\n"
"            \"game title surfix\" : true,\n"
"            \"guide\" : \"one file: if false, games are stored in multi files using game indexes as surfix; game title surfix: use players names, results for file name surfix, affective only when 'one file' is false; rich info: log more info such as scores, depths, elapses\",\n"
"            \"mode\" : true,\n"
"            \"one file\" : true,\n"
"            \"path\" : \"games.pgn\",\n"
"            \"rich info\" : false\n"
"        },\n"
"        \"result\" :\n"
"        {\n"
"            \"mode\" : true,\n"
"            \"path\" : \"logresult.txt\"\n"
"        }\n"
"    },\n"
"    \"openings\" :\n"
"    {\n"
"        \"base\" :\n"
"        {\n"
"            \"allone fen\" : \"\",\n"
"            \"allone san moves\" : \"\",\n"
"            \"guide\" : \"seed for random, -1 completely random; select types: samepair: same opening for a pair, allnew: all games use different openings, allone: all games use one opening, from 'allone fen' or 'allone san moves' or books\",\n"
"            \"seed\" : -1,\n"
"            \"select type\" : \"allnew\"\n"
"        },\n"
"        \"books\" :\n"
"        [\n"
"            {\n"
"                \"mode\" : false,\n"
"                \"path\" : \"\",\n"
"                \"type\" : \"epd\"\n"
"            },\n"
"            {\n"
"                \"mode\" : false,\n"
"                \"path\" : \"\",\n"
"                \"type\" : \"pgn\"\n"
"            },\n"
"            {\n"
"                \"guide\" : \"maxply: ply to play; top100: percents of top moves (for a given position) to select ranndomly an opening move, 0 is always the best\",\n"
"                \"maxply\" : 12,\n"
"                \"mode\" : false,\n"
"                \"path\" : \"\",\n"
"                \"top100\" : 20,\n"
"                \"type\" : \"polyglot\"\n"
"            }\n"
"        ]\n"
"    },\n"
"    \"endgames\" : {\n"
"        \"guide\" : \"syzygypath used for both 'override options' and 'game adjudication'\",\n"
"        \"syzygypath\" : \"\"\n"
"    },\n"
"    \"game adjudication\" :\n"
"    {\n"
"        \"mode\" : true,\n"
"        \"guide\" : \"finish and adjudicate result; set game length zero to turn it off; tablebase path is from endgames\",\n"
"        \"draw if game length over\" : 500,\n"
"        \"tablebase max pieces\" : 7,\n"
"        \"tablebase\" : true\n"
"    },\n"
"    \"override options\" :\n"
"    {\n"
"        \"base\" :\n"
"        {\n"
"            \"guide\" : \"threads (cores), memory (hash), syzygypath (from endgames) will overwrite for any relative options (both UCI and Winboard), memory in MB, set zero/empty to disable them; options will relplace engines' options which are same names and types, 'value' is the most important, others ignored; to avoid some options from specific engines being overridden, add and set field 'overridable' to false for them\",\n"
"            \"mode\" : true,\n"
"            \"threads\" : 1,\n"
"            \"memory\" : 64\n"
"        },\n"
"        \"options\" :\n"
"        [\n"
"            {\n"
"                \"default\" : 2,\n"
"                \"max\" : 100,\n"
"                \"min\" : 1,\n"
"                \"name\" : \"SyzygyProbeDepth\",\n"
"                \"type\" : \"spin\",\n"
"                \"value\" : 1\n"
"            },\n"
"            {\n"
"                \"default\" : false,\n"
"                \"name\" : \"Syzygy50MoveRule\",\n"
"                \"type\" : \"check\",\n"
"                \"value\" : true\n"
"            },\n"
"            {\n"
"                \"default\" : 6,\n"
"                \"max\" : 7,\n"
"                \"min\" : 0,\n"
"                \"name\" : \"SyzygyProbeLimit\",\n"
"                \"type\" : \"spin\",\n"
"                \"value\" : 7\n"
"            }\n"
"        ]\n"
"    },\n"
"    \"players\" :\n"
"    [\n"
"        \"stockfish\",\n"
"        \"fruit\",\n"
"        \"crafty\",\n"
"        \"gaviota-1.0\"\n"
"    ],    \n"
"    \"time control\" :\n"
"    {\n"
"        \"guide\" : \"unit's second; time: could be a real number (e.g. 6.5 for 6.5s) or a string (e.g. '2:10:30' for 2h 20m 30s); mode: standard, infinite, depth, movetime; margin: an extra delay time before checking if time's over\",\n"
"        \"increment\" : 0.5,\n"
"        \"margin\" : 0.8,\n"
"        \"mode\" : \"standard\",\n"
"        \"moves\" : 40,\n"
"        \"time\" : 6.5\n"
"    }\n"
"}";



JsonMaker::JsonMaker()
{
}

bool JsonMaker::isValid() const
{
    return true;
}

std::string JsonMaker::toString() const
{
    std::ostringstream stringStream;
    return stringStream.str();
}

void JsonMaker::tickWork()
{
    if (state != JsonMakerState::working) {
        return;
    }
    
    tick_idle++;
    
    if (tick_idle > tick_idle_max) {
        std::cout << "Error: idle too long. Force to quit!" << std::endl;
        exit(1);
    }
    
    std::vector<JsonEngine*> removingEngineVec;
    for(auto && e : workingEngineVec) {
        e->tickWork();
        if (e->isFinished() && e->isSafeToDelete()) {
            removingEngineVec.push_back(e);
        }
    }
    
    for(auto && e : removingEngineVec) {
        auto it = std::find(workingEngineVec.begin(), workingEngineVec.end(), e);
        if (it == workingEngineVec.end()) {
            std::cout << "Error: cannot delete the engine: " << e->getName() << std::endl;
        } else {
            workingEngineVec.erase(it);
        }
        delete e;
    }
    
    // kick start one each time, not all at one
    if (!configVec.empty() && workingEngineVec.size() < concurrency) {
        tick_idle = 0;
        auto config = configVec.back();
        configVec.pop_back();
        
        auto jsonEngine = new JsonEngine(config);
        workingEngineVec.push_back(jsonEngine);
        
        jsonEngine->kickStart([=](Config* rConfig) {
            tick_idle = 0;
            
            if (rConfig) {
                if (rConfig->name.empty() || rConfig->name.find("<<<") != std::string::npos) {
                    if (!rConfig->idName.empty()) {
                        rConfig->name = rConfig->idName;
                    } else {
                        rConfig->name = getFileName(rConfig->command);
                    }
                }
                
                std::cout << "OK, an engine detected: " << rConfig->name << ", " << nameFromProtocol(rConfig->protocol) << std::endl;
                goodConfigVec.push_back(*rConfig);
            } else {
                std::cout << "  not an engine: " << config.command << std::endl;
            }
        });
    }
    
    // Completed
    if (configVec.empty() && workingEngineVec.empty()) {
        completed();
    }
}

//auto jsonPath = "/Users/nguyenpham/workspace/BanksiaMatch/test.json";

void JsonMaker::completed()
{
    state = JsonMakerState::done;
    
    std::cout << "All engines / executable files are checked, finishing! Total engines: " << goodConfigVec.size() << std::endl;
    
    std::sort(goodConfigVec.begin(), goodConfigVec.end(), [](const Config& l, const Config& r)
              {
                  auto lname = l.name, rname = r.name;
                  toLower(lname); toLower(rname);
                  return lname.compare(rname) < 0;
              });
    
    ConfigMng::instance->clear();
    for(auto && c : goodConfigVec) {
        ConfigMng::instance->insert(c);
    }
    
    ConfigMng::instance->setJsonPath(jsonEngineConfigPath);
    ConfigMng::instance->saveToJsonFile();
    
    // update tour json
    {
        Json::Value jsonData;
        JsonSavable::loadFromJsonFile(jsonTourMngPath, jsonData, false);
        
        Json::Value sample;
        JsonSavable::loadFromJsonString(jsonTourString, sample, true);

		auto curPath = currentWorkingFolder() + folderSlash;
		auto logs = sample["logs"];
		logs["engine"]["path"] = curPath + logs["engine"]["path"].asString();
		logs["pgn"]["path"] = curPath + logs["pgn"]["path"].asString();
		logs["result"]["path"] = curPath + logs["result"]["path"].asString();
		sample["logs"] = logs;

        Jsonable::merge(jsonData, sample, JsonMerge::fillmissing);

        auto s = "engine configurations";
        Json::Value v;
        if (jsonData.isMember(s)) {
            v = jsonData[s];
        }
        v["path"] = jsonEngineConfigPath;
        if (!v.isMember("update")) {
            v["update"] = false;
        }
        jsonData[s] = v;
        
        s = "players";
        //if (!jsonData.isMember(s))
        {
            Json::Value v;
			std::string preName;
            for(auto && c : goodConfigVec) {
				if (preName == c.name) continue;
                v.append(c.name);
				preName = c.name;
            }
            jsonData[s] = v;
        }
        
        JsonSavable::saveToJsonFile(jsonTourMngPath, jsonData);
    }
    
    auto elapsed_secs = static_cast<int>(time(nullptr) - startTime);
    std::cout << "Before playing, please add/edit Opening book paths, syzygy path and other information\n";
    std::cout << "All done!!! Elapsed: " << formatPeriod(elapsed_secs) << std::endl;
    
    std::cout << "\nTo play, enter:\n"
    << getAppName() << " -jsonpath " << jsonTourMngPath << std::endl;
    
    // WARNING: exit the app here after completed the maker
    exit(0);
}

void JsonMaker::shutdown()
{
    timer.remove(mainTimerId);
}

void JsonMaker::build(const std::string& mainJsonPath, const std::string& mainEnginesPath, int _concurrency)
{
    startTime = time(nullptr);
    
    auto currentFolder = currentWorkingFolder();
    
    concurrency = _concurrency;
    motherEngineFolder = mainEnginesPath; // "/Users/nguyenpham/workspace/BanksiaMatch";
    
    // Load tour json
    jsonTourMngPath = mainJsonPath.empty() ?  currentFolder + folderSlash + defaultJsonTourName : mainJsonPath;
    Json::Value jsonData;
    JsonSavable::loadFromJsonFile(jsonTourMngPath, jsonData, false);
    
    auto s = "engine configurations";
    if (jsonData.isMember(s)) {
        auto v = jsonData[s];
        jsonEngineConfigPath = v["path"].asString();
    }
    
    if (jsonEngineConfigPath.empty()) {
        jsonEngineConfigPath = currentFolder + folderSlash + defaultJsonEngineName;
    }
    
    std::cout << "Generating JSON files!" << std::endl;
    std::cout << " main engines folder: " << mainEnginesPath << std::endl;
    std::cout << " engine configurations JSON file: " << jsonEngineConfigPath << std::endl;
    std::cout << " tournament JSON file: " << jsonTourMngPath << std::endl;
    
    // Scale engines
    std::set<std::string> pathSet;
    
    ConfigMng::instance->setEditingMode(true);
    ConfigMng::instance->loadFromJsonFile(jsonEngineConfigPath, false);
    
    for(auto && config : ConfigMng::instance->configList()) {
        if (config.command.empty() || pathSet.find(config.command) != pathSet.end()) {
            continue;
        }
        configVec.push_back(config);
        pathSet.insert(config.command);
    }
    
    if (!motherEngineFolder.empty()) {
        Config config;
        config.protocol = Protocol::none;
        auto allPaths = listExcecutablePaths(motherEngineFolder);
        for(auto && path : allPaths) {
            if (path.empty() || pathSet.find(path) != pathSet.end())
                continue;
            config.command = path;
            config.workingFolder = getFolder(path);
            configVec.push_back(config);
        }
    }
    
    std::cout << " executable file number: " << configVec.size() << ", concurrency: " << concurrency << std::endl << std::endl;
    
    state = JsonMakerState::working;
    
    mainTimerId = timer.add(std::chrono::milliseconds(500), [=](CppTime::timer_id) { tick(); }, std::chrono::milliseconds(500));
}

#ifdef _WIN32

bool JsonMaker::isRunable(const std::string& path)
{
    return isExecutable(path);
}

#else

static const std::set<std::string>extSet {
    "txt", "pdf", "ini", "db", "mak", "def", "prj", "sln", "dat",
    "htm", "html", "xml", "json", "doc", "docx", "rtf", "md", "md5", "log", "bk",
    "jpg", "jpeg", "gif", "png", "bmp", "ico", "rc", "rb",
    "zip", "7z", "rar", "arj", "gz", "tgz",
    "bok", "pgn", "lrn", "epd",
    "rtbw", "rtbz", "cp4", "atbw", "atbz", "emd", "cmp",
    "h", "hpp", "c", "cpp", "cc", "java", "bas", "o", "obj",
    "bat", "bin", "exe", "dll"
};

static const std::set<std::string>exclusiveFileNameSet {
    "makefile", "readme", "license",
};

bool JsonMaker::isRunable(const std::string& path)
{
    if (!isExecutable(path)) return false;
    
    auto p = path.rfind(".");
    if (p != std::string::npos && path.size() - p <= 5) {
        auto extString = path.substr(p + 1);
        toLower(extString);
        if (extSet.find(extString) != extSet.end()) {
            return false;
        }
    }
    
    auto fileName = getFileName(path);
    toLower(fileName);
    return exclusiveFileNameSet.find(fileName) == exclusiveFileNameSet.end();
}

#endif

std::vector<std::string> JsonMaker::listExcecutablePaths(const std::string& dirname)
{
    
    auto fullpath = getFullPath(dirname.c_str());
    auto vec = listdir(fullpath);
    
    std::vector<std::string> v;
    for(auto && path : vec) {
        if (isRunable(path)) {
            v.push_back(path);
        }
    }
    return v;
}


