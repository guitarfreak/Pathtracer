
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

struct Camera {
	Vec3 pos;
	Vec3 rot;
	Vec2 dim;
	float dist;
	// float fov;
};

struct Orientation {
	Vec3 dir;
	Vec3 up;
	Vec3 right;
};

Vec3 getRotationFromVectors(Orientation) {
	return {};
}

Orientation getVectorsFromRotation(Vec3 rot) {

	Orientation baseOrientation = {vec3(0,1,0), vec3(0,0,1), vec3(1,0,0)};
	Orientation o = baseOrientation;

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
	int sampleCount;

	int rayBouncesMax;

	int randomDirectionCount;
	Vec3* randomDirections;
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

	Vec2 samples[32*32];
	int mode = settings.sampleMode;
	int sampleCount;
	switch(mode) {
		case SAMPLE_MODE_GRID: {
			int sampleCount2 = settings.sampleCount;
			sampleCount = sampleCount2*sampleCount2;
			for(int i = 0; i < sampleCount; i++) {
				samples[i] = vec2(((i%sampleCount2)*sampleCount2 + 1) / (float)sampleCount, 
				                  ((i/sampleCount2)*sampleCount2 + 1) / (float)sampleCount);
			}
		} break;

		case SAMPLE_MODE_MSAA4X: {
			sampleCount = 4;
			for(int i = 0; i < sampleCount; i++) samples[i] = msaa4xPatternSamples[i];
		} break;

		case SAMPLE_MODE_MSAA8X: {
			sampleCount = 8;
			for(int i = 0; i < sampleCount; i++) samples[i] = msaa8xPatternSamples[i];
		} break;
	}

	Camera camera = world.camera;
	Orientation camRot = getVectorsFromRotation(camera.rot);
	Vec3 camTopLeft = camera.pos + camRot.dir*camera.dist + (camRot.right*-1)*(camera.dim.w/2.0f) + (camRot.up)*(camera.dim.h/2.0f);
	Vec2 percent = vec2(x/(float)settings.texDim.w, y/(float)settings.texDim.h);
	Vec2 pixelPercent = vec2(1/(float)settings.texDim.w, 1/(float)settings.texDim.h);

	Vec3 finalColor = vec3(0,0,0);
	for(int i = 0; i < sampleCount; i++) {
		Vec3 p = camTopLeft;
		p += camRot.right * (camera.dim.w*percent.x + pixelPercent.w*samples[i].x);
		p += -camRot.up   * (camera.dim.h*percent.y + pixelPercent.h*samples[i].y);

		// finalColor += castRay(world, vec3(1,1,1), camera.pos, normVec3(p - camera.pos), 0, rayMaxCount, -1);

		// Cast rays.
	
		int rayIndex = 0;
		Vec3 attenuation = vec3(1,1,1);
		Vec3 rayPos = p;
		Vec3 rayDir = normVec3(p - camera.pos);
		int lastShapeIndex = -1;
		for(;;) {
			if(rayIndex >= settings.rayBouncesMax) break;
			if(attenuation == vec3(0,0,0)) break;

			// Find shape with closest intersection.

			Vec3 shapeReflectionPos, shapeReflectionDir, shapeReflectionNormal;
			int shapeIndex = -1;
			{
				float minDistance = FLT_MAX;
				for(int i = 0; i < world.shapeCount; i++) {
					if(lastShapeIndex == i) continue;

					Shape* s = world.shapes + i;

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
			}

			if(shapeIndex != -1) {
				Shape* s = world.shapes + shapeIndex;
				lastShapeIndex = shapeIndex;

				finalColor += attenuation * s->emitColor;
				attenuation = attenuation * s->color;

				int dirIndex = randomIntPCG(0, settings.randomDirectionCount-1);
				Vec3 randomDir = settings.randomDirections[dirIndex];

				if(dot(randomDir, shapeReflectionNormal) <= 0) randomDir = reflectVector(randomDir, shapeReflectionNormal);

				randomDir.x = lerp(s->reflectionMod, randomDir.x, shapeReflectionDir.x);
				randomDir.y = lerp(s->reflectionMod, randomDir.y, shapeReflectionDir.y);
				randomDir.z = lerp(s->reflectionMod, randomDir.z, shapeReflectionDir.z);
				randomDir = normVec3(randomDir);

				rayPos = shapeReflectionPos;
				rayDir = randomDir;
				rayIndex++;

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

	clampMax(&finalColor.r, 1);
	clampMax(&finalColor.g, 1);
	clampMax(&finalColor.b, 1);

	buffer[y*settings.texDim.w + x] = finalColor;
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

		int y = i / texDim.w;
		int x = i % texDim.w;

		processPixel(*d->world, *d->settings, x, y, d->buffer);
	}

}