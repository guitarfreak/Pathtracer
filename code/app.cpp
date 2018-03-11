/*
=================================================================================

	ToDo:
	- Depth of field.
	- Refraction.
	- More advanced lighting function
	- Clean global light and multiple lights
	- Multiple selection
	- Revert.
	- Color picker.
	- Clearn up ui.
	- Have cam independent, mini window.
	- Ellipses.
	- Spacial data structure to speed up processing.
	- Converge method on sampling for speed up.
	- Clean up of whole code folder. Make it somewhat presentable, remove unused things.
	- Clean up repetitive gui code. (Layout.)

	- DArray.
	- Simd.

	Bugs:
	- Windows key slow sometimes.
	- Memory leak? Flashing when drawing scene in opengl.

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

	char* fontFile;
	char* fontFileBold;
	char* fontFileItalic;
	int fontHeight;
	float fontScale;

	NewGui gui;
	float panelHeightLeft;
	float panelHeightRight;

	float panelWidthLeft;
	float panelWidthRight;

	float panelAlpha;
	float panelAlphaFadeState;

	Font* font;

	float menuHeight;

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

	bool activeProcessing;
	int threadCount;
	ProcessPixelsData threadData[RAYTRACE_THREAD_JOB_COUNT];

	bool abortProcessing;
	bool tracerFinished;
  
	f64 processStartTime;
	f64 processTime;
};



// #ifdef SHIPPING_MODE
// #pragma optimize( "", on )
// #else 
#pragma optimize( "", off )
// #endif

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

		// int windowStyle = (WS_POPUP | WS_BORDER);
		// int windowStyle = (WS_POPUP);
		int windowStyle = WS_OVERLAPPEDWINDOW & ~WS_SYSMENU;
		initSystem(systemData, ws, windowsData, vec2i(1920*0.85f, 1080*0.85f), windowStyle, 1);


		windowHandle = systemData->windowHandle;

		printf("%Opengl Version: %s\n", (char*)glGetString(GL_VERSION));

		loadFunctions();

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
		int fps = wglGetSwapIntervalEXT();

		initInput(&ad->input);
		systemData->input = &ad->input;

		systemData->minWindowDim = vec2i(200,200);
		systemData->maxWindowDim = ws->biggestMonitorSize;

		#ifndef SHIPPING_MODE
		makeWindowTopmost(systemData);
		#endif

		gs->useSRGB = true;

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

		uint vao = 0;
		glCreateVertexArrays(1, &vao);
		glBindVertexArray(vao);

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
			int div = 4;

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
				at.fontScale = 1;
				at.panelWidthLeft = systemData->fontHeight*15;
				at.panelWidthRight = systemData->fontHeight*15;
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
		ad->entityUI.snapGridSize = 1;
		ad->entityUI.snapGridDim = 100;

		ad->entityUI.objectCopy = defaultObject();

		ad->fontFile       = getPStringCpy("LiberationSans-Regular.ttf");
		ad->fontFileBold   = getPStringCpy("LiberationSans-Bold.ttf");
		ad->fontFileItalic = getPStringCpy("LiberationSans-Italic.ttf");




		// ad->settings.texDim = vec2i(1280, 720);
		ad->settings.texDim = vec2i(768, 432);

		ad->settings.sampleMode = SAMPLE_MODE_BLUE_MULTI;
		ad->settings.sampleCountGrid = 4;
		ad->settings.sampleGridWidth = 10;

		ad->settings.rayBouncesMax = 6;

		ad->threadCount = RAYTRACE_THREAD_JOB_COUNT;
		// ad->threadCount = 1;

		ad->drawSceneWired = true;
		ad->fitToScreen = true;

		ad->settings.randomDirectionCount = 20000;

		ad->sceneFileMax = 200;
		ad->sceneFile = getPString(ad->sceneFileMax);

		if(strLen(appSessionSettings.sceneFile) && 
		   fileExists(appSessionSettings.sceneFile)) {
			strCpy(ad->sceneFile, appSessionSettings.sceneFile);
			ad->sceneHasFile = true;
			loadScene(&ad->world, ad->sceneFile);
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
			ad->dt = timerStop(&ad->frameTimer);
			ad->time += ad->dt;

			ad->fpsTime += ad->dt;
			ad->fpsCounter++;
			if(ad->fpsTime >= 1) {
				ad->avgFps = 1 / (ad->fpsTime / (f64)ad->fpsCounter);
				ad->fpsTime = 0;
				ad->fpsCounter = 0;
			}

			timerStart(&ad->frameTimer);
			// printf("%f\n", ad->dt);
		}
	}

	clearTMemory();

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

	if(input->keysPressed[KEYCODE_ESCAPE]) {
		if(ws->fullscreen) {
			if(input->keysPressed[KEYCODE_ESCAPE]) setWindowMode(windowHandle, ws, WINDOW_MODE_WINDOWED);
		} else {
			// *isRunning = false;
		}
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
		glClear(GL_COLOR_BUFFER_BIT);


		glClearColor(0,0,0,1);
		bindFrameBuffer(FRAMEBUFFER_2dMsaa);

		scissorState(false);
	}


	// @AppLoop.

	NewGui* gui = &ad->gui;

	{
		newGuiBegin(gui, &ad->input, ws);
	}

	#ifdef ENABLE_CUSTOM_WINDOW_FRAME
	if(!ws->fullscreen)
	{
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

		char* titleText = fillString("<b>%s<b>", App_Name);

		if(ad->sceneHasFile) {
			char* sceneFileName = getFileNameFromPath(ad->sceneFile);
			titleText = fillString("%s - %s", titleText, sceneFileName);
		}

		sd->normalTitleHeight = sd->fontHeight*1.4f;
		sd->normalBorderSize = 5;
		sd->normalVisualBorderSize = 1;
		sd->buttonMargin = 3;
		sd->cornerGrabSize = 20;

		Vec4 cBorder = vec4(0,1);

		// Vec3 cTitleBarFocusedLeft     = vec3(0.03f,0.7f,0.25f);
		// Vec3 cTitleBarFocusedRight    = cTitleBarFocusedLeft  + vec3(0.03f,-0.1f,0.1f);
		// Vec3 cTitleBarNotFocusedLeft  = cTitleBarFocusedLeft  + vec3(0, -1, -0.1f);
		// Vec3 cTitleBarNotFocusedRight = cTitleBarFocusedRight + vec3(0, -1, -0.1f);

		Vec3 cTitleBarFocusedLeft     = vec3(0.62f,0.2f,0.15f);
		Vec3 cTitleBarFocusedRight    = cTitleBarFocusedLeft  + vec3(0.00f,0.0f,0.15f);
		Vec3 cTitleBarNotFocusedLeft  = cTitleBarFocusedLeft;
		Vec3 cTitleBarNotFocusedRight = cTitleBarFocusedRight;

		Vec4 cTitleBarLeft, cTitleBarRight;
		if(!ws->windowHasFocus) {
			cTitleBarLeft = vec4(hslToRgbFloat(cTitleBarNotFocusedLeft),1);
			cTitleBarRight = vec4(hslToRgbFloat(cTitleBarNotFocusedRight),1);
		} else {
			cTitleBarLeft = vec4(hslToRgbFloat(cTitleBarFocusedLeft),1);
			cTitleBarRight = vec4(hslToRgbFloat(cTitleBarFocusedRight),1);
		}

		float cButtons = 0.7f;
		Vec4 cButton0 = vec4(cButtons,1);
		Vec4 cButton1 = vec4(cButtons,1);
		Vec4 cButton2 = vec4(cButtons,1);

		Vec4 cButtonOutline = cBorder;

		Vec4 cText = vec4(0.9f,1);
		if(!ws->windowHasFocus) cText = vec4(0.5f,1);

		Vec4 cTextShadow = vec4(0,1);

		float fontHeight = sd->normalTitleHeight*0.7f;
		float textPadding = fontHeight*0.3f;

		glDepthMask(false);

		drawRect(ws->windowRect, cBorder);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glBlendEquation(GL_FUNC_ADD);

		setSRGB(false);
		drawRectNewColoredW(ws->titleRect, cTitleBarLeft, cTitleBarRight);
		setSRGB(true);

		{
			glLineWidth(1);
			Vec2 p0 = ws->titleRect.min;
			Vec2 p1 = rectBR(ws->titleRect);
			drawLine(vec2(roundFloat(p0.x),roundFloat(p0.y)+0.5f), vec2(roundFloat(p1.x),roundFloat(p1.y)+0.5f), cBorder);
			glLineWidth(1);
		}

		if(sd->ncTestRegion == HTMINBUTTON) cButton0.rgb += vec3(0.4f);
		if(sd->ncTestRegion == HTMAXBUTTON) cButton1.rgb += vec3(0.4f);
		if(sd->ncTestRegion == HTCLOSE)     cButton2.rgb += vec3(0.4f);

		{
			glLineWidth(1);

			float off = rectW(sd->rMinimize)*0.2f;
			{
				Rect r = sd->rMinimize;
				Vec2 p0 = rectBL(r) + vec2(off);
				Vec2 p1 = rectBR(r) + vec2(-off,off);
				drawLine(roundVec2(p0)+vec2(0,0.5f), roundVec2(p1)+vec2(0,0.5f), cButton0);
			}

			{
				Rect r = sd->rMaximize;
				r = rectExpand(r, vec2(-off*2));
				r = rectRound(r);
				drawRectOutline(r, cButton1);
				drawLine(rectTL(r)+vec2(0,-1.5f), rectTR(r)+vec2(0,-1.5f), cButton1);
			}

			{
				Rect r = sd->rClose;
				drawCross(roundVec2(rectCen(r)), rectW(r) - off*2, 1.5f, cButton2);
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

			drawText(titleText, tp, vec2i(-1,0), ts);

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
	}

	#if 1

	{
		// Mouse capture.
		{
			if(!ad->captureMouse) {
				if((input->keysPressed[KEYCODE_F3] || mouseButtonPressedRight(gui, input)) && ad->drawSceneWired) {
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
			ad->fpsMode = (ad->captureMouse || input->mouseButtonDown[1]) && windowHasFocus(windowHandle);
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
			if(input->keysDown[KEYCODE_CTRL]) o.dir = normVec3(cross(vec3(0,0,1), o.right));
			if(keyDown(gui, input, KEYCODE_W)) vel += o.dir;
			if(keyDown(gui, input, KEYCODE_S)) vel += -o.dir;
			if(keyDown(gui, input, KEYCODE_A)) vel += -o.right;
			if(keyDown(gui, input, KEYCODE_D)) vel += o.right;
			if(keyDown(gui, input, KEYCODE_E)) vel += vec3(0,0,1);
			if(keyDown(gui, input, KEYCODE_Q)) vel += vec3(0,0,-1);

			if(vel != vec3(0,0,0)) cam->pos += normVec3(vel) * speed;
		}

		{
			Texture* texture = &ad->raycastTexture;
			Vec2i texDim = ad->settings.texDim;
			if(!texture->isCreated || (texDim != texture->dim)) {
				if(texDim != texture->dim) deleteTexture(texture);

				initTexture(texture, -1, INTERNAL_TEXTURE_FORMAT, texDim, 3, GL_NEAREST, GL_CLAMP);
				// initTexture(texture, -1, INTERNAL_TEXTURE_FORMAT, texDim, 3, GL_LINEAR, GL_CLAMP);

				Texture* t = &ad->raycastTexture;
				Vec3 black = vec3(0.2f);
				glClearTexSubImage(t->id, 0, 0,0,0, t->dim.w,t->dim.h, 1, GL_RGB, GL_FLOAT, &ad->world.defaultEmitColor);
			}
		}

		if((keyPressed(gui, input, KEYCODE_SPACE)) && (!ad->activeProcessing && ad->drawSceneWired) && !ad->fpsMode) {

			ad->activeProcessing = true;
			ad->drawSceneWired = false;
			ad->tracerFinished = false;

			RaytraceSettings* settings = &ad->settings;

			settings->mode = RENDERING_MODE_RAY_TRACER; // MODE_PATH_TRACER

			Texture* t = &ad->raycastTexture;
			// Vec3 black = vec3(0.2f);
			glClearTexSubImage(t->id, 0, 0,0,0, t->dim.w,t->dim.h, 1, GL_RGB, GL_FLOAT, &ad->world.defaultEmitColor);

			int pixelCount = settings->texDim.w*settings->texDim.h;
			reallocArraySave(Vec3, ad->buffer, pixelCount);

			// Setup samples.
			{
				int mode = settings->sampleMode;

				Vec2* blueNoiseSamples;

				int sampleCount;
				if(mode == SAMPLE_MODE_GRID) sampleCount = settings->sampleCountGrid*settings->sampleCountGrid;
				else if(mode == SAMPLE_MODE_BLUE) {
					float sampleCountGrid = settings->sampleCountGrid * 1.3f;  // Mod for blue noise.
					sampleCount = blueNoise(rect(0,0,1,1), 1/(float)sampleCountGrid, &blueNoiseSamples);
				} else if(mode == SAMPLE_MODE_BLUE_MULTI) {
					float sampleCountGrid = settings->sampleCountGrid * 1.3f;
					sampleCount = blueNoise(rect(vec2(0.0f),vec2(settings->sampleGridWidth)), 1/(float)sampleCountGrid, &blueNoiseSamples);
				}
				else if(mode == SAMPLE_MODE_MSAA4X) sampleCount = 4;
				else if(mode == SAMPLE_MODE_MSAA8X) sampleCount = 8;

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
						for(int i = 0; i < sampleCount; i++) settings->samples[i] = blueNoiseSamples[i];
						free(blueNoiseSamples);
					} break;

					case SAMPLE_MODE_BLUE_MULTI: {

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

					case SAMPLE_MODE_MSAA4X: {
						for(int i = 0; i < sampleCount; i++) settings->samples[i] = msaa4xPatternSamples[i];
					} break;

					case SAMPLE_MODE_MSAA8X: {
						for(int i = 0; i < sampleCount; i++) settings->samples[i] = msaa8xPatternSamples[i];
					} break;
				}
			}

			// Precalc stuff.
			{
				World* world = &ad->world;
				settings->pixelPercent = vec2(1/(float)settings->texDim.w, 1/(float)settings->texDim.h);

				Camera camera = world->camera;
				OrientationVectors ovecs = camera.ovecs;
				settings->camTopLeft = camera.pos + ovecs.dir*camera.nearDist + (ovecs.right*-1)*(camera.dim.w/2.0f) + (ovecs.up)*(camera.dim.h/2.0f);
			}

			int threadCount = ad->threadCount;
			int pixelCountPerThread = pixelCount/threadCount;
			int pixelCountRest = pixelCount % threadCount;

			for(int i = 0; i < threadCount; i++) {
				ProcessPixelsData* data = ad->threadData + i;
				*data = {};
				data->pixelIndex = i*pixelCountPerThread;
				data->pixelCount = i < threadCount-1 ? pixelCountPerThread : pixelCountPerThread + pixelCountRest;
				data->world = &ad->world;
				data->settings = settings;
				data->buffer = ad->buffer;

				threadQueueAdd(globalThreadQueue, processPixelsThreaded, data);
			}

			ad->processStartTime = ad->time;
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
			glTextureSubImage2D(ad->raycastTexture.id, 0, 0, 0, ad->settings.texDim.w, ad->settings.texDim.h, GL_RGB, GL_FLOAT, ad->buffer);
			glGenerateTextureMipmap(ad->raycastTexture.id);
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
				Vec2 c = rectCen(sr);
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

			if(!ad->drawSceneWired) {
				glDepthMask(false);
				drawRect(ad->textureScreenRect, vec4(1,1,1,1), rect(0,0,1,1), ad->raycastTexture.id);
				glDepthMask(true);
			}
		}

		if(keyPressed(gui, input, KEYCODE_F)) ad->fitToScreen = !ad->fitToScreen;
	}

	{
		// @EntityUI code

		World* world = &ad->world;
		EntityUI* eui = &ad->entityUI;
		Camera* cam = &ad->world.camera;

		eui->guiHasFocus = guiHotMouseClick(gui);

		if(!ad->drawSceneWired) {
			eui->selectedObject = 0;
		}

		// @Dialogs.
		{
			DialogData* dd = &ad->dialogData;
			if(dd->finished) {
				if(!dd->error) {
					if(strCompare(dd->type, "SceneDialog")) {
						if(dd->saveMode) {
							saveScene(world, dd->result);
							ad->sceneHasFile = true;
							strCpy(ad->sceneFile, dd->result);
						} else {
							loadScene(world, dd->result);
							ad->sceneHasFile = true;
							strCpy(ad->sceneFile, dd->result);
						}

					} else if(strCompare(dd->type, "ScreenshotDialog")) {
						Vec2i texDim = ad->settings.texDim;
						int size = texDim.w * texDim.h;
						char* intBuffer = mallocArray(char, size*3);

						Vec3* floatBuffer = ad->buffer;
						for(int i = 0; i < size; i++) {
							intBuffer[i*3 + 0] = colorFloatToInt(floatBuffer[i].r);
							intBuffer[i*3 + 1] = colorFloatToInt(floatBuffer[i].g);
							intBuffer[i*3 + 2] = colorFloatToInt(floatBuffer[i].b);
						}

						stbi_write_png(ad->dialogData.filePath, texDim.w, texDim.h, 3, intBuffer, 0);

						free(intBuffer);
					}
				}

				dd->finished = false;
				gui->activeId = 0;
				ws->dontUpdateCursor = false;
			}

			// If active dialog, disable everything.
			if(dd->active) {
				ws->dontUpdateCursor = true;
				gui->activeId = -1;
			}
		}

		if(ad->drawSceneWired) {
			if(mouseWheel(gui, input) && eui->selectionState != ENTITYUI_ACTIVE) {
				ad->entityUI.selectionMode = mod(ad->entityUI.selectionMode - mouseWheel(gui, input), ENTITYUI_MODE_SIZE);
			}

			if(input->keysDown[KEYCODE_SHIFT]) {
				eui->snappingEnabled = true;
			} else {
				eui->snappingEnabled = false;
			}

			// @Selection.

			if(mouseButtonPressedLeft(gui, input) && eui->selectionState == ENTITYUI_INACTIVE) {
				Vec3 rayDir = mouseRayCast(ad->textureScreenRectFitted, input->mousePosNegative, &ad->world.camera);

				int objectIndex = castRay(ad->world.camera.pos, rayDir, ad->world.objects, ad->world.objectCount);
				if(objectIndex != -1) {
					eui->selectedObject = objectIndex+1;
					eui->selectionAnimState = 0;
				} else eui->selectedObject = 0;
			}

			if(input->mouseButtonReleased[0] && eui->selectionState == ENTITYUI_ACTIVE) {
				eui->selectionState = ENTITYUI_INACTIVE;
			}

			if(keyPressed(gui, input, KEYCODE_ESCAPE) && eui->selectedObject) {
				eui->selectedObject = 0;
				eui->selectionState = ENTITYUI_INACTIVE;
			}

			if(eui->selectedObject) {
				// Delete.
				if(keyPressed(gui, input, KEYCODE_DEL)) {
					deleteObject(world->objects, &world->objectCount, &eui->selectedObject, &eui->selectionState, false);
				}

				// Copy.
				if(keyDown(gui, input, KEYCODE_CTRL) && keyPressed(gui, input, KEYCODE_C)) {
					eui->objectCopy = world->objects[eui->selectedObject-1];
				}

				// Cut.
				if(keyDown(gui, input, KEYCODE_CTRL) && keyPressed(gui, input, KEYCODE_X)) {
					// Copy and delete.
					eui->objectCopy = world->objects[eui->selectedObject-1];
					deleteObject(world->objects, &world->objectCount, &eui->selectedObject, &eui->selectionState, false);
				}
			}

			// Insert default.
			if(keyPressed(gui, input, KEYCODE_RETURN)) {
				int id = insertObject(&ad->world, defaultObject());
				if(eui->selectedObject) eui->selectedObject = id;
			}

			// Insert.
			if(keyDown(gui, input, KEYCODE_CTRL) && keyPressed(gui, input, KEYCODE_V)) {
				int id = insertObject(&ad->world, eui->objectCopy, true);
				if(id) {
					eui->selectedObject = id;
				}
			}

			if(keyPressed(gui, input, KEYCODE_TAB)) eui->localMode = !eui->localMode;

			if(keyPressed(gui, input, KEYCODE_1)) eui->selectionMode = ENTITYUI_MODE_TRANSLATION;
			if(keyPressed(gui, input, KEYCODE_2)) eui->selectionMode = ENTITYUI_MODE_ROTATION;
			if(keyPressed(gui, input, KEYCODE_3)) eui->selectionMode = ENTITYUI_MODE_SCALE;



			eui->gotActive = false;

			if(mouseButtonPressedLeft(gui, input) && eui->selectionState == ENTITYUI_HOT) {
				eui->selectionState = ENTITYUI_ACTIVE;
				eui->gotActive = true;
			}


			if(eui->selectionState == ENTITYUI_ACTIVE) {

				Object* obj = ad->world.objects + (eui->selectedObject-1);
				Camera* cam = &ad->world.camera;
				Vec3 rayPos = cam->pos;
				Vec3 rayDir = mouseRayCast(ad->textureScreenRectFitted, input->mousePosNegative, &ad->world.camera);

				if(eui->selectionMode == ENTITYUI_MODE_TRANSLATION) {

					if(eui->translateMode == TRANSLATE_MODE_AXIS) {
						Vec3 cameraOnAxis = projectPointOnLine(obj->pos, eui->axis, cam->pos);
						Vec3 planeNormal = normVec3(cam->pos - cameraOnAxis);

						Vec3 planeIntersection;
						float distance = linePlaneIntersection(rayPos, rayDir, obj->pos, planeNormal, &planeIntersection);
						if(distance != -1) {
							Vec3 linePointOnAxis = projectPointOnLine(obj->pos, eui->axis, planeIntersection);

							// Init.
							if(eui->gotActive) {
								eui->objectDistanceVector = obj->pos - linePointOnAxis;
								eui->startPos = obj->pos;
							}

							obj->pos = linePointOnAxis + eui->objectDistanceVector;
						}

					} else if(eui->translateMode == TRANSLATE_MODE_PLANE) {
						Vec3 planeNormal = eui->axis;

						Vec3 planeIntersection;
						float distance = linePlaneIntersection(rayPos, rayDir, obj->pos, planeNormal, &planeIntersection);
						if(distance != -1) {

							// Init.
							if(eui->gotActive) {
								eui->objectDistanceVector = obj->pos - planeIntersection;
								eui->startPos = obj->pos;
							}

							obj->pos = planeIntersection + eui->objectDistanceVector;
						}
					} else if(eui->translateMode == TRANSLATE_MODE_CENTER) {
						Vec3 pos = obj->pos + eui->centerOffset;
						Vec3 pp = cam->pos + cam->ovecs.dir * eui->centerDistanceToCam + eui->centerOffset;

						Vec3 planeIntersection;
						float distance = linePlaneIntersection(rayPos, rayDir, pp, -cam->ovecs.dir, &planeIntersection);
						if(distance != -1) {

							// Init.
							if(eui->gotActive) {
								eui->objectDistanceVector = eui->centerOffset;
								eui->startPos = obj->pos;
							}

							obj->pos = planeIntersection - eui->objectDistanceVector;
						}
					}

					if(eui->snappingEnabled) {

						// We always snap every axis to keep it simple.
						for(int i = 0; i < 3; i++) {
							Vec3 p = projectPointOnLine(eui->startPos, eui->axes[i], obj->pos);
							float length = lenVec3(p - eui->startPos);
							float snappedLength = roundMod(length, eui->snapGridSize);
							float lengthDiff = length - snappedLength;

							if(dot(p - eui->startPos, eui->axes[i]) > 0) lengthDiff *= -1;

							obj->pos += eui->axes[i]*lengthDiff;
						}
					}
				}

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

							eui->startDim = obj->dim.e[eui->axisIndex-1];
						}

						float oldLength = lenVec3(eui->objectDistanceVector);
						float newLength = lenVec3(obj->pos - linePointOnAxis);

						float currentAxisLength = eui->startDim + (newLength - oldLength)*2;

						currentAxisLength = clampMin(currentAxisLength, 0);
						if(dot(eui->objectDistanceVector, obj->pos - linePointOnAxis) < 0) currentAxisLength = 0;

						if(eui->snappingEnabled) {
							currentAxisLength = roundMod(currentAxisLength, eui->snapGridSize);
						}

						if(obj->geometry.type == GEOM_TYPE_SPHERE) {
							// obj->dim = vec3(currentAxisLength*0.5f);
							obj->dim = vec3(currentAxisLength);
						} else if(obj->geometry.type == GEOM_TYPE_BOX) {
							obj->dim.e[eui->axisIndex-1] = currentAxisLength;
						}

						geometryBoundingSphere(obj);
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

		Rect tr = ad->textureScreenRectFitted;

		glClearColor(0,0,0, 1);
		// glClear(GL_COLOR_BUFFER_BIT);

		Vec2i res = ws->clientRes;
		setClientViewport(ws, rect(tr.left, res.h+tr.bottom, tr.right, res.h+tr.top));


		glDepthMask(false);
		Vec3 cc = world->defaultEmitColor;
		drawRect(rectCenDim(0,0,10000,10000), vec4(cc,1));
		glDepthMask(true);

			// GLfloat mat_specular[] = { 1.0, 1.0, 1.0, 1.0 };
			// GLfloat mat_shininess[] = { 50.0 };
			// // glClearColor (0.0, 0.0, 0.0, 0.0);
			// glShadeModel (GL_SMOOTH);

			// glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
			// glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);
			// glLightfv(GL_LIGHT0, GL_POSITION, light_position);

			// glShadeModel(GL_FLAT);
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

		Light light;
		light.type = LIGHT_TYPE_DIRECTION;
		light.pos = normVec3(vec3(-1.5f,-1,-2.0f));
		// light.diffuseColor = vec3(0.9,0,0);
		light.diffuseColor = vec3(0.9f);
		light.specularColor = vec3(1.0f);
		light.brightness = 1.0f;

			Vec4 lp = vec4(-light.dir, 0);
			glLightfv(GL_LIGHT0, GL_POSITION, lp.e);

		// Vec4 lightAmbientColor = vec4(world->defaultEmitColor,1);
		// glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbientColor.e);

		// Vec4 lightDiffuseColor = vec4(world->globalLightColor,1);
			Vec4 lightDiffuseColor = vec4(light.diffuseColor,1);
			glLightfv(GL_LIGHT0, GL_AMBIENT, lightDiffuseColor.e);

		// Vec4 lightDiffuseColor = vec4(world->globalLightColor,1);
		// glLightfv(GL_LIGHT0, GL_AMBIENT, lightDiffuseColor.e);



		glEnable(GL_DEPTH_TEST);
		glBindTexture(GL_TEXTURE_2D, 0);

		{
			// @Grid
			EntityUI* eui = &ad->entityUI;

			glDisable(GL_LIGHTING);
			ad->entityUI.snapGridSize = 3;

			glLineWidth(1);

			Vec4 lineColor = vec4(0,0.5f);
			float size = ad->entityUI.snapGridSize;
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
			// glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			// glDisable(GL_CULL_FACE);

			// Vec4 ambientColor = COLOR_SRGB(vec4(world->defaultEmitColor, 1));
			// Vec4 ambientColor = COLOR_SRGB(vec4(vec3(1,0,0), 1));
			// glMaterialfv(GL_FRONT, GL_AMBIENT, ambientColor.e);

			glPushMatrix();
			glLoadIdentity();

			for(int i = 0; i < world->objectCount; i++) {
				Object* obj = world->objects + i;
				Geometry* g = &obj->geometry;
				Material* m = &obj->material;

				Vec4 diffuseColor = COLOR_SRGB(vec4(obj->color, 1));
				// glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuseColor.e);
				glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, diffuseColor.e);

				Vec4 emissionColor = COLOR_SRGB(vec4(m->emitColor, 1));
				glMaterialfv(GL_FRONT, GL_EMISSION, emissionColor.e);

				float shininess = lerp(m->reflectionMod, 0,128);
				glMaterialf(GL_FRONT, GL_SHININESS, shininess);


				Vec4 c = COLOR_SRGB(vec4(obj->color, 1));
				glColor4f(c.r, c.g, c.b, c.a);

				Vec3 scale = vec3(1,1,1);
				scale = obj->dim;
				if(g->type == GEOM_TYPE_SPHERE) scale = scale / 2;

				Mat4 tm = translationMatrix(obj->pos);
				Mat4 rm = quatRotationMatrix(obj->rot);
				Mat4 sm = scaleMatrix(scale);

				Mat4 fm = vm * tm * rm * sm;
				rowToColumn(&fm);
				glLoadMatrixf(fm.e);

				switch(g->type) {
					case GEOM_TYPE_BOX: {
						drawBoxRaw();
					} break;

					case GEOM_TYPE_SPHERE: {
						drawSphereRaw();
					} break;
				}

				// Draw ui grid.

				if(i == ad->entityUI.selectedObject-1) {

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
						float animSpeed = 3;
						ad->entityUI.selectionAnimState += ad->dt * animSpeed;
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

					c = COLOR_SRGB(color);
					glColor4f(c.r, c.g, c.b, c.a);

					glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
					glDisable(GL_LIGHTING);

					glLineWidth(0.5f);

					switch(g->type) {
						case GEOM_TYPE_BOX: {
							drawBoxRaw();
						} break;

						case GEOM_TYPE_SPHERE: {
							drawSphereRaw();
						} break;
					}

					glEnable(GL_LIGHTING);
					glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
					glDisable(GL_CULL_FACE);
					glLineWidth(1);

					// glEnable(GL_DEPTH_TEST);
				}
			}

			glPopMatrix();

			// @EntityUI draw.
			{
				EntityUI* eui = &ad->entityUI;

				if(eui->selectedObject) {
					Object* obj = world->objects + eui->selectedObject-1;
					Geometry* geom = &obj->geometry;

					// Ugh. Make sure to not draw anything after this.
					// glClear(GL_DEPTH_BUFFER_BIT);
					glDisable(GL_LIGHTING);

					// Draw Gizmo.

					Camera* cam = &world->camera;
					float boundRadius = geom->boundingSphereRadius*1.5f;
					float uiAlpha = 0.7f;

					Vec4 rotationSegmentColor = vec4(hslToRgbFloat(vec3(0.5f,0.9f,0.5f)), 0.5f);
					Vec4 translationVectorColor = rotationSegmentColor;
					translationVectorColor.a = 1;
					Vec4 translationCenterBoxColor = vec4(0.5f,1);
					float translationCenterBoxColorMod = 0.1f;


					float distToCam = lenVec3(cam->pos - obj->pos);
					float translateLineWidth = 5;

					float translationArrowLength = distToCam * 0.2f;
					float translationArrowThickness = distToCam * 0.008f;
					float translationVectorThickness = translationArrowThickness*0.5f;

					float translationPlaneSize = translationArrowLength * 0.25f;

					float translationCenterBoxSize = distToCam * 0.015f;

					float rotationRingThickness = translationArrowThickness;
					float rotationRingRadius = translationArrowLength;

					float scaleArrowBoxDim = translationCenterBoxSize;
					float scaleArrowLineWidth = 3.0f;



					if(eui->selectionMode == ENTITYUI_MODE_TRANSLATION) {
						Vec2 d = vec2(translationArrowThickness,translationArrowLength);
						Vec3 n = cam->pos - obj->pos;
						Vec3 c[] = { vec3(1,0,0), vec3(0,1,0), vec3(0,0,1) };
						Vec3 colorSquares = vec3(1);

						Vec3 axis[3];
						for(int i = 0; i < 3; i++) {
							Vec3 a = vec3(0,0,0);
							a.e[i] = 1;

							if(eui->localMode) a = obj->rot * a;

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
								Vec3 pp = obj->pos + axis[i]*d.h*0.5f;
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

							drawArrow(obj->pos, obj->pos + axis[i]*d.h, n, d.w, vec4(c[i],a));
						}

						// Move plane.

						int planeIndex = 0;
						for(int i = 0; i < 3; i++) {
							widgetId++;

							// Whatever.
							float dim = translationPlaneSize;
							Vec3 edgePoint = obj->pos + axis[(i+1)%3]*d.h + axis[(i+2)%3]*d.h;
							Vec3 diag = normVec3(edgePoint - obj->pos);

							float squareDiag = sqrt(2 * (dim*dim));
							Vec3 p = obj->pos + diag*(squareDiag/2) * 2;


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
								bool result = boxRaycastRotated(cam->pos, rayDir, obj->pos, vec3(translationCenterBoxSize), obj->rot, &intersection);
								if(result) {
									eui->centerOffset = intersection - obj->pos;
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
								drawBox(obj->pos, vec3(translationCenterBoxSize), &vm, obj->rot, color);
							} else {
								drawBox(obj->pos, vec3(translationCenterBoxSize), &vm, quat(), color);
							}
						}

						// Show current translation vector.

						if(eui->selectionState == ENTITYUI_ACTIVE) {
							// glEnable(GL_DEPTH_TEST);

							glLineWidth(translateLineWidth);

							drawArrow(eui->startPos, obj->pos, n, translationVectorThickness, translationVectorColor);

							{
								Vec3 globalAxis[] = {vec3(1,0,0), vec3(0,1,0), vec3(0,0,1)};
								float l[3];
								for(int i = 0; i < 3; i++) {
									if(dot(globalAxis[i], eui->startPos-obj->pos) > 0) globalAxis[i] *= -1;

									l[i] = lenVec3(projectPointOnLine(eui->startPos, globalAxis[i], obj->pos) - eui->startPos);		
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
									eui->axis = cam->pos - obj->pos;
									eui->centerDistanceToCam = lenVec3(vectorToCam(obj->pos, cam));
									eui->selectionState = ENTITYUI_HOT;
									eui->translateMode = TRANSLATE_MODE_CENTER;
								} else if(planeIndex) {
									eui->axis = axis[planeIndex-1];
									eui->axisIndex = planeIndex;
									eui->selectionState = ENTITYUI_HOT;
									eui->translateMode = TRANSLATE_MODE_PLANE;
								} else if(axisIndex) {
									eui->axis = axis[axisIndex-1];
									eui->axisIndex = axisIndex;
									eui->selectionState = ENTITYUI_HOT;
									eui->translateMode = TRANSLATE_MODE_AXIS;
								}
								for(int i = 0; i < 3; i++) {
									eui->axes[i] = axis[i];
								}
							} else {
								eui->selectionState = ENTITYUI_INACTIVE;
							}
						}

						eui->hotId = contenderId;
					}

					if(eui->selectionMode == ENTITYUI_MODE_ROTATION) {

						float thickness = rotationRingThickness;
						float radius = rotationRingRadius;
						// Vec3 axis[] = { vec3(1,0,0), vec3(0,1,0), vec3(0,0,1) };
						Vec3 c[] = { vec3(1,0,0), vec3(0,1,0), vec3(0,0,1) };

						Vec3 axis[3];
						for(int i = 0; i < 3; i++) {
							Vec3 a = vec3(0,0,0);
							a.e[i] = 1;

							if(eui->localMode) a = obj->rot * a;

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
								float dist = linePlaneIntersection(cam->pos, rayDir, obj->pos, axis[i], &intersection);
								if(dist != -1) {
									float distToObj = lenVec3(intersection - obj->pos);
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
					
						// Draw rings.

						// Should switch to different depth buffer, but this works for now.
						glClear(GL_DEPTH_BUFFER_BIT);

						drawSphere(obj->pos, radius-thickness, vec4(0,0,0,0));

						for(int i = 0; i < 3; i++) {
							float a = uiAlpha;
							drawRing(obj->pos, axis[i], radius, thickness, vec4(c[i],a));
						}

						// Paint over hot/active ring.
						if(axisIndex && (!eui->guiHasFocus || eui->selectionState == ENTITYUI_ACTIVE)) {
							glDisable(GL_DEPTH_TEST);
	
							int i = axisIndex-1;
							drawRing(obj->pos, axis[i], radius, thickness, vec4(c[i],1));

							if(eui->selectionState == ENTITYUI_ACTIVE) {
								Vec3 left = obj->pos + normVec3(eui->objectDistanceVector)*radius;
								drawTriangleFan(obj->pos, left, eui->currentRotationAngle, axis[i], rotationSegmentColor);
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

					if(eui->selectionMode == ENTITYUI_MODE_SCALE) {

						Vec3 c[] = { vec3(0.9f,0,0), vec3(0,0.9f,0), vec3(0,0,0.9f) };

						Vec3 hotAxis = {};
						int hotAxisIndex = 0;
						for(int i = 0; i < 3; i++) {
							Vec3 color = c[i];

							float arrowLength = obj->dim.e[i]/2 + scaleArrowBoxDim;

							Vec3 axis = vec3(0,0,0);
							axis.e[i] = 1;
							axis = obj->rot*axis;
							if(dot(axis, cam->pos - obj->pos) < 0) axis *= -1;
							Vec3 p = obj->pos + axis * arrowLength;

							if(eui->selectionState != ENTITYUI_ACTIVE) {
								Vec3 rayDir = mouseRayCast(ad->textureScreenRectFitted, input->mousePosNegative, cam);
								Vec3 intersection;
								bool result = boxRaycastRotated(cam->pos, rayDir, p, vec3(scaleArrowBoxDim), obj->rot, &intersection);
								if(result) {
									hotAxisIndex = i+1;
									hotAxis = normVec3(p - obj->pos);
									eui->centerOffset = intersection - p;

									if(!eui->guiHasFocus)
										color += vec3(0.2f);
								}
							} else {
								if(eui->axisIndex-1 == i) color += vec3(0.2f);
							}

							glDisable(GL_DEPTH_TEST);
							glLineWidth(scaleArrowLineWidth);
							drawLine(obj->pos, p, vec4(color,1));
							glLineWidth(1);
							drawBox(p, vec3(scaleArrowBoxDim), &vm, obj->rot, vec4(color, 1));
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
	#endif

	#if 1
	{
		setClientViewport(ws, rect(0,0,ws->clientRes.w, ws->clientRes.h));
		setClientScissor(ws, rect(0,0,ws->clientRes.w, ws->clientRes.h));



		NewGui* gui = &ad->gui;

		if(ad->entityUI.selectionState == ENTITYUI_ACTIVE) gui->hotId[Gui_Focus_MLeft] = 0;

		{
			float fontHeight = ad->fontHeight;
			Font* font = getFont(ad->fontFile, ad->fontHeight, ad->fontFileBold, ad->fontFileItalic);
			
			// Dark.
			Vec4 cText = vec4(1,1);
			Vec4 cBackground = vec4(0.3f,1);
			Vec4 cEdit = vec4(0.2f,1);
			Vec4 cButton = vec4(0.4f,1);
			Vec4 cOutline = vec4(0.5f,1);

			// Bright.
			// Vec4 cText = vec4(0,1);
			// Vec4 cBackground = vec4(1,1);
			// Vec4 cEdit = vec4(0.9f,1);
			// Vec4 cButton = vec4(0.7f,1);
			// Vec4 cOutline = vec4(0.1f,1);

			float panelRounding = 7;
			float buttonRounding = 4;

			float textPadding = font->height*0.4f;

			BoxSettings bs = boxSettings(cBackground, 0, cOutline);
			TextSettings ts = textSettings(font, cText);

			BoxSettings bous = boxSettings(cButton, buttonRounding, cOutline);
			TextBoxSettings bus = textBoxSettings(ts, bous);

			gui->textSettings = ts;
			gui->boxSettings = bs;
			gui->textBoxSettings = textBoxSettings(ts, bs);
			gui->buttonSettings = bus;

			TextBoxSettings etbs = gui->textBoxSettings;
			etbs.boxSettings.color = cEdit;
			etbs.boxSettings.roundedCorner = 0;
			gui->editSettings = textEditSettings(etbs, vec4(0,0,0,0), gui->editText, ESETTINGS_SINGLE_LINE | ESETTINGS_START_RIGHT, 1, 1.1f, vec4(0.5f,0.2,0,1), vec4(0,1,1,1), textPadding);
			float sw = fontHeight*1.0f;
			gui->sliderSettings = sliderSettings(etbs, sw, sw, 0, 0, 4, cButton, vec4(0,0,0,0));

			// ScrollRegionSettings scrollSettings;
			gui->popupSettings = boxSettings(cEdit, 0, cOutline);
			gui->comboBoxSettings = textBoxSettings(gui->textSettings, boxSettings(cEdit, 0, cOutline), textPadding);

			BoxSettings cbs = boxSettings(cEdit, panelRounding, cOutline);
			gui->checkBoxSettings = checkBoxSettings(cbs, cButton, 0.5f);
		}

		ad->menuHeight = ad->fontHeight * 1.5f;

		// @Menu.
		{
			float menuHeight = ad->menuHeight;
			Rect mr = rectRSetB(rectTLDim(0,0, ws->clientRes.w, ws->clientRes.h), menuHeight);

			Vec4 cMenu = vec4(0.3f,1);
			float padding = ad->fontHeight * 1.4;
			float border = 1;

			Font* font = gui->textSettings.font;
			char* s;

			drawRect(mr, cMenu);


			TextBoxSettings tbs = textBoxSettings(gui->textSettings, boxSettings(cMenu));
			TextBoxSettings tbsActive = gui->editSettings.textBoxSettings;

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
				float buttonOffset = ad->fontHeight*1;
				float buttonMargin = buttonWidth * 0.3f;
				float separatorWidth = padding*0.5f;
				Vec4 cButtonActive = vec4(1,1);
				Vec4 cButtonInactive = vec4(0.7f,1);
				Vec4 cSeparator = gui->buttonSettings.boxSettings.borderColor;

				p.x += buttonOffset;

				EntityUI* eui = &ad->entityUI;

				Rect r;
				
				// Make Screenshot.

				r = rectTLDim(p, vec2(buttonWidth)); p.x += buttonWidth;
				if(ad->tracerFinished) {
					if(newGuiQuickButton(gui, r, "", &tbs)) {
						openScreenshotDialog(&ad->dialogData);
					}
					rectExpand(&r, vec2(-buttonMargin));
					drawRect(rectRound(r), cButtonActive, rect(0,0,1,1), getTexture("screenshotIcon.png")->id);
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

				{
					p.x += separatorWidth * 0.5f;
					float x = roundFloat(p.x) + 0.5f;
					float off = padding*0.2f;
					drawLine(vec2(x, p.y - off), vec2(x, p.y-menuHeight + off), cSeparator);
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

				for(int i = 0; i < ENTITYUI_MODE_SIZE; i++) {
					r = rectTLDim(p, vec2(buttonWidth)); p.x += buttonWidth;

					if(eui->selectionMode == i) newGuiIncrementId(gui);
					else if(newGuiQuickPButton(gui, r, "", &tbs)) {
						eui->selectionMode = i;
					}
					rectExpand(&r, vec2(-buttonMargin));
					drawRect(rectRound(r), eui->selectionMode==i?cButtonActive:cButtonInactive, rect(0,0,1,1), getTexture(icons[i])->id);
				}

			}

		}



		bindFrameBuffer(FRAMEBUFFER_2dPanels);
		glEnable(GL_BLEND);
		glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);


		// @Panel

		Vec2 panelOffset = vec2(0,ad->menuHeight);
		float panelMargin = roundFloat(ad->fontHeight*0.3f);
		float panelWidthMax = ws->clientRes.w*0.5f;
		float panelWidthMin = 100;

		Vec2 panelDim;

		clamp(&ad->panelWidthLeft, panelWidthMin, panelWidthMax);
		clamp(&ad->panelWidthRight, panelWidthMin, panelWidthMax);

		panelDim = vec2(ad->panelWidthLeft,ad->panelHeightLeft);
		Rect rectPanelLeft = rectTLDim(vec2(panelOffset.x,-panelOffset.y), panelDim);

		Vec2i res = ws->clientRes;
		panelDim = vec2(ad->panelWidthRight,ad->panelHeightRight);
		Rect rectPanelRight = rectTRDim(vec2(res.w,0) + vec2(-panelOffset.x,-panelOffset.y), panelDim);

		// Panel fade animation.
		{
			bool inc = false;

			if(ad->entityUI.selectionState == ENTITYUI_ACTIVE &&
			   (pointInRect(input->mousePosNegative, rectPanelLeft) || 
			   pointInRect(input->mousePosNegative, rectPanelRight))) {

				inc = true;
			}

			float speed = 1.0f;

			if(!inc) speed *= -2.5f;
			ad->panelAlphaFadeState += ad->dt * speed;

			ad->panelAlphaFadeState = clamp(ad->panelAlphaFadeState, 0, 0.8f);
			ad->panelAlpha = 1-pow(ad->panelAlphaFadeState,2);
		}

		// Left panel.
		#if 1
		{
			newGuiSetHotAllMouseOver(gui, rectPanelLeft, gui->zLevel);

			// Resize panel width.
			{
				float border = panelMargin;
				Rect r = rectRSetL(rectPanelLeft, border);

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
				}
				if(newGuiIsHot(gui)) {
					newGuiSetCursor(gui, IDC_SIZEWE);
				}
			}

			newGuiQuickBox(gui, rectPanelLeft);

			{
				newGuiScissorPush(gui, rectExpand(rectPanelLeft, vec2(-2)));

				Rect pri = rectExpand(rectPanelLeft, -vec2(panelMargin*2));

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

					TextSettings headerTextSettings = textSettings(gui->textSettings.font, gui->textSettings.color, TEXTSHADOW_MODE_SHADOW, 1.0f, vec4(0, 1));
					TextBoxSettings headerSettings = textBoxSettings(headerTextSettings, boxSettings(vec4(0,0.4f,0.6f,0.5f)));

					Rect r;
					char* s;
					QuickRow qr;


					r = rectTLDim(p, vec2(ew, eh)); p.y -= eh+pad.y;
					r = rectExpand(r, vec2((panelMargin-1)*2,-eh*0.2f));
					newGuiQuickTextBox(gui, r, "<b>Pathtracer Settings<b>", vec2i(0,0), &headerSettings);

					s = "TexDim";
					r = rectTLDim(p, vec2(ew, eh)); p.y -= eh+pad.y;
					qr = quickRow(r, pad.x, getTextDim(s, font).w, 0, 0);
					newGuiQuickText(gui, quickRowNext(&qr), s, vec2i(-1,0));
					newGuiQuickTextEdit(gui, quickRowNext(&qr), &settings->texDim.w);
					newGuiQuickTextEdit(gui, quickRowNext(&qr), &settings->texDim.h);

					s = "SampleMode";
					r = rectTLDim(p, vec2(ew, eh)); p.y -= eh+pad.y;
					qr = quickRow(r, pad.x, getTextDim(s, font).w, 0);
					newGuiQuickText(gui, quickRowNext(&qr), s, vec2i(-1,0));
					newGuiQuickComboBox(gui, quickRowNext(&qr), &settings->sampleMode, sampleModeStrings, arrayCount(sampleModeStrings));

					s = "SampleGridDim";
					r = rectTLDim(p, vec2(ew, eh)); p.y -= eh+pad.y;
					qr = quickRow(r, pad.x, getTextDim(s, font).w, 0);
					newGuiQuickText(gui, quickRowNext(&qr), s, vec2i(-1,0));
					newGuiQuickTextEdit(gui, quickRowNext(&qr), &settings->sampleCountGrid);

					s = "SampleCellCount";
					r = rectTLDim(p, vec2(ew, eh)); p.y -= eh+pad.y;
					qr = quickRow(r, pad.x, getTextDim(s, font).w, 0);
					newGuiQuickText(gui, quickRowNext(&qr), s, vec2i(-1,0));
					newGuiQuickTextEdit(gui, quickRowNext(&qr), &settings->sampleGridWidth);

					s = "MaxRayBounces";
					r = rectTLDim(p, vec2(ew, eh)); p.y -= eh+pad.y;
					qr = quickRow(r, pad.x, getTextDim(s, font).w, 0);
					newGuiQuickText(gui, quickRowNext(&qr), s, vec2i(-1,0));
					newGuiQuickTextEdit(gui, quickRowNext(&qr), &settings->rayBouncesMax);



					r = rectTLDim(p, vec2(ew, eh)); p.y -= eh+pad.y;
					r = rectExpand(r, vec2((panelMargin-1)*2,-eh*0.2f));
					newGuiQuickTextBox(gui, r, "<b>Statistics<b>", vec2i(0,0), &headerSettings);

					eh = font->height;
					char* stats[] = {
						"Pixel count", fillString("%i.", settings->texDim.x * settings->texDim.h),
						"Samples per pixel", fillString("%i.", ad->settings.sampleCount), 
						"Total samples", fillString("%i.", settings->texDim.x * settings->texDim.h * ad->settings.sampleCount), 
						"Total time", fillString("%fs", (float)ad->processTime),
						"Time per pixel", fillString("%fms", (float)(ad->processTime/(settings->texDim.x*settings->texDim.y)*1000000)),
					};

					for(int i = 0; i < arrayCount(stats); i += 2) {
						char* s1 = stats[i];
						char* s2 = stats[i+1];

						r = rectTLDim(p, vec2(ew, eh)); p.y -= eh+pad.y;
						qr = quickRow(r, pad.x, getTextDim(s1, font).w, 0, getTextDim(s2, font).w);

						newGuiQuickText(gui, quickRowNext(&qr), s1, vec2i(-1,0));
						quickRowNext(&qr);
						newGuiQuickText(gui, quickRowNext(&qr), s2, vec2i(-1,0));
					}
					eh = elementHeight;


					r = rectTLDim(p, vec2(ew, eh)); p.y -= eh+pad.y;
					r = rectExpand(r, vec2((panelMargin-1)*2,-eh*0.2f));
					newGuiQuickTextBox(gui, r, "<b>Scene<b>", vec2i(0,0), &headerSettings);


					{
						World* world = &ad->world;
						Camera* cam = &world->camera;
						EntityUI* eui = &ad->entityUI;

						s = "Cam pos";
						r = rectTLDim(p, vec2(ew, eh)); p.y -= eh+pad.y;
						qr = quickRow(r, pad.x, getTextDim(s, font).w, 0,0,0);
						newGuiQuickText(gui, quickRowNext(&qr), s, vec2i(-1,0));
						newGuiQuickTextEdit(gui, quickRowNext(&qr), &cam->pos.x);
						newGuiQuickTextEdit(gui, quickRowNext(&qr), &cam->pos.y);
						newGuiQuickTextEdit(gui, quickRowNext(&qr), &cam->pos.z);

						s = "Cam rot";
						r = rectTLDim(p, vec2(ew, eh)); p.y -= eh+pad.y;
						qr = quickRow(r, pad.x, getTextDim(s, font).w, 0,0);
						newGuiQuickText(gui, quickRowNext(&qr), s, vec2i(-1,0));
						newGuiQuickTextEdit(gui, quickRowNext(&qr), &cam->rot.x);
						newGuiQuickTextEdit(gui, quickRowNext(&qr), &cam->rot.y);

						s = "Cam Fov";
						r = rectTLDim(p, vec2(ew, eh)); p.y -= eh+pad.y;
						qr = quickRow(r, pad.x, getTextDim(s, font).w, 0);
						newGuiQuickText(gui, quickRowNext(&qr), s, vec2i(-1,0));
						newGuiQuickSlider(gui, quickRowNext(&qr), &world->camera.fov, 20, 150);




						s = "ObjectCount";
						r = rectTLDim(p, vec2(ew, eh)); p.y -= eh+pad.y;
						qr = quickRow(r, pad.x, getTextDim(s, font).w, 0);
						newGuiQuickText(gui, quickRowNext(&qr), s, vec2i(-1,0));
						newGuiQuickText(gui, quickRowNext(&qr), fillString("%i", world->objectCount), vec2i(1,0));

						{
							float spawnDistance = 30;

							r = rectTLDim(p, vec2(ew, eh)); p.y -= eh+pad.y;
							qr = quickRow(r, pad.x, 0.0f, 0.0f);

							if(newGuiQuickButton(gui, quickRowNext(&qr), "Insert Object")) {
								int id = insertObject(&ad->world, defaultObject());
								if(eui->selectedObject) eui->selectedObject = id;
							}

							if(newGuiQuickButton(gui, quickRowNext(&qr), "Insert Copy")) {
								int id = insertObject(&ad->world, eui->objectCopy, true);
								if(eui->selectedObject) eui->selectedObject = id;
							}
						}

					}

				}

				ad->panelHeightLeft = rectPanelLeft.top - p.y - pad.y + panelMargin;

				newGuiScissorPop(gui);
			}
		}
		#endif

		// Right panel.
		#if 1
		{
			newGuiSetHotAllMouseOver(gui, rectPanelRight, gui->zLevel);

			// Resize panel width.
			{
				float border = panelMargin;
				Rect r = rectRSetR(rectPanelRight, border);

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
				}
				if(newGuiIsHot(gui)) {
					newGuiSetCursor(gui, IDC_SIZEWE);
				}
			}

			newGuiQuickBox(gui, rectPanelRight);

			{
				newGuiScissorPush(gui, rectExpand(rectPanelRight, vec2(-2)));

				Rect pri = rectExpand(rectPanelRight, -vec2(panelMargin*2));

				Font* font = gui->textSettings.font;

				float elementHeight = font->height * 1.5f;
				float elementWidth = rectW(pri);
				Vec2 padding = vec2(panelMargin-1, panelMargin-1);

				Vec2 p = rectTL(pri);
				float eh = elementHeight;
				float ew = elementWidth;
				Vec2 pad = padding;

				{
					TextSettings headerTextSettings = textSettings(gui->textSettings.font, gui->textSettings.color, TEXTSHADOW_MODE_SHADOW, 1.0f, vec4(0, 1));
					TextBoxSettings headerSettings = textBoxSettings(headerTextSettings, boxSettings(vec4(0,0.4f,0.6f,0.5f)));

					Rect r;
					char* s;
					char* t;
					QuickRow qr;


					RaytraceSettings* settings = &ad->settings;
					World* world = &ad->world;
					EntityUI* eui = &ad->entityUI;
					Object* obj = ad->world.objects + (eui->selectedObject-1);
					Geometry* geom = &obj->geometry;
					Material* mat = &obj->material;

					r = rectTLDim(p, vec2(ew, eh)); p.y -= eh+pad.y;
					
					float leftRightButtonWidth = eh * 1.2f;
					if(eui->selectedObject) {
						r = rectExpand(r, vec2(0,-eh*0.2f));
						qr = quickRow(r, pad.x, leftRightButtonWidth, 0, leftRightButtonWidth);
					} else {
						r = rectExpand(r, vec2((panelMargin-1)*2,-eh*0.2f));
						qr = quickRow(r, pad.x, 0);
					}

					if(eui->selectedObject) {
						if(newGuiQuickButton(gui, quickRowNext(&qr), "◄")) {
							eui->selectedObject = mod(eui->selectedObject-2, world->objectCount);
							eui->selectedObject++;
						}
					}

					newGuiQuickTextBox(gui, quickRowNext(&qr), "<b>Entities<b>", vec2i(0,0), &headerSettings);

					if(eui->selectedObject) {
						if(newGuiQuickButton(gui, quickRowNext(&qr), "►")) {
							eui->selectedObject = mod(eui->selectedObject, world->objectCount);
							eui->selectedObject++;
						}
					}


					if(eui->selectedObject) {

						s = "Id";
						t = fillString("%i", eui->selectedObject-1);
						r = rectTLDim(p, vec2(ew, eh)); p.y -= eh+pad.y;
						qr = quickRow(r, pad.x, getTextDim(s, font).w, getTextDim(t, font).w);
						newGuiQuickText(gui, quickRowNext(&qr), s, vec2i(-1,0));
						newGuiQuickText(gui, quickRowNext(&qr), t, vec2i(-1,0));

						s = "Position";
						r = rectTLDim(p, vec2(ew, eh)); p.y -= eh+pad.y;
						qr = quickRow(r, pad.x, getTextDim(s, font).w, 0,0,0);
						newGuiQuickText(gui, quickRowNext(&qr), s, vec2i(-1,0));
						newGuiQuickTextEdit(gui, quickRowNext(&qr), &obj->pos.x);
						newGuiQuickTextEdit(gui, quickRowNext(&qr), &obj->pos.y);
						newGuiQuickTextEdit(gui, quickRowNext(&qr), &obj->pos.z);

						s = "Rotation";
						r = rectTLDim(p, vec2(ew, eh)); p.y -= eh+pad.y;
						float cols[] = {getTextDim(s, font).w, 0,0,0,0};
						qr = quickRow(r, pad.x, cols, arrayCount(cols));
						newGuiQuickText(gui, quickRowNext(&qr), s, vec2i(-1,0));
						newGuiQuickTextEdit(gui, quickRowNext(&qr), &obj->rot.w);
						newGuiQuickTextEdit(gui, quickRowNext(&qr), &obj->rot.x);
						newGuiQuickTextEdit(gui, quickRowNext(&qr), &obj->rot.y);
						newGuiQuickTextEdit(gui, quickRowNext(&qr), &obj->rot.z);

						s = "Color";
						r = rectTLDim(p, vec2(ew, eh)); p.y -= eh+pad.y;
						qr = quickRow(r, pad.x, getTextDim(s, font).w, 0,0,0);
						newGuiQuickText(gui, quickRowNext(&qr), s, vec2i(-1,0));
						newGuiQuickTextEdit(gui, quickRowNext(&qr), &obj->color.r);
						newGuiQuickTextEdit(gui, quickRowNext(&qr), &obj->color.g);
						newGuiQuickTextEdit(gui, quickRowNext(&qr), &obj->color.b);

						s = "Type";
						r = rectTLDim(p, vec2(ew, eh)); p.y -= eh+pad.y;
						qr = quickRow(r, pad.x, getTextDim(s, font).w, 0);
						newGuiQuickText(gui, quickRowNext(&qr), s, vec2i(-1,0));
						if(newGuiQuickComboBox(gui, quickRowNext(&qr), &geom->type, geometryTypeStrings, arrayCount(geometryTypeStrings))) {

							if(geom->type == GEOM_TYPE_SPHERE) {
								obj->dim = vec3(max(obj->dim.x, obj->dim.y, obj->dim.z));
							}
							geometryBoundingSphere(obj);
						}

						s = "BoundingRadius";
						r = rectTLDim(p, vec2(ew, eh)); p.y -= eh+pad.y;
						qr = quickRow(r, pad.x, getTextDim(s, font).w, 0);
						newGuiQuickText(gui, quickRowNext(&qr), s, vec2i(-1,0));
						newGuiQuickText(gui, quickRowNext(&qr), fillString("%f", geom->boundingSphereRadius), vec2i(-1,0));

						s = "Dimension";
						r = rectTLDim(p, vec2(ew, eh)); p.y -= eh+pad.y;
						qr = quickRow(r, pad.x, getTextDim(s, font).w, 0,0,0);
						newGuiQuickText(gui, quickRowNext(&qr), s, vec2i(-1,0));
						int dimChanged = 0;
						if(newGuiQuickTextEdit(gui, quickRowNext(&qr), &obj->dim.x)) dimChanged = 1;
						if(newGuiQuickTextEdit(gui, quickRowNext(&qr), &obj->dim.y)) dimChanged = 2;
						if(newGuiQuickTextEdit(gui, quickRowNext(&qr), &obj->dim.z)) dimChanged = 3;

						if(dimChanged) {
							if(geom->type == GEOM_TYPE_SPHERE) {
								float d = obj->dim.e[dimChanged-1];
								obj->dim = vec3(d);
							}

							geometryBoundingSphere(obj);
						}

						s = "EmitColor";
						r = rectTLDim(p, vec2(ew, eh)); p.y -= eh+pad.y;
						qr = quickRow(r, pad.x, getTextDim(s, font).w, 0,0,0);
						newGuiQuickText(gui, quickRowNext(&qr), s, vec2i(-1,0));
						newGuiQuickTextEdit(gui, quickRowNext(&qr), &mat->emitColor.r);
						newGuiQuickTextEdit(gui, quickRowNext(&qr), &mat->emitColor.g);
						newGuiQuickTextEdit(gui, quickRowNext(&qr), &mat->emitColor.b);

						s = "Reflection";
						r = rectTLDim(p, vec2(ew, eh)); p.y -= eh+pad.y;
						qr = quickRow(r, pad.x, getTextDim(s, font).w, 0);
						newGuiQuickText(gui, quickRowNext(&qr), s, vec2i(-1,0));
						newGuiQuickSlider(gui, quickRowNext(&qr), &mat->reflectionMod, 0, 1);



						r = rectTLDim(p, vec2(ew, eh)); p.y -= eh+pad.y;
						qr = quickRow(r, pad.x, 0.0f, 0.0f);
						if(newGuiQuickButton(gui, quickRowNext(&qr), "Copy")) {
							eui->objectCopy = world->objects[eui->selectedObject-1];
						}
						if(newGuiQuickButton(gui, quickRowNext(&qr), "Delete")) {
							deleteObject(world->objects, &world->objectCount, &eui->selectedObject, &eui->selectionState);
						}

					}

				}

				ad->panelHeightRight = rectPanelRight.top - p.y - pad.y + panelMargin;

				newGuiScissorPop(gui);
			}
		}
		#endif



		newGuiPopupSetup(gui);

		// Handle custom popups.
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
				float ew = popupWidth;
				float ep = fontHeight * 0.7f;

				float textSidePadding = fontHeight*0.7f;
				float topBottomPadding = textSidePadding*0.4f;

				float separatorHeight = eh*0.4f;
				Vec4 cSeparator = gui->popupSettings.borderColor;


				float elementCount;
				float separatorCount;
				float popupHeight;

				if(strCompare(pd.name, "FileMenu")) {
					elementCount = 5;
					separatorCount = 2;
					popupHeight = elementCount*(eh) - padding + border*2 + topBottomPadding*2 + separatorHeight*separatorCount;
				} else if(strCompare(pd.name, "SettingsMenu")) {
					elementCount = 4;
					separatorCount = 1;
					popupHeight = elementCount*(eh) - padding + border*2 + topBottomPadding*2 + separatorHeight*separatorCount + topBottomPadding*0.5f;
				}


				Rect rPop = rectTLDim(pd.p-vec2(0,popupOffset), vec2(popupWidth,popupHeight));

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
							getDefaultScene(world);
							ad->sceneHasFile = false;
							strClear(ad->sceneFile);
							close = true;
						}

						r = rectTLDim(p, vec2(ew, eh)); p.y -= eh + padding;
						if(newGuiQuickPButton(gui, r, "Open...", vec2i(-1,0))) {
							openSceneDialog(&ad->dialogData);
							close = true;
						}

						{
							p.y -= separatorHeight*0.5f;
							Vec2 lp = vec2(p.x,roundFloat(p.y))+vec2(0,0.5f);
							drawLine(lp + vec2(textSidePadding*0.5f,0), lp + vec2(ew,0)  - vec2(textSidePadding*0.5f,0), cSeparator);
							p.y -= separatorHeight*0.5f;
						}

						r = rectTLDim(p, vec2(ew, eh)); p.y -= eh + padding;
						if(newGuiQuickPButton(gui, r, "Save", vec2i(-1,0))) {
							if(ad->sceneHasFile) saveScene(world, ad->sceneFile);
							else openSceneDialog(&ad->dialogData, true);

							close = true;
						}

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
	#endif


	openglDrawFrameBufferAndSwap(ws, systemData, &ad->swapTimer, init, ad->panelAlpha);

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
