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

#include <algorithm>
#include <assert.h>
#include <fstream>
#include <sstream>

// for scaning files from a given path
#ifdef _WIN32

#include <windows.h>
#include <direct.h>

#else

#include <glob.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#endif


#include "jsonmaker.h"

using namespace banksia;

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
                std::cout << "not an engine: " << config.command << std::endl;
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
        TourMng::fixJson(jsonData, currentWorkingFolder());
        
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
        if (!jsonData.isMember(s)) {
            Json::Value v;
            
            for(auto && c : goodConfigVec) {
                v.append(c.name);
            }
            jsonData[s] = v;
        }
        
        JsonSavable::saveToJsonFile(jsonTourMngPath, jsonData);
    }
    
    
    auto elapsed_secs = static_cast<int>(time(nullptr) - startTime);
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
static void findFiles(std::vector<std::string>& names, const std::string& dirname) {
    std::string search_path = dirname + "/*.*";
    
    WIN32_FIND_DATAA file;
    //        WIN32_FIND_DATA file;
    HANDLE search_handle = FindFirstFileA(search_path.c_str(), &file);
    if (search_handle) {
        do {
            std::string fullpath = dirname + "\\" + file.cFileName;
            if ((file.dwFileAttributes | FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY) {
                if (file.cFileName[0] != '.')
                    findFiles(names, fullpath);
            } else {
                names.push_back(fullpath);
            }
        } while (FindNextFileA(search_handle, &file));
        ::FindClose(search_handle);
    }
}

std::vector<std::string> JsonMaker::listdir(std::string dirname) {
    std::vector<std::string> pathVec;
    findFiles(pathVec, dirname);
    return pathVec;
}

i64 JsonMaker::getFileSize(const std::string& path)
{
    // Good to compile with Visual C++
    struct __stat64 fileStat;
    int err = _stat64(path.c_str(), &fileStat );
    if (0 != err) return 0;
    return fileStat.st_size;
}

bool JsonMaker::isExecutable(const std::string& path)
{
    return path.find(".exe") != std::string::npos;
}


#else

std::vector<std::string> JsonMaker::listdir(std::string dirname) {
    DIR* d_fh;
    struct dirent* entry;
    
    std::vector<std::string> vec;
    
    while( (d_fh = opendir(dirname.c_str())) == NULL) {
        //        std::cerr << "Couldn't open directory: %s\n", dirname.c_str());
        return vec;
    }
    
    dirname += "/";
    
    while ((entry=readdir(d_fh)) != NULL) {
        
        // Don't descend up the tree or include the current directory
        if(strncmp(entry->d_name, "..", 2) != 0 &&
           strncmp(entry->d_name, ".", 1) != 0) {
            
            // If it's a directory print it's name and recurse into it
            if (entry->d_type == DT_DIR) {
                auto vec2 = listdir(dirname + entry->d_name);
                vec.insert(vec.end(), vec2.begin(), vec2.end());
            }
            else {
                auto s = dirname + entry->d_name;
                vec.push_back(s);
            }
        }
    }
    
    closedir(d_fh);
    return vec;
}

i64 JsonMaker::getFileSize(const std::string& fileName)
{
    struct stat st;
    if(stat(fileName.c_str(), &st) != 0) {
        return 0;
    }
    return st.st_size;
}

static const std::set<std::string>extSet{
    "txt", "pdf", "ini", "db", "mak", "def", "prj", "dat",
    "htm", "html", "xml", "json", "doc", "docx", "md", "md5", "log",
    "jpg", "jpeg", "gif", "png", "bmp", "rc",
    "zip", "7z", "rar", "arj", "gz", "tgz",
    "bok", "pgn", "lrn",
    "rtbw", "rtbz", "cp4", "atbw", "atbz", "emd", "cmp",
    "h", "hpp", "c", "cpp", "java", "py", "bas",
    "bat", "bin", "exe", "dll"
};

bool JsonMaker::isExecutable(const std::string& path)
{
    if (access(path.c_str(), X_OK)) return false;
    
    auto p = path.rfind(".");
    if (p != std::string::npos && path.size() - p <= 4) {
        auto extString = path.substr(p + 1);
        toLower(extString);
        return extSet.find(extString) == extSet.end();
    }
    return true;
}

#endif

std::vector<std::string> JsonMaker::listExcecutablePaths(const std::string& dirname)
{
    std::vector<std::string> v;
    auto vec = listdir(dirname);
    
    for(auto && path : vec) {
        if (isExecutable(path)) {
            v.push_back(path);
        }
    }
    return v;
}

std::string JsonMaker::currentWorkingFolder()
{
    char buf[1024];
    
#ifdef _WIN32
    auto r = _getcwd(buf, sizeof(buf));
#else
    auto r = getcwd(buf, sizeof(buf));
#endif
    
    if (!r) {
        buf[0] = 0;
    }
    return buf;
}

