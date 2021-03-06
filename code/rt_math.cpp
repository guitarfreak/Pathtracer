#pragma once
#include <math.h>

#define M_E         2.7182818284590452354               // e
#define M_LOG2E     1.4426950408889634074               // log_2 e
#define M_LOG10E    0.43429448190325182765              // log_10 e
#define M_LN2       0.69314718055994530942              // log_e 2
#define M_LN10      2.30258509299404568402              // log_e 10
#define M_2PI       6.2831853071795864769252867665590   // 2*pi
#define M_PI        3.1415926535897932384626433832795   // pi
#define M_3PI_2		4.7123889803846898576939650749193	// 3/2*pi
#define M_PI_2      1.5707963267948966192313216916398   // pi/2
#define M_PI_4      0.78539816339744830962              // pi/4
#define M_1_PI      0.31830988618379067154              // 1/pi
#define M_2_PI      0.63661977236758134308              // 2/pi
#define M_2_SQRTPI  1.12837916709551257390              // 2/sqrt(pi)
#define M_SQRT2     1.41421356237309504880              // sqrt(2)
#define M_SQRT1_2   0.70710678118654752440              // 1/sqrt(2)
#define M_PI_180    0.0174532925199432957692369076848   // pi/180
#define M_180_PI    57.295779513082320876798154814105   // 180/pi
#define M_BOLTZ		1.44269504							// boltzmann constant

#define swapGeneric(type, a, b) 	\
		type swap##type = a;		\
		a = b;							\
		b = swap##type;



int mod(int a, int b) {
	int result;
	result = a % b;
	if(result < 0) result += b;

	return result;
}

float modFloat(float val, float d) {
	// fmod seems to be wrong.
	float result = fmod(val, d);
	if(result < 0 && abs(result) < d) {
		result = d + result;
	}

	return result;
}

inline void swap(int* a, int* b) {
	int temp = *a;
	*a = *b;
	*b = temp;
}

inline void swap(float* a, float* b) {
	float temp = *a;
	*a = *b;
	*b = temp;
}

inline float min(float a, float b) {
	return a <= b ? a : b;
}

inline float min(float a, float b, float c) {
	return min(min(a, b), min(b, c));
}

inline float max(float a, float b) {
	return a >= b ? a : b;
}

inline float max(float a, float b, float c) {
	return max(max(a, b), max(b, c));
}

inline int minInt(int a, int b) {
	return a <= b ? a : b;
}

inline int maxInt(int a, int b) {
	return a >= b ? a : b;
}

inline int maxReturnIndex(float a, float b, float c) {
	float result = max(a,b,c);
	int index;
	if(result == a) index = 0;
	else if(result == b) index = 1;
	else index = 2;
	return index;
}

inline float equal(float a, float b, float c) {
	return a == b && a == c;
}

inline float diff(float a, float b) {
	return abs(a - b);
}

inline bool sameSign(float a, float b) {
	return (a < 0 && b < 0) || (a > 0 && b > 0); 
}

// inline float clampMin(float min, float a) {
inline float clampMin(float a, float min) {
	return a < min ? min : a;
}

inline void clampMin(float* a, float min) {
	if(*a < min) *a = min;
}

inline float clampMax(float a, float max) {
	return a > max ? max : a;
}

inline void clampMax(float* a, float max) {
	if(*a > max) *a = max;
}

inline float clamp(float n, float min, float max) {
	float result = clampMax(clampMin(n, min), max);
	return result;
};

inline void clamp(float* n, float min, float max) {
	*n = clampMax(clampMin(*n, min), max);
};


inline double clampMinDouble(double a, double min) {
	return a < min ? min : a;
}

inline void clampMinDouble(double* a, double min) {
	if(*a < min) *a = min;
}

inline double clampMaxDouble(double a, double max) {
	return a > max ? max : a;
}

inline void clampMaxDouble(double* a, double max) {
	if(*a > max) *a = max;
}

inline double clampDouble(double n, double min, double max) {
	double result = clampMaxDouble(clampMinDouble(n, min), max);
	return result;
};

inline void clampDouble(double* n, double min, double max) {
	*n = clampMaxDouble(clampMinDouble(*n, min), max);
};


inline int clampMinInt(int a, int min) {
	return a < min ? min : a;
}

inline void clampMinInt(int* a, int min) {
	*a < min ? min : *a;
}

inline int clampMaxInt(int a, int max) {
	return a > max ? max : a;
}

inline void clampMaxInt(int* a, int max) {
	*a > max ? max : *a;
}

inline int clampInt(int n, int min, int max) {
	int result = clampMaxInt(clampMinInt(n, min), max);
	return result;
};

inline void clampInt(int* n, int min, int max) {
	*n = clampMax(clampMinInt(*n, min), max);
};

inline float mapRange(float value, float min, float max, float rangeMin, float rangeMax) {
	float off = min < 0 ? abs(min) : -abs(min);
	float result = ((value+off)/((max+off)-(min+off))) * (rangeMax-rangeMin) + rangeMin;

	return result;
};

inline double mapRangeDouble(double value, double min, double max, double rangeMin, double rangeMax) {
	double off = min < 0 ? abs(min) : -abs(min);
	double result = ((value+off)/((max+off)-(min+off))) * (rangeMax-rangeMin) + rangeMin;

	return result;
};

inline float mapRange01(float value, float min, float max) {
	float off = min < 0 ? abs(min) : -abs(min);
	float result = ((value+off)/((max+off)-(min+off)));

	return result;
};

// inline float mapRangeU64(u64 value, u64 min, u64 max, u64 rangeMin, u64 rangeMax) {
// 	u64 off = min;
// 	u64 result = ((value+off)/((max+off)-(min+off))) * (rangeMax-rangeMin) + rangeMin;

// 	return result;
// };

inline float mapRangeClamp(float value, float min, float max, float rangeMin, float rangeMax) {
	float result = mapRange(value, min, max, rangeMin, rangeMax);
	result = clamp(result, rangeMin, rangeMax);

	return result;
};

float lerp(float percent, float min, float max) {
	return min + percent * (max-min);
}

float lerpCheck(float percent, float min, float max) {
	if(percent == 0) return min;
	else if(percent == 1) return max;
	else return min + percent * (max-min);
}

inline bool valueBetween(float v, float min, float max) {
	return (v >= min && v <= max);
}

inline bool valueBetweenInt(int v, int min, int max) {
	return (v >= min && v <= max);
}

inline bool valueBetweenDouble(double v, double min, double max) {
	return (v >= min && v <= max);
}

inline bool valueBetween2(float v, float min, float max) {
	bool result = (v > min && v <= max);
	return result;
}

inline float roundFloat(float f, int x) {
	return floor(f*x + 0.5f) / x;
};

/** a roughly b */
inline bool roughlyEqual(float a, float b, float margin) {
	return (a > b - margin) && (a < b + margin);
};

inline float radianToDegree(float angle) {
	return angle*((float)180 / M_PI);
}

inline float degreeToRadian(float angle) {
	return angle*((float)M_PI / 180);
}

inline float sign(float p1[2], float p2[2], float p3[2]) {
	return (p1[0] - p3[0]) * (p2[1] - p3[1]) - (p2[0] - p3[0]) * (p1[1] - p3[1]);
}

inline float sign(float x) {
	int s;
	if(x < 0) s = -1;
	if(x > 0) s = 1;
	else s = 0;

	return s; 
}	

inline bool pointIsLeftOfLine(float a[2], float b[2], float c[2]) {
	return ((b[0] - a[0])*(c[1] - a[1]) - (b[1] - a[1])*(c[0] - a[0])) > 0;
}

inline int round(int i) {
	return (int)floor(i + 0.5f);
}

inline float roundFloat(float i) {
	return (int)floor(i + 0.5f);
}

inline float roundUpFloat(float i) {
	return ceil(i);
}

inline int roundUpInt(float i) {
	return ceil(i);
}

inline float roundDownFloat(float i) {
	float result = i < 0 ? ((int)i)-1 : (int)i;
	return result;
}
inline float truncateFloat(float i) {
	return (float)((int)i);
}

inline double roundDouble(double i) {
	return floor(i + 0.5f);
}

inline double roundDownDouble(double i) {
	return (double)((int)i);
}

inline float roundMod(float i, float val) {
	return (roundFloat(i/val))*val;
}

inline float roundModDown(float i, float val) {
	return ((int)(i/val))*val;
}

inline double roundModDouble(double i, double val) {
	return (roundDouble(i/val))*val;
}

inline int roundInt(float i) {
	return (int)floor(i + 0.5f);
}

inline int triangularNumber(int n) {
	return n*(n+1) / 2;
}

inline double divZero(double a, double b) {
	if(a == 0 || b == 0) return 0;
	else return a/b;
}

inline float root(float a, float root) {
	return pow(a, 1/root);
}

inline float camDistanceFromFOVandWidth(float fovInDegrees, float w) {
	float angle = degreeToRadian(fovInDegrees);
	float sideAngle = ((M_PI-angle)/2.0f);
	float side = w/sin(angle) * sin(sideAngle);
	float h = side*sin(sideAngle);
	
	return h;
}

//

inline int colorFloatToInt(float color) {
	return (int)round(color * 255);
};

inline float colorIntToFloat(int color) {
	return ((float)1 / 255) * color;
};

inline uint mapRGB(int r, int g, int b) {
	return (r << 16 | g << 8 | b);
};

uint mapRGBA(float r, float g, float b, float a) {
	int ri = colorFloatToInt(r);
	int gi = colorFloatToInt(g);
	int bi = colorFloatToInt(b);
	int ai = colorFloatToInt(a);
	return (ri << 24 | gi << 16 | bi << 8 | ai);
}

inline uint mapRGB(float r, float g, float b) {
	return mapRGB(colorFloatToInt(r), colorFloatToInt(g), colorFloatToInt(b));
}

inline uint mapRGB(float color[3]) {
	return mapRGB(color[0], color[1], color[2]);
}

inline uint mapRGBA(float color[4]) {
	return mapRGBA(color[0], color[1], color[2], color[3]);
}

inline uint mapRGBA(float c) {
	return mapRGBA(c,c,c,c);
}

inline void colorAddAlpha(uint &color, float alpha) {
	color = (color << 8) + colorFloatToInt(alpha);
}

void rgbToHsl(float color[3], double r, double g, double b) {
	//NOTE: color[3] = {h, s, l}

	double M = 0.0, m = 0.0, c = 0.0;
	M = max(r, g, b);
	m = min(r, g, b);
	c = M - m;
	color[2] = 0.5 * (M + m);
	if (c != 0.0)
	{
		if (M == r)
		{
			// color[0] = fmod(((g - b) / c), 6.0);
			color[0] = modFloat(((g - b) / c), 6.0);
		}
		else if (M == g)
		{
			color[0] = ((b - r) / c) + 2.0;
		}
		else/*if(M==b)*/
		{
			color[0] = ((r - g) / c) + 4.0;
		}
		color[0] *= 60.0;
		color[1] = c / (1.0 - fabs(2.0 * color[2] - 1.0));
	}
	else
	{
		color[0] = 0;
		color[1] = 0;
		color[2] = r;
	}
}

void vSet3(float res[3], float x, float y, float z);
void hslToRgb(float color[3], double h, double s, double l) {
	double c = 0.0, m = 0.0, x = 0.0;
	c = (1.0 - fabs(2 * l - 1.0)) * s;
	m = 1.0 * (l - 0.5 * c);
	// x = c * (1.0 - fabs(fmod(h / 60.0, 2) - 1.0));
	x = c * (1.0 - fabs(modFloat(h / 60.0, 2) - 1.0));
	if (h == 360) h = 0;
	if (h >= 0.0 && h < 60)  vSet3(color, c + m, x + m, m);
	else if (h >= 60 && h < 120) vSet3(color, x + m, c + m, m);
	else if (h >= 120 && h < 180) vSet3(color, m, c + m, x + m);
	else if (h >= 180 && h < 240) vSet3(color, m, x + m, c + m);
	else if (h >= 240 && h < 300) vSet3(color, x + m, m, c + m);
	else if (h >= 300 && h < 360) vSet3(color, c + m, m, x + m);
	else vSet3(color, m, m, m);
}

void hueToRgb(float color[3], double h) {
	if (h >= 0.0 && h < 60)  {
		float x = mapRange(h, 0, 60, 0, 1);
		vSet3(color, 1, x, 0);
	}
	else if (h >= 60 && h < 120) {
		float x = mapRange(h, 60, 120, 1, 0);
		vSet3(color, x, 1, 0);
	}
	else if (h >= 120 && h < 180) {
		float x = mapRange(h, 120, 180, 0, 1);
		vSet3(color, 0, 1, x);
	}
	else if (h >= 180 && h < 240) {
		float x = mapRange(h, 180, 240, 1, 0);
		vSet3(color, 0, x, 1);
	}
	else if (h >= 240 && h < 300) {
		float x = mapRange(h, 240, 300, 0, 1);
		vSet3(color, x, 0, 1);
	}
	else if (h >= 300 && h < 360) {
		float x = mapRange(h, 300, 360, 1, 0);
		vSet3(color, 1, 0, x);
	}
	else vSet3(color, 1, 0, 0);
}

float colorGet(uint color, int i) {
	if (i < 0 || i > 3) {
		printf("colorGet wants to access element: %f", (float)i);
		return -1;
	}
	int colorChannel = color >> ((3 - i) * 8) & 255;
	float colorChannelFloat = colorIntToFloat(colorChannel);
	return colorChannelFloat;
}

void colorGetRGB(uint color, float v[3]) {
	v[0] = colorIntToFloat(color >> 24 & 255);
	v[1] = colorIntToFloat(color >> 16 & 255);
	v[2] = colorIntToFloat(color >> 8 & 255);
}

void colorGetRGBA(uint color, float v[4]) {
	v[0] = colorIntToFloat(color >> 24 & 255);
	v[1] = colorIntToFloat(color >> 16 & 255);
	v[2] = colorIntToFloat(color >> 8 & 255);
	v[3] = colorIntToFloat(color & 255);
}

void colorAdd(uint &c1, uint c2) {
	float v1[4], v2[4];
	colorGetRGBA(c1, v1);
	colorGetRGBA(c2, v2);
	for (int i = 0; i < 3; ++i) {
		v1[i] += v2[i];
		v1[i] = clamp(v1[i], 0, 1);
	}
	c1 = mapRGBA(v1);
}

int colorSet(uint &color, int i, float f) {
	if (i < 0 || i > 3) {
		printf("colorGet wants to access element: %f", (float)i);
		return -1;
	}

	float vec[4];
	colorGetRGBA(color,vec);
	vec[i] = f;
	color = mapRGBA(vec);
	return 0;
}

void colorSetRGB(uint &color, float r, float g, float b) {
	color = mapRGBA(r, g, b, colorGet(color, 3));
}

void colorSetRGB(uint &color, float rgb[3]) {
	colorSetRGB(color, rgb[0], rgb[1], rgb[2]);
}

void colorSetAlpha(uint &color, float a) {
	color = (color ^ (color & 255)) | colorFloatToInt(a);
}

float linearTween(float t, float b, float c, float d) {
	return c*t / d + b;
};

float easeInQuad(float t, float b, float c, float d) {
	t /= d;
	return c*t*t + b;
};

float easeOutQuad(float t, float b, float c, float d) {
	t /= d;
	return -c * t*(t - 2) + b;
};

float easeInOutQuad(float t, float b, float c, float d) {
	t /= d / 2;
	if (t < 1) return c / 2 * t*t + b;
	t--;
	return -c / 2 * (t*(t - 2) - 1) + b;
};

float easeInCubic(float t, float b, float c, float d) {
	t /= d;
	return c*t*t*t + b;
};

float easeOutCubic(float t, float b, float c, float d) {
	t /= d;
	t--;
	return c*(t*t*t + 1) + b;
};

float easeInOutCubic(float t, float b, float c, float d) {
	t /= d / 2;
	if (t < 1) return c / 2 * t*t*t + b;
	t -= 2;
	return c / 2 * (t*t*t + 2) + b;
};

float easeInQuart(float t, float b, float c, float d) {
	t /= d;
	return c*t*t*t*t + b;
};

float easeOutQuart(float t, float b, float c, float d) {
	t /= d;
	t--;
	return -c * (t*t*t*t - 1) + b;
};

