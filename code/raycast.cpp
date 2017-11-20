

enum {
	SHAPE_BOX = 0,
	SHAPE_SPHERE,

	SHAPE_COUNT,
};

union Shape {
	int type;

	struct {
		Vec3 pos;
		Vec3 dim;
		Quat rot;
	};

	struct {
		Vec3 pos;
		float r;
	};

};