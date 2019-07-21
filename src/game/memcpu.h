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
#include "engine.h"

//#if (defined _WIN32)
//#include <windows.h>
//#endif

typedef struct {
	unsigned int dwLowDateTime;
	unsigned int dwHighDateTime;
} MYFILETIME;


namespace banksia {

    class Profile {
    public:
        u64 cpuTotal = 0, cpuTime = 0;
        u64 memTotal = 0, memCall = 0;
        u64 threadTotal = 0, threadCall = 0;
        int memMax = 0, threadMax = 0;
        
        static std::string memSizeString(u64 memSize);
        std::string toString() const;
        void reset() {
            memset(this, 0, sizeof(Profile));
        }
        
        bool isEmpty() const {
            return cpuTime == 0;
        }
         
        void addFrom(const Profile& o);
    };
    
    class EngineProfile : public Engine
    {
    public:
        EngineProfile();
        EngineProfile(const Config& config);

        virtual ~EngineProfile();
        
        void setProfileMode(bool mode);
        virtual void tickWork() override;

        Profile profile;

    private:
//#ifdef _WIN32
        void resetProfile();

        //system total times
		MYFILETIME m_ftPrevSysKernel;
		MYFILETIME m_ftPrevSysUser;
        
        //process times
		MYFILETIME m_ftPrevProcKernel;
		MYFILETIME m_ftPrevProcUser;
//#endif
        
	private:
        bool profileMode = false;
        i64 tickCnt = 0;
    };
    
} // namespace banksia


#endif /* memcpu_h */

