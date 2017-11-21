/*
=================================================================================

	ToDo:
	- Blue noise.

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

	// 

	Texture raycastTexture;
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
		// float th = 
		Vec2i texSize = vec2i(1920,1080);
		// Vec2i texSize = vec2i(1280,720);
		// Vec2i texSize = vec2i(1280/2,720/2);
		// Vec2i texSize = vec2i(320,180);
		// Vec2i texSize = vec2i(160,90);
		float aspectRatio = (float)texSize.w / texSize.h;

		if(init) {
			Texture* tex = &ad->raycastTexture;
			*tex = {};

			int mipLevels = 1;
			int internalFormat = INTERNAL_TEXTURE_FORMAT;

			int w = texSize.w;
			int h = texSize.h;

			glCreateTextures(GL_TEXTURE_2D, 1, &tex->id);
			glTextureStorage2D(tex->id, mipLevels, internalFormat, w, h);

			tex->dim = vec2i(w,h);
			tex->channels = 4;
			tex->levels = mipLevels;

			glTextureParameteri(tex->id, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTextureParameteri(tex->id, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

			// glTextureParameteri(tex->id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			// glTextureParameteri(tex->id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			glTextureParameteri(tex->id, GL_TEXTURE_WRAP_S, GL_CLAMP);
			glTextureParameteri(tex->id, GL_TEXTURE_WRAP_T, GL_CLAMP);

			glGenerateTextureMipmap(tex->id);
		} 

		if(input->keysPressed[KEYCODE_SPACE] || reload || init) {
		// if(true) {

			// Vec3 camRot
			Vec3 camPos = vec3(0, -20, 4);
			Vec3 camDir = normVec3(vec3(0, 1, 0));
			// float camFov = 90;
			// float camAspect = (float)texSize.w / (float)texSize.h;
			float camDist = 1;


			int shapeCount = 0;
			Shape* shapes = getTArray(Shape, 10);

			Shape s;

			s = {};
			s.type = SHAPE_BOX;
			s.pos = vec3(0,0,0);
			// s.dim = vec3(12,12,1);
			s.dim = vec3(10000,10000,0.01f);
			s.color = vec3(0.5f);
			s.reflectionMod = 0.5f;
			shapes[shapeCount++] = s;

			s = {};
			s.type = SHAPE_BOX;
			s.pos = vec3(-15,2,0);
			s.dim = vec3(1,10,50);
			s.color = vec3(0,0.8f,0.5f);
			s.reflectionMod = 0.5f;
			shapes[shapeCount++] = s;

			s = {};
			s.type = SHAPE_BOX;
			s.pos = vec3(0,-10,1);
			s.dim = vec3(2,2,2);
			s.color = vec3(0.8f,0.3f,0.5f);
			s.reflectionMod = 0.9f;
			shapes[shapeCount++] = s;



			s = {};
			s.type = SHAPE_SPHERE;
			float animSpeed = 0.5f;
			// s.pos = vec3(6*sin(ad->time*animSpeed), 3*cos(ad->time*animSpeed) , 7 + 1*cos(ad->time*animSpeed*0.5f));
			s.pos = vec3(8, -2, 1);
			s.r = 4;
			s.color = vec3(0.3f,0.5f,0.8f);
			s.reflectionMod = 0.9f;
			shapes[shapeCount++] = s;

			s = {};
			s.type = SHAPE_SPHERE;
			s.pos = vec3(-8,0,1);
			s.r = 2;
			s.color = vec3(0.0f);
			s.emitColor = vec3(2,0,0);
			s.reflectionMod = 1;
			shapes[shapeCount++] = s;

			s = {};
			s.type = SHAPE_SPHERE;
			s.pos = vec3(-2,20,10);
			s.r = 8;
			s.color = vec3(0.5f);
			s.emitColor = vec3(0.0f);
			s.reflectionMod = 1;
			shapes[shapeCount++] = s;


			Vec3 defaultEmitColor = vec3(0.5f, 0.8f, 0.9f);
			// Vec3 defaultEmitColor = vec3(0.95f);
			// Vec3 defaultEmitColor = vec3(0.0f);
			// Vec3 globalLightDir = normVec3(vec3(1,1,-1));
			Vec3 globalLightDir = normVec3(vec3(-1,0,-1));
			Vec3 globalLightColor = vec3(1,1,1);



			Texture* tex = &ad->raycastTexture;

			Vec2i texDim = tex->dim;
			Vec3* data = (Vec3*)malloc(texDim.w * texDim.h * sizeof(Vec3));



			float pixelPercent = (float)1/texDim.w;

			float camWidth = camDist * 2; // 90 degrees for now.
			Vec2 camDim = vec2(camWidth, camWidth*(1/aspectRatio)); 

			Vec3 camRight = normVec3(vec3(1,0,0));
			Vec3 camUp = normVec3(vec3(0,0,1));
			Vec3 camLeft = camRight * -1;

			for(int y = 0; y < texDim.h; y++) {
				for(int x = 0; x < texDim.w; x++) {

					float xPercent = x/((float)texDim.w-1);
					float yPercent = y/((float)texDim.h-1);
					Vec3 p = camPos + camDir*camDist;

					p += camLeft * -((camDim.w*(xPercent + (pixelPercent*0.5f + pixelPercent*randomFloat(-0.5f,0.5f,0.0001f)))) - camDim.w*0.5f);
					p += camUp * -((camDim.h*(yPercent + (pixelPercent*0.5f + pixelPercent*randomFloat(-0.5f,0.5f,0.0001f)))) - camDim.h*0.5f);

					Vec3 rayPos = camPos;
					Vec3 rayDir = normVec3(p - camPos);

					Vec3 finalColor = vec3(0,0,0);
					Vec3 attenuation = vec3(1,1,1);

					int rayIndex = 0;
					int rayMaxCount = 10;

					int lastShapeIndex = -1;

					for(;;) {

						// Find shape with closest intersection.

						Vec3 shapeReflectionPos;
						Vec3 shapeReflectionDir;
						Vec3 shapeReflectionNormal;

						int shapeIndex = -1;
						float minDistance = FLT_MAX;
						for(int i = 0; i < shapeCount; i++) {
							if(lastShapeIndex == i) continue;

							Shape* s = shapes + i;

							Vec3 reflectionPos = vec3(0,0,0);
							Vec3 reflectionDir = vec3(0,0,0);
							Vec3 reflectionNormal = vec3(0,0,0);
							bool intersection = lineShapeIntersection(rayPos, rayDir, *s, &reflectionPos, &reflectionDir, &reflectionNormal);

							if(intersection) {
								float distance = lenVec3(reflectionPos - rayPos);
								if(distance < minDistance) {
									minDistance = distance;
									shapeIndex = i;

									shapeReflectionPos = reflectionPos;
									shapeReflectionDir = reflectionDir;
									shapeReflectionNormal = reflectionNormal;
								}
							}
						}

						// if(x > 600 && y == 670) {
						// 	// finalColor = vec3(1,0,0);
						// 	// break;

						// 	int stop = 234;
						// }

						if(shapeIndex != -1) {
							Shape* s = shapes + shapeIndex;
							lastShapeIndex = shapeIndex;

							finalColor += attenuation * s->emitColor;
							attenuation = attenuation * s->color;

							rayPos = shapeReflectionPos;

							Vec3 randomVec = vec3(randomFloat(0,1,0.01f), randomFloat(0,1,0.01f), randomFloat(0,1,0.01f));
							Vec3 randomDir = normVec3(shapeReflectionNormal + randomVec);
							rayDir.x = lerp(s->reflectionMod, randomDir.x, shapeReflectionDir.x);
							rayDir.y = lerp(s->reflectionMod, randomDir.y, shapeReflectionDir.y);
							rayDir.z = lerp(s->reflectionMod, randomDir.z, shapeReflectionDir.z);
							rayDir = normVec3(rayDir);

							// rayDir = shapeReflectionDir;

							// if(rayIndex == 1) {
								// finalColor = vec3(abs(rayDir.x));
								// break;
							// }

							// float test = dot(rayDir, -globalLightDir);
							// finalColor = vec3(test);
							// break;

							if(attenuation == vec3(0,0,0)) break;

						} else {
							// Sky hit.
							if(rayIndex == 0) {
								finalColor = defaultEmitColor;
							} else {
								// finalColor += attenuation * defaultEmitColor;

								Vec3 light = globalLightColor * dot(rayDir, -globalLightDir);
								// finalColor += attenuation * (defaultEmitColor + light);
								finalColor += attenuation * (light);
							}

							break;
						}

						rayIndex++;
						if(rayIndex >= rayMaxCount) break;
					}


					data[y*texDim.w + x] = finalColor;
				}
			}

			glTextureSubImage2D(tex->id, 0, 0, 0, texDim.w, texDim.h, GL_RGB, GL_FLOAT, data);

			free(data);
		}


		{
			Vec2 texDim = vec2(ad->raycastTexture.dim);

			Rect tr = sr;
			Vec2 sd = rectDim(sr);
			if(((float)texDim.h / texDim.w) > (sd.h / sd.w)) {
				tr = rectSetW(tr, ((float)texDim.w / texDim.h)*sd.h);
			} else {
				tr = rectSetH(tr, ((float)texDim.h / texDim.w)*sd.w);
			}
			// tr = rectExpand(tr, vec2(-50));
			drawRect(tr, rect(0,0,1,1), ad->raycastTexture.id);
		}

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
