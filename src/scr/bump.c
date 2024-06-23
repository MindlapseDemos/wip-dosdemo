/* Bump effect (not moving yet of course, I have many ideas on this to commit before it's ready) */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "demo.h"
#include "screen.h"
#include "imago2.h"
#include "opt_rend.h"

static int init(void);
static void destroy(void);
static void start(long trans_time);
static void draw(void);

static struct screen scr = {
	"bump",
	init,
	destroy,
	start,
	0,
	draw
};

struct point {
	int x, y;
};

typedef struct Diff {
	short x, y;
} Diff;

#define NUM_BIG_LIGHTS 3
#define BIG_LIGHT_WIDTH 256
#define BIG_LIGHT_HEIGHT BIG_LIGHT_WIDTH

#define NUM_PARTICLES 64
#define PARTICLE_LIGHT_WIDTH 16
#define PARTICLE_LIGHT_HEIGHT 16

#define HMAP_WIDTH 256
#define HMAP_HEIGHT 256


static unsigned long startingTime;

static unsigned char *heightmap;
static unsigned short *lightmap;
static Diff *bumpOffset;
static Diff *bumpOffsetFinal;

static unsigned short *bigLight[NUM_BIG_LIGHTS];
static struct point bigLightPoint[NUM_BIG_LIGHTS];

static unsigned short *particleLight;
static struct point particlePoint[NUM_PARTICLES];


struct screen *bump_screen(void)
{
	return &scr;
}

static int loadHeightMapTest()
{
	static struct img_pixmap bumpPic;

	int x, y, i;
	uint32_t* src;
	int imgWidth, imgHeight;

	img_init(&bumpPic);
	if (img_load(&bumpPic, "data/bump/circ_col.png") == -1) {
		fprintf(stderr, "failed to load bump image\n");
		return -1;
	}

	src = (uint32_t*)bumpPic.pixels;
	imgWidth = bumpPic.width;
	imgHeight = bumpPic.width;

	i = 0;
	for (y = 0; y < HMAP_HEIGHT; ++y) {
		for (x = 0; x < HMAP_WIDTH; ++x) {
			uint32_t c32 = src[((y & (imgHeight - 1)) * imgWidth) + (x & (imgWidth - 1))];
			int a = (c32 >> 24) & 255;
			int r = (c32 >> 16) & 255;
			int g = (c32 >> 8) & 255;
			int b = c32 & 255;
			int c = (a + r + g + b) / 4;
			heightmap[i++] = c;
		}
	}

	return 0;
}