float easeInOutQuart(float t, float b, float c, float d) {
	t /= d / 2;
	if (t < 1) return c / 2 * t*t*t*t + b;
	t -= 2;
	return -c / 2 * (t*t*t*t - 2) + b;
};

float easeInQuint(float t, float b, float c, float d) {
	t /= d;
	return c*t*t*t*t*t + b;
};

float easeOutQuint(float t, float b, float c, float d) {
	t /= d;
	t--;
	return c*(t*t*t*t*t + 1) + b;
};

float easeInOutQuint(float t, float b, float c, float d) {
	t /= d / 2;
	if (t < 1) return c / 2 * t*t*t*t*t + b;
	t -= 2;
	return c / 2 * (t*t*t*t*t + 2) + b;
};

float easeInSine(float t, float b, float c, float d) {
	return -c * cos(t / d * M_PI_2) + c + b;
};

float easeOutSine(float t, float b, float c, float d) {
	return c * sin(t / d * M_PI_2) + b;
};

float easeInOutSine(float t, float b, float c, float d) {
	return -c / 2 * (cos(M_PI*t / d) - 1) + b;
};

float easeInExpo(float t, float b, float c, float d) {
	return c * pow(2, 10 * (t / d - 1)) + b;
};

float easeOutExpo(float t, float b, float c, float d) {
	return c * (-pow(2, -10 * t / d) + 1) + b;
};

float easeInOutExpo(float t, float b, float c, float d) {
	t /= d / 2;
	if (t < 1) return c / 2 * pow(2, 10 * (t - 1)) + b;
	t--;
	return c / 2 * (-pow(2, -10 * t) + 2) + b;
};

float easeInCirc(float t, float b, float c, float d) {
	t /= d;
	return -c * (sqrt(1 - t*t) - 1) + b;
};

float easeOutCirc(float t, float b, float c, float d) {
	t /= d;
	t--;
	return c * sqrt(1 - t*t) + b;
};

float easeInOutCirc(float t, float b, float c, float d) {
	t /= d / 2;
	if (t < 1) return -c / 2 * (sqrt(1 - t*t) - 1) + b;
	t -= 2;
	return c / 2 * (sqrt(1 - t*t) + 1) + b;
};


float cubicBezierInterpolationSeemless(float A, float B, float C, float D, float t) {
	float a = -A/2.0f + (3.0f*B)/2.0f - (3.0f*C)/2.0f + D/2.0f;
	float b = A - (5.0f*B)/2.0f + 2.0f*C - D / 2.0f;
	float c = -A/2.0f + C/2.0f;
	float d = B;
	
	return a*t*t*t + b*t*t + c*t + d;
}


int randomInt(int from, int to) {
	return rand() % (to - from + 1) + from;
};

float randomFloat(float from, float to, float offset) {
	float oneOverOffset = 1/offset;
	from *= oneOverOffset;
	to *= oneOverOffset;
	int rInt = randomInt(from, to);
	// float result = (float)(rand() % (int)((to - from + 1) + from)) * offset;
	float result = (float)rInt * offset;
	return result;
};

// inline double mapRangePrecise(double value, double min, double max, double rangeMin, double rangeMax) {
// 	double offset = 0;
// 	const double width = max - min;
// 	const double rangeWidth = rangeMax - rangeMin;
// 	if (min < 0) offset = abs(min);
// 	if (min > 0) offset = -abs(min);
// 	value += offset;
// 	min += offset;
// 	max += offset;
// 	return ((double)value / width)*rangeWidth + rangeMin;
// };

// angle is radian
float vectorToAngle(float vector[2]) {
	float angle = atan2(vector[1], vector[0]);
	if (angle < 0) angle = M_2PI - abs(angle);
	return angle;
}

bool pointInTriangle(float pt[2], float v1[2], float v2[2], float v3[2]) {
	bool b1, b2, b3;

	b1 = sign(pt, v1, v2) < 0.0f;
	b2 = sign(pt, v2, v3) < 0.0f;
	b3 = sign(pt, v3, v1) < 0.0f;

	return ((b1 == b2) && (b2 == b3));
}

//
// Vectors
//

// inline float* vec4Init(float a, float b, float c, float d)
// {
// 	float v[4] = { a, b, c, d };
// 	return v;
// }

inline void vSet4(float res[4], float a, float b, float c, float d) {
	res[0] = a;
	res[1] = b;
	res[2] = c;
	res[3] = d;
}

inline void vSet3(float res[3], float x, float y, float z) {
	res[0] = x;
	res[1] = y;
	res[2] = z;
}

inline void vSet3(float res[3], const float vec[3]) {
	res[0] = vec[0];
	res[1] = vec[1];
	res[2] = vec[2];
}

inline void vSet2(float res[2], const float vec[2]) {
	res[0] = vec[0];
	res[1] = vec[1];
}

inline void vSet2(float res[2], float x, float y) {
	res[0] = x;
	res[1] = y;
}

inline void vSet4(float res[4], const float vec[4]) {
	res[0] = vec[0];
	res[1] = vec[1];
	res[2] = vec[2];
	res[3] = vec[3];
}

inline void vSub3(float a_less_b[3], const float a[3], const float b[3]) {
	a_less_b[0] = a[0]-b[0];
	a_less_b[1] = a[1]-b[1];
	a_less_b[2] = a[2]-b[2];
}

inline void vSub3(float a[3], const float b[3]) {
	a[0] -= b[0];
	a[1] -= b[1];
	a[2] -= b[2];
}

inline void vClamp3(float a[3], float min, float max) {
	for(int dim=0; dim<3; dim++)
		if(a[dim]<min) a[dim]=min; 
		else if(a[dim]>max) a[dim] = max;
}

inline void vAdd3(float a_plus_b[3], const float a[3], const float b[3]) {
	a_plus_b[0] = a[0]+b[0];
	a_plus_b[1] = a[1]+b[1];
	a_plus_b[2] = a[2]+b[2];
}

inline void vAdd3(float a[3], const float b[3]) {
	a[0] += b[0];
	a[1] += b[1];
	a[2] += b[2];
}

inline void vAdd3(float a[3], const float b) {
	a[0] += b;
	a[1] += b;
	a[2] += b;
}

// Multiply-then add
inline void vMAD3(float a_plus_b_times_s[3], const float a[3], const float b[3], float s) {
	a_plus_b_times_s[0] = a[0]+b[0]*s;
	a_plus_b_times_s[1] = a[1]+b[1]*s;
	a_plus_b_times_s[2] = a[2]+b[2]*s;
}

inline void vMAD3(float a[3], const float b[3], float s) {
	a[0] += b[0]*s;
	a[1] += b[1]*s;
	a[2] += b[2]*s;
}

inline void vInterpol3(float a_times_u_plus_b_times_v[3], 
					   const float a[3], float u, 
					   const float b[3], float v) {
	a_times_u_plus_b_times_v[0] = a[0]*u+b[0]*v;
	a_times_u_plus_b_times_v[1] = a[1]*u+b[1]*v;
	a_times_u_plus_b_times_v[2] = a[2]*u+b[2]*v;
}

inline void vInterpolTri3(float a_times_u_plus_b_times_v_plus_c_times_w[3], 
						  const float a[3], float u, 
						  const float b[3], float v, 
						  const float c[3], float w) {
	a_times_u_plus_b_times_v_plus_c_times_w[0] = a[0]*u+b[0]*v+c[0]*w;
	a_times_u_plus_b_times_v_plus_c_times_w[1] = a[1]*u+b[1]*v+c[1]*w;
	a_times_u_plus_b_times_v_plus_c_times_w[2] = a[2]*u+b[2]*v+c[2]*w;
}

inline void vScl3(float a_times_s[3], const float a[3], float s) {
	a_times_s[0] = a[0]*s;
	a_times_s[1] = a[1]*s;
	a_times_s[2] = a[2]*s;
}

inline void vScl3(float a[3], float s) {
	a[0]*=s;
	a[1]*=s;
	a[2]*=s;
}

inline void vScl3(float a_times_s[3], const float a[3], float s[3]) {
	a_times_s[0] = a[0]*s[0];
	a_times_s[1] = a[1]*s[1];
	a_times_s[2] = a[2]*s[2];
}

inline void vScl3(float a[3], const float s[3]) {
	a[0]*=s[0];
	a[1]*=s[1];
	a[2]*=s[2];
}

inline float vDot3(const float a[3], const float b[3]) {
	return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];
}
inline float vDot3(float a[3], float b[3]) {
	return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];
}

inline void vCross3(float cp[3], const float a[3], const float b[3]) {
	// assert(cp!=a && cp!=b);

	cp[0]=a[1]*b[2]-a[2]*b[1];
	cp[1]=a[2]*b[0]-a[0]*b[2];
	cp[2]=a[0]*b[1]-a[1]*b[0];
}

inline float vSqrLen3(const float a[3]) {
	return vDot3(a,a);
}

inline float vSqrLen3(const float a[3], const float b[3]) {
	return (a[0]-b[0])*(a[0]-b[0])
		+(a[1]-b[1])*(a[1]-b[1])
		+(a[2]-b[2])*(a[2]-b[2]);
}

inline float vLen3(const float a[3]) {
	return sqrt(vDot3(a, a));
}
inline float vLen3(float a[3]) {
	return sqrt(vDot3(a,a));
}

inline float vLen3(const float a[3], const float b[3]) {
	return sqrt(vSqrLen3(a, b));
}

inline float vLen2(const float a[2], const float b[2]) {
	float a3[] = { a[0], a[1], 0 };
	float b3[] = { b[0], b[1], 0 };
	return vLen3(a, b);
}

inline float vLen2(float a[2]) {
	return sqrt(a[0] * a[0] + a[1] * a[1]);
}

inline void vNorm3(float a_norm[3], const float a[3]) {
	float sqrlen = vDot3(a,a);
	if(sqrlen>0) sqrlen = 1.0f/sqrt(sqrlen);
	vScl3(a_norm,a,sqrlen);
}

inline void vNorm3(float a[3]) {
	float sqrlen = vDot3(a, a);
	if (sqrlen>0) sqrlen = 1.0f / sqrt(sqrlen);
	vScl3(a, sqrlen);
}

inline void vNorm2(float a[2]) {
	float length = vLen2(a);
	a[0] /= length;
	a[1] /= length;
}

inline void vFaceNormal(float n[3], const float a[3], const float b[3], const float c[3]) {
	float u[3], v[3];
	vSub3(u,b,a);
	vSub3(v,c,a);
	vCross3(n,u,v);
}

inline void vFaceCenter(float n[3], const float a[3], const float b[3], const float c[3]) {
	n[0] = (1/(float)3) * (a[0] + b[0] + c[0]);
	n[1] = (1/(float)3) * (a[1] + b[1] + c[1]);
	n[2] = (1/(float)3) * (a[2] + b[2] + c[2]);
}

inline bool vEqual(float a[3], float b[3]) {
	return ((a[0] == b[0]) && (a[1] == b[1]) && (a[2] == b[2]));  
}

inline bool vEqual(float a[3], float b[3], int roundingFactor) {
	auto roundfloat = [](float f, int x) { return floor(f*x+0.5)/x; };
	return (roundfloat(a[0], roundingFactor) == roundfloat(b[0], roundingFactor)) 
		&& (roundfloat(a[1], roundingFactor) == roundfloat(b[1], roundingFactor))
		&& (roundfloat(a[2], roundingFactor) == roundfloat(b[2], roundingFactor));
}

inline bool vEqual(float a[3], float x, float y, float z) {
	return ((a[0] == x) && (a[1] == y) && (a[2] == z));  
}

inline void vRotate2(float a[2], int degrees) {
	// TODO: cant use degreeToRadian function...
	float radians = degrees*((float)M_PI / 180);
	float length = vLen2(a);
	float cs = cos(radians);
	float sn = sin(radians);
	float px = a[0] * cs - a[1] * sn;
	float py = a[0] * sn + a[1] * cs;
	a[0] = px;
	a[1] = py;
}

inline float vAngleR(float a[3], float b[3]) {
	float x = vDot3(a,b);
	float y = (vLen3(a) * vLen3(b));
	float angle = acos(clamp(x/y, -1, 1));
	return angle;
}

inline float vAngleR2d(float a[3], float b[3]) {
	return atan2(b[1] - a[1], b[0] - a[0]);
}

inline float vAngleR(float v[3], float v1[3], float v2[3]) {
	float a[3], b[3];
	vSub3(a, v1, v);
	vSub3(b, v2, v);
	return vAngleR(a, b);
}

inline float vAngleD(float a[3], float b[3]) {
	float x = vDot3(a,b);
	float y = (vLen3(a) * vLen3(b));
	float angle = acos(clamp(x/y, -1, 1));
	angle = angle * ((float)180/M_PI);
	return angle;
}

inline float vAngleD(float v[3], float v1[3], float v2[3]) {
	float a[3], b[3];
	vSub3(a, v1, v);
	vSub3(b, v2, v);
	return vAngleD(a, b);
}

inline void vCircumcenter2d(float center[3], float a[3], float b[3], float c[3]) {
	if(!(a[0] == 0 && a[1] == 0)) return;

	float x = c[1]*(pow(b[0],2) + pow(b[1],2)) - b[1]*(pow(c[0],2) + pow(c[1],2));
	float y = b[0]*(pow(c[0],2) + pow(c[1],2)) - c[0]*(pow(b[0],2) + pow(b[1],2));
	float d = 2*(b[0]*c[1] - b[1]*c[0]);
	x = x / d;
	y = y / d;
	center[0] = x;
	center[1] = y;
	return;
}

inline float vCircumcenter3d(float center[3], float a[3], float b[3], float c[3]) {

	//		  |c-a|^2 [(b-a)x(c-a)]x(b-a) + |b-a|^2 (c-a)x[(b-a)x(c-a)]
	//m = a + ---------------------------------------------------------.
	//						    2 | (b-a)x(c-a) |^2

	//Vector3f a,b,c // are the 3 pts of the tri

	float ac[3], ab[3], abXac[3], toCircumsphereCenter[3];
	vSub3(ac, c, a);
	vSub3(ab, b, a);
	vCross3(abXac, ab, ac);

	// this is the vector from a TO the circumsphere center
	float t1[3], t2[3];
	vCross3(t1, abXac, ab);
	vCross3(t2, ac, abXac);
	vScl3(t1, pow(vLen3(ac),2));
	vScl3(t2, pow(vLen3(ab),2));
	vAdd3(t1, t2);
	vScl3(t1, (float)1/(pow(vLen3(abXac),2) * 2));
	vSet3(toCircumsphereCenter, t1);

	// The 3 space coords of the circumsphere center then:
	vAdd3(center, a, toCircumsphereCenter);

	float circumsphereRadius = vLen3(toCircumsphereCenter);
	return circumsphereRadius;
}

//
//
//

union Vec2 {
	struct {
		float x, y;
	};

	struct {
		float w, h;
	};

	struct {
		float min, max;
	};

	float e[2];
};

union Vec2i {
	struct {
		int x, y;
	};

	struct {
		int w, h;
	};

	int e[2];
};

union Vec3 {
	struct {
		float x, y, z;
	};

	struct {
		float r, g, b;
	};

	struct {
		Vec2 xy;
		// float z;
	};

	struct {
		float nothing;
		// float x;
		Vec2 yz;
	};

	float e[3];
};

union Vec3i {
	struct {
		int x, y, z;
	};

	struct {
		Vec2i xy;
		// int z;
	};

	struct {
		int nothing;
		// int x;
		Vec2i yz;
	};

	int e[3];
};

union Vec4 {
	struct {
		float x, y, z, w;
	};

	struct {
		Vec3 xyz;
		float w;
	};

	struct {
		float r, g, b, a;
	};

	struct {
		Vec3 rgb;
		float a;
	};

	float e[4];
};

union Mat4 {
	struct {
		float xa, xb, xc, xd;
		float ya, yb, yc, yd;
		float za, zb, zc, zd;
		float wa, wb, wc, wd;
	};

	struct {
		float x1, y1, z1, w1;
		float x2, y2, z2, w2;
		float x3, y3, z3, w3;
		float x4, y4, z4, w4;
	};

	float e[16];

	float e2[4][4];
};

