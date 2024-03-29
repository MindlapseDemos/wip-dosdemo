#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <imago2.h>
#include "image.h"
#include "treestor.h"
#include "util.h"
#include "gfxutil.h"

/* TODO support alpha masks in raw image dumps */

static void calc_pow2(struct image *img)
{
	int mask;
	if(img->scanlen <= 0 || img->height <= 0) {
		return;
	}

	img->xmask = img->scanlen - 1;
	img->ymask = img->height - 1;

	img->xshift = 0;
	mask = img->xmask;
	while(mask) {
		img->xshift++;
		mask >>= 1;
	}
}

int load_image(struct image *img, const char *fname)
{
	int i, pixcount;
	FILE *fp;
	char sig[8];
	uint16_t width, height;
	unsigned char *pptr;
	struct img_pixmap pixmap;

	memset(img, 0, sizeof *img);

	if(!(fp = fopen(fname, "rb"))) {
		fprintf(stderr, "load_image: failed to open file: %s: %s\n", fname, strerror(errno));
		return -1;
	}
	if(fread(sig, 1, 8, fp) < 8) {
		fprintf(stderr, "unexpected EOF while reading: %s\n", fname);
		fclose(fp);
		return -1;
	}

	if(memcmp(sig, "IDUMP565", 8) != 0) {
		goto not565;
	}

	/* it's a 565 dump, read it and return */
	if(!fread(&width, 2, 1, fp) || !fread(&height, 2, 1, fp)) {
		fprintf(stderr, "unexpected EOF while reading: %s\n", fname);
		fclose(fp);
		return -1;
	}
#ifdef BUILD_BIGENDIAN
	width = BSWAP16(width);
	height = BSWAP16(height);
#endif

	if(!(img->pixels = malloc(width * height * 2))) {
		fprintf(stderr, "failed to allocate %dx%d pixel buffer for %s\n", width, height, fname);
		fclose(fp);
		return -1;
	}
	if(fread(img->pixels, 2, width * height, fp) < width * height) {
		fprintf(stderr, "unexpected EOF while reading: %s\n", fname);
		free(img->pixels);
		img->pixels = 0;
		fclose(fp);
		return -1;
	}

#ifdef BUILD_BIGENDIAN
	{
		int i, npix = width * height;
		for(i=0; i<npix; i++) {
			uint16_t p = img->pixels[i];
			img->pixels[i] = BSWAP16(p);
		}
	}
#endif

	fclose(fp);
	img->width = width;
	img->height = height;
	img->scanlen = width;
	calc_pow2(img);
	return 0;
not565:
	fclose(fp);

	if(memcmp(sig, "animtex", 7) == 0) {
		/* it's an animated texture. read metadata, and recurse with the new name*/
		struct ts_node *root, *node;
		const char *imgfname;
		int fps, num_frames = 0;

		if(!(root = ts_load(fname))) {
			fprintf(stderr, "failed to load animation %s\n", fname);
			return -1;
		}
		if(!(imgfname = ts_lookup_str(root, "animtex.image", 0))) {
			fprintf(stderr, "animtex %s missing `image` attribute\n", fname);
			ts_free_tree(root);
			return -1;
		}
		if(strcmp(imgfname, fname) == 0) {
			fprintf(stderr, "animtex %s is silly...\n", fname);
			ts_free_tree(root);
			return -1;
		}

		if(load_image(img, imgfname) == -1) {
			ts_free_tree(root);
			return -1;
		}

		fps = ts_lookup_int(root, "animtex.framerate", 25);
		img->frame_interval = 1.0f / (float)fps;

		/* count frames */
		node = root->child_list;
		while(node) {
			if(strcmp(node->name, "frame") == 0) {
				num_frames++;
			}
			node = node->next;
		}

		if(!(img->uoffs = malloc(2 * num_frames * sizeof *img->uoffs))) {
			fprintf(stderr, "animtex %s: failed to allocate uvoffset array for %d frames\n", fname, num_frames);
			free(img->pixels);
			ts_free_tree(root);
			return -1;
		}
		img->voffs = img->uoffs + num_frames;

		num_frames = 0;
		node = root->child_list;
		while(node) {
			if(strcmp(node->name, "frame") == 0) {
				float *v = ts_get_attr_vec(node, "uvoffset", 0);
				if(v) {
					img->uoffs[num_frames] = v[0];
					img->voffs[num_frames++] = v[1];
				} else {
					fprintf(stderr, "animtex %s: ignoring frame without uvoffset\n", fname);
				}
			}
			node = node->next;
		}

		img->num_frames = num_frames;

		ts_free_tree(root);
		return 0;
	}

	/* just a regular image */
	img_init(&pixmap);
	if(img_load(&pixmap, fname) == -1) {
		fprintf(stderr, "failed to load image: %s\n", fname);
		return -1;
	}
	img->width = pixmap.width;
	img->height = pixmap.height;
	img->scanlen = img->width;

	pixcount = img->width * img->height;

	/* for images with an alpha channel, make an alpha image from the alpha
	 * channel first, before converting to 565
	 */
	if(img_has_alpha(&pixmap)) {
		/* In most cases, like this image having been loaded from a PNG or a TGA
		 * file, the following conversion is a no-op. Only if we were doing
		 * something weird like loading a 16bit per color channel PNG, or a
		 * floating point image format, will this have to do work.
		 */
		if(img_convert(&pixmap, IMG_FMT_RGBA32) == -1) {
			fprintf(stderr, "failed to convert %s to RGBA32 to grab alpha mask\n", fname);
			goto noalpha;
		}
		if(!(img->alpha = malloc(pixcount))) {
			fprintf(stderr, "failed to allocate %dx%d alpha mask for: %s\n",
					img->width, img->height, fname);
			goto noalpha;
		}

		pptr = (unsigned char*)pixmap.pixels + 3;
		for(i=0; i<pixcount; i++) {
			img->alpha[i] = *pptr;
			pptr += 4;
		}
	}
noalpha:
	if(img_convert(&pixmap, IMG_FMT_RGB565) == -1) {
		fprintf(stderr, "failed to convert %s to RGB565\n", fname);
		free(img->alpha);
		return -1;
	}

	img->pixels = pixmap.pixels;
	calc_pow2(img);

	if(img->alpha) {
		/* premultiply alpha */
		for(i=0; i<pixcount; i++) {
			int r = UNPACK_R16(img->pixels[i]);
			int g = UNPACK_G16(img->pixels[i]);
			int b = UNPACK_B16(img->pixels[i]);
			r = (r * img->alpha[i]) >> 8;
			g = (g * img->alpha[i]) >> 8;
			b = (b * img->alpha[i]) >> 8;
			img->pixels[i] = PACK_RGB16(r, g, b);
		}
	}
	return 0;
}

