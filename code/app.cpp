/*
=================================================================================

	ToDo:
	* Better sampling pattern.
	- Blue noise.
	- Replace rand().
	- Better angle calculation.
	- Simd.
	- Ui.
	- Random placement.
	- Entity editor.
	- More Shapes and rotations.

	Done Today: 

	Bugs:
	- Black pixels.
	- Grid sampling wrong on many samples.

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

	bool keepUpdating;

	Vec2i texDim;

	World world;
	Vec3* buffer;
	Texture raycastTexture;
	bool activeProcessing;

	ProcessPixelsData threadData[THREAD_JOB_COUNT];

	f64 processStartTime;
	f64 processTime;

	Rect textureScreenRect;
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




		// Vec2i texDim = vec2i(1920*2,1080*2);
		// Vec2i texDim = vec2i(1920,1080);
		// Vec2i texDim = vec2i(1280,720);
		// Vec2i texDim = vec2i(1280/2,720/2);
		Vec2i texDim = vec2i(320,180);
		// Vec2i texDim = vec2i(160,90);
		// Vec2i texDim = vec2i(8,8);
		// Vec2i texDim = vec2i(2,2);

		ad->texDim = texDim;
		ad->keepUpdating = true;

		Texture* texture = &ad->raycastTexture;
		if(!texture->isCreated || (texDim != texture->dim)) {
			if(texDim != texture->dim) deleteTexture(texture);

			// initTexture(texture, -1, INTERNAL_TEXTURE_FORMAT, texDim, 3, GL_NEAREST, GL_CLAMP);
			initTexture(texture, -1, INTERNAL_TEXTURE_FORMAT, texDim, 3, GL_LINEAR, GL_CLAMP);
		}


		if(init || reload) {
			ad->world.camera.pos = vec3(0, -20, 4);
			// ad->world.camera.pos = vec3(1, -1, -3);
			ad->world.camera.rot = vec3(0, 0, 0);
		}

		if(input->keysPressed[KEYCODE_F3]) {
			ad->captureMouse = !ad->captureMouse;
		}

		ad->fpsMode = ad->captureMouse && windowHasFocus(windowHandle);
		// printf("%i\n", windowHasFocus(windowHandle));
		if(ad->fpsMode) captureMouse(windowHandle, ad->captureMouse, input);

		{
			Camera* cam = &ad->world.camera;

			if((!ad->fpsMode && input->mouseButtonDown[0]) || ad->fpsMode) {
				float speed = 0.1f;
				
				cam->rot.x += -input->mouseDelta.x*speed*ad->dt;
				cam->rot.y += input->mouseDelta.y*speed*ad->dt;
				clamp(&cam->rot.y, -M_PI_2 + 0.001f, M_PI_2 - 0.001f);
			}

			Orientation o = getVectorsFromRotation(cam->rot);
			float speed = 5.0f*ad->dt;

			if(input->keysDown[KEYCODE_SHIFT]) {
				speed *= 2;
				// if(input->keysDown[KEYCODE_CTRL]) speed *= 2;
			}

			if(input->keysDown[KEYCODE_CTRL]) o.dir = normVec3(cross(vec3(0,0,1), o.right));
			if(input->keysDown[KEYCODE_W]) cam->pos += o.dir * speed;
			if(input->keysDown[KEYCODE_S]) cam->pos += -o.dir * speed;
			if(input->keysDown[KEYCODE_A]) cam->pos += -o.right * speed;
			if(input->keysDown[KEYCODE_D]) cam->pos += o.right * speed;
			if(input->keysDown[KEYCODE_E]) cam->pos += vec3(0,0,1) * speed;
			if(input->keysDown[KEYCODE_Q]) cam->pos += vec3(0,0,-1) * speed;

			cam->dist = 1;
			float camWidth = cam->dist * 2; // 90 degrees for now.
			float aspectRatio = (float)texDim.w / texDim.h;
			cam->dim = vec2(camWidth, camWidth*(1/aspectRatio)); 
		}

		if((input->keysPressed[KEYCODE_SPACE] || reload || init || ad->keepUpdating) && (ad->activeProcessing == false)) {
		// if(false) {
			ad->activeProcessing = true;

			World* world = &ad->world;

			world->shapeCount = 0;
			if(init) {
				world->shapes = getPArray(Shape, 100);
			}

			Shape s;

			s = {};
			s.type = SHAPE_BOX;
			s.pos = vec3(0,0,0);
			// s.dim = vec3(12,12,1);
			s.dim = vec3(10000,10000,0.0001f);
			s.color = vec3(0.5f);
			// s.color = vec3(0.99f);
			// s.reflectionMod = 0.3f;
			s.reflectionMod = 0.5f;
			// s.reflectionMod = 0.1f;
			world->shapes[world->shapeCount++] = s;

			s = {};
			s.type = SHAPE_BOX;
			s.pos = vec3(-15,2,0);
			s.dim = vec3(1,10,50);
			s.color = vec3(0,0.8f,0.5f);
			s.reflectionMod = 0.7f;
			world->shapes[world->shapeCount++] = s;

			s = {};
			s.type = SHAPE_BOX;
			s.dim = vec3(5,2,0.3f);
			s.pos = vec3(0,-10,s.dim.z*0.5f + 0.5f);
			s.color = vec3(0.8f,0.3f,0.5f);
			s.reflectionMod = 0.9f;
			world->shapes[world->shapeCount++] = s;

			s = {};
			s.type = SHAPE_BOX;
			s.dim = vec3(1,1,1);
			s.pos = vec3(0,0,0);
			s.color = vec3(0,0,0);
			s.reflectionMod = 0.1f;
			world->shapes[world->shapeCount++] = s;


			s = {};
			s.type = SHAPE_SPHERE;
			float animSpeed = 0.5f;
			// s.pos = vec3(6*sin(ad->time*animSpeed), 3*cos(ad->time*animSpeed) , 7 + 1*cos(ad->time*animSpeed*0.5f));
			s.pos = vec3(8, -2, 0);
			s.r = 4;
			s.color = vec3(0.3f,0.5f,0.8f);
			s.reflectionMod = 0.8f;
			world->shapes[world->shapeCount++] = s;

			s = {};
			s.type = SHAPE_SPHERE;
			s.pos = vec3(-8,0,1);
			s.r = 2;
			s.color = vec3(0.0f);
			s.emitColor = vec3(2,0,0);
			s.reflectionMod = 1;
			world->shapes[world->shapeCount++] = s;

			s = {};
			s.type = SHAPE_SPHERE;
			Vec3 animRange = vec3(5,5,5);
			// s.pos = vec3(-2 + animRange.x*sin(ad->time*animSpeed), 20 + animRange.y*cos(ad->time*animSpeed) , 10 + animRange.z*cos(ad->time*animSpeed*0.5f));
			s.pos = vec3(-2,20,10);
			s.r = 8;
			s.color = vec3(0.5f);
			s.emitColor = vec3(0.0f);
			s.reflectionMod = 1;
			world->shapes[world->shapeCount++] = s;


			world->defaultEmitColor = vec3(0.7f, 0.8f, 0.9f);
			// world->defaultEmitColor = vec3(0.95f);
			// world->defaultEmitColor = vec3(0.0f);
			// world->globalLightDir = normVec3(vec3(1,1,-1));
			// world->globalLightDir = normVec3(vec3(-1,1,-1));
			world->globalLightDir = normVec3(vec3(-1,0,-1));
			// world->globalLightDir = normVec3(vec3(0,0,-1));
			world->globalLightColor = vec3(1);



			if(ad->buffer != 0) free(ad->buffer);
			ad->buffer = mallocArray(Vec3, texDim.w * texDim.h);

			int threadCount = THREAD_JOB_COUNT;
			// int threadCount = 1;
			int pixelCount = texDim.w*texDim.h;
			int pixelCountPerThread = pixelCount/threadCount;
			int pixelCountRest = pixelCount % threadCount;

			for(int i = 0; i < threadCount; i++) {
				ProcessPixelsData* data = ad->threadData + i;
				*data = {};
				data->pixelIndex = i*pixelCountPerThread;
				data->pixelCount = i < threadCount-1 ? pixelCountPerThread : pixelCountPerThread + pixelCountRest;
				data->world = world;
				data->dim = texDim;
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
		}

		if((!ad->keepUpdating && ad->activeProcessing) || doneProcessing || (ad->keepUpdating && !ad->activeProcessing))
			glTextureSubImage2D(ad->raycastTexture.id, 0, 0, 0, texDim.w, texDim.h, GL_RGB, GL_FLOAT, ad->buffer);

		// Screenshot.
		if(input->keysPressed[KEYCODE_RETURN] && !ad->activeProcessing) {
			Vec2i texDim = ad->texDim;
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



		{
			Vec2 texDim = vec2(ad->raycastTexture.dim);

			Rect sr = getScreenRect(ws);
			Rect tr = sr;
			Vec2 sd = rectDim(sr);
			if(((float)texDim.h / texDim.w) > (sd.h / sd.w)) {
				tr = rectSetW(tr, ((float)texDim.w / texDim.h)*sd.h);
			} else {
				tr = rectSetH(tr, ((float)texDim.h / texDim.w)*sd.w);
			}
			ad->textureScreenRect = tr;
			// tr = rectExpand(tr, vec2(-50));
			glDepthMask(false);
			drawRect(tr, rect(0,0,1,1), ad->raycastTexture.id);
			glDepthMask(true);


		}

	}


	if(true)
	{
		Camera* cam = &ad->world.camera;
		Vec2 d = cam->dim;

		Rect tr = ad->textureScreenRect;
		glViewport(tr.left, -tr.top, rectW(tr), rectH(tr));

		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glFrustum(-d.w/2.0f, d.w/2.0f, -d.h/2.0f, d.h/2.0f, cam->dist, 10000);

		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();

		Orientation o = getVectorsFromRotation(cam->rot);
		Mat4 vm = viewMatrix(cam->pos, o.dir, o.up, o.right);
		rowToColumn(&vm);
		glLoadMatrixf(vm.e);


		glEnable(GL_DEPTH_TEST);
		glBindTexture(GL_TEXTURE_2D, 0);

		{
			float l = 100;
			drawLine(vec3(-l,0,0), vec3(0,0,0), vec4(1,0,0,1));
			drawLine(vec3(0,0,0), vec3(l,0,0), vec4(1,0.5f,0.5f,1));
			drawLine(vec3(0,-l,0), vec3(0,0,0), vec4(0,1,0,1));
			drawLine(vec3(0,0,0), vec3(0,l,0), vec4(0.5f,1,0.5f,1));
			drawLine(vec3(0,0,-l), vec3(0,0,0), vec4(0,0,1,1));
			drawLine(vec3(0,0,0), vec3(0,0,l), vec4(0.5f,0.5f,1,1));
		}

		{
			World* world = &ad->world;
			Shape* shapes = world->shapes;

			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

			for(int i = 0; i < world->shapeCount; i++) {
				Shape* s = shapes + i;

				switch(s->type) {
					case SHAPE_BOX: {
						drawBox(s->pos, s->dim, vec4(s->color, 1));
					} break;

					case SHAPE_SPHERE: {
						drawCircle(s->pos, s->r, normVec3(cam->pos - s->pos), vec4(s->color, 1));
					} break;
				}
			}

			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}

		glMatrixMode(GL_PROJECTION); glPopMatrix();
		glMatrixMode(GL_MODELVIEW); glPopMatrix();
		glDisable(GL_DEPTH_TEST);
	}


	#if 0
	{
		int cellCount = 8;
		float cellSize = 800;
		Rect r = rectTLDim(vec2(100,-100), vec2(cellSize,cellSize));
		static Vec2* noiseSamples;
		static int sampleCount;

		if(init || reload) {
			// sampleCount = blueNoise(r, 0, &noiseSamples, 8*8);
			// sampleCount = blueNoise(r, (800/8)/M_SQRT2*0.4f, &noiseSamples);
			sampleCount = blueNoise(r, 50, &noiseSamples);
			// sampleCount = blueNoise(r, cellSize/cellCount/M_SQRT2, &noiseSamples);
			// sampleCount = blueNoise(r, cellSize/cellCount/2, &noiseSamples);
		}

		drawRect(r, vec4(0,1));

		// for(int y = 0; y < cellCount; y++) {
		// 	for(int x = 0; x < cellCount; x++) {
		// 		drawPoint(vec2(
		// 	}
		// }

		Vec2 tl = rectTL(r);
		for(int i = 0; i < cellCount; i++) drawLine(tl + vec2(0,-cellSize) * i/(cellCount), tl + vec2(0,-cellSize) * i/(cellCount) + vec2(cellSize,0), vec4(0,1,0,1));
		for(int i = 0; i < cellCount; i++) drawLine(tl + vec2(cellSize,0) * i/(cellCount), tl + vec2(cellSize,0) * i/(cellCount) + vec2(0,-cellSize), vec4(0,1,0,1));

		glPointSize(10);
		for(int i = 0; i < sampleCount; i++) {
			Vec2 p = noiseSamples[i];
			drawPoint(p, vec4(1,0,0,1));
		}
		// drawRect(rectCenDim(100,-100,100,100), vec4(1,0,0,1));

		// free(noiseSamples);

	}
	#endif




	// Draw Info.
	{
		Rect sr = getScreenRect(ws);
		glViewport(0,0, rectW(sr), rectH(sr));
		// glViewport(tr.left, -tr.top, rectW(tr), rectH(tr));

		Rect tr = ad->textureScreenRect;
		Font* font = getFont("OpenSans-Bold.ttf", 20);
		TextSettings settings = textSettings(font, vec4(1,0.5f,0,1), TEXT_SHADOW, vec2(1,-1), 1.5, vec4(0,0,0,1));

		Vec2 p = rectTR(tr) + vec2(-font->height*0.25f,0);
		float lh = font->height * 0.8f;
		drawText(fillString("%i x %i", ad->texDim.x, ad->texDim.h), p, vec2i(1,1), settings); p += vec2(0,-lh);
		drawText(fillString("%i. pixels", ad->texDim.x * ad->texDim.h), p, vec2i(1,1), settings); p += vec2(0,-lh);
		drawText(fillString("%fs", (float)ad->processTime), p, vec2i(1,1), settings); p += vec2(0,-lh);
		drawText(fillString("%fms per pixel", (float)(ad->processTime/(ad->texDim.x*ad->texDim.y)*1000000)), p, vec2i(1,1), settings); p += vec2(0,-lh);

		drawText(fillString("cpos: %f,%f,%f", PVEC3(ad->world.camera.pos)), p, vec2i(1,1), settings); p += vec2(0,-lh);
		drawText(fillString("crot: %f,%f,%f", PVEC3(ad->world.camera.rot)), p, vec2i(1,1), settings); p += vec2(0,-lh);

		// Quat q = eulerAnglesToQuat(0,0.1f,0);
		// Vec3 d = vec3(0,1,0);
		// d = normVec3(q*d);

		// drawText(fillString("%f,%f,%f\n", PVEC3(d)), p, vec2i(1,1), settings); p += vec2(0,-lh);


		// float a = dot(vec3(1,0,0), normVec3(vec3(1,1,0)));
		// a = dotUnitPercent(vec3(1,0,0), normVec3(vec3(-1,1,0)));

		// drawText(fillString("%f\n", a), p, vec2i(1,1), settings); p += vec2(0,-lh);

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
