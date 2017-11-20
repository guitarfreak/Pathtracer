/*
=================================================================================

	ToDo:

	Done Today: 

	Bugs:

=================================================================================
*/


#pragma optimize( "", on )


// External.

#define WIN32_LEAN_AND_MEAN 
#define NOMINMAX
#include <windows.h>
#include <gl\gl.h>
// #include "external\glext.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#include "external\stb_image.h"

#define STB_RECT_PACK_IMPLEMENTATION
#include "external\stb_rect_pack.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "external\stb_truetype.h"


struct ThreadQueue;
struct GraphicsState;
struct MemoryBlock;
struct DebugState;
struct Timer;
ThreadQueue* globalThreadQueue;
GraphicsState* globalGraphicsState;
MemoryBlock* globalMemory;
Timer* globalTimer;

// Internal.


// #ifndef RELEASE_BUILD
// 	#define TIMER_BLOCKS_ENABLED
// #endif 

#undef TIMER_BLOCKS_ENABLED



#include "rt_types.cpp"
#include "rt_timer.cpp"
#include "rt_misc.cpp"
#include "rt_math.cpp"
#include "rt_hotload.cpp"
#include "rt_misc_win32.cpp"
#include "rt_platformWin32.cpp"

#include "memory.cpp"
#include "openglDefines.cpp"
#include "userSettings.cpp"
#include "rendering.cpp"


#include "debug.cpp"




#define Window_Min_Size_X 300
#define Window_Min_Size_Y 400

#define App_Font_Folder "Fonts\\"
#define Windows_Font_Folder "\\Fonts\\"
#define Windows_Font_Path_Variable "windir"

#define App_Save_File ".\\temp"

#define Fallback_Font "arial.ttf"
#define Fallback_Font_Italic "ariali.ttf"
#define Fallback_Font_Bold "arialbd.ttf"







struct AppTempSettings {
	Rect windowRect;
};

void appWriteTempSettings(char* filePath, AppTempSettings* at) {
	writeDataToFile((char*)at, sizeof(AppTempSettings), filePath);
}

void appReadTempSettings(char* filePath, AppTempSettings* at) {
	readDataFile((char*)at, filePath);
}

void appTempDefault(AppTempSettings* at, Rect monitor) {
	Rect r = monitor;
	Vec2 center = vec2(rectCenX(r), (r.top - r.bottom)/2);
	Vec2 dim = vec2(rectW(r), -rectH(r));
	at->windowRect = rectCenDim(center, dim*0.85f);
}



struct AppData {

	// General.

	SystemData systemData;
	Input input;
	WindowSettings wSettings;
	GraphicsState graphicsState;

	i64 lastTimeStamp;
	f64 dt;
	f64 time;
	int frameCount;
	i64 swapTime;

	bool updateFrameBuffers;
	int msaaSamples;

	// Window.

	Rect clientRect;
	Vec2i frameBufferSize;

	// App.

	Font* font;
};




#pragma optimize( "", off )
// #pragma optimize( "", on )