static int init(void)
{
	int i, j, x, y, c, r, g, b;

	const int hm_size = HMAP_WIDTH * HMAP_HEIGHT;
	const int fb_size = FB_WIDTH * FB_HEIGHT;

	const int lightRadius = BIG_LIGHT_WIDTH / 2;
	const int particleRadius = PARTICLE_LIGHT_WIDTH / 2;

	const int bigLightSize = BIG_LIGHT_WIDTH * BIG_LIGHT_HEIGHT;
	const int particleLightSize = PARTICLE_LIGHT_WIDTH * PARTICLE_LIGHT_HEIGHT;

	/* Just some parameters to temporary test the colors of 3 lights
	 * if every light uses it's own channel bits, it's better
	 */
	const float rgbMul[9] = { 0.75, 0,    0,
							  0,    0.75, 0,
							  0,    0,    0.75 };

	heightmap = malloc(sizeof(*heightmap) * hm_size);
	lightmap = malloc(sizeof(*lightmap) * fb_size);
	bumpOffset = malloc(sizeof(*bumpOffset) * hm_size);
	bumpOffsetFinal = malloc(sizeof(*bumpOffset) * hm_size);

	for (i = 0; i < NUM_BIG_LIGHTS; i++)
		bigLight[i] = malloc(sizeof(*bigLight[i]) * bigLightSize);

	particleLight = malloc(sizeof(*particleLight) * particleLightSize);

	memset(lightmap, 0, sizeof(*lightmap) * fb_size);
	memset(bumpOffset, 0, sizeof(*bumpOffset) * hm_size);
	memset(particlePoint, 0, sizeof(*particlePoint) * NUM_PARTICLES);

	if (loadHeightMapTest() == -1) return - 1;


	/* Inclination precalculation */
	i = 0;
	for (y = 0; y < HMAP_HEIGHT; y++) {
		const int yp0 = y * HMAP_WIDTH;
		const int yp1 = ((y+1) & (HMAP_HEIGHT-1)) * HMAP_WIDTH;
		for (x = 0; x < HMAP_WIDTH; x++) {
			const int ii = yp0 + x;
			const int iRight = yp0 + ((x + 1) & (HMAP_WIDTH - 1));
			const int iDown = yp1 + x;
			const float offsetPower = 0.75f;
			int dx, dy;

			dx = (int)((heightmap[ii] - heightmap[iRight]) * offsetPower);
			dy = (int)((heightmap[ii] - heightmap[iDown]) * offsetPower);

			CLAMP(dx, -32768, 32767);
			CLAMP(dy, -32768, 32767);

			bumpOffset[i].x = dx;
			bumpOffset[i].y = dy;
			++i;
		}
	}

	/* Generate three lights */
	i = 0;
	for (y = 0; y < BIG_LIGHT_HEIGHT; y++) {
		int yc = y - (BIG_LIGHT_HEIGHT / 2);
		for (x = 0; x < BIG_LIGHT_WIDTH; x++) {
			int xc = x - (BIG_LIGHT_WIDTH / 2);
			float d = (float)sqrt(xc * xc + yc * yc);
			float invDist = ((float)lightRadius - (float)sqrt(xc * xc + yc * yc)) / (float)lightRadius;
			if (invDist < 0.0f) invDist = 0.0f;

			c = (int)(invDist * 63);
			r = c >> 1;
			g = c;
			b = c >> 1;

			for (j = 0; j < NUM_BIG_LIGHTS; j++) {
				bigLight[j][i] = ((int)(r * rgbMul[j * 3]) << 11) | ((int)(g * rgbMul[j * 3 + 1]) << 5) | (int)(b * rgbMul[j * 3 + 2]);
			}
			i++;
		}
	}

	i = 0;
	for (y = 0; y < PARTICLE_LIGHT_HEIGHT; y++) {
		int yc = y - (PARTICLE_LIGHT_HEIGHT / 2);
		for (x = 0; x < PARTICLE_LIGHT_WIDTH; x++) {
			int xc = x - (PARTICLE_LIGHT_WIDTH / 2);

			float invDist = ((float)particleRadius - (float)sqrt(xc * xc + yc * yc)) / (float)particleRadius;
			if (invDist < 0.0f) invDist = 0.0f;

			c = (int)(pow(invDist, 0.75f) * 31);
			particleLight[i++] = ((c >> 1) << 11) | (c << 5) | (c >> 1);
		}
	}

	return 0;
}

static void destroy(void)
{
}

static void start(long trans_time)
{
	startingTime = time_msec;
}

static void renderLight(struct point *p, int width, int height, unsigned short *light)
{
	int x, y, dx;
	unsigned short *src, *dst;

	int x0 = p->x;
	int y0 = p->y;
	int x1 = p->x + width;
	int y1 = p->y + height;

	int xl = 0;
	int yl = 0;

	if (x0 < 0) {
		xl = -x0;
		x0 = 0;
	}

	if (y0 < 0) {
		yl = -y0;
		y0 = 0;
	}

	if (x1 > FB_WIDTH) x1 = FB_WIDTH;
	if (y1 > FB_HEIGHT) y1 = FB_HEIGHT;

	dx = x1 - x0;

	dst = lightmap + y0 * FB_WIDTH + x0;
	src = light + yl * width + xl;

	for (y = y0; y < y1; y++) {
		for (x = x0; x < x1; x++) {
			*dst++ |= *src++;
		}
		dst += FB_WIDTH - dx;
		src += width - dx;
	}
}

