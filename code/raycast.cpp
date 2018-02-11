
#define RAYTRACE_THREAD_JOB_COUNT 7*4


enum {
	GEOM_TYPE_SPHERE = 0,
	GEOM_TYPE_BOX,

	GEOM_TYPE_COUNT,
};

struct Material {
	float diffuseRatio;
	float specularRatio;

	float shininess;

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
			r = g->dim.x*0.5f + g->dim.y*0.5f + g->dim.z*0.5f;
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


enum SampleMode {
	SAMPLE_MODE_GRID = 0,
	SAMPLE_MODE_BLUE,
	SAMPLE_MODE_BLUE_MULTI,
	SAMPLE_MODE_MSAA4X,
	SAMPLE_MODE_MSAA8X,

	SAMPLE_MODE_COUNT,	
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

	int rayBouncesMax;

	int randomDirectionCount;
	Vec3* randomDirections;

	int sampleCount;
	Vec2* samples;

	int sampleGridWidth;
	int sampleGridCount; // width*width
	int* sampleGridOffsets;

	// Precalc.

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

#if 0
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

						Shape* s = world.objects + objectIndex;
						lastObjectIndex = objectIndex;


						// Color calculation.

						finalColor += attenuation * g->emitColor;
						attenuation = attenuation * g->color;
			
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

						randomDir = lerp(g->reflectionMod, randomDir, objectReflectionDir);

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
						Geometry* g = &obj->geometry;
						Material* m = &obj->material;
						lastObjectIndex = objectIndex;

						// Color calculation.

						// Vec3 gDir = -world.globalLightDir;
						// Vec3 gDiffuseColor = world.globalLightColor;
						// Vec3 gSpecularColor = world.globalLightColor;

						// Blinn-Phong.

						// float lightDot = dot(rayDir, -world.globalLightDir);

						Vec3 lightIntensity = {};

						Vec3 ambient = world.ambientRatio * world.ambientColor;

						for(int i = 0; i < world.lightCount; i++) {
							Light* l = world.lights + i;

							Vec3 diffuse = m->diffuseRatio * l->diffuseColor * clampMin(dot(objectReflectionNormal, -l->dir), 0);

							Vec3 halfwayVector = normVec3(-rayDir + -l->dir);
							Vec3 specular = l->specularColor * pow(clampMin(dot(objectReflectionNormal, halfwayVector), 0), m->shininess);

							lightIntensity += diffuse * specular;
						}

						finalColor = obj->color * (ambient + lightIntensity);

						// finalColor += obj->color * (gAmbientColor + diffuseColor + specularColor);

						// attenuation = attenuation * g->color;
						attenuation = black;
					
						if(attenuation == black) {
							endTimer(4);
							break;
						}

						break;

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
							// float lightDot = dot(rayDir, -world.globalLightDir);
							// lightDot = clampMin(lightDot, 0);
							// // lightDot = dotUnitToPercent(lightDot);
							// Vec3 light = world.globalLightColor * lightDot;

							// finalColor += attenuation * (world.defaultEmitColor + light);
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
			}
		}
	}

	return objectIndex;
}