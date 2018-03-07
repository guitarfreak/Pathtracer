
#include "external\iacaMarks.h"

#define RAYTRACE_THREAD_JOB_COUNT 7*4


enum {
	GEOM_TYPE_SPHERE = 0,
	GEOM_TYPE_BOX,

	GEOM_TYPE_COUNT,
};

char* geometryTypeStrings[] = {
	"GEOM_TYPE_SPHERE",
	"GEOM_TYPE_BOX",
};

struct Material {
	// float diffuseRatio;
	// float specularRatio;
	// float shininess;

	Vec3 emitColor;
	float reflectionMod;
};

struct Geometry {
	int type;
	float boundingSphereRadius;

	union {
		// Sphere
		struct {
			float r;
		};

		// Box
		struct {
			Vec3 dim;
		};
	};
};

Vec3 geometryGetDim(Geometry* geom) {
	switch(geom->type) {
		case GEOM_TYPE_SPHERE: return vec3(geom->r*2);
		case GEOM_TYPE_BOX: return geom->dim;
	}
	return vec3(0,0,0);
}

void geometrySetDim(Geometry* geom, Vec3 dim) {
	switch(geom->type) {
		case GEOM_TYPE_SPHERE: geom->r = dim.x/2; return;
		case GEOM_TYPE_BOX: geom->dim = dim; return;
	}
}

void geometrySetDim(Geometry* geom, float length, int axisIndex) {
	switch(geom->type) {
		case GEOM_TYPE_SPHERE: geom->r = length/2; return;
		case GEOM_TYPE_BOX: geom->dim.e[axisIndex] = length; return;
	}
}

enum {
	LIGHT_TYPE_DIRECTION = 0,
	LIGHT_TYPE_POINT,
};

struct Light {
	int type;
	Vec3 diffuseColor;
	Vec3 specularColor;
	float brightness;

	union {
		Vec3 pos;
		Vec3 dir;
	};
};

struct Object {
	Vec3 pos;
	Quat rot;

	Vec3 color;
	Geometry geometry;
	Material material;
};

float lineShapeIntersection(Vec3 lp, Vec3 ld, Object* obj, Vec3* reflectionPos, Vec3* reflectionNormal) {

	float distance;
	Geometry* geometry = &obj->geometry;
	switch(geometry->type) {
		case GEOM_TYPE_BOX: {
			int face;
			bool hit = boxRaycast(lp, ld, rect3CenDim(obj->pos, geometry->dim), &distance, &face);
			if(hit) {
				*reflectionPos = lp + ld*distance;
				Vec3 normal;
				if(face == 0) normal = vec3(-1,0,0);
				else if(face == 1) normal = vec3( 1, 0, 0);
				else if(face == 2) normal = vec3( 0,-1, 0);
				else if(face == 3) normal = vec3( 0, 1, 0);
				else if(face == 4) normal = vec3( 0, 0,-1);
				else if(face == 5) normal = vec3( 0, 0, 1);

				*reflectionNormal = normal;

				return distance;
			}
		} break;

		case GEOM_TYPE_SPHERE: {
			distance = lineSphereIntersection(lp, ld, obj->pos, geometry->r, reflectionPos);

			if(distance > 0) {
				*reflectionNormal = normVec3(*reflectionPos - obj->pos);

				return distance;
			}
		}

		default: break;
	}

	return -1;
}

void geometryBoundingSphere(Geometry* g) {
	float r;
	switch(g->type) {
		case GEOM_TYPE_BOX: {
			r = lenVec3(g->dim)/2;
		} break;

		case GEOM_TYPE_SPHERE: {
			r = g->r;
		} break;
	}

	g->boundingSphereRadius = r;
}




struct OrientationVectors {
	Vec3 dir;
	Vec3 up;
	Vec3 right;
};

struct Camera {
	Vec3 pos;
	Vec3 rot;
	Vec2 dim;
	float nearDist;
	float farDist;
	float fov;
	OrientationVectors ovecs;
};

Vec3 vectorToCam(Vec3 pos, Camera* cam) {
	Vec3 intersection;
	float distance = linePlaneIntersection(pos, -cam->ovecs.dir, cam->pos, cam->ovecs.dir, &intersection);
	if(distance != -1) {
		return intersection - pos;
	}

	return vec3(0,0,0);
}


Vec3 getRotationFromVectors(OrientationVectors) {
	return {};
}