union Rect {
	struct {
		Vec2 min;
		Vec2 max;
	};
	struct {
		Vec2 cen;
		Vec2 dim;
	};
	struct {
		float left;
		float bottom;
		float right;
		float top;
	};
	struct {
		float e[4];
	};
};

union Recti {
	struct {
		Vec2i min;
		Vec2i max;
	};
	struct {
		Vec2i cen;
		Vec2i dim;
	};
	struct {
		int left;
		int bottom;
		int right;
		int top;
	};
	struct {
		int e[4];
	};
};

union Rect3 {
	struct {
		Vec3 min;
		Vec3 max;
	};
	struct {
		Vec3 cen;
		Vec3 dim;
	};
	struct {
		float e[6];
	};
};

union Rect3i {
	struct {
		Vec3i min;
		Vec3i max;
	};
	struct {
		Vec3i cen;
		Vec3i dim;
	};
	struct {
		float e[6];
	};
};

//
//
//

inline Vec2i vec2i(int a, int b) {
	Vec2i vec;
	vec.x = a;
	vec.y = b;
	return vec;
}

inline Vec2i vec2i(int a) {
	Vec2i vec;
	vec.x = a;
	vec.y = a;
	return vec;
}

inline Vec2i vec2i(Vec2 a) {
	Vec2i vec;
	vec.x = a.x;
	vec.y = a.y;
	return vec;
}

inline bool operator==(Vec2i a, Vec2i b) {
	bool equal = (a.x == b.x) && (a.y == b.y);
	return equal;
}

inline bool operator!=(Vec2i a, Vec2i b) {
	return !(a==b);
}

inline Vec2i operator+(Vec2i a, int b) {
	a.x += b;
	a.y += b;
	return a;
}

inline Vec2i operator+(Vec2i a, Vec2i b) {
	a.x += b.x;
	a.y += b.y;
	return a;
}

inline Vec2i & operator+=(Vec2i & a, Vec2i b) {
	a = a + b;
	return a;
}

inline Vec2i & operator+=(Vec2i & a, int b) {
	a = a + b;
	return a;
}

inline Vec2i operator-(Vec2i a, int b) {
	a.x -= b;
	a.y -= b;
	return a;
}

inline Vec2i operator-(Vec2i a, Vec2i b) {
	a.x -= b.x;
	a.y -= b.y;
	return a;
}

inline Vec2i & operator-=(Vec2i & a, Vec2i b) {
	a = a - b;
	return a;
}

inline Vec2i & operator-=(Vec2i & a, int b) {
	a = a - b;
	return a;
}

inline Vec2i operator*(Vec2i a, int b) {
	a.x *= b;
	a.y *= b;
	return a;
}

inline Vec2i operator/(Vec2i a, Vec2i b) {
	a.x /= b.x;
	a.y /= b.y;
	return a;
}

// //
// //
// //

inline Vec2 vec2(float a, float b) {
	Vec2 vec;
	vec.x = a;
	vec.y = b;
	return vec;
}

inline Vec2 vec2(Vec2i a) {
	Vec2 vec;
	vec.x = a.x;
	vec.y = a.y;
	return vec;
}

// inline Vec2 vec2(float* a) /*float a[2]*/
// {
// 	Vec2 vec;
// 	vec.x = a[0];
// 	vec.y = a[1];
// 	return vec;
// }

inline Vec2 vec2(float a) {
	Vec2 vec;
	vec.x = a;
	vec.y = a;
	return vec;
}

inline Vec2 operator*(Vec2 a, float b) {
	a.x *= b;
	a.y *= b;
	return a;
}

inline Vec2 operator*(float b, Vec2 a) {
	a.x *= b;
	a.y *= b;
	return a;
}

inline Vec2 operator*(Vec2 a, Vec2 b) {
	a.x *= b.x;
	a.y *= b.y;
	return a;
}

inline Vec2 & operator*=(Vec2 & a, float b) {
	a = a * b;
	return a;
}

inline Vec2 & operator*=(Vec2 & a, Vec2 b) {
	a = a * b;
	return a;
}

inline Vec2 operator+(Vec2 a, float b) {
	a.x += b;
	a.y += b;
	return a;
}

inline Vec2 operator+(Vec2 a, Vec2 b) {
	a.x += b.x;
	a.y += b.y;
	return a;
}

inline Vec2 & operator+=(Vec2 & a, Vec2 b) {
	a = a + b;
	return a;
}

inline Vec2 & operator+=(Vec2 & a, float b) {
	a = a + b;
	return a;
}

inline Vec2 operator-(Vec2 a, float b) {
	a.x -= b;
	a.y -= b;
	return a;
}

inline Vec2 operator-(Vec2 a, Vec2 b) {
	a.x -= b.x;
	a.y -= b.y;
	return a;
}

inline Vec2 operator-(Vec2 a) {
	a.x = -a.x;
	a.y = -a.y;
	return a;
}

inline Vec2 & operator-=(Vec2 & a, Vec2 b) {
	a = a - b;
	return a;
}

inline Vec2 & operator-=(Vec2 & a, float b) {
	a = a - b;
	return a;
}

inline Vec2 operator/(Vec2 a, float b) {
	a.x /= b;
	a.y /= b;
	return a;
}

inline bool operator==(Vec2 a, Vec2 b) {
	bool equal = (a.x == b.x) && (a.y == b.y);
	return equal;
}

inline bool operator!=(Vec2 a, Vec2 b) {
	return !(a==b);
}

inline float dot(Vec2 a, Vec2 b) {
	return a.x*b.x + a.y*b.y;
}


inline float lenVec2(Vec2 a) {
	float length = sqrt(a.x*a.x + a.y*a.y);
	return length;
}

inline float lenSquaredVec2(Vec2 a) {
	float lengthSquared = a.x*a.x + a.y*a.y;
	return lengthSquared;
}

inline Vec2 normVec2(Vec2 a) {
	Vec2 norm;
	float len = sqrt(a.x*a.x + a.y*a.y);
	if(len > 0) norm = a/len;
	else norm = {0,0};

	return norm;
}

inline Vec2 normVec2Unsafe(Vec2 a) {
	Vec2 result = a/sqrt(a.x*a.x + a.y*a.y);

	return result;
}

inline float angleVec2(Vec2 dir1, Vec2 dir2) {
	float d = dot(normVec2(dir1), normVec2(dir2));
	d = clamp(d, -1, 1);
	float angle = acos(d);
	return angle;
}

inline float angleDirVec2(Vec2 dir) {
	float angle = atan2(dir.y, dir.x);
	return angle;
}

inline float lenLine(Vec2 p0, Vec2 p1) {
	float result = lenVec2(p1 - p0);
	return result;
}

inline Vec2 lineMidPoint(Vec2 p1, Vec2 p2) {
	float x = (p1.x + p2.x)/2;
	float y = (p1.y + p2.y)/2;
	Vec2 midPoint = vec2(x,y);

	return midPoint;
}

inline Vec2 lineNormal(Vec2 p1, Vec2 p2) {
	float dx = p2.x - p1.x;
	float dy = p2.y - p1.y;
	Vec2 normal = vec2(-dy,dx); // or (dy,-dx)
	normal = normVec2(normal);

	return normal;
}

inline Vec2 lineNormal(Vec2 dir) {
	Vec2 normal = vec2(-dir.y,dir.x);
	normal = normVec2(normal);

	return normal;
}

inline int lineSide(Vec2 p1, Vec2 p2, Vec2 point) {
	int side = sign( (p2.x-p1.x)*(point.y-p1.y) - (p2.y-p1.y)*(point.x-p1.x) );
	return side;
}

inline float detVec2(Vec2 a, Vec2 b) {
	a = normVec2(a);
	b = normVec2(b);
	float det = a.x * b.y - b.x * a.y;
	return det;	
}

inline Vec2 midOfTwoVectors(Vec2 p0, Vec2 p1, Vec2 p2) {
	Vec2 dir1 = normVec2(p0 - p1);
	Vec2 dir2 = normVec2(p2 - p1);
	Vec2 mid = lineMidPoint(dir1, dir2);
	mid = normVec2(mid);
	return mid;
}

inline bool getLineIntersection(Vec2 p0, Vec2 p1, Vec2 p2, Vec2 p3, Vec2 * i = 0) {
    Vec2 s1 = p1 - p0;
    Vec2 s2 = p3 - p2;

    float s, t;

    s = (-s1.y * (p0.x - p2.x) + s1.x * (p0.y - p2.y)) / (-s2.x * s1.y + s1.x * s2.y);
    t = ( s2.x * (p0.y - p2.y) - s2.y * (p0.x - p2.x)) / (-s2.x * s1.y + s1.x * s2.y);

    if (s >= 0 && s <= 1 && t >= 0 && t <= 1)
    {
    	if(i) *i = p0 + t*s1;
        return true;
    }

    return false;
}

// clockwise
Vec2 rotateVec2(Vec2 a, float radian) {
	float cs = cos(-radian);
	float sn = sin(-radian);

	float nx = a.x * cs - a.y * sn;
	float ny = a.x * sn + a.y * cs;

	return vec2(nx, ny);
}

float distancePointLine(Vec2 v1, Vec2 v2, Vec2 point, bool infinite = false) {
	Vec2 diff = v2 - v1;
    if (diff == vec2(0,0))
    {
    	diff = point - v1;
        return lenVec2(diff);
    }

    float t = ((point.x - v1.x) * diff.x + (point.y - v1.y) * diff.y) / 
    		   (diff.x * diff.x + diff.y * diff.y);

   	if(infinite) {
		diff = point - (v1 + t*diff);
   	} else {
	    if 		(t < 0) diff = point - v1;
	    else if (t > 1) diff = point - v2;
	    else 			diff = point - (v1 + t*diff);
   	}

    return lenVec2(diff);
}

Vec2 lineClosestPoint(Vec2 v1, Vec2 v2, Vec2 point) {
	Vec2 result = {};
	Vec2 diff = v2 - v1;
    if (diff == vec2(0,0)) {
    	result = v1;
    } else {
	    float t = ((point.x - v1.x) * diff.x + (point.y - v1.y) * diff.y) / 
	    		   (diff.x * diff.x + diff.y * diff.y);

	    if 		(t < 0) result = v1;
	    else if (t > 1) result = v2;
	    else 			result = v1 + t*diff;
    }

	return result;
}

void closestPointToTriangle(float closest[2], float a[2], float b[2], float c[2], float p[2]) {
	// get 2 closest points
	float p1[2], p2[2];
	float lena = vLen2(p, a);
	float lenb = vLen2(p, b);
	float lenc = vLen2(p, c);
	float maxLen = max(lena, lenb, lenc);
	float nearest[2];
	if (maxLen == lena) {
		vSet2(p1, b);
		vSet2(p2, c);
		vSet2(nearest, a);
	}
	if (maxLen == lenb) {
		vSet2(p1, a);
		vSet2(p2, c);
		vSet2(nearest, b);
	}
	if (maxLen == lenc) {
		vSet2(p1, a);
		vSet2(p2, b);
		vSet2(nearest, c);
	}

	// check if point inside normal rectangle
	float p1LinePoint[2] = { p1[0] + 1, -p1[1] + 1 };
	float p2LinePoint[2] = { p2[0] + 1, -p2[1] + 1 };
	bool resultp1 = pointIsLeftOfLine(p1, p1LinePoint, p);
	bool resultp2 = pointIsLeftOfLine(p2, p2LinePoint, p);
	if (resultp1 != resultp2)
	{
		vSet2(closest, p);
	}
	else
	{
		// closest point is nearest triangle point
		vSet2(closest, nearest);
	}
}

Vec2 projectPointOnLine(Vec2 p, Vec2 lp0, Vec2 lp1, bool clampDist = false) {

	Vec2 ld = normVec2(lp1 - lp0);
	float dist = (dot(p-lp0, ld) / dot(ld, ld));

	if(clampDist) dist = clamp(dist, 0, lenVec2(lp1 - lp0));

	Vec2 result = lp0 + ld * dist;

	return result;
}

float sign(Vec2 p1, Vec2 p2, Vec2 p3) {
	return (p1.x - p3.x) * (p2.y - p3.y) - (p2.x - p3.x) * (p1.y - p3.y);
}

bool pointInTriangle(Vec2 pt, Vec2 v1, Vec2 v2, Vec2 v3) {
	bool b1, b2, b3;

	b1 = sign(pt, v1, v2) < 0.0f;
	b2 = sign(pt, v2, v3) < 0.0f;
	b3 = sign(pt, v3, v1) < 0.0f;

	return ((b1 == b2) && (b2 == b3));
}

Vec2 closestPointToTriangle(Vec2 p, Vec2 a, Vec2 b, Vec2 c) {
	bool insideTriangle = pointInTriangle(p.e, a.e, b.e, c.e);
	if(insideTriangle) return p;

	Vec2 p0 = projectPointOnLine(p, a, b, true);
	Vec2 p1 = projectPointOnLine(p, b, c, true);
	Vec2 p2 = projectPointOnLine(p, c, a, true);

	float d0 = lenVec2(p0 - p);
	float d1 = lenVec2(p1 - p);
	float d2 = lenVec2(p2 - p);

	float shortestDist = min(d0, d1, d2);

	Vec2 closestPoint = vec2(0,0);
	     if(shortestDist == d0) closestPoint = p0;
	else if(shortestDist == d1) closestPoint = p1;
	else if(shortestDist == d2) closestPoint = p2;

	return closestPoint;
}

inline bool lineCircleIntersection(Vec2 lp0, Vec2 lp1, Vec2 cp, float r, Vec2 * i) {
	Vec2 d = lp1 - lp0;
	Vec2 f = lp0 - cp;

	float a = dot(d,d);
	float b = 2*dot(f,d);
	float c = dot(f,f) - r*r;

	float discriminant = b*b-4*a*c;
	if( discriminant < 0 )
	{
		return false;
	}
	else
	{
		discriminant = sqrt( discriminant );

		float t1 = (-b - discriminant)/(2*a);
		float t2 = (-b + discriminant)/(2*a);

		if( t1 >= 0 && t1 <= 1 )
		{
			*i = lp0 + d*t1;
			return true;
		}

		if( t2 >= 0 && t2 <= 1 )
		{
			*i = lp0 + d*t2;
			return true;
		}

		return false;
	}

	return false;
}

inline Vec2 calculatePosition(Vec2 oldPosition, Vec2 velocity, Vec2 acceleration, float time) {
	oldPosition += 0.5f*acceleration*time*time + velocity*time;
	return oldPosition;
}

inline Vec2 calculateVelocity(Vec2 oldVelocity, Vec2 acceleration, float time) {
	oldVelocity += acceleration*time;
	return oldVelocity;
}

inline void calculateVelocityMul(float* velocity, float acceleration, float minVelocity, float time) {
	if(abs(*velocity) >= minVelocity) *velocity *= acceleration;
	else *velocity = 0;
}

inline void calculateVelocityAdd(float* velocity, float acceleration, float minVelocity, float time) {
	if(abs(*velocity) > minVelocity) {
		if(*velocity > 0) {
			*velocity -= acceleration;
			clampMin(&*velocity, 0);
		}  else if(*velocity < 0) {
			*velocity += acceleration;
			clampMax(&*velocity, 0);
		}
	} else *velocity = 0;
}


inline Vec2 clampMin(Vec2 v, Vec2 dim) {
	Vec2 result;
	result.x = clampMin(v.x, dim.x);
	result.y = clampMin(v.y, dim.y);

	return result;
}	

inline Vec2 clampMax(Vec2 v, Vec2 dim) {
	Vec2 result = v;
	result.x = clampMax(v.x, dim.x);
	result.y = clampMax(v.y, dim.y);
	
	return result;
}	

Vec2 clamp(Vec2 v, Rect region) {
	Vec2 result = v;
	result.x = clamp(v.x, region.min.x, region.max.x);
	result.y = clamp(v.y, region.min.y, region.max.y);

	return result;
}

void clamp(Vec2* v, Rect region) {
	v->x = clamp(v->x, region.min.x, region.max.x);
	v->y = clamp(v->y, region.min.y, region.max.y);
}

inline Vec2i clampMin(Vec2i v, Vec2i dim) {
	Vec2i result;
	result.x = clampMinInt(v.x, dim.x);
	result.y = clampMinInt(v.y, dim.y);

	return result;
}	

