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


#include <cstdarg>
#include <regex>
#include <fstream>
#include <iomanip> // for setfill, setw

// for scaning files from a given path
#ifdef _WIN32

#include <windows.h>
#include <direct.h>
#include <stdlib.h> // for full path

#include <tlhelp32.h> // for isRunning

#else

#include <glob.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#include <sys/types.h>
#include <signal.h>

#ifdef MACOS
#include <sys/param.h>
#include <sys/sysctl.h>
#endif

#if defined(BSD)
#include <sys/sysctl.h>
#endif

#endif


#include "comm.h"

namespace banksia {
    
    const char* BANKSIA_VERSION = "3.1.5";
    bool banksiaVerbose = true;
    bool profileMode = false;

    extern const char* pieceTypeName;
    extern const char* reasonStrings[12];
    extern const char* resultStrings[5];
    extern const char* sideStrings[4];
    extern const char* shortSideStrings[4];

    const char* pieceTypeName = ".kqrbnp";
    const char* reasonStrings[] = {
        "*", "mate", "stalemate", "repetition", "resign", "fifty moves", "insufficient material", "illegal move", "timeout", "adjudication", "crash", nullptr
    };
    
    // noresult, win, draw, loss
    const char* resultStrings[] = {
        "*", "1-0", "1/2-1/2", "0-1", nullptr
    };

    const char* sideStrings[] = {
        "black", "white", "none", nullptr
    };
    const char* shortSideStrings[] = {
        "b", "w", "n", nullptr
    };

    static std::mutex consoleMutex;
    
    std::string resultType2String(ResultType type) {
        auto t = static_cast<int>(type);
        if (t < 0 || t > 3) t = 0;
        return resultStrings[t];
    }
    
    ResultType string2ResultType(const std::string& s) {
        for(int i = 0; resultStrings[i]; i++) {
            if (resultStrings[i] == s) {
                return static_cast<ResultType>(i);
            }
        }
        return ResultType::noresult;
    }
    
    std::string reasonType2String(ReasonType type) {
        auto t = static_cast<int>(type);
        if (t < 0) t = 0;
        return reasonStrings[t];
    }
    
    ReasonType string2ReasonType(const std::string& s) {
        for(int i = 0; reasonStrings[i]; i++) {
            if (reasonStrings[i] == s) {
                return static_cast<ReasonType>(i);
            }
        }
        return ReasonType::noreason;
    }
    
    std::string side2String(Side side, bool shortFrom)
    {
        auto sd = static_cast<int>(side);
        if (sd < 0 || sd > 1) sd = 2;
        return shortFrom ? shortSideStrings[sd] : sideStrings[sd];
    }
    
    Side string2Side(std::string s)
    {
        toLower(s);
        for(int i = 0; sideStrings[i]; i++) {
            if (sideStrings[i] == s || shortSideStrings[i] == s) {
                return static_cast<Side>(i);
            }
        }
        return Side::none;

    }

    void printText(const std::string& str)
    {
        std::lock_guard<std::mutex> dolock(consoleMutex);
        std::cout << str << std::endl;
    }
    
    void toLower(std::string& str) {
        for (size_t i = 0; i < str.size(); ++i) {
            str[i] = tolower(str[i]);
        }
    }
    
    void toLower(char* str) {
        for (size_t i = 0; str[i]; ++i) {
            str[i] = tolower(str[i]);
        }
    }
    
    std::string posToCoordinateString(int pos) {
        int row = pos / 8, col = pos % 8;
        std::ostringstream stringStream;
        stringStream << char('a' + col) << 8 - row;
        return stringStream.str();
    }
    
    int coordinateStringToPos(const char* str) {
        auto colChr = str[0], rowChr = str[1];
        if (colChr >= 'a' && colChr <= 'h' && rowChr >= '1' && rowChr <= '8') {
            int col = colChr - 'a';
            int row = rowChr - '1';
            
            return (7 - row) * 8 + col;
        }
        return -1;
    }
    
    std::string getFileName(const std::string& path) {
        auto pos = path.find_last_of("/\\");
        std::string str = pos != std::string::npos ? path.substr(pos + 1) : path;
        pos = str.find_last_of(".");
        if (pos != std::string::npos) {
            str = str.substr(0, pos);
        }
        return str;
    }

    std::string getFolder(const std::string& path) {
        auto pos = path.find_last_of("/\\");
        std::string str = pos != std::string::npos ? path.substr(0, pos) : "";
        return str;
    }

    std::string getVersion() {
        return BANKSIA_VERSION;
    }
    
    std::string getAppName() {
        return "banksia";
    }
    
