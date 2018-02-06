#ifndef VMATH_H_
#define VMATH_H_

#ifdef __GNUC__
#define INLINE __inline

#elif defined(__WATCOMC__)
#define INLINE __inline

#else
#define INLINE
#endif

typedef struct { float x, y, z; } vec3_t;
typedef struct { float x, y, z, w; } vec4_t;

typedef vec4_t quat_t;

/* vector functions */
static INLINE vec3_t v3_cons(float x, float y, float z)
{
	vec3_t res;
	res.x = x;
	res.y = y;
	res.z = z;
	return res;
}

static INLINE float v3_dot(vec3_t v1, vec3_t v2)
{
	return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

/* quaternion functions */
static INLINE quat_t quat_cons(float s, float x, float y, float z)
{
	quat_t q;
	q.x = x;
	q.y = y;
	q.z = z;
	q.w = s;
	return q;
}

static INLINE vec3_t quat_vec(quat_t q)
{
	vec3_t v;
	v.x = q.x;
	v.y = q.y;
	v.z = q.z;
	return v;
}

static INLINE quat_t quat_mul(quat_t q1, quat_t q2)
{
	quat_t res;
	vec3_t v1 = quat_vec(q1);
	vec3_t v2 = quat_vec(q2);

	res.w = q1.w * q2.w - v3_dot(v1, v2);
	res.x = v2.x * q1.w + v1.x * q2.w + (v1.y * v2.z - v1.z * v2.y);
	res.y = v2.y * q1.w + v1.y * q2.w + (v1.z * v2.x - v1.x * v2.z);
	res.z = v2.z * q1.w + v1.z * q2.w + (v1.x * v2.y - v1.y * v2.x);
	return res;
}

static INLINE void quat_to_mat(float *res, quat_t q)
{
	res[0] = 1.0f - 2.0f * q.y*q.y - 2.0f * q.z*q.z;
	res[1] = 2.0f * q.x * q.y - 2.0f * q.w * q.z;
	res[2] = 2.0f * q.z * q.x + 2.0f * q.w * q.y;
	res[3] = 0.0f;
	res[4] = 2.0f * q.x * q.y + 2.0f * q.w * q.z;
	res[5] = 1.0f - 2.0f * q.x*q.x - 2.0f * q.z*q.z;
	res[6] = 2.0f * q.y * q.z - 2.0f * q.w * q.x;
	res[7] = 0.0f;
	res[8] = 2.0f * q.z * q.x - 2.0f * q.w * q.y;
	res[9] = 2.0f * q.y * q.z + 2.0f * q.w * q.x;
	res[10] = 1.0f - 2.0f * q.x*q.x - 2.0f * q.y*q.y;
	res[11] = 0.0f;
	res[12] = res[13] = res[14] = 0.0f;
	res[15] = 1.0f;
}

static INLINE quat_t quat_rotate(quat_t q, float angle, float x, float y, float z)
{
	quat_t rq;
	float half_angle = angle * 0.5;
	float sin_half = sin(half_angle);

	rq.w = cos(half_angle);
	rq.x = x * sin_half;
	rq.y = y * sin_half;
	rq.z = z * sin_half;

	return quat_mul(q, rq);
}

#endif	/* VMATH_H_ */