inline Vec2i clampMax(Vec2i v, Vec2i dim) {
	Vec2i result = v;
	result.x = clampMaxInt(v.x, dim.x);
	result.y = clampMaxInt(v.y, dim.y);
	
	return result;
}	

Vec2i clamp(Vec2i v, Rect region) {
	Vec2i result = v;
	result.x = clampInt(v.x, region.min.x, region.max.x);
	result.y = clampInt(v.y, region.min.y, region.max.y);

	return result;
}

void clamp(Vec2i* v, Rect region) {
	v->x = clampInt(v->x, region.min.x, region.max.x);
	v->y = clampInt(v->y, region.min.y, region.max.y);
}

float mapRange(float value, Vec2 dim, Vec2 range) {
	return mapRange(value, dim.x, dim.y, range.x, range.y);
};

float mapRangeClamp(float value, Vec2 dim, Vec2 range) {
	return mapRangeClamp(value, dim.x, dim.y, range.x, range.y);
};

inline Vec2 toVec2(Vec3 a) {
	Vec2 result;
	result.x = a.x;
	result.y = a.y;

	return result;
}

inline Vec3 toVec3(Vec2 a);
inline Vec3 cross(Vec3 a, Vec3 b);

inline Vec2 perpToPoint(Vec2 dir, Vec2 dirPoint) {
	Vec3 ab = {dir.x, dir.y, 0};
	Vec3 ap = {dirPoint.x, dirPoint.y, 0};

	Vec3 abxap = {	ab.y*ap.z - ab.z*ap.y,
					ab.z*ap.x - ab.x*ap.z,
					ab.x*ap.y - ab.y*ap.x };

	Vec3 xab = {	abxap.y*ab.z - abxap.z*ab.y,
					abxap.z*ab.x - abxap.x*ab.z,
					abxap.x*ab.y - abxap.y*ab.x };

	Vec2 result = xab.xy;

	return result;
}

inline Vec2 perpToPoint(Vec2 a, Vec2 b, Vec2 p) {
	Vec2 ab = {	b.x - a.x,
				b.y - a.y, };

	Vec2 ap = {	p.x - a.x,
				p.y - a.y, };

	Vec2 result = perpToPoint(ab, ap);

	return result;
}

inline float cross(Vec2 a, Vec2 b) {
	float result = a.x*b.y - a.y*b.x;
	return result;
}

inline Vec2 dirTurnRight(Vec2 dir) {
	Vec2 result = vec2(dir.y, -dir.x);
	return result;
}

inline Vec2 dirTurnLeft(Vec2 dir) {
	Vec2 result = vec2(-dir.y, dir.x);
	return result;
}

inline bool equalVec2(Vec2 a, Vec2 b, float margin) {
	bool equalX = roughlyEqual(a.x, b.x, margin);
	bool equalY = roughlyEqual(a.y, b.y, margin);
	bool equal = equalX && equalY;
	
	return equal;
}

inline Vec2 mapVec2(Vec2 a, Vec2 oldMin, Vec2 oldMax, Vec2 newMin, Vec2 newMax) {
	Vec2 result = a;
	result.x = mapRange(result.x, oldMin.x, oldMax.x, newMin.x, newMax.x);
	result.y = mapRange(result.y, oldMin.y, oldMax.y, newMin.y, newMax.y);

	return result;
}

inline Vec2 angleToDir(float angleInRadians) {
	Vec2 result = vec2(cos(angleInRadians), sin(angleInRadians));
	return result;
}

inline bool vecBetweenVecs(Vec2 v, Vec2 left, Vec2 right) {
	bool result;
	float ca = cross(left,v);
	float cb = cross(right,v);

	result = ca < 0 && cb > 0;
	return result;
}

inline Vec2 lerpVec2(Vec2 min, Vec2 max, float percent) {
	Vec2 result;
	result.x = lerp(percent, min.x, max.x);
	result.y = lerp(percent, min.y, max.y);
	return result;
}

inline Vec2 quadraticBezierInterpolation(Vec2 p0, Vec2 p1, Vec2 p2, float t) {
	Vec2 pa = lerpVec2(p0, p1, t);
	Vec2 pb = lerpVec2(p1, p2, t);

	Vec2 v = lerpVec2(pa, pb, t);
	return v;
}

inline Vec2 cubicBezierInterpolation(Vec2 p0, Vec2 p1, Vec2 p2, Vec2 p3, float t) {
	Vec2 pa = quadraticBezierInterpolation(p0, p1, p2, t);
	Vec2 pb = quadraticBezierInterpolation(p1, p2, p3, t);

	Vec2 v = lerpVec2(pa, pb, t);
	return v;
}

inline Vec2 cubicBezierInterpolationSeemless(Vec2 p0, Vec2 p1, Vec2 p2, Vec2 p3, float t) {
	Vec2 v;
	v.x = cubicBezierInterpolationSeemless(p0.x, p1.x, p2.x, p3.x, t);
	v.y = cubicBezierInterpolationSeemless(p0.y, p1.y, p2.y, p3.y, t);
	return v;
}

inline float cubicBezierGuessLength(Vec2 p0, Vec2 p1, Vec2 p2, Vec2 p3) {
	float length = lenLine(p0,p1) + lenLine(p1,p2) + lenLine(p2,p3) + lenLine(p3,p0);
	length = length / 2;
	return length;
}

void cubicBezierTesselate(Vec2* points, int* pointsCount, Vec2 p0, Vec2 p1, Vec2 p2, Vec2 p3, float tolerance, int step = 0) {

	if(step == 0) *pointsCount = 0;

	if(step > 10) return;

	float d = distancePointLine(p0, p3, p1) + distancePointLine(p0, p3, p2);
	bool lineFlat = d < tolerance * lenLine(p0, p3);
	if(!lineFlat) {
		Vec2 p01 = lerpVec2(p0, p1, 0.5f);
		Vec2 p12 = lerpVec2(p1, p2, 0.5f);
		Vec2 p23 = lerpVec2(p2, p3, 0.5f);
		Vec2 p012 = lerpVec2(p01, p12, 0.5f);
		Vec2 p123 = lerpVec2(p12, p23, 0.5f);
		Vec2 p0123 = lerpVec2(p012, p123, 0.5f);

		if(step == 0) {
			if(points) points[(*pointsCount)++] = p0;
			else (*pointsCount)++;
		}

		cubicBezierTesselate(points, pointsCount, p0, p01, p012, p0123, tolerance, step+1);

		if(points) points[(*pointsCount)++] = p0123;
		else (*pointsCount)++;

		cubicBezierTesselate(points, pointsCount, p0123, p123, p23, p3, tolerance, step+1);

		if(step == 0) {
			if(points) points[(*pointsCount)++] = p3;
			else (*pointsCount)++;
		}

	} else {
		if(step == 0) {
			if(points) points[(*pointsCount)++] = p0;
			else (*pointsCount)++;
		}

		if(step == 0) {
			if(points) points[(*pointsCount)++] = p3;
			else (*pointsCount)++;
		}
	}
}

Vec2 operator+(Vec2 a, Vec2i b) {
	a.x += b.x;
	a.y += b.y;
	return a;	
}

Vec2 roundVec2(Vec2 a) {
	return vec2(roundFloat(a.x), roundFloat(a.y));
}

//
//
//

inline Vec3 vec3(float a);
Vec3 VEC3_ZERO = vec3(0.0f);
Vec3 VEC3_ONE = vec3(1.0f);

inline Vec3 vec3(float a, float b, float c) {
	Vec3 vec;
	vec.x = a;
	vec.y = b;
	vec.z = c;
	return vec;
}

inline Vec3 vec3(Vec3i v) {
	Vec3 vec;
	vec.x = v.x;
	vec.y = v.y;
	vec.z = v.z;
	return vec;
}

inline Vec3 vec3(float a[3]) {
	Vec3 vec;
	vec.x = a[0];
	vec.y = a[1];
	vec.z = a[2];
	return vec;
}

inline Vec3 vec3(float a) {
	Vec3 vec;
	vec.x = a;
	vec.y = a;
	vec.z = a;
	return vec;
}

inline Vec3 vec3(Vec2 a, float b) {
	Vec3 vec;
	vec.xy = a;
	vec.z = b;
	return vec;
}

inline Vec3 vec3(float a, Vec2 b) {
	Vec3 vec;
	vec.x = a;
	vec.yz = b;
	return vec;
}

inline Vec3 operator*(Vec3 a, float b) {
	a.x *= b;
	a.y *= b;
	a.z *= b;
	return a;
}

inline Vec3 operator*(float b, Vec3 a) {
	a.x *= b;
	a.y *= b;
	a.z *= b;
	return a;
}

inline Vec3 operator*(Vec3 a, Vec3 b) {
	a.x *= b.x;
	a.y *= b.y;
	a.z *= b.z;
	return a;
}

inline Vec3 & operator*=(Vec3 & a, float b) {
	a = a * b;
	return a;
}

inline Vec3 operator+(Vec3 a, float b) {
	a.x += b;
	a.y += b;
	a.z += b;
	return a;
}

inline Vec3 operator+(Vec3 a, Vec3 b) {
	a.x += b.x;
	a.y += b.y;
	a.z += b.z;
	return a;
}

inline Vec3 & operator+=(Vec3 & a, Vec3 b) {
	a = a + b;
	return a;
}

inline Vec3 operator-(Vec3 a, float b) {
	a.x -= b;
	a.y -= b;
	a.z -= b;
	return a;
}

inline Vec3 operator-(Vec3 a, Vec3 b) {
	a.x -= b.x;
	a.y -= b.y;
	a.z -= b.z;
	return a;
}

inline Vec3 & operator-=(Vec3 & a, Vec3 b) {
	a = a - b;
	return a;
}

inline Vec3 operator/(Vec3 a, float b) {
	a.x /= b;
	a.y /= b;
	a.z /= b;
	return a;
}

inline Vec3 operator/(float b, Vec3 a) {
	a.x = b / a.x;
	a.y = b / a.y;
	a.z = b / a.z;
	return a;
}

inline Vec3 operator/(Vec3 a, Vec3 b) {
	a.x /= b.x;
	a.y /= b.y;
	a.z /= b.z;
	return a;
}

inline Vec3 operator-(Vec3 a) {
	a.x = -a.x;
	a.y = -a.y;
	a.z = -a.z;
	return a;
}

inline bool operator==(Vec3 a, Vec3 b) {
	bool equal = (a.x == b.x) && (a.y == b.y) && (a.z == b.z);
	return equal;
}

inline bool operator!=(Vec3 a, Vec3 b) {
	return !(a==b);
}

inline float dot(Vec3 a, Vec3 b) {
	return a.x*b.x + a.y*b.y + a.z*b.z;
}

inline float dot(Vec3 a) {
	return a.x*a.x + a.y*a.y + a.z*a.z;
}

// A and B assumed to be unit vectors.
inline float dotUnitToPercent(float dotProduct) {
	float result = 1 - acos(dotProduct)/M_PI_2;
	return result;
}

inline Vec3 cross(Vec3 a, Vec3 b) {
	Vec3 result;
	result.x = a.y*b.z - a.z*b.y;
	result.y = a.z*b.x - a.x*b.z;
	result.z = a.x*b.y - a.y*b.x;

	return result;
}

inline Vec3 toVec3(Vec2 a) {
	Vec3 result;
	result.x = a.x;
	result.y = a.y;
	result.z = 0;

	return result;
}

inline float lenVec3(Vec3 a) {
	return sqrt(dot(a));
}

inline Vec3 normVec3(Vec3 a) {
	return a/lenVec3(a);
}

inline Vec3 projectPointOnLine(Vec3 lPos, Vec3 lDir, Vec3 p) {
	Vec3 result = lPos + ((dot(p-lPos, lDir) / dot(lDir))) * lDir;
	return result;
}

inline float angleBetweenVectors(Vec3 a, Vec3 b) {
	float angle = acos(dot(normVec3(a), normVec3(b)));
	return angle;
}

// inline float angleBetweenVectors(Vec3 a, Vec3 b) {
// 	float angle = acos(dot(normVec3(a), normVec3(b)));
// 	return angle;
// }

Vec3 boxRaycastNormals[6] = {vec3(-1,0,0), vec3(1,0,0), vec3(0,-1,0), vec3(0,1,0), vec3(0,0,-1), vec3(0,0,1)};

float boxRaycast(Vec3 lp, Vec3 ld, Vec3 boxPos, Vec3 boxDim, Vec3* intersection = 0, Vec3* intersectionNormal = 0, bool secondIntersection = false) {

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

	float distance;
	// if tmax < 0, ray (line) is intersecting AABB, but whole AABB is behind us
	if (tmax < 0) return -1;

	// if tmin > tmax, ray doesn't intersect AABB
	if (tmin > tmax) return -1;

	distance = secondIntersection ? tmax : tmin;

	if(distance < 0) return -1;

	if(intersection) *intersection = lp + ld*distance;

	if(intersectionNormal) {
		     if(distance == t1) *intersectionNormal = boxRaycastNormals[0];
		else if(distance == t2) *intersectionNormal = boxRaycastNormals[1];
		else if(distance == t3) *intersectionNormal = boxRaycastNormals[2];
		else if(distance == t4) *intersectionNormal = boxRaycastNormals[3];
		else if(distance == t5) *intersectionNormal = boxRaycastNormals[4];
		else if(distance == t6) *intersectionNormal = boxRaycastNormals[5];
	}

	return distance;
}

int getBiggestAxis(Vec3 v, int smallerAxis[2] = 0) {
	int biggestAxis = maxReturnIndex(abs(v.x), abs(v.y), abs(v.z));
	if(smallerAxis != 0) {
		smallerAxis[0] = mod(biggestAxis-1, 3);
		smallerAxis[1] = mod(biggestAxis+1, 3);
	}

	return biggestAxis;
}

Vec3 rgbToHsl(Vec3 rgbColor) {
	Vec3 result = {};
	rgbToHsl(result.e, rgbColor.x, rgbColor.y, rgbColor.z);
	return result;
}

Vec3 hslToRgb(Vec3 hslColor) {
	Vec3 result = {};
	hslToRgb(result.e, hslColor.x, hslColor.y, hslColor.z);
	return result;
}

inline Vec3 hslToRgbFloat(Vec3 hsl) {
	float c[3];
	hsl.x = modFloat(hsl.x, 1.0f);
	hsl.y = clamp(hsl.y, 0, 1);
	hsl.z = clamp(hsl.z, 0, 1);
	hslToRgb(c, 360 * hsl.x, hsl.y, hsl.z);

	return vec3(c[0], c[1], c[2]);
}
inline Vec3 hslToRgbFloat(float h, float s, float l) {
	return hslToRgbFloat(vec3(h,s,l));
}

inline Vec3 rgbToHslFloat(Vec3 rgb) {
	Vec3 hsl = rgbToHsl(rgb);
	Vec3 hslFloat = vec3(hsl.x / (float)360, hsl.y, hsl.z);
	return hslFloat;
}

bool linesegmentSphereIntersection(Vec3 linePoint0, Vec3 linePoint1, Vec3 circleCenter, double circleRadius)
{

    double cx = circleCenter.x;
    double cy = circleCenter.y;
    double cz = circleCenter.z;

    double px = linePoint0.x;
    double py = linePoint0.y;
    double pz = linePoint0.z;

    double vx = linePoint1.x - px;
    double vy = linePoint1.y - py;
    double vz = linePoint1.z - pz;

    double A = vx * vx + vy * vy + vz * vz;
    double B = 2.0 * (px * vx + py * vy + pz * vz - vx * cx - vy * cy - vz * cz);
    double C = px * px - 2 * px * cx + cx * cx + py * py - 2 * py * cy + cy * cy +
               pz * pz - 2 * pz * cz + cz * cz - circleRadius * circleRadius;

    // discriminant
    double D = B * B - 4 * A * C;

    double t1 = (-B - sqrt(D)) / (2.0 * A);

    Vec3 solution1 = vec3(linePoint0.x * (1 - t1) + t1 * linePoint1.x,
                                     linePoint0.y * (1 - t1) + t1 * linePoint1.y,
                                     linePoint0.z * (1 - t1) + t1 * linePoint1.z);

    double t2 = (-B + sqrt(D)) / (2.0 * A);
    Vec3 solution2 = vec3(linePoint0.x * (1 - t2) + t2 * linePoint1.x,
                                     linePoint0.y * (1 - t2) + t2 * linePoint1.y,
                                     linePoint0.z * (1 - t2) + t2 * linePoint1.z);

    if (D < 0 || t1 > 1 || t1 < 0 || t2 > 1 || t2 < 0) return false;

    // D == 0 one solution;

    return true;
}