int dump_image(struct image *img, const char *fname)
{
	FILE *fp;
	uint16_t width, height;

#ifdef BUILD_BIGENDIAN
	int i, npix = img->width * img->height;
	width = BSWAP16(img->scanlen);
	height = BSWAP16(img->height);
#else
	width = img->scanlen;
	height = img->height;
#endif

	if(!(fp = fopen(fname, "wb"))) {
		fprintf(stderr, "dump_image: failed to open %s: %s\n", fname, strerror(errno));
		return -1;
	}
	fwrite("IDUMP565", 1, 8, fp);
	fwrite(&width, 2, 1, fp);
	fwrite(&height, 2, 1, fp);

#ifdef BUILD_BIGENDIAN
	for(i=0; i<npix; i++) {
		uint16_t p = BSWAP16(img->pixels[i]);
		fwrite(&p, 2, 1, fp);
	}
#else
	fwrite(img->pixels, 2, img->scanlen * img->height, fp);
#endif
	fclose(fp);
	return 0;
}

void destroy_image(struct image *img)
{
	if(img) {
		img_free_pixels(img->pixels);
		img->pixels = 0;
		free(img->alpha);
		img->alpha = 0;
	}
}

int load_cubemap(struct image *cube, const char *fname_fmt)
{
	int i;
	char dirstr[3] = {0};
	char fname[256];

	for(i=0; i<6; i++) {
		dirstr[0] = i & 1 ? 'n' : 'p';
		dirstr[1] = i < 2 ? 'x' : (i < 4 ? 'y' : 'z');
		sprintf(fname, fname_fmt, dirstr);
		if(load_image(cube + i, fname) == -1) {
			while(--i >= 0) {
				destroy_image(cube + i);
			}
			return -1;
		}
	}
	return 0;
}

void destroy_cubemap(struct image *cube)
{
	int i;

	if(!cube) return;

	for(i=0; i<6; i++) {
		destroy_image(cube + i);
	}
}

void init_image(struct image *img, int x, int y, uint16_t *pixels, int scanlen)
{
	memset(img, 0, sizeof *img);

	img->width = x;
	img->height = y;
	img->scanlen = scanlen > 0 ? scanlen : x;
	img->pixels = pixels;

	calc_pow2(img);
}
