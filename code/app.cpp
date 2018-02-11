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
	- More advanced rendering techniques.
	- Put input updating in own thread.
	- Float not precise enough at hundreds of samples per pixel?!

	- Depth of field.
	- Reflection.
	- Refraction.
	
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
#define STBI_ONLY_PNG
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






template <class t>
bool greaterThen(t a, t b) {
	return a > b;
}


template <class Type>
struct DArray {
	Type* data;
	int count = 0;
	int size;

	bool memoryAllocated = false;
	bool usesPoolMemory;

	void init(int size) {
		data = mallocArray(Type, size);
		count = 0;
		size = size;
		usesPoolMemory = false;
		memoryAllocated = true;
	}

	void init(Type* dataBlock, int size) {
		data = dataBlock;
		count = 0;
		size = size;
		usesPoolMemory = true;
		memoryAllocated = true;
	}

	void push(Type s) {
		data[count++] = s;
	}

	void release() {
		if(memoryAllocated) {
			if(!usesPoolMemory) free(data);
			memoryAllocated = false;
		}
	}
};

template <class Type>
struct LinkedList {
	struct Node {
		Type data;
		Node* next;
	};

	Node* first;
	Node* last;

	void init(int size) {
		list = mallocArray(Node<Type>, size);
		last = first;
	}

	// void append(Type s) {
	// 	last->next = 
	// }