float linePlaneIntersection(Vec3 lp, Vec3 ld, Vec3 pp, Vec3 pn, Vec3 pu, Vec2 dim, Vec3* intersection = 0, Vec3* intersectionNormal = 0) {
	// Assuming pu is unit vector.

	float a = dot(pn, ld);
	if(a == 0) return -1;

	float distance = -(dot(pn, lp) - dot(pn, pp)) / a;
	if(distance >= 0) {
		Vec3 ip = lp + ld*distance;
		ip = ip - pp;
		float ix = dot(ip, pu);   // ip*cos(angle)

		float lenIp = lenVec3(ip);
		float angle = acos(ix/lenIp);
		float iy = lenIp*sin(angle); // sin(angle)

		if(abs(ix) > dim.y/2.0f || abs(iy > dim.x/2.0f)) return -1;

		if(intersection) {
			*intersection = lp + ld*distance;
			if(intersectionNormal) *intersectionNormal = pn;
		}

		return distance;
	}

	return -1;
}

float linePlaneIntersection(Vec3 lp, Vec3 ld, Vec3 pp, Vec3 pn, Vec3* intersection = 0, Vec3* intersectionNormal = 0) {

	// N * (Q - E) = 0, t = N*(Q-E) / N*D.

	float a = dot(pn, ld);
	if(a == 0) return -1;

	float distance = dot(pn, (pp-lp)) / a;
	if(distance >= 0) {
		if(intersection) *intersection = lp + ld*distance;
		if(intersectionNormal) 
			*intersectionNormal = a < 0 ? pn : -pn;
		return distance;
	}

	return -1;
}

bool lineSphereCollision(Vec3 lp, Vec3 ld, Vec3 sp, float sr) {
	lp -= sp;
	float dotLdLp = dot(ld,lp);
	float lenLp = lenVec3(lp);
	float b = dotLdLp*dotLdLp - lenLp*lenLp + sr*sr;

	if(b < 0) return false;

	float distance = -(dot(ld, lp)) + sqrt(b);
	if(distance < 0) return false;

	return true;
}

float lineSphereIntersection(Vec3 lp, Vec3 ld, Vec3 sp, float sr, Vec3* intersection = 0, Vec3* intersectionNormal = 0, bool secondIntersection = false) {
	// ld must be unit.

	Vec3 oldP = lp;
	lp -= sp;

	float dotLdLp = dot(ld,lp);
	float lenLp = lenVec3(lp);
	float b = dotLdLp*dotLdLp - lenLp*lenLp + sr*sr;

	if(b < 0) return -1;

	// Always choose shorter distance to get intersection that's closest.

	float distance = -(dot(ld, lp));
	if(secondIntersection) distance += sqrt(b);
	else distance += -sqrt(b);

	if(distance < 0) return -1;

	if(intersection) {
		*intersection = oldP + ld*distance;
		if(intersectionNormal) {
			(*intersectionNormal) = normVec3((*intersection) - sp);
		}
	}

    return distance;
}

inline Vec3 reflectVector(Vec3 dir, Vec3 normal) {
	Vec3 result = dir - 2*(dot(dir, normal))*normal;
	return result;
}

// Not tested.
Vec3 linesClosestPoint(Vec3 lp0, Vec3 ld0, Vec3 lp1, Vec3 ld1) {
	Vec3 p = vec3(10,0,0);

	Vec3 a = ld0 * 10000000;
	Vec3 b = ld1 * 10000000;
	Vec3 c = lp1 - lp0; // Normalized???

	float upper, lower;

	upper = -dot(a,b) * dot(b,c) + dot(a,c) * dot(b,b);
	lower =  dot(a,a) * dot(b,b) - dot(a,b) * dot(a,b);
	Vec3 d = lp0 + a*(upper / lower);

	upper = dot(a,b) * dot(a,c) - dot(b,c) * dot(a,a);
	Vec3 e = lp1 + b*(upper / lower);

	// p = (d + e) / 2;
	p = e;

	return p;
}

//
//
//

inline Vec3i vec3i(int a, int b, int c) {
	Vec3i vec;
	vec.x = a;
	vec.y = b;
	vec.z = c;
	return vec;
}

inline Vec3i vec3i(int a) {
	Vec3i vec;
	vec.x = a;
	vec.y = a;
	vec.z = a;
	return vec;
}

inline Vec3i vec3i(Vec3 a) {
	return vec3i(a.x,a.y,a.z);
}

inline Vec3i operator+(Vec3i a, float b) {
	a.x += b;
	a.y += b;
	a.z += b;
	return a;
}

inline Vec3i operator+(Vec3i a, Vec3i b) {
	a.x += b.x;
	a.y += b.y;
	a.z += b.z;
	return a;
}

inline Vec3i & operator+=(Vec3i & a, Vec3i b) {
	a = a + b;
	return a;
}

inline Vec3i operator-(Vec3i a, float b) {
	a.x -= b;
	a.y -= b;
	a.z -= b;
	return a;
}

inline Vec3i operator-(Vec3i a, Vec3i b) {
	a.x -= b.x;
	a.y -= b.y;
	a.z -= b.z;
	return a;
}

inline Vec3i operator*(Vec3i a, Vec3i b) {
	a.x *= b.x;
	a.y *= b.y;
	a.z *= b.z;
	return a;
}

inline Vec3i operator/(Vec3i a, int b) {
	a.x /= b;
	a.y /= b;
	a.z /= b;
	return a;
}

inline Vec3i & operator-=(Vec3i & a, Vec3i b) {
	a = a - b;
	return a;
}

inline bool operator==(Vec3i a, Vec3i b) {
	bool equal = (a.x == b.x) && (a.y == b.y) && (a.z == b.z);
	return equal;
}

inline Vec3 lerp(float percent, Vec3 a, Vec3 b) {
	a.x = lerp(percent, a.x, b.x);
	a.y = lerp(percent, a.y, b.y);
	a.z = lerp(percent, a.z, b.z);
	return a;
}

inline Vec3 lerpCheck(float percent, Vec3 a, Vec3 b) {
	a.x = lerpCheck(percent, a.x, b.x);
	a.y = lerpCheck(percent, a.y, b.y);
	a.z = lerpCheck(percent, a.z, b.z);
	return a;
}

inline Vec3 clamp(Vec3 n, Vec3 min, Vec3 max) {
	n.x = clamp(n.x, min.x, max.x);
	n.y = clamp(n.y, min.y, max.y);
	n.z = clamp(n.z, min.z, max.z);
	return n;
}

inline Vec3 clampMin(Vec3 n, Vec3 min) {
	n.x = clampMin(n.x, min.x);
	n.y = clampMin(n.y, min.y);
	n.z = clampMin(n.z, min.z);
	return n;
}

inline Vec3 clampMax(Vec3 n, Vec3 max) {
	n.x = clampMax(n.x, max.x);
	n.y = clampMax(n.y, max.y);
	n.z = clampMax(n.z, max.z);
	return n;
}

//
//
//

inline Vec4 vec4(float a, float b, float c, float d) {
	Vec4 vec;
	vec.x = a;
	vec.y = b;
	vec.z = c;
	vec.w = d;
	return vec;
}

inline Vec4 vec4(float a[4]) {
	Vec4 vec;
	vec.x = a[0];
	vec.y = a[1];
	vec.z = a[2];
	vec.w = a[3];
	return vec;
}

inline Vec4 vec4(float color, float alpha) {
	Vec4 vec;
	vec.r = color;
	vec.g = color;
	vec.b = color;
	vec.a = alpha;
	return vec;
}

inline Vec4 vec4(Vec3 a, float w) {
	Vec4 vec;
	vec.xyz = a;
	vec.w = w;
	return vec;
}

inline Vec4 operator*(Vec4 a, float b) {
	a.x *= b;
	a.y *= b;
	a.z *= b;
	a.w *= b;
	return a;
}

inline Vec4 operator*(float a, Vec4 b) {
	a *= b.x;
	a *= b.y;
	a *= b.z;
	a *= b.w;
	return b;
}

inline Vec4 operator+(Vec4 a, Vec4 b) {
	a.x += b.x;
	a.y += b.y;
	a.z += b.z;
	a.w += b.w;
	return a;
}

inline Vec4 operator+=(Vec4 & a, Vec4 b) {
	a = a + b;
	return a;
}

inline Vec4 operator-(Vec4 a, Vec4 b) {
	a.x -= b.x;
	a.y -= b.y;
	a.z -= b.z;
	a.w -= b.w;
	return a;
}

inline bool operator==(Vec4 a, Vec4 b) {
	bool result = (a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w);
	return result;
}

inline bool operator!=(Vec4 a, Vec4 b) {
	bool result = !(a == b);
	return result;
}

inline Vec4 hslToRgbFloatAlpha(Vec3 hsl) {
	return vec4(hslToRgbFloat(hsl), 1);
}

//
//
//

inline Mat4 operator*(Mat4 a, Mat4 b) {
	Mat4 r;
	int i = 0;
	for(int y = 0; y < 16; y += 4) {
		for(int x = 0; x < 4; x++) {
			r.e[i++] = a.e[y]*b.e[x] + a.e[y+1]*b.e[x+4] + a.e[y+2]*b.e[x+8] + a.e[y+3]*b.e[x+12];
		}
	}
	return r;
}

inline Vec4 operator*(Mat4 m, Vec4 v) {
	Vec4 result;
	result.x = m.xa*v.x + m.xb*v.y + m.xc*v.z + m.xd*v.w;
	result.y = m.ya*v.x + m.yb*v.y + m.yc*v.z + m.yd*v.w;
	result.z = m.za*v.x + m.zb*v.y + m.zc*v.z + m.zd*v.w;
	result.w = m.wa*v.x + m.wb*v.y + m.wc*v.z + m.wd*v.w;

	return result;
}

inline Vec3 operator*(Mat4 m, Vec3 v) {
	Vec4 result = m*vec4(v,0);
	return result.xyz;
}

inline void rowToColumn(Mat4* m) {
	for(int x = 1; x < 4; x++) {
		for(int y = 0; y < x; y++) {
			float temp = m->e2[y][x];
			m->e2[y][x] = m->e2[x][y];
			m->e2[x][y] = temp;
		}
	}
}

inline void scaleMatrix(Mat4* m, Vec3 a) {
	*m = {};
	m->x1 = a.x;
	m->y2 = a.y;
	m->z3 = a.z;
	m->w4 = 1;
}

inline Mat4 scaleMatrix(Vec3 a) {
	Mat4 m = {};
	m.x1 = a.x;
	m.y2 = a.y;
	m.z3 = a.z;
	m.w4 = 1;

	return m;
}

inline void rotationMatrix(Mat4* m, Vec3 a) {
	*m = {	cos(a.y)*cos(a.z), cos(a.z)*sin(a.x)*sin(a.y)-cos(a.x)*sin(a.z), cos(a.x)*cos(a.z)*sin(a.x)+sin(a.x)*sin(a.z), 0,
			cos(a.y)*sin(a.z), cos(a.x)*cos(a.z)+sin(a.x)*sin(a.y)*sin(a.z), -cos(a.z)*sin(a.x)+cos(a.x)*sin(a.y)*sin(a.z), 0,
			-sin(a.y), 		   cos(a.y)*sin(a.x), 							 cos(a.x)*cos(a.y), 0,
			0, 0, 0, 1};
}

inline void rotationMatrixX(Mat4* m, float a) {
	float ca = cos(a);
	float sa = sin(a);
	*m = {	1, 0, 0, 0,
			0, ca, sa, 0,
			0, -sa, ca, 0,
			0, 0, 0, 1};
}

inline void rotationMatrixY(Mat4* m, float a) {
	float ca = cos(a);
	float sa = sin(a);
	*m = {	ca, 0, -sa, 0,
			0, 1, 0, 0,
			sa, 0, ca, 0,
			0, 0, 0, 1};
}

inline void rotationMatrixZ(Mat4* m, float a) {
	float ca = cos(a);
	float sa = sin(a);
	*m = {	ca, sa, 0, 0,
			-sa, ca, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1};
}

inline void translationMatrix(Mat4* m, Vec3 a) {
	*m = {};
	m->x1 = 1;
	m->y2 = 1;
	m->z3 = 1;
	m->w4 = 1;

	m->w1 = a.x;
	m->w2 = a.y;
	m->w3 = a.z;
}

inline Mat4 translationMatrix(Vec3 a) {
	Mat4 m = {};
	m.x1 = 1;
	m.y2 = 1;
	m.z3 = 1;
	m.w4 = 1;

	m.w1 = a.x;
	m.w2 = a.y;
	m.w3 = a.z;

	return m;
}

inline void viewMatrix(Mat4* m, Vec3 cPos, Vec3 cLook, Vec3 cUp, Vec3 cRight) {
	// Bug: Why does look have to be negated?
	*m = {	cRight.x, cRight.y, cRight.z, -(dot(cPos,cRight)), 
			cUp.x, 	  cUp.y, 	cUp.z,    -(dot(cPos,cUp)), 
			-cLook.x,  -cLook.y,  -cLook.z,  -(dot(cPos,-cLook)), 
			0, 		  0, 		0, 		  1 };
}

inline Mat4 viewMatrix(Vec3 cPos, Vec3 cLook, Vec3 cUp, Vec3 cRight) {
	Mat4 m;
	viewMatrix(&m, cPos, cLook, cUp, cRight);

	return m;
}

inline void projMatrix(Mat4* m, float fov, float ar, float n, float f) {
	*m = { 	1/(ar*tan(fov*0.5f)), 0, 				 0, 			 0,
			0, 					  1/(tan(fov*0.5f)), 0, 			 0,
			0, 					  0, 				 -((f+n)/(f-n)), -((2*f*n)/(f-n)),
			0, 					  0, 				 -1, 			 0 };
}

inline Mat4 projMatrix(float fov, float ar, float n, float f) {
	Mat4 m;
	projMatrix(&m, fov, ar, n, f);
	
	return m;
}

//
//
//

union Quat {
	struct {
		float w, x, y, z;
	};

	struct {
		float nothing;
		// float w;
		Vec3 xyz;
	};

	float e[4];
};

Quat quat() {
	Quat r = {1,0,0,0};
	return r;
}

Quat quat(float w, float x, float y, float z) {
	Quat r = {w,x,y,z};
	return r;
}

Quat quat(float a, Vec3 axis) {
	Quat r;
	r.w = cos(a*0.5f);
	r.x = axis.x * sin(a*0.5f);
	r.y = axis.y * sin(a*0.5f);
	r.z = axis.z * sin(a*0.5f);
	return r;
}

// Not the right name for this.
float quatDot(Quat q) {
	return (q.w*q.w + q.x*q.x + q.y*q.y + q.z*q.z);
}

float quatMagnitude(Quat q) {
	return sqrt(quatDot(q));
}

Quat quatNorm(Quat q) {
	Quat result;
	float m = quatMagnitude(q);
	return quat(q.w/m, q.x/m, q.y/m, q.z/m);
}

Quat quatConjugate(Quat q) {
	return quat(q.w, -q.x, -q.y, -q.z);
}

Quat quatScale(Quat q, float s) {
	return quat(q.w*s, q.x*s, q.y*s, q.z*s);
}

Quat quatInverse(Quat q) {
	return quatScale(quatConjugate(q), (1/quatDot(q)));
}

// quat*axis -> local rotation.
// axis*quat -> world rotation.
Quat operator*(Quat a, Quat b) {
	Quat r;
	r.w = (a.w*b.w - a.x*b.x - a.y*b.y - a.z*b.z);
	r.x = (a.w*b.x + a.x*b.w + a.y*b.z - a.z*b.y);
	r.y = (a.w*b.y - a.x*b.z + a.y*b.w + a.z*b.x);
	r.z = (a.w*b.z + a.x*b.y - a.y*b.x + a.z*b.w);
	return r;
}