OrientationVectors getVectorsFromRotation(Vec3 rot) {

	OrientationVectors baseOrientation = {vec3(0,1,0), vec3(0,0,1), vec3(1,0,0)};
	OrientationVectors o = baseOrientation;

	Quat q = quat(rot.x, vec3(1,0,0)) * quat(rot.y, vec3(0,1,0)) * quat(rot.z, vec3(0,0,1));

	o.dir = normVec3(q*baseOrientation.dir);
	o.up = normVec3(q*baseOrientation.up);
	o.right = normVec3(q*baseOrientation.right);

	o.dir = baseOrientation.dir;
	rotateVec3(&o.dir, rot.x, baseOrientation.up);
	rotateVec3(&o.dir, rot.y, normVec3(cross(baseOrientation.up, o.dir)));
	o.up = normVec3(cross(o.dir, normVec3(cross(baseOrientation.up, o.dir))));
	o.right = -normVec3(cross(o.up, o.dir));

	return o;
}



Object defaultObject() {
	Material m = {};
	m.emitColor = vec3(0,0,0);
	m.reflectionMod = 0.5f;

	Object obj = {};
	obj.pos = vec3(0,0,0);
	obj.rot = quat();
	obj.color = vec3(0.5f,0.5f,0.5f);
	obj.material = m;
	obj.geometry.type = GEOM_TYPE_SPHERE;
	obj.geometry.r = 5;

	geometryBoundingSphere(&obj.geometry);
	
	return obj;
}

void deleteObject(Object* objects, int* objectCount, int* selected) {
	objects[(*selected)-1] = objects[(*objectCount)-1];
	(*objectCount)--;

	if((*objectCount) > 0) {
		(*selected) = mod((*selected)-1, (*objectCount));
		(*selected)++;
	} else {
		(*selected) = 0;
	}
}

void insertObject(Object obj, Object* objects, int* objectCount, Camera* cam) {
	float spawnDistance = 30;
	obj.pos = cam->pos + cam->ovecs.dir * spawnDistance;
	objects[(*objectCount)++] = obj;
}



enum SampleMode {
	SAMPLE_MODE_GRID = 0,
	SAMPLE_MODE_BLUE,
	SAMPLE_MODE_BLUE_MULTI,
	SAMPLE_MODE_MSAA4X,
	SAMPLE_MODE_MSAA8X,

	SAMPLE_MODE_COUNT,	
};

char* sampleModeStrings[] = {
	"GRID",
	"BLUE",
	"BLUE_MULTI",
	"MSAA4X",
	"MSAA8X",
};

struct World {
	Camera camera;

	Object* objects;
	int objectCount;

	Light* lights;
	int lightCount;

	float ambientRatio;
	Vec3 ambientColor;

	Vec3 defaultEmitColor;

	Vec3 globalLightDir;
	Vec3 globalLightColor;
};

enum {
	RENDERING_MODE_RAY_TRACER = 0, 
	RENDERING_MODE_PATH_TRACER, 
};

struct RaytraceSettings {
	int mode;
	Vec2i texDim;
	int sampleMode;
	int sampleCountGrid;
	int sampleGridWidth;
	int rayBouncesMax;

	// Precalc.

	int randomDirectionCount;
	Vec3* randomDirections;

	int sampleCount;
	Vec2* samples;

	// Blue noise.
	int sampleGridCount; // width*width
	int* sampleGridOffsets;

	Vec3 camTopLeft;
	Vec2 pixelPercent;
};

const Vec2 msaa4xPatternSamples[] = { 
	vec2(1/8.0f, 3/8.0f), 
	vec2(3/8.0f, 7/8.0f), 
	vec2(5/8.0f, 1/8.0f), 
	vec2(7/8.0f, 5/8.0f), 
};

const Vec2 msaa8xPatternSamples[] = { 
	vec2( 1/16.0f, 11/16.0f), 
	vec2( 3/16.0f,  3/16.0f), 
	vec2( 5/16.0f, 15/16.0f), 
	vec2( 7/16.0f,  7/16.0f), 
	vec2( 9/16.0f,  1/16.0f), 
	vec2(11/16.0f,  9/16.0f), 
	vec2(13/16.0f, 13/16.0f), 
	vec2(15/16.0f,  5/16.0f), 
};

struct ProcessPixelsData {
	World* world;
	RaytraceSettings* settings;

