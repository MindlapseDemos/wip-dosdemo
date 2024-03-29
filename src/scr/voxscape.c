/* Voxel landscape attempt */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>

#include "demo.h"
#include "screen.h"


#define FP_VIEWER 8
#define FP_BASE 12
#define FP_SCAPE 10
#define FP_REC 18

#define SIN_SHIFT 12
#define SIN_LENGTH (1 << SIN_SHIFT)
#define SIN_TO_COS (SIN_LENGTH / 4)

#define FOV 48
#define VIS_NEAR 16
#define VIS_FAR 224

#define PIXEL_SIZE 1
#define PIXEL_ABOVE (FB_WIDTH / PIXEL_SIZE)
#define VIS_VER_SKIP 1
#define PAL_SHADES 32

#define VIS_VER_STEPS ((VIS_FAR - VIS_NEAR) / VIS_VER_SKIP)
#define VIS_HOR_STEPS (FB_WIDTH / PIXEL_SIZE)

#define V_PLAYER_HEIGHT 176
#define V_HEIGHT_SCALER_SHIFT 7
#define V_HEIGHT_SCALER (1 << V_HEIGHT_SCALER_SHIFT)
#define HORIZON (3 * FB_HEIGHT / 4)

#define HMAP_WIDTH 512
#define HMAP_HEIGHT 512
#define HMAP_SIZE (HMAP_WIDTH * HMAP_HEIGHT)


typedef struct Point2D
{
	int x,y;
}Point2D;

typedef struct Vector3D
{
	int x,y,z;
}Vector3D;


static int *heightScaleTab = NULL;
static int *isin = NULL;
static Point2D *viewNearPosVec = NULL;
static Point2D *viewNearStepVec = NULL;

static unsigned char *hmap = NULL;
static unsigned char *cmap = NULL;

static Vector3D viewPos;
static Vector3D viewAngle;

static int prevTime;


static int init(void);
static void destroy(void);
static void start(long trans_time);
static void draw(void);

static struct screen scr = {
	"voxelscape",
	init,
	destroy,
	start,
	0,
	draw
};

struct screen *voxscape_screen(void)
{
	return &scr;
}

static void initSinTab(const int numSines, const int repeats, const int amplitude)
{
	int i;
	const int numSinesRepeat = numSines / repeats;
	float sMul = (2.0f * 3.1415926536f) / (float)(numSinesRepeat-1);

	if (!isin) isin = malloc(numSines * sizeof(int));

	for (i=0; i<numSines; ++i) {
		isin[i] = (int)(sin((float)i * sMul) * amplitude);
	}
}

static void createHeightScaleTab()
{
	int i;
	float z = 0;
	const float dz = 1.0f / (VIS_VER_STEPS - 1);

	if (!heightScaleTab) heightScaleTab = malloc(VIS_VER_STEPS * sizeof(int));

	for (i = 0; i < VIS_VER_STEPS; ++i) {
		const int stepZ = (int)(z * (1 << FP_SCAPE));
		heightScaleTab[i] = (1 << FP_REC) /  (VIS_NEAR + ((stepZ * VIS_FAR) >> FP_SCAPE));
		z += dz;
	}
}

static int readMapFile(const char *path, int count, void *dst)
{
	FILE *f = fopen(path, "rb");
	if(!f) {
		fprintf(stderr, "voxscape: failed to open %s: %s\n", path, strerror(errno));
		return -1;
	}

	fread(dst, 1, count, f);

	fclose(f);
	return 0;
}

static void initPalShades()
{
	int i,j;

	uint16_t *shadedPmap = (uint16_t*)&cmap[HMAP_SIZE + 512];

	for (j=1; j<PAL_SHADES; ++j) {
		uint16_t *origPmap = (uint16_t*)&cmap[HMAP_SIZE];
		const int shade = PAL_SHADES - j;
		for (i=0; i<256; ++i) {
			const uint16_t c = origPmap[i];
			const int r = (((c >> 11) & 31) * shade) / PAL_SHADES;
			const int g = (((c >> 5) & 63) * shade) / PAL_SHADES;
			const int b = ((c & 31) * shade) / PAL_SHADES;
			*shadedPmap++ = (r << 11) | (g << 5) | b;
		}
	}
}

static int initHeightmapAndColormap()
{
	const int cmapSize = HMAP_SIZE + 512 * PAL_SHADES;

	if (!hmap) hmap = malloc(HMAP_SIZE);
	if (!cmap) cmap = malloc(cmapSize);

	if(readMapFile("data/hmap1.bin", HMAP_SIZE, hmap) == -1) return -1;
	if(readMapFile("data/cmap1.bin", cmapSize, cmap) == -1) return -1;

	initPalShades();
	return 0;
}


static int init(void)
{
	memset(fb_pixels, 0, (FB_WIDTH * FB_HEIGHT * FB_BPP) / 8);

	initSinTab(SIN_LENGTH, 1, 65536);
	createHeightScaleTab();

	hmap = malloc(HMAP_SIZE);
	viewNearPosVec = malloc(VIS_HOR_STEPS * sizeof(Point2D));
	viewNearStepVec = malloc(VIS_HOR_STEPS * sizeof(Point2D));

	if(initHeightmapAndColormap() == -1) {
		return -1;
	}

	return 0;
}

static void destroy(void)
{
	free(isin);
	free(heightScaleTab);
	free(viewNearPosVec);
	free(viewNearStepVec);
	free(hmap);
	free(cmap);
}


static void setPoint2D(Point2D *pt, int x, int y)
{
	pt->x = x;
	pt->y = y;
}

