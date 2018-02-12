#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include "rt_misc.cpp"
#include "rt_hotload.cpp"
#include "rt_misc_win32.cpp"

#define Libcurl_Dll_File "External\\libcurl.dll"

#include <iacaMarks.h>
#include "rt_math.cpp"
// #include "raycast.cpp"

#define OPEN_CONSOLE 1

int CALLBACK WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR commandLine, int showCode) {

	HotloadDll hotloadDll;

	#ifndef RELEASE_BUILD	
	initDll(&hotloadDll, "app.dll", "appTemp.dll", "lock.tmp");
	#else 
	initDll(&hotloadDll, "app.dll", "appTemp.dll", "lock.tmp", false);
	#endif 

	WindowsData wData = windowsData(instance, prevInstance, commandLine, showCode);

	ThreadQueue threadQueue;
	threadInit(&threadQueue, 7);

	AppMemory appMemory = {};

	if(OPEN_CONSOLE) {
		AllocConsole();
		freopen("conin$","r",stdin);
		freopen("conout$","w",stdout);
		freopen("conout$","w",stderr);
	}

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