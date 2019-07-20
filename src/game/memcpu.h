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


#ifndef memcpu_h
#define memcpu_h

#include <stdio.h>
#include "../base/comm.h"

//#if (defined _WIN32)
//
//#include <windows.h>
//
//#endif

typedef struct _MYFILETIME {
	unsigned int dwLowDateTime;
	unsigned int dwHighDateTime;
} MYFILETIME;


namespace banksia {

    class MemCpu
    {
    public:
        MemCpu();
        virtual ~MemCpu();
        
        void tickUpdate();
        void init(int pid);

    private:
#ifdef _WIN32
        //system total times
		MYFILETIME m_ftPrevSysKernel;
		MYFILETIME m_ftPrevSysUser;
        
        //process times
		MYFILETIME m_ftPrevProcKernel;
		MYFILETIME m_ftPrevProcUser;
#endif
        
	protected:
		u64 cpuUsage = 0, cpuTime = 0, memUsage = 0, memCall = 0, threadCnt = 0, threadCall = 0;
		int threadMax = 0;

	private:
        i64 tickCnt = 0;
        int processId = 0;
    };
    
} // namespace banksia


#endif /* memcpu_h */

