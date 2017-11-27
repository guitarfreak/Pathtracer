
#include "external\pcg_basic.c"

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

bool lineShapeIntersection(Vec3 lp, Vec3 ld, Shape shape, Vec3* reflectionPos, Vec3* reflectionDir, Vec3* reflectionNormal) {

	switch(shape.type) {
		case SHAPE_BOX: {
			float distance;
			int face;
			bool hit = boxRaycast(lp, ld, rect3CenDim(shape.pos, shape.dim), &distance, &face);
			if(hit) {
				*reflectionPos = lp + ld*distance;
				Vec3 normal;
				if(face == 0) normal = vec3(-1,0,0);
				else if(face == 1) normal = vec3( 1, 0, 0);
				else if(face == 2) normal = vec3( 0,-1, 0);
				else if(face == 3) normal = vec3( 0, 1, 0);
				else if(face == 4) normal = vec3( 0, 0,-1);
				else if(face == 5) normal = vec3( 0, 0, 1);

				*reflectionDir = reflectVector(ld, normal);
				*reflectionNormal = normal;

				return true;
			}
		} break;

		case SHAPE_SPHERE: {
			bool hit = lineSphereIntersection(lp, lp + ld*1000, shape.pos, shape.r, reflectionPos);
			if(hit) {
				Vec3 normal = normVec3(*reflectionPos - shape.pos);
				*reflectionDir = reflectVector(ld, normal);
				*reflectionNormal = normal;

				return true;
			}
		}

		default: break;
	}

	return false;
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

void processPixel(World world, RaytraceSettings settings, int x, int y, Vec3* buffer) {
	// IACA_VC64_START;

	int sampleCount = settings.sampleCount;	
	Vec2* samples = settings.samples;
	Camera camera = world.camera;

	Vec3 black = vec3(0.0f);
	Vec3 white = vec3(1.0f);

	Vec2 percent = vec2(x/(float)settings.texDim.w, y/(float)settings.texDim.h);
	Vec3 finalColor = black;

	for(int i = 0; i < sampleCount; i++) {
		Vec3 rayPos = settings.camTopLeft;
		rayPos += camera.ovecs.right * (world.camera.dim.w*percent.x + settings.pixelPercent.w*samples[i].x);
		rayPos += -camera.ovecs.up   * (world.camera.dim.h*percent.y + settings.pixelPercent.h*samples[i].y);

		Vec3 rayDir = normVec3(rayPos - world.camera.pos);
	


		int rayIndex = 0;
		Vec3 attenuation = white;
		int lastShapeIndex = -1;
		for(;;) {

			// Find shape with closest intersection.

			Vec3 shapeReflectionPos, shapeReflectionDir, shapeReflectionNormal;
			int shapeIndex = -1;
			{
				float minDistance = FLT_MAX;
				for(int i = 0; i < world.shapeCount; i++) {
					if(lastShapeIndex == i) continue;

					Shape* s = world.shapes + i;

					Vec3 reflectionPos = black;
					Vec3 reflectionDir = black;
					Vec3 reflectionNormal = black;
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
			}

			if(shapeIndex != -1) {
				Shape* s = world.shapes + shapeIndex;
				lastShapeIndex = shapeIndex;

				finalColor += attenuation * s->emitColor;
				attenuation = attenuation * s->color;
	
				if(attenuation == black) break;

				int dirIndex = randomIntPCG(0, settings.randomDirectionCount-1);
				Vec3 randomDir = settings.randomDirections[dirIndex];

				// Reflect.
				float d = dot(randomDir, shapeReflectionNormal);
				if(d <= 0) randomDir = randomDir - 2*d*shapeReflectionNormal;

				// randomDir = normVec3(lerp(s->reflectionMod, randomDir, shapeReflectionDir));
				randomDir = lerp(s->reflectionMod, randomDir, shapeReflectionDir);

				rayPos = shapeReflectionPos;
				rayDir = randomDir;
	
				rayIndex++;
				if(rayIndex >= settings.rayBouncesMax) break;

			} else {

				if(rayIndex == 0) {
					// Sky hit.
					finalColor += world.defaultEmitColor;
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
		}
	}

	finalColor = finalColor/(float)sampleCount;
	finalColor = clampMax(finalColor, white);

	buffer[y*settings.texDim.w + x] = finalColor;

	// IACA_VC64_END;
}

struct ProcessPixelsData {
	World* world;
	RaytraceSettings* settings;

	Vec3* buffer;
	int pixelIndex;
	int pixelCount;

	bool stopProcessing;
};
	
void processPixelsThreaded(void* data) {
	ProcessPixelsData* d = (ProcessPixelsData*)data;

	Vec2i texDim = d->settings->texDim;
	int totalPixelCount = texDim.w * texDim.h;
	for(int i = d->pixelIndex; i < d->pixelIndex+d->pixelCount; i++) {

		if(d->stopProcessing) {
			d->stopProcessing = false;
			break;
		}
		
		if(i >= totalPixelCount) break;

		processPixel(*d->world, *d->settings, i % texDim.w, i / texDim.w, d->buffer);
	}
}