// Does not make sense. Only used internally once.
Quat operator-(Quat a, Quat b) {
	Quat r;
	r.w = a.w - b.w;
	r.x = a.x - b.x;
	r.y = a.y - b.y;
	r.z = a.z - b.z;

	return r;
}
Quat operator+(Quat a, Quat b) {
	Quat r;
	r.w = a.w + b.w;
	r.x = a.x + b.x;
	r.y = a.y + b.y;
	r.z = a.z + b.z;

	return r;
}

void quatRotationMatrix(Mat4* m, Quat q) {
	float w = q.w, x = q.x, y = q.y, z = q.z;
	float x2 = x*x, y2 = y*y, z2 = z*z;
	float w2 = w*w;
	*m = {	w2+x2-y2-z2, 2*x*y-2*w*z, 2*x*z+2*w*y, 0,
			2*x*y+2*w*z, w2-x2+y2-z2, 2*y*z-2*w*x, 0,
			2*x*z-2*w*y, 2*y*z+2*w*x, w2-x2-y2+z2, 0,
			0, 			 0, 		  0, 		   1};
}

Mat4 quatRotationMatrix(Quat q) {
	float w = q.w, x = q.x, y = q.y, z = q.z;
	float x2 = x*x, y2 = y*y, z2 = z*z;
	float w2 = w*w;
	Mat4 m = {	w2+x2-y2-z2, 2*x*y-2*w*z, 2*x*z+2*w*y, 0,
				2*x*y+2*w*z, w2-x2+y2-z2, 2*y*z-2*w*x, 0,
				2*x*z-2*w*y, 2*y*z+2*w*x, w2-x2-y2+z2, 0,
				0, 			 0, 		  0, 		   1};

	return m;
}

Vec3 operator*(Quat q, Vec3 v) {
	Vec3 t = 2 * cross(q.xyz, v);
	Vec3 result = v + q.w * t + cross(q.xyz, t);

	return result;
}

Vec3 rotateVec3(Vec3 v, float a, Vec3 axis) {
	Vec3 r = quat(a, axis)*v;
	return r;
}

void rotateVec3(Vec3* v, float a, Vec3 axis) {
	*v = rotateVec3(*v, a, axis);
}

Vec3 rotateVec3Around(Vec3 v, float a, Vec3 axis, Vec3 point) {
	Vec3 aroundOrigin = rotateVec3(v - point, a, axis);
	aroundOrigin += point;

	return aroundOrigin;
}

void rotateVec3Around(Vec3* v, float a, Vec3 axis, Vec3 point) {
	*v = rotateVec3Around(*v, a, axis, point);
}

Vec3 rotateVec3Around(Vec3 v, Quat q, Vec3 point) {
	Vec3 aroundOrigin = q * (v - point);
	aroundOrigin += point;

	return aroundOrigin;
}

void rotateVec3Around(Vec3* v, Quat q, Vec3 point) {
	*v = rotateVec3Around(*v, q, point);
}

// From Wikipedia.
Quat eulerAnglesToQuat(float pitch, float roll, float yaw) {
	Quat q;

    // Abbreviations for the various angular functions
	float cy = cos(yaw * 0.5);
	float sy = sin(yaw * 0.5);
	float cr = cos(roll * 0.5);
	float sr = sin(roll * 0.5);
	float cp = cos(pitch * 0.5);
	float sp = sin(pitch * 0.5);

	q.w = cy * cr * cp + sy * sr * sp;
	q.x = cy * sr * cp - sy * cr * sp;
	q.y = cy * cr * sp + sy * sr * cp;
	q.z = sy * cr * cp - cy * sr * sp;
	return q;
}
void quatToEulerAngles(Quat q, float* pitch, float* roll, float* yaw) {
	// roll (x-axis rotation)
	float sinr = +2.0 * (q.w * q.x + q.y * q.z);
	float cosr = +1.0 - 2.0 * (q.x * q.x + q.y * q.y);
	*roll = atan2(sinr, cosr);

	// pitch (y-axis rotation)
	float sinp = +2.0 * (q.w * q.y - q.z * q.x);
	if (fabs(sinp) >= 1)
		*pitch = copysign(M_PI / 2, sinp); // use 90 degrees if out of range
	else
		*pitch = asin(sinp);

	// yaw (z-axis rotation)
	float siny = +2.0 * (q.w * q.z + q.x * q.y);
	float cosy = +1.0 - 2.0 * (q.y * q.y + q.z * q.z);  
	*yaw = atan2(siny, cosy);
}


Mat4 modelMatrix(Vec3 trans, Vec3 scale, float degrees = 0, Vec3 rot = vec3(0,0,0)) {
	Mat4 sm; scaleMatrix(&sm, scale);
	Mat4 rm; quatRotationMatrix(&rm, quat(degrees, rot));
	Mat4 tm; translationMatrix(&tm, trans);
	Mat4 model = tm*rm*sm;

	return model;
}

bool operator==(Quat q0, Quat q1) {
	return q0.w==q1.w && q0.x==q1.x && q0.y==q1.y && q0.z==q1.z;
}

// 

float boxRaycastRotated(Vec3 lp, Vec3 ld, Vec3 pos, Vec3 dim, Quat rot, Vec3* intersection = 0, Vec3* intersectionNormal = 0, bool secondIntersection = false) {

	bool rotated = rot == quat() ? false : true;

	Vec3 rotatedPos, rotatedDir;
	if(rotated) {
		rotatedPos = rotateVec3Around(lp, quatInverse(rot), pos);
		rotatedDir = rotateVec3Around(lp + ld, quatInverse(rot), pos);
		rotatedDir = normVec3(rotatedDir - rotatedPos);
	} else {
		rotatedPos = lp;
		rotatedDir = ld;
	}

	float distance = boxRaycast(rotatedPos, rotatedDir, pos, dim, intersection, intersectionNormal, secondIntersection);
	if(distance != -1) {
		if(intersectionNormal) {
			if(rotated) *intersectionNormal = rot * (*intersectionNormal);
		}
	}

	return distance;
}

//
//
//

inline Rect  rect       () { return {0,0,0,0}; }
inline Rect  rect       (Vec2 min, Vec2 max) { return {min, max}; }
inline Rect  rect       (float left, float bottom, float right, float top) { return {left, bottom, right, top}; }
inline Rect  rectSides  (float left, float right, float bottom, float top) { return {left, bottom, right, top}; }

inline Rect  rectCenDim (Vec2 a, Vec2 d)    { return rect(a.x-d.w/2, a.y - d.h/2, a.x + d.w/2, a.y + d.h/2); }
inline Rect  rectBLDim  (Vec2 a, Vec2 d)    { return rect(a, a+d); };
inline Rect  rectTLDim  (Vec2 a, Vec2 d)    { return rect(a.x, a.y-d.h, a.x+d.w, a.y); };
inline Rect  rectTRDim  (Vec2 a, Vec2 d)    { return rect(a-d, a); };
inline Rect  rectBRDim  (Vec2 a, Vec2 d)    { return rect(a.x-d.w, a.y, a.x, a.y+d.h); };
inline Rect  rectLDim   (Vec2 a, Vec2 d)    { return rect(a.x, a.y-d.h/2, a.x+d.w, a.y+d.h/2); };
inline Rect  rectTDim   (Vec2 a, Vec2 d)    { return rect(a.x-d.w/2, a.y-d.h, a.x+d.w/2, a.y); };
inline Rect  rectRDim   (Vec2 a, Vec2 d)    { return rect(a.x-d.w, a.y-d.h/2, a.x, a.y+d.h/2); };
inline Rect  rectBDim   (Vec2 a, Vec2 d)    { return rect(a.x-d.w/2, a.y, a.x+d.w/2, a.y+d.h); };

inline Rect  rectCenDim (float x, float y, float w, float h) { return rectCenDim(vec2(x,y), vec2(w,h));};
inline Rect  rectBLDim  (float x, float y, float w, float h) { return rectBLDim(vec2(x,y), vec2(w,h)); };
inline Rect  rectTLDim  (float x, float y, float w, float h) { return rectTLDim(vec2(x,y), vec2(w,h)); };
inline Rect  rectTRDim  (float x, float y, float w, float h) { return rectTRDim(vec2(x,y), vec2(w,h)); };
inline Rect  rectBRDim  (float x, float y, float w, float h) { return rectBRDim(vec2(x,y), vec2(w,h)); };
inline Rect  rectLDim   (float x, float y, float w, float h) { return rectLDim(vec2(x,y), vec2(w,h)); };
inline Rect  rectTDim   (float x, float y, float w, float h) { return rectTDim(vec2(x,y), vec2(w,h)); };
inline Rect  rectRDim   (float x, float y, float w, float h) { return rectRDim(vec2(x,y), vec2(w,h)); };
inline Rect  rectBDim   (float x, float y, float w, float h) { return rectBDim(vec2(x,y), vec2(w,h)); };

inline float rectW      (Rect r)            { return r.right - r.left; };
inline float rectH      (Rect r)            { return r.top - r.bottom; };
inline float rectCenX   (Rect r)            { return r.left + rectW(r)/2; };
inline float rectCenY   (Rect r)            { return r.bottom + rectH(r)/2; };
inline Vec2  rectDim    (Rect r)            { return r.max - r.min; };
inline Vec2  rectCen    (Rect r)            { return r.min + rectDim(r)/2; };
inline Vec2  rectBL     (Rect r)            { return r.min; }
inline Vec2  rectL      (Rect r)            { return vec2(r.left, rectCen(r).y); }
inline Vec2  rectTL     (Rect r)            { return vec2(r.left, r.top); }
inline Vec2  rectT      (Rect r)            { return vec2(rectCen(r).x, r.top); }
inline Vec2  rectTR     (Rect r)            { return r.max; }
inline Vec2  rectR      (Rect r)            { return vec2(r.right, rectCen(r).y); }
inline Vec2  rectBR     (Rect r)            { return vec2(r.right, r.bottom); }
inline Vec2  rectB      (Rect r)            { return vec2(rectCen(r).x, r.bottom); }

inline Rect  rectSetCen (Rect r, Vec2 p)    { return rectCenDim(p, rectDim(r)); }
inline Rect  rectSetDim (Rect r, Vec2 d)    { return rectCenDim(rectCen(r), d); }
inline Rect  rectSetW   (Rect r, float p)   { return rectCenDim(rectCen(r), vec2(p, rectH(r))); }
inline Rect  rectSetH   (Rect r, float p)   { return rectCenDim(rectCen(r), vec2(rectW(r), p)); }
inline Rect  rectSetBL  (Rect r, Vec2 p)    { r.min = p; return r; }
inline Rect  rectSetTL  (Rect r, Vec2 p)    { r.left = p.x; r.top = p.y; return r; }
inline Rect  rectSetTR  (Rect r, Vec2 p)    { r.max = p; return r; }
inline Rect  rectSetBR  (Rect r, Vec2 p)    { r.right = p.x; r.bottom = p.y; return r; }
inline Rect  rectSetL   (Rect r, float p)   { r.left = p; return r; }
inline Rect  rectSetT   (Rect r, float p)   { r.top = p; return r; }
inline Rect  rectSetR   (Rect r, float p)   { r.right = p; return r; }
inline Rect  rectSetB   (Rect r, float p)   { r.bottom = p; return r; }

inline Rect  rectRSetL   (Rect r, float p)   { r.left = r.right - p; return r; }
inline Rect  rectRSetT   (Rect r, float p)   { r.top = r.bottom + p; return r; }
inline Rect  rectRSetR   (Rect r, float p)   { r.right = r.left + p; return r; }
inline Rect  rectRSetB   (Rect r, float p)   { r.bottom = r.top - p; return r; }

inline Rect  rectExpand (Rect r, Vec2 dim)  { return rect(r.min-dim/2, r.max+dim/2); }
inline Rect  rectExpand (Rect r, float s)   { return rect(r.min-vec2(s,s)/2, r.max+vec2(s,s)/2); }
inline Rect  rectTrans  (Rect r, Vec2 off)  { return rect(r.min+off, r.max+off); }
inline Rect  rectAddBL  (Rect r, Vec2 p)    { r.min += p; return r; }
inline Rect  rectAddTL  (Rect r, Vec2 p)    { r.left += p.x; r.top += p.y; return r; }
inline Rect  rectAddTR  (Rect r, Vec2 p)    { r.max += p; return r; }
inline Rect  rectAddBR  (Rect r, Vec2 p)    { r.right += p.x; r.bottom += p.y; return r; }
inline Rect  rectAddL   (Rect r, float p)   { r.left += p; return r; }
inline Rect  rectAddT   (Rect r, float p)   { r.top += p; return r; }
inline Rect  rectAddR   (Rect r, float p)   { r.right += p; return r; }
inline Rect  rectAddB   (Rect r, float p)   { r.bottom += p; return r; }

inline void  rectSetCen (Rect* r, Vec2 p)   { *r = rectCenDim(p, rectDim(*r)); }
inline void  rectSetDim (Rect* r, Vec2 d)   { *r = rectCenDim(rectCen(*r), d); }
inline void  rectSetW   (Rect* r, float p)  { *r = rectCenDim(rectCen(*r), vec2(p, rectH(*r))); }
inline void  rectSetH   (Rect* r, float p)  { *r = rectCenDim(rectCen(*r), vec2(rectW(*r), p)); }
inline void  rectSetBL  (Rect* r, Vec2 p)   { r->min = p; }
inline void  rectSetTL  (Rect* r, Vec2 p)   { r->left = p.x; r->top = p.y; }
inline void  rectSetTR  (Rect* r, Vec2 p)   { r->max = p; }
inline void  rectSetBR  (Rect* r, Vec2 p)   { r->right = p.x; r->bottom = p.y; }
inline void  rectSetL   (Rect* r, float p)  { r->left = p; }
inline void  rectSetT   (Rect* r, float p)  { r->top = p; }
inline void  rectSetR   (Rect* r, float p)  { r->right = p; }
inline void  rectSetB   (Rect* r, float p)  { r->bottom = p; }

inline void  rectRSetL  (Rect* r, float p)   { r->left = r->right - p; }
inline void  rectRSetT  (Rect* r, float p)   { r->top = r->bottom + p; }
inline void  rectRSetR  (Rect* r, float p)   { r->right = r->left + p; }
inline void  rectRSetB  (Rect* r, float p)   { r->bottom = r->top - p; }

inline void  rectExpand (Rect* r, Vec2 dim) { r->min -= dim/2; r->max += dim/2; }
inline void  rectExpand (Rect* r, float s)  { r->min -= vec2(s,s)/2; r->max += vec2(s,s)/2; }
inline void  rectTrans  (Rect* r, Vec2 off) { r->min += off; r->max += off; }
inline void  rectAddBL  (Rect* r, Vec2 p)   { r->min += p; }
inline void  rectAddTL  (Rect* r, Vec2 p)   { r->left += p.x; r->top += p.y; }
inline void  rectAddTR  (Rect* r, Vec2 p)   { r->max += p; }
inline void  rectAddBR  (Rect* r, Vec2 p)   { r->right += p.x; r->bottom += p.y; }
inline void  rectAddL   (Rect* r, float p)  { r->left += p; }
inline void  rectAddT   (Rect* r, float p)  { r->top += p; }
inline void  rectAddR   (Rect* r, float p)  { r->right += p; }
inline void  rectAddB   (Rect* r, float p)  { r->bottom += p; }

inline Rect rectCenDim(Rect r) {
	Rect newR;
	newR.dim = r.max - r.min;
	newR.cen = r.min + newR.dim/2;

	return newR;
}

inline Rect rectGetMinMax(Rect r) {
	Rect newR;
	Vec2 halfDim = r.dim/2;
	newR.min = r.cen - halfDim;
	newR.max = r.cen + halfDim;

	return newR;
}

Vec2 rectAlign(Rect r, Vec2i align) {
	     if(align == vec2i( 0, 0)) return rectCen(r);
	else if(align == vec2i(-1,-1)) return rectBL (r);
	else if(align == vec2i(-1, 0)) return rectL  (r);
	else if(align == vec2i(-1, 1)) return rectTL (r);
	else if(align == vec2i( 0, 1)) return rectT  (r);
	else if(align == vec2i( 1, 1)) return rectTR (r);
	else if(align == vec2i( 1, 0)) return rectR  (r);
	else if(align == vec2i( 1,-1)) return rectBR (r);
	else if(align == vec2i( 0,-1)) return rectB  (r);

	return vec2(0,0);
}

