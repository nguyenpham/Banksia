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

#include <csignal>

#include "game/jsonmaker.h"
#include "game/uciengine.h"
#include "game/playermng.h"
#include "game/tourmng.h"

void show_usage(std::string name);
void show_help();


int main(int argc, const char * argv[])
{
#if defined(_MSC_VER)
    // to make speed of printing to screen to be much faster when the app compiled by MS Visual Studio
    setvbuf(stdout, 0, _IOLBF, 4096);
#endif
    
    // SIGPIPE 13      write on a pipe with no one to read it
#define SIGPIPE     13
    signal(SIGPIPE, SIG_IGN);
    
    std::cout << "Banksia, Chess Tournament Manager, by Nguyen Pham - version " << banksia::getVersion() << std::endl;
    
    if (argc < 2) {
        show_usage(banksia::getAppName());
        return 1;
    }
    
    std::map <std::string, std::string> argmap;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg.empty() || arg.at(0) != '-' || arg == "-h" || arg == "--help") {
            show_usage(banksia::getAppName());
            return 0;
        }
        std::string str = arg;
        auto ok = true;
        
        if (arg == "-jsonpath" || arg == "-d" || arg == "-c") {
            if (i + 1 < argc) {
                i++;
                str = argv[i];
            } else {
                ok = false;
            }
        }
        
        if (!ok || str.empty()) {
            std::cerr << arg << " requires one argument." << std::endl;
            return -1;
        }
        argmap[arg] = str;
    }
    
    std::string mainJsonPath;
    if (argmap.find("-jsonpath") != argmap.end()) {
        mainJsonPath = argmap["-jsonpath"];
    }
    
    banksia::JsonMaker maker;
    banksia::TourMng tourMng;
    
    if (argmap.find("-u") != argmap.end()) {
        std::string mainEnginesPath;
        if (argmap.find("-d") != argmap.end()) {
            mainEnginesPath = argmap["-d"];
        }
        
        int concurrency = 2;
        if (argmap.find("-c") != argmap.end()) {
            concurrency = std::atoi(argmap["-c"].c_str());
        }
        
        // The app will be terminated when all jobs done
        maker.build(mainJsonPath, mainEnginesPath, concurrency);
    } else {
        if (!tourMng.loadFromJsonFile(mainJsonPath)) {
            return -1;
        }
        
        if (!tourMng.createMatchList()) {
            return -1;
        }
        
        // The app will be terminated when all matches completed
        tourMng.startTournament();
    }
    
    while (true) {
        std::string line;
        std::getline(std::cin, line);
        banksia::trim(line);
        
        if (line.empty()) {
            continue;
        }
        
        auto vec = banksia::splitString(line, ' ');
        if (vec.empty()) {
            continue;
        }
        
        auto cmd = vec.front();
        
        if (cmd == "help") {
            show_help();
            continue;
        }
        
        if (cmd == "status") {
            std::cout << tourMng.createTournamentStats() << std::endl;
            continue;
        }
        if (cmd == "engineverbose" || cmd == "ev") {
            std::string str = vec.size() > 1 ? vec[1] : "";
            tourMng.showEgineInOutToScreen(str == "on");
            continue;
        }
        
        if (cmd == "quit") {
            break;
        }
        
        std::cout << line << ": unknown command!\n";
        show_help();
    }
    
    tourMng.shutdown();
    maker.shutdown();
    
    return 0;
}


void show_usage(std::string name)
{
    std::cout << "Usage: " << name << " <option>\n"
    << "Options:\n"
    << "  -h               Show this help message\n"
    << "  -jsonpath PATH   Main json path to manage the tournament\n"
    << "  -u               update\n"
    << "  -c               concurrency (for updating only)\n"
    << "  -d PATH          main engines' folder, may have subfolder (for updating only)\n"
    << "\n"
    << "Examples:\n"
    << "  " << name << " -jsonpath c:\\tour.json\n"
    << "  " << name << " -u -d c:\\mainenginefolder\n"
    << "  " << name << " -u -c 4 -jsonpath c:\\tour.json -d c:\\mainenginefolder\n"
    << "  To update tour.json and engines.json in current folder:\n"
    << "  " << name << " -u -c 4\n"
    << std::endl;
}

void show_help()
{
    std::cout
    << "Usage:\n"
    << "  help                    show this help message\n"
    << "  status                  current result\n"
    << "  ev [on|off]             Show/not to screen engine's input / output (engineverbose)\n"
    << "  quit                    quit\n"
    << std::endl;
}


