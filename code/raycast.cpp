

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