	void release() {
		free(list);
	}
};



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
	bool fitToScreen;

	int threadCount;
	ProcessPixelsData threadData[RAYTRACE_THREAD_JOB_COUNT];
	bool waitingForThreadStop;
  
	f64 processStartTime;
	f64 processTime;

	int texFastMode;

	// Editing

	float selectionAnimState;	
	int selectedObject;

	int axisMoveModeHot;
	int axisMoveMode;

	// Test

	char* grid;
	Vec2i gridSize;
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

			printf("File: %s\n", fd.fileName);
			if(strCompare(fd.fileName, "arrow.png")) {
				Texture tex;
				char* filePath = fillString("%s%s", App_Image_Folder, fd.fileName);
				loadTextureFromFile(&tex, filePath, -1, INTERNAL_TEXTURE_FORMAT, GL_RGBA, GL_UNSIGNED_BYTE);
				addTexture(tex);
			}
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


	#if 0
	{

		// DArray<int> da;

		// da.init(getPArray(int, 100), 100);
		// da.push(2);
		// da.push(453);
		// da.push(1.2f);

		// da.release();

		// glDisable(GL_DEPTH_TEST);


		// glClearColor(0.1f,0.1f,0.1f,1);
		// glClear(GL_COLOR_BUFFER_BIT);

		Rect sr = getScreenRect(ws);
		drawRect(sr, vec4(0.1f,1));

		if(init || reload) {
			Vec2i size = vec2i(10,10);
			int totalSize = size.w * size.h;
			ad->gridSize = size;
			reallocArraySave(char, ad->grid, totalSize);


		}

		zeroMemory(ad->grid, ad->gridSize.w*ad->gridSize.h*sizeof(char));
		Vec2i size = ad->gridSize;
		ad->grid[arrayIndex(size.w, size.h, 3,4)] = 1;
		ad->grid[arrayIndex(size.w, size.h, 4,4)] = 1;
		ad->grid[arrayIndex(size.w, size.h, 5,4)] = 1;
		ad->grid[arrayIndex(size.w, size.h, 6,4)] = 1;


		// Draw. 
		{
			Vec2 startPoint = vec2(300,-100);
			float tileSize = 50;
			Vec2i size = ad->gridSize;

			for(int y = 0; y < size.h; y++) {
				for(int x = 0; x < size.w; x++) {
					Vec4 color = vec4(0.3f,1);

					char value = ad->grid[arrayIndex(size.w, size.h, x,y)];

					if(value != 0) color = vec4(0.2f,0.6f,0.8f,1);

					drawRect(rectTLDim(startPoint + vec2(x*tileSize, -y*tileSize), vec2(tileSize-1)), color);
				}
			}
		}





		// *isRunning = false;
		// return;	
	}
	#endif


	#if 1

	{
		if(init) {
			ad->texFastMode = 2;

			ad->axisMoveModeHot = -1;
		}

		// @Settings.
		ad->settings.texDim = vec2i(240*pow(2, ad->texFastMode), 135*pow(2, ad->texFastMode));

		// ad->settings.sampleMode = SAMPLE_MODE_MSAA8X;
		ad->settings.sampleMode = SAMPLE_MODE_GRID;
		ad->settings.sampleCountGrid = 1;
		ad->settings.sampleGridWidth = 1;

		// ad->settings.sampleMode = SAMPLE_MODE_BLUE_MULTI;
		// ad->settings.sampleCountGrid = 4;
		// ad->settings.sampleGridWidth = 10;

		ad->settings.rayBouncesMax = 6;

		ad->keepUpdating = false;
    
		ad->threadCount = RAYTRACE_THREAD_JOB_COUNT;
		// ad->threadCount = 1;

		// glClearColor(1,0,0,1);
		// glClear(GL_COLOR_BUFFER_BIT);

		if(init) {
			ad->drawSceneWired = true;
			ad->fitToScreen = true;

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
			cam->fov = 90;

			float aspectRatio = (float)ad->settings.texDim.w / ad->settings.texDim.h;
			cam->dim.w = 10;
			cam->dim.h = cam->dim.w*(1/aspectRatio);
			cam->nearDist = camDistanceFromFOVandWidth(cam->fov, cam->dim.w);
			cam->farDist = 10000;
		}
		ad->world.camera.fov = 90;

		if(input->keysPressed[KEYCODE_F3]) {
			ad->captureMouse = !ad->captureMouse;
		}

		{
			ad->fpsMode = ad->captureMouse && windowHasFocus(windowHandle);
			if(ad->fpsMode) captureMouse(windowHandle, ad->captureMouse, input);

			Camera* cam = &ad->world.camera;

			if((!ad->fpsMode && input->mouseButtonDown[1]) || ad->fpsMode) {
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
				glClearTexSubImage(t->id, 0, 0,0,0, t->dim.w,t->dim.h, 1, GL_RGB, GL_FLOAT, &ad->world.defaultEmitColor);
			}
		}

		{
			World* world = &ad->world;

			#if 0

			if(init || input->keysPressed[KEYCODE_T]) {

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

			if(init || reload) {
				world->defaultEmitColor = vec3(1,1,1);

				world->ambientRatio = 0.2f;
				world->ambientColor = vec3(1.0f);

				// world->defaultEmitColor = vec3(0.7f, 0.8f, 0.9f);
				// world->defaultEmitColor = vec3(1.0f);
				// world->globalLightDir = normVec3(vec3(-1.5f,-1,-2.0f));
				// world->globalLightColor = vec3(1,1,1);
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

				obj = {};
				obj.pos = vec3(0,0,0);
				obj.color = vec3(0.5f);
				obj.material = materials[0];
				obj.geometry.type = GEOM_TYPE_BOX;
				// obj.geometry.dim = vec3(100000, 100000, 0.01f);
				obj.geometry.dim = vec3(5, 5, 0.01f);
				world->objects[world->objectCount++] = obj;

				// Sphere.

				obj = {};
				obj.pos = vec3(0,0,20);
				obj.color = vec3(0.3f,0.5f,0.8f);
				obj.material = materials[0];
				obj.geometry.type = GEOM_TYPE_SPHERE;
				obj.geometry.r = 5;
				world->objects[world->objectCount++] = obj;

				obj = {};
				obj.pos = vec3(10,0,15);
				obj.color = vec3(0.6f,0.5f,0.8f);
				obj.material = materials[0];
				obj.geometry.type = GEOM_TYPE_SPHERE;
				obj.geometry.r = 5;
				world->objects[world->objectCount++] = obj;

				obj = {};
				obj.pos = vec3(0,5,5);
				obj.color = vec3(0.3f,0.5f,0.2f);
				obj.material = materials[0];
				obj.geometry.type = GEOM_TYPE_SPHERE;
				obj.geometry.r = 5;
				world->objects[world->objectCount++] = obj;

				// Calc bounding spheres.
				for(int i = 0; i < world->objectCount; i++) {
					geometryBoundingSphere(&world->objects[i].geometry);
				}
			}

		}

		// if((input->keysPressed[KEYCODE_SPACE] || reload || init || ad->keepUpdating) && (ad->activeProcessing == false)) {
		// if((input->keysPressed[KEYCODE_SPACE] || ad->keepUpdating || reload) && (ad->activeProcessing == false)) {
		if((input->keysPressed[KEYCODE_SPACE] || ad->keepUpdating) && (ad->activeProcessing == false)) {
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
					settings->sampleCountGrid *= 1.3f;  // Mod for blue noise.
					sampleCount = blueNoise(rect(0,0,1,1), 1/(float)settings->sampleCountGrid, &blueNoiseSamples);
				} else if(mode == SAMPLE_MODE_BLUE_MULTI) {
					settings->sampleCountGrid *= 1.3f;
					sampleCount = blueNoise(rect(vec2(0.0f),vec2(settings->sampleGridWidth)), 1/(float)settings->sampleCountGrid, &blueNoiseSamples);
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
				glClearTexSubImage(t->id, 0, 0,0,0, t->dim.w,t->dim.h, 1, GL_RGB, GL_FLOAT, &ad->world.defaultEmitColor);
			}
			if(ad->buffer) zeroMemory(ad->buffer, ad->raycastTexture.dim.w*ad->raycastTexture.dim.h*sizeof(Vec3));


			ad->drawSceneWired = true;
		}

		if(input->keysPressed[KEYCODE_F]) ad->fitToScreen = !ad->fitToScreen;

	}

	#if 1
	if(input->mouseButtonPressed[0] && ad->axisMoveModeHot == -1) {
		Vec3 rayDir = mouseRayCast(ad->textureScreenRect, input->mousePosNegative, &ad->world.camera);

		int objectIndex = castRay(ad->world.camera.pos, rayDir, ad->world.objects, ad->world.objectCount);
		if(objectIndex != -1) {
			ad->selectedObject = objectIndex+1;
			ad->selectionAnimState = 0;
		} else ad->selectedObject = 0;
	}

	if(input->mouseButtonPressed[0] && ad->axisMoveModeHot != -1) {
		Vec3 axis[] = { vec3(1,0,0), vec3(0,1,0), vec3(0,0,1) };
		Vec3 dir = axis[ad->axisMoveModeHot];

		float amount = 1;

		ad->world.objects[ad->selectedObject-1].pos += dir*amount;
	}
	#endif

	if(ad->drawSceneWired)
	{
		World* world = &ad->world;
		Camera* cam = &ad->world.camera;
		Vec2 d = cam->dim;

		Rect tr = ad->textureScreenRect;

		glClearColor(0,0,0, 1);
		glClear(GL_COLOR_BUFFER_BIT);

		glViewport(tr.left, -tr.top, rectW(tr), rectH(tr));

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
		rowToColumn(&vm);
		glLoadMatrixf(vm.e);

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
			glDisable(GL_LIGHTING);
			float l = 500;
			Vec3 u = vec3(0,0,0.5f); // Remove z flicker.
			drawLine(u + vec3(-l,0,0), u + vec3(0,0,0), vec4(1,0,0,1));
			drawLine(u + vec3(0,0,0), u + vec3(l,0,0), vec4(1,0.5f,0.5f,1));
			drawLine(u + vec3(0,-l,0), u + vec3(0,0,0), vec4(0,1,0,1));
			drawLine(u + vec3(0,0,0), u + vec3(0,l,0), vec4(0.5f,1,0.5f,1));
			drawLine(u + vec3(0,0,-1), u + vec3(0,0,0), vec4(0,0,1,1));
			drawLine(u + vec3(0,0,0), u + vec3(0,0,1), vec4(0.5f,0.5f,1,1));
			glEnable(GL_LIGHTING);
		}

		{
			// glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			// glDisable(GL_CULL_FACE);

			// Vec4 ambientColor = COLOR_SRGB(vec4(world->defaultEmitColor, 1));
			// Vec4 ambientColor = COLOR_SRGB(vec4(vec3(1,0,0), 1));
			// glMaterialfv(GL_FRONT, GL_AMBIENT, ambientColor.e);

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

				switch(g->type) {
					case GEOM_TYPE_BOX: {
						drawBox(obj->pos, g->dim, vec4(obj->color, 1));
					} break;

					case GEOM_TYPE_SPHERE: {
						drawSphere(obj->pos, g->r, vec4(obj->color, 1));
					} break;
				}
			}

			#if 1
			if(ad->selectedObject > 0) {
				Object* obj = world->objects + ad->selectedObject-1;
				Geometry* geom = &obj->geometry;

				Vec4 color = vec4(1,1,1,1);
				float animSpeed = 3;
				ad->selectionAnimState += ad->dt * animSpeed;
				color.a = (cos(ad->selectionAnimState)+1)/2.0f;

				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
				glDisable(GL_LIGHTING);

				switch(geom->type) {
					case GEOM_TYPE_BOX: {
						drawBox(obj->pos, geom->dim, color);
					} break;

					case GEOM_TYPE_SPHERE: {
						drawSphere(obj->pos, geom->r, color);
					} break;
				}

				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
				glDisable(GL_CULL_FACE);

				Camera* cam = &ad->world.camera;
				float boundRadius = geom->boundingSphereRadius;
				Vec2 d = vec2(1,boundRadius);
				float alpha = 0.7f;

				glDisable(GL_DEPTH_TEST);
				{
					Texture* tex = getTexture(TEXTURE_ARROW);
					Vec3 n = cam->pos - obj->pos;
					Vec3 c[]   = { vec3(1,0,0), vec3(0,1,0), vec3(0,0,1) };
					Vec3 pp[]  = { obj->pos + vec3(1,0,0)*d.h*0.5f, obj->pos + vec3(0,1,0)*d.h*0.5f, obj->pos + vec3(0,0,1)*d.h*0.5f };
					Vec3 pn[]  = { normVec3(vec3(0, n.y, n.z)), normVec3(vec3(n.x, 0, n.z)), normVec3(vec3(n.x, n.y, 0)) };
					Vec3 pu[]  = { vec3(-1,0,0), vec3(0,-1,0), vec3(0,0,-1) };

					Vec3 rayDir = mouseRayCast(ad->textureScreenRect, input->mousePosNegative, cam);

					int selectionIndex = -1;
					for(int i = 0; i < 3; i++) {
						float a = alpha;
						float dist = linePlaneIntersection(cam->pos, rayDir, pp[i], pn[i], pu[i], d);
						if(dist != -1) {
							a = 1;
							selectionIndex = i;
						}

						drawPlane(pp[i], pn[i], pu[i], d, vec4(c[i],a), rect(0,0,1,1), tex->id);
					}

					ad->axisMoveModeHot = selectionIndex;
				}
				glEnable(GL_DEPTH_TEST);

				glEnable(GL_CULL_FACE);
				glEnable(GL_LIGHTING);
			}
			#endif

			// glEnable(GL_CULL_FACE);
			// glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}

		glMatrixMode(GL_PROJECTION); glPopMatrix();
		glMatrixMode(GL_MODELVIEW); glPopMatrix();
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_LIGHTING);
		glDisable(GL_NORMALIZE);
	}




	//@Draw Info.
	{
		Rect sr = getScreenRect(ws);
		glViewport(0,0, rectW(sr), rectH(sr));
		// glViewport(tr.left, -tr.top, rectW(tr), rectH(tr));

		// Rect tr = ad->textureScreenRect;		
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

	#endif

	openglDrawFrameBufferAndSwap(ws, systemData, &ad->swapTime, init);



	if(*isRunning == false) saveAppSettings(systemData);

	// if(init) printf("Startup Time: %fs\n", timerUpdate(startupTimer));

	// @App End.
}

#pragma optimize( "", on ) 
