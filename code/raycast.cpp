
#include "external\iacaMarks.h"


enum {
	GEOM_TYPE_SPHERE = 0,
	GEOM_TYPE_BOX,
	GEOM_TYPE_CYLINDER,
	GEOM_TYPE_CONE,

	GEOM_TYPE_COUNT,
};

char* geometryTypeStrings[] = {
	"GEOM_TYPE_SPHERE",
	"GEOM_TYPE_BOX",
	"GEOM_TYPE_CYLINDER",
	"GEOM_TYPE_CONE",
};

struct Material {
	Vec3 emitColor;
	float reflectionMod;

	float refractiveIndex;
};

struct Geometry {
	int type;
	float boundingSphereRadius;
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
	int id;

	Vec3 pos;
	Quat rot;
	Vec3 dim;

	Vec3 color;
	Geometry geometry;
	Material material;

	bool markedForDeletion;
};

float lineShapeIntersection(Vec3 lp, Vec3 ld, Object* obj, Vec3* reflectionPos, Vec3* reflectionNormal) {

	float distance;
	Geometry* geometry = &obj->geometry;
	switch(geometry->type) {
		case GEOM_TYPE_BOX: {
			int face;
			bool hit = boxRaycast(lp, ld, rect3CenDim(obj->pos, obj->dim), &distance, &face);
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
			distance = lineSphereIntersection(lp, ld, obj->pos, obj->dim.x*0.5f, reflectionPos);

			if(distance != -1) {
				*reflectionNormal = normVec3(*reflectionPos - obj->pos);

				return distance;
			}
		}

		default: break;
	}

	return -1;
}

void geometryBoundingSphere(Object* obj) {
	float r;
	switch(obj->geometry.type) {
		case GEOM_TYPE_BOX:    r = lenVec3(obj->dim)*0.5f; break;
		case GEOM_TYPE_SPHERE: {
			r = max(obj->dim.x, obj->dim.y, obj->dim.z)*0.5f;
		} break;
		case GEOM_TYPE_CYLINDER: {
			r = max(obj->dim.x, obj->dim.y)*0.5f + obj->dim.z*0.5f;
		} break;
		case GEOM_TYPE_CONE: {
			r = max(obj->dim.x, obj->dim.y)*0.5f + obj->dim.z*0.5f;
		} break;
	}

	obj->geometry.boundingSphereRadius = r;
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

// https://www.scratchapixel.com/lessons/3d-basic-rendering/introduction-to-shading/reflection-refraction-fresnel
Vec3 refract(Vec3 incident, Vec3 normal, float refractiveIndex) {
    float cosi = clamp(-1, 1, dot(incident, normal)); 
    float etai = 1; 
    float etat = refractiveIndex; 

    Vec3 n = normal; 
    if(cosi < 0) cosi = -cosi; 
    else { 
    	swap(&etai, &etat); 
    	n = -normal; 
    } 

    float eta = etai / etat; 
    float k = 1 - eta * eta * (1 - cosi * cosi); 

    if(k < 0) {
    	return vec3(0,0,0);
    } else {
    	Vec3 result = eta * incident + (eta * cosi - sqrtf(k)) * n;	
    	return result;
    }
}

float fresnel(Vec3 incident, Vec3 normal, float refractiveIndex) {
	float cosi = clamp(-1, 1, dot(incident, normal)); 
	float etai = 1; 
	float etat = refractiveIndex; 

	if(cosi > 0) {
		swap(&etai, &etat); 
	}

    // Compute sini using Snell's law
    float sint = etai / etat * sqrtf(max(0.0f, 1 - cosi * cosi)); 

    float kr;
    // Total internal reflection
    if(sint >= 1) { 
        kr = 1; 
    } else { 
        float cost = sqrtf(max(0.0f, 1 - sint * sint)); 
        cosi = fabsf(cosi); 
        float Rs = ((etat * cosi) - (etai * cost)) / ((etat * cosi) + (etai * cost)); 
        float Rp = ((etai * cosi) - (etat * cost)) / ((etai * cosi) + (etat * cost)); 
        kr = (Rs * Rs + Rp * Rp) / 2; 
    } 

    return kr;
} 


enum SampleMode {
	SAMPLE_MODE_GRID = 0,
	SAMPLE_MODE_BLUE,

	SAMPLE_MODE_COUNT,	
};

char* sampleModeStrings[] = {
	"GRID",
	"BLUE_NOISE",
};

struct World {
	Camera camera;

	DArray<Object> objects;

	Vec3 defaultEmitColor;

	Vec3 globalLightDir;
	Vec3 globalLightColor;

	float focalPointDistance;
	float apertureSize;

	bool lockFocalPoint;
	Vec3 focalPoint;

	Vec2i globalLightRot;
};

void worldCalcLightDir(World* world) {
	Vec2i lightRot = world->globalLightRot;
	float angleXY = degreeToRadian(lightRot.x);
	float angleZ = degreeToRadian(lightRot.y);

	world->globalLightDir = quat(angleXY, vec3(0,0,-1)) * quat(-angleZ, vec3(0,1,0)) * vec3(1,0,0);
}

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

	int sampleCountWanted;

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
	Vec2i pixelPos;
	Vec2i pixelDim;

	bool stopProcessing;
};




