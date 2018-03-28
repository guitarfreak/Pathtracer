/*
=================================================================================

	ToDo:
	- Clean global light and multiple lights.
	- Spacial data structure to speed up processing.
	- Converge method on sampling for speed up.
	- Float not good enough.
	- More advanced lighting function.
	- Have cam independent, mini window.

	- Check out setTimer for pitfalls.
	- Torus for rotation widget.

	- Clean up of whole code folder. Make it somewhat presentable, remove unused things.
	- Turning while dragging is glitchy.
	- Title buttons don't scale nicely.
	- Redraw menu buttons to look better at smaller sizes.
	- Clean up repetitive gui code. (Layout.)
	- App hangs when calculating blueNoise samples.
	- Simd.

	Bugs:
	- Windows key slow often.
	- Memory leak? Flashing when drawing scene in opengl.
	- Saving sometimes crashes. Hard to debug...
	- If framerate to low we generate too many timer messages and the mouse locks up.
	- If multiple objects selected and hold ctrl+v and let mouse go it doesn't set inactive.

=================================================================================
*/


// External.

#define WIN32_LEAN_AND_MEAN 
#define NOMINMAX
#include <windows.h>
#include "Commdlg.h"

#include <gl\gl.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "external\stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "external\stb_image_write.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_PARAMETER_TAGS_H
#include FT_MODULE_H


struct ThreadQueue;
struct GraphicsState;
struct MemoryBlock;
ThreadQueue* globalThreadQueue;
GraphicsState* globalGraphicsState;
MemoryBlock* globalMemory;

// Internal.

#include "rt_types.cpp"
#include "rt_misc.cpp"
#include "rt_math.cpp"
#include "rt_hotload.cpp"
#include "rt_misc_win32.cpp"
#include "rt_platformWin32.cpp"

#include "memory.cpp"
#include "openglDefines.cpp"
#include "userSettings.cpp"
#include "rendering.cpp"
#include "newGui.cpp"

#include "raycast.cpp"




struct AppColors {
	Vec4 text = vec4(1,1);
	Vec4 button = vec4(0.42f,1);
	Vec4 background = vec4(0.33f,1);
	Vec4 menu = vec4(0.27f,1);
	Vec4 edit = vec4(0.23f,1);
	Vec4 border = vec4(0.21f,1);
	Vec4 outline = vec4(0.19f,1);
	Vec4 outlineBright = vec4(0.41f,1);
	Vec4 ledgeDark = vec4(0.16f,1);
	Vec4 ledgeBright = vec4(0.42f,1);
};

struct AppData {

	// General.

	SystemData systemData;
	Input input;
	WindowSettings wSettings;
	GraphicsState graphicsState;

	Timer frameTimer;
	i64 lastTimeStamp;
	f64 dt;
	f64 time;
	int frameCount;

	Timer swapTimer;

	f64 fpsTime;
	int fpsCounter;
	float avgFps;

	bool updateFrameBuffers;
	int msaaSamples; 

	// Window.

	Rect clientRect;
	Vec2i frameBufferSize;

	bool captureMouse;
	bool captureMouseKeepCenter;
	bool fpsMode;

	float mouseSpeed;

	// App.

	AppColors colors;

	char* fontFile;
	char* fontFileBold;
	char* fontFileItalic;
	int fontHeight;
	float fontScale;
	int menuFontHeight;

	NewGui gui;
	float panelHeightLeft;
	float panelHeightRight;
	float oldPanelHeightRight;

	float panelWidthLeft;
	float panelWidthRight;

	float panelAlpha;
	float panelAlphaFadeState;

	float menuHeight;

	Rect menuRect;
	Rect panelLeftRect;
	Rect panelRightRect;
	Rect panelColorPickerRect;

	bool showPanels;
	float hidePanelAnim;
	bool panelsVisible;

	// 

	bool sceneHasFile;
	char* sceneFile;
	int sceneFileMax;

	DialogData dialogData;

	World world;
	EntityUI entityUI;
	RaytraceSettings settings;
	Vec3* buffer;
	Texture raycastTexture;
	Rect textureScreenRect;
	Rect textureScreenRectFitted;

	bool fitToScreen;
	bool drawSceneWired;

	bool startProcessing;
	bool activeProcessing;
	int threadCount;
	ProcessPixelsData threadData[THREAD_JOB_MAX];

	bool abortProcessing;
	bool tracerFinished;
  
	f64 processStartTime;
	f64 processTime;
};



#ifdef FULL_OPTIMIZE
#pragma optimize( "", on )
#else 
#pragma optimize( "", off )
#endif

