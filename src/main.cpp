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

#include <csignal>

#include "game/jsonmaker.h"
#include "game/tourmng.h"

#include "3rdparty/fathom/tbprobe.h"

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
    
//    {
//        std::cout << "getNumberOfCores: " << banksia::getNumberOfCores() << ", getMemorySize: " << banksia::getMemorySize() << std::endl;
//
//        banksia::TimeController tc;
//        tc.setup(banksia::TimeControlMode::standard, 40, 60.02, 0.5, 0.5);
//        auto v = tc.saveToJson();
//        banksia::Jsonable::printOut(v, " ");
//
//        banksia::JsonSavable::saveToJsonFile("/Users/nguyenpham/workspace/BanksiaMatch/logs/t.txt", v);
//    }
//    {
//        Tablebase::SyzygyTablebase::tb_init("/Users/nguyenpham/workspace/BanksiaMatch/syzygy345:/Users/nguyenpham/workspace/BanksiaMatch/syzygy6");
//        banksia::ChessBoard board;
//        board.newGame("8/4R3/8/1k6/8/2Q5/1P4P1/7K w - - 0 1");
//        board.printOut();
//
//        auto r = board.probeSyzygy();
//        std::cout << r.toShortString() << std::endl;
//    }
    
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
        
        if (arg == "-t" || arg == "-jsonpath" || arg == "-d" || arg == "-c" || arg == "-v") {
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
    if (argmap.find("-t") != argmap.end()) {
        mainJsonPath = argmap["-t"];
    } else if (argmap.find("-jsonpath") != argmap.end()) {
        mainJsonPath = argmap["-jsonpath"];
    }

    if (argmap.find("-v") != argmap.end()) {
        banksia::banksiaVerbose = argmap["-v"] == "on";
    }
    if (argmap.find("-profile") != argmap.end()) {
#ifdef _WIN32
        banksia::profileMode = true;
        std::cout << "Warning: profile mode is on." << std::endl;
#else
        std::cout << "Sorry: profile has just been implemented for Windows only." << std::endl;
#endif
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
        
        // The app will be auto terminated when all jobs done
        maker.build(mainJsonPath, mainEnginesPath, concurrency);
    } else {
        
        if (mainJsonPath.empty()) {
            std::cerr << "Error: jsonpath is empty." << std::endl;
            return -1;
        }
        
        auto noReply = argmap.find("-no") != argmap.end();
        auto yesReply = argmap.find("-yes") != argmap.end();
        
        // The app will be auto terminated when all matches completed
        if (!tourMng.start(mainJsonPath, yesReply, noReply)) {
            return -1;
        }
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
        if (cmd == "v") {
            std::string str = vec.size() > 1 ? vec[1] : "";
            //tourMng.showEgineInOutToScreen(str == "on");
            banksia::banksiaVerbose = str == "on";
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
    << "  -h           Show this help message\n"
    << "  -t PATH      A path to json tour file and is also used to run/manage tournament/matches. Example:\n"
    << "               banksia -t c:\\t5.json, to run a tournament whose conditions are specified in t5.json file.\n"
    << "  -yes         A flag to auto answer yes when being ask (to resume a tournament). Example:\n"
    << "               banksia -yes -t c:\\t5.json, to resume the tournament that was stopped before,\n"
    << "               without waiting for typing y/n.\n"
    << "  -no          A flag to auto answer no when being ask (to resume a tournament).\n"
    << "  -u           A flag to create/update engines and tournament json files. Example:\n"
    << "               banksia -u -d c:\\myengines, to create/update engines.json file and tour.json file, where\n"
    << "               engines are located in c:\\myengines. engines.json and tour.json files will be located on\n"
    << "               the folder where banksia.exe is.\n"
    << "  -c VALUE     Concurrency, it is used to execute a task faster for updating only. Examples:\n"
    << "               banksia -u -c 4, to update tour.json and engines.json files.\n"
    << "               banksia -u -c 4 -d c:\\myengines, to create and update engines.json file based on the engines\n"
    << "               found in c:\\myengines.\n"
    << "  -d PATH      PATH is the location of the engines and may contain subfolders. It is also used to\n"
    << "               create engines.json and tour.json files. Example:\n"
    << "               banksia -u -d c:\\myengines, will create engines.json and tour.json files at the folder where\n"
    << "               banksia.exe is located. banksia will search the engines located in c:\\myengines in this case.\n"
    << "  -v on|off    turn on/off verbose (default on)\n"
    
#ifdef _WIN32
    << "  -profile     profile engines (cpu, mem, threads)\n"
#endif
    << "\n\n"
    << "FAQ:\n"
    << "    Q1. How to automatically create engines.json and tour.json files?\n"
    << "    A1. banksia -u -d c:\\chess\\engines\n"
    << "where:\n"
    << "    c:\\chess\\engines is the path of your engines and engines.json file will be\n"
    << "    created in the same folder where banksia.exe is.\n"
    << "    That command line also creates a tour.json file that can be edited and used to\n"
    << "    run a tournament.\n"
    << "\n"
    << "    Q2. How to run a tournament?\n"
    << "    A2. Create a tournament file say tour.json then type,\n"
    << "    banksia -t c:\\banksia\\tour.json\n"

    << std::endl;
}

void show_help()
{
    std::cout
    << "Usage:\n"
    << "  help                    show this help message\n"
    << "  status                  current result\n"
    << "  v [on|off]              verbose on/off. Show/Don't show individual match (default on)\n"
    << "  quit                    quit\n"
    << std::endl;
}



