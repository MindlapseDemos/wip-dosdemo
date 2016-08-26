#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "screen.h"
#include "demo.h"

struct screen *tunnel_screen(void);

#define NUM_SCR	1
static struct screen *scr[NUM_SCR];

static struct screen *cur, *prev, *next;
static long trans_start, trans_dur;

int scr_init(void)
{
	int i;

	if(!(scr[0] = tunnel_screen())) {
		return -1;
	}

	for(i=0; i<NUM_SCR; i++) {
		if(scr[i]->init() == -1) {
			return -1;
		}
	}
	return 0;
}

void scr_shutdown(void)
{
	int i;
	for(i=0; i<NUM_SCR; i++) {
		scr[i]->shutdown();
	}
}

void scr_update(void)
{
	if(prev) {	/* we're in the middle of a transition */
		long interval = time_msec - trans_start;
		if(interval >= trans_dur) {
			next->start(trans_dur);
			prev = 0;
			cur = next;
			next = 0;
		}
	}
}

void scr_draw(void)
{
	if(cur) cur->draw();
}

struct screen *scr_lookup(const char *name)
{
	int i;
	for(i=0; i<NUM_SCR; i++) {
		if(strcmp(scr[i]->name, name) == 0) {
			return scr[i];
		}
	}
	return 0;
}

int scr_change(struct screen *s, long trans_time)
{
	if(!s) return -1;
	if(s == cur) return 0;

	if(trans_time) {
		trans_dur = trans_time / 2;	/* half for each part transition out then in */
		trans_start = time_msec;
	} else {
		trans_dur = 0;
	}

	if(cur) {
		cur->stop(trans_dur);

		prev = cur;
		next = s;
	} else {
		s->start(trans_dur);

		cur = s;
		prev = 0;
	}
	return 0;
}