	Vec3* buffer;
	int pixelIndex;
	int pixelCount;

	bool stopProcessing;
};

struct TimeStamp {
	i64 cycleStart;
	i64 cycles;
	i64 hits;
	i64 cyclesOverHits;
};
void startTimeStamp(TimeStamp* t) { 
	t->cycleStart = getCycleStamp(); 
}
void endTimeStamp(TimeStamp* t) { 
	t->cycles += getCycleStamp() - t->cycleStart;
	t->hits++;
}
void calcTimeStamp(TimeStamp* t) {
	if(t->hits == 0) t->cyclesOverHits = 0;
	else t->cyclesOverHits = t->cycles / t->hits;
}

TimeStamp processPixelsThreadedTimings[5] = {};

#define printRaytraceTimings false

#ifdef printRaytraceTimings
#define startTimer(i) startTimeStamp(pixelTimings + i)
#define endTimer(i) endTimeStamp(pixelTimings + i)
#else
#define startTimer(i)
#define endTimer(i)
#endif

void processPixelsThreaded(void* data) {
	TimeStamp pixelTimings[5] = {};

	ProcessPixelsData* d = (ProcessPixelsData*)data;

	World world = *d->world;
	RaytraceSettings settings = *d->settings;	
	Vec3* buffer = d->buffer;

	int sampleCount = settings.sampleCount;	
	Vec2* samples = settings.samples;
	Camera camera = world.camera;

	Vec3 black = vec3(0.0f);
	Vec3 white = vec3(1.0f);

	Vec2i texDim = settings.texDim;
	int totalPixelCount = texDim.w * texDim.h;
	int pixelRangeEnd = d->pixelIndex+d->pixelCount;
	for(int pixelIndex = d->pixelIndex; pixelIndex < pixelRangeEnd; pixelIndex++) {
		startTimer(0);

		if(d->stopProcessing) {
			d->stopProcessing = false;
			break;
		}
		
		int x = pixelIndex % texDim.w;
		int y = pixelIndex / texDim.w;

		if(settings.sampleMode == SAMPLE_MODE_BLUE_MULTI) {
			int index = (y%settings.sampleGridWidth)*settings.sampleGridWidth + (x%settings.sampleGridWidth);
			int offset = settings.sampleGridOffsets[index];
			samples = settings.samples + offset;
			sampleCount = settings.sampleGridOffsets[index+1] - offset;
		}

		{
			// IACA_VC64_START;

			Vec2 percent = vec2(x/(float)texDim.w, y/(float)texDim.h);
			Vec3 finalColor = black;

			for(int sampleIndex = 0; sampleIndex < sampleCount; sampleIndex++) {
				startTimer(1);

				Vec3 rayPos = settings.camTopLeft;
				rayPos += camera.ovecs.right * (camera.dim.w * (percent.w + settings.pixelPercent.w*samples[sampleIndex].x));
				rayPos -= camera.ovecs.up  * (camera.dim.h * (percent.h + settings.pixelPercent.h*samples[sampleIndex].y));

				Vec3 rayDir = normVec3(rayPos - camera.pos);

				Vec3 attenuation = white;
				int lastObjectIndex = -1;
				for(int rayIndex = 0; rayIndex < settings.rayBouncesMax; rayIndex++) {
					startTimer(2);

					// Find shape with closest intersection.

					Vec3 objectReflectionPos, objectReflectionDir, objectReflectionNormal;
					int objectIndex = -1;
					{
						startTimer(3);

						float minDistance = FLT_MAX;
						for(int i = 0; i < world.objectCount; i++) {
							if(lastObjectIndex == i) continue;

							Object* obj = world.objects + i;
							Geometry* g = &obj->geometry;

							// Check collision with bounding sphere.
							bool possibleIntersection = lineSphereCollision(rayPos, rayDir, obj->pos, g->boundingSphereRadius);
							if(possibleIntersection) {

								Vec3 reflectionPos, reflectionNormal;
								float distance = -1;
								{
									switch(g->type) {
										case GEOM_TYPE_BOX: {
											int face;
											bool hit = boxRaycast(rayPos, rayDir, rect3CenDim(obj->pos, g->dim), &distance, &face);
											if(hit) {
												reflectionPos = rayPos + rayDir*distance;
												reflectionNormal = boxRaycastNormals[face];
											}
										} break;

										case GEOM_TYPE_SPHERE: {
											distance = lineSphereIntersection(rayPos, rayDir, obj->pos, g->r, &reflectionPos);
											if(distance > 0) {
												reflectionNormal = normVec3(reflectionPos - obj->pos);
											}
										} break;
									}
								}

								if(distance > 0 && distance < minDistance) {
									minDistance = distance;
									objectIndex = i;

									objectReflectionPos = reflectionPos;
									objectReflectionNormal = reflectionNormal;
								}
							}
						}

						endTimer(3);
					}


					if(objectIndex != -1) {
						startTimer(4);

						Object* obj = world.objects + objectIndex;
						// Geometry* g = &world.objects[objectIndex].geometry;
						Material* m = &obj->material;
						lastObjectIndex = objectIndex;


						// Color calculation.

						finalColor += attenuation * m->emitColor;
						attenuation = attenuation * obj->color;
			
						if(attenuation == black) {
							endTimer(4);
							break;
						}


						// Calculate new direction.

						int dirIndex = randomIntPCG(0, settings.randomDirectionCount-1);
						Vec3 randomDir = settings.randomDirections[dirIndex];

						// Reflect.
						float d = dot(randomDir, objectReflectionNormal);
						if(d <= 0) randomDir = reflectVector(randomDir, objectReflectionNormal);

						Vec3 objectReflectionDir = reflectVector(rayDir, objectReflectionNormal);

						randomDir = lerp(m->reflectionMod, randomDir, objectReflectionDir);

						rayPos = objectReflectionPos;
						rayDir = randomDir;


						endTimer(4);
					} else {

						if(rayIndex == 0) {
							finalColor += world.defaultEmitColor; // Sky hit.
						} else {
							float lightDot = dot(rayDir, -world.globalLightDir);
							lightDot = clampMin(lightDot, 0);
							// lightDot = dotUnitToPercent(lightDot);
							Vec3 light = world.globalLightColor * lightDot;

							finalColor += attenuation * (world.defaultEmitColor + light);
						}

						break;
					}



					endTimer(2);
				}

				endTimer(1);
			}

			finalColor = finalColor/(float)sampleCount;
			finalColor = clampMax(finalColor, white);

			buffer[y*texDim.w + x] = finalColor;

			// IACA_VC64_END;
		}

		endTimer(0);
	}

	if(printRaytraceTimings) {
		for(int i = 0; i < arrayCount(pixelTimings); i++) {
			processPixelsThreadedTimings[i].cycles += pixelTimings[i].cycles;
			processPixelsThreadedTimings[i].hits += pixelTimings[i].hits;
		}
	}
}

