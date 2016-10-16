#include <math.h>
#include <assert.h>
#include "polyclip.h"

struct ray {
	float origin[3];
	float dir[3];
};

static int clip_edge(struct g3d_vertex *poly, int *vnumptr,
		const struct g3d_vertex *v0, const struct g3d_vertex *v1,
		const struct cplane *plane);
static float distance_signed(float *pos, const struct cplane *plane);
static int intersect(const struct ray *ray, const struct cplane *plane, float *t);


int clip_poly(struct g3d_vertex *vout, int *voutnum,
		const struct g3d_vertex *vin, int vnum, struct cplane *plane)
{
	int i;
	int edges_clipped = 0;
	int out_vnum = 0;

	for(i=0; i<vnum; i++) {
		int res = clip_edge(vout, &out_vnum, vin + i, vin + (i + 1) % vnum, plane);
		if(res == 0) {
			edges_clipped++;
		}
	}

	if(out_vnum <= 0) {
		assert(edges_clipped == 0);
		return -1;
	}

	*voutnum = out_vnum;
	return edges_clipped > 0 ? 0 : 1;
}

#define LERP_VATTR(res, v0, v1, t) \
	do { \
		(res)->nx = (v0)->nx + ((v1)->nx - (v0)->nx) * (t); \
		(res)->ny = (v0)->ny + ((v1)->ny - (v0)->ny) * (t); \
		(res)->nz = (v0)->nz + ((v1)->nz - (v0)->nz) * (t); \
		(res)->u = (v0)->u + ((v1)->u - (v0)->u) * (t); \
		(res)->v = (v0)->v + ((v1)->v - (v0)->v) * (t); \
		(res)->r = (v0)->r + ((v1)->r - (v0)->r) * (t); \
		(res)->g = (v0)->g + ((v1)->g - (v0)->g) * (t); \
		(res)->b = (v0)->b + ((v1)->b - (v0)->b) * (t); \
	} while(0)


/* returns:
 *  1 -> both inside
 *  0 -> straddling and clipped
 * -1 -> both outside
 *
 *  also returns the size of the polygon through vnumptr
 */
static int clip_edge(struct g3d_vertex *poly, int *vnumptr,
		const struct g3d_vertex *v0, const struct g3d_vertex *v1,
		const struct cplane *plane)
{
	float pos0[3], pos1[3];
	float d0, d1, t;
	struct ray ray;
	int i, vnum = *vnumptr;

	pos0[0] = v0->x; pos0[1] = v0->y; pos0[2] = v0->z;
	pos1[0] = v1->x; pos1[1] = v1->y; pos1[2] = v1->z;

	d0 = distance_signed(pos0, plane);
	d1 = distance_signed(pos1, plane);

	for(i=0; i<3; i++) {
		ray.origin[i] = pos0[i];
		ray.dir[i] = pos1[i] - pos0[i];
	}

	if(d0 >= 0.0) {
		/* start inside */
		if(d1 >= 0.0) {
			/* all inside */
			poly[vnum++] = *v1;	/* append v1 */
			*vnumptr = vnum;
			return 1;
		} else {
			/* going out */
			struct g3d_vertex *vptr = poly + vnum;

			intersect(&ray, plane, &t);

			vptr->x = ray.origin[0] + ray.dir[0] * t;
			vptr->y = ray.origin[1] + ray.dir[1] * t;
			vptr->z = ray.origin[2] + ray.dir[2] * t;
			vptr->w = 1.0f;

			LERP_VATTR(vptr, v0, v1, t);
			vnum++;	/* append new vertex on the intersection point */
		}
	} else {
		/* start outside */
		if(d1 >= 0) {
			/* going in */
			struct g3d_vertex *vptr = poly + vnum;

			intersect(&ray, plane, &t);

			vptr->x = ray.origin[0] + ray.dir[0] + t;
			vptr->y = ray.origin[1] + ray.dir[1] + t;
			vptr->z = ray.origin[2] + ray.dir[2] + t;
			vptr->w = 1.0f;

			LERP_VATTR(vptr, v0, v1, t);
			vnum++;	/* append new vertex on the intersection point */

			/* then append v1 ... */
			poly[vnum++] = *v1;
		} else {
			/* all outside */
			return -1;
		}
	}

	*vnumptr = vnum;
	return 0;
}


static float distance_signed(float *pos, const struct cplane *plane)
{
	float dx = pos[0] - plane->x;
	float dy = pos[1] - plane->y;
	float dz = pos[2] - plane->z;
	return dx * plane->nx + dy * plane->ny + dz * plane->nz;
}

static int intersect(const struct ray *ray, const struct cplane *plane, float *t)
{
	float orig_pt_dir[3];

	float ndotdir = plane->nx * ray->dir[0] + plane->ny * ray->dir[1] + plane->nz * ray->dir[2];
	if(fabs(ndotdir) < 1e-4) {
		*t = 0.0f;
		return 0;
	}

	orig_pt_dir[0] = plane->x - ray->origin[0];
	orig_pt_dir[1] = plane->y - ray->origin[1];
	orig_pt_dir[2] = plane->z - ray->origin[2];

	*t = (plane->nx * orig_pt_dir[0] + plane->ny * orig_pt_dir[1] + plane->nz * orig_pt_dir[2]) / ndotdir;
	return 1;
}