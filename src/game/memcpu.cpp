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

#ifdef _WIN32

    #include <windows.h>
    #include <psapi.h>

#else

#endif

#include "memcpu.h"

using namespace banksia;

MemCpu::~MemCpu()
{
}

void MemCpu::init(int pid)
{
    processId = pid;
}

#ifdef _WIN32

MemCpu::MemCpu()
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

void MemCpu::tickUpdate()
{
    if (processId == 0) return;
	tickCnt++;
    
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



#else


MemCpu::MemCpu()
{
}

void MemCpu::tickUpdate()
{
}

#endif