extern "C" APPMAINFUNCTION(appMain) {

	i64 startupTimer = timerInit();

	if(init) {

		// Init memory.

		SYSTEM_INFO info;
		GetSystemInfo(&info);

		ExtendibleMemoryArray* pMemory = &appMemory->extendibleMemoryArrays[appMemory->extendibleMemoryArrayCount++];
		initExtendibleMemoryArray(pMemory, megaBytes(2), info.dwAllocationGranularity, 0);

		MemoryArray* tMemory = &appMemory->memoryArrays[appMemory->memoryArrayCount++];
		initMemoryArray(tMemory, megaBytes(2), 0);
	}

	// Setup memory and globals.

	MemoryBlock gMemory = {};
	gMemory.pMemory = &appMemory->extendibleMemoryArrays[0];
	gMemory.tMemory = &appMemory->memoryArrays[0];
	globalMemory = &gMemory;

	AppData* ad = (AppData*)getBaseExtendibleMemoryArray(gMemory.pMemory);
	GraphicsState* gs = &ad->graphicsState;

	Input* input = &ad->input;
	SystemData* systemData = &ad->systemData;
	HWND windowHandle = systemData->windowHandle;
	WindowSettings* ws = &ad->wSettings;

	globalThreadQueue = threadQueue;
	globalGraphicsState = &ad->graphicsState;

	// Init.

	if(init) {

		// @AppInit.

		//
		// AppData.
		//

		TIMER_BLOCK_BEGIN_NAMED(initAppData, "Init AppData");

		getPMemory(sizeof(AppData));
		*ad = {};

		// int windowStyle = (WS_POPUP | WS_BORDER);
		// int windowStyle = (WS_POPUP | WS_BORDER);
		int windowStyle = WS_OVERLAPPEDWINDOW;
		initSystem(systemData, ws, windowsData, vec2i(1920*0.85f, 1080*0.85f), windowStyle, 1);

		windowHandle = systemData->windowHandle;

		printf("%Opengl Version: %s\n", (char*)glGetString(GL_VERSION));

		loadFunctions();

		const char* extensions = wglGetExtensionsStringEXT();

		wglSwapIntervalEXT(1);
		int fps = wglGetSwapIntervalEXT();

		initInput(&ad->input);

		TIMER_BLOCK_END(initAppData);


		//
		// Setup Textures.
		//

		TIMER_BLOCK_BEGIN_NAMED(initGraphics, "Init Graphics");

		for(int i = 0; i < TEXTURE_SIZE; i++) {
			Texture tex;
			uchar buffer [] = {255,255,255,255 ,255,255,255,255 ,255,255,255,255, 255,255,255,255};
			loadTexture(&tex, buffer, 2,2, 1, INTERNAL_TEXTURE_FORMAT, GL_RGBA, GL_UNSIGNED_BYTE);

			addTexture(tex);
		}

		// for(int i = 0; i < TEXTURE_SIZE; i++) {
		// 	Texture tex;
		// 	loadTextureFromFile(&tex, texturePaths[i], -1, INTERNAL_TEXTURE_FORMAT, GL_RGBA, GL_UNSIGNED_BYTE);
		// 	addTexture(tex);
		// }

		//
		// Setup Meshs.
		//

		uint vao = 0;
		glCreateVertexArrays(1, &vao);
		glBindVertexArray(vao);

		// 
		// Samplers.
		//

		gs->samplers[SAMPLER_NORMAL] = createSampler(16.0f, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR);
		gs->samplers[SAMPLER_NEAREST] = createSampler(16.0f, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_NEAREST, GL_NEAREST);

		//
		//
		//

		ad->msaaSamples = 4;
		ad->dt = 1/(float)60;

		ad->graphicsState.zOrder = 0;

		//
		// FrameBuffers.
		//

		{
			for(int i = 0; i < FRAMEBUFFER_SIZE; i++) {
				FrameBuffer* fb = getFrameBuffer(i);
				initFrameBuffer(fb);
			}

			attachToFrameBuffer(FRAMEBUFFER_2dMsaa, FRAMEBUFFER_SLOT_COLOR, GL_RGBA16F, 0, 0, ad->msaaSamples);
			attachToFrameBuffer(FRAMEBUFFER_2dMsaa, FRAMEBUFFER_SLOT_DEPTH, GL_DEPTH_COMPONENT32F, 0, 0, ad->msaaSamples);
			attachToFrameBuffer(FRAMEBUFFER_2dNoMsaa, FRAMEBUFFER_SLOT_COLOR, GL_RGBA16F, 0, 0);

			attachToFrameBuffer(FRAMEBUFFER_DebugMsaa, FRAMEBUFFER_SLOT_COLOR, GL_RGBA8, 0, 0, ad->msaaSamples);
			attachToFrameBuffer(FRAMEBUFFER_DebugNoMsaa, FRAMEBUFFER_SLOT_COLOR, GL_RGBA8, 0, 0);

		// attachToFrameBuffer(FRAMEBUFFER_ScreenShot, FRAMEBUFFER_SLOT_COLOR, GL_SRGB8_ALPHA8, 0, 0);
			attachToFrameBuffer(FRAMEBUFFER_ScreenShot, FRAMEBUFFER_SLOT_COLOR, GL_SRGB8, 0, 0);

			ad->updateFrameBuffers = true;



		// ad->frameBufferSize = vec2i(2560, 1440);
			ad->frameBufferSize = ws->biggestMonitorSize;
			Vec2i fRes = ad->frameBufferSize;

			setDimForFrameBufferAttachmentsAndUpdate(FRAMEBUFFER_2dMsaa, fRes.w, fRes.h);
			setDimForFrameBufferAttachmentsAndUpdate(FRAMEBUFFER_2dNoMsaa, fRes.w, fRes.h);
			setDimForFrameBufferAttachmentsAndUpdate(FRAMEBUFFER_DebugMsaa, fRes.w, fRes.h);
			setDimForFrameBufferAttachmentsAndUpdate(FRAMEBUFFER_DebugNoMsaa, fRes.w, fRes.h);

		// setDimForFrameBufferAttachmentsAndUpdate(FRAMEBUFFER_ScreenShot, fRes.w, fRes.h);
		}

		TIMER_BLOCK_END(initGraphics);

	//

		TIMER_BLOCK_BEGIN_NAMED(initRest, "Init Rest");




		globalGraphicsState->fontFolders[globalGraphicsState->fontFolderCount++] = getPStringCpy(App_Font_Folder);
		char* windowsFontFolder = fillString("%s%s", getenv(Windows_Font_Path_Variable), Windows_Font_Folder);
		globalGraphicsState->fontFolders[globalGraphicsState->fontFolderCount++] = getPStringCpy(windowsFontFolder);

		globalGraphicsState->fallbackFont = getPStringCpy(Fallback_Font);
		globalGraphicsState->fallbackFontBold = getPStringCpy(Fallback_Font_Bold);
		globalGraphicsState->fallbackFontItalic = getPStringCpy(Fallback_Font_Italic);

		// Setup app temp settings.
		{
			if(!fileExists(App_Save_File)) {
				AppTempSettings at = {};
				appTempDefault(&at, ws->monitors[0].workRect);

				appWriteTempSettings(App_Save_File, &at);
			}

			AppTempSettings at = {};
			{
				appReadTempSettings(App_Save_File, &at);

				Rect r = at.windowRect;
				MoveWindow(windowHandle, r.left, r.top, r.right-r.left, r.bottom-r.top, true);
			}
		}


	}



	// @AppStart.

	TIMER_BLOCK_BEGIN_NAMED(reload, "Reload");

	if(reload) {
		loadFunctions();
		SetWindowLongPtr(systemData->windowHandle, GWLP_WNDPROC, (LONG_PTR)mainWindowCallBack);

		gs->screenRes = ws->currentRes;
	}

	TIMER_BLOCK_END(reload);

	// Update timer.
	{
		if(init) {
			ad->lastTimeStamp = timerInit();
			ad->dt = 1/(float)60;
		} else {
			ad->dt = timerUpdate(ad->lastTimeStamp, &ad->lastTimeStamp);
			ad->time += ad->dt;
		}
	}

	clearTMemory();

	//
	// Update input.
	//

	{
		TIMER_BLOCK_NAMED("Input");

		updateInput(&ad->input, windowHandle);

		if(ad->input.closeWindow) *isRunning = false;

		ad->frameCount++;

		// ws->customCursor = true;
		// updateCursor(ws);
	}


	UpdateWindow(systemData->windowHandle);

	if(input->keysPressed[KEYCODE_ESCAPE]) {
		if(ws->fullscreen) {
			if(input->keysPressed[KEYCODE_ESCAPE]) setWindowMode(windowHandle, ws, WINDOW_MODE_WINDOWED);
		} else {
			// *isRunning = false;
		}
	}

	if(input->keysPressed[KEYCODE_F11]) {
		if(ws->fullscreen) setWindowMode(windowHandle, ws, WINDOW_MODE_WINDOWED);
		else setWindowMode(windowHandle, ws, WINDOW_MODE_FULLBORDERLESS);
	}


	if(windowSizeChanged(windowHandle, ws)) {
		if(!windowIsMinimized(windowHandle)) {
			TIMER_BLOCK_NAMED("UpdateWindowRes");

			updateResolution(windowHandle, ws);
			ad->updateFrameBuffers = true;
		}
	}

	if(ad->updateFrameBuffers) {
		TIMER_BLOCK_NAMED("Upd FBOs");

		ad->updateFrameBuffers = false;
		gs->screenRes = ws->currentRes;
	}




	openglDebug();
	openglDefaultSetup();
	openglClearFrameBuffers();



	// @AppLoop.

	{
		glLoadIdentity();
		Vec2i res = ws->currentRes;
		glViewport(0,0, res.w, res.h);
		glOrtho(0, res.w, -res.h, 0, -10,10);

		bindFrameBuffer(FRAMEBUFFER_2dMsaa);
	}


	{
		Rect sr = getScreenRect(ws);

		// dfsdgserzg

		float size = 10;
		Vec2 pos = rectTR(sr) - vec2(100 + size, 100 + size);
		int eCount = 100;
		float tailLength = 10;
		float time = ad->time / 40;
		float dist = 100;
		float patternMod = 8;
		glPointSize(size);

		for(int i = eCount-1; i >= 0; i--) {
			f64 t = time - (i*tailLength/eCount);
			float d = sin(t*patternMod) * dist;
			Vec2 p = pos + vec2((sin(t)*d), cos(t)*d);

			float percent = (float)i/(eCount-1);
			// percent = sqrt(percent);
			Vec4 c = vec4(1-percent,0,percent/2,1-percent);

			// drawRect(rectCenDim(p, vec2(20)), c);
			drawPoint(p, c);
		}

		glPointSize(1);
	}





	openglDrawFrameBufferAndSwap(ws, systemData, &ad->swapTime, init);


	// Save app state.
	#if 1
	if(*isRunning == false && fileExists(App_Save_File)) {
		AppTempSettings at = {};

		RECT r; 
		GetWindowRect(windowHandle, &r);

		at.windowRect = rect(r.left, r.bottom, r.right, r.top);

		appWriteTempSettings(App_Save_File, &at);
	}
	#endif
	
	// if(init) printf("Startup Time: %fs\n", timerUpdate(startupTimer));

	// @App End.
}

#pragma optimize( "", on ) 
