/*
=================================================================================

	ToDo:
	- More Shapes.
	- Simd.
	- Ui.
	- Entity editor.
	- More advanced rendering techniques.
	- Put input updating in own thread.
	- Float not precise enough at hundreds of samples per pixel?!

	- Depth of field.
	- Reflection.
	- Refraction.
	
	- Multiple selection.
	- Revert.
	- Cleanup.

	- Panel.
	- Investigae slow compile times with -d2cgsummary flag.
	- Shortest distance to camera for widgets.

	- Check for monitor framerate and Sleep accordingly.
	- Cap framerate when not in vsync.

	- Detect windows text size and test ui with different sizes.

	- Menu.

	Done Today: 

	Bugs:
	- Windows key slow sometimes.
	- F11 rapid pressing.

=================================================================================
*/



// External.

#define WIN32_LEAN_AND_MEAN 
#define NOMINMAX
#include <windows.h>
#include <gl\gl.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG
#include "external\stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "external\stb_image_write.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_PARAMETER_TAGS_H
#include FT_MODULE_H

#define ENABLE_CUSTOM_WINDOW_FRAME

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





void updateWindowFrameData(SystemData* sd, WindowSettings* ws) {
	#ifdef ENABLE_CUSTOM_WINDOW_FRAME

	Vec2 bDim = vec2(sd->titleHeight);

	sd->titleHeight = sd->normalTitleHeight;
	sd->borderSize = sd->normalBorderSize;
	sd->visualBorderSize = sd->normalVisualBorderSize;

	if(sd->maximized) {
		sd->borderSize = 0;
		sd->visualBorderSize = 0;
	}

	if(ws->fullscreen) {
		sd->borderSize = 0;
		sd->visualBorderSize = 0;
		sd->titleHeight = 0;
	}

	Vec2i res = ws->currentRes;

	ws->windowRes = res;
	ws->windowRect = rect(0, -ws->windowRes.h, ws->windowRes.w, 0);

	Rect wr = ws->windowRect;
	ws->clientRect = rect(wr.min + vec2(sd->visualBorderSize), wr.max - vec2(sd->visualBorderSize, sd->titleHeight+sd->visualBorderSize));
	ws->clientRes = vec2i(roundInt(rectW(ws->clientRect)), roundInt(rectH(ws->clientRect)));

	float vbs = sd->visualBorderSize;
	ws->titleRect = rect(vbs, -sd->titleHeight - vbs, rectW(ws->windowRect)-vbs, -vbs);

	float bm = sd->buttonMargin*2;

	Rect titleRectWithSepLine = ws->titleRect;
	titleRectWithSepLine.bottom += 1;

	Vec2 p = vec2(titleRectWithSepLine.right - bDim.w/2, rectCen(titleRectWithSepLine).y);
	sd->rClose = rectCenDim(p, bDim - bm); p.x -= bDim.w - bm/2;
	sd->rMaximize = rectCenDim(p, bDim - bm); p.x -= bDim.w - bm/2;
	sd->rMinimize = rectCenDim(p, bDim - bm); p.x -= bDim.w - bm/2;

	if(ws->fullscreen) {
		sd->rClose    = rect(-1,-1,-1,-1);
		sd->rMaximize = rect(-1,-1,-1,-1);
		sd->rMinimize = rect(-1,-1,-1,-1);
	}

	ws->windowRectBL = rect(0, 0, res.w, res.h);

	ws->clientRectBL = ws->windowRectBL;
	ws->clientRectBL.min += vec2(sd->visualBorderSize);
	ws->clientRectBL.max -= vec2(sd->visualBorderSize,sd->visualBorderSize+sd->titleHeight);

	if(sd->maximized) {
		float sbs = ws->styleBorderSize;
		ws->windowRectBL = rect(sbs, sbs, sbs+res.w, sbs+res.h);
	}

	#else 

	Vec2i res = ws->currentRes;
	ws->clientRes = res;
	ws->windowRes = res;

	#endif
}

void setClientViewport(WindowSettings* ws, Rect vp) {
	vp = rectTrans(vp, ws->clientRectBL.min);
	setViewPort(vp);
}

void setClientScissor(WindowSettings* ws, Rect vp) {
	vp = rectTrans(vp, ws->clientRectBL.min);
	scissorTest(vp);
}

void setWindowViewport(WindowSettings* ws, Rect vp) {
	vp = rectTrans(vp, ws->windowRectBL.min);
	setViewPort(vp);
}


bool keyPressed(NewGui* gui, Input* input, int keycode) {
	if(gui->activeId != 0) return false;
	else return input->keysPressed[keycode];
}

bool keyDown(NewGui* gui, Input* input, int keycode) {
	if(gui->activeId != 0) return false;
	else return input->keysDown[keycode];
}

int mouseWheel(NewGui* gui, Input* input) {
	if(gui->hotId[Gui_Focus_MWheel] != 0 || gui->activeId != 0) return false;
	else return input->mouseWheel; 
}

int mouseButtonPressedLeft(NewGui* gui, Input* input) {
	if(gui->hotId[Gui_Focus_MLeft] != 0 || gui->activeId != 0) return false;
	else return input->mouseButtonPressed[0]; 
}

bool guiHotMouseClick(NewGui* gui) {
	return gui->hotId[Gui_Focus_MLeft] != 0;
}


Vec3 mouseRayCast(Rect tr, Vec2 mp, Camera* cam) {

	Vec2 mousePercent = {};
	mousePercent.x = mapRange01(mp.x, tr.left, tr.right);
	mousePercent.y = mapRange01(mp.y, tr.bottom, tr.top);

	OrientationVectors ovecs = cam->ovecs;
	Vec3 camBottomLeft = cam->pos + ovecs.dir*cam->nearDist + (-ovecs.right)*(cam->dim.w/2.0f) + (-ovecs.up)*(cam->dim.h/2.0f);

	Vec3 p = camBottomLeft;
	p += (cam->ovecs.right*cam->dim.w) * mousePercent.x;
	p += (cam->ovecs.up*cam->dim.h) * mousePercent.y;

	Vec3 rayDir = normVec3(p - cam->pos);

	return rayDir;
}

