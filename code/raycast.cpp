
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

	// Precalc.

	Vec3 emitColorLinear;
};

struct Geometry {
	int type;
	float boundingSphereRadius;
	Vec3 boundingBox;
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

	// Precalc.

	Vec3 colorLinear; // Converted from gamma space.
	bool isRotated;
	bool evenDim;
	Quat rotInverse;
	Vec3 dimScale;
	Vec3 dimScaleInverse;
};

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

// Not exact.
void geometryBoundingBox(Object* obj) {
	Quat rot = obj->rot;
	Vec3 dim = obj->dim;

	Vec3 points[] = {
		{ 0.5f, 0.5f, 0.5f },
		{ 0.5f, 0.5f,-0.5f },
		{ 0.5f,-0.5f, 0.5f },
		{ 0.5f,-0.5f,-0.5f },
		{-0.5f, 0.5f, 0.5f },
		{-0.5f, 0.5f,-0.5f },
		{-0.5f,-0.5f, 0.5f },
		{-0.5f,-0.5f,-0.5f },
	};

	for(int i = 0; i < arrayCount(points); i++) {
		points[i] = dim * points[i];
		points[i] = rot * points[i];
	}

	Vec3 minPos = vec3(FLT_MAX); 
	Vec3 maxPos = vec3(-FLT_MAX);
	for(int i = 0; i < arrayCount(points); i++) {
		minPos.x = min(minPos.x, points[i].x);
		minPos.y = min(minPos.y, points[i].y);
		minPos.z = min(minPos.z, points[i].z);
		maxPos.x = max(maxPos.x, points[i].x);
		maxPos.y = max(maxPos.y, points[i].y);
		maxPos.z = max(maxPos.z, points[i].z);
	}

	Vec3 bbox;
	bbox.x = maxPos.x*2;
	bbox.y = maxPos.y*2;
	bbox.z = maxPos.z*2;

	obj->geometry.boundingBox = bbox;
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

	Vec3 ambientLightColor;

	Vec3 globalLightDir;
	Vec3 globalLightColor;

	float focalPointDistance;
	float apertureSize;

	bool lockFocalPoint;
	Vec3 focalPoint;

	Vec2i globalLightRot;

	// Precalc.

	Vec3 ambientLightColorLinear;
	Vec3 globalLightColorLinear;
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

struct ObjectList {
	int* data;
	int count;
	int max;
};

// http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.42.3443&rep=rep1&type=pdf
struct SpatialGrid {
	Vec3 bMinStart;
	Vec3 bMaxStart;

	Vec3i coordMin;
	Vec3i coordMax;

	Vec3i bMin;
	Vec3i bMax;

	Vec3i cellCount;
	Vec3i cellSize;

	// Grid Traversal.

	Vec3 startPos;

	Vec3i coord;
	Vec3i step;
	Vec3 tMax;
	Vec3 tDelta;

	bool outside;
};

struct RaytraceSettings {
	Vec2i texDim;
	int sampleMode;
	int sampleCountGrid;
	int sampleGridWidth;
	int rayBouncesMaxWanted;

	int sampleCountWanted;

	int rayBouncesMax;
	bool darken;

	int* gridBuffer;
	int gridBufferCount;
	int gridBufferMax;

	SpatialGrid spatialGrid;
	ObjectList* grid;
	int gridCount;

	// Precalc.

	int randomDirectionCount;
	Vec3* randomDirections;
	int randomDirectionIndex;

	int randomUnitCirclePointCount;
	Vec2* randomUnitCirclePoints;
	int randomUnitIndex;

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


float lineSphereIntersectionX(Vec3 lp, Vec3 ld, Vec3 sp, float sr, Vec3* intersection, Vec3* intersectionNormal) {
	// ld must be unit.

	Vec3 oldP = lp;
	lp -= sp;

	float dotLdLp = dot(ld,lp);
	float lenLp = lenVec3(lp);
	float b = dotLdLp*dotLdLp - lenLp*lenLp + sr*sr;

	if(b < 0) return -1;

	float distance = -(dot(ld, lp));
	float sq = sqrt(b);

	// Get closest intersection that's not behind line position.

	if(distance - sq < 0) {
		if(distance + sq < 0) return -1;
		else distance += sq;
	} else {
		distance -= sq;
	}

	*intersection = oldP + ld*distance;
	*intersectionNormal = normVec3((*intersection) - sp);

    return distance;
}

Vec3 boxIntersectionNormals[6] = {vec3(-1,0,0), vec3(1,0,0), vec3(0,-1,0), vec3(0,1,0), vec3(0,0,-1), vec3(0,0,1)};
float lineBoxIntersection(Vec3 lp, Vec3 ld, Vec3 boxPos, Vec3 boxDim, Vec3* intersection, Vec3* intersectionNormal) {

	Vec3 boxHalfDim = boxDim/2;
	Vec3 boxMin = boxPos - boxHalfDim;
	Vec3 boxMax = boxPos + boxHalfDim;

	// ld is unit
	Vec3 dirfrac = 1.0f / ld;
	// lb is the corner of AABB with minimal coordinates - left bottom, rt is maximal corner
	// r.org is origin of ray
	float t1 = (boxMin.x - lp.x)*dirfrac.x;
	float t2 = (boxMax.x - lp.x)*dirfrac.x;
	float t3 = (boxMin.y - lp.y)*dirfrac.y;
	float t4 = (boxMax.y - lp.y)*dirfrac.y;
	float t5 = (boxMin.z - lp.z)*dirfrac.z;
	float t6 = (boxMax.z - lp.z)*dirfrac.z;

	float tmin = max(max(min(t1, t2), min(t3, t4)), min(t5, t6));
	float tmax = min(min(max(t1, t2), max(t3, t4)), max(t5, t6));

	// if tmax < 0, ray (line) is intersecting AABB, but whole AABB is behind us
	if (tmax < 0) return -1;

	// if tmin > tmax, ray doesn't intersect AABB
	if (tmin > tmax) return -1;

	float distance;
	if(tmin < 0) distance = tmax;
	else distance = tmin;

	*intersection = lp + ld*distance;

	     if(distance == t1) *intersectionNormal = boxIntersectionNormals[0];
	else if(distance == t2) *intersectionNormal = boxIntersectionNormals[1];
	else if(distance == t3) *intersectionNormal = boxIntersectionNormals[2];
	else if(distance == t4) *intersectionNormal = boxIntersectionNormals[3];
	else if(distance == t5) *intersectionNormal = boxIntersectionNormals[4];
	else if(distance == t6) *intersectionNormal = boxIntersectionNormals[5];

	return distance;
}

// http://www.cl.cam.ac.uk/teaching/1999/AGraphHCI/SMAG/node2.html#eqn:rectcylrayquad
float lineCylinderIntersection(Vec3 p, Vec3 d, Vec3* intersection, Vec3* intersectionNormal) {

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

	float distance = min(t1,t2);
	if(distance < 0) distance = max(t1,t2);

	*intersection = p + d*distance;

	// Normal.
	{
		int type = distance == t1 ? type1 : type2;

		if(type == 0) {
			if(intersection->xy == vec2(0,0)) *intersectionNormal = vec3(1,0,0);
			else *intersectionNormal = normVec3(vec3((*intersection).xy, 0));
		}
		else if(type == 1) *intersectionNormal = vec3(0,0,1);
		else *intersectionNormal = vec3(0,0,-1);
	}	

	return distance;
}

float lineConeIntersection(Vec3 p, Vec3 d, Vec3* intersection, Vec3* intersectionNormal) {

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

	float distance = min(t1,t2);
	if(distance < 0) distance = max(t1,t2);

	p.z += 0.5f;
	*intersection = p + d*distance;

	// Normal.
	{
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

	return distance;
}


// float lineTorusIntersection(Vec3 p, Vec3 d, Vec3* intersection = 0, Vec3* intersectionNormal = 0, bool secondIntersection = false) {

// 2*R*sqrt((x^2) + 2*t*x*a + (t^2)*(a^2) + (y^2) + 2*t*y*b + (t^2)*(b^2)) = (R^2) + (x^2) + 2*t*x*a+(t^2)*(a^2) + (y^2)+2*t*y*b+(t^2)*(b^2) + (z^2)+2*t*z*c+(t^2)*(c^2) - (r^2)

// }


void spatialGridInit(SpatialGrid* grid, Vec3 boxMin, Vec3 boxMax, Vec3i cellCount) {
	grid->bMinStart = boxMin;
	grid->bMaxStart = boxMax;
	grid->cellCount = cellCount;

	Vec3 dim = boxMax - boxMin;

	for(int i = 0; i < 3; i++) 
		grid->cellSize.e[i] = (int)((float)dim.e[i]/cellCount.e[i]);

	for(int i = 0; i < 3; i++) 
		grid->coordMin.e[i] = (roundInt(boxMin.e[i]/grid->cellSize.e[i] - 0.5f));

	for(int i = 0; i < 3; i++) 
		grid->coordMax.e[i] = (roundInt(boxMax.e[i]/grid->cellSize.e[i] + 0.5f));

	grid->cellCount = grid->coordMax - grid->coordMin;

	grid->bMin = grid->coordMin*grid->cellSize;
	grid->bMax = grid->coordMax*grid->cellSize;
}

void spatialGridTraverseInit(SpatialGrid* grid, Vec3 rayPos, Vec3 rayDir) {
	
	Vec3 bMin = vec3(grid->bMin);
	Vec3 bMax = vec3(grid->bMax);
	bool inGrid = pointInBox(rayPos, bMin, bMax);
	if(!inGrid) {
		Vec3 bDim = (bMax - bMin);
		Vec3 bPos = bMin + bDim/2;

		Vec3 intersection, intersectionNormal;
		float distance = lineBoxIntersection(rayPos, rayDir, bPos, bDim, &intersection, &intersectionNormal);
		if(distance != -1) {
			rayPos = intersection + rayDir*0.0001f;
			grid->startPos = rayPos;

			grid->outside = false;
		} else {
			grid->outside = true;
			return;
		}
	}

	for(int i = 0; i < 3; i++) {
		grid->coord.e[i] = (int)(rayPos.e[i]/grid->cellSize.e[i]) - grid->coordMin.e[i];
		if(rayPos.e[i] < 0) grid->coord.e[i]--;

		// Precision bug.
		if(grid->coord.e[i] == -1) grid->coord.e[i] = 0;
		if(grid->coord.e[i] >= grid->cellCount.e[i]) grid->coord.e[i]--;
	}

	for(int i = 0; i < 3; i++) {
		if(rayDir.e[i] == 0) grid->step.e[i] = 0;
		else grid->step.e[i] = rayDir.e[i] > 0 ? 1 : -1;
	}

	for(int i = 0; i < 3; i++) {
		int coord = grid->step.e[i] == -1 ? grid->coord.e[i] : grid->coord.e[i]+1;
		coord += grid->coordMin.e[i];
		if(grid->step.e[i] != 0) 
			grid->tMax.e[i] = ((coord * grid->cellSize.e[i]) - rayPos.e[i]) / rayDir.e[i];
		else grid->tMax.e[i] = FLT_MAX;
	}

	for(int i = 0; i < 3; i++) {
		grid->tDelta.e[i] = grid->cellSize.e[i] / rayDir.e[i];
		if(grid->tDelta.e[i] < 0) 
			grid->tDelta.e[i] = -grid->tDelta.e[i];
	}

}

bool spatialGridTraverseNext(SpatialGrid* grid) {

	if(grid->tMax.x < grid->tMax.y) {
		if(grid->tMax.x < grid->tMax.z) {
			grid->coord.x = grid->coord.x + grid->step.x;
			if(grid->coord.x < 0 || 
			   grid->coord.x >= grid->cellCount.x) return false;

			grid->tMax.x += grid->tDelta.x;
		} else {
			grid->coord.z = grid->coord.z + grid->step.z;
			if(grid->coord.z < 0 || 
			   grid->coord.z >= grid->cellCount.z) return false;

			grid->tMax.z += grid->tDelta.z;
		}
	} else {
		if(grid->tMax.y < grid->tMax.z) {
			grid->coord.y = grid->coord.y + grid->step.y;
			if(grid->coord.y < 0 || 
			   grid->coord.y >= grid->cellCount.y) return false;

			grid->tMax.y += grid->tDelta.y;
		} else {
			grid->coord.z = grid->coord.z + grid->step.z;
			if(grid->coord.z < 0 || 
			   grid->coord.z >= grid->cellCount.z) return false;

			grid->tMax.z += grid->tDelta.z;
		}
	}

	return true;
}




// Transform a vector to unit system and back:
//   -translate, -rotate, -scale -> scale, rotate, translate.
// Transform a direction to unit system and back:
//   -rotate, -scale -> -scale, rotate.

// Maybe we should just make a combined matrix?
inline void transformToUnitSpace(Object* obj, Vec3 pos, Vec3 dir, Vec3* posTransformed, Vec3* dirTransformed) {
	pos = pos - obj->pos;
	if(obj->isRotated) pos = obj->rotInverse * pos;
	pos = pos * obj->dimScaleInverse;
	*posTransformed = pos;

	if(obj->isRotated) dir = obj->rotInverse * dir;
	dir = dir * obj->dimScaleInverse;
	dir = normVec3(dir);
	*dirTransformed = dir;
}

inline void transformToGlobalSpace(Object* obj, Vec3* pos, Vec3* dir) {
	(*pos) = (*pos) * obj->dimScale;
	if(obj->isRotated) (*pos) = obj->rot * (*pos);
	(*pos) = (*pos) + obj->pos;

	(*dir) = (*dir) * obj->dimScaleInverse;
	if(obj->isRotated) (*dir) = obj->rot * (*dir);
	(*dir) = normVec3((*dir));
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

// #define printRaytraceTimings 1

#ifdef printRaytraceTimings
#define startTimer(i) startTimeStamp(pixelTimings + i)
#define endTimer(i) endTimeStamp(pixelTimings + i)
#else
#define startTimer(i)
#define endTimer(i)
#endif

float getIntersection(Object* obj, Vec3 rayPos, Vec3 rayDir, Vec3* position, Vec3* normal) {
	float distance = -1;

	switch(obj->geometry.type) {

		case GEOM_TYPE_SPHERE: {
			if(obj->evenDim) {
				distance = lineSphereIntersectionX(rayPos, rayDir, obj->pos, obj->dim.x*0.5f, position, normal);
			} else {

				Vec3 lp, ld;
				transformToUnitSpace(obj, rayPos, rayDir, &lp, &ld);

				distance = lineSphereIntersectionX(lp, ld, VEC3_ZERO, 0.5f, position, normal);

				if(distance != -1) {
					transformToGlobalSpace(obj, position, normal);
					distance = lenVec3(rayPos - (*position));
				}
			}
		} break;

		case GEOM_TYPE_BOX: {
			if(!obj->isRotated) {
				distance = lineBoxIntersection(rayPos, rayDir, obj->pos, obj->dim, position, normal);
			} else {

				Vec3 lp, ld;
				lp = rayPos - obj->pos;
				ld = lp + rayDir;
				lp = obj->rotInverse * lp;
				ld = obj->rotInverse * ld;
				ld = normVec3(ld - lp);

				distance = lineBoxIntersection(lp, ld, VEC3_ZERO, obj->dim, position, normal);

				if(distance != -1) {
					(*position) = obj->rot * (*position);
					(*position) = (*position) + obj->pos;

					(*normal) = obj->rot * (*normal);
					distance = lenVec3(rayPos - (*position));
				}
			}
		} break;

		case GEOM_TYPE_CYLINDER: {

			Vec3 lp, ld;
			transformToUnitSpace(obj, rayPos, rayDir, &lp, &ld);

			distance = lineCylinderIntersection(lp, ld, position, normal);

			if(distance != -1) {
				transformToGlobalSpace(obj, position, normal);
				distance = lenVec3(rayPos - (*position));
			}
		} break;

		case GEOM_TYPE_CONE: {

			Vec3 lp, ld;
			transformToUnitSpace(obj, rayPos, rayDir, &lp, &ld);

			distance = lineConeIntersection(lp, ld, position, normal);

			if(distance != -1) {
				transformToGlobalSpace(obj, position, normal);
				distance = lenVec3(rayPos - (*position));
			}
		} break;
	}

	return distance;
}

int castRayAll(Vec3 rayPos, Vec3 rayDir, DArray<Object>* objects, Vec3* intersection = 0, Vec3* intersectionNormal = 0) {

	int objectIndex = 0;
	float minDistance = FLT_MAX;
	for(int i = 0; i < objects->count; i++) {
		Object* obj = objects->data + i;
		Geometry* g = &obj->geometry;

		Vec3 position, normal;
		float distance = -1;

		bool possibleIntersection = lineSphereCollision(rayPos, rayDir, obj->pos, g->boundingSphereRadius);
		if(possibleIntersection) {

			distance = getIntersection(obj, rayPos, rayDir, &position, &normal);

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

int castRay(Vec3 rayPos, Vec3 rayDir, RaytraceSettings* settings, DArray<Object>* objects, Vec3* intersection = 0, Vec3* intersectionNormal = 0) {

	SpatialGrid* grid = &settings->spatialGrid;
	spatialGridTraverseInit(grid, rayPos, rayDir);

	if(grid->outside) return 0;

	int objectIndex = 0;
	float minDistance = FLT_MAX;

	do {
		int index = arrayIndex3D(grid->cellCount.x, grid->cellCount.y, 
		                         grid->coord.x, grid->coord.y, grid->coord.z);
		ObjectList* list = &settings->grid[index];
		for(int i = 0; i < list->count; i++) {

			Object* obj = objects->data + list->data[i];
			Geometry* g = &obj->geometry;

			Vec3 position, normal;
			float distance = -1;

			distance = getIntersection(obj, rayPos, rayDir, &position, &normal);

			if(distance > 0 && distance < minDistance) {
				minDistance = distance;
				objectIndex = list->data[i]+1;

				if(intersection) *intersection = position;
				if(intersectionNormal) *intersectionNormal = normal;
			}

		}

		if(objectIndex != 0) break;

	} while(spatialGridTraverseNext(grid));


	return objectIndex;
}

// Naive static bias.
float INTERSECTION_BIAS = 0.0001f;

void processPixelsThreaded(void* data) {
	TimeStamp pixelTimings[5] = {};

	pcg32_srandom(0, __rdtsc());

	ProcessPixelsData* d = (ProcessPixelsData*)data;

	World world = *d->world;
	RaytraceSettings settings = *d->settings;	

	Vec3* buffer = d->buffer;

	int sampleCount = settings.sampleCount;	
	Vec2* samples = settings.samples;
	Camera camera = world.camera;

	Vec2i texDim = settings.texDim;
	Vec2 texDimFloat = vec2(settings.texDim);

	Vec3 camOvecsRight = camera.ovecs.right;
	Vec3 camOvecsDown = -camera.ovecs.up;
	Vec3 camOvecsBack = -camera.ovecs.dir;
	Vec3 camRightWidth = camera.ovecs.right * camera.dim.w;
	Vec3 camDownHeight = camOvecsDown * camera.dim.h;

	Vec3 camRightOnePixel = camOvecsRight * camera.dim.w * (1.0f/texDim.w);
	Vec3 camDownOnePixel = camOvecsDown * camera.dim.h * (1.0f/texDim.h);

	world.globalLightDir = normVec3(world.globalLightDir);

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

		Vec2 percent = vec2(x/texDimFloat.w, y/texDimFloat.h);
		Vec3 camPixelPos = settings.camTopLeft + camRightWidth*percent.w + camDownHeight*percent.h;

		if(settings.sampleMode == SAMPLE_MODE_BLUE) {
			int index = (y%settings.sampleGridWidth)*settings.sampleGridWidth + (x%settings.sampleGridWidth);
			int offset = settings.sampleGridOffsets[index];
			samples = settings.samples + offset;
			sampleCount = settings.sampleGridOffsets[index+1] - offset;
		}

		int randomUnitIndex = randomIntPCG(0, settings.randomUnitCirclePointCount-1);

		// IACA_VC64_START

		Vec3 pixelColor = VEC3_ZERO;
		for(int sampleIndex = 0; sampleIndex < sampleCount; sampleIndex++) {
			startTimer(1);

			Vec2 sample = samples[sampleIndex];
			Vec3 rayPos = camPixelPos + camRightOnePixel * sample.x + 
			                            camDownOnePixel * sample.y;

			Vec3 rayDir;

			if(world.apertureSize == 0.0f) {
				rayDir = normVec3(rayPos - camera.pos);
			} else {
				Vec3 focalPoint;

				Vec3 rDir = rayPos - camera.pos;
				Vec3 focalPlanePos = camera.pos + camera.ovecs.dir * world.focalPointDistance;
				float distance = linePlaneIntersection(rayPos, rDir, focalPlanePos, camOvecsBack, &focalPoint);

				// int pointIndex = randomIntPCG(0, settings.randomUnitCirclePointCount-1);
				int pointIndex = randomUnitIndex % settings.randomUnitCirclePointCount;
				randomUnitIndex++;
				float randomOffsetX = settings.randomUnitCirclePoints[pointIndex].x * world.apertureSize;
				float randomOffsetY = settings.randomUnitCirclePoints[pointIndex].y * world.apertureSize;

				rayPos += camera.ovecs.right * randomOffsetX + 
				          camOvecsDown * randomOffsetY;

				rayDir = normVec3(focalPoint - rayPos);
			}


			// Trace path.

			Vec3 sampleColor = VEC3_ZERO;
			Vec3 attenuation = VEC3_ONE;

			for(int rayIndex = 0; rayIndex < settings.rayBouncesMax; rayIndex++) {

				// Find object with closest intersection.

				Vec3 intersection, intersectionNormal;
				int objectIndex = castRay(rayPos, rayDir, &settings, &world.objects, &intersection, &intersectionNormal);
				// int objectIndex = castRayAll(rayPos, rayDir, &world.objects, &intersection, &intersectionNormal);

				if(objectIndex != 0) {

					Object* obj = world.objects.data + (objectIndex-1);
					Material* m = &obj->material;

					// Color calculation.

					sampleColor += attenuation * m->emitColorLinear;

					if(m->refractiveIndex != 1.0f) {

						// Transparent objects.

						float ratio = fresnel(rayDir, intersectionNormal, m->refractiveIndex);

						Vec3 refractionDir = refract(rayDir, intersectionNormal, m->refractiveIndex);
						// Refract is not quite in sync with the fresnal calculation.
						if(refractionDir == VEC3_ZERO) ratio = 1;

						// Choose reflection/refraction direction based on ratio.

						Vec3 bias = intersectionNormal*INTERSECTION_BIAS;
						bool outside = dot(rayDir, intersectionNormal) < 0;

						float random = randomFloatPCG(0, 1.0f, 0.001f);

						if(random < ratio) {
							rayDir = reflectVector(rayDir, intersectionNormal);
							rayPos = intersection;
							rayPos += outside ? bias : -bias;
						} else {
							rayDir = refractionDir;
							rayPos = intersection;
							rayPos += outside ? -bias : bias;
						}

						attenuation = attenuation * obj->colorLinear;

					} else {

						// Reflection direction.

						rayDir = reflectVector(rayDir, intersectionNormal);

						if(m->reflectionMod != 1.0f) {

							// If not mirror, scatter randomly.

							int dirIndex = randomIntPCG(0, settings.randomDirectionCount-1);

							Vec3 randomDir = settings.randomDirections[dirIndex];

							float d = dot(randomDir, intersectionNormal);
							if(d <= 0) randomDir = reflectVector(randomDir, intersectionNormal);

							rayDir = lerp(m->reflectionMod, randomDir, rayDir);
						}

						rayPos = intersection + intersectionNormal*INTERSECTION_BIAS;

						float diffuseMod = lerp(1-m->reflectionMod, 1, dot(rayDir, intersectionNormal));
						attenuation = attenuation * obj->colorLinear * diffuseMod;

						// attenuation = attenuation * obj->colorLinear;
					}
					
					if(attenuation == VEC3_ZERO) break;

				} else if(rayIndex == 0) {

					// Sky hit.
					sampleColor += world.ambientLightColorLinear;

					break;

				} else {

					float lightDot = dot(rayDir, world.globalLightDir);
					lightDot = clampMin(lightDot, 0); 
					Vec3 light = world.globalLightColorLinear * lightDot;
					
					sampleColor += attenuation * (world.ambientLightColorLinear + light);

					break;
				}
			}

			pixelColor += clampMax(sampleColor, VEC3_ONE);

			endTimer(1);
		}

		// IACA_VC64_END

		buffer[y*texDim.w + x] = clampMax(pixelColor/(float)sampleCount, VEC3_ONE);

		if(settings.darken) {
			buffer[y*texDim.w + x] *= 0.2f;
		}

		endTimer(0);
	}

	#ifdef printRaytraceTimings
	for(int i = 0; i < arrayCount(pixelTimings); i++) {
		processPixelsThreadedTimings[i].cycles += pixelTimings[i].cycles;
		processPixelsThreadedTimings[i].hits += pixelTimings[i].hits;
		calcTimeStamp(pixelTimings + i);

		TimeStamp* stamp = pixelTimings + i;
		printf("%lld %lld %lld\n", stamp->cycles, stamp->hits, stamp->cyclesOverHits);
	}
	#endif
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

	world->ambientLightColor = vec3(1,1,1);
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
	
	geometryBoundingSphere(&obj);

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

void preCalcObjects(DArray<Object>* objects) {
	for(int i = 0; i < objects->count; i++) {
		Object* obj = objects->data + i;
		geometryBoundingSphere(obj);
		geometryBoundingBox(obj);

		obj->colorLinear = colorSRGB(obj->color);
		obj->isRotated = !(obj->rot == quat());
		obj->evenDim = equal(obj->dim.x, obj->dim.y, obj->dim.z);
		obj->material.emitColorLinear = colorSRGB(obj->material.emitColor);
		obj->rotInverse = quatInverse(obj->rot);

		// Scale transforms dimension from render dimension to
		// intersection calculation dimension.
		// e.g: We render a cone with radius 0.5f and height 1, but 
		// the intersection equation want's a cone with radius 1 and height 1;

		switch(obj->geometry.type) {
			case GEOM_TYPE_SPHERE: {
				Vec3 scale = vec3(1);
				obj->dimScale = obj->dim/scale;
				obj->dimScaleInverse = scale/obj->dim;
			} break;

			case GEOM_TYPE_BOX: {
				Vec3 scale = vec3(1);
				obj->dimScale = obj->dim/scale;
				obj->dimScaleInverse = scale/obj->dim;
			} break;

			case GEOM_TYPE_CYLINDER: {
				Vec3 scale = vec3(2);
				obj->dimScale = obj->dim/scale;
				obj->dimScaleInverse = scale/obj->dim;
			} break;

			case GEOM_TYPE_CONE: {
				Vec3 scale = vec3(2,2,1);
				obj->dimScale = obj->dim/scale;
				obj->dimScaleInverse = scale/obj->dim;
			} break;
		}

	}
}