// @Duplication with processPixels.
int castRay(Vec3 rayPos, Vec3 rayDir, Object* objects, int objectCount) {

	int objectIndex = -1;

	float minDistance = FLT_MAX;
	for(int i = 0; i < objectCount; i++) {
		Object* obj = objects + i;
		Geometry* g = &obj->geometry;

		// Check collision with bounding sphere.
		bool possibleIntersection = lineSphereCollision(rayPos, rayDir, obj->pos, g->boundingSphereRadius);
		if(possibleIntersection) {

			Vec3 reflectionPos, reflectionNormal;
			float distance = -1;
			{
				switch(g->type) {
					case GEOM_TYPE_BOX: {
						int face;
						// bool hit = boxRaycast(rayPos, rayDir, rect3CenDim(obj->pos, g->dim), &distance, &face);
						// if(hit) {
						// 	reflectionPos = rayPos + rayDir*distance;
						// 	reflectionNormal = boxRaycastNormals[face];
						// }

						Vec3 intersection;
						bool hit = boxRaycastRotated(rayPos, rayDir, obj->pos, g->dim, obj->rot, &intersection, &face);
						if(hit) {
							reflectionPos = intersection;
							reflectionNormal = boxRaycastNormals[face];
							distance = lenVec3(intersection - rayPos);
						}

					} break;

					case GEOM_TYPE_SPHERE: {
						distance = lineSphereIntersection(rayPos, rayDir, obj->pos, g->r, &reflectionPos);
						if(distance > 0) {
							reflectionNormal = normVec3(reflectionPos - obj->pos);
						}
					} break;
				}
			}

			if(distance > 0 && distance < minDistance) {
				minDistance = distance;
				objectIndex = i;
			}
		}
	}

	return objectIndex;
}