// http://www.cl.cam.ac.uk/teaching/1999/AGraphHCI/SMAG/node2.html#eqn:rectcylrayquad
float lineCylinderIntersection(Vec3 p, Vec3 d, Vec3* intersection = 0, Vec3* intersectionNormal = 0, bool secondIntersection = false) {

	// Cylinder is at origin and has r = 1 and h = 2;

	// Solve for circle:
	// Substitute ray equation in circle equation and then solve quadratic equation.

	float a = dot(d.xy, d.xy);
	float b = 2*dot(p.xy, d.xy);
	float c = dot(p.xy, p.xy) - 1;

	float discriminant = b*b - 4*a*c;

	if(discriminant < 0) return -1;

	float t1 = (-b + sqrt(discriminant)) / (2*a);
	float t2 = (-b - sqrt(discriminant)) / (2*a);

	float z1 = p.z + d.z*t1;
	float z2 = p.z + d.z*t2;

	// Type 0 = middle, 1 = top, 2 = bottom, 3 = invalid.
	int type1 = 0, type2 = 0;

	// If above or below check if cap hit, else set invalid.
	     if(z1 >  1) if(z2 <=  1) { type1 = 1; t1 = ( 1-p.z)/d.z; } else type1 = 3;
	else if(z1 < -1) if(z2 >= -1) { type1 = 2; t1 = (-1-p.z)/d.z; } else type1 = 3;
	     if(z2 >  1) if(z1 <=  1) { type2 = 1; t2 = ( 1-p.z)/d.z; } else type2 = 3;
	else if(z2 < -1) if(z1 >= -1) { type2 = 2; t2 = (-1-p.z)/d.z; } else type2 = 3;

	// Both above or below.
	if(type1 == 3 && type2 == 3) return -1;

	// Both behind.
	if(t1 < 0 && t2 < 0) return -1;

	float distance;
	if(!secondIntersection) {
		distance = min(t1,t2);
		if(distance < 0) distance = max(t1,t2);
	} else {
		distance = max(t1,t2);
	}

	if(intersection) {
		*intersection = p + d*distance;

		if(intersectionNormal) {
			int type = distance == t1 ? type1 : type2;

			if(type == 0) {
				if(intersection->xy == vec2(0,0)) *intersectionNormal = vec3(1,0,0);
				else *intersectionNormal = normVec3(vec3((*intersection).xy, 0));
			}
			else if(type == 1) *intersectionNormal = vec3(0,0,1);
			else *intersectionNormal = vec3(0,0,-1);
		}	
	}

	return distance;
}

float lineConeIntersection(Vec3 p, Vec3 d, Vec3* intersection = 0, Vec3* intersectionNormal = 0, bool secondIntersection = false) {

	// Offset because equation is from -1 to 0.
	p.z -= 0.5;

	// Cone top is at origin, radius = 1 and h = 1;

	// Solve x^2 + y^2 = z^2, -1 < z < 0:
	// Substitute ray equation and then solve quadratic equation.

	float a = d.x*d.x + d.y*d.y - d.z*d.z;
	float b = 2*p.x*d.x + 2*p.y*d.y - 2*p.z*d.z;
	float c = p.x*p.x + p.y*p.y - p.z*p.z;

	float discriminant = b*b - 4*a*c;

	if(discriminant < 0) return -1;

	float t1 = (-b + sqrt(discriminant)) / (2*a);
	float t2 = (-b - sqrt(discriminant)) / (2*a);

	float z1 = p.z + d.z*t1;
	float z2 = p.z + d.z*t2;

	// Type 0 = side, 1 = bottom cap, 2 = invalid.
	int type1 = 0, type2 = 0;

	if(z1 > 0 || z1 < -1) type1 = 2;
	if(z2 > 0 || z2 < -1) type2 = 2;

	// Both invalid.
	if(type1 == 2 && type2 == 2) return -1;

	// If one invalid we must hit the bottom cap.
	if(type1 == 2 || type2 == 2) {
		if(type1 == 2) t1 = (-1-p.z)/d.z;
		else t2 = (-1-p.z)/d.z;
	}

	// Both behind.
	if(t1 < 0 && t2 < 0) return -1;

	float distance;
	if(!secondIntersection) {
		distance = min(t1,t2);
		if(distance < 0) max(t1,t2);
	} else {
		distance = max(t1,t2);
	}

	if(intersection) {
		p.z += 0.5f;
		*intersection = p + d*distance;

		if(intersectionNormal) {
			int type = distance == t1 ? type1 : type2;

			if(type == 0) {
				if((*intersection).xy == vec2(0,0)) {
					*intersectionNormal = vec3(0,0,1);
				} else {
					Vec3 n = normVec3(vec3((*intersection).xy, 0));
					n = normVec3(vec3(n.xy, 1));
					*intersectionNormal = n;
				}
			}
			else *intersectionNormal = vec3(0,0,-1);
		}	
	}

	return distance;
}

// 2*R*sqrt((x^2) + 2*t*x*a + (t^2)*(a^2) + (y^2) + 2*t*y*b + (t^2)*(b^2)) = (R^2) + (x^2) + 2*t*x*a+(t^2)*(a^2) + (y^2)+2*t*y*b+(t^2)*(b^2) + (z^2)+2*t*z*c+(t^2)*(c^2) - (r^2)

// float lineTorusIntersection(Vec3 p, Vec3 d, Vec3* intersection = 0, Vec3* intersectionNormal = 0, bool secondIntersection = false) {
// }