    static const char* trimChars = " \t\n\r\f\v";
    
    // trim from left
    std::string& ltrim(std::string& s)
    {
        s.erase(0, s.find_first_not_of(trimChars));
        return s;
    }
    
    // trim from right
    std::string& rtrim(std::string& s)
    {
        s.erase(s.find_last_not_of(trimChars) + 1);
        return s;
    }
    
    // trim from left & right
    std::string& trim(std::string& s)
    {
        return ltrim(rtrim(s));
    }
    
    std::string replaceString(std::string subject, const std::string& search, const std::string& replace) {
        size_t pos = 0;
        while((pos = subject.find(search, pos)) != std::string::npos) {
            subject.replace(pos, search.length(), replace);
            pos += replace.length();
        }
        return subject;
    }
    std::vector<std::string> splitString(const std::string& string, const std::string& regexString)
    {
        std::regex re(regexString);
        std::sregex_token_iterator first{ string.begin(), string.end(), re }, last;
        return { first, last };
    }
    
    std::vector<std::string> splitString(const std::string &s, char delim)
    {
        std::vector<std::string> elems;
        std::stringstream ss;
        ss.str(s);
        std::string item;
        while (std::getline(ss, item, delim)) {
            auto s = trim(item);
            if (!s.empty())
                elems.push_back(s);
        }
        return elems;
    }
    
    std::vector<std::string> readTextFileToArray(const std::string& path)
    {
        std::ifstream inFile;
        inFile.open(path);
        
        std::string line;
        std::vector<std::string> vec;
        while (getline(inFile, line)) {
            vec.push_back(line);
        }
        return vec;
    }
    
    std::string formatPeriod(int seconds)
    {
        int s = seconds % 60, minutes = seconds / 60, m = minutes % 60, hours = minutes / 60, h = hours % 24, d = hours / 24;
        
        std::ostringstream stringStream;
        if (d > 0) {
            stringStream << d << "d ";
        }
        if (h > 0) {
            stringStream << h << ":"
            << std::setfill('0') << std::setw(2);
        }
        
        stringStream
        << m << ":"
        << std::setfill('0') << std::setw(2)
        << s
        << std::setfill(' ') << std::setw(0);
        return stringStream.str();
    }
    
    // https://stackoverflow.com/questions/38034033/c-localtime-this-function-or-variable-may-be-unsafe
    std::tm localtime_xp(std::time_t timer)
    {
        std::tm bt{};
#if defined(__unix__)
        localtime_r(&timer, &bt);
#elif defined(_MSC_VER)
        localtime_s(&bt, &timer);
#else
        static std::mutex mtx;
        std::lock_guard<std::mutex> lock(mtx);
        bt = *std::localtime(&timer);
#endif
        return bt;
    }
    

    std::string currentWorkingFolder()
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
    
