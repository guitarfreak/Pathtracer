/*
=================================================================================

	ToDo:
	* Better sampling pattern.
	* Replace rand().
	* Better angle calculation.
	* Fov.
	* Random placement.
	* Better pre vis.
	* Split hit tests and reflection calculations.
	* Blue noise.
	* Aliasing.
	* Wrapping blue noise.
	- More Shapes and rotations.
	- Simd.
	- Ui.
	- Entity editor.
	- Put input updating in own thread.
	- Float not precise enough at hundreds of samples per pixel.

	Done Today: 

	Bugs:
	- Black pixels.
	- One Thread job gets stuck till the end? (Noticible by unprocessed black pixel bar.)

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

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "external\stb_image_write.h"

#define STB_RECT_PACK_IMPLEMENTATION
#include "external\stb_rect_pack.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "external\stb_truetype.h"

#include <iacaMarks.h>


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

#include "Raycast.cpp"


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



#define THREAD_JOB_COUNT 8*4

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

	bool captureMouse;
	bool fpsMode;

	// App.

	Font* font;

	// 

	World world;
	RaytraceSettings settings;
	Vec3* buffer;
	Texture raycastTexture;
	Rect textureScreenRect;

	bool activeProcessing;
	bool keepUpdating;
	bool drawSceneWired;

	int threadCount;
	ProcessPixelsData threadData[THREAD_JOB_COUNT];
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

		pcg32_srandom(0, __rdtsc());
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
		// ad->settings.texDim = vec2i(1920*2,1080*2);
		// ad->settings.texDim = vec2i(1920,1080);
		// ad->settings.texDim = vec2i(1280,720);
		// ad->settings.texDim = vec2i(1280/2,720/2);
		// ad->settings.texDim = vec2i(320,180);
		// ad->settings.texDim = vec2i(160,90);
		// ad->settings.texDim = vec2i(8,8);
		// ad->settings.texDim = vec2i(2,2);

		ad->settings.texDim = vec2i(240*pow(2, ad->texFastMode), 135*pow(2, ad->texFastMode));

		// ad->settings.sampleMode = SAMPLE_MODE_MSAA8X;
		// ad->settings.sampleMode = SAMPLE_MODE_GRID;
		ad->settings.sampleMode = SAMPLE_MODE_BLUE;
		ad->settings.sampleCountGrid = 10*1.3f;
		// ad->settings.sampleCountGrid = 10;
		ad->settings.rayBouncesMax = 6;

		ad->keepUpdating = false;

		ad->threadCount = THREAD_JOB_COUNT;
		// ad->threadCount = 1;

		// glClearColor(1,0,0,1);
		// glClear(GL_COLOR_BUFFER_BIT);

		if(init) {
			ad->drawSceneWired = true;

			{
				int count = 20000;

				ad->settings.randomDirectionCount = count;
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

		// if(init || reload) {
		if(init) {
			Camera* cam = &ad->world.camera;
			cam->pos = vec3(0, -50, 10);
			cam->rot = vec3(0, 0, 0);
			cam->fov = 100;

			float aspectRatio = (float)ad->settings.texDim.w / ad->settings.texDim.h;
			cam->dim.w = 10;
			cam->dim.h = cam->dim.w*(1/aspectRatio);
			cam->nearDist = camDistanceFromFOVandWidth(cam->fov, cam->dim.w);
			cam->farDist = 10000;
		}

		if(input->keysPressed[KEYCODE_F3]) {
			ad->captureMouse = !ad->captureMouse;
		}

		{
			ad->fpsMode = ad->captureMouse && windowHasFocus(windowHandle);
			if(ad->fpsMode) captureMouse(windowHandle, ad->captureMouse, input);

			Camera* cam = &ad->world.camera;

			if((!ad->fpsMode && input->mouseButtonDown[0]) || ad->fpsMode) {
				float speed = 0.1f;
				
				cam->rot.x += -input->mouseDelta.x*speed*ad->dt;
				cam->rot.y += input->mouseDelta.y*speed*ad->dt;
				clamp(&cam->rot.y, -M_PI_2 + 0.001f, M_PI_2 - 0.001f);
			}

			OrientationVectors o = getVectorsFromRotation(cam->rot);
			cam->ovecs = o;

			float speed = 40.0f*ad->dt;

			if(input->keysDown[KEYCODE_SHIFT]) {
				speed *= 2;
				// if(input->keysDown[KEYCODE_CTRL]) speed *= 2;
			}

			Vec3 vel = vec3(0,0,0);
			if(input->keysDown[KEYCODE_CTRL]) o.dir = normVec3(cross(vec3(0,0,1), o.right));
			if(input->keysDown[KEYCODE_W]) vel += o.dir;
			if(input->keysDown[KEYCODE_S]) vel += -o.dir;
			if(input->keysDown[KEYCODE_A]) vel += -o.right;
			if(input->keysDown[KEYCODE_D]) vel += o.right;
			if(input->keysDown[KEYCODE_E]) vel += vec3(0,0,1);
			if(input->keysDown[KEYCODE_Q]) vel += vec3(0,0,-1);

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
				glClearTexSubImage(t->id, 0, 0,0,0, t->dim.w,t->dim.h, 1, GL_RGB, GL_FLOAT, &black);
			}
		}

		{
			World* world = &ad->world;

			if(init || input->keysPressed[KEYCODE_T]) {

				world->shapeCount = 0;
				if(world->shapes) free(world->shapes);
				world->shapes = mallocArray(Shape, 1000);

				int count = 5;
				float r = 30;
				Vec3 offset = vec3(0,0,r/2.0f);
				for(int i = 0; i < count; i++) {
					int type = randomIntPCG(0, SHAPE_COUNT-1);
					Vec3 pos = vec3(randomFloatPCG(-r,r,0.01f), randomFloatPCG(-r,r,0.01f), randomFloatPCG(-r/2.0f,r/2.0f,0.01f));
					pos += offset;
					float size = randomFloatPCG(15,30,0.01f);

					bool emitter = randomIntPCG(0,1);
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

				// Plane

				Shape s = {};
				s.type = SHAPE_BOX;
				s.pos = vec3(0,0,0);
				s.dim = vec3(1000,1000,0.0001f);
				s.color = vec3(0.5f);
				// s.color = vec3(0.99f);
				// s.reflectionMod = 0.3f;
				s.reflectionMod = 0.5f;
				// s.reflectionMod = 0.9f;
				// s.reflectionMod = 0.1f;
				world->shapes[world->shapeCount++] = s;




				// Shape s = {};
				// s.type = SHAPE_SPHERE;
				// s.pos = vec3(0,0,10);
				// s.r = 10;
				// s.color = vec3(0.0f);
				// // s.color = vec3(0.99f);
				// // s.reflectionMod = 0.3f;
				// s.reflectionMod = 0.0f;
				// // s.reflectionMod = 0.9f;
				// // s.reflectionMod = 0.1f;
				// world->shapes[world-> shapeCount++] = s;

				// Shape s = {};
				// s.type = SHAPE_BOX;
				// s.pos = vec3(0,0,10);
				// s.dim = vec3(10.0f);
				// s.color = vec3(0.0f);
				// // s.color = vec3(0.99f);
				// // s.reflectionMod = 0.3f;
				// s.reflectionMod = 0.0f;
				// // s.reflectionMod = 0.9f;
				// // s.reflectionMod = 0.1f;
				// world->shapes[world-> shapeCount++] = s;



				world->defaultEmitColor = vec3(0.7f, 0.8f, 0.9f);
				// world->defaultEmitColor = vec3(1.0f);
				world->globalLightDir = normVec3(vec3(-1.5f,-1,-2.0f));
				world->globalLightColor = vec3(1,1,1);
				// world->globalLightColor = vec3(0.0f);

				// Calc bounding spheres.
				for(int i = 0; i < world->shapeCount; i++) {
					Shape* s = world->shapes + i;
					shapeBoundingSphere(s);
				}
			}

		}

		// if((input->keysPressed[KEYCODE_SPACE] || reload || init || ad->keepUpdating) && (ad->activeProcessing == false)) {
		if((input->keysPressed[KEYCODE_SPACE] || ad->keepUpdating) && (ad->activeProcessing == false)) {
		// if(false) {
			ad->activeProcessing = true;
			ad->drawSceneWired = false;

			RaytraceSettings* settings = &ad->settings;

			Texture* t = &ad->raycastTexture;
			Vec3 black = vec3(0.2f);
			if(!ad->keepUpdating)
				glClearTexSubImage(t->id, 0, 0,0,0, t->dim.w,t->dim.h, 1, GL_RGB, GL_FLOAT, &black);

			int pixelCount = settings->texDim.w*settings->texDim.h;
			if(ad->buffer != 0) free(ad->buffer);
			ad->buffer = mallocArray(Vec3, pixelCount);

			// Setup samples.
			{
				int mode = settings->sampleMode;

				Vec2* blueNoiseSamples;

				int sampleCount;
				if(mode == SAMPLE_MODE_GRID) sampleCount = settings->sampleCountGrid*settings->sampleCountGrid;
				else if(mode == SAMPLE_MODE_BLUE) sampleCount = blueNoise(rect(0,0,1,1), 1/(float)settings->sampleCountGrid, &blueNoiseSamples);
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

		if((!ad->keepUpdating && ad->activeProcessing) || doneProcessing || (ad->keepUpdating && !ad->activeProcessing))
			glTextureSubImage2D(ad->raycastTexture.id, 0, 0, 0, ad->settings.texDim.w, ad->settings.texDim.h, GL_RGB, GL_FLOAT, ad->buffer);

		// Screenshot.
		if(input->keysPressed[KEYCODE_RETURN] && !ad->activeProcessing) {
			Vec2i texDim = ad->settings.texDim;
			int size = texDim.w * texDim.h;
			char* intBuffer = mallocArray(char, size*3);

			Vec3* floatBuffer = ad->buffer;
			for(int i = 0; i < size; i++) {
				intBuffer[i*3 + 0] = colorFloatToInt(floatBuffer[i].r);
				intBuffer[i*3 + 1] = colorFloatToInt(floatBuffer[i].g);
				intBuffer[i*3 + 2] = colorFloatToInt(floatBuffer[i].b);
			}

			stbi_write_png("test.png", texDim.w, texDim.h, 3, intBuffer, 0);

			free(intBuffer);
		}

		if(input->keysPressed[KEYCODE_1]) ad->texFastMode = 0;
		if(input->keysPressed[KEYCODE_2]) ad->texFastMode = 1;
		if(input->keysPressed[KEYCODE_3]) ad->texFastMode = 2;
		if(input->keysPressed[KEYCODE_4]) ad->texFastMode = 3;
		if(input->keysPressed[KEYCODE_5]) ad->texFastMode = 4;



		{

			Rect sr = getScreenRect(ws);
			Rect tr = sr;
			Vec2 sd = rectDim(sr);
			Vec2 texDim = vec2(ad->raycastTexture.dim);
			if(((float)texDim.h / texDim.w) > (sd.h / sd.w)) {
				tr = rectSetW(tr, ((float)texDim.w / texDim.h)*sd.h);
			} else {
				tr = rectSetH(tr, ((float)texDim.h / texDim.w)*sd.w);
			}

			glDepthMask(false);
			drawRect(tr, rect(0,0,1,1), ad->raycastTexture.id);
			glDepthMask(true);

			ad->textureScreenRect = tr;
		}

		if(input->keysPressed[KEYCODE_R]) {
			for(int i = 0; i < ad->threadCount; i++) ad->threadData[i].stopProcessing = true;
			ad->waitingForThreadStop = true;
		}

		if(ad->waitingForThreadStop && threadQueueFinished(threadQueue)) {
			ad->waitingForThreadStop = false;

			Texture* t = &ad->raycastTexture;
			Vec3 black = vec3(0.2f);
			if(t->isCreated) {
				glClearTexSubImage(t->id, 0, 0,0,0, t->dim.w,t->dim.h, 1, GL_RGB, GL_FLOAT, &black);
			}
			if(ad->buffer) zeroMemory(ad->buffer, ad->raycastTexture.dim.w*ad->raycastTexture.dim.h*sizeof(Vec3));


			ad->drawSceneWired = true;
		}

	}


	if(ad->drawSceneWired)
	{
		World* world = &ad->world;
		Camera* cam = &ad->world.camera;
		Vec2 d = cam->dim;

		Rect tr = ad->textureScreenRect;

		glDepthMask(false);
		Vec3 cc = world->defaultEmitColor;
		drawRect(tr, vec4(cc,1));
		glDepthMask(true);

		glViewport(tr.left, -tr.top, rectW(tr), rectH(tr));



		// glClearColor(cc.r, cc.g, cc.g, 1);
		// glClear(GL_COLOR_BUFFER_BIT);


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
		rowToColumn(&vm);
		glLoadMatrixf(vm.e);

		Vec4 lp = vec4(-world->globalLightDir, 0);
		glLightfv(GL_LIGHT0, GL_POSITION, lp.e);

		// Vec4 lightAmbientColor = vec4(world->defaultEmitColor,1);
		// glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbientColor.e);
		Vec4 lightDiffuseColor = vec4(world->globalLightColor,1);
		glLightfv(GL_LIGHT0, GL_AMBIENT, lightDiffuseColor.e);

		// Vec4 lightDiffuseColor = vec4(world->globalLightColor,1);
		// glLightfv(GL_LIGHT0, GL_AMBIENT, lightDiffuseColor.e);



		glEnable(GL_DEPTH_TEST);
		glBindTexture(GL_TEXTURE_2D, 0);



		{
			glDisable(GL_LIGHTING);
			float l = 500;
			drawLine(vec3(-l,0,0), vec3(0,0,0), vec4(1,0,0,1));
			drawLine(vec3(0,0,0), vec3(l,0,0), vec4(1,0.5f,0.5f,1));
			drawLine(vec3(0,-l,0), vec3(0,0,0), vec4(0,1,0,1));
			drawLine(vec3(0,0,0), vec3(0,l,0), vec4(0.5f,1,0.5f,1));
			drawLine(vec3(0,0,-1), vec3(0,0,0), vec4(0,0,1,1));
			drawLine(vec3(0,0,0), vec3(0,0,1), vec4(0.5f,0.5f,1,1));
			glEnable(GL_LIGHTING);
		}

		// if(false)
		{
			Shape* shapes = world->shapes;


			// glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			// glDisable(GL_CULL_FACE);

			// Vec4 ambientColor = COLOR_SRGB(vec4(world->defaultEmitColor, 1));
			// Vec4 ambientColor = COLOR_SRGB(vec4(vec3(1,0,0), 1));
			// glMaterialfv(GL_FRONT, GL_AMBIENT, ambientColor.e);

			for(int i = 0; i < world->shapeCount; i++) {
				Shape* s = shapes + i;

				Vec4 diffuseColor = COLOR_SRGB(vec4(s->color, 1));
				// glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuseColor.e);
				glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, diffuseColor.e);

				Vec4 emissionColor = COLOR_SRGB(vec4(s->emitColor, 1));
				glMaterialfv(GL_FRONT, GL_EMISSION, emissionColor.e);

				float shininess = lerp(s->reflectionMod, 0,128);
				glMaterialf(GL_FRONT, GL_SHININESS, shininess);

				switch(s->type) {
					case SHAPE_BOX: {
						drawBox(s->pos, s->dim, vec4(s->color, 1));
					} break;

					case SHAPE_SPHERE: {
						drawSphere(s->pos, s->r, vec4(s->color, 1));
					} break;
				}
			}

			// glEnable(GL_CULL_FACE);
			// glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}

		if(false)
		{
			// Vec4 ambientColor = vec4(world->defaultEmitColor, 1);
			// Vec4 diffuseColor = vec4(1,0.5f,0.2f, 1);
			// float shininess = 80;
			// glMaterialfv(GL_FRONT, GL_AMBIENT,   ambientColor.e);
			// glMaterialfv(GL_FRONT, GL_DIFFUSE,   diffuseColor.e);
			// // glMaterialfv(GL_FRONT, GL_SPECULAR,  diffuseColor.e);
			// // glMaterialfv(GL_FRONT, GL_EMISSION,  diffuseColor.e);
			// glMaterialfv(GL_FRONT, GL_SHININESS, &shininess);


			// Vec3 p = vec3(0,0,10);

			// // drawPlane(p, vec2(5,5), vec3(0,0,1), vec4(1,1,1,1));
			// drawBox(p, vec3(5.0f), vec4(1,1,1,1));

			Vec3 p = vec3(0,0,10);
			// drawPlane(p, vec2(10,20), vec4(0.2f,1));

			Vec3 lp = vec3(-10,0,20);
			Vec3 ld = normVec3(vec3(1,0,-1));

			glDisable(GL_LIGHTING);
			drawLine(lp, lp + ld*100, vec4(1,0,0,1));
		}

		if(false)
		{
			glDisable(GL_LIGHTING);
			Vec3 lp = vec3(0,-10 + sin(ad->time)*13,20);
			Vec3 ld = normVec3(vec3(0, 1, -1));
			Vec3 pp = vec3(0,0,10);
			Vec3 pn = normVec3(vec3(0,-2,1));
			Vec3 pu = cross(pn, vec3(1,0,0));
			Vec2 pdim = vec2(13,20);

			drawLine(lp, lp+ld*100, vec4(0.8f,1));

			drawPlane(pp, pn, pu, pdim, vec4(1,1,1,1));


			Vec3 a = vec3(1,2,3);
			Vec3 b = vec3(5,6,-23);
			float c = 34;

			float x = dot(a, b) * c;
			float y = dot(a, b*c);

			Vec3 ip, in;
			float distance = linePlaneIntersection(lp, ld, pp, pn, pu, pdim, &ip, &in);
			if(distance >= 0) {
				Vec4 c = vec4(1,0,0,1);
				glMaterialfv(GL_FRONT, GL_DIFFUSE, c.e);
				drawSphere(ip, 0.1f, vec4(1,0,0,1));
				drawLine(ip, ip+in*10, vec4(0,1,1,1));
			}

			// Vec3 ip, in;
			// bool result = lineSphereIntersection(lp, ld, sp, sr, &ip, &in);

			// if(result) {
			// 	Vec4 c = vec4(1,0,0,1);
			// 	glMaterialfv(GL_FRONT, GL_DIFFUSE, c.e);
			// 	drawSphere(ip, 0.1f, vec4(1,0,0,1));
			// 	drawLine(ip, ip+in*10, vec4(0,1,1,1));
			// }

			// drawSphere(vec3(0,-5,0), 0.2f, vec4(1,0,1,1));
		}

		glMatrixMode(GL_PROJECTION); glPopMatrix();
		glMatrixMode(GL_MODELVIEW); glPopMatrix();
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_LIGHTING);
		glDisable(GL_NORMALIZE);
	}

	#if 0
	{
		int cellCount = 8;
		float radius = 40;
		float cellSize = 200;
		Rect r = rectTLDim(vec2(0,0), vec2(cellSize,cellSize));
		static Vec2* noiseSamples;
		static int sampleCount;


		if(init || reload || input->keysPressed[KEYCODE_H]) {
			if(noiseSamples) free(noiseSamples);

			// sampleCount = blueNoise(r, 0, &noiseSamples, 8*8);
			// sampleCount = blueNoise(r, (800/8)/M_SQRT2*0.4f, &noiseSamples);
			sampleCount = blueNoise(r, radius, &noiseSamples);
			// sampleCount = blueNoise(r, cellSize/cellCount/M_SQRT2, &noiseSamples);
			// sampleCount = blueNoise(r, cellSize/cellCount/2, &noiseSamples);

			// float ce = radius / M_SQRT2;
			// float asdf = cellSize/ce;
			// printf("%f %i %f\n", asdf*asdf, sampleCount, (float)sampleCount/(asdf*asdf));
		}


		// Vec2 tl = rectTL(r);
		// Vec4 lc = vec4(0.5f, 1);
		// for(int i = 0; i < cellCount; i++) 
		// 	drawLine(tl + vec2(0,-cellSize) * i/(cellCount), tl + vec2(0,-cellSize) * i/(cellCount) + vec2(cellSize,0), lc);
		// for(int i = 0; i < cellCount; i++) 
		// 	drawLine(tl + vec2(cellSize,0) * i/(cellCount), tl + vec2(cellSize,0) * i/(cellCount) + vec2(0,-cellSize), lc);


		drawRect(rectCenDim(0,0,10000,10000), vec4(0,1));


		Vec2 p = vec2(300,-200);
		glPointSize(10);

		for(int y = 0; y < 3; y++) {
			for(int x = 0; x < 3; x++) {
				// if(x == 1 && y == 1) 
				{
					// y = 1; 
					// x = 1;

					Vec2 offset = p + vec2(x*cellSize, y*-cellSize);
					// drawRect(rectTLDim(offset, vec2(cellSize)), vec4(0,1));
			
					for(int i = 0; i < sampleCount; i++) {
						Vec2 p = noiseSamples[i] + offset;
						drawPoint(p, vec4(1,0,0,1));
						// drawCircle(p, radius/2.0f, vec4(1,1,0,1));
					}
				}
			}
		}

		// glPointSize(10);
		// for(int i = 0; i < sampleCount; i++) {
		// 	Vec2 p = noiseSamples[i];
		// 	drawPoint(p, vec4(1,0,0,1));
		// 	drawCircle(p, radius/2.0f, vec4(1,1,0,1));
		// }

		// drawRect(rectCenDim(100,-100,100,100), vec4(1,0,0,1));

		// free(noiseSamples);
	}
	#endif

	#if 0
	{
		drawRect(rectCenDim(0,0,10000,10000), vec4(0,1));

		Vec2 p = vec2(300,-200);
		glPointSize(10);

		float cellSize = 200;
		int sampleCount = ad->settings.sampleCount;
		Vec2* noiseSamples = ad->settings.samples;

		for(int y = 0; y < 3; y++) {
			for(int x = 0; x < 3; x++) {
				Vec2 offset = p + vec2(x*cellSize, y*-cellSize);
				// drawRect(rectTLDim(offset, vec2(cellSize)), vec4(0,1));
		
				for(int i = 0; i < sampleCount; i++) {
					Vec2 p = noiseSamples[i]*cellSize + offset;
					drawPoint(p, vec4(1,0,0,1));
					// drawCircle(p, radius/2.0f, vec4(1,1,0,1));
				}
			}
		}

		// glPointSize(10);
		// for(int i = 0; i < sampleCount; i++) {
		// 	Vec2 p = noiseSamples[i];
		// 	drawPoint(p, vec4(1,0,0,1));
		// 	drawCircle(p, radius/2.0f, vec4(1,1,0,1));
		// }

		// drawRect(rectCenDim(100,-100,100,100), vec4(1,0,0,1));

		// free(noiseSamples);
	}
	#endif

	if(false)
	{

		int w = 1920;
		float cw = 10.0f;

		float pixelPercent = 1/(float)w;

		float res = 1/10.0f;

		float a = pixelPercent * res;


				// rayPos += camera.ovecs.right * (camera.dim.w * (percent.w + settings.pixelPercent.w*samples[sampleIndex].x));


		exit(0);
	}

	// Draw Info.
	{
		Rect sr = getScreenRect(ws);
		glViewport(0,0, rectW(sr), rectH(sr));
		// glViewport(tr.left, -tr.top, rectW(tr), rectH(tr));

		Rect tr = ad->textureScreenRect;
		Font* font = getFont("OpenSans-Bold.ttf", 20);
		TextSettings settings = textSettings(font, vec4(1,0.5f,0,1), TEXT_SHADOW, vec2(1,-1), 1.5, vec4(0,0,0,1));

		Vec2i texDim = ad->settings.texDim;

		Vec2 p = rectTR(tr) + vec2(-font->height*0.25f,0);
		float lh = font->height * 0.9f;
		drawText(fillString("%i x %i", texDim.x, texDim.h), p, vec2i(1,1), settings); p += vec2(0,-lh);
		drawText(fillString("%i. pixels", texDim.x * texDim.h), p, vec2i(1,1), settings); p += vec2(0,-lh);
		drawText(fillString("%i. samples", ad->settings.sampleCount), p, vec2i(1,1), settings); p += vec2(0,-lh);
		drawText(fillString("%fs", (float)ad->processTime), p, vec2i(1,1), settings); p += vec2(0,-lh);
		drawText(fillString("%fms per pixel", (float)(ad->processTime/(texDim.x*texDim.y)*1000000)), p, vec2i(1,1), settings); p += vec2(0,-lh);

		drawText(fillString("cpos: %f,%f,%f", PVEC3(ad->world.camera.pos)), p, vec2i(1,1), settings); p += vec2(0,-lh);
		drawText(fillString("crot: %f,%f,%f", PVEC3(ad->world.camera.rot)), p, vec2i(1,1), settings); p += vec2(0,-lh);
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
