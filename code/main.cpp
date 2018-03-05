#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include "rt_misc.cpp"
#include "rt_hotload.cpp"
#include "rt_misc_win32.cpp"

#include "external\iacaMarks.h"

#ifdef SHIPPING_MODE
#define OPEN_CONSOLE 0
#else 
#define OPEN_CONSOLE 1
#endif


int CALLBACK WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR commandLine, int showCode) {

	if(OPEN_CONSOLE) {
		AllocConsole();
		freopen("conin$","r",stdin);
		freopen("conout$","w",stdout);
		freopen("conout$","w",stderr);
	}

	HotloadDll hotloadDll;

	#ifndef SHIPPING_MODE	
	initDll(&hotloadDll, "app.dll", "appTemp.dll", "lock.tmp");
	#else 
	initDll(&hotloadDll, "app.dll", "appTemp.dll", "lock.tmp", false);
	#endif 

	WindowsData wData = windowsData(instance, prevInstance, commandLine, showCode);

	SYSTEM_INFO sysinfo;
	GetSystemInfo(&sysinfo);
	int coreCount = sysinfo.dwNumberOfProcessors;

	ThreadQueue threadQueue;
	threadInit(&threadQueue, coreCount-1);

	AppMemory appMemory = {};

    bool firstFrame = true;
    bool secondFrame = false;
    bool isRunning = true;
    while(isRunning) {

    	bool reload = false;
		
		#ifndef RELEASE_BUILD	
		if(threadQueueFinished(&threadQueue)) reload = updateDll(&hotloadDll);
     	#endif 

     	platform_appMain = (appMainType*)getDllFunction(&hotloadDll, "appMain");
        platform_appMain(firstFrame, reload, &isRunning, wData, &threadQueue, &appMemory);

        if(firstFrame) firstFrame = false;
    }

	return 0;
}