extern "C" APPMAINFUNCTION(appMain) {

	// i64 startupTimer = timerInit();

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

		//
		// AppData.
		//

		getPMemory(sizeof(AppData));
		*ad = {};

		int windowStyle = WS_OVERLAPPEDWINDOW & ~WS_SYSMENU;
		initSystem(systemData, ws, windowsData, vec2i(1920*0.85f, 1080*0.85f), windowStyle, 1);

		windowHandle = systemData->windowHandle;

		loadFunctions();

		// Ask for specific opengl version.
		{
		    GLint attribs[] = {
		        WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
		        WGL_CONTEXT_MINOR_VERSION_ARB, 3,
		        // WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
		        WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
		        0
		    };

		    HGLRC compatibilityContext = wglCreateContextAttribsARB(systemData->deviceContext, 0, attribs);
		    if (compatibilityContext && wglMakeCurrent(systemData->deviceContext, compatibilityContext)) {
		        systemData->openglContext = compatibilityContext;
		    } else {
				printf("Could not set Opengl compatibility context\n");
		    }
		}

		GLint majorVersion, minorVersion;
		glGetIntegerv(GL_MAJOR_VERSION, &majorVersion);
		glGetIntegerv(GL_MINOR_VERSION, &minorVersion);

		printf("Opengl Version: %s - %i.%i\n", (char*)glGetString(GL_VERSION), majorVersion, minorVersion);



		const char* extensions = wglGetExtensionsStringEXT();

		if(true) {
			wglSwapIntervalEXT(1);
			ws->vsync = true;
			ws->frameRate = ws->refreshRate;
		} else {
			wglSwapIntervalEXT(0);
			ws->vsync = false;
			ws->frameRate = 200;
		}
		int swapInterval = wglGetSwapIntervalEXT();

		initInput(&ad->input);
		systemData->input = &ad->input;

		systemData->minWindowDim = vec2i(200,200);
		systemData->maxWindowDim = ws->biggestMonitorSize;

		#ifndef SHIPPING_MODE
		if(!IsDebuggerPresent()) {
			makeWindowTopmost(systemData);
		}
		#endif
	
		//
		// Setup Textures.
		//

		globalGraphicsState->textureCountMax = 20;
		globalGraphicsState->textures = getPArray(Texture, globalGraphicsState->textureCountMax);
		globalGraphicsState->textureCount = 0;

		FolderSearchData fd;
		folderSearchStart(&fd, fillString("%s*", App_Image_Folder));

		while(folderSearchNextFile(&fd)) {

			if(strLen(fd.fileName) <= 2) continue; // Skip ".."

			Texture tex;
			char* filePath = fillString("%s%s", App_Image_Folder, fd.fileName);

			loadTextureFromFile(&tex, filePath, -1, INTERNAL_TEXTURE_FORMAT, GL_RGBA, GL_UNSIGNED_BYTE);

			tex.name = getPStringCpy(fd.fileName);
			addTexture(tex);
		}

		//
		// Setup Meshs.
		//

		gs->meshCountMax = 10;
		gs->meshes = getPArray(Mesh, gs->meshCountMax);
		gs->meshCount = 0;

		MeshVertex* buffer = getTArray(MeshVertex, 500);

		// Box.
		{
			int count = 0;

			for(int i = 0; i < arrayCount(boxVertices); i += 5) {
				buffer[count++] = { boxVertices[i+0], boxVertices[i+4] };
				buffer[count++] = { boxVertices[i+1], boxVertices[i+4] };
				buffer[count++] = { boxVertices[i+2], boxVertices[i+4] };
				buffer[count++] = { boxVertices[i+2], boxVertices[i+4] };
				buffer[count++] = { boxVertices[i+3], boxVertices[i+4] };
				buffer[count++] = { boxVertices[i+0], boxVertices[i+4] };
			}

			Mesh m = {};
			m.vertices = getPArray(MeshVertex, count);
			copyArray(m.vertices, buffer, MeshVertex, count);
			m.vertexCount = count;

			gs->meshes[gs->meshCount++] = m;
		}

		// Sphere.
		{
			int count = 0;
			int div = 3;

			triangleSubDivVertex(buffer, &count, vec3(0,0,1),  vec3(0,1,0),  vec3(1,0,0),  div );
			triangleSubDivVertex(buffer, &count, vec3(0,0,1),  vec3(-1,0,0), vec3(0,1,0),  div );
			triangleSubDivVertex(buffer, &count, vec3(0,0,1),  vec3(0,-1,0), vec3(-1,0,0), div );
			triangleSubDivVertex(buffer, &count, vec3(0,0,1),  vec3(1,0,0),  vec3(0,-1,0), div );
			triangleSubDivVertex(buffer, &count, vec3(0,0,-1), vec3(1,0,0),  vec3(0,1,0),  div );
			triangleSubDivVertex(buffer, &count, vec3(0,0,-1), vec3(0,1,0),  vec3(-1,0,0), div );
			triangleSubDivVertex(buffer, &count, vec3(0,0,-1), vec3(-1,0,0), vec3(0,-1,0), div );
			triangleSubDivVertex(buffer, &count, vec3(0,0,-1), vec3(0,-1,0), vec3(1,0,0),  div );

			Mesh m = {};
			m.vertices = getPArray(MeshVertex, count);
			copyArray(m.vertices, buffer, MeshVertex, count);
			m.vertexCount = count;

			gs->meshes[gs->meshCount++] = m;
		}

		// Cylinder.
		{
			int count = 0;

			Vec3 topDiskPos = vec3(0,0,0.5f);
			Vec3 bottomDiskPos = vec3(0,0,-0.5f);

			float r = 0.5f;
			int segments = 20;
			for(int i = 0; i < segments+1; i++) {
				float angle1 = (i * M_2PI/(float)segments);
				float angle2 = ((i+1) * M_2PI/(float)segments);

				Vec3 pTop1 = topDiskPos + rotateVec3(vec3(0,1,0), angle1, vec3(0,0,1)) * r;
				Vec3 pTop2 = topDiskPos + rotateVec3(vec3(0,1,0), angle2, vec3(0,0,1)) * r;
				buffer[count++] = { topDiskPos, vec3(0,0,1) };
				buffer[count++] = { pTop2, vec3(0,0,1) };
				buffer[count++] = { pTop1, vec3(0,0,1) };

				Vec3 pBottom1 = bottomDiskPos + rotateVec3(vec3(0,1,0), angle1, vec3(0,0,1)) * r;
				Vec3 pBottom2 = bottomDiskPos + rotateVec3(vec3(0,1,0), angle2, vec3(0,0,1)) * r;
				buffer[count++] = { bottomDiskPos	, vec3(0,0,-1) };
				buffer[count++] = { pBottom1, vec3(0,0,-1) };
				buffer[count++] = { pBottom2, vec3(0,0,-1) };

				buffer[count++] = { pTop1,    normVec3(vec3(pTop1.xy,0)) };
				buffer[count++] = { pTop2,    normVec3(vec3(pTop2.xy,0)) };
				buffer[count++] = { pBottom1, normVec3(vec3(pTop1.xy,0)) };
				buffer[count++] = { pTop2,    normVec3(vec3(pTop2.xy,0)) };
				buffer[count++] = { pBottom2, normVec3(vec3(pTop2.xy,0)) };
				buffer[count++] = { pBottom1, normVec3(vec3(pTop1.xy,0)) };
			}

			Mesh m = {};
			m.vertices = getPArray(MeshVertex, count);
			copyArray(m.vertices, buffer, MeshVertex, count);
			m.vertexCount = count;

			gs->meshes[gs->meshCount++] = m;
		}

		// Cone.
		{
			int count = 0;

			Vec3 topPos = vec3(0,0,0.5f);
			Vec3 bottomDiskPos = vec3(0,0,-0.5f);
			float r = 0.5f;

			int segments = 20;
			for(int i = 0; i < segments+1; i++) {
				float angle1 = (i * M_2PI/(float)segments);
				float angle2 = ((i+1) * M_2PI/(float)segments);

				Vec3 pTop = topPos;
				Vec3 pBottom1 = bottomDiskPos + rotateVec3(vec3(0,1,0), angle1, vec3(0,0,1)) * r;
				Vec3 pBottom2 = bottomDiskPos + rotateVec3(vec3(0,1,0), angle2, vec3(0,0,1)) * r;
				buffer[count++] = { bottomDiskPos, vec3(0,0,-1) };
				buffer[count++] = { pBottom1, vec3(0,0,-1) };
				buffer[count++] = { pBottom2, vec3(0,0,-1) };

				Vec3 a = rotateVec3(vec3(0,1,0), angle1, vec3(0,0,1));
				Vec3 b = rotateVec3(vec3(0,1,0), angle2, vec3(0,0,1));
				Vec3 c = rotateVec3(vec3(0,1,0), angle1 + (angle2-angle1)/2.0f, vec3(0,0,1));

				buffer[count++] = { topPos,   normVec3(vec3(c.xy,1)) };
				buffer[count++] = { pBottom2, normVec3(vec3(b.xy,1)) };
				buffer[count++] = { pBottom1, normVec3(vec3(a.xy,1)) };
			}

			Mesh m = {};
			m.vertices = getPArray(MeshVertex, count);
			copyArray(m.vertices, buffer, MeshVertex, count);
			m.vertexCount = count;

			gs->meshes[gs->meshCount++] = m;
		}

		// 
		// Samplers.
		//

		gs->samplers[SAMPLER_NORMAL] = createSampler(16.0f, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR);
		gs->samplers[SAMPLER_NEAREST] = createSampler(16.0f, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_NEAREST, GL_NEAREST);

		glBindSampler(0, gs->samplers[SAMPLER_NORMAL]);

		//
		//
		//

		ad->msaaSamples = 8;
		ad->dt = 1/(float)60;

		ad->graphicsState.zOrder = 0;
		gs->useSRGB = true;

		//
		// FrameBuffers.
		//

		{
			for(int i = 0; i < FRAMEBUFFER_SIZE; i++) {
				FrameBuffer* fb = getFrameBuffer(i);
				initFrameBuffer(fb);
			}

			int msaa = ad->msaaSamples;

			attachToFrameBuffer(FRAMEBUFFER_2dMsaa, FRAMEBUFFER_SLOT_COLOR, GL_SRGB8_ALPHA8, 0, 0, msaa);
			attachToFrameBuffer(FRAMEBUFFER_2dMsaa, FRAMEBUFFER_SLOT_DEPTH, GL_DEPTH_COMPONENT32F, 0, 0, msaa);
			attachToFrameBuffer(FRAMEBUFFER_2dNoMsaa, FRAMEBUFFER_SLOT_COLOR, GL_SRGB8_ALPHA8, 0, 0);

			attachToFrameBuffer(FRAMEBUFFER_2dPanels, FRAMEBUFFER_SLOT_COLOR, GL_SRGB8_ALPHA8, 0, 0, msaa);
			attachToFrameBuffer(FRAMEBUFFER_2dPanels, FRAMEBUFFER_SLOT_DEPTH_STENCIL, GL_DEPTH32F_STENCIL8, 0, 0, msaa);

			attachToFrameBuffer(FRAMEBUFFER_ScreenShot, FRAMEBUFFER_SLOT_COLOR, GL_SRGB8, 0, 0);

			ad->updateFrameBuffers = true;


			ad->frameBufferSize = ws->biggestMonitorSize;
			Vec2i fRes = ad->frameBufferSize;
			
			// Vec2i fRes = ws->currentRes;

			setDimForFrameBufferAttachmentsAndUpdate(FRAMEBUFFER_2dMsaa, fRes.w, fRes.h);
			setDimForFrameBufferAttachmentsAndUpdate(FRAMEBUFFER_2dNoMsaa, fRes.w, fRes.h);
			setDimForFrameBufferAttachmentsAndUpdate(FRAMEBUFFER_2dPanels, fRes.w, fRes.h);
		}




		globalGraphicsState->fontFolders[globalGraphicsState->fontFolderCount++] = getPStringCpy(App_Font_Folder);
		char* windowsFontFolder = fillString("%s%s", getenv(Windows_Font_Path_Variable), Windows_Font_Folder);
		globalGraphicsState->fontFolders[globalGraphicsState->fontFolderCount++] = getPStringCpy(windowsFontFolder);

		globalGraphicsState->fallbackFont = getPStringCpy(Fallback_Font);
		globalGraphicsState->fallbackFontBold = getPStringCpy(Fallback_Font_Bold);
		globalGraphicsState->fallbackFontItalic = getPStringCpy(Fallback_Font_Italic);

		// Setup app temp settings.
		AppSessionSettings appSessionSettings = {};
		{
			// @AppSessionDefaults
			if(!fileExists(App_Session_File)) {
				AppSessionSettings at = {};

				Rect r = ws->monitors[0].workRect;
				Vec2 center = vec2(rectCenX(r), (r.top - r.bottom)/2);
				Vec2 dim = vec2(rectW(r), -rectH(r));
				at.windowRect = rectCenDim(center, dim*0.85f);

				at.mouseSpeed = 1;
				at.fontScale = 0.95;
				
				ad->fontHeight = roundInt(at.fontScale*systemData->fontHeight);

				at.panelWidthLeft = ad->fontHeight*18;
				at.panelWidthRight = ad->fontHeight*16;

				strClear(at.sceneFile);

				appWriteSessionSettings(App_Session_File, &at);
			}

			// @AppSessionLoad
			{
				AppSessionSettings at = {};
				appReadSessionSettings(App_Session_File, &at);

				Recti r = rectiRound(at.windowRect);
				MoveWindow(windowHandle, r.left, r.top, r.right-r.left, r.bottom-r.top, true);

				ad->mouseSpeed = at.mouseSpeed;
				ad->fontScale = at.fontScale;
				ad->panelWidthLeft = at.panelWidthLeft;
				ad->panelWidthRight = at.panelWidthRight;

				appSessionSettings = at;
			}
		}

		pcg32_srandom(0, __rdtsc());


		// @AppInit.

		timerInit(&ad->swapTimer);
		timerInit(&ad->frameTimer);

		folderExistsCreate(Scenes_Folder);
		folderExistsCreate(Screenshot_Folder);


		ad->entityUI.selectionMode = ENTITYUI_MODE_TRANSLATION;
		ad->entityUI.localMode = false;
		ad->entityUI.snapGridSize = 3;
		ad->entityUI.snapGridDim = 100;

		ad->entityUI.objectCopies.push(defaultObject());

		ad->fontFile       = getPStringCpy("LiberationSans-Regular.ttf");
		ad->fontFileBold   = getPStringCpy("LiberationSans-Bold.ttf");
		ad->fontFileItalic = getPStringCpy("LiberationSans-Italic.ttf");

		ad->showPanels = true;
		ad->panelsVisible = true;

		// ad->settings.texDim = vec2i(1280, 720);
		ad->settings.texDim = vec2i(768, 432);

		ad->settings.sampleMode = SAMPLE_MODE_BLUE;
		ad->settings.sampleCountWanted = 100;
		ad->settings.sampleGridWidth = 10;
		ad->settings.rayBouncesMax = 10;

		ad->drawSceneWired = true;
		ad->fitToScreen = false;

		ad->settings.randomDirectionCount = 20000;

		ad->sceneFileMax = 200;
		ad->sceneFile = getPString(ad->sceneFileMax);

		if(strLen(appSessionSettings.sceneFile) && 
		   fileExists(appSessionSettings.sceneFile)) {
			strCpy(ad->sceneFile, appSessionSettings.sceneFile);
			ad->sceneHasFile = true;
			loadScene(&ad->world, ad->sceneFile, &ad->entityUI);
		} else {
			ad->sceneHasFile = false;
			getDefaultScene(&ad->world);
		}

		ad->dialogData.active = false;
		ad->dialogData.filePathSize = 400;
		ad->dialogData.filePath = getPString(ad->dialogData.filePathSize);
		ad->dialogData.windowHandle = windowHandle;

		// Precalc random directions.
		{
			int count = ad->settings.randomDirectionCount;
			ad->settings.randomDirections = mallocArray(Vec3, count);
			float precision = 0.001f;

			for(int i = 0; i < count; i++) {
				// Cube discard method.

				Vec3 randomDir;
				do {
					randomDir = vec3(randomFloatPCG(-1,1,precision), randomFloatPCG(-1,1,precision), randomFloatPCG(-1,1,precision));
				} while(lenVec3(randomDir) > 1);
				randomDir = normVec3(randomDir);
				if(randomDir == vec3(0,0,0)) randomDir = vec3(1,0,0);

				ad->settings.randomDirections[i] = randomDir;
			}
		}
	}

	// @AppStart.

	if(reload) {
		loadFunctions();
		SetWindowLongPtr(systemData->windowHandle, GWLP_WNDPROC, (LONG_PTR)mainWindowCallBack);
	    SetWindowLongPtr(systemData->windowHandle, GWLP_USERDATA, (LONG_PTR)systemData);

	    DeleteFiber(systemData->messageFiber);
	    systemData->messageFiber = CreateFiber(0, (PFIBER_START_ROUTINE)updateInput, systemData);

		// gs->screenRes = ws->currentRes;
		gs->screenRes = ws->clientRes;
		gs->clientRectOffset = ws->clientRectBL.min;

		// Bad news.
		for(int i = 0; i < arrayCount(globalGraphicsState->fonts); i++) {
			for(int j = 0; j < arrayCount(globalGraphicsState->fonts[0]); j++) {
				Font* font = &globalGraphicsState->fonts[i][j];
				if(font->heightIndex != 0) {
					freeFont(font);
				} else break;
			}
		}
	}

	// Update timer.
	{
		if(init) {
			timerStart(&ad->frameTimer);
			ad->dt = 1/(float)60;
		} else {
			ad->dt = timerUpdate(&ad->frameTimer);
			ad->time += ad->dt;

			ad->fpsTime += ad->dt;
			ad->fpsCounter++;
			if(ad->fpsTime >= 1) {
				ad->avgFps = 1 / (ad->fpsTime / (f64)ad->fpsCounter);
				ad->fpsTime = 0;
				ad->fpsCounter = 0;
			}

			// timerStart(&ad->frameTimer);
			// printf("%f\n", ad->dt);
		}
	}

	clearTMemory();

	// {
		// if(init)
    		// SetWindowPos(systemData->windowHandle, 0, 0,0,1280,800, SWP_NOMOVE);
	// }

	//
	// Update input.
	//

	{
		updateWindowFrameData(systemData, ws);

		inputPrepare(input);
		SwitchToFiber(systemData->messageFiber);

		if(ad->input.closeWindow) *isRunning = false;

		ad->frameCount++;

		#ifdef ENABLE_CUSTOM_WINDOW_FRAME
		if(systemData->mouseInClient && !ws->dontUpdateCursor) updateCursor(ws);
		#else 
	    if(mouseInClientArea(windowHandle) && !ws->dontUpdateCursor) updateCursor(ws);
		#endif

		ws->windowHasFocus = systemData->windowIsFocused;

		systemData->fontHeight = getSystemFontHeight(systemData->windowHandle);
		ad->fontHeight = roundInt(ad->fontScale*systemData->fontHeight);
	}

	if(input->keysPressed[KEYCODE_F11] && !systemData->maximized) {
		if(ws->fullscreen) setWindowMode(windowHandle, ws, WINDOW_MODE_WINDOWED);
		else setWindowMode(windowHandle, ws, WINDOW_MODE_FULLBORDERLESS);
	}

	if(input->resize) {
		if(!windowIsMinimized(windowHandle)) {
			updateResolution(windowHandle, systemData, ws);
			ad->updateFrameBuffers = true;
		}
		input->resize = false;

		updateWindowFrameData(systemData, ws);
	}

	if(ad->updateFrameBuffers) {
		gs->screenRes = ws->clientRes;
		gs->clientRectOffset = ws->clientRectBL.min;

		// setDimForFrameBufferAttachmentsAndUpdate(FRAMEBUFFER_2dMsaa, ws->currentRes.w, ws->currentRes.h);
		// setDimForFrameBufferAttachmentsAndUpdate(FRAMEBUFFER_2dNoMsaa, ws->currentRes.w, ws->currentRes.h);

		ad->updateFrameBuffers = false;
	}

	openglDebug();
	openglDefaultSetup();
	// openglClearFrameBuffers();

	{
		glClearColor(0,0,0,1);
		scissorState();
		glScissor(0,0,ws->windowRes.w,ws->windowRes.h);

		bindFrameBuffer(FRAMEBUFFER_2dMsaa);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		glClearColor(0,0,0,0);
		bindFrameBuffer(FRAMEBUFFER_2dPanels);
		glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);


		glClearColor(0,0,0,1);
		bindFrameBuffer(FRAMEBUFFER_2dMsaa);

		scissorState(false);
	}


	// @AppLoop.

	NewGui* gui = &ad->gui;

	{
		newGuiBegin(gui, &ad->input, ws, ad->dt, systemData->mouseInClient);
	}

	#ifdef ENABLE_CUSTOM_WINDOW_FRAME
	if(!ws->fullscreen)
	{
		if(systemData->maximized) {
			int stop = 234;
		}

		// @DrawFrame
		{
			scissorState(false);

			glLoadIdentity();
			Vec2i res = ws->windowRes;
			glViewport(0,0,res.w,res.h);
			glOrtho(0, res.w, -res.h, 0, -10,10);

			bindFrameBuffer(FRAMEBUFFER_2dMsaa);
		}

		SystemData* sd = systemData;

		if(!systemData->mouseInClient && !mouseInClientArea(windowHandle)) systemData->ncTestRegion = 0;

		char* titleText = App_Name;
		if(ad->sceneHasFile) {
			titleText = fillString("%s", ad->sceneFile);
		}

		sd->normalTitleHeight = sd->fontHeight*1.4f;
		sd->normalBorderSize = 5;
		sd->normalVisualBorderSize = 1;

		sd->buttonMargin = 3;
		sd->cornerGrabSize = 20;

		Vec4 cBorder = ad->colors.border;

		Vec4 cButtons = vec4(0.6f,1);
		Vec4 cButtonsHot = vec4(1.0f,1);

		Vec4 cButton0 = cButtons;
		Vec4 cButton1 = cButtons;
		Vec4 cButton2 = cButtons;

		Vec4 cText = vec4(0.85f,1);
		if(!ws->windowHasFocus) cText = vec4(0.6f,1);

		Vec4 cTextShadow = ad->colors.outline;

		float fontHeight = sd->normalTitleHeight*0.7f;
		float textPadding = fontHeight*0.3f;

		glDepthMask(false);

		// Background
		drawRect(ws->windowRect, cBorder);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glBlendEquation(GL_FUNC_ADD);

		// Titlebar
		drawRect(ws->titleRect, ad->colors.background);

		{
			glLineWidth(1);
			Vec2 p0 = ws->titleRect.min;
			Vec2 p1 = rectBR(ws->titleRect);
			drawLine(vec2(roundFloat(p0.x),roundFloat(p0.y)+0.5f), vec2(roundFloat(p1.x),roundFloat(p1.y)+0.5f), cBorder);
			glLineWidth(1);
		}

		if(sd->ncTestRegion == HTMINBUTTON) cButton0 = cButtonsHot;
		if(sd->ncTestRegion == HTMAXBUTTON) cButton1 = cButtonsHot;
		if(sd->ncTestRegion == HTCLOSE)     cButton2 = cButtonsHot;

		glLineWidth(1);
		for(int i = 0; i < 2; i++)
		{
			Vec4 cShadow = ad->colors.outline;
			Vec4 c;
			Vec2 so = i == 0 ? vec2(0,-1) : vec2(0,0);

			float off = rectW(sd->rMinimize)*0.2f;
			{
				Rect r = sd->rMinimize;
				Vec2 p0 = rectBL(r) + vec2(off);
				Vec2 p1 = rectBR(r) + vec2(-off,off);
				
				Vec4 c = i == 1 ? cButton0 : cShadow;
				drawLine(roundVec2(p0)+vec2(0,0.5f)+so, roundVec2(p1)+vec2(0,0.5f)+so, c);
			}

			{
				Rect r = sd->rMaximize;
				r = rectExpand(r, vec2(-off*2));
				r = rectRound(r);
				Vec4 c = i == 1 ? cButton1 : cShadow;
				r = rectTrans(r, so);
				drawRectOutline(r, c);
				drawRect(rectRound(rectRSetB(r, 2)), c);
			}

			{
				Rect r = sd->rClose;
				Vec4 c = i == 1 ? cButton2 : cShadow;
				drawCross(roundVec2(rectCen(r))+so, rectW(r) - off*2, 1.5f, c);
			}
		}


		Vec2 tp = vec2(ws->titleRect.left + textPadding, rectCen(ws->titleRect).y);

		Font* font = getFont(ad->fontFile, fontHeight, ad->fontFileBold, ad->fontFileItalic);
		TextSettings ts = textSettings(font, cText, TEXTSHADOW_MODE_SHADOW, vec2(-1,-1), 1, cTextShadow);

		// Draw title text.
		{
			Rect r = rectSetR(ws->titleRect, sd->rMinimize.left-sd->buttonMargin);
			scissorTest(rect(r.left,0,r.right,ws->windowRes.h));
			scissorState();

			// drawText(titleText, tp, vec2i(-1,0), ts);
			drawText(fillString("<b>%s<b>", titleText), tp, vec2i(-1,0), ts);

			scissorState(false);
		}

		glDepthMask(true);
	}
	#endif


	if(true)
	{
		bindFrameBuffer(FRAMEBUFFER_2dMsaa);

		glLoadIdentity();
		Vec2i res = ws->clientRes;
		setClientViewport(ws, rect(0,0,res.w,res.h));
		glOrtho(0, res.w, -res.h, 0, -10,10);

		scissorState(true);
		setClientScissor(ws, rect(0,0,res.w,res.h));

		glDisable(GL_DEPTH_TEST);
		drawRect(rectTLDim(0,0,ws->windowRes.w, ws->windowRes.h), vec4(0.15f,1));
		glEnable(GL_DEPTH_TEST);
	}


	{
		if(keyDown(gui, input, KEYCODE_CTRL) && keyPressed(gui, input, KEYCODE_N)) {
			newSceneCommand(&ad->world, ad->sceneFile, &ad->sceneHasFile, &ad->entityUI);
		}
		if(keyDown(gui, input, KEYCODE_CTRL) && keyPressed(gui, input, KEYCODE_O)) {
			openSceneCommand(&ad->dialogData);
		}
		if(keyDown(gui, input, KEYCODE_CTRL) && keyPressed(gui, input, KEYCODE_S)) {
			saveSceneCommand(&ad->world, ad->sceneFile, ad->sceneHasFile, &ad->dialogData);
		}

		// Mouse capture.
		{
			if(!ad->captureMouse) {
				bool mouseOverUI = pointInRectEx(input->mousePosNegative, ad->menuRect) ||
								   pointInRectEx(input->mousePosNegative, ad->panelLeftRect) || 
								   pointInRectEx(input->mousePosNegative, ad->panelRightRect);

				if((input->keysPressed[KEYCODE_F3] || (mouseButtonPressedRight(gui, input) && !mouseOverUI)) && ad->drawSceneWired) {
					ad->captureMouse = true;

					if(input->keysPressed[KEYCODE_F3]) ad->captureMouseKeepCenter = true;
					else ad->captureMouseKeepCenter = false;

					GetCursorPos(&ws->lastMousePosition);
				}
			} else {
				if(input->keysPressed[KEYCODE_F3] || mouseButtonReleasedRight(gui, input)) {
					ad->captureMouse = false;

					SetCursorPos(ws->lastMousePosition.x, ws->lastMousePosition.y);
				}
			}
		}

		if(!ad->activeProcessing && ad->drawSceneWired)
		{
			ad->fpsMode = ad->captureMouse && windowHasFocus(windowHandle);
			if(ad->fpsMode) {
				if(ad->captureMouseKeepCenter) {
					int w,h;
					Vec2i wPos;
					getWindowProperties(windowHandle, &w, &h, 0, 0, &wPos.x, &wPos.y);

					SetCursorPos(wPos.x + w/2, wPos.y + h/2);
					input->lastMousePos = getMousePos(windowHandle,false);
				} else {
					SetCursorPos(ws->lastMousePosition.x, ws->lastMousePosition.y);
					input->lastMousePos = getMousePos(windowHandle,false);
				}

				while(ShowCursor(false) >= 0) {};
			} else {
				while(ShowCursor(true) < 0) {};
			}

			Camera* cam = &ad->world.camera;

			float aspectRatio = (float)ad->settings.texDim.w / ad->settings.texDim.h;
			cam->dim.h = cam->dim.w*(1/aspectRatio);
			cam->nearDist = camDistanceFromFOVandWidth(cam->fov, cam->dim.w);

			if(ad->fpsMode) {
				float speed = ad->mouseSpeed * 0.0015f;

				cam->rot.x += -input->mouseDelta.x*speed;
				cam->rot.y += input->mouseDelta.y*speed;
				clamp(&cam->rot.y, -M_PI_2 + 0.001f, M_PI_2 - 0.001f);
			}

			OrientationVectors o = getVectorsFromRotation(cam->rot);
			cam->ovecs = o;

			float speed = 40.0f*ad->dt;

			if(input->keysDown[KEYCODE_SHIFT]) speed *= 2;

			Vec3 vel = vec3(0,0,0);
			// if(input->keysDown[KEYCODE_CTRL]) o.dir = normVec3(cross(vec3(0,0,1), o.right));
			if(!input->keysDown[KEYCODE_CTRL]) {
				if(keyDown(gui, input, KEYCODE_W)) vel += o.dir;
				if(keyDown(gui, input, KEYCODE_S)) vel += -o.dir;
				if(keyDown(gui, input, KEYCODE_A)) vel += -o.right;
				if(keyDown(gui, input, KEYCODE_D)) vel += o.right;
				if(keyDown(gui, input, KEYCODE_E)) vel += vec3(0,0,1);
				if(keyDown(gui, input, KEYCODE_Q)) vel += vec3(0,0,-1);
	
				if(vel != vec3(0,0,0)) cam->pos += normVec3(vel) * speed;
			}
		}

		{
			Texture* texture = &ad->raycastTexture;
			Vec2i texDim = ad->settings.texDim;
			if(!texture->isCreated || (texDim != texture->dim)) {
				if(texDim != texture->dim) deleteTexture(texture);

				// initTexture(texture, -1, INTERNAL_TEXTURE_FORMAT, texDim, 3, GL_NEAREST, GL_CLAMP);
				// initTexture(texture, -1, INTERNAL_TEXTURE_FORMAT, texDim, 3, GL_LINEAR, GL_CLAMP);

				initTexture(texture, -1, GL_RGBA8, texDim, 3, GL_NEAREST, GL_CLAMP);

				Texture* t = &ad->raycastTexture;

				// glClearTexSubImage(t->id, 0, 0,0,0, t->dim.w,t->dim.h, 1, GL_RGB, GL_FLOAT, &ad->world.defaultEmitColor);
			}
		}

		if(keyPressed(gui, input, KEYCODE_SPACE) && ad->drawSceneWired) {
			ad->startProcessing = true;
		}

		if(ad->startProcessing && (!ad->activeProcessing && ad->drawSceneWired) && !ad->fpsMode) {

			ad->startProcessing = false;
			ad->activeProcessing = true;
			ad->drawSceneWired = false;
			ad->tracerFinished = false;

			RaytraceSettings* settings = &ad->settings;

			settings->mode = RENDERING_MODE_RAY_TRACER; // MODE_PATH_TRACER

			Texture* t = &ad->raycastTexture;
			// Vec3 black = vec3(0.2f);
			// glClearTexSubImage(t->id, 0, 0,0,0, t->dim.w,t->dim.h, 1, GL_RGB, GL_FLOAT, &ad->world.defaultEmitColor);

			int pixelCount = settings->texDim.w*settings->texDim.h;
			reallocArraySave(Vec3, ad->buffer, pixelCount);

			// Setup samples.
			{
				int mode = settings->sampleMode;

				Vec2* blueNoiseSamples;

				settings->sampleCountGrid = sqrt(settings->sampleCountWanted);

				int sampleCount;
				if(mode == SAMPLE_MODE_GRID) {
					sampleCount = settings->sampleCountGrid*settings->sampleCountGrid;
				} else if(mode == SAMPLE_MODE_BLUE) {
					float sampleCountGrid = settings->sampleCountGrid * 1.3f;
					sampleCount = blueNoise(rect(vec2(0.0f),vec2(settings->sampleGridWidth)), 1/(float)sampleCountGrid, &blueNoiseSamples);
				}

				settings->sampleCount = sampleCount;
				if(settings->samples) free(settings->samples);
				settings->samples = mallocArray(Vec2, sampleCount);

				switch(mode) {
					case SAMPLE_MODE_GRID: {
						if(settings->sampleCount == 1) {
							settings->samples[0] = vec2(0.5f, 0.5f);
							break;
						}

						int sampleCount2 = sqrt(settings->sampleCount);
						for(int i = 0; i < sampleCount; i++) {
							settings->samples[i] = vec2(((i%sampleCount2)*sampleCount2 + 1) / (float)sampleCount, 
							                  ((i/sampleCount2)*sampleCount2 + 1) / (float)sampleCount);
						}
					} break;

					case SAMPLE_MODE_BLUE: {

						assert(settings->sampleGridWidth > 0);

						settings->sampleGridCount = settings->sampleGridWidth * settings->sampleGridWidth + 1;
						if(settings->sampleGridOffsets) free(settings->sampleGridOffsets);
						settings->sampleGridOffsets = mallocArray(int, settings->sampleGridCount);

						for(int i = 0; i < settings->sampleGridCount; i++) settings->sampleGridOffsets[i] = 0;

						// Go through samples, calculate sample grid position and count how many samples per grid tile.
						int width = settings->sampleGridWidth;
						for(int i = 0; i < sampleCount; i++) {
							Vec2i gridTilePos = vec2i(blueNoiseSamples[i]); // Casting to int rounds down and gives grid pos.
							gridTilePos = clampMax(gridTilePos, vec2i(settings->sampleGridWidth-1)); // In case sample spawned at exact right border.

							// First element is zero so we can calculate offset and count like this:
							// index = array[i]; count = array[i+1] - array[i];
							settings->sampleGridOffsets[1 + gridTilePos.y*width + gridTilePos.x] += 1;
						}

						// Turn counts into offsets.
						for(int i = 0; i < settings->sampleGridCount-1; i++) {
							settings->sampleGridOffsets[i+1] += settings->sampleGridOffsets[i];
						}

						int* counter = mallocArray(int, settings->sampleGridCount);
						for(int i = 0; i < settings->sampleGridCount; i++) counter[i] = 0;

						for(int i = 0; i < sampleCount; i++) {
							Vec2i gridTilePos = vec2i(blueNoiseSamples[i]);
							gridTilePos = clampMax(gridTilePos, vec2i(settings->sampleGridWidth-1)); // In case sample spawned at exact right border.

							int gridIndex = gridTilePos.y*width + gridTilePos.x;

							int index = settings->sampleGridOffsets[gridIndex] + counter[gridIndex];
							counter[gridIndex]++;

							settings->samples[index] = blueNoiseSamples[i] - vec2(gridTilePos);
						}

						free(counter);
						free(blueNoiseSamples);
					} break;
				}
			}

			// Precalc.
			{
				World* world = &ad->world;
				settings->pixelPercent = vec2(1/(float)settings->texDim.w, 1/(float)settings->texDim.h);

				Camera camera = world->camera;
				OrientationVectors ovecs = camera.ovecs;
				settings->camTopLeft = camera.pos + ovecs.dir*camera.nearDist + (ovecs.right*-1)*(camera.dim.w/2.0f) + (ovecs.up)*(camera.dim.h/2.0f);

				for(int i = 0; i < world->objects.count; i++) {
					Object* obj = world->objects + i;
					geometryBoundingSphere(obj);
					geometryBoundingBox(obj);
				}

				worldCalcLightDir(world);
			}

			// Push thread jobs in circles.
			{
				Vec2 dim = vec2(ad->settings.texDim);

				int count = arrayCount(ad->threadData) * 0.8f;
				float size = sqrt(((dim.w*dim.h) / count));
				int tileSize = (int)size;
				Vec2 realCount = vec2(dim.x / tileSize, dim.y / tileSize);

				Vec2 cen = realCount/2;
				Vec2i startIndex = vec2i(cen);

				int threadJobCount = 0;

				{
					int threadIndex = 0;
					Vec2i si = startIndex;

					int maxOff = (max(realCount.x, realCount.y)/2) + 1;
					for(int off = 0; off < maxOff+1; off++) {

						Vec2i startP = vec2i(-off, off-1);
						if(off == 0) startP = vec2i(0,0);
						Vec2i p = startP;
						int counter = 0;
						Vec2i dir = vec2i(0,-1);
						for(;;) {
							Vec2i rp = vec2i(si.x + p.x, si.y + p.y);
							if(rp.x >= 0 && rp.x < roundUpInt(realCount.x) && 
							   rp.y >= 0 && rp.y < roundUpInt(realCount.y)) {

								Vec2i topLeft = vec2i(0,0);
								Vec2i pixelPos = vec2i(rp.x * tileSize, rp.y * tileSize);

								ProcessPixelsData* data = ad->threadData + threadIndex++;
								*data = {};
								data->pixelPos = pixelPos;
								data->pixelDim = vec2i(tileSize);

								data->world = &ad->world;
								data->settings = settings;
								data->buffer = ad->buffer;

								threadQueueAdd(globalThreadQueue, processPixelsThreaded, data);
								threadJobCount++;
							}

							if(off == 0) break;

							if(abs(p.x) == off && abs(p.y) == off) {
								// dir = vec2i(-dir.y, dir.x);
								dir = vec2i(-dir.y, dir.x);
							}
							p += dir;
							if(p == startP) break;
						}
					}
				}

				ad->threadCount = threadJobCount;
			}

			ad->processStartTime = ad->time;
			ad->processTime = 0;
		}

		if(keyPressed(gui, input, KEYCODE_ESCAPE) && !ad->drawSceneWired) {
			for(int i = 0; i < ad->threadCount; i++) ad->threadData[i].stopProcessing = true;
			ad->abortProcessing = true;
		}

		if(!ad->activeProcessing && ad->abortProcessing) {
			ad->drawSceneWired = true;
			ad->abortProcessing = false;
		}

		bool doneProcessing = false;
		if(ad->activeProcessing && threadQueueFinished(threadQueue)) {
			ad->activeProcessing = false;
			doneProcessing = true;

			ad->processTime = ad->time - ad->processStartTime;

			if(!ad->abortProcessing) {
				ad->tracerFinished = true;
			} else {
				ad->drawSceneWired = true;
				ad->abortProcessing = false;
			}
		}

		if(doneProcessing || ad->activeProcessing) {
			// glTextureSubImage2D(ad->raycastTexture.id, 0, 0, 0, ad->settings.texDim.w, ad->settings.texDim.h, GL_RGB, GL_FLOAT, ad->buffer);

			glBindTexture(GL_TEXTURE_2D, ad->raycastTexture.id);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,ad->settings.texDim.w, ad->settings.texDim.h, GL_RGB, GL_FLOAT, ad->buffer);

			// glGenerateTextureMipmap(ad->raycastTexture.id);
			glGenerateMipmap(GL_TEXTURE_2D);
		}

		// Calc texture rect.
		{
			Rect sr = getScreenRect(ws);
			Rect tr = rectAddT(sr, -ad->menuHeight);
			Vec2 sd = rectDim(tr);
			Vec2 texDim = vec2(ad->raycastTexture.dim);

			if(((float)texDim.h / texDim.w) > (sd.h / sd.w)) {
				ad->textureScreenRectFitted = rectSetW(tr, ((float)texDim.w / texDim.h)*sd.h);
			} else {
				ad->textureScreenRectFitted = rectSetH(tr, ((float)texDim.h / texDim.w)*sd.w);
			}

			{
				Vec2 c = rectCen(tr);
				Vec2i td = ad->raycastTexture.dim;
				c.x -= td.w/2.0f;
				c.y += td.h/2.0f;
				c.x = roundFloat(c.x);
				c.y = roundFloat(c.y);

				ad->textureScreenRect = rectTLDim(c, texDim);
			}

			if(ad->fitToScreen) {
				ad->textureScreenRect = ad->textureScreenRectFitted;
			}

			glDisable(GL_DEPTH_TEST);
			drawRect(rectTLDim(0,0,ws->windowRes.w, ws->windowRes.h), vec4(0.15f,1));
			glEnable(GL_DEPTH_TEST);

			if(!ad->drawSceneWired) {
				glDisable(GL_DEPTH_TEST);
				drawRect(ad->textureScreenRect, vec4(1,1,1,1), rect(0,0,1,1), ad->raycastTexture.id);
				glEnable(GL_DEPTH_TEST);
			}
		}

		if(keyPressed(gui, input, KEYCODE_F)) ad->fitToScreen = !ad->fitToScreen;
	}

	{
		// @EntityUI

		World* world = &ad->world;
		EntityUI* eui = &ad->entityUI;
		Camera* cam = &ad->world.camera;

		eui->guiHasFocus = guiHotMouseClick(gui);
		if(!eui->mouseOverScene) eui->guiHasFocus = true;

		// @Dialogs.
		{
			DialogData* dd = &ad->dialogData;
			if(dd->finished) {
				if(!dd->error) {
					if(strCompare(dd->type, "SceneDialog")) {
						if(dd->saveMode) {
							printf("Start saving scene.\n");
							saveScene(world, dd->result);
							ad->sceneHasFile = true;
							strCpy(ad->sceneFile, dd->result);
							printf("Finished saving scene.\n");
						} else {
							printf("Start loading scene.\n");
							loadScene(world, dd->result, eui);
							ad->sceneHasFile = true;
							strCpy(ad->sceneFile, dd->result);

							historyReset(&eui->history);
							eui->selectedObjects.count = 0;
							printf("Finished loading scene.\n");
						}

					} else if(strCompare(dd->type, "ScreenshotDialog")) {
						Vec2i texDim = ad->settings.texDim;
						int size = texDim.w * texDim.h;
						char* intBuffer = mallocArray(char, size*3);

						Vec3* floatBuffer = ad->buffer;
						for(int i = 0; i < size; i++) {
							Vec3 c = floatBuffer[i];
							intBuffer[i*3 + 0] = colorFloatToInt(floatToSrgb(c.r));
							intBuffer[i*3 + 1] = colorFloatToInt(floatToSrgb(c.g));
							intBuffer[i*3 + 2] = colorFloatToInt(floatToSrgb(c.b));
						}

						stbi_write_png(ad->dialogData.filePath, texDim.w, texDim.h, 3, intBuffer, 0);

						free(intBuffer);
					}
				}

				dd->finished = false;
				gui->activeId = 0;
				ws->dontUpdateCursor = false;
				gui->disable = false;
			}

			// If active dialog, disable everything.
			if(dd->active) {
				ws->dontUpdateCursor = true;
				gui->activeId = -1;
				gui->disable = true;
			}
		}

		if(ad->drawSceneWired) {

			if(pointInRectEx(input->mousePosNegative, ad->textureScreenRectFitted)) {
				eui->mouseOverScene = true;
			} else {
				gui->contenderId[Gui_Focus_MLeft] = -1;
				eui->mouseOverScene = false;
			}

			if(mouseWheel(gui, input)) { 
				if(eui->selectionState != ENTITYUI_ACTIVE) {
					ad->entityUI.selectionMode = mod(ad->entityUI.selectionMode - mouseWheel(gui, input), ENTITYUI_MODE_SCALE+1);
				} else {
					if(eui->selectionMode == ENTITYUI_MODE_TRANSLATION && eui->translateMode == TRANSLATE_MODE_CENTER) {
						float wheelObjectCenterSpeed = 0.4f;
						eui->centerDistanceToCam += mouseWheel(gui, input) * cam->dim.w*wheelObjectCenterSpeed;
					}
				}
			}

			if(eui->selectedObjects.count > 1) {
				if(eui->selectionMode == ENTITYUI_MODE_ROTATION || 
				   eui->selectionMode == ENTITYUI_MODE_SCALE) {
					eui->selectionMode = ENTITYUI_MODE_TRANSLATION;
				}
			}

			if(input->keysDown[KEYCODE_SHIFT]) {
				eui->snappingEnabled = true;
			} else {
				eui->snappingEnabled = false;
			}

			// @Selection.

			// Bad.
			for(int i = 0; i < world->objects.count; i++) {
				Object* obj = world->objects + i;
				geometryBoundingSphere(obj);
				geometryBoundingBox(obj);
			}

			if(keyPressed(gui, input, KEYCODE_TAB)) {
				ad->showPanels = !ad->showPanels;
			}

			bool multipleSelectionMode = input->keysDown[KEYCODE_CTRL];
			eui->multipleSelectionMode = multipleSelectionMode;

			if(mouseButtonPressedMiddle(gui, input)) {
				eui->dragSelectionStart = input->mousePosNegative;
				eui->dragSelectionActive = true;
				eui->selectionMode = ENTITYUI_MODE_DRAG;
				eui->selectionState = ENTITYUI_ACTIVE;
			}

			if(input->mouseButtonReleased[2]) {
				eui->dragSelectionActive = false;
				eui->selectionChanged = true;
				eui->selectionState = ENTITYUI_INACTIVE;
				eui->selectionMode = ENTITYUI_MODE_TRANSLATION;
			}

			if(eui->selectedObjects.count == 0 && eui->selectionState == ENTITYUI_HOT) {
				eui->selectionState = ENTITYUI_INACTIVE;
			}

			if(mouseButtonPressedLeft(gui, input) && eui->selectionState == ENTITYUI_INACTIVE) {
				Vec3 rayDir = mouseRayCast(ad->textureScreenRectFitted, input->mousePosNegative, &ad->world.camera);

				int objectIndex = castRay(ad->world.camera.pos, rayDir, &ad->world.objects);
				if(objectIndex) {
					objectIndex--;

					if(multipleSelectionMode) {
						if(isObjectSelected(eui, objectIndex)) {
							// Deselect.
							int i = eui->selectedObjects.find(objectIndex)-1;
							eui->selectedObjects.remove(i);
						} else {
							// Select.
							eui->selectedObjects.push(objectIndex);
							eui->selectionAnimState = 0;
						}
					} else {
						eui->selectedObjects.clear();
						eui->selectedObjects.push(objectIndex);
						eui->selectionAnimState = 0;
					}

					eui->selectionChanged = true;
				} else {
					if(!multipleSelectionMode) {
						eui->selectedObjects.clear();
						eui->selectionState = ENTITYUI_INACTIVE;
						eui->selectionChanged = true;
					}
				}
			}

			if(keyPressed(gui, input, KEYCODE_ESCAPE) && eui->selectedObjects.count && !ad->drawSceneWired) {
				eui->selectedObjects.clear();
				eui->selectionState = ENTITYUI_INACTIVE;
				eui->selectionChanged = true;

				// Hack.
				gui->popupStackCount = 0;
			}

			if(eui->selectionChanged) {
				// Compare current selection with previous and only push when different.
				if(eui->selectedObjects != eui->history.previousSelection)
					historySelection(&eui->history, &eui->selectedObjects);

				eui->selectionChanged = false;
			}

			if(eui->selectedObjects.count) {
				// Delete.
				if(keyPressed(gui, input, KEYCODE_DEL) && eui->selectionState != ENTITYUI_ACTIVE) {
					deleteObjects(&world->objects, &eui->selectedObjects, &eui->history, false);
				}

				// Copy.
				if(keyDown(gui, input, KEYCODE_CTRL) && keyPressed(gui, input, KEYCODE_C)) {
					copyObjects(&world->objects, &eui->objectCopies, &eui->selectedObjects);
				}

				// Cut.
				if(keyDown(gui, input, KEYCODE_CTRL) && keyPressed(gui, input, KEYCODE_X) && eui->selectionState != ENTITYUI_ACTIVE) {
					// Copy and delete.
					copyObjects(&world->objects, &eui->objectCopies, &eui->selectedObjects);
					deleteObjects(&world->objects, &eui->selectedObjects, &eui->history, false);
				}
			}
			// Paste.
			bool paste = false;
			if(keyDown(gui, input, KEYCODE_CTRL) && keyPressed(gui, input, KEYCODE_V)) {
				paste = true;
				eui->objectsEdited = true;
			}

			if(keyPressed(gui, input, KEYCODE_R)) eui->localMode = !eui->localMode;

			if(keyPressed(gui, input, KEYCODE_1)) eui->selectionMode = ENTITYUI_MODE_TRANSLATION;
			if(keyPressed(gui, input, KEYCODE_2)) eui->selectionMode = ENTITYUI_MODE_ROTATION;
			if(keyPressed(gui, input, KEYCODE_3)) eui->selectionMode = ENTITYUI_MODE_SCALE;

			if(keyDown(gui, input, KEYCODE_CTRL) && keyPressed(gui, input, KEYCODE_Y) && eui->selectionState != ENTITYUI_ACTIVE) {
				historyChange(&eui->history, world, &eui->selectedObjects);
			}

			if(keyDown(gui, input, KEYCODE_CTRL) && keyPressed(gui, input, KEYCODE_Z) && eui->selectionState != ENTITYUI_ACTIVE) {
				historyChange(&eui->history, world, &eui->selectedObjects, false);
			}

			// if((input->mouseButtonReleased[0] || paste) && eui->selectionState == ENTITYUI_ACTIVE) {
			if((input->mouseButtonReleased[0]) && eui->selectionState == ENTITYUI_ACTIVE) {
				eui->objectsEdited = true;
			}

			// if((input->mouseButtonReleased[0] || paste) && eui->selectionState == ENTITYUI_ACTIVE) {
			if(eui->objectsEdited) {
				eui->objectsEdited = false;

				if(!paste) {
					eui->selectionState = ENTITYUI_INACTIVE;
				}
				if(eui->objectNoticeableChange) {

					for(int i = 0; i < eui->history.objectsPreMod.count; i++) {
						Object objAfterMod = world->objects[eui->selectedObjects[i]];
						eui->history.objectsPreMod[i] = objectDiff(eui->history.objectsPreMod[i], objAfterMod);
					}

					historyEdit(&eui->history, &eui->selectedObjects);
				}

				eui->objectNoticeableChange = false;
			}

			if(paste) {
				// Copy paste if active.
				if(eui->selectionState == ENTITYUI_ACTIVE) {
					copyObjects(&world->objects, &eui->objectCopies, &eui->selectedObjects);
				}

				insertObjects(world, &eui->objectCopies, &eui->selectedObjects, &eui->objectsEdited, &eui->history, true);

				// Reset edit state. A paste should be an edit + paste.
				if(paste && eui->selectionState == ENTITYUI_ACTIVE) {

					// Code duplication with entityui code.

					eui->history.objectsPreMod.clear();
					for(int i = 0; i < eui->selectedObjects.count; i++) {
						Object obj = world->objects[eui->selectedObjects[i]];
						eui->history.objectsPreMod.push(obj);
					}

					Object* obj = &world->objects[eui->selectedObjects[0]];

					if(eui->selectionMode == ENTITYUI_MODE_TRANSLATION) {
						eui->startPos = eui->currentPos;
					} else if(eui->selectionMode == ENTITYUI_MODE_ROTATION) {
						eui->startRot = obj->rot;
						eui->objectDistanceVector = eui->currentObjectDistanceVector;
					} else if(eui->selectionMode == ENTITYUI_MODE_SCALE) {
						eui->startDim = obj->dim;
						eui->objectDistanceVector = eui->currentObjectDistanceVector;
					}
				}
			}

			if(keyDown(gui, input, KEYCODE_CTRL)) eui->enableScaleEqually = true;
			else eui->enableScaleEqually = false;

			// 

			eui->gotActive = false;

			if(mouseButtonPressedLeft(gui, input) && eui->selectionState == ENTITYUI_HOT) {
				eui->selectionState = ENTITYUI_ACTIVE;
				eui->gotActive = true;
			}

			if(eui->selectionState == ENTITYUI_ACTIVE && eui->selectionMode != ENTITYUI_MODE_DRAG) {

				Camera* cam = &ad->world.camera;
				Vec3 rayPos = cam->pos;
				Vec3 rayDir = mouseRayCast(ad->textureScreenRectFitted, input->mousePosNegative, &ad->world.camera);

				bool multipleSelection = eui->selectedObjects.count > 1;

				// Save all the objects before modifying them.
				if(eui->gotActive) {
					eui->history.objectsPreMod.clear();
					for(int i = 0; i < eui->selectedObjects.count; i++) {
						Object obj = world->objects[eui->selectedObjects[i]];
						eui->history.objectsPreMod.push(obj);
					}
				}

				if(eui->selectionMode == ENTITYUI_MODE_TRANSLATION) {

					Vec3 pos;
					if(!multipleSelection) {
						Object* obj = ad->world.objects + eui->selectedObjects.first();
						pos = obj->pos;
					} else {
						pos = selectedObjectsGetCenter(&world->objects, &eui->selectedObjects);

						// Get offset from all selected objects to center.
						if(eui->gotActive) {
							eui->objectCenterOffsets.clear();

							for(int i = 0; i < eui->selectedObjects.count; i++) {
								Object* obj = world->objects + eui->selectedObjects[i];
								Vec3 offset = obj->pos - pos;

								eui->objectCenterOffsets.push(offset);
							}
						}
					}

					if(eui->translateMode == TRANSLATE_MODE_AXIS) {
						Vec3 cameraOnAxis = projectPointOnLine(pos, eui->axis, cam->pos);
						Vec3 planeNormal = normVec3(cam->pos - cameraOnAxis);

						Vec3 planeIntersection;
						float distance = linePlaneIntersection(rayPos, rayDir, pos, planeNormal, &planeIntersection);
						if(distance != -1) {
							Vec3 linePointOnAxis = projectPointOnLine(pos, eui->axis, planeIntersection);

							// Init.
							if(eui->gotActive) {
								eui->objectDistanceVector = pos - linePointOnAxis;
								eui->startPos = pos;
							}

							pos = linePointOnAxis + eui->objectDistanceVector;
						}

					} else if(eui->translateMode == TRANSLATE_MODE_PLANE) {
						Vec3 planeNormal = eui->axis;

						Vec3 planeIntersection;
						float distance = linePlaneIntersection(rayPos, rayDir, pos, planeNormal, &planeIntersection);
						if(distance != -1) {

							// Init.
							if(eui->gotActive) {
								eui->objectDistanceVector = pos - planeIntersection;
								eui->startPos = pos;
							}

							pos = planeIntersection + eui->objectDistanceVector;
						}
					} else if(eui->translateMode == TRANSLATE_MODE_CENTER) {
						Vec3 pp = cam->pos + cam->ovecs.dir * eui->centerDistanceToCam + eui->centerOffset;

						Vec3 planeIntersection;
						float distance = linePlaneIntersection(rayPos, rayDir, pp, -cam->ovecs.dir, &planeIntersection);
						if(distance != -1) {

							// Init.
							if(eui->gotActive) {
								eui->objectDistanceVector = eui->centerOffset;
								eui->startPos = pos;
							}

							pos = planeIntersection - eui->objectDistanceVector;
						}
					}

					if(eui->snappingEnabled) {

						// We always snap every axis to keep it simple.
						for(int i = 0; i < 3; i++) {
							Vec3 p = projectPointOnLine(eui->startPos, eui->axes[i], pos);
							float length = lenVec3(p - eui->startPos);
							float snappedLength = roundMod(length, roundFloat(eui->snapGridSize));
							float lengthDiff = length - snappedLength;

							if(dot(p - eui->startPos, eui->axes[i]) > 0) lengthDiff *= -1;

							pos += eui->axes[i]*lengthDiff;
						}
					}

					if(!multipleSelection) {
						Object* obj = ad->world.objects + eui->selectedObjects.first();
						obj->pos = pos;
					} else {
						for(int i = 0; i < eui->selectedObjects.count; i++) {
							Object* obj = world->objects + eui->selectedObjects[i];
							obj->pos = pos + eui->objectCenterOffsets[i];
							Vec3 offset = obj->pos - pos;
						}
					}

					eui->currentPos = pos;

					// Center grabbing is not accurate enough and oscelates sometimes so we have to use a margin for the position difference calculation.
					float diff = lenVec3(eui->startPos - pos);
					float comparisonMargin = 0.00001f; // Static value not a good idea?
					eui->objectNoticeableChange = diff > comparisonMargin;
				} else {

					Object* obj = ad->world.objects + eui->selectedObjects.first();

					if(eui->selectionMode == ENTITYUI_MODE_ROTATION) {

						Vec3 intersection;
						float dist = linePlaneIntersection(cam->pos, rayDir, obj->pos, eui->axis, &intersection);
						if(dist != -1) {
							float distToObj = lenVec3(intersection - obj->pos);

							if(eui->gotActive) {
								eui->startRot = obj->rot;
								eui->objectDistanceVector = normVec3(intersection - obj->pos);
							}

							Vec3 start = eui->objectDistanceVector;
							Vec3 end = normVec3(intersection - obj->pos);

							Vec3 a = cross(start, end);
							float len = lenVec3(a);
							if(dot(normVec3(a), eui->axis) < 0) len *= -1;
							float angle = asin(len);
							if(dot(start, end) < 0) {
								if(angle > 0) angle = M_PI_2 + M_PI_2-angle;
								else angle = -M_PI_2 - (M_PI_2-abs(angle));
							}

							if(eui->snappingEnabled) angle = roundMod(angle, M_PI_4);

							obj->rot = quat(angle, eui->axis)*eui->startRot;

							eui->currentRotationAngle = angle;

							eui->currentObjectDistanceVector = end;
							eui->objectNoticeableChange = eui->currentRotationAngle != 0;
						}
					}

					if(eui->selectionMode == ENTITYUI_MODE_SCALE) {

						Vec3 cameraOnAxis = projectPointOnLine(obj->pos, eui->axis, cam->pos);
						Vec3 planeNormal = normVec3(cam->pos - cameraOnAxis);

						Vec3 planeIntersection;
						float distance = linePlaneIntersection(rayPos, rayDir, obj->pos, planeNormal, &planeIntersection);
						if(distance != -1) {
							Vec3 linePointOnAxis = projectPointOnLine(obj->pos, eui->axis, planeIntersection);

							if(eui->gotActive) {
								eui->objectDistanceVector = obj->pos - linePointOnAxis;

								eui->startDim = obj->dim;
							}

							float oldLength = lenVec3(eui->objectDistanceVector);
							float newLength = lenVec3(obj->pos - linePointOnAxis);

							float currentAxisLength = eui->startDim.e[eui->axisIndex-1] + (newLength - oldLength)*2;

							currentAxisLength = clampMin(currentAxisLength, 0);
							if(dot(eui->objectDistanceVector, obj->pos - linePointOnAxis) < 0) currentAxisLength = 0;

							if(eui->snappingEnabled) {
								currentAxisLength = roundMod(currentAxisLength, roundFloat(eui->snapGridSize));
							}

							if(eui->enableScaleEqually) {
								obj->dim = vec3(currentAxisLength);
							} else {
								obj->dim = eui->startDim;
								clampMin(&currentAxisLength, 0.0001f);
								obj->dim.e[eui->axisIndex-1] = currentAxisLength;
							}

							eui->currentObjectDistanceVector = obj->pos - linePointOnAxis;

							eui->objectNoticeableChange = obj->dim != eui->startDim;
						}
					}
				}

			}

		}
	}

	if(ad->drawSceneWired)
	{
		World* world = &ad->world;
		Camera* cam = &ad->world.camera;
		Vec2 d = cam->dim;

		float animSpeed = 3;
		ad->entityUI.selectionAnimState += ad->dt * animSpeed;

		Rect tr = ad->textureScreenRectFitted;

		glClearColor(0,0,0, 1);
		// glClear(GL_COLOR_BUFFER_BIT);

		Vec2i res = ws->clientRes;
		setClientViewport(ws, rect(tr.left, res.h+tr.bottom, tr.right, res.h+tr.top));


		glDepthMask(false);
		Vec3 cc = world->defaultEmitColor;
		drawRect(rectCenDim(0,0,10000,10000), vec4(cc,1));
		glDepthMask(true);

		glShadeModel(GL_SMOOTH);
		glEnable(GL_LIGHTING);
		glEnable(GL_LIGHT0);
		glEnable(GL_NORMALIZE);

		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();

		glFrustum(-d.w/2.0f, d.w/2.0f, -d.h/2.0f, d.h/2.0f, camDistanceFromFOVandWidth(cam->fov,d.w), cam->farDist);

		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();

		Mat4 vm = viewMatrix(cam->pos, cam->ovecs.dir, cam->ovecs.up, cam->ovecs.right);
		Mat4 temp = vm;
		rowToColumn(&temp);
		glLoadMatrixf(temp.e);

		worldCalcLightDir(world);
		Vec4 lp = vec4(normVec3(world->globalLightDir), 0);
		glLightfv(GL_LIGHT0, GL_POSITION, lp.e);

		Vec4 light;
		
		// light = vec4(world->defaultEmitColor,1);
		// glLightfv(GL_LIGHT0, GL_AMBIENT, light.e);

		light = vec4(world->globalLightColor,1);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, light.e);

		// light = vec4(world->globalLightColor,1);
		// glLightfv(GL_LIGHT0, GL_SPECULAR, light.e);

		glEnable(GL_DEPTH_TEST);
		glBindTexture(GL_TEXTURE_2D, 0);

		{
			// @Grid
			EntityUI* eui = &ad->entityUI;

			glDisable(GL_LIGHTING);

			glLineWidth(1);

			Vec4 lineColor = vec4(0,0.5f);
			float size = roundFloat(ad->entityUI.snapGridSize);
			float count = roundFloat(eui->snapGridDim / size);
			Vec3 start = vec3(-(size*count)/2, -(size*count)/2, 0);
			for(int i = 0; i < count+1; i++) {
				Vec3 p = start + vec3(1,0,0) * i*size;
				drawLine(p, p + vec3(0,size*count,0), lineColor);
			}

			for(int i = 0; i < count+1; i++) {
				Vec3 p = start + vec3(0,1,0) * i*size;
				drawLine(p, p + vec3(size*count,0,0), lineColor);
			}
			glEnable(GL_LIGHTING);
		}

		{
			glPushMatrix();
			glLoadIdentity();

			for(int objectIndex = 0; objectIndex < world->objects.count; objectIndex++) {

				Object* obj = world->objects + objectIndex;
				Geometry* g = &obj->geometry;
				Material* m = &obj->material;

				Vec4 color;

				// color = vec4(vec3(0.1f), 1);
				// glMaterialfv(GL_FRONT, GL_AMBIENT, color.e);

				// color = vec4(obj->color, 1);
				// glMaterialfv(GL_FRONT, GL_DIFFUSE, color.e);

				color = COLOR_SRGB(vec4(obj->color, 1));
				glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, color.e);

				color = vec4(m->emitColor, 1);
				glMaterialfv(GL_FRONT, GL_EMISSION, color.e);

				float shininess = lerp(m->reflectionMod, 0,128);
				glMaterialf(GL_FRONT, GL_SHININESS, shininess);



				Vec3 scale = vec3(1,1,1);
				scale = obj->dim;
				if(g->type == GEOM_TYPE_SPHERE) scale = scale / 2;

				Mat4 tm = translationMatrix(obj->pos);
				Mat4 rm = quatRotationMatrix(obj->rot);
				Mat4 sm = scaleMatrix(scale);

				Mat4 fm = vm * tm * rm * sm;
				rowToColumn(&fm);

				glLoadMatrixf(fm.e);

				drawGeometry(g->type);

				// Draw ui grid.

				if(isObjectSelected(&ad->entityUI, objectIndex)) {

					{
						Mat4 tm = translationMatrix(obj->pos);
						Mat4 rm = quatRotationMatrix(obj->rot);

						// So mesh grid will show up correctly.
						scale += vec3(0.01f);
						Mat4 sm = scaleMatrix(scale);

						Mat4 fm = vm * tm * rm * sm;
						rowToColumn(&fm);
						glLoadMatrixf(fm.e);
					}

					Vec4 color = vec4(1,1,1,1);

					// Color fading animation on selection.
					{
						float percent = (cos(ad->entityUI.selectionAnimState)+1)/2.0f;

						Vec4 faceColors[] = { vec4(0,0,0,1), vec4(1,0,1,1), vec4(1,1,1,1) };
						float timings[] = { 0.0f, 0.4f, 1.0f };

						int index = 0;
						for(int i = 0; i < arrayCount(timings)-1; i++) {
							if(valueBetween(percent, timings[i], timings[i+1])) index = i;
						}

						float l = mapRange(percent, timings[index], timings[index+1], 0, 1);
						for(int i = 0; i < 3; i++) {
							float ce = lerp(l, faceColors[index].e[i], faceColors[index+1].e[i]);
							color.e[i] = linearToGamma(ce);
						}
					}

					Vec4 c = COLOR_SRGB(color);
					glColor4f(c.r, c.g, c.b, c.a);

					glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
					glDisable(GL_LIGHTING);

					glLineWidth(0.5f);

					drawGeometry(g->type);

					glEnable(GL_LIGHTING);
					glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
					glDisable(GL_CULL_FACE);
					glLineWidth(1);

					// glEnable(GL_DEPTH_TEST);
				}
			}

			glPopMatrix();

			if(false)
			{
				// Scene bounding box.

				glDisable(GL_LIGHTING);
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

				Vec3 boxMin = vec3(FLT_MAX), boxMax = vec3(-FLT_MAX);

				for(int objectIndex = 0; objectIndex < world->objects.count; objectIndex++) {
					Object* obj = &world->objects.at(objectIndex);

					Vec3 pos = obj->pos;
					Vec3 bbox = obj->geometry.boundingBox;

					Vec3 bMin = pos - bbox/2;
					Vec3 bMax = pos + bbox/2;

					boxMin.x = min(boxMin.x, bMin.x);
					boxMin.y = min(boxMin.y, bMin.y);
					boxMin.z = min(boxMin.z, bMin.z);
					boxMax.x = max(boxMax.x, bMax.x);
					boxMax.y = max(boxMax.y, bMax.y);
					boxMax.z = max(boxMax.z, bMax.z);
				}

				drawBox(boxMin + (boxMax - boxMin)/2, (boxMax - boxMin), vec4(1,0,0,1));

				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
				glEnable(GL_LIGHTING);
			}

			// @Testing.
			#if 0
			{
				glDisable(GL_LIGHTING);

				Object* obj1 = &world->objects.at(1);
				Object* obj2 = &world->objects.at(0);
				Object* obj3 = &world->objects.at(2);

				Vec3 rayPos = obj1->pos;
				Vec3 rayDir = normVec3(obj3->pos - obj1->pos);

				Vec3 intersection, intersectionNormal;
				int objectIndex = castRay(rayPos, rayDir, &world->objects, 0, &intersection, &intersectionNormal);

				if(objectIndex) {
					glLineWidth(2);

					drawSphere(intersection, 0.1f, vec4(0,0,0,1));
					drawLine(obj1->pos, intersection, vec4(1,0,0,1));
					drawLine(intersection, intersection+intersectionNormal, vec4(0,0,1,1));

					int objectIndex = castRay(rayPos, rayDir, &world->objects, -1, &intersection, &intersectionNormal);
					if(objectIndex) {
						drawSphere(intersection, 0.1f, vec4(0,0,0,1));
						drawLine(obj1->pos, intersection, vec4(1,0,0,1));
						drawLine(intersection, intersection+intersectionNormal, vec4(0,0,1,1));
					}

					glLineWidth(1);
				} else {
					// drawLine(lp, lp + ld*5, vec4(0,0,0,1));
				}
			
				glEnable(GL_LIGHTING);
			}
			#endif

			// @EntityUI draw.
			{
				EntityUI* eui = &ad->entityUI;

				if(!eui->selectedObjects.empty()) {
					Object* object = world->objects + eui->selectedObjects.first();
					Geometry* geom = &object->geometry;
					Camera* cam = &world->camera;

					Vec3 pos = object->pos;
					Quat rot = object->rot;
					Vec3 dim = object->dim;

					bool multipleSelection = eui->selectedObjects.count > 1;
					if(multipleSelection) {
						pos = selectedObjectsGetCenter(&world->objects, &eui->selectedObjects);
					}

					glDisable(GL_LIGHTING);

					float uiAlpha = 0.7f;

					Vec4 rotationSegmentColor = vec4(hslToRgbFloat(vec3(0.5f,0.9f,0.5f)), 0.5f);
					Vec4 translationVectorColor = rotationSegmentColor;
					translationVectorColor.a = 1;
					Vec4 translationCenterBoxColor = vec4(0.5f,1);
					float translationCenterBoxColorMod = 0.1f;

					// Scale ui widgets with window size so they stay operable at smaller window sizes.
					float screenMod = (float)ws->biggestMonitorSize.w / (float)ws->clientRes.w;
					screenMod = powf(screenMod, 0.7f);
					
					float distToCam = lenVec3(cam->pos - pos) * screenMod;
					float translateLineWidth = 5;

					float translationArrowLength = distToCam * 0.15f;
					float translationArrowThickness = distToCam * 0.008f;
					float translationVectorThickness = translationArrowThickness*0.5f;

					float translationPlaneSize = translationArrowLength * 0.25f;

					float translationCenterBoxSize = distToCam * 0.015f;

					float rotationRingThickness = translationArrowThickness;
					float rotationRingRadius = translationArrowLength;

					float scaleArrowBoxDim = translationCenterBoxSize;
					float scaleArrowLineWidth = 3.0f;

					//

					if(eui->selectionMode == ENTITYUI_MODE_TRANSLATION) {
						Vec2 d = vec2(translationArrowThickness,translationArrowLength);
						Vec3 n = cam->pos - pos;
						Vec3 c[] = { vec3(1,0,0), vec3(0,1,0), vec3(0,0,1) };
						Vec3 colorSquares = vec3(1);

						Vec3 axis[3];
						for(int i = 0; i < 3; i++) {
							Vec3 a = vec3(0,0,0);
							a.e[i] = 1;

							if(eui->localMode) a = rot * a;

							if(dot(a, n) < 0) a *= -1;
							axis[i] = a;
						}

						Vec3 rayDir = mouseRayCast(ad->textureScreenRectFitted, input->mousePosNegative, cam);

						glDisable(GL_DEPTH_TEST);

						int contenderId = 0;
						int widgetId = 0;

						// Move axis.

						int axisIndex = 0;
						for(int i = 0; i < 3; i++) {
							widgetId++;

							float a = uiAlpha;
							{
								Vec3 pp = pos + axis[i]*d.h*0.5f;
								// Getting right angle with two cross products.
								Vec3 pn = normVec3(cross( cross(axis[i], n), axis[i])); 

								Vec3 intersection;
								float dist = linePlaneIntersection(cam->pos, rayDir, pp, pn, axis[i], d, &intersection);
								if(dist != -1) {
									axisIndex = i+1;

									contenderId = widgetId;
									if(!eui->guiHasFocus && eui->hotId == contenderId) a = 1;
								}
							} 

							if(eui->selectionState == ENTITYUI_ACTIVE) {
								if(eui->translateMode == TRANSLATE_MODE_AXIS)
									if(i == eui->axisIndex-1) a = 1;
							}

							drawArrow(pos, pos + axis[i]*d.h, n, d.w, vec4(c[i],a));
						}

						// Move plane.

						int planeIndex = 0;
						for(int i = 0; i < 3; i++) {
							widgetId++;

							// Whatever.
							float dim = translationPlaneSize;
							Vec3 edgePoint = pos + axis[(i+1)%3]*d.h + axis[(i+2)%3]*d.h;
							Vec3 diag = normVec3(edgePoint - pos);

							float squareDiag = sqrt(2 * (dim*dim));
							Vec3 p = pos + diag*(squareDiag/2) * 2;


							float a = uiAlpha;
							{
								Vec3 intersection;
								float dist = linePlaneIntersection(cam->pos, rayDir, p, axis[i], axis[(i+1)%3], vec2(dim), &intersection);
								if(dist != -1) {
									planeIndex = i+1;

									contenderId = widgetId;
									if(!eui->guiHasFocus && eui->hotId == contenderId) a = 1;
								}
							} 

							if(eui->selectionState == ENTITYUI_ACTIVE) {
								if(eui->translateMode == TRANSLATE_MODE_PLANE)
									if(axis[i] == eui->axis) a = 1;
							}

							drawPlane(p, axis[i], axis[(i+1)%3], vec2(dim), vec4(colorSquares,a), rect(0,0,1,1), getTexture("roundedSquare.png")->id);
						}

						// Move center.

						int centerIndex = 0;
						{
							widgetId++;

							Vec4 color = translationCenterBoxColor;
							Vec3 addedColor = color.rgb + vec3(translationCenterBoxColorMod);

							{
								Vec3 intersection;
								float distance = boxRaycastRotated(cam->pos, rayDir, pos, vec3(translationCenterBoxSize), rot, &intersection);
								if(distance != -1) {
									eui->centerOffset = intersection - pos;
									centerIndex = 1;

									contenderId = widgetId;
									if(!eui->guiHasFocus && eui->hotId == contenderId)
										color.rgb = addedColor;
								}
							} 

							if(eui->selectionState == ENTITYUI_ACTIVE) {
								if(eui->translateMode == TRANSLATE_MODE_CENTER)
									color.rgb = addedColor;
							}

							if(eui->localMode) {
								drawBox(pos, vec3(translationCenterBoxSize), &vm, rot, color);
							} else {
								drawBox(pos, vec3(translationCenterBoxSize), &vm, quat(), color);
							}
						}

						// Show current translation vector.

						if(eui->selectionState == ENTITYUI_ACTIVE) {
							// glEnable(GL_DEPTH_TEST);

							glLineWidth(translateLineWidth);

							drawArrow(eui->startPos, pos, n, translationVectorThickness, translationVectorColor);

							{
								Vec3 globalAxis[] = {vec3(1,0,0), vec3(0,1,0), vec3(0,0,1)};
								float l[3];
								for(int i = 0; i < 3; i++) {
									if(dot(globalAxis[i], eui->startPos-pos) > 0) globalAxis[i] *= -1;

									l[i] = lenVec3(projectPointOnLine(eui->startPos, globalAxis[i], pos) - eui->startPos);		
								}

								Vec3 p = eui->startPos;
								for(int i = 0; i < 3; i++) {
									drawArrow(p, p + globalAxis[i]*l[i], n, translationVectorThickness, translationVectorColor);
									p = p + globalAxis[i]*l[i];
								}
							}

							glLineWidth(1);
							// glDisable(GL_DEPTH_TEST);
						}

						glEnable(GL_DEPTH_TEST);

						// Should not be in draw code, but it's easiest for now.

						if(eui->selectionState != ENTITYUI_ACTIVE) {
							if(axisIndex || planeIndex || centerIndex) {
								if(centerIndex) {
									eui->axis = cam->pos - pos;
									eui->centerDistanceToCam = lenVec3(vectorToCam(pos, cam));
									eui->translateMode = TRANSLATE_MODE_CENTER;
								} else if(planeIndex) {
									eui->axis = axis[planeIndex-1];
									eui->axisIndex = planeIndex;
									eui->translateMode = TRANSLATE_MODE_PLANE;
								} else if(axisIndex) {
									eui->axis = axis[axisIndex-1];
									eui->axisIndex = axisIndex;
									eui->translateMode = TRANSLATE_MODE_AXIS;
								}

								eui->selectionState = ENTITYUI_HOT;
								for(int i = 0; i < 3; i++) {
									eui->axes[i] = axis[i];
								}
							} else {
								eui->selectionState = ENTITYUI_INACTIVE;
							}
						}

						eui->hotId = contenderId;
					}

					if(eui->selectionMode == ENTITYUI_MODE_ROTATION && !multipleSelection) {

						float thickness = rotationRingThickness;
						float radius = rotationRingRadius;
						// Vec3 axis[] = { vec3(1,0,0), vec3(0,1,0), vec3(0,0,1) };
						Vec3 c[] = { vec3(1,0,0), vec3(0,1,0), vec3(0,0,1) };

						Vec3 axis[3];
						for(int i = 0; i < 3; i++) {
							Vec3 a = vec3(0,0,0);
							a.e[i] = 1;

							if(eui->localMode) a = rot * a;

							// if(dot(a, n) < 0) a *= -1;
							axis[i] = a;
						}

						Vec3 rayDir = mouseRayCast(ad->textureScreenRectFitted, input->mousePosNegative, cam);

						// Find closest ring to camera.

						float closestDistance = FLT_MAX;
						int axisIndex = 0;
						if(eui->selectionState != ENTITYUI_ACTIVE) {
							for(int i = 0; i < 3; i++) {
								Vec3 intersection;
								float dist = linePlaneIntersection(cam->pos, rayDir, pos, axis[i], &intersection);
								if(dist != -1) {
									float distToObj = lenVec3(intersection - pos);
									if(valueBetween(distToObj, radius-thickness, radius)) {

										float distToCam = -(vm*intersection).z;
										if(distToCam < closestDistance) {
											closestDistance = distToCam;
											axisIndex = i+1;
										}
									}
								}
							}
						} else {
							axisIndex = eui->axisIndex;
						}
					
						// Also check closest distance to occlusion sphere.
						{
							if(eui->selectionState != ENTITYUI_ACTIVE) {
								Vec3 intersection;
								float sphereDistance = lineSphereIntersection(cam->pos, rayDir, pos, radius-thickness, &intersection);
								if(sphereDistance != -1) {
									float distToCam = -(vm*intersection).z;
									if(distToCam < closestDistance) axisIndex = 0;
								}
							}
						}

						// Occlude rings with sphere.
						drawSphere(pos, radius-thickness, vec4(0,0,0,0));

						// Draw rings.

						// Should switch to different depth buffer, but this works for now.
						glClear(GL_DEPTH_BUFFER_BIT);

						drawSphere(pos, radius-thickness, vec4(0,0,0,0));

						if(eui->selectionState != ENTITYUI_ACTIVE) {
							for(int i = 0; i < 3; i++) {
								drawRing(pos, axis[i], radius, thickness, vec4(c[i],uiAlpha));
							}
						}

						// Paint over hot/active ring.
						if(axisIndex && (!eui->guiHasFocus || eui->selectionState == ENTITYUI_ACTIVE)) {

							glDisable(GL_DEPTH_TEST);
	
							int i = axisIndex-1;
							drawRing(pos, axis[i], radius, thickness, vec4(c[i],1));

							if(eui->selectionState == ENTITYUI_ACTIVE) {
								Vec3 left = pos + normVec3(eui->objectDistanceVector)*radius;
								drawTriangleFan(pos, left, eui->currentRotationAngle, axis[i], rotationSegmentColor);
							}

							glEnable(GL_DEPTH_TEST);
						}

						// Should not be in draw code, but it's easiest for now.

						if(eui->selectionState != ENTITYUI_ACTIVE) {
							if(axisIndex) {
								eui->axisIndex = axisIndex;
								eui->axis = axis[axisIndex-1];
								eui->selectionState = ENTITYUI_HOT;
							} else {
								eui->selectionState = ENTITYUI_INACTIVE;
							}
						}
					}

					if(eui->selectionMode == ENTITYUI_MODE_SCALE && !multipleSelection) {

						Vec3 c[] = { vec3(0.9f,0,0), vec3(0,0.9f,0), vec3(0,0,0.9f) };

						Vec3 hotAxis = {};
						int hotAxisIndex = 0;
						for(int i = 0; i < 3; i++) {
							Vec3 color = c[i];

							float arrowLength = dim.e[i]/2 + scaleArrowBoxDim;

							Vec3 axis = vec3(0,0,0);
							axis.e[i] = 1;
							axis = rot*axis;
							if(dot(axis, cam->pos - pos) < 0) axis *= -1;
							Vec3 p = pos + axis * arrowLength;

							if(eui->selectionState != ENTITYUI_ACTIVE) {
								Vec3 rayDir = mouseRayCast(ad->textureScreenRectFitted, input->mousePosNegative, cam);
								Vec3 intersection;
								float distance = boxRaycastRotated(cam->pos, rayDir, p, vec3(scaleArrowBoxDim), rot, &intersection);
								if(distance != -1) {
									hotAxisIndex = i+1;
									hotAxis = normVec3(p - pos);
									eui->centerOffset = intersection - p;

									if(!eui->guiHasFocus)
										color += vec3(0.2f);
								}
							} else {
								if(eui->axisIndex-1 == i) color += vec3(0.2f);
							}

							glDisable(GL_DEPTH_TEST);
							glLineWidth(scaleArrowLineWidth);
							drawLine(pos, p, vec4(color,1));
							glLineWidth(1);
							drawBox(p, vec3(scaleArrowBoxDim), &vm, rot, vec4(color, 1));
							glEnable(GL_DEPTH_TEST);
						}

						// Should not be in draw code, but it's easiest for now.

						if(eui->selectionState != ENTITYUI_ACTIVE) {
							if(hotAxisIndex) {
								eui->axisIndex = hotAxisIndex;
								eui->axis = hotAxis;
								eui->selectionState = ENTITYUI_HOT;
							} else {
								eui->selectionState = ENTITYUI_INACTIVE;
							}
						}
					}


					glEnable(GL_CULL_FACE);
					glEnable(GL_LIGHTING);
				}
			}

			// glEnable(GL_CULL_FACE);
			// glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}

		glMatrixMode(GL_PROJECTION); glPopMatrix();
		glMatrixMode(GL_MODELVIEW); glPopMatrix();
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_LIGHTING);
		glDisable(GL_NORMALIZE);
	}

	{
		setClientViewport(ws, rect(0,0,ws->clientRes.w, ws->clientRes.h));
		setClientScissor(ws, rect(0,0,ws->clientRes.w, ws->clientRes.h));

		{
			Vec4 cDragOutline = vec4(0,1);
			Vec4 cDrag = vec4(0,0.8f,0.8f,0.2f);

			EntityUI* eui = &ad->entityUI;
			if(eui->dragSelectionActive) {
				Vec2 start = eui->dragSelectionStart;
				Vec2 end = input->mousePosNegative;

				float left = min(start.x, end.x);
				float right = max(start.x, end.x);
				float bottom = min(start.y, end.y);
				float top = max(start.y, end.y);

				Rect r = rectRound(rect(left, bottom, right, top));
				drawRect(r, cDrag);
				drawRectOutline(r, cDragOutline);

				Camera* cam = &ad->world.camera;
				Vec3 rayDirLeft = mouseRayCast(ad->textureScreenRectFitted, rectL(r), cam);
				Vec3 coneLeftDir = normVec3(cross(cam->ovecs.up, rayDirLeft));

				Vec3 rayDirRight = mouseRayCast(ad->textureScreenRectFitted, rectR(r), cam);
				Vec3 coneRightDir = normVec3(-cross(cam->ovecs.up, rayDirRight));

				Vec3 rayDirBottom = mouseRayCast(ad->textureScreenRectFitted, rectB(r), cam);
				Vec3 coneBottomDir = normVec3(-cross(cam->ovecs.right, rayDirBottom));

				Vec3 rayDirTop = mouseRayCast(ad->textureScreenRectFitted, rectT(r), cam);
				Vec3 coneTopDir = normVec3(cross(cam->ovecs.right, rayDirTop));

				eui->selectedObjects.clear();
				for(int i = 0; i < ad->world.objects.count; i++) {

					Object* obj = ad->world.objects + i;
					Vec3 objDir = normVec3(obj->pos - cam->pos);

					bool objectNotInCone = false;
					if(dot(objDir, coneLeftDir) > 0 || 
					   dot(objDir, coneRightDir) > 0 || 
					   dot(objDir, coneBottomDir) > 0 || 
					   dot(objDir, coneTopDir) > 0) {
						objectNotInCone = true;
					}

					if(!objectNotInCone) {
						eui->selectedObjects.push(i);
					}
				}
			}
		}


		NewGui* gui = &ad->gui;

		if(ad->entityUI.selectionState == ENTITYUI_ACTIVE) {
			for(int i = 0; i < Gui_Focus_Size; i++) gui->hotId[i] = 0;
		}

		{
			// @GuiSettings

			float fontHeight = ad->fontHeight;
			Font* font = getFont(ad->fontFile, ad->fontHeight, ad->fontFileBold, ad->fontFileItalic);
			
			AppColors c = ad->colors;

			Vec4 cEditCursor = c.text;
			Vec4 cEditSelection = vec4(hslToRgbFloat(0.6f,0.4f,0.4f),1);

			float buttonRounding = ad->fontHeight * 0.3f;
			float textPadding = font->height*0.4f;

			BoxSettings bs = boxSettings(c.background, 0, c.outline);
			TextSettings ts = textSettings(font, c.text);

			BoxSettings bous = boxSettings(c.button, buttonRounding, c.outline);
			bous.vertGradientOffset = 0.04f;
			TextBoxSettings bus = textBoxSettings(ts, bous);

			gui->textSettings = ts;
			gui->boxSettings = bs;
			gui->textBoxSettings = textBoxSettings(ts, bs);
			gui->buttonSettings = bus;

			TextBoxSettings etbs = gui->textBoxSettings;
			etbs.boxSettings.color = c.edit;
			etbs.boxSettings.roundedCorner = 0;
			gui->editSettings = textEditSettings(etbs, vec4(0,0,0,0), gui->editText, ESETTINGS_SINGLE_LINE | ESETTINGS_START_RIGHT, 1, 1.1f, cEditSelection, cEditCursor, 6, textPadding);

			float sw = fontHeight*1.0f;
			gui->sliderSettings = sliderSettings(etbs, sw, sw, 0, 0, fontHeight*0.20f, c.button, vec4(0,0,0,0));
			gui->sliderSettings.borderColor = c.outline;

			gui->popupSettings = boxSettings(c.background, 0, c.outline);

			gui->comboBoxSettings = textBoxSettings(gui->textSettings, boxSettings(c.edit, 0, c.outline), textPadding);

			BoxSettings cbs = boxSettings(c.edit, buttonRounding, c.outline);
			gui->checkBoxSettings = checkBoxSettings(cbs, c.button, 0.5f);
		}

		// @Menu.
		{
			int fontHeight = roundInt(ad->fontHeight * 1.1f);
			float menuHeight = roundFloat(fontHeight * 1.5f);
			float padding = ad->fontHeight * 1.4;
			float border = 1;
			float regionGap = padding/2;
			Vec4 cMenu = ad->colors.menu;

			float separatorWidth = padding*0.5f;
			float separatorOffset = padding*0.2f;
			float statsSeparatorWidth = padding;
			Vec4 cSeparator = ad->colors.outlineBright;

			Font* font = getFont(ad->fontFile, fontHeight, ad->fontFileBold, ad->fontFileItalic);

			Rect mr = rectRSetB(rectTLDim(0,0, ws->clientRes.w, ws->clientRes.h), menuHeight);
			newGuiSetHotAllMouseOver(gui, mr, gui->zLevel);

			ad->menuHeight = menuHeight;
			ad->menuFontHeight;
			ad->menuRect = mr;


			char* s;

			drawRect(mr, cMenu);

			TextBoxSettings tbs = textBoxSettings(gui->textSettings, boxSettings(cMenu));
			TextBoxSettings tbsActive = gui->textBoxSettings;
			tbs.textSettings.font = font;
			tbsActive.textSettings.font = font;

			TextSettings menuTextSettings = tbs.textSettings;


			Rect r;
			Vec2 p = rectTL(mr);

			char* menuTitles[] = {"File", "Settings"};
			char* menuTags[] = {"FileMenu", "SettingsMenu"};
			float menuWidths[] = {ad->fontHeight * 7, ad->fontHeight * 11};

			int menuCount = 2;
			for(int i = 0; i < menuCount; i++) {
				char* s = menuTitles[i];
				r = rectTLDim(p, vec2(getTextDim(s, font).w + padding, menuHeight)); p.x += rectW(r);

				if(newGuiQuickPButton(gui, r, s, &tbs) || 
				   (gui->menuActive && pointInRectEx(input->mousePosNegative, r) && gui->menuId != newGuiCurrentId(gui))) {

					gui->popupStackCount = 0;

					PopupData pd = {};
					pd.type = POPUP_TYPE_OTHER;
					pd.id = newGuiCurrentId(gui);
					strCpy(pd.name, menuTags[i]);
					pd.p = rectBL(r);
					pd.width = menuWidths[i];
					pd.settings = gui->boxSettings;
					pd.border = vec2(5,5);
					pd.rSource = r;

					newGuiPopupPush(gui, pd);

					gui->menuId = newGuiCurrentId(gui);
					gui->menuActive = true;
				}
				if(gui->menuActive && newGuiCurrentId(gui) == gui->menuId) {
					newGuiQuickTextBox(gui, r, s, &tbsActive);
				}
			}

			// @MenuButtons.
			{
				float buttonWidth = menuHeight;
				float buttonMargin = buttonWidth * 0.3f;
				Vec4 cButtonActive = vec4(1,1);
				Vec4 cButtonInactive = vec4(0.7f,1);
				// Vec4 cSeparator = gui->buttonSettings.boxSettings.borderColor;

				p.x += regionGap;

				EntityUI* eui = &ad->entityUI;

				Rect r;
				
				// Make Screenshot.

				{
					r = rectTLDim(p, vec2(buttonWidth)); p.x += buttonWidth;
					if(!ad->tracerFinished) gui->setInactive = true;

					Vec4 c = gui->setInactive ? cButtonInactive : cButtonActive;

					if(newGuiQuickButton(gui, r, "", &tbs)) {
						openScreenshotDialog(&ad->dialogData);
					}
					rectExpand(&r, vec2(-buttonMargin));

					drawRect(rectRound(r), c, rect(0,0,1,1), getTexture("screenshotIcon.png")->id);
				}

				// Open Folder.

				r = rectTLDim(p, vec2(buttonWidth)); p.x += buttonWidth;
				if(newGuiQuickButton(gui, r, "", &tbs)) {
					shellExecuteNoWindow(fillString("explorer.exe %s", Screenshot_Folder));
				}
				rectExpand(&r, vec2(-buttonMargin));
				drawRect(rectRound(r), cButtonActive, rect(0,0,1,1), getTexture("folderIcon.png")->id);

				// Fit to Screen.

				r = rectTLDim(p, vec2(buttonWidth)); p.x += buttonWidth;
				if(newGuiQuickPButton(gui, r, "", &tbs)) {
					ad->fitToScreen = !ad->fitToScreen;
				}
				rectExpand(&r, vec2(-buttonMargin));
				drawRect(rectRound(r), ad->fitToScreen?cButtonActive:cButtonInactive, rect(0,0,1,1), getTexture("fitToScreenIcon.png")->id);

				// Hide panels.

				r = rectTLDim(p, vec2(buttonWidth)); p.x += buttonWidth;
				if(newGuiQuickPButton(gui, r, "", &tbs)) {
					ad->showPanels = !ad->showPanels;
				}
				rectExpand(&r, vec2(-buttonMargin));
				drawRect(rectRound(r), ad->showPanels?cButtonActive:cButtonInactive, rect(0,0,1,1), getTexture("hidePanelsIcon.png")->id);

				{
					p.x += separatorWidth * 0.5f;
					float x = roundFloat(p.x) + 0.5f;
					drawLine(vec2(x, p.y - separatorOffset), vec2(x, p.y-menuHeight + separatorOffset), cSeparator);
					p.x += separatorWidth * 0.5f;
				}

				// Local mode.

				r = rectTLDim(p, vec2(buttonWidth)); p.x += buttonWidth;
				if(newGuiQuickPButton(gui, r, "", &tbs)) {
					eui->localMode = !eui->localMode;
				}
				rectExpand(&r, vec2(-buttonMargin));
				drawRect(rectRound(r), eui->localMode?cButtonActive:cButtonInactive, rect(0,0,1,1), getTexture("coordIcon")->id);

				// Selection mode.

				char* icons[] = { "translationIcon.png", "rotationIcon.png", 
								  "scalingIcon.png" };

				for(int i = 0; i < ENTITYUI_MODE_SCALE + 1; i++) {
					r = rectTLDim(p, vec2(buttonWidth)); p.x += buttonWidth;

					bool multipleSelection = eui->selectedObjects.count > 1;

					if(multipleSelection && (i == ENTITYUI_MODE_ROTATION || i == ENTITYUI_MODE_SCALE)) {
						gui->setInactive = true;
					}

					if(eui->selectionMode == i) newGuiIncrementId(gui);
					else if(newGuiQuickPButton(gui, r, "", &tbs)) {
						eui->selectionMode = i;
					}
					rectExpand(&r, vec2(-buttonMargin));
					drawRect(rectRound(r), eui->selectionMode==i?cButtonActive:cButtonInactive, rect(0,0,1,1), getTexture(icons[i])->id);
				}

				p.x += regionGap;
			}

			// Stats.
			if(!ad->drawSceneWired)
			{
				Rect rStats = rect(p-vec2(0,menuHeight), rectTR(mr));
				rStats.right -= padding/2;

				RaytraceSettings* settings = &ad->settings;

				int samplesPerPixel;
				if(settings->sampleMode == SAMPLE_MODE_GRID) {
					samplesPerPixel = settings->sampleCount;
				} else {
					samplesPerPixel = settings->sampleCount/pow(ad->settings.sampleGridWidth,2);
				}

				i64 totalSampleCount = ((i64)settings->texDim.x * (i64)settings->texDim.h * (i64)samplesPerPixel);
				char* stats[] = {
					fillString("Pixels: %i.", settings->texDim.x * settings->texDim.h),
					fillString("Samples/Pixel: %i.", samplesPerPixel), 
					fillString("Samples: %i.", (totalSampleCount)),
					fillString("Time: %fs", (float)ad->processTime),
					fillString("Time/Pixel: %fms", (float)(ad->processTime/(settings->texDim.x*settings->texDim.y)*1000000)),
				};

				float statsWidth = 0;
				for(int i = 0; i < arrayCount(stats); i++) {
					statsWidth += getTextDim(stats[i], font).w;
					if(i <= arrayCount(stats)) statsWidth += statsSeparatorWidth;
				}

				Vec2 tp = rectR(mr) - vec2(padding/2, 0);
				if(mr.right - p.x + padding/2 < statsWidth) tp.x = rStats.left + (statsWidth - (padding));

				newGuiScissorPush(gui, rStats);
				for(int i = arrayCount(stats)-1; i >= 0; i--) {
					
					drawText(stats[i], tp, vec2i(1,0), menuTextSettings);
					tp.x -= getTextDim(stats[i], font).w;

					if(i > 0) {
						tp.x -= statsSeparatorWidth * 0.5f;
						drawLineV(tp + vec2(0,separatorOffset*2), tp - vec2(0,separatorOffset*2), cSeparator);
						tp.x -= statsSeparatorWidth * 0.5f;
					}
				}
				newGuiScissorPop(gui);
			}
		}


		bindFrameBuffer(FRAMEBUFFER_2dPanels);
		glEnable(GL_BLEND);
		glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);


		// @Panel

		Vec2 panelOffset = vec2(0,ad->menuHeight);
		float panelMargin = roundFloat(ad->fontHeight*0.3f);
		float panelBorder = roundFloat(ad->fontHeight*0.5f);
		float panelWidthMax = ws->clientRes.w*0.5f;
		float panelWidthMin = 100;

		Vec2 panelDim;

		clamp(&ad->panelWidthLeft, panelWidthMin, panelWidthMax);
		clamp(&ad->panelWidthRight, panelWidthMin, panelWidthMax);

		Vec2i res = ws->clientRes;

		// Hide panel animation.
		{
			float speed = 4.0f;
			if(!ad->showPanels) ad->hidePanelAnim += ad->dt * speed;
			else ad->hidePanelAnim -= ad->dt * speed*0.75f;

			clamp(&ad->hidePanelAnim, 0, 1);
			panelOffset.x = -ad->hidePanelAnim * (max(ad->panelWidthLeft, ad->panelWidthRight));

			if(ad->hidePanelAnim == 1) ad->panelsVisible = false;
			else ad->panelsVisible = true;
		}

		panelDim = vec2(ad->panelWidthLeft,ad->panelHeightLeft);
		Rect rectPanelLeft = rectTLDim(vec2(panelOffset.x,-panelOffset.y), panelDim);
		ad->panelLeftRect = rectPanelLeft;

		panelDim = vec2(ad->panelWidthRight,ad->panelHeightRight);
		Rect rectPanelRight = rectTRDim(vec2(res.w,0) + vec2(-panelOffset.x,-panelOffset.y), panelDim);
		ad->panelRightRect = rectPanelRight;

		bool entityPanelActive = ad->entityUI.selectedObjects.count > 0;

		// Panel fade animation.
		{
			bool inc = false;

			if(ad->entityUI.selectionState == ENTITYUI_ACTIVE && 
			   (pointInRect(input->mousePosNegative, rectPanelLeft) || 
			   (entityPanelActive && (pointInRect(input->mousePosNegative, rectPanelRight) ||
			                          pointInRect(input->mousePosNegative, ad->panelColorPickerRect))))) {

				inc = true;
			}

			float speed = 1.0f;

			if(!inc) speed *= -2.5f;
			ad->panelAlphaFadeState += ad->dt * speed;

			ad->panelAlphaFadeState = clamp(ad->panelAlphaFadeState, 0, 0.8f);
			ad->panelAlpha = 1-pow(ad->panelAlphaFadeState,2);
		}

		glDisable(GL_DEPTH_TEST);

		// Left panel.
		if(ad->drawSceneWired && ad->panelsVisible)
		{
			glDisable(GL_DEPTH_TEST);

			newGuiSetHotAllMouseOver(gui, rectPanelLeft, gui->zLevel);

			// Resize panel width.
			{
				Rect r = rectRSetL(rectPanelLeft, panelBorder);

				int result = newGuiGoDragAction(gui, r, gui->zLevel, Gui_Focus_MLeft);
				if(result == 1) {
					gui->mouseAnchor.x = rectPanelLeft.left + (input->mousePos.x-rectPanelLeft.right);
				}
				if(result) {
					newGuiSetCursor(gui, IDC_SIZEWE);
					ad->panelWidthLeft = input->mousePos.x - gui->mouseAnchor.x;

					// Clamp Width.
					clamp(&ad->panelWidthLeft, panelWidthMin, panelWidthMax);

					rectPanelLeft.right = rectPanelLeft.left + ad->panelWidthLeft;
					ad->panelLeftRect = rectPanelLeft;
				}
				if(newGuiIsHot(gui)) {
					newGuiSetCursor(gui, IDC_SIZEWE);
				}
			}

			newGuiQuickBox(gui, rectPanelLeft);

			{
				// newGuiScissorPush(gui, rectExpand(rectPanelLeft, vec2(-2)));

				Rect pri = rectExpand(rectPanelLeft, -vec2(panelBorder*2));
				newGuiScissorPush(gui, rectExpand(pri, vec2(0)));

				Font* font = gui->textSettings.font;

				float elementHeight = font->height * 1.5f;
				float elementWidth = rectW(pri);
				Vec2 padding = vec2(panelMargin-1, panelMargin-1);

				Vec2 p = rectTL(pri);
				float eh = elementHeight;
				float ew = elementWidth;
				Vec2 pad = padding;

				{
					RaytraceSettings* settings = &ad->settings;
					World* world = &ad->world;

					TextSettings headerTextSettings = textSettings(gui->textSettings.font, gui->textSettings.color, TEXTSHADOW_MODE_SHADOW, 1.0f, ad->colors.outline);

					TextBoxSettings headerSettings = textBoxSettings(headerTextSettings, boxSettings());
					float headerHeight = eh * 1.2f;
					float separatorHeight = font->height * 0.3f;
					Vec4 cSeparatorDark = ad->colors.ledgeDark;
					Vec4 cSeparatorBright = ad->colors.ledgeBright;

					Rect r;
					char* s;
					QuickRow qr;

					//

					r = rectTLDim(p, vec2(ew, headerHeight)); p.y -= headerHeight+pad.y;
					newGuiQuickTextBox(gui, r, "<b>Render Settings<b>", vec2i(0,0), &headerSettings);

					{
						char* labels[] = {"Texture dim", "Sample mode", "Sample count", "Max ray bounces"};
						int labelIndex = 0;
						float labelsMaxWidth = 0;
						for(int i = 0; i < arrayCount(labels); i++) {
							labelsMaxWidth = max(labelsMaxWidth, getTextDim(labels[i], font).w);
						}
						labelsMaxWidth += padding.w;

						r = rectTLDim(p, vec2(ew, eh)); p.y -= eh+pad.y;
						qr = quickRow(r, pad.x, labelsMaxWidth, 0, 0);
						newGuiQuickText(gui, quickRowNext(&qr), labels[labelIndex++], vec2i(-1,0));
						newGuiQuickTextEdit(gui, quickRowNext(&qr), &settings->texDim.w);
						newGuiQuickTextEdit(gui, quickRowNext(&qr), &settings->texDim.h);
						clampInt(&settings->texDim.w, arrayCount(ad->threadData), 10000);
						clampInt(&settings->texDim.h, arrayCount(ad->threadData), 10000);

						r = rectTLDim(p, vec2(ew, eh)); p.y -= eh+pad.y;
						qr = quickRow(r, pad.x, labelsMaxWidth, 0);
						newGuiQuickText(gui, quickRowNext(&qr), labels[labelIndex++], vec2i(-1,0));
						newGuiQuickComboBox(gui, quickRowNext(&qr), &settings->sampleMode, sampleModeStrings, arrayCount(sampleModeStrings));

						r = rectTLDim(p, vec2(ew, eh)); p.y -= eh+pad.y;
						qr = quickRow(r, pad.x, labelsMaxWidth, 0);
						newGuiQuickText(gui, quickRowNext(&qr), labels[labelIndex++], vec2i(-1,0));
						newGuiQuickTextEdit(gui, quickRowNext(&qr), &settings->sampleCountWanted);

						settings->sampleCountGrid = clampMinInt(settings->sampleCountGrid, 1);

						r = rectTLDim(p, vec2(ew, eh)); p.y -= eh+pad.y;
						qr = quickRow(r, pad.x, labelsMaxWidth, 0);
						newGuiQuickText(gui, quickRowNext(&qr), labels[labelIndex++], vec2i(-1,0));
						newGuiQuickTextEdit(gui, quickRowNext(&qr), &settings->rayBouncesMax);

						settings->rayBouncesMax = clampMinInt(settings->rayBouncesMax, 1);
					}

					{
						p.y -= separatorHeight/2;
						drawLineH(p+vec2(0,0.5f), p+vec2(ew,0)+vec2(0,0.5f), cSeparatorDark);
						drawLineH(p+vec2(0,-0.5f), p+vec2(ew,0)+vec2(0,-0.5f), cSeparatorBright);
						p.y -= separatorHeight/2+pad.y;
					}

					// 

					r = rectTLDim(p, vec2(ew, headerHeight)); p.y -= headerHeight+pad.y;
					newGuiQuickTextBox(gui, r, "<b>Scene<b>", vec2i(0,0), &headerSettings);

					{
						World* world = &ad->world;
						Camera* cam = &world->camera;
						EntityUI* eui = &ad->entityUI;

						char* labels[] = {"Cam position", "Cam rotation", "Cam fov", "Ambient light", "Global light", "Global light dir", "Aperture size", "Focal distance"};
						int labelIndex = 0;
						float labelsMaxWidth = 0;
						for(int i = 0; i < arrayCount(labels); i++) {
							labelsMaxWidth = max(labelsMaxWidth, getTextDim(labels[i], font).w);
						}
						labelsMaxWidth += padding.w;

						r = rectTLDim(p, vec2(ew, eh)); p.y -= eh+pad.y;
						qr = quickRow(r, pad.x, labelsMaxWidth, 0,0,0);
						newGuiQuickText(gui, quickRowNext(&qr), labels[labelIndex++], vec2i(-1,0));
						newGuiQuickTextEdit(gui, quickRowNext(&qr), &cam->pos.x);
						newGuiQuickTextEdit(gui, quickRowNext(&qr), &cam->pos.y);
						newGuiQuickTextEdit(gui, quickRowNext(&qr), &cam->pos.z);

						r = rectTLDim(p, vec2(ew, eh)); p.y -= eh+pad.y;
						qr = quickRow(r, pad.x, labelsMaxWidth, 0,0);
						newGuiQuickText(gui, quickRowNext(&qr), labels[labelIndex++], vec2i(-1,0));
						newGuiQuickTextEdit(gui, quickRowNext(&qr), &cam->rot.x);
						newGuiQuickTextEdit(gui, quickRowNext(&qr), &cam->rot.y);

						r = rectTLDim(p, vec2(ew, eh)); p.y -= eh+pad.y;
						qr = quickRow(r, pad.x, labelsMaxWidth, 0);
						newGuiQuickText(gui, quickRowNext(&qr), labels[labelIndex++], vec2i(-1,0));
						newGuiQuickSlider(gui, quickRowNext(&qr), &world->camera.fov, 20, 150);

						{
							p.y -= separatorHeight/2;
							drawLineH(p+vec2(0,0.5f), p+vec2(ew,0)+vec2(0,0.5f), cSeparatorDark);
							drawLineH(p+vec2(0,-0.5f), p+vec2(ew,0)+vec2(0,-0.5f), cSeparatorBright);
							p.y -= separatorHeight/2+pad.y;
						}

						r = rectTLDim(p, vec2(ew, eh)); p.y -= eh+pad.y;
						qr = quickRow(r, pad.x, labelsMaxWidth, 0,0,0);
						newGuiQuickText(gui, quickRowNext(&qr), labels[labelIndex++], vec2i(-1,0));
						newGuiQuickSlider(gui, quickRowNext(&qr), &world->defaultEmitColor.r, 0, 1);
						newGuiQuickSlider(gui, quickRowNext(&qr), &world->defaultEmitColor.g, 0, 1);
						newGuiQuickSlider(gui, quickRowNext(&qr), &world->defaultEmitColor.b, 0, 1);

						r = rectTLDim(p, vec2(ew, eh)); p.y -= eh+pad.y;
						qr = quickRow(r, pad.x, labelsMaxWidth, 0,0,0);
						newGuiQuickText(gui, quickRowNext(&qr), labels[labelIndex++], vec2i(-1,0));
						newGuiQuickSlider(gui, quickRowNext(&qr), &world->globalLightColor.r, 0, 10);
						newGuiQuickSlider(gui, quickRowNext(&qr), &world->globalLightColor.g, 0, 10);
						newGuiQuickSlider(gui, quickRowNext(&qr), &world->globalLightColor.b, 0, 10);




						// r = rectTLDim(p, vec2(ew, eh)); p.y -= eh+pad.y;
						// qr = quickRow(r, pad.x, labelsMaxWidth, 0,0,0);
						// newGuiQuickText(gui, quickRowNext(&qr), labels[labelIndex++], vec2i(-1,0));
						// newGuiQuickSlider(gui, quickRowNext(&qr), &world->globalLightDir.x, -1, 1);
						// newGuiQuickSlider(gui, quickRowNext(&qr), &world->globalLightDir.y, -1, 1);
						// newGuiQuickSlider(gui, quickRowNext(&qr), &world->globalLightDir.z, -1, 1);


						// {
						// 	Vec3 dir = normVec3(world->globalLightDir);
						// 	Vec2 dirXY = dir.xy == vec2(0,0) ? vec2(1,0) : normVec2(dir.xy);

						// 	float angleXY2 = atan2(-1,0) - atan2(dirXY.x, dirXY.y);
						// 	if(angleXY2 < 0) angleXY2 += 2*M_PI;
						// 	angleXY2 -= M_PI;

						// 	float angleZ2 = asin(dir.z);

						// 	eui->globalLightDirTemp = vec2(angleXY2, angleZ2);
						// }

						r = rectTLDim(p, vec2(ew, eh)); p.y -= eh+pad.y;
						qr = quickRow(r, pad.x, labelsMaxWidth, 0,0);
						newGuiQuickText(gui, quickRowNext(&qr), labels[labelIndex++], vec2i(-1,0));
						newGuiQuickSlider(gui, quickRowNext(&qr), &world->globalLightRot.x, 0, 360);
						newGuiQuickSlider(gui, quickRowNext(&qr), &world->globalLightRot.y, -90, 90);
						
						{
							worldCalcLightDir(world);

							#if 0
							{
								Vec3 dir = normVec3(world->globalLightDir);
								Vec2 dirXY = dir.xy == vec2(0,0) ? vec2(1,0) : normVec2(dir.xy);
								// float angleXY2 = -(atan2(dirXY.x, dirXY.y) - M_PI_2);
								// angleXY2 += M_PI;
								// float angleXY2 = (atan2(dirXY.x, dirXY.y));
								// if (angleXY2 < 0) angleXY2 = M_2PI - abs(angleXY2);

								float angleXY2 = atan2(-1,0) - atan2(dirXY.x, dirXY.y);
								if(angleXY2 < 0) angleXY2 += 2*M_PI;
								angleXY2 -= M_PI;


								// angle = atan2(vector2.y, vector2.x) - atan2(vector1.y, vector1.x);
								// and you may want to normalize it to the range 0 .. 2 * Pi:

								// if (angle < 0) angle += 2 * M_PI;


								// if(angleXY2 > M_PI) {
								// 	angleXY2 = -((M_PI_2 - (angleXY2 - M_PI)) + M_PI_2);
								// 	if(angleXY2 > 0) angleXY2 = -angleXY2;
								// }


								float angleZ2 = asin(dir.z);

								// printf("%f %f %f,  %f %f, %f %f\n", PVEC3(dir), angleXY, angleZ, angleXY2, angleZ2);

							}
							#endif
						}



						{
							p.y -= separatorHeight/2;
							drawLineH(p+vec2(0,0.5f), p+vec2(ew,0)+vec2(0,0.5f), cSeparatorDark);
							drawLineH(p+vec2(0,-0.5f), p+vec2(ew,0)+vec2(0,-0.5f), cSeparatorBright);
							p.y -= separatorHeight/2+pad.y;
						}

						r = rectTLDim(p, vec2(ew, eh)); p.y -= eh+pad.y;
						qr = quickRow(r, pad.x, labelsMaxWidth, 0);
						newGuiQuickText(gui, quickRowNext(&qr), labels[labelIndex++], vec2i(-1,0));
						newGuiQuickSlider(gui, quickRowNext(&qr), &world->apertureSize, 0, 5);

						{
							r = rectTLDim(p, vec2(ew, eh)); p.y -= eh+pad.y;
							qr = quickRow(r, pad.x, labelsMaxWidth, 0, rectH(r), rectH(r));
							newGuiQuickText(gui, quickRowNext(&qr), labels[labelIndex++], vec2i(-1,0));

							newGuiQuickTextEdit(gui, quickRowNext(&qr), &world->focalPointDistance);

							Vec4 bc = ad->colors.background;
							Vec4 c = vec4(gui->textSettings.color.rgb, 0.3f);
							Vec4 cActive = vec4(c.rgb, 0.8f);
							float margin = rectH(r)*0.2f;
							{
								Rect pr = quickRowNext(&qr);
								Rect scissor = getRectScissor(gui->scissor, pr);
								if(newGuiGoButtonAction(gui, scissor)) {
									world->lockFocalPoint = !world->lockFocalPoint;
								}

								Texture* tex = getTexture("lock.png");
								drawRect(pr, bc + newGuiHotActiveColorMod(newGuiIsHot(gui), false));

								Vec4 col = !world->lockFocalPoint ? c : cActive;
								drawRect(rectExpand(pr, vec2(-margin)), col, rect(0,0,1,1), tex->id);
							}

							bool updateFocalDistance = false;
							Rect r = quickRowNext(&qr);
							Rect scissor = getRectScissor(gui->scissor, r);
							int event = newGuiGoDragAction(gui, scissor, gui->zLevel, Gui_Focus_MLeft);

							{
								drawRect(r, bc + newGuiHotActiveColorMod(newGuiIsHot(gui) && event < 1, false));

								Rect pr = rectExpand(r, vec2(-margin));

								float off = (rectH(pr)/2) * 0.3f;
								float w = rectH(pr)*0.1f;

								int index = 0;
								Rect lines[] = {
									rectSetH(rect(rectL(pr), rectCen(pr)+vec2(-off,0)), w),
									rectSetH(rect(rectCen(pr)+vec2(off,0), rectR(pr)), w),
									rectSetW(rect(rectB(pr), rectCen(pr)+vec2(0,-off)), w),
									rectSetW(rect(rectCen(pr)+vec2(0,off), rectT(pr)), w),
								};
								for(int i = 0; i < arrayCount(lines); i++) 
									drawRect(lines[index++], cActive);
							}

							if(event >= 1) {
								newGuiSetCursor(gui, IDC_CROSS);

								Vec3 rayDir = mouseRayCast(ad->textureScreenRectFitted, input->mousePosNegative, cam);

								Vec3 intersection;
								int objectIndex = castRay(cam->pos, rayDir, &ad->world.objects, 0, &intersection);
								if(objectIndex) {
									world->focalPoint = intersection;
									updateFocalDistance = true;
								}
							}

							// Update focal distance.
							if(updateFocalDistance || world->lockFocalPoint) {
								float distance = linePlaneIntersection(world->focalPoint, -cam->ovecs.dir, cam->pos, cam->ovecs.dir);

								world->focalPointDistance = distance;
							}
						}

						{
							p.y -= separatorHeight/2;
							drawLineH(p+vec2(0,0.5f), p+vec2(ew,0)+vec2(0,0.5f), cSeparatorDark);
							drawLineH(p+vec2(0,-0.5f), p+vec2(ew,0)+vec2(0,-0.5f), cSeparatorBright);
							p.y -= separatorHeight/2+pad.y;
						}

						{
							r = rectTLDim(p, vec2(ew, eh)); p.y -= eh+pad.y;
							qr = quickRow(r, pad.x, 0.0f, 0.0f);

							if(newGuiQuickButton(gui, quickRowNext(&qr), "Insert default")) {
								eui->objectTempArray.clear();
								eui->objectTempArray.push(defaultObject());
								insertObjects(&ad->world, &eui->objectTempArray, &eui->selectedObjects, &eui->objectsEdited, &eui->history, false);
							}

							if(newGuiQuickButton(gui, quickRowNext(&qr), "Insert copies")) {
								insertObjects(&ad->world, &eui->objectCopies, &eui->selectedObjects, &eui->objectsEdited, &eui->history, false);
							}
						}

						{
							char* text = "Render";
							float padding = ad->fontHeight * 0.9f;
							float renderButtonWidth = getTextDim(text, font).w + padding*2;
							renderButtonWidth = min(ew, renderButtonWidth);
							float height = eh*1.2f;

							r = rectTLDim(p, vec2(ew, height)); p.y -= height+pad.y;
							rectSetW(&r, renderButtonWidth);
							if(newGuiQuickButton(gui, r, text)) {
								if(!ad->activeProcessing) ad->startProcessing = true;
							}
						}
					}

				}

				ad->panelHeightLeft = rectPanelLeft.top - p.y - pad.y + panelBorder;

				newGuiScissorPop(gui);
			}
		}

		// @RightPanel.
		if(entityPanelActive && ad->drawSceneWired && ad->panelsVisible)
		{
			newGuiSetHotAllMouseOver(gui, rectPanelRight, gui->zLevel);

			// Resize panel width.
			{
				Rect r = rectRSetR(rectPanelRight, panelBorder);

				int result = newGuiGoDragAction(gui, r, gui->zLevel, Gui_Focus_MLeft);
				if(result == 1) {
					// gui->mouseAnchor.x = rectPanelRight.right + (input->mousePos.x-rectPanelRight.left);
					gui->mouseAnchor.x = input->mousePos.x - rectPanelRight.left;
				}
				if(result) {
					newGuiSetCursor(gui, IDC_SIZEWE);
					float panelLeft = input->mousePos.x - gui->mouseAnchor.x;
					ad->panelWidthRight = rectPanelRight.right - panelLeft;

					// Clamp Width.
					clamp(&ad->panelWidthRight, panelWidthMin, panelWidthMax);

					rectPanelRight.left = rectPanelRight.right - ad->panelWidthRight;
					ad->panelRightRect = rectPanelRight;
				}
				if(newGuiIsHot(gui)) {
					newGuiSetCursor(gui, IDC_SIZEWE);
				}
			}

			newGuiQuickBox(gui, rectPanelRight);

			{
				newGuiScissorPush(gui, rectExpand(rectPanelRight, vec2(-2)));

				Rect pri = rectExpand(rectPanelRight, -vec2(panelBorder*2));

				Font* font = gui->textSettings.font;

				float elementHeight = font->height * 1.5f;
				float elementWidth = rectW(pri);
				Vec2 padding = vec2(panelMargin-1, panelMargin-1);

				float separatorHeight = font->height * 0.3f;
				Vec4 cSeparatorDark = ad->colors.ledgeDark;
				Vec4 cSeparatorBright = ad->colors.ledgeBright;

				float headerHeight = elementHeight * 1.2f;

				Vec4 textColor = gui->textSettings.color;
				Vec4 diffColor = vec4(1,0,1,1);

				Vec2 p = rectTL(pri);
				float eh = elementHeight;
				float ew = elementWidth;
				Vec2 pad = padding;

				{
					TextSettings headerTextSettings = textSettings(gui->textSettings.font, gui->textSettings.color, TEXTSHADOW_MODE_SHADOW, 1.0f, vec4(0, 1));
					TextBoxSettings headerSettings = textBoxSettings(headerTextSettings, boxSettings());

					Rect r;
					char* s;
					char* t;
					QuickRow qr;

					RaytraceSettings* settings = &ad->settings;
					World* world = &ad->world;
					EntityUI* eui = &ad->entityUI;

					bool multipleSelection = eui->selectedObjects.count > 1;

					Object objectDiffs = {};

					// if(eui->selectionChanged) {
					{
						eui->multiChangeObject = {};
						eui->selectionChanged = false;

						Object o = world->objects[eui->selectedObjects[0]];

						// Find values that are identical on all selected objects.
						// We need -1 values can naturally be 0.
						for(int i = 1; i < eui->selectedObjects.count; i++) {
							Object* obj = world->objects + eui->selectedObjects[i];
							if(o.pos.x != obj->pos.x) o.pos.x = -1;
							if(o.pos.y != obj->pos.y) o.pos.y = -1;
							if(o.pos.z != obj->pos.z) o.pos.z = -1;
							if(o.rot.w != obj->rot.w) o.rot.w = -1;
							if(o.rot.x != obj->rot.x) o.rot.x = -1;
							if(o.rot.y != obj->rot.y) o.rot.y = -1;
							if(o.rot.z != obj->rot.z) o.rot.z = -1;
							if(o.dim.x != obj->dim.x) o.dim.x = -1;
							if(o.dim.y != obj->dim.y) o.dim.y = -1;
							if(o.dim.z != obj->dim.z) o.dim.z = -1;
							if(o.color.r != obj->color.r) o.color.r = -1;
							if(o.color.g != obj->color.g) o.color.g = -1;
							if(o.color.b != obj->color.b) o.color.b = -1;
							if(o.geometry.type != obj->geometry.type) o.geometry.type = -1;
							if(o.material.emitColor.r != obj->material.emitColor.r) o.material.emitColor.r = -1;
							if(o.material.emitColor.g != obj->material.emitColor.g) o.material.emitColor.g = -1;
							if(o.material.emitColor.b != obj->material.emitColor.b) o.material.emitColor.b = -1;
							if(o.material.reflectionMod != obj->material.reflectionMod) o.material.reflectionMod = -1;
							if(o.material.refractiveIndex != obj->material.refractiveIndex) o.material.refractiveIndex = -1;
						}

						objectDiffs = o;
						{
							// Ugh... probably needs some meta programming?
							if(o.pos.x == -1) o.pos.x = 0;
							if(o.pos.y == -1) o.pos.y = 0;
							if(o.pos.z == -1) o.pos.z = 0;
							if(o.rot.w == -1) o.rot.w = 0;
							if(o.rot.x == -1) o.rot.x = 0;
							if(o.rot.y == -1) o.rot.y = 0;
							if(o.rot.z == -1) o.rot.z = 0;
							if(o.dim.x == -1) o.dim.x = 0;
							if(o.dim.y == -1) o.dim.y = 0;
							if(o.dim.z == -1) o.dim.z = 0;
							if(o.color.r == -1) o.color.r = 0;
							if(o.color.g == -1) o.color.g = 0;
							if(o.color.b == -1) o.color.b = 0;
							if(o.geometry.type == -1) o.geometry.type = 0;
							if(o.material.emitColor.r == -1) o.material.emitColor.r = 0;
							if(o.material.emitColor.g == -1) o.material.emitColor.g = 0;
							if(o.material.emitColor.b == -1) o.material.emitColor.b = 0;
							if(o.material.reflectionMod == -1) o.material.reflectionMod = 0;
							if(o.material.refractiveIndex == -1) o.material.refractiveIndex = 0;
						}
						eui->multiChangeObject = o;

						if(!multipleSelection) diffColor = textColor;
					}

					Object* obj = &eui->multiChangeObject;

					// Offset and size of value that changes for multiple selection.
					Vec2i offSize = vec2i(0,0);
					Vec2i offSize2 = vec2i(0,0);

					//

					r = rectTLDim(p, vec2(ew, headerHeight)); p.y -= headerHeight+pad.y;
					
					float leftRightButtonWidth = eh * 1.5f;
					r = rectExpand(r, vec2(0,-eh*0.2f));
					qr = quickRow(r, pad.x, leftRightButtonWidth, 0, leftRightButtonWidth);

					if(!multipleSelection) {
						if(newGuiQuickButton(gui, quickRowNext(&qr), "◄")) {
							eui->selectedObjects[0] = mod(eui->selectedObjects[0]-1, world->objects.count);
							eui->selectionChanged = true;
						}
					} else quickRowNext(&qr);

					char* headerString = "<b>Entities<b>";
					if(!multipleSelection) headerString = fillString("<b>Entity %i<b>", eui->selectedObjects.first());
					newGuiQuickTextBox(gui, quickRowNext(&qr), headerString, vec2i(0,0), &headerSettings);

					if(!multipleSelection) {
						if(newGuiQuickButton(gui, quickRowNext(&qr), "►")) {
							eui->selectedObjects[0] = mod(eui->selectedObjects[0]+1, world->objects.count);
							eui->selectionChanged = true;
						}
					} else quickRowNext(&qr);

					{
						char* labels[] = {"Position", "Rotation", "Color", "Type", "Dimension", "EmitColor", "Reflection", "Refraction"};
						int labelIndex = 0;
						float labelsMaxWidth = 0;
						for(int i = 0; i < arrayCount(labels); i++) {
							labelsMaxWidth = max(labelsMaxWidth, getTextDim(labels[i], font).w);
						}
						labelsMaxWidth += padding.w;

						r = rectTLDim(p, vec2(ew, eh)); p.y -= eh+pad.y;
						qr = quickRow(r, pad.x, labelsMaxWidth, 0,0,0);
						newGuiQuickText(gui, quickRowNext(&qr), labels[labelIndex++], vec2i(-1,0));

						// if(objectDiffs.pos.x != -1) gui->editSettings.textBoxSettings.textSettings.color = diffColor;
						if(newGuiQuickTextEdit(gui, quickRowNext(&qr), &obj->pos.x)) offSize = memberOffsetSize(Object, pos.x);
						// gui->editSettings.textBoxSettings.textSettings.color = textColor;

						// if(objectDiffs.pos.y != -1) gui->editSettings.textBoxSettings.textSettings.color = diffColor;
						if(newGuiQuickTextEdit(gui, quickRowNext(&qr), &obj->pos.y)) offSize = memberOffsetSize(Object, pos.y);
						// gui->editSettings.textBoxSettings.textSettings.color = textColor;

						// if(objectDiffs.pos.z != -1) gui->editSettings.textBoxSettings.textSettings.color = diffColor;
						if(newGuiQuickTextEdit(gui, quickRowNext(&qr), &obj->pos.z)) offSize = memberOffsetSize(Object, pos.z);
						// gui->editSettings.textBoxSettings.textSettings.color = textColor;

						{
							float roll, pitch, yaw;
							quatToEulerAngles(obj->rot, &roll, &pitch, &yaw);

							r = rectTLDim(p, vec2(ew, eh)); p.y -= eh+pad.y;
							float cols[] = {labelsMaxWidth, 0,0,0};
							qr = quickRow(r, pad.x, cols, arrayCount(cols));
							newGuiQuickText(gui, quickRowNext(&qr), labels[labelIndex++], vec2i(-1,0));
							bool change = false;

							float rollDegree = radianToDegree(roll);
							float pitchDegree = radianToDegree(pitch);
							float yawDegree = radianToDegree(yaw);

							if(newGuiQuickSlider(gui, quickRowNext(&qr), &rollDegree, -180, 180)) change = true;
							if(newGuiQuickSlider(gui, quickRowNext(&qr), &pitchDegree, -180, 180)) change = true;
							if(newGuiQuickSlider(gui, quickRowNext(&qr), &yawDegree, -180, 180)) change = true;

							if(change) {
								roll = degreeToRadian(rollDegree);
								pitch = degreeToRadian(pitchDegree);
								yaw = degreeToRadian(yawDegree);

								obj->rot = eulerAnglesToQuat(roll, pitch, yaw);
								offSize = memberOffsetSize(Object, rot);
							}
						}


						r = rectTLDim(p, vec2(ew, eh)); p.y -= eh+pad.y;
						qr = quickRow(r, pad.x, labelsMaxWidth, 0,0,0);
						newGuiQuickText(gui, quickRowNext(&qr), labels[labelIndex++], vec2i(-1,0));
						if(newGuiQuickTextEdit(gui, quickRowNext(&qr), &obj->color.r)) offSize = memberOffsetSize(Object, color.r);
						if(newGuiQuickTextEdit(gui, quickRowNext(&qr), &obj->color.g)) offSize = memberOffsetSize(Object, color.g);
						if(newGuiQuickTextEdit(gui, quickRowNext(&qr), &obj->color.b)) offSize = memberOffsetSize(Object, color.b);


						// Hack.

						r = rectTLDim(p, vec2(ew, eh)); p.y -= eh+pad.y;
						qr = quickRow(r, pad.x, labelsMaxWidth, 0);
						newGuiQuickText(gui, quickRowNext(&qr), labels[labelIndex++], vec2i(-1,0));
						if(!eui->changedGeomType) eui->temp.geometry.type = obj->geometry.type;
						if(newGuiQuickComboBox(gui, quickRowNext(&qr), &eui->temp.geometry.type, geometryTypeStrings, arrayCount(geometryTypeStrings))) {
							obj->geometry.type = eui->temp.geometry.type;
							eui->changedGeomType = false;

							offSize = memberOffsetSize(Object, geometry.type);

							if(obj->geometry.type == GEOM_TYPE_SPHERE) {
								obj->dim = vec3(max(obj->dim.x, obj->dim.y, obj->dim.z));
								offSize2 = memberOffsetSize(Object, dim);
							}
						}
						if(newGuiGotActive(gui, newGuiCurrentId(gui)-2)) {
							eui->temp.geometry.type = obj->geometry.type;
							eui->changedGeomType = true;
						}

						r = rectTLDim(p, vec2(ew, eh)); p.y -= eh+pad.y;
						qr = quickRow(r, pad.x, labelsMaxWidth, 0,0,0);
						newGuiQuickText(gui, quickRowNext(&qr), labels[labelIndex++], vec2i(-1,0));
						int dimChanged = 0;
						if(newGuiQuickTextEdit(gui, quickRowNext(&qr), &obj->dim.x)) dimChanged = 1;
						if(newGuiQuickTextEdit(gui, quickRowNext(&qr), &obj->dim.y)) dimChanged = 2;
						if(newGuiQuickTextEdit(gui, quickRowNext(&qr), &obj->dim.z)) dimChanged = 3;

						if(dimChanged) {
							if(obj->geometry.type == GEOM_TYPE_SPHERE) {
								float d = obj->dim.e[dimChanged-1];
								obj->dim = vec3(d);
								offSize = memberOffsetSize(Object, dim);
							} else {
								if(dimChanged == 1) offSize = memberOffsetSize(Object, dim.x);
								if(dimChanged == 2) offSize = memberOffsetSize(Object, dim.y);
								if(dimChanged == 3) offSize = memberOffsetSize(Object, dim.z);
							}
						}

						{
							p.y -= separatorHeight/2;
							drawLineH(p+vec2(0,0.5f), p+vec2(ew,0)+vec2(0,0.5f), cSeparatorDark);
							drawLineH(p+vec2(0,-0.5f), p+vec2(ew,0)+vec2(0,-0.5f), cSeparatorBright);
							p.y -= separatorHeight/2+pad.y+0.5f;
						}

						//

						r = rectTLDim(p, vec2(ew, eh)); p.y -= eh+pad.y;
						qr = quickRow(r, pad.x, labelsMaxWidth, 0,0,0);
						newGuiQuickText(gui, quickRowNext(&qr), labels[labelIndex++], vec2i(-1,0));
						if(newGuiQuickTextEdit(gui, quickRowNext(&qr), &obj->material.emitColor.r)) offSize = memberOffsetSize(Object, material.emitColor.r);
						if(newGuiQuickTextEdit(gui, quickRowNext(&qr), &obj->material.emitColor.g)) offSize = memberOffsetSize(Object, material.emitColor.g);
						if(newGuiQuickTextEdit(gui, quickRowNext(&qr), &obj->material.emitColor.b)) offSize = memberOffsetSize(Object, material.emitColor.b);

						r = rectTLDim(p, vec2(ew, eh)); p.y -= eh+pad.y;
						qr = quickRow(r, pad.x, labelsMaxWidth, 0);
						newGuiQuickText(gui, quickRowNext(&qr), labels[labelIndex++], vec2i(-1,0));
						if(newGuiQuickSlider(gui, quickRowNext(&qr), &obj->material.reflectionMod, 0, 1)) offSize = memberOffsetSize(Object, material.reflectionMod);

						r = rectTLDim(p, vec2(ew, eh)); p.y -= eh+pad.y;
						qr = quickRow(r, pad.x, labelsMaxWidth, 0);
						newGuiQuickText(gui, quickRowNext(&qr), labels[labelIndex++], vec2i(-1,0));
						if(newGuiQuickSlider(gui, quickRowNext(&qr), &obj->material.refractiveIndex, 1, 3)) offSize = memberOffsetSize(Object, material.refractiveIndex);

						{
							p.y -= separatorHeight/2;
							drawLineH(p+vec2(0,0.5f), p+vec2(ew,0)+vec2(0,0.5f), cSeparatorDark);
							drawLineH(p+vec2(0,-0.5f), p+vec2(ew,0)+vec2(0,-0.5f), cSeparatorBright);
							p.y -= separatorHeight/2+pad.y+0.5f;
						}

						r = rectTLDim(p, vec2(ew, eh)); p.y -= eh+pad.y;
						qr = quickRow(r, pad.x, 0.0f, 0.0f);
						if(newGuiQuickButton(gui, quickRowNext(&qr), "Copy")) {
							copyObjects(&world->objects, &eui->objectCopies, &eui->selectedObjects);
						}
						if(newGuiQuickButton(gui, quickRowNext(&qr), "Delete")) {
							deleteObjects(&world->objects, &eui->selectedObjects, &eui->history, false);
						}


						// Set value of all objects to the value that changed.
						if(offSize.y) {
							eui->history.objectsPreMod.clear();
							for(int i = 0; i < eui->selectedObjects.count; i++) {
								Object obj = world->objects[eui->selectedObjects[i]];
								eui->history.objectsPreMod.push(obj);
							}

							eui->objectsEdited = true;
							eui->objectNoticeableChange = true;

							{
								for(int i = 0; i < eui->selectedObjects.count; i++) {
									Object* o = world->objects + eui->selectedObjects[i];
									memCpy((char*)(o) + offSize.x, (char*)(obj) + offSize.x, offSize.y);
									int stop = 234;
								}

								if(offSize2.y) {
									Vec2i offSize = offSize2;
									for(int i = 0; i < eui->selectedObjects.count; i++) {
										Object* o = world->objects + eui->selectedObjects[i];
										memCpy((char*)(o) + offSize.x, (char*)(obj) + offSize.x, offSize.y);
									}
								}
							}
						}
					}

				}

				ad->oldPanelHeightRight = ad->panelHeightRight;
				ad->panelHeightRight = rectPanelRight.top - p.y - pad.y + panelBorder;

				newGuiScissorPop(gui);
			}
		}

		// @ColorPicker.
		if(entityPanelActive && ad->oldPanelHeightRight != 0 && ad->drawSceneWired && ad->panelsVisible)
		{
			Rect rEntities = ad->panelRightRect;

			float panelHeight = 10 * ad->fontHeight;
			if(rectW(rEntities) < panelHeight) {
				panelHeight = rectW(rEntities);
			}

			Rect rectColorPicker = rectTRDim(rectBR(rEntities), vec2(panelHeight, panelHeight));
			ad->panelColorPickerRect = rectColorPicker;

			newGuiSetHotAllMouseOver(gui, rectColorPicker, gui->zLevel);
			newGuiQuickBox(gui, rectColorPicker);

			newGuiScissorPush(gui, rectExpand(rectColorPicker, vec2(-2)));

			{
				Rect r = rectExpand(rectColorPicker, vec2(-panelBorder*2));

				Vec2 cen = rectCen(r);
				float height = rectH(r);

				//

				float ringThickness = height * 0.13f;
				float triangleOffset = 0.1f * ad->fontHeight;

				float markerRadius = ringThickness/2 / 2;
				Vec4 cOutline = ad->colors.outline;
				// Vec4 cOutline = vec4(1,1);
				// Vec4 cMarker = vec4(1,1);
				// Vec4 cMarkerOutline = cOutline;

				Vec4 cMarker1 = vec4(1,1);
				Vec4 cMarker2 = vec4(0,1);
				float markerThickness1 = 1.0f;
				float markerThickness2 = 2.0f;

				// So we don't clamp to one extreme. Not the best solution.
				float minRange = 0.001f;
				float maxRange = 0.999f;

				//

				bool colorChanged = false;
				bool applyImmediately = true;

				float hue = 0;
				float saturation = 0;
				float lightness = 0;

				EntityUI* eui = &ad->entityUI;
				World* world = &ad->world;

				{
					// Check if everyone has the same color, and if so use that.
					bool everyoneHasSameColor = true;
					Vec3 startColor = world->objects[eui->selectedObjects[0]].color;
					for(int i = 1; i < eui->selectedObjects.count; i++) {
						Object* obj = world->objects + eui->selectedObjects[i];
						if(obj->color != startColor) {
							everyoneHasSameColor = false;
							break;
						}
					}

					Vec3 hsl;
					if(everyoneHasSameColor) {
						hsl = rgbToHslFloat(startColor);
					} else {
						hsl = vec3(0,0.5f,0.5f);
						applyImmediately = false;
					}

					hue = hsl.x;
					saturation = hsl.y;
					lightness = hsl.z;
				}

				// Triangle.
				{
					float triangleRadius = rectW(r) / 2 - ringThickness - triangleOffset;
					Vec2 dir = normVec2(vec2(0,1)) * triangleRadius;

					Vec2 tp0 = cen + rotateVec2(dir, degreeToRadian((360/3.0f)*2));
					Vec2 tp1 = cen + dir;
					Vec2 tp2 = cen + rotateVec2(dir, degreeToRadian(360/3.0f));

					{
						bool hot = lenVec2(input->mousePosNegative - cen) <= triangleRadius;
						int id = newGuiDragAction(gui, hot);
						if(newGuiGotActive(gui, id)) {
							// Code duplication.
							eui->history.objectsPreMod.clear();
							for(int i = 0; i < eui->selectedObjects.count; i++) {
								Object obj = world->objects[eui->selectedObjects[i]];
								eui->history.objectsPreMod.push(obj);
							}
						}
						if(newGuiIsActive(gui, id) || newGuiWasActive(gui, id)) {
							// Convert point to value.
							Vec2 mp = input->mousePosNegative;
							Vec2 trianglePoint = closestPointToTriangle(mp, tp0, tp1, tp2);

							// saturation = mapRange(trianglePoint.y, tp0.y, tp1.y, minRange, maxRange);
							// float xMin = mapRange(saturation, minRange, maxRange, tp0.x, tp1.x);
							// float xMax = mapRange(saturation, minRange, maxRange, tp2.x, tp1.x);
							// if(xMin == xMax) lightness = maxRange;
							// else lightness = mapRange(trianglePoint.x, xMin, xMax, minRange, maxRange);

							lightness = mapRange(trianglePoint.x, tp0.x, tp2.x, minRange, maxRange);
							lightness = clamp(lightness, minRange, maxRange);
							float yMax;
							if(lightness <= 0.5f) yMax = mapRange(lightness, minRange, 0.5f, tp0.y, tp1.y);
							else yMax = mapRange(lightness, 0.5f, maxRange, tp1.y, tp0.y);

							saturation = mapRange(trianglePoint.y, tp0.y, yMax, minRange, maxRange);

							saturation = clamp(saturation, minRange, maxRange);
						}
						if(newGuiWasActive(gui, id)) {
							colorChanged = true;
						}
					}

					// Draw triangle.
					{
						setSRGB(false);
						Vec4 c0 = COLOR_SRGB(vec4(hslToRgbFloat(hue,0,0), 1));
						Vec4 c1 = COLOR_SRGB(vec4(hslToRgbFloat(hue,1,0.5f), 1));
						Vec4 c2 = COLOR_SRGB(vec4(hslToRgbFloat(hue,1,1), 1));

						Vec2 tp;
						glBindTexture(GL_TEXTURE_2D, 0);
						glBegin(GL_TRIANGLES);
							pushColor(c0); pushVec(tp0);
							pushColor(c1); pushVec(tp1);
							pushColor(c2); pushVec(tp2);
						glEnd();
						setSRGB();
					}

					// Triangle outline.
					{
						setSRGB(false);
						glBindTexture(GL_TEXTURE_2D, 0);
						Vec4 color = COLOR_SRGB(cOutline);
						pushColor(color);

						glBegin(GL_LINE_STRIP);
							pushVec(tp0);
							pushVec(tp1);
							pushVec(tp2);
							pushVec(tp0);
						glEnd();
						setSRGB();
					}

					// Draw triangle point.
					{
						Vec2 trianglePoint;

						// Convert value to point.
						{
							// trianglePoint.y = mapRange(saturation, 0, 1, tp0.y, tp1.y);
							// float xMin = mapRange(saturation, 0, 1, tp0.x, tp1.x);
							// float xMax = mapRange(saturation, 0, 1, tp2.x, tp1.x);
							// trianglePoint.x = mapRange(lightness, 0, 1, xMin, xMax);
	
							trianglePoint.x = mapRange(lightness, minRange, maxRange, tp0.x, tp2.x);
							float yMax;
							if(lightness <= 0.5f) yMax = mapRange(lightness, minRange, 0.5f, tp0.y, tp1.y);
							else yMax = mapRange(lightness, 0.5f, maxRange, tp1.y, tp0.y);

							trianglePoint.y = mapRange(saturation, minRange, maxRange, tp0.y, yMax);
						}

						drawRing(trianglePoint, markerRadius + markerThickness1/2, markerThickness1, cMarker1);
						drawRing(trianglePoint, markerRadius - markerThickness1/2, markerThickness2, cMarker2);

						// drawRing(trianglePoint, markerRadius + 0.5f, cMarkerOutline);
						// drawCircle(trianglePoint, markerRadius, cMarker);
					}
				}

				// Circle.
				{
					Vec2 cp = cen;
					float cr = rectW(r)/2;

					{
						float distToCenter = lenVec2(input->mousePosNegative - cen);
						bool hot = valueBetween(distToCenter, cr-ringThickness, cr);

						int id = newGuiDragAction(gui, hot);
						if(newGuiGotActive(gui, id)) {
							// Code duplication.
							eui->history.objectsPreMod.clear();
							for(int i = 0; i < eui->selectedObjects.count; i++) {
								Object obj = world->objects[eui->selectedObjects[i]];
								eui->history.objectsPreMod.push(obj);
							}
						}
						if(newGuiIsActive(gui, id) || newGuiWasActive(gui, id)) {
							// Angle to hue.

							Vec2 mp = input->mousePosNegative;
							Vec2 dir = mp - cp;
							float angle = angleVec2(vec2(0,1), dir);

							if(dot(dir, vec2(1,0)) < 0) angle = M_PI + M_PI-angle;
							hue = mapRange(angle, 0, M_2PI, 0, 1);

							if(saturation == 0) saturation = minRange;
						}
						if(newGuiWasActive(gui, id)) {
							colorChanged = true;
						}
					}

					{
						setSRGB(false);

						glBindTexture(GL_TEXTURE_2D, 0);
						glBegin(GL_TRIANGLE_STRIP);
						int segments = (M_2PI * cr)*0.2f;
						for(int i = 0; i < segments+1; i++) {
							float angle = (i * M_2PI/(float)segments);

							float degrees = radianToDegree(angle);
							Vec3 hueRGB;
							hueToRgb(hueRGB.e, degrees);

							Vec4 c = vec4(hueRGB, 1);
							pushColor(c);

							Vec2 dir = rotateVec2(vec2(0,1), angle);
							Vec2 p0 = cp + dir * (cr - ringThickness);
							Vec2 p1 = cp + dir * (cr);

							pushVec(p0);
							pushVec(p1);
						}
						glEnd();

						setSRGB();
					}

					// Draw hue point.
					{
						Vec2 dir = rotateVec2(vec2(0,1), hue*M_2PI);
						Vec2 huePoint = cen + dir * (cr - ringThickness/2);

						// glLineWidth(2);
						// // drawRing(huePoint + dir * 0.5f, huePointRadius, vec4(1,1));
						// drawRing(huePoint, huePointRadius, cMarkerOutline);
						// glLineWidth(1);

						drawRing(huePoint, markerRadius + markerThickness1/2, markerThickness1, cMarker1);
						drawRing(huePoint, markerRadius - markerThickness1/2, markerThickness2, cMarker2);

						// drawRing(huePoint, huePointRadius + 0.5f, cMarkerOutline);
						// drawCircle(huePoint, huePointRadius, cMarker);
					}

					// Circle outline.
					{
						Vec2 cc = cen;
						float off = 0.5f;
						glLineWidth(1);
						drawRing(cc, cr+off, cOutline);
						drawRing(cc, cr-off - ringThickness, cOutline);
						glLineWidth(1);
					}
				}

				Vec3 color = hslToRgbFloat(hue, saturation, lightness);

				if(applyImmediately || colorChanged) {
					for(int i = 0; i < eui->selectedObjects.count; i++) {
						Object* obj = world->objects + eui->selectedObjects[i];
						obj->color = color;
					}
				}

				if(colorChanged) {
					eui->objectsEdited = true;
					eui->objectNoticeableChange = true;
				}
			}

			newGuiScissorPop(gui);
		}


		newGuiPopupSetup(gui);

		// @Popups.
		{
			for(int i = 0; i < gui->popupStackCount; i++) {
				PopupData pd = gui->popupStack[i];

				if(pd.type != POPUP_TYPE_OTHER) continue;

				float fontHeight = gui->textSettings.font->height;

				float popupOffset = 0;
				float popupWidth = pd.width;

				float padding = 0;
				float border = 1;
				float eh = fontHeight * 1.4;
				float ep = fontHeight * 0.7f;

				float textSidePadding = fontHeight*0.7f;
				float topBottomPadding = textSidePadding*0.4f;

				float separatorHeight = eh*0.4f;
				Vec4 cSeparator = gui->popupSettings.borderColor;


				float elementCount;
				float separatorCount;
				float popupHeight;

				if(strCompare(pd.name, "FileMenu")) {
					popupWidth = ad->fontHeight * 8.5f;
					elementCount = 5;
					separatorCount = 2;
					popupHeight = elementCount*(eh) - padding + border*2 + topBottomPadding*2 + separatorHeight*separatorCount;
				} else if(strCompare(pd.name, "SettingsMenu")) {
					elementCount = 6;
					separatorCount = 2;
					popupHeight = elementCount*(eh) - padding + border*2 + topBottomPadding*2 + separatorHeight*separatorCount + topBottomPadding*0.5f;
				}

				float ew = popupWidth;


				Rect rPop = rectTLDim(pd.p-vec2(0,popupOffset), vec2(popupWidth,popupHeight));
				gui->popupStack[i].rCheck = rPop;

				newGuiSetHotAllMouseOver(gui, rPop, gui->zLevel);

				Rect shadowRect = rectTrans(rPop, vec2(1,-1)*textSidePadding*0.5f);
				drawRect(shadowRect, vec4(0,0.8f));

				newGuiQuickBox(gui, rPop, &gui->popupSettings);

				// Overdraw borders to make transition seemless. (Dirty hack...)
				{
					Rect r = pd.rSource;
					Rect lr = rect(r.left  + border, r.bottom-border*2, 
					               r.right - border, r.bottom+border*2);
					drawRect(rectRound(lr), gui->popupSettings.color);

					float y = roundFloat(r.bottom) - 0.5f;
					float off = textSidePadding*0.5f;
					drawLine(vec2(lr.left + off, y), vec2(lr.right - off, y), cSeparator);
				}

				if(gui->popupSettings.borderColor.a) {
					rectExpand(&rPop, vec2(-border*2));
					ew -= border*2;
				}

				newGuiScissorPush(gui, rPop);

				{
					Vec4 cButton = gui->popupSettings.color;

					TextBoxSettings bs = gui->buttonSettings;
					gui->buttonSettings = textBoxSettings(bs.textSettings, boxSettings(cButton));
					gui->buttonSettings.sideAlignPadding = textSidePadding;

					TextSettings ts = gui->textSettings;
					ts.color.a = 0.5f;

					Vec2 p = rectTL(rPop);
					Rect r;
					char* s;
					QuickRow qr;
					Font* font = gui->textSettings.font;

					if(strCompare(pd.name, "FileMenu")) {
						Rect pr = rPop;
						pr.top -= topBottomPadding;
						pr.bottom -= topBottomPadding;

						p = rectTL(pr);


						bool close = false;
						World* world = &ad->world;

						r = rectTLDim(p, vec2(ew, eh)); p.y -= eh + padding;
						if(newGuiQuickPButton(gui, r, "New...", vec2i(-1,0))) {
							newSceneCommand(world, ad->sceneFile, &ad->sceneHasFile, &ad->entityUI);

							close = true;
						}
						newGuiQuickText(gui, rectAddR(r,-textSidePadding), "Ctrl+N", vec2i(1,0), &ts);

						r = rectTLDim(p, vec2(ew, eh)); p.y -= eh + padding;
						if(newGuiQuickPButton(gui, r, "Open...", vec2i(-1,0))) {
							openSceneCommand(&ad->dialogData);

							close = true;
						}
						newGuiQuickText(gui, rectAddR(r,-textSidePadding), "Ctrl+O", vec2i(1,0), &ts);

						{
							p.y -= separatorHeight*0.5f;
							Vec2 lp = vec2(p.x,roundFloat(p.y))+vec2(0,0.5f);
							drawLine(lp + vec2(textSidePadding*0.5f,0), lp + vec2(ew,0)  - vec2(textSidePadding*0.5f,0), cSeparator);
							p.y -= separatorHeight*0.5f;
						}

						r = rectTLDim(p, vec2(ew, eh)); p.y -= eh + padding;
						if(newGuiQuickPButton(gui, r, "Save", vec2i(-1,0))) {
							saveSceneCommand(world, ad->sceneFile, ad->sceneHasFile, &ad->dialogData);

							close = true;
						}
						newGuiQuickText(gui, rectAddR(r,-textSidePadding), "Ctrl+S", vec2i(1,0), &ts);

						r = rectTLDim(p, vec2(ew, eh)); p.y -= eh + padding;
						if(newGuiQuickPButton(gui, r, "Save as...", vec2i(-1,0))) {
							openSceneDialog(&ad->dialogData, true);
							close = true;
						}

						{
							p.y -= separatorHeight*0.5f;
							Vec2 lp = vec2(p.x,roundFloat(p.y))-vec2(0,0.5f);
							drawLine(lp + vec2(textSidePadding*0.5f,0), lp + vec2(ew,0)  - vec2(textSidePadding*0.5f,0), cSeparator);
							p.y -= separatorHeight*0.5f;
						}

						r = rectTLDim(p, vec2(ew, eh)); p.y -= eh + padding;
						if(newGuiQuickPButton(gui, r, "Exit", vec2i(-1,0))) {
							*isRunning = false;
							close = true;
						}

						if(close) gui->popupStackCount = 0;
					}

					if(strCompare(pd.name, "SettingsMenu")) {
						World* world = &ad->world;

						p += vec2(textSidePadding, -topBottomPadding);
						ew -= textSidePadding*2;

						s = "Font scale";
						char* s2 = fillString("Height: %i", ad->fontHeight);
						r = rectTLDim(p, vec2(ew, eh)); p.y -= eh + padding;
						qr = quickRow(r, 0, getTextDim(s, font).w, 0, getTextDim(s2, font).w);
						newGuiQuickText(gui, quickRowNext(&qr), s, vec2i(-1,0)); 
						quickRowNext(&qr);
						newGuiQuickText(gui, quickRowNext(&qr), s2, vec2i(1,0)); 

						newGuiQuickText(gui, r, s, vec2i(-1,0)); 

						gui->sliderSettings.applyAfter = true;
						bool reopenPopup = false;
						r = rectTLDim(p, vec2(ew, eh)); p.y -= eh + padding;
						if(newGuiQuickSlider(gui, r, &ad->fontScale, 0.5f, 2)) {
							reopenPopup = true;
						}
						gui->sliderSettings.applyAfter = false;


						p.y -= separatorHeight;


						s = "Mouse sensitivity";
						r = rectTLDim(p, vec2(ew, eh)); p.y -= eh + padding;
						newGuiQuickText(gui, r, s, vec2i(-1,0)); 

						r = rectTLDim(p, vec2(ew, eh)); p.y -= eh + padding;
						newGuiQuickSlider(gui, r, &ad->mouseSpeed, 0, 2); 


						p.y -= separatorHeight;


						s = "Snap grid size";
						r = rectTLDim(p, vec2(ew, eh)); p.y -= eh + padding;
						newGuiQuickText(gui, r, s, vec2i(-1,0)); 

						r = rectTLDim(p, vec2(ew, eh)); p.y -= eh + padding;
						newGuiQuickSlider(gui, r, &ad->entityUI.snapGridSize, 1, 10); 


						if(reopenPopup) {
							newGuiForceActive(gui, gui->menuId);
							gui->popupStackCount = 0;
							break;
						}
					}

					gui->buttonSettings = bs;
				}

				newGuiScissorPop(gui);
			}

		}

		newGuiEnd(gui);
	}


	openglDrawFrameBufferAndSwap(ws, systemData, &ad->swapTimer, init, ad->panelAlpha, ad->dt);

	// @AppSessionWrite
	if(*isRunning == false) {
		Rect windowRect = getWindowWindowRect(systemData->windowHandle);
		if(ws->fullscreen) windowRect = ws->previousWindowRect;

		AppSessionSettings at = {};

		at.windowRect = windowRect;
		at.mouseSpeed = ad->mouseSpeed;
		at.fontScale = ad->fontScale;
		at.panelWidthLeft = ad->panelWidthLeft;
		at.panelWidthRight = ad->panelWidthRight;
		strCpy(at.sceneFile, ad->sceneFile);

		saveAppSettings(at);
	}
}