// Transform a vector to unit system and back:
//   -translate, -rotate, -scale -> scale, rotate, translate.
// Transform a direction to unit system and back:
//   -rotate, -scale -> -scale, rotate.
void transformToUnitSpace(Object* obj, Vec3 dimScale, Vec3 pos, Vec3 dir, Vec3* posTransformed, Vec3* dirTransformed) {
	bool rotated = !(obj->rot == quat());
	bool unequalDim = equal(obj->dim.x, obj->dim.y, obj->dim.z);

	pos = pos - obj->pos;
	// if(rotated) pos = quatInverse(obj->rot) * pos;
	pos = quatInverse(obj->rot) * pos;
	pos = pos * (dimScale/obj->dim);
	*posTransformed = pos;

	// if(rotated) dir = quatInverse(obj->rot) * dir;
	dir = quatInverse(obj->rot) * dir;
	dir = dir * (dimScale/obj->dim);
	dir = normVec3(dir);
	*dirTransformed = dir;
}

void transformToGlobalSpace(Object* obj, Vec3 dimScale, Vec3 pos, Vec3 dir, Vec3* posTransformed, Vec3* dirTransformed) {
	bool rotated = !(obj->rot == quat());
	bool unequalDim = equal(obj->dim.x, obj->dim.y, obj->dim.z);

	pos = pos * (obj->dim/dimScale);
	// if(rotated) pos = obj->rot * pos;
	pos = obj->rot * pos;
	pos = pos + obj->pos;
	*posTransformed = pos;

	dir = dir * (dimScale/obj->dim);
	// if(rotated) dir = obj->rot * dir;
	dir = obj->rot * dir;
	dir = normVec3(dir);
	*dirTransformed = dir;
}


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

int castRay(Vec3 rayPos, Vec3 rayDir, DArray<Object>* objects, int lastObjectIndex = 0, Vec3* intersection = 0, Vec3* intersectionNormal = 0) {

	int objectIndex = 0;

	// Were inside a transparent object.

	bool insideObject = lastObjectIndex < 0;

	float minDistance = FLT_MAX;
	for(int i = 0; i < objects->count; i++) {
		if(lastObjectIndex-1 == i) continue;

		if(insideObject) i = (-lastObjectIndex) - 1;

		Object* obj = &objects->at(i);
		Geometry* g = &obj->geometry;

		bool possibleIntersection;
		if(!insideObject) possibleIntersection = lineSphereCollision(rayPos, rayDir, obj->pos, g->boundingSphereRadius);
		else possibleIntersection = true;

		if(possibleIntersection) {

			Vec3 position, normal;
			float distance = -1;

			switch(g->type) {

				case GEOM_TYPE_BOX: {
					distance = boxRaycastRotated(rayPos, rayDir, obj->pos, obj->dim, obj->rot, &position, &normal, insideObject);
				} break;

				case GEOM_TYPE_SPHERE: {
					if(equal(obj->dim.x, obj->dim.y, obj->dim.z)) {
						distance = lineSphereIntersection(rayPos, rayDir, obj->pos, obj->dim.x*0.5f, &position, &normal, insideObject);
					} else {

						Vec3 dimScale = vec3(1.0f);
						Vec3 lp, ld;
						transformToUnitSpace(obj, dimScale, rayPos, rayDir, &lp, &ld);

						distance = lineSphereIntersection(lp, ld, vec3(0,0,0), 0.5f, &position, &normal, insideObject);

						if(distance != -1) {
							transformToGlobalSpace(obj, dimScale, position, normal, &position, &normal);
							distance = lenVec3(rayPos - position);
						}
					}
				} break;

				case GEOM_TYPE_CYLINDER: {

					Vec3 dimScale = vec3(2.0f); // Radius = 1, h = 2.
					Vec3 lp, ld;
					transformToUnitSpace(obj, dimScale, rayPos, rayDir, &lp, &ld);

					distance = lineCylinderIntersection(lp, ld, &position, &normal, insideObject);

					if(distance != -1) {
						transformToGlobalSpace(obj, dimScale, position, normal, &position, &normal);
						distance = lenVec3(rayPos - position);
					}
				} break;

				case GEOM_TYPE_CONE: {

					Vec3 dimScale = vec3(2,2,1); // Radius = 1, h = 1.
					Vec3 lp, ld;
					transformToUnitSpace(obj, dimScale, rayPos, rayDir, &lp, &ld);

					distance = lineConeIntersection(lp, ld, &position, &normal, insideObject);

					if(distance != -1) {
						transformToGlobalSpace(obj, dimScale, position, normal, &position, &normal);
						distance = lenVec3(rayPos - position);
					}
				} break;
			}

			if(insideObject) {
				if(intersection) *intersection = position;
				if(intersectionNormal) *intersectionNormal = normal;

				return lastObjectIndex;
			}

			if(distance > 0 && distance < minDistance) {
				minDistance = distance;
				objectIndex = i+1;

				if(intersection) *intersection = position;
				if(intersectionNormal) *intersectionNormal = normal;
			}
		}
	}

	return objectIndex;
}

