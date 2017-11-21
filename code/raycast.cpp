

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
	float reflectionMod; // In percent.
	bool emitsLight;

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


bool lineShapeIntersection(Vec3 lp, Vec3 ld, Shape shape, Vec3* reflectionPos, Vec3* reflectionDir) {

	switch(shape.type) {
		case SHAPE_BOX: {
			float distance;
			bool hit = boxRaycast(lp, ld, rect3CenDim(shape.pos, shape.dim), &distance);
			if(hit) {
				// *reflectionPos = lp + ld*distance;
				// reflectVector(

				// *reflectionVector = lp

				*reflectionPos = vec3(0,0,0);
				*reflectionDir = vec3(0,0,0);

				return true;
			}
		} break;

		case SHAPE_SPHERE: {
			bool hit = lineSphereIntersection(lp, lp + ld*1000, shape.pos, shape.r, reflectionPos);
			if(hit) {
				Vec3 normal = *reflectionPos - shape.pos;
				*reflectionDir = reflectVector(ld, normal);
				
				return true;
			}
		}

		default: break;
	}

	return false;
}