static void updateRaySamplePosAndStep()
{
	int i;
	Point2D pl, pr, dHor;
	
	const int halfFov = (FOV / 2) << (SIN_SHIFT-8);
	const int yaw = viewAngle.y & (SIN_LENGTH-1);
	const int yawL = (yaw - halfFov) & (SIN_LENGTH-1);
	const int yawR = (yaw + halfFov) & (SIN_LENGTH-1);
	const int length = (1 << (FP_BASE + FP_SCAPE)) / isin[halfFov + SIN_TO_COS];

	Point2D *viewPosVec = viewNearPosVec;
	Point2D *viewStepVec = viewNearStepVec;

	setPoint2D(&pl, isin[(yawL+SIN_TO_COS)&(SIN_LENGTH-1)]*length, isin[yawL]*length);
	setPoint2D(&pr, isin[(yawR+SIN_TO_COS)&(SIN_LENGTH-1)]*length, isin[yawR]*length);
	setPoint2D(&dHor, (pr.x - pl.x) / (VIS_HOR_STEPS - 1), (pr.y - pl.y) / (VIS_HOR_STEPS - 1));

	for (i=0; i<VIS_HOR_STEPS; ++i) {
		setPoint2D(viewStepVec++, (VIS_VER_SKIP * pl.x) >> FP_BASE, (VIS_VER_SKIP * pl.y) >> FP_BASE);
		setPoint2D(viewPosVec++, (VIS_NEAR * pl.x) >> FP_BASE, (VIS_NEAR * pl.y) >> FP_BASE);

		pl.x += dHor.x;
		pl.y += dHor.y;
	}
}

static void renderScape()
{
	int i,j,l;

	const int playerHeight = viewPos.y >> FP_VIEWER;
	const int viewerOffset = (viewPos.z >> FP_VIEWER) * HMAP_WIDTH + (viewPos.x >> FP_VIEWER);

	uint16_t *dstBase = (uint16_t*)fb_pixels + (FB_HEIGHT-1) * FB_WIDTH;

	updateRaySamplePosAndStep();

	for (j=0; j<VIS_HOR_STEPS; ++j) {
		int yMax = 0;

		int vx = viewNearPosVec[j].x;
		int vy = viewNearPosVec[j].y;
		const int dvx = viewNearStepVec[j].x;
		const int dvy = viewNearStepVec[j].y;

		#if PIXEL_SIZE == 2
			uint32_t *dst = (uint32_t*)dstBase;
		#elif PIXEL_SIZE == 1
			uint16_t *dst = dstBase;
		#endif


		uint16_t *pmapPtr = (uint16_t*)&cmap[HMAP_SIZE];
		for (i=0; i<VIS_VER_STEPS; ++i) {
			const int sampleOffset = (vy >> FP_SCAPE) * HMAP_WIDTH + (vx >> FP_SCAPE);
			const int mapOffset = (viewerOffset + sampleOffset) & (HMAP_SIZE - 1);
			const int hm = hmap[mapOffset];
			int h = (((-playerHeight + hm) * heightScaleTab[i]) >> (FP_REC - V_HEIGHT_SCALER_SHIFT)) + HORIZON;
			if (h > FB_HEIGHT-1) h = FB_HEIGHT-1;

			if (yMax < h) {
				#if PIXEL_SIZE == 2
					const uint16_t c16 = pmapPtr[cmap[mapOffset]];
					const uint32_t cv = (c16<<16) | c16;
				#elif PIXEL_SIZE == 1
					const uint16_t cv = pmapPtr[cmap[mapOffset]];
				#endif

				int hCount = h-yMax;
				do {
					*dst = cv;
					dst -= PIXEL_ABOVE;
				}while(--hCount > 0);

				yMax = h;
				if (yMax == FB_HEIGHT - 1) break;
			}
			vx += dvx;
			vy += dvy;

			if (i > VIS_VER_STEPS - PAL_SHADES) pmapPtr += 256;
		}

		for (l=yMax; l<FB_HEIGHT; ++l) {
			*dst = 0;
			dst -= PIXEL_ABOVE;
		}

		dstBase += PIXEL_SIZE;
	}

}

static void setViewPos(int px, int py, int pz)
{
	viewPos.x = px << FP_VIEWER;
	viewPos.y = py << FP_VIEWER;
	viewPos.z = pz << FP_VIEWER;
}

static void setViewAngle(int rx, int ry, int rz)
{
	viewAngle.x = rx & (SIN_LENGTH-1);
	viewAngle.y = ry & (SIN_LENGTH-1);
	viewAngle.z = rz & (SIN_LENGTH-1);
}

static void start(long trans_time)
{
	setViewPos(3*HMAP_WIDTH/4, V_PLAYER_HEIGHT, HMAP_HEIGHT/6);
	setViewAngle(0,128,0);

	prevTime = time_msec;
}

static void move()
{
	const int dt = time_msec - prevTime;
 
	const int speedX = (dt << FP_VIEWER) >> 8;
	const int speedZ = (dt << FP_VIEWER) >> 8;

	const int velX = (speedX * isin[(viewAngle.y + SIN_TO_COS) & (SIN_LENGTH-1)]) >> FP_BASE;
	const int velZ = (speedZ * isin[viewAngle.y]) >> FP_BASE;

	viewPos.x += velX;
	viewPos.z += velZ;

	if(mouse_bmask & MOUSE_BN_LEFT) {
		viewAngle.y = (4*mouse_x) & (SIN_LENGTH-1);
	}
	if(mouse_bmask & MOUSE_BN_RIGHT) {
		viewPos.y = mouse_y << FP_VIEWER;
	}

	prevTime = time_msec;
}

static void draw(void)
{
	move();

	renderScape();

	swap_buffers(0);
}