Rect rectAlignDim(Vec2 v, Vec2i align, Vec2 dim) {
	     if(align == vec2i(0,  0)) return rectCenDim( v, dim );
	else if(align == vec2i(-1,-1)) return rectBLDim ( v, dim );
	else if(align == vec2i(-1, 0)) return rectLDim  ( v, dim );
	else if(align == vec2i(-1, 1)) return rectTLDim ( v, dim );
	else if(align == vec2i( 0, 1)) return rectTDim  ( v, dim );
	else if(align == vec2i( 1, 1)) return rectTRDim ( v, dim );
	else if(align == vec2i( 1, 0)) return rectRDim  ( v, dim );
	else if(align == vec2i( 1,-1)) return rectBRDim ( v, dim );
	else if(align == vec2i( 0,-1)) return rectBDim  ( v, dim );

	return rect(0,0,0,0);
}

Rect rectAlignDim(Rect r, Vec2i align, Vec2 dim) {
	     if(align == vec2i( 0, 0)) return rectCenDim( rectCen(r), dim );
	else if(align == vec2i(-1,-1)) return rectBLDim ( rectBL (r), dim );
	else if(align == vec2i(-1, 0)) return rectLDim  ( rectL  (r), dim );
	else if(align == vec2i(-1, 1)) return rectTLDim ( rectTL (r), dim );
	else if(align == vec2i( 0, 1)) return rectTDim  ( rectT  (r), dim );
	else if(align == vec2i( 1, 1)) return rectTRDim ( rectTR (r), dim );
	else if(align == vec2i( 1, 0)) return rectRDim  ( rectR  (r), dim );
	else if(align == vec2i( 1,-1)) return rectBRDim ( rectBR (r), dim );
	else if(align == vec2i( 0,-1)) return rectBDim  ( rectB  (r), dim );

	return r;
}

inline bool operator==(Rect r1, Rect r2) { return (r1.min == r2.min) && (r1.max == r2.max); }
inline bool operator!=(Rect r1, Rect r2) { return (r1.min != r2.min) && (r1.max != r2.max); }

inline bool rectIntersection(Rect r1, Rect r2) {
	bool hasIntersection = !(r2.min.x > r1.max.x ||
							 r2.max.x < r1.min.x ||
							 r2.max.y < r1.min.y ||
							 r2.min.y > r1.max.y);
	return hasIntersection;
}

Rect rectIntersect(Rect r1, Rect r2) {
	bool hasIntersection = rectIntersection(r1, r2);
	Rect intersectionRect;
	if (hasIntersection) {
		intersectionRect.min.x = max(r1.min.x, r2.min.x);
		intersectionRect.max.x = min(r1.max.x, r2.max.x);
		intersectionRect.max.y = min(r1.max.y, r2.max.y);
		intersectionRect.min.y = max(r1.min.y, r2.min.y);
	} else intersectionRect = rect(0,0,0,0);
	
	return intersectionRect;
};

bool rectGetIntersection(Rect * intersectionRect, Rect r1, Rect r2) {
	bool hasIntersection = rectIntersection(r1, r2);
	if (hasIntersection) {
		intersectionRect->min.x = max(r1.min.x, r2.min.x);
		intersectionRect->max.x = min(r1.max.x, r2.max.x);
		intersectionRect->max.y = min(r1.max.y, r2.max.y);
		intersectionRect->min.y = max(r1.min.y, r2.min.y);
	}
	else *intersectionRect = rect(0,0,0,0);
	
	return hasIntersection;
};

bool pointInRect(Vec2 p, Rect r) {
	bool inRect = ( p.x >= r.min.x &&
					p.x <= r.max.x &&
					p.y >= r.min.y &&
					p.y <= r.max.y   );

	return inRect;
}

bool pointInRectEx(Vec2 p, Rect r) {
	bool inRect = ( p.x >= r.min.x &&
					p.x <  r.max.x &&
					p.y >= r.min.y &&
					p.y <  r.max.y   );

	return inRect;
}

bool rectEmpty(Rect r) {
	bool result = (r == rect(0,0,0,0));
	return result;
}

bool rectZero(Rect r) {
	return (rectW(r) <= 0.0f || rectH(r) <= 0.0f);
}

bool rectInsideRect(Rect r0, Rect r1) {
	bool result = (r0.min.x > r1.min.x &&
	               r0.min.y > r1.min.y &&
	               r0.max.x < r1.max.x &&
	               r0.max.y < r1.max.y);
	return result;
}

Vec2 rectInsideRectClamp(Rect r0, Rect r1) {
	Vec2 offset = vec2(0,0);
	if(r0.min.x < r1.min.x) offset += vec2(r1.min.x-r0.min.x, 0);
	if(r0.min.y < r1.min.y) offset += vec2(0, r1.min.y-r0.min.y);
	if(r0.max.x > r1.max.x) offset += vec2(r1.max.x-r0.max.x, 0);
	if(r0.max.y > r1.max.y) offset += vec2(0, r1.max.y-r0.max.y);

	return offset;
}

Rect mapRect(Rect r, Rect oldInterp, Rect newInterp) {
	Rect result = r;
	result.min = mapVec2(result.min, oldInterp.min, oldInterp.max, newInterp.min, newInterp.max);
	result.max = mapVec2(result.max, oldInterp.min, oldInterp.max, newInterp.min, newInterp.max);

	return result;
}

Vec2 rectDistancePos(Rect r, Vec2 p) {
	Vec2 result;
		 if(p.x >= r.max.x) result.x = p.x - r.max.x;
	else if(p.x <= r.min.x) result.x = p.x - r.min.x;
		 if(p.y >= r.max.y) result.y = p.y - r.max.y;
	else if(p.y <= r.min.y) result.y = p.y - r.min.y;
	return result;
}

Rect rectRound(Rect r) {
	for(int i = 0; i < 4; i++) r.e[i] = roundFloat(r.e[i]);
	return r;
}

//
//
//

inline Recti recti() { return {0,0,0,0}; }
inline Recti recti(int left, int bottom, int right, int top) { 
	return {left,bottom,right,top}; 
}
inline Recti rectiRound(Rect r) {
	return recti(roundInt(r.left), roundInt(r.bottom), 
	             roundInt(r.right), roundInt(r.top));
}

//
//
//

inline Rect3 rect3(Vec3 min, Vec3 max) {
	Rect3 r;
	r.min = min;
	r.max = max;

	return r;
}

inline Rect3 rect3CenDim(Vec3 cen, Vec3 dim) {
	Rect3 r;
	r.min = cen-dim*0.5f;
	r.max = cen+dim*0.5f;

	return r;
}

// Rect rectExpand(Rect r, Rect rExpand) {
// 	Vec2 dim = rectDim(rExpand);
// 	r.min -= dim/2;
// 	r.max += dim/2;

// 	return r;
// }

Rect3 rect3Expand(Rect3 r, Vec3 dim) {
	r.min -= (dim/2);
	r.max += (dim/2);

	return r;
}

Vec3 rect3Dim(Rect3 r) {
	return r.max - r.min;
}

Vec3 rect3Cen(Rect3 r) {
	return r.min + rect3Dim(r)/2;
}

bool pointInBox(Vec3 p, Vec3 bMin, Vec3 bMax) {
	bool result = ( p.x >= bMin.x &&
					p.x <= bMax.x &&
					p.y >= bMin.y &&
					p.y <= bMax.y &&
					p.z >= bMin.z &&
					p.z <= bMax.z );

	return result;
}

//
//
//

inline Rect3i rect3i(Vec3i min, Vec3i max) {
	Rect3i r;
	r.min = min;
	r.max = max;

	return r;
}

inline Rect3i rect3iCenRad(Vec3i cen, Vec3i radius) {
	Rect3i r;
	r.min = cen-radius;
	r.max = cen+radius;

	return r;
}

Rect3i rect3iExpand(Rect3i r, Vec3i dim) {
	r.min -= (dim/2);
	r.max += (dim/2);

	return r;
}

//
//
//

float ellipseDistanceCenterEdge(float width, float height, Vec2 dir) {
	// dir has to be normalized
	float dirOverWidth = dir.x/width;
	float dirOverHeight = dir.y/height;
	float length = sqrt(1 / (dirOverWidth*dirOverWidth + dirOverHeight*dirOverHeight));
	return length;
}

bool ellipseGetLineIntersection(float a, float b, float h, float k, Vec2 p0, Vec2 p1, Vec2 &i1, Vec2 &i2) {
	// y = mx + c
	float m;
	if(p1.x-p0.x == 0) m = 1000000;
	// if(p1.x-p0.x == 0) m = 0.00001f;
	else m = (p1.y-p0.y)/(p1.x-p0.x);
	float c = p0.y - m*p0.x;

	float aa = a*a;
	float bb = b*b;
	float mm = m*m;
	float temp1 = c + m*h;
	float d = aa*mm + bb - pow(temp1,2) - k*k + 2*temp1*k;

	if (d < 0)
	{
		return false;
	}
	else
	{
		Vec2 inter1;
		Vec2 inter2;
		float q = aa*mm + bb;
		float r = h*bb - m*aa*(c-k);
		float s = bb*temp1 + k*aa*mm;

		float u = a*b*sqrt(d);
		inter1.x = (r - u) / q;
		inter2.x = (r + u) / q;

		float v = a*b*m*sqrt(d);
		inter1.y = (s - v) / q;
		inter2.y = (s + v) / q;

		float lLine = lenVec2(p1 - p0);
		float lp0 = lenVec2(inter1 - p0);
		float lp1 = lenVec2(inter2 - p0);
		if(lp0 <= lp1) {
			i1 = inter1;
			i2 = inter2;
		} else {
			i1 = inter2;
			i2 = inter1;			
		}
		if(lenVec2(i1 - p0) > lLine) return false;
		// if(lp0 > lLine) return false;

		return true;
	}

	return false;
}

Vec2 ellipseNormal(Vec2 pos, float width, float height, Vec2 point) {
	Vec2 dir = vec2((point.x-pos.x)/pow(width,2), (point.y-pos.y)/pow(height,2));
	dir = normVec2(dir);
	dir *= -1;
	return dir;
}

Mat4 orthographicProjection(float left, float right, float bottom, float top, float nearr, float farr) {
	Mat4 mat;
	float v[16] = { 2/(right-left), 0, 0, 0, 
					0, 2/(top-bottom), 0, 0, 
					0, 0, (-2)/(farr-nearr), 0, 
					(-(right+left)/(right-left)), (-(top+bottom)/(top-bottom)), (farr+nearr)/(farr-nearr), 1};

	for(int i = 0; i < 16; ++i) mat.e[i] = v[i];

	return mat;
}

float polygonArea(Vec2* polygon, int count) {
	float signedArea = 0;
	for(int i = 0; i < count; i++) {
		int secondIndex = i+1;
		if(i == count-1) secondIndex = 0;
		Vec2 p1 = polygon[i];
		Vec2 p2 = polygon[secondIndex];

		signedArea += (p1.x * p2.y - p2.x * p1.y);
	}

	return signedArea / 2;
}




void whiteNoise(Rect region, int sampleCount, Vec2* samples) {
	for(int i = 0; i < sampleCount; ++i) {
		Vec2 randomPos = vec2(randomInt(region.min.x, region.max.x), 
		                      randomInt(region.min.y, region.max.y));
		samples[i] = randomPos;
	}
}

// Wraps automatically.
int blueNoise(Rect region, float radius, Vec2** noiseSamples) {
	bool wrapAround = true;

	Vec2 dim = rectDim(region);
	float cs = radius/M_SQRT2;

	Vec2i gdim = vec2i(roundUpFloat(dim.w/cs), roundUpFloat(dim.h/cs));
	int gridSize = gdim.w*gdim.h;

	*noiseSamples = mallocArray(Vec2, gridSize);
	Vec2* samples = *noiseSamples;
	int sampleCount = 0;

	float randPrecision = 0.0001f;
	int testCount = 30;

	int* grid = mallocArray(int, gridSize);
	for(int i = 0; i < gridSize; i++) grid[i] = -1;

	int* activeList = mallocArray(int, gridSize);
	int activeListCount = 0;

	// Setup first sample randomly.
	samples[sampleCount++] = vec2(randomFloatPCG(0, dim.w, randPrecision), randomFloatPCG(0, dim.h, randPrecision));
	activeList[activeListCount++] = 0;

	Vec2 pos = samples[0];
	Vec2i gridPos = vec2i(pos/cs);
	grid[gridPos.y*gdim.w + gridPos.x] = 0;

	Rect regionOrigin = rectTrans(region, region.min*-1);
	Vec2 regionDim = rectDim(regionOrigin);
	while(activeListCount > 0) {

		int activeIndex = randomIntPCG(0, activeListCount-1);
		int sampleIndex = activeList[activeIndex];
		Vec2 activeSample = samples[sampleIndex];

		for(int i = 0; i < testCount; ++i) {
			float angle = randomFloatPCG(0, M_2PI, randPrecision);
			float distance = randomFloatPCG(radius, radius*2, randPrecision);
			Vec2 newSample = activeSample + angleToDir(angle)*distance;

			if(!pointInRect(newSample, regionOrigin)) continue;

			// Search around sample point.

			int minx = roundDownFloat((newSample.x - radius*2.0f)/cs);
			int miny = roundDownFloat((newSample.y - radius*2.0f)/cs);
			int maxx = roundDownFloat((newSample.x + radius*2.0f)/cs);
			int maxy = roundDownFloat((newSample.y + radius*2.0f)/cs);

			bool validPosition = true;
			for(int y = miny; y <= maxy; ++y) {
				for(int x = minx; x <= maxx; ++x) {
					int mx = mod(x, gdim.w);
					int my = mod(y, gdim.h);
					int index = grid[my*gdim.w+mx];

					if(index > -1) {
						Vec2 s = samples[index];

						// Wrap sample position if we check outside boundaries.
						if(mx != x) s.x += mx > x ? -regionDim.w : regionDim.w;
						if(my != y) s.y += my > y ? -regionDim.h : regionDim.h;

						float distance = lenVec2(s - newSample);
						if(distance < radius) {
							validPosition = false;
							break;
						}
					}
				}
				if(!validPosition) break;
			}

			if(validPosition) {
				samples[sampleCount] = newSample;
				activeList[activeListCount++] = sampleCount;

				Vec2i gridPos = vec2i(newSample/cs);
				grid[gridPos.y*gdim.w + gridPos.x] = sampleCount;
				sampleCount++;
			}
		}

		// delete active sample after testCount times
		activeList[activeIndex] = activeList[activeListCount-1];
		activeListCount--;
	}

	for(int i = 0; i < sampleCount; ++i) samples[i] += region.min;

	free(grid);
	free(activeList);

	return sampleCount;
}

inline float linearToGamma(float a) {
	return powf(a, 1/2.2f);
}

inline float gammaToLinear(float a) {
	return powf(a, 2.2f);
}

Vec3 gammaToLinear(Vec3 c) {
	c.x = powf(c.r, 2.2f);
	c.y = powf(c.g, 2.2f);
	c.z = powf(c.b, 2.2f);
	return c;
}
Vec3 linearToGamma(Vec3 c) {
	c.x = powf(c.r, 1/2.2f);
	c.y = powf(c.g, 1/2.2f);
	c.z = powf(c.b, 1/2.2f);
	return c;
}

// These should be called srgbToLinearSpace or something.
// Also these are regular gamma and not srgb.
Vec4 colorSRGB(Vec4 color) {
	color.r = powf(color.r, 2.2f);
	color.g = powf(color.g, 2.2f);
	color.b = powf(color.b, 2.2f);
	// color.a = powf(color.a, 2.2f);
	// color.a = root(color.a, 2.85f);
	return color;
}

Vec3 colorSRGB(Vec3 color) {
	color.x = powf(color.x, 2.2f);
	color.y = powf(color.y, 2.2f);
	color.z = powf(color.z, 2.2f);
	return color;
}

// Taken from d3dx_dxgiformatconvert.inl.
inline float srgbToFloat(float val) {
    if( val < 0.04045f )
        val /= 12.92f;
    else
        val = pow((val + 0.055f)/1.055f,2.4f);
    return val;
}
inline float floatToSrgb(float val) { 
    if( val < 0.0031308f )
        val *= 12.92f;
    else
        val = 1.055f * pow(val,1.0f/2.4f) - 0.055f;
    return val;
}