static void renderLights()
{
	int i;
	for (i = 0; i < NUM_BIG_LIGHTS; i++) {
		renderLight(&bigLightPoint[i], BIG_LIGHT_WIDTH, BIG_LIGHT_HEIGHT, bigLight[i]);
	}
}

static void renderParticles()
{
	int i;
	for (i = 0; i < NUM_PARTICLES; i++) {
		renderLight(&particlePoint[i], PARTICLE_LIGHT_WIDTH, PARTICLE_LIGHT_HEIGHT, particleLight);
	}
}

static void animateLights()
{
	struct point center;
	float dt = (float)(time_msec - startingTime) / 1000.0f;
	float tt = 1.0f - sin(dt);
	float disperse = tt * 16.0f;

	center.x = (FB_WIDTH >> 1) - (BIG_LIGHT_WIDTH / 2);
	center.y = (FB_HEIGHT >> 1) - (BIG_LIGHT_HEIGHT / 2);

	bigLightPoint[0].x = center.x + sin(1.2f * dt) * (96.0f + disperse);
	bigLightPoint[0].y = center.y + sin(0.8f * dt) * (64.0f + disperse);

	bigLightPoint[1].x = center.x + sin(1.5f * dt) * (80.0f + disperse);
	bigLightPoint[1].y = center.y + sin(1.1f * dt) * (48.0f + disperse);

	bigLightPoint[2].x = center.x + sin(2.0f * dt) * (72.0f + disperse);
	bigLightPoint[2].y = center.y + sin(1.3f * dt) * (56.0f + disperse);
}

static void renderBump(unsigned short *vram)
{
	int x,y,i=0;
	for (y = 0; y < FB_HEIGHT; ++y) {
		const int yp = (y & (HMAP_HEIGHT - 1)) * HMAP_WIDTH;
		for (x = 0; x < FB_WIDTH; ++x) {
			Diff* d = &bumpOffsetFinal[yp + (x & (HMAP_WIDTH - 1))];
			int ix = x + d->x;
			int iy = y + d->y;
			CLAMP(ix, 0, FB_WIDTH - 1);
			CLAMP(iy, 0, FB_HEIGHT - 1);
			vram[i++] = lightmap[iy * FB_WIDTH + ix];
		}
	}
}

static void animateParticles()
{
	int i;
	struct point center;
	float dt = (float)(time_msec - startingTime) / 2000.0f;
	float tt = 0.25f * sin(dt) + 0.75f;

	center.x = (FB_WIDTH >> 1) - (PARTICLE_LIGHT_WIDTH / 2);
	center.y = (FB_HEIGHT >> 1) - (PARTICLE_LIGHT_HEIGHT / 2);

	for (i = 0; i < NUM_PARTICLES; i++) {
		particlePoint[i].x = center.x + (sin(1.2f * (i*i*i + dt)) * 74.0f + sin(0.6f * (i + dt)) * 144.0f) * tt;
		particlePoint[i].y = center.y + (sin(1.8f * (i + dt)) * 68.0f + sin(0.5f * (i*i + dt)) * 132.0f) * tt;
	}
}

static void moveBumpOffset(int t)
{
	const int tt = t >> 5;
	int y;
	for (y = 0; y < HMAP_HEIGHT; ++y) {
		const int yp = (y + tt) & (HMAP_HEIGHT - 1);

		memcpy(&bumpOffsetFinal[y * HMAP_WIDTH], &bumpOffset[yp * HMAP_WIDTH], HMAP_WIDTH * sizeof(*bumpOffsetFinal));
	}
}

static void draw(void)
{
	const int t = time_msec - startingTime;

	memset(lightmap, 0, FB_WIDTH * FB_HEIGHT * sizeof(*lightmap));

	animateLights();
	renderLights();

	/* animateParticles();
	renderParticles(); */

	moveBumpOffset(t);

	renderBump((unsigned short*)fb_pixels);

	swap_buffers(0);
}
