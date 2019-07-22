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


#include <sstream>
#include <algorithm>
#include <iomanip> // for setprecision

#ifdef _WIN32

#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>

#else

#endif

#include "engineprofile.h"

using namespace banksia;

std::string Profile::toString() const
{
    std::ostringstream stringStream;
    
    stringStream
    << std::right << std::setw(4) << cpuTotal * 100 / std::max(1ULL, cpuTime)
    << std::right << std::setw(8) << int(memTotal / (std::max(1ULL, memCall) * 1024 * 1024))
    << std::right << std::setw(8) << int(memMax / (1024 * 1024))
    << std::right << std::setw(4) << int(threadTotal /  std::max(1ULL, threadCall)) << threadMax;
    
    return stringStream.str();
}

void Profile::addFrom(const Profile& o)
{
    cpuTotal += o.cpuTotal;
    cpuTime += o.cpuTime;
    memTotal += o.memTotal;
    memCall += o.memCall;
    threadTotal += o.threadTotal;
    threadCall += o.threadCall;
    memMax = std::max(memMax, o.memMax);
    threadMax = std::max(threadMax, o.threadMax);
}

//////////////////////////////////////////
EngineProfile::EngineProfile()
: Engine()
{
}
EngineProfile::EngineProfile(const Config& config)
: Engine(config)
{
}

EngineProfile::~EngineProfile()
{
}

void EngineProfile::setProfileMode(bool mode)
{
    profileMode = mode;
}

#ifdef _WIN32

void EngineProfile::resetProfile()
{
    ZeroMemory(&m_ftPrevSysKernel, sizeof(FILETIME));
    ZeroMemory(&m_ftPrevSysUser, sizeof(FILETIME));
    ZeroMemory(&m_ftPrevProcKernel, sizeof(FILETIME));
    ZeroMemory(&m_ftPrevProcUser, sizeof(FILETIME));
}


u64 subtractTimes(const FILETIME& ftA, const MYFILETIME& ftB)
{
    LARGE_INTEGER a, b;
    a.LowPart = ftA.dwLowDateTime;
    a.HighPart = ftA.dwHighDateTime;
    
    b.LowPart = ftB.dwLowDateTime;
    b.HighPart = ftB.dwHighDateTime;
    
    return a.QuadPart - b.QuadPart;
}

void EngineProfile::tickWork()
{
    Engine::tickWork();
    
    if (!profileMode || state == PlayerState::stopped || processId == 0) {
        return;
    }
    
    tickCnt++;
    
    {
        // then get a process list snapshot.
        HANDLE const  snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPALL, 0);
        
        // initialize the process entry structure.
        PROCESSENTRY32 entry = { 0 };
        entry.dwSize = sizeof(entry);
        
        // get the first process info.
        BOOL  ret = true;
        ret = Process32First(snapshot, &entry);
        while (ret && entry.th32ProcessID != processId) {
            ret = Process32Next(snapshot, &entry);
        }
        CloseHandle(snapshot);
        if (ret) {
            threadCnt += entry.cntThreads;
            threadCall++;
            if (threadMax < entry.cntThreads)
                threadMax = entry.cntThreads;
        }
    }
    
    
    /////
    
    auto hProcess = OpenProcess(  PROCESS_QUERY_INFORMATION |
                                PROCESS_VM_READ,
                                FALSE, processId);
    if (nullptr == hProcess)
        return;
    
    if ((tickCnt & 0x1) == 0) {
        PROCESS_MEMORY_COUNTERS_EX pmc;
        GetProcessMemoryInfo(hProcess, (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));
        memUsage += pmc.PrivateUsage;
        memCall++;
    }
    
    FILETIME ftSysIdle, ftSysKernel, ftSysUser;
    FILETIME ftProcCreation, ftProcExit, ftProcKernel, ftProcUser;
    
    if (!GetSystemTimes(&ftSysIdle, &ftSysKernel, &ftSysUser) ||
        !GetProcessTimes(hProcess, &ftProcCreation, &ftProcExit, &ftProcKernel, &ftProcUser))
    {
        return;
    }
    
    if (m_ftPrevSysKernel.dwLowDateTime)
    {
        /*
         CPU usage is calculated by getting the total amount of time the system has operated
         since the last measurement (made up of kernel + user) and the total
         amount of time the process has run (kernel + user).
         */
        auto ftSysKernelDiff = subtractTimes(ftSysKernel, m_ftPrevSysKernel);
        auto ftSysUserDiff = subtractTimes(ftSysUser, m_ftPrevSysUser);
        
        auto ftProcKernelDiff = subtractTimes(ftProcKernel, m_ftPrevProcKernel);
        auto ftProcUserDiff = subtractTimes(ftProcUser, m_ftPrevProcUser);
        
        cpuTime +=  ftSysKernelDiff + ftSysUserDiff;
        cpuUsage += ftProcKernelDiff + ftProcUserDiff;
    }
    
    m_ftPrevSysKernel = *(MYFILETIME*)&ftSysKernel;
    m_ftPrevSysUser = *(MYFILETIME*)&ftSysUser;
    m_ftPrevProcKernel = *(MYFILETIME*)&ftProcKernel;
    m_ftPrevProcUser = *(MYFILETIME*)&ftProcUser;
    
    CloseHandle( hProcess );
}


#endif

