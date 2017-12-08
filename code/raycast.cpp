
enum {
	SHAPE_BOX = 0,
	SHAPE_SPHERE,

	SHAPE_COUNT,
};

struct Shape {
	int type;

	Vec3 pos;
	Quat rot;

	Vec3 color;
	Vec3 emitColor;
	float reflectionMod;

	float boundingSphereRadius;

	union {
		// Box
		struct {
			Vec3 dim;
		};

		// Sphere
		struct {
			float r;
		};
	};
};

float lineShapeIntersection(Vec3 lp, Vec3 ld, Shape* shape, Vec3* reflectionPos, Vec3* reflectionNormal) {

	float distance;
	switch(shape->type) {
		case SHAPE_BOX: {
			int face;
			bool hit = boxRaycast(lp, ld, rect3CenDim(shape->pos, shape->dim), &distance, &face);
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

		case SHAPE_SPHERE: {
			distance = lineSphereIntersection(lp, ld, shape->pos, shape->r, reflectionPos);

			if(distance > 0) {
				*reflectionNormal = normVec3(*reflectionPos - shape->pos);

				return distance;
			}
		}

		default: break;
	}

	return -1;
}

void shapeBoundingSphere(Shape* s) {
	float r;
	switch(s->type) {
		case SHAPE_BOX: {
			r = lenVec3(s->dim);
		} break;

		case SHAPE_SPHERE: {
			r = s->r;
		} break;
	}

	s->boundingSphereRadius = r;
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
	Shape* shapes;
	int shapeCount;

	Vec3 defaultEmitColor;
	Vec3 globalLightColor;
	Vec3 globalLightDir;
};

struct RaytraceSettings {
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
				int lastShapeIndex = -1;
				for(int rayIndex = 0; rayIndex < settings.rayBouncesMax; rayIndex++) {
					startTimer(2);

					// Find shape with closest intersection.

					Vec3 shapeReflectionPos, shapeReflectionDir, shapeReflectionNormal;
					int shapeIndex = -1;
					{
						startTimer(3);

						float minDistance = FLT_MAX;
						for(int i = 0; i < world.shapeCount; i++) {
							if(lastShapeIndex == i) continue;

							Shape* s = world.shapes + i;

							// Check collision with bounding sphere.
							bool possibleIntersection = lineSphereCollision(rayPos, rayDir, s->pos, s->boundingSphereRadius);
							if(possibleIntersection) {

								Vec3 reflectionPos, reflectionNormal;
								float distance = -1;
								{
									switch(s->type) {
										case SHAPE_BOX: {
											int face;
											bool hit = boxRaycast(rayPos, rayDir, rect3CenDim(s->pos, s->dim), &distance, &face);
											if(hit) {
												reflectionPos = rayPos + rayDir*distance;
												reflectionNormal = boxRaycastNormals[face];
											}
										} break;

										case SHAPE_SPHERE: {
											distance = lineSphereIntersection(rayPos, rayDir, s->pos, s->r, &reflectionPos);
											if(distance > 0) {
												reflectionNormal = normVec3(reflectionPos - s->pos);
											}
										} break;
									}
								}

								if(distance > 0 && distance < minDistance) {
									minDistance = distance;
									shapeIndex = i;

									shapeReflectionPos = reflectionPos;
									shapeReflectionNormal = reflectionNormal;
								}
							}
						}

						endTimer(3);
					}

					if(shapeIndex != -1) {
						startTimer(4);

						Shape* s = world.shapes + shapeIndex;
						lastShapeIndex = shapeIndex;

						finalColor += attenuation * s->emitColor;
						attenuation = attenuation * s->color;
			
						if(attenuation == black) {
							endTimer(4);
							break;
						}

						int dirIndex = randomIntPCG(0, settings.randomDirectionCount-1);
						Vec3 randomDir = settings.randomDirections[dirIndex];

						// Reflect.
						float d = dot(randomDir, shapeReflectionNormal);
						if(d <= 0) randomDir = reflectVector(randomDir, shapeReflectionNormal);

						Vec3 shapeReflectionDir = reflectVector(rayDir, shapeReflectionNormal);

						// randomDir = normVec3(lerp(s->reflectionMod, randomDir, shapeReflectionDir));
						randomDir = lerp(s->reflectionMod, randomDir, shapeReflectionDir);

						rayPos = shapeReflectionPos;
						rayDir = randomDir;

						endTimer(4);
					} else {

						if(rayIndex == 0) {
							finalColor += world.defaultEmitColor; // Sky hit.
						} else {
							// finalColor += attenuation * defaultEmitColor;

							float lightDot = dot(rayDir, -world.globalLightDir);
							lightDot = clampMin(lightDot, 0);
							lightDot = dotUnitToPercent(lightDot);
							Vec3 light = world.globalLightColor * lightDot;

							finalColor += attenuation * (world.defaultEmitColor + light);
							// finalColor += attenuation * light;
							// finalColor += attenuation * world->defaultEmitColor;
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
int castRay(Vec3 rayPos, Vec3 rayDir, Shape* shapes, int shapeCount) {

	int shapeIndex = -1;

	float minDistance = FLT_MAX;
	for(int i = 0; i < shapeCount; i++) {
		Shape* s = shapes + i;

		// Check collision with bounding sphere.
		bool possibleIntersection = lineSphereCollision(rayPos, rayDir, s->pos, s->boundingSphereRadius);
		if(possibleIntersection) {

			Vec3 reflectionPos, reflectionNormal;
			float distance = -1;
			{
				switch(s->type) {
					case SHAPE_BOX: {
						int face;
						bool hit = boxRaycast(rayPos, rayDir, rect3CenDim(s->pos, s->dim), &distance, &face);
						if(hit) {
							reflectionPos = rayPos + rayDir*distance;
							reflectionNormal = boxRaycastNormals[face];
						}
					} break;

					case SHAPE_SPHERE: {
						distance = lineSphereIntersection(rayPos, rayDir, s->pos, s->r, &reflectionPos);
						if(distance > 0) {
							reflectionNormal = normVec3(reflectionPos - s->pos);
						}
					} break;
				}
			}

			if(distance > 0 && distance < minDistance) {
				minDistance = distance;
				shapeIndex = i;
			}
		}
	}

	return shapeIndex;
}