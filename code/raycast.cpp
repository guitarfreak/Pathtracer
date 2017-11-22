

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


struct World {
	Vec2 camDim;
	Vec3 camPos;
	Vec3 camDir;
	Vec3 camRight;
	Vec3 camUp;
	float camDist;

	Shape* shapes;
	int shapeCount;

	Vec3 defaultEmitColor;
	Vec3 globalLightColor;
	Vec3 globalLightDir;
};

void processPixel(World* world, Vec2i dim, int x, int y, Vec3* buffer) {

	float pixelPercent = (float)1/dim.w;

	Vec3 camLeft = world->camRight * -1;

	float xPercent = x/((float)dim.w-1);
	float yPercent = y/((float)dim.h-1);
	Vec3 p = world->camPos + world->camDir*world->camDist;

	p += camLeft * -((world->camDim.w*(xPercent + (pixelPercent*0.5f + pixelPercent*randomFloat(-0.5f,0.5f,0.0001f)))) - world->camDim.w*0.5f);
	p += world->camUp * -((world->camDim.h*(yPercent + (pixelPercent*0.5f + pixelPercent*randomFloat(-0.5f,0.5f,0.0001f)))) - world->camDim.h*0.5f);

	Vec3 rayPos = world->camPos;
	Vec3 rayDir = normVec3(p - world->camPos);

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
			// if(dot(randomDir, shapeReflectionNormal) <= 0) randomDir = reflectVector(randomDir, shapeReflectionNormal);

			while(dot(randomDir, shapeReflectionNormal) <= 0.9f) {
				randomDir = normVec3(vec3(randomFloat(-1,1,0.01f), randomFloat(-1,1,0.01f), randomFloat(-1,1,0.01f)));
			}

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

		processPixel(d->world, d->dim, x, y, d->buffer);
	}

}
