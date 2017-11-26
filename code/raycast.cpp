
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

struct World {
	Camera camera;
	Shape* shapes;
	int shapeCount;

	Vec3 defaultEmitColor;
	Vec3 globalLightColor;
	Vec3 globalLightDir;
};

Vec3 castRay(World* world, Vec3 attenuation, Vec3 rayPos, Vec3 rayDir, int rayIndex, int rayMaxCount, int lastShapeIndex) {

	if(rayIndex >= rayMaxCount) return vec3(0,0,0);
	if(attenuation == vec3(0,0,0)) return vec3(0,0,0);

	Vec3 finalColor = vec3(0,0,0);

	// Find shape with closest intersection.

	Vec3 shapeReflectionPos;
	Vec3 shapeReflectionDir;
	Vec3 shapeReflectionNormal;

	int shapeIndex = -1;
	float minDistance = FLT_MAX;
	for(int i = 0; i < world->shapeCount; i++) {
		if(lastShapeIndex == i) continue;

		Shape* s = world->shapes + i;

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

	if(shapeIndex != -1) {
		Shape* s = world->shapes + shapeIndex;
		lastShapeIndex = shapeIndex;

		finalColor += attenuation * s->emitColor;
		attenuation = attenuation * s->color;

			// rayPos = shapeReflectionPos;

			// Vec3 randomDir = normVec3(vec3(randomFloat(-1,1,0.01f), randomFloat(-1,1,0.01f), randomFloat(-1,1,0.01f)));
			// if(dot(randomDir, shapeReflectionNormal) <= 0) randomDir = reflectVector(randomDir, shapeReflectionNormal);

			// rayDir.x = lerp(s->reflectionMod, randomDir.x, shapeReflectionDir.x);
			// rayDir.y = lerp(s->reflectionMod, randomDir.y, shapeReflectionDir.y);
			// rayDir.z = lerp(s->reflectionMod, randomDir.z, shapeReflectionDir.z);
			// rayDir = normVec3(rayDir);

			// finalColor += castRay(world, attenuation, rayPos, rayDir, rayIndex + 1, rayMaxCount, lastShapeIndex);




		if(true) {

			// Vec3 colorReflection = castRay(world, attenuation, shapeReflectionPos, shapeReflectionDir, rayIndex + 1, rayMaxCount, lastShapeIndex);

			// int diffuseRayCount = 4;
			int diffuseRayCount = 1;
			Vec3 colorDiffusion = vec3(0,0,0);
			for(int i = 0; i < diffuseRayCount; i++) {
				// Vec3 randomDir = normVec3(vec3(randomFloat(-1,1,0.01f), randomFloat(-1,1,0.01f), randomFloat(-1,1,0.01f)));
				// if(dot(randomDir, shapeReflectionNormal) <= 0) randomDir = reflectVector(randomDir, shapeReflectionNormal);


				// Quat q = eulerAnglesToQuat(randomFloat(0,M_2PI, 0.001f), randomFloat(0,M_2PI, 0.001f), randomFloat(0,M_2PI, 0.001f));
				// Vec3 d = vec3(0,1,0);
				// d = normVec3(q*d);

				// Vec3 randomDir = d;
				// if(dot(randomDir, shapeReflectionNormal) <= 0) randomDir = reflectVector(randomDir, shapeReflectionNormal);


				// Cube discard method.
				Vec3 randomDir;
				do {
					// randomDir = vec3(randomFloat(-1,1,0.01f), randomFloat(-1,1,0.01f), randomFloat(-1,1,0.01f));

					randomDir = vec3(randomFloatPCG(-1,1,0.01f), randomFloatPCG(-1,1,0.01f), randomFloatPCG(-1,1,0.01f));

					// randomDir = shapeReflectionNormal;

				} while(lenVec3(randomDir) > 1);
				randomDir = normVec3(randomDir);
				if(randomDir == vec3(0,0,0)) randomDir = shapeReflectionNormal;

				if(dot(randomDir, shapeReflectionNormal) <= 0) randomDir = reflectVector(randomDir, shapeReflectionNormal);


				randomDir.x = lerp(s->reflectionMod, randomDir.x, shapeReflectionDir.x);
				randomDir.y = lerp(s->reflectionMod, randomDir.y, shapeReflectionDir.y);
				randomDir.z = lerp(s->reflectionMod, randomDir.z, shapeReflectionDir.z);
				randomDir = normVec3(randomDir);

				colorDiffusion += castRay(world, attenuation, shapeReflectionPos, randomDir, rayIndex + 1, rayMaxCount, lastShapeIndex);
			}

			colorDiffusion = colorDiffusion/(float)diffuseRayCount;


			finalColor += colorDiffusion;
		}


	} else {


		if(rayIndex == 0) {
			// Sky hit.
			finalColor += world->defaultEmitColor;
		} else {
			// finalColor += attenuation * defaultEmitColor;

			float lightDot = dot(rayDir, -world->globalLightDir);
			lightDot = clampMin(lightDot, 0);
			lightDot = dotUnitToPercent(lightDot);
			Vec3 light = world->globalLightColor * lightDot;

			finalColor += attenuation * (world->defaultEmitColor + light);
			// finalColor += attenuation * light;
			// finalColor += attenuation * world->defaultEmitColor;
		}


		// // if(rayIndex == 0) {
		// 	// finalColor += world->defaultEmitColor;
		// // } else {
		// 	// float lightDot = dot(rayDir, -world->globalLightDir);
		// 	// lightDot = clampMin(lightDot, 0);
		// 	// Vec3 light = world->globalLightColor * lightDot;

		// 	// finalColor += attenuation * (world->defaultEmitColor + light);
		// 	// finalColor += attenuation * light;
		// 	finalColor += attenuation * world->defaultEmitColor;
		// // }

	}

	return finalColor;
}

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

void processPixelRecursive(World* world, Vec2i texDim, int x, int y, Vec3* buffer) {

	Camera* camera = &world->camera;
	Orientation camRot = getVectorsFromRotation(camera->rot);

	int rayMaxCount = 5;
	// int rayMaxCount = 3;

	bool sampleModeFixedGrid = false; // Grid or MSAA.
	// int sampleCount = 1;
	// int sampleCount = pow(32,2);
	// int sampleCount = pow(4,2);
	// int sampleCount = 4;
	int sampleCount = 8;

	// myAssert(sampleCount <= 8*8);

	Vec2 samples[32*32];
	if(sampleModeFixedGrid) {
		// int sampleCount2 = sampleCount/2;
		int sampleCount2 = sqrt(sampleCount);
		for(int i = 0; i < sampleCount; i++) {
			samples[i] = vec2(((i%sampleCount2)*sampleCount2 + 1) / (float)sampleCount, 
			                  ((i/sampleCount2)*sampleCount2 + 1) / (float)sampleCount);
		}
	} else {
		myAssert(sampleCount == 4 || sampleCount == 8);

		for(int i = 0; i < sampleCount; i++) {
			if(sampleCount == 4)      samples[i] = msaa4xPatternSamples[i];
			else if(sampleCount == 8) samples[i] = msaa8xPatternSamples[i];
		}
	}

	Vec3 camTopLeft = camera->pos + camRot.dir*camera->dist + (camRot.right*-1)*(camera->dim.w/2.0f) + (camRot.up)*(camera->dim.h/2.0f);
	Vec2 percent = vec2(x/(float)texDim.w, y/(float)texDim.h);
	Vec2 pixelPercent = vec2(1/(float)texDim.w, 1/(float)texDim.h);

	Vec3 finalColor = vec3(0,0,0);
	for(int i = 0; i < sampleCount; i++) {
		Vec3 p = camTopLeft;
		p += camRot.right * (camera->dim.w*percent.x + pixelPercent.w*samples[i].x);
		p += -camRot.up   * (camera->dim.h*percent.y + pixelPercent.h*samples[i].y);
		finalColor += castRay(world, vec3(1,1,1), camera->pos, normVec3(p - camera->pos), 0, rayMaxCount, -1);
	}
	finalColor = finalColor/(float)sampleCount;


	clampMax(&finalColor.r, 1);
	clampMax(&finalColor.g, 1);
	clampMax(&finalColor.b, 1);

	buffer[y*texDim.w + x] = finalColor;
}

struct ProcessPixelsData {
	World* world;
	Vec2i dim;
	int pixelIndex;
	int pixelCount;
	Vec3* buffer;
};
	
void processPixelsThreaded(void* data) {
	ProcessPixelsData* d = (ProcessPixelsData*)data;

	int totalPixelCount = d->dim.w * d->dim.h;
	for(int i = d->pixelIndex; i < d->pixelIndex+d->pixelCount; i++) {

		if(i >= totalPixelCount) break;

		int y = i / d->dim.w;
		int x = i % d->dim.w;

		// processPixel(d->world, d->dim, x, y, d->buffer);
		processPixelRecursive(d->world, d->dim, x, y, d->buffer);
	}

}


#if 0
void processPixel(World* world, Vec2i dim, int x, int y, Vec3* buffer) {

	float pixelPercent = (float)1/dim.w;

	Camera* camera = &world->camera;
	Orientation camRot = getVectorsFromRotation(camera->rot);

	float xPercent = x/((float)dim.w-1);
	float yPercent = y/((float)dim.h-1);
	Vec3 p = camera->pos + camRot.dir*camera->dist;

	p += (camRot.right*-1) * -((camera->dim.w*(xPercent + (pixelPercent*0.5f + pixelPercent*randomFloat(-0.5f,0.5f,0.0001f)))) - camera->dim.w*0.5f);
	p += camRot.up * -((camera->dim.h*(yPercent + (pixelPercent*0.5f + pixelPercent*randomFloat(-0.5f,0.5f,0.0001f)))) - camera->dim.h*0.5f);

	Vec3 rayPos = camera->pos;
	Vec3 rayDir = normVec3(p - camera->pos);

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
		for(int i = 0; i < world->shapeCount; i++) {
			if(lastShapeIndex == i) continue;

			Shape* s = world->shapes + i;

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
			Shape* s = world->shapes + shapeIndex;
			lastShapeIndex = shapeIndex;

			finalColor += attenuation * s->emitColor;
			attenuation = attenuation * s->color;

			rayPos = shapeReflectionPos;

			// Vec3 randomVec = vec3(randomFloat(0,1,0.01f), randomFloat(0,1,0.01f), randomFloat(0,1,0.01f));
			// Vec3 randomDir = normVec3(shapeReflectionNormal + randomVec);




			Vec3 randomDir = normVec3(vec3(randomFloat(-1,1,0.01f), randomFloat(-1,1,0.01f), randomFloat(-1,1,0.01f)));
			if(dot(randomDir, shapeReflectionNormal) <= 0) randomDir = reflectVector(randomDir, shapeReflectionNormal);

			// while(dot(randomDir, shapeReflectionNormal) <= 0) {
			// 	randomDir = normVec3(vec3(randomFloat(-1,1,0.01f), randomFloat(-1,1,0.01f), randomFloat(-1,1,0.01f)));
			// }

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
				finalColor = world->defaultEmitColor;
			} else {
				// finalColor += attenuation * defaultEmitColor;

				float lightDot = dot(rayDir, -world->globalLightDir);
				// lightDot = clampMin(lightDot, 0);
				Vec3 light = world->globalLightColor * lightDot;

				// finalColor += attenuation * (world->defaultEmitColor + light);
				finalColor += attenuation * light;
				// finalColor += attenuation * world->defaultEmitColor;
			}

			break;
		}

		rayIndex++;
		if(rayIndex >= rayMaxCount) break;
	}

	buffer[y*dim.w + x] = finalColor;
}
#endif 