    std::string getFullPath(const char* path)
    {
#ifdef _WIN32
        char buff[_MAX_PATH];
        if (_fullpath( buff, path, _MAX_PATH) != NULL ) {
            return buff;
        }
        return path;
        
#else
        char buff[PATH_MAX];
        char *home, *ptr;
        if (*path == '~' && (home = getenv("HOME"))) {
            char s[PATH_MAX];
            ptr = realpath(strcat(strcpy(s, home), path+1), buff);
        } else {
            ptr = realpath(path, buff);
        }
        return ptr ? ptr : path;
#endif
    }
    
    
#ifdef _WIN32
    void findFiles(std::vector<std::string>& names, const std::string& dirname) {
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
    
    std::vector<std::string> listdir(std::string dirname) {
        std::vector<std::string> pathVec;
        findFiles(pathVec, dirname);
        return pathVec;
    }
    
    i64 getFileSize(const std::string& path)
    {
        // Good to compile with Visual C++
        struct __stat64 fileStat;
        int err = _stat64(path.c_str(), &fileStat );
        if (0 != err) return 0;
        return fileStat.st_size;
    }
    
    bool isExecutable(const std::string& path)
    {
        return path.find(".exe") != std::string::npos || path.find(".bat") != std::string::npos;
    }

    bool isRunning(int pid)
    {
        HANDLE pss = CreateToolhelp32Snapshot(TH32CS_SNAPALL, 0);
        
        PROCESSENTRY32 pe = { 0 };
        pe.dwSize = sizeof(pe);
        
        if (Process32First(pss, &pe))
        {
            do
            {
                // pe.szExeFile can also be useful
                if (pe.th32ProcessID == pid)
                    return true;
            }
            while(Process32Next(pss, &pe));
        }
        
        CloseHandle(pss);
        
        return false;
    }

#else
    
    std::vector<std::string> listdir(std::string dirname) {
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
    
    i64 getFileSize(const std::string& fileName)
    {
        struct stat st;
        if(stat(fileName.c_str(), &st) != 0) {
            return 0;
        }
        return st.st_size;
    }

    bool isExecutable(const std::string& path)
    {
        return !access(path.c_str(), X_OK);
    }
    
    bool isRunning(int pid)
    {
        return 0 == kill(pid, 0);
    }
    
#endif

    
    int getNumberOfCores() {
#ifdef WIN32
		SYSTEM_INFO sysinfo;
		GetSystemInfo(&sysinfo);
		return sysinfo.dwNumberOfProcessors;
#elif MACOS
        int nm[2];
        size_t len = 4;
        uint32_t count;
     
        nm[0] = CTL_HW; nm[1] = HW_AVAILCPU;
        sysctl(nm, 2, &count, &len, NULL, 0);
     
        if(count < 1) {
            nm[1] = HW_NCPU;
            sysctl(nm, 2, &count, &len, NULL, 0);
            if(count < 1) { count = 1; }
            }
        return count;
#else
        return int(sysconf(_SC_NPROCESSORS_ONLN));
#endif
    }

    
    /**
     * http://nadeausoftware.com/articles/2012/09/c_c_tip_how_get_physical_memory_size_system
     * Returns the size of physical memory (RAM) in bytes.
     */
    size_t getMemorySize()
    {
#if defined(_WIN32) && (defined(__CYGWIN__) || defined(__CYGWIN32__))
        /* Cygwin under Windows. ------------------------------------ */
        /* New 64-bit MEMORYSTATUSEX isn't available.  Use old 32.bit */
        MEMORYSTATUS status;
        status.dwLength = sizeof(status);
        GlobalMemoryStatus( &status );
        return (size_t)status.dwTotalPhys;
        
#elif defined(_WIN32)
        /* Windows. ------------------------------------------------- */
        /* Use new 64-bit MEMORYSTATUSEX, not old 32-bit MEMORYSTATUS */
        MEMORYSTATUSEX status;
        status.dwLength = sizeof(status);
        GlobalMemoryStatusEx( &status );
        return (size_t)status.ullTotalPhys;
        
#elif defined(__unix__) || defined(__unix) || defined(unix) || (defined(__APPLE__) && defined(__MACH__))
        /* UNIX variants. ------------------------------------------- */
        /* Prefer sysctl() over sysconf() except sysctl() HW_REALMEM and HW_PHYSMEM */
        
#if defined(CTL_HW) && (defined(HW_MEMSIZE) || defined(HW_PHYSMEM64))
        int mib[2];
        mib[0] = CTL_HW;
#if defined(HW_MEMSIZE)
        mib[1] = HW_MEMSIZE;            /* OSX. --------------------- */
#elif defined(HW_PHYSMEM64)
        mib[1] = HW_PHYSMEM64;          /* NetBSD, OpenBSD. --------- */
#endif
        int64_t size = 0;               /* 64-bit */
        size_t len = sizeof( size );
        if ( sysctl( mib, 2, &size, &len, NULL, 0 ) == 0 )
            return (size_t)size;
        return 0L;            /* Failed? */
        
#elif defined(_SC_AIX_REALMEM)
        /* AIX. ----------------------------------------------------- */
        return (size_t)sysconf( _SC_AIX_REALMEM ) * (size_t)1024L;
        
#elif defined(_SC_PHYS_PAGES) && defined(_SC_PAGESIZE)
        /* FreeBSD, Linux, OpenBSD, and Solaris. -------------------- */
        return (size_t)sysconf( _SC_PHYS_PAGES ) *
        (size_t)sysconf( _SC_PAGESIZE );
        
#elif defined(_SC_PHYS_PAGES) && defined(_SC_PAGE_SIZE)
        /* Legacy. -------------------------------------------------- */
        return (size_t)sysconf( _SC_PHYS_PAGES ) *
        (size_t)sysconf( _SC_PAGE_SIZE );
        
#elif defined(CTL_HW) && (defined(HW_PHYSMEM) || defined(HW_REALMEM))
        /* DragonFly BSD, FreeBSD, NetBSD, OpenBSD, and OSX. -------- */
        int mib[2];
        mib[0] = CTL_HW;
#if defined(HW_REALMEM)
        mib[1] = HW_REALMEM;        /* FreeBSD. ----------------- */
#elif defined(HW_PYSMEM)
        mib[1] = HW_PHYSMEM;        /* Others. ------------------ */
#endif
        unsigned int size = 0;        /* 32-bit */
        size_t len = sizeof( size );
        if ( sysctl( mib, 2, &size, &len, NULL, 0 ) == 0 )
            return (size_t)size;
        return 0L;            /* Failed? */
#endif /* sysctl and sysconf variants */
        
#else
        return 0L;            /* Unknown OS. */
#endif
    }
}


////////////////////////
using namespace banksia;

void Obj::printOut(const char* msg) const
{
    if (msg) {
        std::cout << msg << std::endl;
    }
    std::cout << toString() << std::endl;
}

void Jsonable::printOut(const Json::Value& obj, std::string prefix)
{
    if (obj.isString()) {
        std::cout << "\"" << obj.asString() << "\"";
    } else if (obj.isInt()) {
        std::cout << obj.asInt();
    } else if (obj.isDouble()) {
        std::cout << obj.asDouble();
    } else if (obj.isBool()) {
        std::cout << (obj.asBool() ? "true" : "false");
    } else if (obj.isArray()) {
        std::cout << "[\n";
        for (Json::Value::const_iterator it = obj.begin(); it != obj.end(); ++it) {
            printOut(*it, prefix + "  ");
            std::cout << ",\n";
        }
        std::cout << prefix << "\n],\n";
    } else {
        std::cout << prefix << "{\n";
        auto names = obj.getMemberNames();
        for(auto && name : names) {
            std::cout << prefix << "\"" << name << "\" : ";
            auto v = obj[name];
            printOut(v, prefix + "  ");
            std::cout << ",\n";
        }
        std::cout << prefix << "},\n";
    }
}

void Jsonable::merge(Json::Value& main, const Json::Value& from, JsonMerge merge)
{
    auto names = from.getMemberNames();
    for(auto && name : names) {
        if (merge == JsonMerge::overwrite || !main.isMember(name)) {
            main[name] = from[name];
        }
    }
}

bool JsonSavable::loadFromJsonFile(const std::string& jsonPath, bool verbose)
{
    try {
        Json::Value obj;
        return JsonSavable::_loadFromJsonFile(jsonPath, obj, verbose) && parseJsonAfterLoading(obj);
    } catch (Json::Exception const& e) {
        if (verbose)
            std::cerr << "Error: Exception when parsing json file - " << e.what() << std::endl;
    }
    return false;
}

bool JsonSavable::_loadFromJsonFile(const std::string& path, Json::Value& jsonData, bool verbose)
{
    jsonPath = path;
    return loadFromJsonFile(path, jsonData, verbose);
}

bool JsonSavable::loadFromJsonFile(const std::string& path, Json::Value& jsonData, bool verbose)
{
    std::ifstream ifs(path);
    
    Json::CharReaderBuilder jsonReader;
    std::string errorString;
    if (Json::parseFromStream(jsonReader, ifs, &jsonData, &errorString)) {
        return true;
    }
    if (verbose) {
        std::cerr << "Error: cannot load (or broken) json file " << path << ", error: " << errorString << std::endl;
    }
    return false;
}

bool JsonSavable::saveToJsonFile()
{
    Json::Value jsonData = createJsonForSaving();
    return saveToJsonFile(jsonPath, jsonData);
}

bool JsonSavable::saveToJsonFile(Json::Value& jsonData)
{
    return saveToJsonFile(jsonPath, jsonData);
}

bool JsonSavable::saveToJsonFile(const std::string& path, Json::Value& jsonData)
{
    auto r = false;
    std::ofstream outFile;
    outFile.open(path, std::ofstream::trunc);
    if (outFile.is_open())
    {
        Json::StreamWriterBuilder builder;
        builder["indentation"] = "  ";
        builder["precision"] = 1;
        builder["precisionType"] = "decimal";
        std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
        writer->write(jsonData, &outFile);
        r = true;
    }
    outFile.close();
    return r;
}

std::string JsonSavable::getJsonPath() const
{
    return jsonPath;
}

void JsonSavable::setJsonPath(const std::string& _jsonPath)
{
    jsonPath = _jsonPath;
}

bool JsonSavable::loadFromJsonString(const std::string& string, Json::Value& jsonData, bool verbose)
{
    std::stringstream ss;
    ss.str (string);

    Json::CharReaderBuilder jsonReader;
    std::string errorString;
    if (Json::parseFromStream(jsonReader, ss, &jsonData, &errorString)) {
        return true;
    }
    if (verbose) {
        std::cerr << "Error: cannot load (or broken) json file " << string << ", error: " << errorString << std::endl;
    }
    return false;
}