void getDefaultScene(World* world) {
	float zLevel = 7;

	Camera* cam = &world->camera;
	cam->pos = vec3(0, -50, zLevel);
	cam->rot = vec3(0, 0, 0);
	cam->fov = 90;
	cam->dim.w = 10;
	cam->farDist = 10000;


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
	materials[0].emitColor = vec3(0,0,0);
	materials[0].reflectionMod = 0.8f;

	materials[1].emitColor = vec3(0,0,0);
	materials[1].reflectionMod = 0.2f;

	Object obj;

	// Ground plane.

	obj = {};
	obj.pos = vec3(0,0,0);
	obj.rot = quat();
	obj.color = vec3(0.5f);
	obj.material = materials[0];
	obj.geometry.type = GEOM_TYPE_BOX;
	// obj.geometry.dim = vec3(100000, 100000, 0.01f);
	obj.geometry.dim = vec3(50, 50, 0.01f);
	world->objects[world->objectCount++] = obj;

	// Sphere.

	obj = {};
	obj.pos = vec3(0,0,zLevel);
	obj.rot = quat();
	obj.color = vec3(0.3f,0.5f,0.8f);
	obj.material = materials[1];
	obj.geometry.type = GEOM_TYPE_SPHERE;
	obj.geometry.r = 5;
	world->objects[world->objectCount++] = obj;
	
	// Calc bounding spheres.
	for(int i = 0; i < world->objectCount; i++) {
		geometryBoundingSphere(&world->objects[i].geometry);
	}
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
	Object objectCopy;

	int selectedObject;

	int selectionMode;
	int selectionState;
	bool gotActive;
	int hotId;

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


void saveScene(World* world, char* filePath) {
	FILE* file = fopen(filePath, "wb");

	if(file) {
		fwrite(world, sizeof(World), 1, file);

		fwrite(world->objects, sizeof(Object)*world->objectCount, 1, file);
		fwrite(world->lights, sizeof(Light)*world->lightCount, 1, file);
	}

	fclose(file);
}

void loadScene(World* world, char* filePath) {
	FILE* file = fopen(filePath, "rb");

	if(file) {
		fread(world, sizeof(World), 1, file);

		world->objects = mallocArray(Object, world->objectCount);
		fread(world->objects, sizeof(Object)*world->objectCount, 1, file);

		world->lights = mallocArray(Light, world->lightCount);
		fread(world->lights, sizeof(Object)*world->lightCount, 1, file);
	}

	fclose(file);
}

char* openSceneDialog(bool saveMode = false) {
	int filePathSize = 200;
	char* filePath = getTString(filePathSize);
	char* fileName = getTString(filePathSize);
	strClear(filePath);

	if(saveMode) strCpy(filePath, ".scene");

	OPENFILENAME ofn = {};

	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = NULL;
	ofn.lpstrFile = filePath;
	ofn.nMaxFile = filePathSize;
	ofn.lpstrFileTitle = fileName;
	ofn.nMaxFileTitle = filePathSize;

	ofn.lpstrInitialDir = Scenes_Folder;

	ofn.lpstrFilter = "Scene Files\0*.scene\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrDefExt = ".scene";

	ofn.Flags = OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR;

	if(saveMode) ofn.Flags |= OFN_OVERWRITEPROMPT;
	else ofn.Flags |= OFN_FILEMUSTEXIST;

	int error;
	if(saveMode) error = GetSaveFileName(&ofn);	
	else error = GetOpenFileName(&ofn);
	
	if(error == 0) return "";

	return ofn.lpstrFile;
}



void openSceneCommand(World* world, char* sceneFile, bool* sceneHasFile) {
	char* filePath = openSceneDialog();
	if(strLen(filePath)) {
		loadScene(world, filePath);
		(*sceneHasFile) = true;
		strCpy(sceneFile, filePath);
	}
}

void saveAsSceneCommand(World* world, char* sceneFile, bool* sceneHasFile) {
	char* filePath = openSceneDialog(true);
	if(strLen(filePath)) {
		saveScene(world, filePath);
		(*sceneHasFile) = true;
		strCpy(sceneFile, filePath);
	}
}

void saveSceneCommand(World* world, char* sceneFile, bool* sceneHasFile) {
	if((*sceneHasFile)) saveScene(world, sceneFile);
	else {
		saveAsSceneCommand(world, sceneFile, sceneHasFile);
	}
} 