enum EntitySelectionMode {
	ENTITYUI_MODE_SELECTED = 0,
	ENTITYUI_MODE_TRANSLATION,
	ENTITYUI_MODE_ROTATION,
	ENTITYUI_MODE_SCALE,

	ENTITYUI_MODE_SIZE,
};

enum EntityUIState {
	ENTITYUI_INACTIVE = 0,
	ENTITYUI_HOT,
	ENTITYUI_ACTIVE,
};

enum EntityTranslateMode {
	TRANSLATE_MODE_AXIS = 0,
	TRANSLATE_MODE_PLANE,
	TRANSLATE_MODE_CENTER,
};

struct EntityUI {
	int selectedObject;

	int selectionMode;
	int selectionState;
	bool gotActive;

	bool guiHasFocus;

	float selectionAnimState;
	bool localMode;
	bool snappingEnabled;
	float snapGridSize;
	float snapGridDim;

	int axisIndex;
	Vec3 axis;
	Vec3 objectDistanceVector;

	// Translation mode.

	Vec3 startPos;
	int translateMode;
	Vec3 centerOffset;
	float centerDistanceToCam;
	Vec3 axes[3];

	// Rotation mode.

	Quat startRot;
	float currentRotationAngle;

	// Scale mode.

	float startDim;
};

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

	// App.

	NewGui gui;
	float panelHeight;

	Font* font;

	// 

	World world;
	EntityUI entityUI;
	RaytraceSettings settings;
	Vec3* buffer;
	Texture raycastTexture;
	Rect textureScreenRect;

	bool keepUpdating;
	bool fitToScreen;
	bool drawSceneWired;

	char screenShotName[50];

	bool activeProcessing;
	int threadCount;
	ProcessPixelsData threadData[RAYTRACE_THREAD_JOB_COUNT];
	bool waitingForThreadStop;
  
	f64 processStartTime;
	f64 processTime;

	int texFastMode;
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

		//
		// AppData.
		//

		getPMemory(sizeof(AppData));
		*ad = {};

		// int windowStyle = (WS_POPUP | WS_BORDER);
		// int windowStyle = (WS_POPUP);
		int windowStyle = WS_OVERLAPPEDWINDOW;
		initSystem(systemData, ws, windowsData, vec2i(1920*0.85f, 1080*0.85f), windowStyle, 1);

		windowHandle = systemData->windowHandle;

		printf("%Opengl Version: %s\n", (char*)glGetString(GL_VERSION));

		loadFunctions();

		const char* extensions = wglGetExtensionsStringEXT();

		if(true) {
			wglSwapIntervalEXT(1);
			ws->vsync = true;
		} else {
			wglSwapIntervalEXT(0);
			ws->vsync = false;
		}
		int fps = wglGetSwapIntervalEXT();

		initInput(&ad->input);
		systemData->input = &ad->input;

		systemData->minWindowDim = vec2i(200,200);
		systemData->maxWindowDim = ws->biggestMonitorSize;

		makeWindowTopmost(systemData);

		//
		// Setup Textures.
		//

		// for(int i = 0; i < TEXTURE_SIZE; i++) {
		// 	Texture tex;
		// 	uchar buffer [] = {255,255,255,255 ,255,255,255,255 ,255,255,255,255, 255,255,255,255};
		// 	loadTexture(&tex, buffer, 2,2, 1, INTERNAL_TEXTURE_FORMAT, GL_RGBA, GL_UNSIGNED_BYTE);

		// 	addTexture(tex);
		// }

		FolderSearchData fd;
		folderSearchStart(&fd, App_Image_Folder);
		while(folderSearchNextFile(&fd)) {

			if(strLen(fd.fileName) <= 2) continue; // Skip ..

			Texture tex;
			char* filePath = fillString("%s%s", App_Image_Folder, fd.fileName);
			loadTextureFromFile(&tex, filePath, -1, INTERNAL_TEXTURE_FORMAT, GL_RGBA, GL_UNSIGNED_BYTE);
			addTexture(tex);
		}

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

			attachToFrameBuffer(FRAMEBUFFER_2dMsaa, FRAMEBUFFER_SLOT_COLOR, GL_RGBA16F, 0, 0, ad->msaaSamples);
			// attachToFrameBuffer(FRAMEBUFFER_2dMsaa, FRAMEBUFFER_SLOT_COLOR, GL_SRGB8_ALPHA8, 0, 0, ad->msaaSamples);
			attachToFrameBuffer(FRAMEBUFFER_2dMsaa, FRAMEBUFFER_SLOT_DEPTH, GL_DEPTH_COMPONENT32F, 0, 0, ad->msaaSamples);
			attachToFrameBuffer(FRAMEBUFFER_2dNoMsaa, FRAMEBUFFER_SLOT_COLOR, GL_RGBA16F, 0, 0);
			// attachToFrameBuffer(FRAMEBUFFER_2dNoMsaa, FRAMEBUFFER_SLOT_COLOR, GL_SRGB8_ALPHA8, 0, 0);

			attachToFrameBuffer(FRAMEBUFFER_ScreenShot, FRAMEBUFFER_SLOT_COLOR, GL_SRGB8, 0, 0);

			ad->updateFrameBuffers = true;


			ad->frameBufferSize = ws->biggestMonitorSize;
			Vec2i fRes = ad->frameBufferSize;
			
			// Vec2i fRes = ws->currentRes;

			setDimForFrameBufferAttachmentsAndUpdate(FRAMEBUFFER_2dMsaa, fRes.w, fRes.h);
			setDimForFrameBufferAttachmentsAndUpdate(FRAMEBUFFER_2dNoMsaa, fRes.w, fRes.h);
		}

	//




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

		pcg32_srandom(0, __rdtsc());


		// @AppInit.

		ad->texFastMode = 2;
		
		ad->entityUI.selectionMode = ENTITYUI_MODE_TRANSLATION;
		ad->entityUI.localMode = false;
		ad->entityUI.snapGridSize = 1;
		ad->entityUI.snapGridDim = 100;


		strCpy(ad->screenShotName, "screenshot");
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
			ad->lastTimeStamp = timerInit();
			ad->dt = 1/(float)60;
		} else {
			ad->dt = timerUpdate(ad->lastTimeStamp, &ad->lastTimeStamp);
			ad->time += ad->dt;

			ad->fpsTime += ad->dt;
			ad->fpsCounter++;
			if(ad->fpsTime >= 1) {
				ad->avgFps = 1 / (ad->fpsTime / (f64)ad->fpsCounter);
				ad->fpsTime = 0;
				ad->fpsCounter = 0;
			}
		}
	}

	clearTMemory();

	//
	// Update input.
	//

	{
		updateWindowFrameData(systemData, ws);

		SwitchToFiber(systemData->messageFiber);

		if(ad->input.closeWindow) *isRunning = false;

		ad->frameCount++;

		if(systemData->mouseInClient) updateCursor(ws);

		ws->windowHasFocus = systemData->windowIsFocused;
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

		// Bug.
		input->keysPressed[KEYCODE_F11] = false;
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
		// gs->screenRes = ws->currentRes;
		gs->screenRes = ws->clientRes;

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

		scissorState(false);
	}

	// @AppLoop.

	NewGui* gui = &ad->gui;

	{
		newGuiBegin(gui, &ad->input, ws);
	}

	#ifdef ENABLE_CUSTOM_WINDOW_FRAME
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

		sd->normalTitleHeight = 22;
		sd->normalBorderSize = 5;
		sd->normalVisualBorderSize = 1;
		sd->buttonMargin = 3;
		sd->cornerGrabSize = 20;

		Vec4 cBorder = vec4(0,1);

		// Vec3 cTitleBarFocusedLeft     = vec3(0.03f,0.7f,0.25f);
		// Vec3 cTitleBarFocusedRight    = cTitleBarFocusedLeft  + vec3(0.03f,-0.1f,0.1f);
		// Vec3 cTitleBarNotFocusedLeft  = cTitleBarFocusedLeft  + vec3(0, -1, -0.1f);
		// Vec3 cTitleBarNotFocusedRight = cTitleBarFocusedRight + vec3(0, -1, -0.1f);

		Vec3 cTitleBarFocusedLeft     = vec3(0.00f,0.0f,0.15f);
		Vec3 cTitleBarFocusedRight    = cTitleBarFocusedLeft  + vec3(0.00f,0.0f,0.1f);
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

		float cButtons = 0.6f;
		Vec4 cButton0 = vec4(cButtons,1);
		Vec4 cButton1 = vec4(cButtons,1);
		Vec4 cButton2 = vec4(cButtons,1);

		Vec4 cButtonOutline = cBorder;

		Vec4 cText = vec4(1.0f,1);
		if(!ws->windowHasFocus) cText = vec4(0.5f,1);

		Vec4 cTextShadow = vec4(0,1);

		float fontHeight = sd->normalTitleHeight*0.7f;
		float textPadding = fontHeight*0.3f;

		glDepthMask(false);

		drawRect(ws->windowRect, cBorder);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glBlendEquation(GL_FUNC_ADD);

		drawRectNewColoredW(ws->titleRect, cTitleBarLeft, cTitleBarRight);

		{
			glLineWidth(1);
			Vec2 p0 = ws->titleRect.min;
			Vec2 p1 = rectBR(ws->titleRect);
			drawLine(vec2(roundFloat(p0.x),roundFloat(p0.y)+0.5f), vec2(roundFloat(p1.x),roundFloat(p1.y)+0.5f), cBorder);
			glLineWidth(1);
		}

		if(pointInRectEx(input->mousePosWindow, sd->rMinimize)) cButton0.rgb += vec3(0.4f);
		if(pointInRectEx(input->mousePosWindow, sd->rMaximize)) cButton1.rgb += vec3(0.4f);
		if(pointInRectEx(input->mousePosWindow, sd->rClose))    cButton2.rgb += vec3(0.4f);

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

		Font* font = getFont("LiberationSans-Regular.ttf", fontHeight, "LiberationSans-Bold.ttf", "LiberationSans-Italic.ttf");
		TextSettings ts = textSettings(font, cText, TEXTSHADOW_MODE_SHADOW, vec2(-1,-1), 1, cTextShadow);

		drawText("<b>PathTracer<b>", tp, vec2i(-1,0), ts);

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
		// @Settings.

		if(init) {
			ad->settings.texDim = vec2i(240*pow(2, ad->texFastMode), 135*pow(2, ad->texFastMode));

			ad->settings.sampleMode = SAMPLE_MODE_BLUE_MULTI;
			ad->settings.sampleCountGrid = 4;
			ad->settings.sampleGridWidth = 10;

			ad->settings.rayBouncesMax = 6;

			ad->threadCount = RAYTRACE_THREAD_JOB_COUNT;
			// ad->threadCount = 1;

			ad->keepUpdating = false;

			ad->drawSceneWired = true;
			ad->fitToScreen = true;

			ad->settings.randomDirectionCount = 20000;

			Camera* cam = &ad->world.camera;
			cam->pos = vec3(0, -50, 10);
			cam->rot = vec3(0, 0, 0);
			cam->fov = 90;
			cam->dim.w = 10;
			cam->farDist = 10000;

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

		// Mouse capture.
		{
			if(!ad->captureMouse) {
				if(input->keysPressed[KEYCODE_F3] || input->mouseButtonPressed[1]) {
					ad->captureMouse = true;

					if(input->keysPressed[KEYCODE_F3]) ad->captureMouseKeepCenter = true;
					else ad->captureMouseKeepCenter = false;

					POINT point;
					GetCursorPos(&ws->lastMousePosition);
				}
			} else {
				if(input->keysPressed[KEYCODE_F3] || input->mouseButtonReleased[1]) {
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
				float speed = 0.0015f;
				
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

				// initTexture(texture, -1, INTERNAL_TEXTURE_FORMAT, texDim, 3, GL_NEAREST, GL_CLAMP);
				initTexture(texture, -1, INTERNAL_TEXTURE_FORMAT, texDim, 3, GL_LINEAR, GL_CLAMP);

				Texture* t = &ad->raycastTexture;
				Vec3 black = vec3(0.2f);
				glClearTexSubImage(t->id, 0, 0,0,0, t->dim.w,t->dim.h, 1, GL_RGB, GL_FLOAT, &ad->world.defaultEmitColor);
			}
		}

		{
			World* world = &ad->world;

			#if 0

			if(init || keyPressed(gui, input, KEYCODE_T) {

				world->shapeCount = 0;
				if(world->shapes) free(world->shapes);
				world->shapes = mallocArray(Shape, 1000);

				int count = 5;
				float r = 30;
				Vec3 offset = vec3(0,0,r/2.0f);
				for(int i = 0; i < count; i++) {
					// int type = randomIntPCG(0, SHAPE_COUNT-1);
					int type = SHAPE_SPHERE;

					Vec3 pos = vec3(randomFloatPCG(-r,r,0.01f), randomFloatPCG(-r,r,0.01f), randomFloatPCG(-r/2.0f,r/2.0f,0.01f));
					pos += offset;
					float size = randomFloatPCG(15,30,0.01f);

					// bool emitter = randomIntPCG(0,1);
					bool emitter = 0;
					Vec3 c = hslToRgbFloat(vec3(randomFloatPCG(0,1,0.01f), randomFloatPCG(0.5f,1,0.01f), randomFloatPCG(0.25f,0.75f,0.01f)));

					float rm = randomFloatPCG(0,1,0.01f);

					Shape s = {};

					s.type = type;
					s.pos = pos;
					if(type == SHAPE_SPHERE) s.r = size/2.0f;
					else if(type == SHAPE_BOX) s.dim = vec3(size/2.0f);
					s.color = c;
					if(emitter) s.emitColor = c;
					else s.color = c;
					s.reflectionMod = rm;

					world->shapes[world->shapeCount++] = s;
				}

				Shape s = {};

				// Plane

				s = {};
				s.type = SHAPE_BOX;
				s.pos = vec3(0,0,0);
				s.dim = vec3(1000000,1000000,0.0001f);
				s.color = vec3(0.5f);
				// s.color = vec3(0.99f);
				// s.reflectionMod = 0.3f;
				s.reflectionMod = 0.5f;
				// s.reflectionMod = 0.9f;
				// s.reflectionMod = 0.1f;
				world->shapes[world->shapeCount++] = s;

				// Mirror Sphere

				s = {};
				s.type = SHAPE_SPHERE;
				s.pos = vec3(0,0,20);
				s.r = 10;
				s.color = vec3(0.5f);
				// s.color = vec3(0.99f);
				// s.reflectionMod = 0.3f;
				s.reflectionMod = 1.0f;
				// s.reflectionMod = 0.9f;
				// s.reflectionMod = 0.1f;
				world->shapes[world->shapeCount++] = s;





				// world->defaultEmitColor = vec3(0.7f, 0.8f, 0.9f);
				world->defaultEmitColor = vec3(1.0f);
				world->globalLightDir = normVec3(vec3(-1.5f,-1,-2.0f));
				world->globalLightColor = vec3(1,1,1);
				// world->globalLightColor = vec3(0.0f);

				// Calc bounding spheres.
				for(int i = 0; i < world->shapeCount; i++) {
					Shape* s = world->shapes + i;
					shapeBoundingSphere(s);
				}
			}

			#endif

			// if(init || reload) {
			if(init || keyPressed(gui, input, KEYCODE_R)) {
				world->defaultEmitColor = vec3(1,1,1);

				world->ambientRatio = 0.2f;
				world->ambientColor = vec3(1.0f);

				// world->defaultEmitColor = vec3(0.7f, 0.8f, 0.9f);
				// world->defaultEmitColor = vec3(1.0f);
				world->globalLightDir = normVec3(vec3(-1.5f,-1,-2.0f));
				world->globalLightColor = vec3(1,1,1);
				// world->globalLightColor = vec3(0.0f);



				world->lightCount = 0;
				reallocArraySave(Light, world->lights, 10);

				Light l;

				l = {};
				l.type = LIGHT_TYPE_DIRECTION;
				l.pos = normVec3(vec3(-1.5f,-1,-2.0f));
				// l.diffuseColor = vec3(0.9,0,0);
				l.diffuseColor = vec3(0.9f);
				l.specularColor = vec3(1.0f);
				l.brightness = 1.0f;
				world->lights[world->lightCount++] = l;

				world->objectCount = 0;
				reallocArraySave(Object, world->objects, 100);

				Material materials[10] = {};
				materials[0].diffuseRatio = 0.5f;
				materials[0].specularRatio = 0.5f;
				materials[0].shininess = 15;

				Object obj;

				// Ground plane.

				// obj = {};
				// obj.pos = vec3(0,0,0);
				// obj.rot = quat();
				// obj.color = vec3(0.5f);
				// obj.material = materials[0];
				// obj.geometry.type = GEOM_TYPE_BOX;
				// // obj.geometry.dim = vec3(100000, 100000, 0.01f);
				// obj.geometry.dim = vec3(5, 5, 0.01f);
				// world->objects[world->objectCount++] = obj;

				// Sphere.

				obj = {};
				obj.pos = vec3(0,0,20);
				obj.rot = quat();
				obj.color = vec3(0.3f,0.5f,0.8f);
				obj.material = materials[0];
				obj.geometry.type = GEOM_TYPE_SPHERE;
				obj.geometry.r = 5;
				world->objects[world->objectCount++] = obj;

				obj = {};
				obj.pos = vec3(10,0,15);
				obj.rot = quat();
				obj.color = vec3(0.6f,0.5f,0.8f);
				obj.material = materials[0];
				obj.geometry.type = GEOM_TYPE_SPHERE;
				obj.geometry.r = 5;
				world->objects[world->objectCount++] = obj;

				obj = {};
				obj.pos = vec3(0,5,5);
				obj.rot = quat();
				obj.color = vec3(0.3f,0.5f,0.2f);
				obj.material = materials[0];
				obj.geometry.type = GEOM_TYPE_SPHERE;
				obj.geometry.r = 5;
				world->objects[world->objectCount++] = obj;
				
				// Box.
				
				obj = {};
				obj.pos = vec3(-5,-10,10);
				obj.rot = quat();
				obj.color = vec3(0.87f,0.1f,0.3f);
				obj.material = materials[0];
				obj.geometry.type = GEOM_TYPE_BOX;
				// obj.geometry.dim = vec3(5,6,7);
				obj.geometry.dim = vec3(5,10,10);
				world->objects[world->objectCount++] = obj;

				// Calc bounding spheres.
				for(int i = 0; i < world->objectCount; i++) {
					geometryBoundingSphere(&world->objects[i].geometry);
				}
			}

		}

		if(keyPressed(gui, input, KEYCODE_SPACE) && !ad->drawSceneWired) {
			for(int i = 0; i < ad->threadCount; i++) ad->threadData[i].stopProcessing = true;
			ad->waitingForThreadStop = true;
		}

		// if((input->keysPressed[KEYCODE_SPACE] || reload || init || ad->keepUpdating) && (ad->activeProcessing == false)) {
		// if((input->keysPressed[KEYCODE_SPACE] || ad->keepUpdating || reload) && (ad->activeProcessing == false)) {
		if((keyPressed(gui, input, KEYCODE_SPACE) || ad->keepUpdating) && (!ad->activeProcessing && ad->drawSceneWired)) {
		// if(false) {
			ad->activeProcessing = true;
			ad->drawSceneWired = false;

			RaytraceSettings* settings = &ad->settings;

			settings->mode = RENDERING_MODE_RAY_TRACER; // MODE_PATH_TRACER

			Texture* t = &ad->raycastTexture;
			// Vec3 black = vec3(0.2f);
			if(!ad->keepUpdating) glClearTexSubImage(t->id, 0, 0,0,0, t->dim.w,t->dim.h, 1, GL_RGB, GL_FLOAT, &ad->world.defaultEmitColor);

			int pixelCount = settings->texDim.w*settings->texDim.h;
			if(ad->buffer != 0) free(ad->buffer);
			ad->buffer = mallocArray(Vec3, pixelCount);

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

		bool doneProcessing = false;
		if(ad->activeProcessing && threadQueueFinished(threadQueue)) {
			ad->activeProcessing = false;
			doneProcessing = true;

			ad->processTime = ad->time - ad->processStartTime;

			if(printRaytraceTimings) {
				for(int i = 0; i < arrayCount(processPixelsThreadedTimings); i++) {
					TimeStamp* t = processPixelsThreadedTimings + i;

					calcTimeStamp(t);
					printf("%i: hits: %6i, cycles: %6i\n", i, t->hits, t->cyclesOverHits);
					*t = {};
				}
			}
		}

		if((!ad->keepUpdating && ad->activeProcessing) || doneProcessing || (ad->keepUpdating && !ad->activeProcessing)) {
			glTextureSubImage2D(ad->raycastTexture.id, 0, 0, 0, ad->settings.texDim.w, ad->settings.texDim.h, GL_RGB, GL_FLOAT, ad->buffer);
		}

		// if(input->keysPressed[KEYCODE_1]) ad->texFastMode = 0;
		// if(input->keysPressed[KEYCODE_2]) ad->texFastMode = 1;
		// if(input->keysPressed[KEYCODE_3]) ad->texFastMode = 2;
		// if(input->keysPressed[KEYCODE_4]) ad->texFastMode = 3;
		// if(input->keysPressed[KEYCODE_5]) ad->texFastMode = 4;

		{

			Rect sr = getScreenRect(ws);
			Rect tr = sr;
			Vec2 sd = rectDim(sr);
			Vec2 texDim = vec2(ad->raycastTexture.dim);

			if(ad->fitToScreen) {
				if(((float)texDim.h / texDim.w) > (sd.h / sd.w)) {
					tr = rectSetW(tr, ((float)texDim.w / texDim.h)*sd.h);
				} else {
					tr = rectSetH(tr, ((float)texDim.h / texDim.w)*sd.w);
				}
			} else {
				Vec2 c = rectCen(sr);
				Vec2i td = ad->raycastTexture.dim;
				c.x -= td.w/2.0f;
				c.y += td.h/2.0f;
				c.x = roundFloat(c.x);
				c.y = roundFloat(c.y);

				tr = rectTLDim(c, texDim);

				// tr = rectCenDim(c, texDim);
			}

			if(!ad->drawSceneWired) {
				glDepthMask(false);
				drawRect(tr, rect(0,0,1,1), ad->raycastTexture.id);
				glDepthMask(true);
			}

			ad->textureScreenRect = tr;
		}

		if(ad->waitingForThreadStop && threadQueueFinished(threadQueue)) {
			ad->waitingForThreadStop = false;

			Texture* t = &ad->raycastTexture;
			Vec3 black = vec3(0.2f);
			if(t->isCreated) {
				glClearTexSubImage(t->id, 0, 0,0,0, t->dim.w,t->dim.h, 1, GL_RGB, GL_FLOAT, &ad->world.defaultEmitColor);
			}
			if(ad->buffer) zeroMemory(ad->buffer, ad->raycastTexture.dim.w*ad->raycastTexture.dim.h*sizeof(Vec3));


			ad->drawSceneWired = true;
		}

		if(keyPressed(gui, input, KEYCODE_F)) ad->fitToScreen = !ad->fitToScreen;
	}


	{
		// @EntityUI code

		EntityUI* eui = &ad->entityUI;

		eui->guiHasFocus = guiHotMouseClick(gui);

		if(mouseWheel(gui, input) && eui->selectionState != ENTITYUI_ACTIVE) {
			ad->entityUI.selectionMode = mod(ad->entityUI.selectionMode+ mouseWheel(gui, input), ENTITYUI_MODE_SIZE);
		}

		if(input->mouseButtonPressed[2] && eui->selectionState != ENTITYUI_ACTIVE) {
			eui->localMode = !eui->localMode;
		}

		if(input->keysDown[KEYCODE_SHIFT]) {
			eui->snappingEnabled = true;
		} else {
			eui->snappingEnabled = false;
		}

		// Selection.

		if(mouseButtonPressedLeft(gui, input) && eui->selectionState == ENTITYUI_INACTIVE) {
			Vec3 rayDir = mouseRayCast(ad->textureScreenRect, input->mousePosNegative, &ad->world.camera);

			int objectIndex = castRay(ad->world.camera.pos, rayDir, ad->world.objects, ad->world.objectCount);
			if(objectIndex != -1) {
				eui->selectedObject = objectIndex+1;
				eui->selectionAnimState = 0;
			} else eui->selectedObject = 0;
		}

		if(input->mouseButtonReleased[0] && eui->selectionState == ENTITYUI_ACTIVE) {
			eui->selectionState = ENTITYUI_INACTIVE;
		}


		eui->gotActive = false;

		if(mouseButtonPressedLeft(gui, input) && eui->selectionState == ENTITYUI_HOT) {
			eui->selectionState = ENTITYUI_ACTIVE;
			eui->gotActive = true;
		}


		if(eui->selectionState == ENTITYUI_ACTIVE) {

			Object* obj = ad->world.objects + (eui->selectedObject-1);
			Camera* cam = &ad->world.camera;
			Vec3 rayPos = cam->pos;
			Vec3 rayDir = mouseRayCast(ad->textureScreenRect, input->mousePosNegative, &ad->world.camera);

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

					Geometry* geom = &obj->geometry;

					if(eui->gotActive) {
						eui->objectDistanceVector = obj->pos - linePointOnAxis;

						eui->startDim = geometryGetDim(geom).e[eui->axisIndex-1];
					}

					float oldLength = lenVec3(eui->objectDistanceVector);
					float newLength = lenVec3(obj->pos - linePointOnAxis);

					float currentAxisLength = eui->startDim + (newLength - oldLength)*2;

					currentAxisLength = clampMin(currentAxisLength, 0);
					if(dot(eui->objectDistanceVector, obj->pos - linePointOnAxis) < 0) currentAxisLength = 0;

					if(eui->snappingEnabled) {
						currentAxisLength = roundMod(currentAxisLength, eui->snapGridSize);
					}

					geometrySetDim(geom, currentAxisLength, eui->axisIndex-1);
					geometryBoundingSphere(&obj->geometry);
				}
			}

		}
	}

	if(ad->drawSceneWired)
	{
		World* world = &ad->world;
		Camera* cam = &ad->world.camera;
		Vec2 d = cam->dim;

		Rect tr = ad->textureScreenRect;

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

		Vec4 lp = vec4(-world->lights[0].dir, 0);
		glLightfv(GL_LIGHT0, GL_POSITION, lp.e);

		// Vec4 lightAmbientColor = vec4(world->defaultEmitColor,1);
		// glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbientColor.e);

		// Vec4 lightDiffuseColor = vec4(world->globalLightColor,1);
		Vec4 lightDiffuseColor = vec4(world->lights[0].diffuseColor,1);
		glLightfv(GL_LIGHT0, GL_AMBIENT, lightDiffuseColor.e);

		// Vec4 lightDiffuseColor = vec4(world->globalLightColor,1);
		// glLightfv(GL_LIGHT0, GL_AMBIENT, lightDiffuseColor.e);



		glEnable(GL_DEPTH_TEST);
		glBindTexture(GL_TEXTURE_2D, 0);

		{
			// @Bottom grid
			EntityUI* eui = &ad->entityUI;

			glDisable(GL_LIGHTING);
			float l = eui->snapGridDim/2;
			float arrowSize = 0.5f;
			drawArrow(vec3(0,0,0) + vec3(-1, 0,0)*arrowSize/2, vec3(-1, 0,0) * l, vec3(0,0,1), arrowSize, vec4(1,0,0,1));
			drawArrow(vec3(0,0,0) + vec3( 1, 0,0)*arrowSize/2, vec3( 1, 0,0) * l, vec3(0,0,1), arrowSize, vec4(1,0.5f,0.5f,1));
			drawArrow(vec3(0,0,0) + vec3( 0,-1,0)*arrowSize/2, vec3( 0,-1,0) * l, vec3(0,0,1), arrowSize, vec4(0,1,0,1));
			drawArrow(vec3(0,0,0) + vec3( 0, 1,0)*arrowSize/2, vec3( 0, 1,0) * l, vec3(0,0,1), arrowSize, vec4(0.5f,1,0.5f,1));
			glEnable(GL_LIGHTING);

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
				if(g->type == GEOM_TYPE_BOX) scale = g->dim;
				if(g->type == GEOM_TYPE_SPHERE) scale = vec3(g->r);

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
						for(int i = 0; i < 3; i++) color.e[i] = lerp(l, faceColors[index].e[i], faceColors[index+1].e[i]);
					}

					c = COLOR_SRGB(color);
					glColor4f(c.r, c.g, c.b, c.a);

					glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
					glDisable(GL_LIGHTING);

					glLineWidth(1);

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
					float uiAlpha = 0.5f;

					Vec4 rotationSegmentColor = vec4(hslToRgbFloat(vec3(0.5f,0.9f,0.5f)), 0.5f);
					Vec4 translationVectorColor = rotationSegmentColor;
					translationVectorColor.a = 1;

					float translateLineWidth = 5;

					float translationPlaneSize = boundRadius * 0.3f;
					float rotationRingThickness = boundRadius * 0.05;
					float translationArrowLength = boundRadius;
					float translationArrowThickness = rotationRingThickness;
					float translationVectorThickness = translationArrowThickness*0.5f;

					float translationCenterBoxSize = boundRadius * 0.10f;
					Vec4 translationCenterBoxColor = vec4(0.5f,1);
					float translationCenterBoxColorMod = 0.1f;

					// float scaleArrowBoxDim = boundRadius * 0.1;
					float scaleArrowBoxDim = 1;

					if(eui->selectionMode == ENTITYUI_MODE_TRANSLATION) {
						Vec2 d = vec2(translationArrowThickness,translationArrowLength);
						Vec3 n = cam->pos - obj->pos;
						Vec3 c[] = { vec3(1,0,0), vec3(0,1,0), vec3(0,0,1) };
						Vec3 colorSquares = vec3(0.7f);

						Vec3 axis[3];
						for(int i = 0; i < 3; i++) {
							Vec3 a = vec3(0,0,0);
							a.e[i] = 1;

							if(eui->localMode) a = obj->rot * a;

							if(dot(a, n) < 0) a *= -1;
							axis[i] = a;
						}

						Vec3 rayDir = mouseRayCast(ad->textureScreenRect, input->mousePosNegative, cam);

						glDisable(GL_DEPTH_TEST);


						// Move axis.

						int axisIndex = 0;
						for(int i = 0; i < 3; i++) {
							float a = uiAlpha;
							if(eui->selectionState != ENTITYUI_ACTIVE) {
								Vec3 pp = obj->pos + axis[i]*d.h*0.5f;
								// Getting right angle with two cross products.
								Vec3 pn = normVec3(cross( cross(axis[i], n), axis[i])); 

								Vec3 intersection;
								float dist = linePlaneIntersection(cam->pos, rayDir, pp, pn, axis[i], d, &intersection);
								if(dist != -1) {
									axisIndex = i+1;

									if(!eui->guiHasFocus) a = 1;
								}
							} else {
								if(eui->translateMode == TRANSLATE_MODE_AXIS)
									if(i == eui->axisIndex-1) a = 1;
							}

							drawArrow(obj->pos, obj->pos + axis[i]*d.h, n, d.w, vec4(c[i],a));
						}

						// Move plane.

						int planeIndex = 0;
						for(int i = 0; i < 3; i++) {
							// Whatever.
							float dim = translationPlaneSize;
							Vec3 edgePoint = obj->pos + axis[(i+1)%3]*d.h + axis[(i+2)%3]*d.h;
							Vec3 diag = normVec3(edgePoint - obj->pos);

							float squareDiag = sqrt(2 * (dim*dim));
							Vec3 p = edgePoint - diag*(squareDiag/2);

							float a = uiAlpha;
							if(eui->selectionState != ENTITYUI_ACTIVE) {
							
								Vec3 intersection;
								float dist = linePlaneIntersection(cam->pos, rayDir, p, axis[i], axis[(i+1)%3], vec2(dim), &intersection);
								if(dist != -1) {
									planeIndex = i+1;

									if(!eui->guiHasFocus) a = 1;
								}
							} else {
								if(eui->translateMode == TRANSLATE_MODE_PLANE)
									if(axis[i] == eui->axis) a = 1;
							}

							drawPlane(p, axis[i], axis[(i+1)%3], vec2(dim), vec4(colorSquares,a), rect(0,0,1,1), getTexture(TEXTURE_ROUNDED_SQUARE)->id);
						}

						// Move center.

						int centerIndex = 0;
						{
							Vec4 color = translationCenterBoxColor;

							if(eui->selectionState != ENTITYUI_ACTIVE) {
							
								Vec3 intersection;
								bool result = boxRaycastRotated(cam->pos, rayDir, obj->pos, vec3(translationCenterBoxSize), obj->rot, &intersection);
								if(result) {
									eui->centerOffset = intersection - obj->pos;
									centerIndex = 1;

									if(!eui->guiHasFocus)
										color.rgb += vec3(translationCenterBoxColorMod);
								}
							} else {
								if(eui->translateMode == TRANSLATE_MODE_CENTER)
									color.rgb += vec3(translationCenterBoxColorMod);
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
					}

					if(eui->selectionMode == ENTITYUI_MODE_ROTATION) {

						float thickness = rotationRingThickness;
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

						Vec3 rayDir = mouseRayCast(ad->textureScreenRect, input->mousePosNegative, cam);

						// Find closest ring to camera.

						float closestDistance = FLT_MAX;
						int axisIndex = 0;
						if(eui->selectionState != ENTITYUI_ACTIVE) {
							for(int i = 0; i < 3; i++) {
								Vec3 intersection;
								float dist = linePlaneIntersection(cam->pos, rayDir, obj->pos, axis[i], &intersection);
								if(dist != -1) {
									float distToObj = lenVec3(intersection - obj->pos);
									if(valueBetween(distToObj, boundRadius-thickness, boundRadius)) {

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

						for(int i = 0; i < 3; i++) {
							float a = uiAlpha;
							drawRing(obj->pos, axis[i], boundRadius, thickness, vec4(c[i],a));
						}

						// Paint over hot/active ring.
						if(axisIndex && (!eui->guiHasFocus || eui->selectionState == ENTITYUI_ACTIVE)) {
							int i = axisIndex-1;
							drawRing(obj->pos, axis[i], boundRadius, thickness, vec4(c[i],1));

							glDisable(GL_DEPTH_TEST);
							if(eui->selectionState == ENTITYUI_ACTIVE) {
								Vec3 left = obj->pos + normVec3(eui->objectDistanceVector)*boundRadius;
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

							float arrowLength = geometryGetDim(geom).e[i]/2 + scaleArrowBoxDim;

							Vec3 axis = vec3(0,0,0);
							axis.e[i] = 1;
							axis = obj->rot*axis;
							if(dot(axis, cam->pos - obj->pos) < 0) axis *= -1;
							Vec3 p = obj->pos + axis * arrowLength;

							if(eui->selectionState != ENTITYUI_ACTIVE) {
								Vec3 rayDir = mouseRayCast(ad->textureScreenRect, input->mousePosNegative, cam);
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
							glLineWidth(5);
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

		NewGui* gui = &ad->gui;

		if(ad->entityUI.selectionState == ENTITYUI_ACTIVE) gui->hotId[Gui_Focus_MLeft] = 0;

		{
			Font* font = getFont("LiberationSans-Regular.ttf", -15, "LiberationSans-Bold.ttf", "LiberationSans-Italic.ttf");
			
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
			gui->sliderSettings = sliderSettings(etbs, 15, 15, 0, 0, 4, cButton, vec4(0,0,0,0));

			// ScrollRegionSettings scrollSettings;
			gui->popupSettings = boxSettings(cEdit, 0, cOutline);
			gui->comboBoxSettings = textBoxSettings(gui->textSettings, boxSettings(cEdit, 0, cOutline), textPadding);

			BoxSettings cbs = boxSettings(cEdit, panelRounding, cOutline);
			gui->checkBoxSettings = checkBoxSettings(cbs, cButton, 0.5f);
		}

		// Left panel.
		#if 1
		{
			float panelOffset = 0;
			float panelMargin = 5;

			Vec2 panelDim = vec2(200,ad->panelHeight);
			Rect pr = rectTLDim(vec2(panelOffset,-panelOffset), panelDim);

			newGuiSetHotAllMouseOver(gui, pr, gui->zLevel);
			newGuiQuickBox(gui, pr);


			if(init) newGuiScissorPush(gui, rect(0,0,0,0));
			{
				Rect pri = rectExpand(pr, -vec2(panelMargin*2));

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
					newGuiQuickTextBox(gui, r, "<b>App Settings<b>", vec2i(0,0), &headerSettings);

					s = "KeepUpdating";
					r = rectTLDim(p, vec2(ew, eh)); p.y -= eh+pad.y;
					qr = quickRow(r, pad.x, getTextDim(s, font).w, 0, eh); 
					newGuiQuickText(gui, quickRowNext(&qr), s, vec2i(-1,0)); 
					quickRowNext(&qr);
					newGuiQuickCheckBox(gui, quickRowNext(&qr), &ad->keepUpdating);

					s = "FitToScreen";
					r = rectTLDim(p, vec2(ew, eh)); p.y -= eh+pad.y;
					qr = quickRow(r, pad.x, getTextDim(s, font).w, 0, eh);
					newGuiQuickText(gui, quickRowNext(&qr), s, vec2i(-1,0)); 
					quickRowNext(&qr);
					newGuiQuickCheckBox(gui, quickRowNext(&qr), &ad->fitToScreen);

					s = "Cam Fov";
					r = rectTLDim(p, vec2(ew, eh)); p.y -= eh+pad.y;
					qr = quickRow(r, pad.x, getTextDim(s, font).w, 0);
					newGuiQuickText(gui, quickRowNext(&qr), s, vec2i(-1,0));
					newGuiQuickSlider(gui, quickRowNext(&qr), &world->camera.fov, 20, 150);


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
					qr = quickRow(r, pad.x, 0.6f, 0);
					newGuiQuickTextEdit(gui, quickRowNext(&qr), ad->screenShotName, arrayCount(ad->screenShotName)-1);

					if(newGuiQuickButton(gui, quickRowNext(&qr), "Screenshot")) {
						if(!ad->activeProcessing && ad->buffer) {
							Vec2i texDim = ad->settings.texDim;
							int size = texDim.w * texDim.h;
							char* intBuffer = mallocArray(char, size*3);

							Vec3* floatBuffer = ad->buffer;
							for(int i = 0; i < size; i++) {
								intBuffer[i*3 + 0] = colorFloatToInt(floatBuffer[i].r);
								intBuffer[i*3 + 1] = colorFloatToInt(floatBuffer[i].g);
								intBuffer[i*3 + 2] = colorFloatToInt(floatBuffer[i].b);
							}

							stbi_write_png(fillString("%s%s%s", "Screenshots\\", ad->screenShotName, ".png"), texDim.w, texDim.h, 3, intBuffer, 0);

							free(intBuffer);
						}
					}

				}

				ad->panelHeight = pr.top - p.y - pad.y + panelMargin;
			}
		}
		#endif

		newGuiEnd(gui);
	}
	#endif

	#if 0
	{
		// Gamma test.

		Rect cr = ws->clientRect;
		Vec2 p = vec2(0,0);
		float h = rectH(cr)/7;
		Rect r;

		r = rectTLDim(p, vec2(rectW(cr), h)); p.y -= h;
		drawRect(r, vec4(1,1), rect(0,1,1,0), getTexture(TEXTURE_GRADIENT)->id);
		r = rectTLDim(p, vec2(rectW(cr), h)); p.y -= h;
		drawRectNewColoredW(r , vec4(0,1), vec4(1,1));
		r = rectTLDim(p, vec2(rectW(cr), h)); p.y -= h;
		drawRectNewColoredW(r , vec4(1,0,0,1), vec4(0,1,0,1));

		r = rectTLDim(p, vec2(rectW(cr), h)); p.y -= h;
		drawRectNewColoredW(r, vec4(1,0,0,1), vec4(1,0,0,1));
		r = rectTLDim(p, vec2(rectW(cr), h)); p.y -= h;
		drawRectNewColoredW(r, vec4(1,1,1,1), vec4(1,1,1,1));
		drawRectNewColoredW(r, vec4(1,0,0,0.5f), vec4(1,0,0,0.5f));

		r = rectTLDim(p, vec2(rectW(cr), h)); p.y -= h;
		drawRectNewColoredW(r, vec4(1,1), vec4(1,1));
		float a = 0.75f;
		drawRectNewColoredW(r, vec4(0,a), vec4(0,a));

		r = rectTLDim(p, vec2(rectW(cr), h)); p.y -= h;
		drawRectNewColoredW(r, vec4(1,1), vec4(1,1));
		drawRect(r, vec4(1,1), rect(0,1,1,0), getTexture(TEXTURE_ALPHA_GRADIENT)->id);
	}
	#endif 

	openglDrawFrameBufferAndSwap(ws, systemData, &ad->swapTime, init);

	if(*isRunning == false) saveAppSettings(systemData);
}