struct SortPair {
	float key;
	int index;
};

void bubbleSort(int* list, int size) {
	for(int off = 0; off < size-1; off++) {
		bool sw = false;

		for(int i = 0; i < size-1 - off; i++) {
			if(list[i+1] < list[i]) {
				swap(list + i+1, list + i);
				sw = true;
			}
		}

		if(!sw) break;
	}
}

void bubbleSort(SortPair* list, int size, bool sortDirection = false) {
	for(int off = 0; off < size-1; off++) {
		bool sw = false;

		for(int i = 0; i < size-1 - off; i++) {
			bool result = sortDirection ? (list[i+1].key > list[i].key) : 
										  (list[i+1].key < list[i].key);
			if(result) {
				swapGeneric(SortPair, list[i], list[i+1]);
				sw = true;
			}
		}

		if(!sw) break;
	}
}

void mergeSort(int* list, int size) {
	// int* buffer = getTArray(int, size);
	int* buffer = (int*)malloc(sizeof(int)*size);
	int stage = 0;

	for(;;) {
		stage++;
		int stageSize = 1 << stage;
		int splitSize = 1 << stage-1;

		int* src = stage%2 == 0 ? list : buffer;
		int* dest = stage%2 == 0 ? buffer : list;

		int count = ceil(size/(float)splitSize);
		if(count <= 1) {
			if(stage%2 == 0) memCpy(list, buffer, size);
			break;
		}

		for(int i = 0; i < size; i += stageSize) {
			int* fbuf = src + i;
			int fi = 0;
			int remainder = size - i;
			int as0 = min(splitSize, remainder);
			int as1 = min(splitSize, clampMin(remainder-splitSize, 0));
			int ai0 = 0; 
			int ai1 = 0;
			int* buf0 = dest + i;
			int* buf1 = buf0 + as0;

			for(;;) {
				if(ai0 < as0 && ai1 < as1) {
					if(buf0[ai0] < buf1[ai1]) fbuf[fi++] = buf0[ai0++];
					else 					  fbuf[fi++] = buf1[ai1++];
				} 
				else if(ai0 < as0) fbuf[fi++] = buf0[ai0++];
				else if(ai1 < as1) fbuf[fi++] = buf1[ai1++];
				else break;
			}
		}
	}

	free(buffer);
}


// sorts in bytes
void radixSort(int* list, int size) {
	// int* buffer = getTArray(int, size);
	int* buffer = (int*)malloc(sizeof(int)*size);
	int stageCount = 4;

	for(int stage = 0; stage < stageCount; stage++) {
		int* src = stage%2 == 0 ? list : buffer;
		int* dst = stage%2 == 0 ? buffer : list;
		int bucket[257] = {};
		int offset = 8*stage;

		// count 
		for(int i = 0; i < size; i++) {
			uchar byte = src[i] >> offset;
			bucket[byte+1]++;
		}

		// turn sizes into offsets
		for(int i = 0; i < 256-1; i++) {
			bucket[i+1] += bucket[i];
		}

		for(int i = 0; i < size; i++) {
			uchar byte = src[i] >> offset;
			dst[bucket[byte]] = src[i];
			bucket[byte]++;
		}
	}

	free(buffer);
}

// void radixSortSimd(int* list, int size) {
// 	// int* buffer = getTArray(int, size);
// 	int* buffer = (int*)malloc(sizeof(int)*size);

// 	int stageCount = 4;

// 	for(int stage = 0; stage < stageCount; stage++) {
// 		int* src = stage%2 == 0 ? list : buffer;
// 		int* dst = stage%2 == 0 ? buffer : list;
// 		int bucket[257] = {};
// 		int offset = 8*stage;

// 		__m128i stageS = _mm_set1_epi32(stage*8);
// 		__m128i one = _mm_set1_epi32(1);

// 		if(size % 4 != 0) {
// 			int rest = size % 4;
// 			for(int i = 0; i < rest; i++) {
// 				uchar byte = src[i] >> offset;
// 				bucket[byte+1]++;
// 			}
// 		}

// 		for(int i = 0; i < size; i += 4) {
// 			__m128i byte = _mm_set_epi32(src[i], src[i+1], src[i+2], src[i+3]);
// 			byte = _mm_srl_epi32(byte, stageS);
// 			byte = _mm_add_epi32(byte, one);
// 			bucket[byte.m128i_u8[0]] = bucket[byte.m128i_u8[0]] + 1;
// 			bucket[byte.m128i_u8[1]] = bucket[byte.m128i_u8[1]] + 1;
// 			bucket[byte.m128i_u8[2]] = bucket[byte.m128i_u8[2]] + 1;
// 			bucket[byte.m128i_u8[3]] = bucket[byte.m128i_u8[3]] + 1;
// 		}

// 		// turn sizes into offsets
// 		for(int i = 0; i < 256-1; i++) {
// 			bucket[i+1] += bucket[i];
// 		}

// 		for(int i = 0; i < size; i++) {
// 			uchar byte = src[i] >> offset;
// 			dst[bucket[byte]] = src[i];
// 			bucket[byte]++;
// 		}
// 	}

// 	free(buffer);
// }

void radixSortPair(SortPair* list, int size) {
	// SortPair* buffer = getTArray(SortPair, size);
	SortPair* buffer = (SortPair*)malloc(sizeof(SortPair)*size);

	int stageCount = 4;

	for(int stage = 0; stage < stageCount; stage++) {
		SortPair* src = stage%2 == 0 ? list : buffer;
		SortPair* dst = stage%2 == 0 ? buffer : list;
		int bucket[257] = {};

		// count 
		for(int i = 0; i < size; i++) {
			uchar byte = *((int*)&src[i].key) >> (8*stage);
			bucket[byte+1]++;
		}

		// turn sizes into offsets
		for(int i = 0; i < 256-1; i++) {
			bucket[i+1] += bucket[i];
		}

		for(int i = 0; i < size; i++) {
			uchar byte = *((int*)&src[i].key) >> (8*stage);
			dst[bucket[byte]] = src[i];
			bucket[byte]++;
		}
	}

	free(buffer);
}

void getPointsFromQuadAndNormal(Vec3 p, Vec3 normal, float size, Vec3 verts[4]) {
	int sAxis[2];
	int biggestAxis = getBiggestAxis(normal, sAxis);

	float s2 = size*0.5f;

	for(int i = 0; i < 4; i++) {
		Vec3 d = p;
		if(i == 0) { d.e[sAxis[0]] += -s2; d.e[sAxis[1]] += -s2; }
		else if(i == 1) { d.e[sAxis[0]] += -s2; d.e[sAxis[1]] +=  s2; }
		else if(i == 2) { d.e[sAxis[0]] +=  s2; d.e[sAxis[1]] +=  s2; }
		else if(i == 3) { d.e[sAxis[0]] +=  s2; d.e[sAxis[1]] += -s2; }
		verts[i] = d;
	}
}

//

static int SEED = 0;

static int hash[] = {208,34,231,213,32,248,233,56,161,78,24,140,71,48,140,254,245,255,247,247,40,
	185,248,251,245,28,124,204,204,76,36,1,107,28,234,163,202,224,245,128,167,204,
	9,92,217,54,239,174,173,102,193,189,190,121,100,108,167,44,43,77,180,204,8,81,
	70,223,11,38,24,254,210,210,177,32,81,195,243,125,8,169,112,32,97,53,195,13,
	203,9,47,104,125,117,114,124,165,203,181,235,193,206,70,180,174,0,167,181,41,
	164,30,116,127,198,245,146,87,224,149,206,57,4,192,210,65,210,129,240,178,105,
	228,108,245,148,140,40,35,195,38,58,65,207,215,253,65,85,208,76,62,3,237,55,89,
	232,50,217,64,244,157,199,121,252,90,17,212,203,149,152,140,187,234,177,73,174,
	193,100,192,143,97,53,145,135,19,103,13,90,135,151,199,91,239,247,33,39,145,
	101,120,99,3,186,86,99,41,237,203,111,79,220,135,158,42,30,154,120,67,87,167,
	135,176,183,191,253,115,184,21,233,58,129,233,142,39,128,211,118,137,139,255,
	114,20,218,113,154,27,127,246,250,1,8,198,250,209,92,222,173,21,88,102,219};

int noise2(int x, int y)
{
	int tmp = hash[(y + SEED) % 256];
	return hash[(tmp + x) % 256];
}

float lin_inter(float x, float y, float s)
{
	return x + s * (y-x);
}

float smooth_inter(float x, float y, float s)
{
	return lin_inter(x, y, s * s * (3-2*s));
}

float noise2d(float x, float y)
{
	int x_int = x;
	int y_int = y;
	float x_frac = x - x_int;
	float y_frac = y - y_int;
	int s = noise2(x_int, y_int);
	int t = noise2(x_int+1, y_int);
	int u = noise2(x_int, y_int+1);
	int v = noise2(x_int+1, y_int+1);
	float low = smooth_inter(s, t, x_frac);
	float high = smooth_inter(u, v, x_frac);
	return smooth_inter(low, high, y_frac);
}

float perlin2d(float x, float y, float freq, int depth)
{
	float xa = x*freq;
	float ya = y*freq;
	float amp = 1.0;
	float fin = 0;
	float div = 0.0;

	int i;
	for(i=0; i<depth; i++)
	{
		div += 256 * amp;
		fin += noise2d(xa, ya) * amp;
		amp /= 2;
		xa *= 2;
		ya *= 2;
	}

	return fin/div;
}

//

struct GraphCam {
	double x, y, w, h;
	double left, bottom, right, top;
	double xMin, xMax, yMin, yMax;
	Rect viewPort;
};

void graphCamSetBoundaries(GraphCam* cam, double xMin, double xMax, double yMin, double yMax) {
	cam->xMin = xMin;
	cam->xMax = xMax;
	cam->yMin = yMin;
	cam->yMax = yMax;	
}

void graphCamInit(GraphCam* cam, double x, double y, double w, double h, double xMin, double xMax, double yMin, double yMax) {
	cam->x = x;
	cam->y = y;
	cam->w = w;
	cam->h = h;
	graphCamSetBoundaries(cam, xMin, xMax, yMin, yMax);
}

void graphCamInit(GraphCam* cam, double xMin, double xMax, double yMin, double yMax) {
	double w = xMax - xMin;
	double h = yMax - yMin;
	graphCamInit(cam, xMin+w/2, yMin+h/2, w, h, xMin, xMax, yMin, yMax);
}

void graphCamSetViewPort(GraphCam* cam, Rect viewPort) {
	cam->viewPort = viewPort;
}

inline double graphCamLeft(GraphCam* cam) { return cam->x - cam->w/2; }
inline double graphCamRight(GraphCam* cam) { return cam->x + cam->w/2; }
inline double graphCamBottom(GraphCam* cam) { return cam->y - cam->h/2; }
inline double graphCamTop(GraphCam* cam) { return cam->y + cam->h/2; }

void graphCamUpdateSides(GraphCam* cam) {
	cam->left = graphCamLeft(cam);
	cam->bottom = graphCamBottom(cam);
	cam->right = graphCamRight(cam);
	cam->top = graphCamTop(cam);
}

void graphCamSizeClamp(GraphCam* cam, double wMin, double hMin, double wMax = -1, double hMax = -1) {
	if(wMax == -1) wMax = cam->xMax - cam->xMin;
	if(hMax == -1) hMax = cam->yMax - cam->yMin;

	clampDouble(&cam->w, wMin, wMax);
	clampDouble(&cam->h, hMin, hMax);
}

void graphCamScaleToPos(GraphCam* cam, int xAmount, double xScale, double xClamp, int yAmount, double yScale, double yClamp, Vec2 pos) {
	double diff, offset;
	float posOffset;
	double mod;

	diff = cam->w;
	mod = pow(xScale, abs(xAmount));
	offset = xAmount > 0 ? mod : 1/mod;
	cam->w *= offset;
	clampMinDouble(&cam->w, xClamp);
	diff -= cam->w;
	posOffset = mapRange(pos.x, cam->viewPort.left, cam->viewPort.right, -0.5f, 0.5f);
	cam->x += diff * posOffset;

	diff = cam->h;
	mod = pow(yScale, abs(yAmount));
	offset = yAmount > 0 ? mod : 1/mod;
	cam->h *= offset;
	clampMinDouble(&cam->h, yClamp);
	diff -= cam->h;
	posOffset = mapRange(pos.y, cam->viewPort.bottom, cam->viewPort.top, -0.5f, 0.5f);
	cam->y += diff * posOffset;

	graphCamUpdateSides(cam);
}

void graphCamCalcScale(GraphCam* cam, int xAmount, double xScale, float pos, double* newPos, double* newSize) {
	double diff, offset;
	float posOffset;
	double mod;

	diff = cam->w;
	mod = pow(xScale, abs(xAmount));
	offset = xAmount > 0 ? mod : 1/mod;
	*newSize = cam->w * offset;

	diff -= (*newSize);
	posOffset = mapRange(pos, cam->viewPort.left, cam->viewPort.right, -0.5f, 0.5f);
	*newPos = cam->x + (diff * posOffset);
}

void graphCamPosClamp(GraphCam* cam, double xMin, double xMax, double yMin, double yMax) {
	clampDouble(&cam->x, xMin, xMax);
	clampDouble(&cam->y, yMin, yMax);

	graphCamUpdateSides(cam);
}

void graphCamPosClamp(GraphCam* cam) {
	graphCamPosClamp(cam, cam->xMin + cam->w/2, cam->xMax - cam->w/2, cam->yMin + cam->h/2, cam->yMax - cam->h/2);
}

void graphCamTrans(GraphCam* cam, double xTrans, double yTrans) {
	cam->x += xTrans * (cam->w/(rectW(cam->viewPort)));
	cam->y += yTrans * (cam->h/(rectH(cam->viewPort)));

	graphCamUpdateSides(cam);
}

double graphCamScreenToCamSpaceX(GraphCam* cam, float v) {
	return v * (cam->w/(rectW(cam->viewPort)));
}

double graphCamScreenToCamSpaceY(GraphCam* cam, float v) {
	return v * (cam->h/(rectH(cam->viewPort)));
}

float graphCamCamToScreenSpaceX(GraphCam* cam, double v) {
	return v * ((rectW(cam->viewPort))/cam->w);
}

float graphCamCamToScreenSpaceY(GraphCam* cam, double v) {
	return v * ((rectH(cam->viewPort))/cam->h);
}

// Maps camera space to view/screenspace.
inline float graphCamMapX(GraphCam* cam, double v) {
	float x = mapRangeDouble(v, cam->left, cam->right, cam->viewPort.left, cam->viewPort.right);
	return x;
}

inline float graphCamMapY(GraphCam* cam, double v) {
	float y = mapRangeDouble(v, cam->bottom, cam->top, cam->viewPort.bottom, cam->viewPort.top);
	return y;
}

Vec2 graphCamMap(GraphCam* cam, double x, double y) {
	Vec2 vec;
	vec.x = graphCamMapX(cam, x);
	vec.y = graphCamMapY(cam, y);

	return vec;
}

// Maps view/screenspace to camera.
inline double graphCamMapXReverse(GraphCam* cam, float v) {
	double x = mapRangeDouble(v, cam->viewPort.left, cam->viewPort.right, cam->left, cam->right);
	return x;
}

inline double graphCamMapYReverse(GraphCam* cam, float v) {
	double y = mapRangeDouble(v, cam->viewPort.bottom, cam->viewPort.top, cam->bottom, cam->top);
	return y;
}

Rect graphCamMiniMap(GraphCam* cam, Rect viewPort) {
	Rect r;
	r.left = mapRange(cam->left,     cam->xMin, cam->xMax, viewPort.left, viewPort.right);
	r.bottom = mapRange(cam->bottom, cam->yMin, cam->yMax, viewPort.bottom, viewPort.top);
	r.right = mapRange(cam->right,   cam->xMin, cam->xMax, viewPort.left, viewPort.right);
	r.top = mapRange(cam->top,       cam->yMin, cam->yMax, viewPort.bottom, viewPort.top);
	return r;
}

// 

float logBase(float v, float base) {
	float result = log(v) / log(base);
	return result;
}

// 