Vec3 processSample(Vec3 rayPos, Vec3 rayDir, World* world, RaytraceSettings* settings, int lastObjectIndex = 0, Vec3 attenuation = vec3(1,1,1), int rayIndex = 0) {

	Vec3 sampleColor = vec3(0,0,0);

	// Find object with closest intersection.

	Vec3 objectReflectionPos, objectReflectionNormal;
	int objectIndex = castRay(rayPos, rayDir, &world->objects, lastObjectIndex, &objectReflectionPos, &objectReflectionNormal);

	if(objectIndex != 0) {

		Object* obj = world->objects + ((objectIndex<0?(-objectIndex):objectIndex)-1);
		Material* m = &obj->material;

		// Color calculation.

		sampleColor += attenuation * m->emitColor;
		attenuation = attenuation * colorSRGB(obj->color);
		// attenuation = attenuation * obj->color;
	
		if(attenuation == vec3(0,0,0)) return sampleColor;

		if(rayIndex == settings->rayBouncesMax) return sampleColor;


		if(m->refractiveIndex != 1.0f) {

			// Transparent objects.

			float ratio = fresnel(rayDir, objectReflectionNormal, m->refractiveIndex);

			// Cast ray in mirror direction and refraction direction and
			// then combine the results based on the fresnel ratio.

			Vec3 reflectionColor = vec3(0,0,0);
			Vec3 refractionColor = vec3(0,0,0);

			if(ratio != 0.0f) {
				Vec3 objectReflectionDir = reflectVector(rayDir, objectReflectionNormal);
				reflectionColor = processSample(objectReflectionPos, objectReflectionDir, world, settings, objectIndex, attenuation, rayIndex + 1);
			}

			if(ratio != 1.0f) {
				Vec3 refractionDir = refract(rayDir, objectReflectionNormal, m->refractiveIndex);

				// Refract is not quite in sync with the fresnal calculation.
				if(refractionDir == vec3(0,0,0)) {
					ratio = 1;
				} else {
					refractionColor = processSample(objectReflectionPos, refractionDir, world, settings, -objectIndex, attenuation, rayIndex + 1);
				}
			}

			sampleColor += lerp(ratio, refractionColor, reflectionColor);

		} else {

			// Reflection direction.

			rayDir = reflectVector(rayDir, objectReflectionNormal);

			if(m->reflectionMod != 1.0f) {

				// If not mirror, scatter randomly.

				int dirIndex = randomIntPCG(0, settings->randomDirectionCount-1);
				Vec3 randomDir = settings->randomDirections[dirIndex];

				float d = dot(randomDir, objectReflectionNormal);
				if(d <= 0) randomDir = reflectVector(randomDir, objectReflectionNormal);

				rayDir = lerp(m->reflectionMod, randomDir, rayDir);
			}

			sampleColor += processSample(objectReflectionPos, rayDir, world, settings, objectIndex, attenuation, rayIndex + 1);

		}

	} else if(rayIndex == 0) {

		// Sky hit.
		sampleColor += world->defaultEmitColor;

	} else {

		float lightDot = dot(rayDir, world->globalLightDir);
		lightDot = clampMin(lightDot, 0);
		Vec3 light = world->globalLightColor * lightDot;
		
		sampleColor += attenuation * (world->defaultEmitColor + light);
	}

	return sampleColor;
}

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

	world.globalLightDir = normVec3(world.globalLightDir);

	Vec2i texDim = settings.texDim;
	int pixelCount = d->pixelDim.x * d->pixelDim.y;
	for(int pixelIndex = 0; pixelIndex < pixelCount; pixelIndex++) {
		startTimer(0);

		if(d->stopProcessing) {
			d->stopProcessing = false;
			break;
		}
		
		int x = pixelIndex % d->pixelDim.w;
		int y = pixelIndex / d->pixelDim.w;
		x += d->pixelPos.x;
		y += d->pixelPos.y;

		if(x < 0 || x >= texDim.w || 
		   y < 0 || y >= texDim.h) continue;

		Vec2 percent = vec2(x/(float)texDim.w, y/(float)texDim.h);

		if(settings.sampleMode == SAMPLE_MODE_BLUE) {
			int index = (y%settings.sampleGridWidth)*settings.sampleGridWidth + (x%settings.sampleGridWidth);
			int offset = settings.sampleGridOffsets[index];
			samples = settings.samples + offset;
			sampleCount = settings.sampleGridOffsets[index+1] - offset;
		}

		{
			// IACA_VC64_START;

			Vec3 pixelColor = black;

			for(int sampleIndex = 0; sampleIndex < sampleCount; sampleIndex++) {
				startTimer(1);

				Vec3 rayPos = settings.camTopLeft;
				rayPos += camera.ovecs.right * (camera.dim.w * (percent.w + settings.pixelPercent.w*samples[sampleIndex].x));
				rayPos -= camera.ovecs.up  * (camera.dim.h * (percent.h + settings.pixelPercent.h*samples[sampleIndex].y));

				Vec3 rayDir = normVec3(rayPos - camera.pos);
				
				if(world.apertureSize > 0.0f) {
					Vec3 focalPoint;

					Vec3 focalPlanePos = camera.pos + camera.ovecs.dir * world.focalPointDistance;
					float distance = linePlaneIntersection(rayPos, rayDir, focalPlanePos, -camera.ovecs.dir, &focalPoint);

					Vec2 randomOffset = vec2(randomFloatPCG(0.0f, world.apertureSize, 0.01f), 
					                         randomFloatPCG(0.0f, world.apertureSize, 0.01f));

					rayPos += camera.ovecs.right * randomOffset.x;
					rayPos += -camera.ovecs.up * randomOffset.y;

					rayDir = normVec3(focalPoint - rayPos);
				}

				Vec3 sampleColor = processSample(rayPos, rayDir, &world, &settings);

				sampleColor = clampMax(sampleColor, white);
				pixelColor += sampleColor;

				endTimer(1);
			}

			// Pixel end.

			pixelColor = pixelColor/(float)sampleCount;
			pixelColor = clampMax(pixelColor, white);

			buffer[y*texDim.w + x] = pixelColor;

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




void getDefaultScene(World* world) {
	float zLevel = 7;

	Camera* cam = &world->camera;
	cam->pos = vec3(0, -30, zLevel);
	cam->rot = vec3(0, 0, 0);
	cam->fov = 90;
	cam->dim.w = 10;
	cam->farDist = 10000;

	world->focalPoint = vec3(0,0,zLevel);
	// world->focalPointDistance = 30.0f;
	world->apertureSize = 0;
	world->lockFocalPoint = false;

	world->defaultEmitColor = vec3(1,1,1);
	world->globalLightRot = vec2i(0,90);

	world->globalLightColor = vec3(1,1,1);

	world->objects.init();

	Material materials[10] = {};
	materials[0].emitColor = vec3(0,0,0);
	materials[0].reflectionMod = 0.5f;
	materials[0].refractiveIndex = 1.0f;

	materials[1].emitColor = vec3(0,0,0);
	materials[1].reflectionMod = 1.0f;
	materials[1].refractiveIndex = 1.0f;

	Object obj;

	// Ground plane.

	obj = {};
	obj.pos = vec3(0,0,0.1f);
	obj.rot = quat();
	obj.color = vec3(0.5f);
	obj.material = materials[0];
	obj.geometry.type = GEOM_TYPE_BOX;
	obj.dim = vec3(50, 50, 0.01f);
	world->objects.push(obj);

	// Sphere.

	obj = {};
	obj.pos = vec3(0,0,zLevel);
	obj.rot = quat();
	obj.color = vec3(0.7f,0.7f,0.7f);
	obj.material = materials[1];
	obj.dim = vec3(8.0f);
	obj.geometry.type = GEOM_TYPE_SPHERE;
	world->objects.push(obj);
	
	// Calc bounding spheres.
	for(int i = 0; i < world->objects.count; i++) {
		geometryBoundingSphere(world->objects + i);
	}
}



enum EntitySelectionMode {
	ENTITYUI_MODE_TRANSLATION = 0,
	ENTITYUI_MODE_ROTATION,
	ENTITYUI_MODE_SCALE,
	ENTITYUI_MODE_DRAG,

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

// Add selection modification.
enum CommandType {
	COMMAND_TYPE_EDIT = 0,
	COMMAND_TYPE_INSERT,
	COMMAND_TYPE_REMOVE,
	COMMAND_TYPE_SELECTION,

	COMMAND_TYPE_SIZE,
};

struct HistoryCommand {
	int type;
	int count;
};

struct HistoryData {
	int index;
	DArray<char> buffer;
	DArray<int> offsets;

	// Temp.

	DArray<Object> objectsPreMod;
	DArray<Object> tempObjects;
	DArray<int> tempSelected;

	DArray<int> previousSelection;
};

void historyReset(HistoryData* hd) {
	hd->index = 0;
	hd->buffer.clear();
	hd->offsets.clear();
	hd->previousSelection.clear();
}

Object objectDiff(Object o0, Object o1) {
	Object diff = {};

	diff.pos = o0.pos - o1.pos;
	diff.rot = o0.rot - o1.rot;
	diff.dim = o0.dim - o1.dim;
	diff.color = o0.color - o1.color;
	diff.geometry.type = o0.geometry.type - o1.geometry.type;
	diff.material.emitColor = o0.material.emitColor - o1.material.emitColor;
	diff.material.reflectionMod = o0.material.reflectionMod - o1.material.reflectionMod;
	diff.material.refractiveIndex = o0.material.refractiveIndex - o1.material.refractiveIndex;

	return diff;
}

Object objectAdd(Object obj, Object diff) {
	obj.pos = obj.pos + diff.pos;
	obj.rot = obj.rot + diff.rot;
	obj.dim = obj.dim + diff.dim;
	obj.color = obj.color + diff.color;
	obj.geometry.type = obj.geometry.type + diff.geometry.type;
	obj.material.emitColor = obj.material.emitColor + diff.material.emitColor;
	obj.material.reflectionMod = obj.material.reflectionMod + diff.material.reflectionMod;
	obj.material.refractiveIndex = obj.material.refractiveIndex + diff.material.refractiveIndex;

	return obj;
}



struct EntityUI {
	HistoryData history;
	bool objectNoticeableChange;

	DArray<int> selectedObjects;
	DArray<Object> objectCopies;
	DArray<Object> objectTempArray;
	bool objectsEdited;

	// For multiple selection.
	DArray<Vec3> objectCenterOffsets;
	Object multiChangeObject;

	Vec2 dragSelectionStart;
	bool dragSelectionActive;
	bool multipleSelectionMode;

	int selectionMode;
	int selectionState;
	bool gotActive;
	int hotId;

	bool selectionChanged;

	bool guiHasFocus;

	float selectionAnimState;
	bool localMode;
	bool snappingEnabled;
	int snapGridSize;
	float snapGridDim;

	int axisIndex;
	Vec3 axis;
	Vec3 objectDistanceVector;

	// Translation mode.

	Vec3 startPos;
	Vec3 centerOffset;
	float centerDistanceToCam;
	int translateMode;
	Vec3 axes[3];

	// Rotation mode.

	Quat startRot;
	float currentRotationAngle;

	// Scale mode.

	Vec3 startDim;
	bool enableScaleEqually;

	// For resetting state.
	
	Vec3 currentObjectDistanceVector;
	Vec3 currentPos;

	//

	Object temp;
	bool changedGeomType;
	bool mouseOverScene;

	//

	bool setFocalDistance;
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

int mouseButtonPressedRight(NewGui* gui, Input* input) {
	if(gui->hotId[Gui_Focus_MRight] != 0 || gui->activeId != 0) return false;
	else return input->mouseButtonPressed[1]; 
}

int mouseButtonReleasedRight(NewGui* gui, Input* input) {
	if(gui->hotId[Gui_Focus_MRight] != 0 || gui->activeId != 0) return false;
	else return input->mouseButtonReleased[1]; 
}

int mouseButtonPressedMiddle(NewGui* gui, Input* input) {
	if(gui->hotId[Gui_Focus_MMiddle] != 0 || gui->activeId != 0) return false;
	else return input->mouseButtonPressed[2]; 
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

		fwrite(world->objects.data, sizeof(Object)*world->objects.count, 1, file);
	}

	fclose(file);
}

void loadScene(World* world, char* filePath, EntityUI* eui) {
	FILE* file = fopen(filePath, "rb");

	if(file) {
		DArray<Object> oldObjects = world->objects;
		fread(world, sizeof(World), 1, file);

		oldObjects.freeResize(world->objects.count);
		oldObjects.count = world->objects.count;
		fread(oldObjects.data, sizeof(Object)*world->objects.count, 1, file);
		world->objects = oldObjects;
	}

	fclose(file);
}

// #define HistoryDebugStrings

void historyEdit(HistoryData* hd, DArray<int>* selected) {

	#ifdef HistoryDebugStrings
	printf("%*s%i Edit\n", hd->index, "", hd->index);
	#endif

	int type = COMMAND_TYPE_EDIT;

	// Reset buffers to index position;
	hd->offsets.count = hd->index;
	hd->buffer.count = hd->index == 0 ? 0 : hd->offsets[hd->index-1];

	int* dType = (int*)hd->buffer.retrieve(sizeof(int));
	*dType = type;

	{
		int count = selected->count;
		int totalSize = sizeof(int) + sizeof(Object)*count;

		char* d = hd->buffer.retrieve(totalSize);
		int* dCount = (int*)d; d += sizeof(int);
		Object* dObjects = (Object*)d; d += sizeof(Object) * count;

		*dCount = count;
		for(int i = 0; i < count; i++) {
			Object obj = hd->objectsPreMod[i];
			obj.id = selected->at(i);

			dObjects[i] = obj;
		}

		int offsetSize = sizeof(int) + totalSize;
		int offsetBefore = hd->offsets.count == 0 ? 0 : hd->offsets.last();
		hd->offsets.push(offsetBefore + offsetSize);
	}

	hd->index++;
}

void historyInsert(HistoryData* hd, DArray<Object>* copies) {

	#ifdef HistoryDebugStrings
	printf("%*s%i Insert\n", hd->index, "", hd->index);
	#endif

	int type = COMMAND_TYPE_INSERT;

	// Reset buffers to index position;
	hd->offsets.count = hd->index;
	hd->buffer.count = hd->index == 0 ? 0 : hd->offsets[hd->index-1];

	int* dType = (int*)hd->buffer.retrieve(sizeof(int));
	*dType = type;

	{
		int count = copies->count;
		int totalSize = sizeof(int) + sizeof(Object)*count;

		char* d = hd->buffer.retrieve(totalSize);
		int* dCount = (int*)d; d += sizeof(int);
		Object* dObjects = (Object*)d; d += sizeof(Object) * count;

		*dCount = count;
		for(int i = 0; i < count; i++) {
			Object obj = copies->at(i);
			dObjects[i] = obj;
		}

		int offsetSize = sizeof(int) + totalSize;
		int offsetBefore = hd->offsets.count == 0 ? 0 : hd->offsets.last();
		hd->offsets.push(offsetBefore + offsetSize);
	}

	hd->index++;
}

void historyRemove(HistoryData* hd, DArray<Object>* copies) {

	#ifdef HistoryDebugStrings
	printf("%*s%i Remove\n", hd->index, "", hd->index);
	#endif

	int type = COMMAND_TYPE_REMOVE;

	// Reset buffers to index position;
	hd->offsets.count = hd->index;
	hd->buffer.count = hd->index == 0 ? 0 : hd->offsets[hd->index-1];

	int* dType = (int*)hd->buffer.retrieve(sizeof(int));
	*dType = type;

	{
		int count = copies->count;
		int totalSize = sizeof(int) + sizeof(Object)*count;

		char* d = hd->buffer.retrieve(totalSize);
		int* dCount = (int*)d; d += sizeof(int);
		Object* dObjects = (Object*)d; d += sizeof(Object) * count;

		*dCount = count;
		for(int i = 0; i < count; i++) {
			Object obj = copies->at(i);
			dObjects[i] = obj;
		}

		int offsetSize = sizeof(int) + totalSize;
		int offsetBefore = hd->offsets.count == 0 ? 0 : hd->offsets.last();
		hd->offsets.push(offsetBefore + offsetSize);
	}

	hd->index++;
}

void historySelection(HistoryData* hd, DArray<int>* selected) {

	#ifdef HistoryDebugStrings
	printf("%*s%i Selection\n", hd->index, "", hd->index);
	#endif

	int type = COMMAND_TYPE_SELECTION;

	// Reset buffers to index position;
	hd->offsets.count = hd->index;
	hd->buffer.count = hd->index == 0 ? 0 : hd->offsets[hd->index-1];

	int* dType = (int*)hd->buffer.retrieve(sizeof(int));
	*dType = type;

	{
		if(hd->index == 0) hd->previousSelection.clear();

		int pCount = hd->previousSelection.count;
		int count = selected->count;
		int totalSize = sizeof(int) + sizeof(int)*pCount + sizeof(int) + sizeof(int)*count;

		char* d = hd->buffer.retrieve(totalSize);

		int* dPrevCount = (int*)d; d += sizeof(int);
		int* dPrevSelected = (int*)d; d += sizeof(int) * pCount;
		*dPrevCount = pCount;
		copyArray(dPrevSelected, hd->previousSelection.data, int, pCount);

		int* dCount = (int*)d; d += sizeof(int);
		int* dSelected = (int*)d; d += sizeof(int) * count;
		*dCount = count;
		copyArray(dSelected, selected->data, int, count);

		hd->previousSelection.clear();
		hd->previousSelection.push(selected);

		int offsetSize = sizeof(int) + totalSize;
		int offsetBefore = hd->offsets.count == 0 ? 0 : hd->offsets.last();
		hd->offsets.push(offsetBefore + offsetSize);
	}

	hd->index++;
}

void historyChange(HistoryData* hd, World* world, DArray<int>* selected, bool undo = true) {

	int currentIndex;
	if(undo) {
		if(hd->index == 0) return;

		currentIndex = hd->index-1;
		hd->index--;
	} else {
		if(hd->index == hd->offsets.count) return;

		currentIndex = hd->index;
		hd->index++;
	}

	int offset, size;
	if(currentIndex == 0) {
		offset = 0;
		size = hd->offsets[currentIndex];
	} else {
		offset = hd->offsets[currentIndex-1];
		size = hd->offsets[currentIndex] - offset;
	}

	char* d = hd->buffer.data + offset;
	int type = *((int*)d); d += sizeof(int);


	#ifdef HistoryDebugStrings
	char* typeString;
	switch(type) {
		case 0: typeString = "Edit"; break;
		case 1: typeString = "Insert"; break;
		case 2: typeString = "Remove"; break;
		case 3: typeString = "Selection"; break;
	}
	printf("%*s%i %s %s\n", hd->index, "", hd->index, undo?"Undo":"Redo", typeString);
	#endif


	switch(type) {
		case COMMAND_TYPE_EDIT: {
			int count = *((int*)d); d += sizeof(int);
			Object* objects = (Object*)d; d += sizeof(Object)*count;

			for(int i = 0; i < count; i++) {
				Object obj = objects[i];
				int index = obj.id;

				if(undo) world->objects[index] = objectAdd(world->objects[index], objects[i]);
				else world->objects[index] = objectDiff(world->objects[index], objects[i]);
			}
		} break;

		case COMMAND_TYPE_INSERT: {
			int count = *((int*)d); d += sizeof(int);
			Object* objects = (Object*)d; d += sizeof(Object)*count;

			if(undo) {
				world->objects.pop(count);
			} else {
				world->objects.push(objects, count);
			}
		} break;

		case COMMAND_TYPE_REMOVE: {
			int count = *((int*)d); d += sizeof(int);
			Object* objects = (Object*)d; d += sizeof(Object)*count;

			if(undo) {
				// Insert at indexes where they got deleted.
				selected->clear();
				for(int i = 0; i < count; i++) {
					Object obj = objects[i];
					world->objects.insert(obj, obj.id);

					selected->push(obj.id);
				}

				hd->previousSelection.copy(selected);
			} else {

				// Code duplication with deleteObjects().
				for(int i = 0; i < count; i++) {
					Object* obj = objects + i;
					world->objects.at(obj->id).markedForDeletion = true;
				}

				for(int i = 0; i < world->objects.count; i++) {
					if(world->objects.at(i).markedForDeletion) {
						world->objects.remove(i);
						i--;
					}
				}

				selected->clear();
				hd->previousSelection.copy(selected);
			}
		} break;

		case COMMAND_TYPE_SELECTION: {
			int pCount = *((int*)d); d += sizeof(int);
			int* dPrevSelected = (int*)d; d += sizeof(int)*pCount;
			int count = *((int*)d); d += sizeof(int);
			int* dSelected = (int*)d; d += sizeof(int)*count;

			if(undo) {
				selected->clear();
				selected->push(dPrevSelected, pCount);
				hd->previousSelection.copy(selected);
			} else {
				selected->clear();
				selected->push(dSelected, count);
				hd->previousSelection.copy(selected);
			}

		} break;
	}
}



Object defaultObject() {
	Material m = {};
	m.emitColor = vec3(0,0,0);
	m.reflectionMod = 0.5f;
	m.refractiveIndex = 1.0f;

	Object obj = {};
	obj.pos = vec3(0,0,0);
	obj.rot = quat();
	obj.dim = vec3(5);
	obj.color = vec3(0.5f,0.5f,0.5f);
	obj.material = m;
	obj.geometry.type = GEOM_TYPE_SPHERE;
	
	return obj;
}

void copyObjects(DArray<Object>* objects, DArray<Object>* copies, DArray<int>* selected) {
	copies->clear();
	for(int i = 0; i < selected->count; i++) {
		copies->push(objects->at(selected->at(i)));
	}
}

void deleteObjects(DArray<Object>* objects, DArray<int>* selected, HistoryData* hd, bool switchSelected = true) {

	// Push object removal to history.
	hd->tempObjects.clear();

	// Sort selected objects for undo remove.
	// We could do user a better sort algorithm here, but it doesn't matter right now.
	bubbleSort(selected->data, selected->count);
	for(int i = 0; i < selected->count; i++) {
		Object obj = objects->at(selected->at(i));
		obj.id = selected->at(i);
		hd->tempObjects.push(obj);
	}
	historyRemove(hd, &hd->tempObjects);

	// Do the actual removal.
	for(int i = 0; i < selected->count; i++) {
		objects->at(selected->at(i)).markedForDeletion = true;
	}

	for(int i = 0; i < objects->count; i++) {
		if(objects->at(i).markedForDeletion) {
			objects->remove(i);
			i--;
		}
	}

	selected->clear();
	hd->previousSelection.clear();

	// Ignoring the switch for now.

	// if(selected->count > 1 || !switchSelected || objects->empty()) {
	// 	selected->clear();
	// } else if(!objects->empty()) {
	// 	selected->at(0) = mod(selected->at(0), objects->count);
	// }
}

void insertObjects(World* world, DArray<Object>* copies, DArray<int>* selected, bool* objectsEdited, HistoryData* hd, bool keepPosition) {
	if(copies->empty()) return;

	Camera* cam = &world->camera;
	float spawnDistance = cam->dim.w*2;

	if(copies->count == 1 && !keepPosition) {
		copies->atr(0)->pos = cam->pos + cam->ovecs.dir * spawnDistance;
	}

	world->objects.push(copies);

	historyInsert(hd, copies);
}

bool isObjectSelected(EntityUI* eui, int index) {
	bool selected = false;
	for(int i = 0; i < eui->selectedObjects.count; i++) {
		if(index == eui->selectedObjects[i]) {
			selected = true;
			break;
		}
	}

	return selected;
}

Vec3 selectedObjectsGetCenter(DArray<Object>* objects, DArray<int>* selected) {

	Vec3 pMin = vec3(FLT_MAX);
	Vec3 pMax = vec3(-FLT_MAX);
	for(int i = 0; i < selected->count; i++) {
		Object* obj = objects->atr(selected->at(i));

		pMin.x = min(pMin.x, obj->pos.x);
		pMin.y = min(pMin.y, obj->pos.y);
		pMin.z = min(pMin.z, obj->pos.z);
		pMax.x = max(pMax.x, obj->pos.x);
		pMax.y = max(pMax.y, obj->pos.y);
		pMax.z = max(pMax.z, obj->pos.z);

	}
	Vec3 result = pMin + (pMax - pMin)/2;

	return result;
}

void openSceneDialog(DialogData* dd, bool saveMode = false) {
	dd->type = "SceneDialog";

	dd->saveMode = saveMode;
	if(saveMode) {
		strClear(dd->filePath);
		strCpy(dd->filePath, ".scene");
	}

	dd->initialDir = Scenes_Folder;
	dd->filterIndex = 1;
	dd->filter = "Scene Files\0*.scene\0";
	dd->defaultExtension = ".scene";

    HANDLE thread = CreateThread(0, 0, openDialogProc, dd, 0, 0);
}

void openScreenshotDialog(DialogData* dd) {
	dd->type = "ScreenshotDialog";

	dd->saveMode = true;
	strClear(dd->filePath);
	strCpy(dd->filePath, ".png");

	dd->initialDir = Screenshot_Folder;
	dd->filterIndex = 1;
	dd->filter = "Image Files\0*.png\0";
	dd->defaultExtension = ".png";

    HANDLE thread = CreateThread(0, 0, openDialogProc, dd, 0, 0);
}


void newSceneCommand(World* world, char* sceneFile, bool* sceneHasFile, EntityUI* eui) {
	getDefaultScene(world);
	*sceneHasFile = false;
	strClear(sceneFile);

	historyReset(&eui->history);
	eui->selectedObjects.count = 0;
}

void openSceneCommand(DialogData* dialogData) {
	openSceneDialog(dialogData);
}

void saveSceneCommand(World* world, char* sceneFile, bool sceneHasFile, DialogData* dialogData) {
	if(sceneHasFile) saveScene(world, sceneFile);
	else openSceneDialog(dialogData, true);
}

void drawGeometry(int type) {
	switch(type) {
		case GEOM_TYPE_BOX: drawBoxRaw(); break;
		case GEOM_TYPE_SPHERE: drawSphereRaw(); break;
		case GEOM_TYPE_CYLINDER: drawCylinderRaw(); break;
		case GEOM_TYPE_CONE: drawConeRaw(); break;
	}
}