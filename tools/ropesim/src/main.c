#include <stdio.h>
#include <stdlib.h>
#include <GL/glut.h>
#include "cmesh.h"
#include "cgmath/cgmath.h"
#include "ropesim.h"

int init(void);
void cleanup(void);
void display(void);
void idle(void);
void reshape(int x, int y);
void keyb(unsigned char key, int x, int y);
void mouse(int bn, int st, int x, int y);
void motion(int x, int y);
void sball_motion(int x, int y, int z);
void sball_rotate(int rx, int ry, int rz);
void sball_button(int bn, int st);

float cam_theta, cam_phi, cam_dist = 10;
int prev_mx, prev_my;
int bnstate[8];
int modkeys;

long start_msec;

struct cmesh *scn;
struct cmesh *mesh_gout, *mesh_gin, *mesh_suz;

cgm_vec3 gmove;
/*cgm_quat grot = {0, 0, 0, 1};*/
float grot_theta, grot_phi;
float ginner_xform[16], gouter_xform[16];
cgm_vec3 ganchor[4], manchor[4];

cgm_vec3 dbgvec[4];

struct rsim_world rsim;

int main(int argc, char **argv)
{
	glutInit(&argc, argv);
	glutInitWindowSize(1280, 800);
	glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
	glutCreateWindow("ropesim");

	glutDisplayFunc(display);
	glutIdleFunc(idle);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyb);
	glutMouseFunc(mouse);
	glutMotionFunc(motion);
	glutSpaceballMotionFunc(sball_motion);
	glutSpaceballRotateFunc(sball_rotate);
	glutSpaceballButtonFunc(sball_button);

	if(init() == -1) {
		return 1;
	}
	atexit(cleanup);

	start_msec = glutGet(GLUT_ELAPSED_TIME);
	glutMainLoop();
	return 0;
}

#define ROPE_MASSES			10
#define ROPE_SPRINGS		(ROPE_MASSES - 1)
#define ROPE_LEN			0.8f
#define ROPE_MASSES_MASS	0.01f
#define ROPE_K				180.0f

int init(void)
{
	static const char *meshnames[] = {"suzanne", "gimbal_outer", "gimbal_inner"};
	static struct cmesh **meshes[] = {&mesh_suz, &mesh_gout, &mesh_gin};
	static const float amb[] = {0.05, 0.05, 0.08, 1};
	int i, j;
	struct rsim_rope *rope;

	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_LIGHT1);
	glEnable(GL_LIGHT2);

	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, amb);

	if(!(scn = cmesh_alloc()) || cmesh_load(scn, "gimbal.obj") == -1) {
		fprintf(stderr, "failed to load scene file\n");
		return -1;
	}

	for(i=0; i<sizeof meshes / sizeof *meshes; i++) {
		int idx;
		if((idx = cmesh_find_submesh(scn, meshnames[i])) == -1) {
			fprintf(stderr, "failed to locate required submesh (%s)\n", meshnames[i]);
			return -1;
		}
		if(!(*meshes[i] = cmesh_alloc()) || cmesh_clone_submesh(*meshes[i], scn, idx) == -1) {
			fprintf(stderr, "failed to clone submesh\n");
			return -1;
		}
		cmesh_remove_submesh(scn, idx);
	}

	rsim_init(&rsim);
	rsim.damping = 0.3;

	if(!(rope = rsim_alloc_rope(ROPE_MASSES * 4))) {
		fprintf(stderr, "failed to allocate rope\n");
		return -1;
	}
	rsim_add_rope(&rsim, rope);

	/* anchor points on the inner gimbal */
	for(i=0; i<4; i++) {
		ganchor[i].x = (float)(((i & 1) << 1) - 1) * 1.5f;
		ganchor[i].y = (float)((i & 2) - 1) * 1.5f;
		ganchor[i].z = 0;

		manchor[i] = ganchor[i];
		cgm_vscale(manchor + i, 0.32);



		manchor[i].y += 0.15;

		for(j=0; j<ROPE_MASSES; j++) {
			int midx = i * ROPE_MASSES + j;
			struct rsim_mass *mass = rope->masses + midx;

			float t = (float)j / (float)(ROPE_MASSES - 1.0f);
			cgm_vlerp(&mass->p, ganchor + i, manchor + i, t);
			mass->m = ROPE_MASSES_MASS;

			if(j == 0) {
				rsim_freeze_rope_mass(rope, rope->masses + i * ROPE_MASSES);	/* freeze first mass */
			} else {
				rsim_set_rope_spring(rope, midx, midx - 1, ROPE_K, RSIM_RLEN_DEFAULT);
			}
		}
	}

	return 0;
}

void cleanup(void)
{
	cmesh_free(mesh_suz);
	cmesh_free(mesh_gout);
	cmesh_free(mesh_gin);
	cmesh_free(scn);

	rsim_destroy(&rsim);
}

void update(long tmsec, float dt)
{
	int i;
	cgm_vec3 apt0, apt1;
	float theta, phi, brot;
	struct rsim_rope *rope;

	/*
	cgm_mrotation_quat(ginner_xform, &grot);
	cgm_mtranslate(ginner_xform, gmove.x, gmove.y, gmove.z);
	*/

	theta = cgm_deg_to_rad(grot_theta);
	phi = cgm_deg_to_rad(grot_phi);

	cgm_mrotation_euler(ginner_xform, phi, theta, 0, CGM_EULER_XYZ);
	cgm_mrotation_euler(gouter_xform, phi, 0, 0, CGM_EULER_XYZ);

	rope = rsim.ropes;
	for(i=0; i<4; i++) {
		apt0 = ganchor[i];
		cgm_vmul_m4v3(&apt0, ginner_xform);

		dbgvec[i] = apt0;
		rope->masses[i * ROPE_MASSES].p = apt0;
	}

	rsim_step(&rsim, dt);
}

void display(void)
{
	static const float lpos[][4] = {
		{-100, 100, 200, 1},
		{100, 80, 50, 1},
		{20, -50, -150, 1}
	};
	static const float lcol[][4] = {
		{0.9, 0.9, 0.9, 1},
		{0.5, 0.3, 0.2, 1},
		{0.2, 0.3, 0.2, 1}
	};
	int i, j, count;
	long tmsec = glutGet(GLUT_ELAPSED_TIME) - start_msec;
	static long prev_tmsec;
	struct rsim_rope *rope;

	update(tmsec, (float)(tmsec - prev_tmsec) / 1000.0f);
	prev_tmsec = tmsec;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0, 0, -cam_dist);
	glRotatef(cam_phi, 1, 0, 0);
	glRotatef(cam_theta, 0, 1, 0);

	for(i=0; i<3; i++) {
		glLightfv(GL_LIGHT0 + i, GL_POSITION, lpos[i]);
		glLightfv(GL_LIGHT0 + i, GL_DIFFUSE, lcol[i]);
	}

	count = cmesh_submesh_count(scn);
	for(i=0; i<count; i++) {
		cmesh_draw_submesh(scn, i);
	}

	glPushMatrix();
	glMultMatrixf(gouter_xform);
	cmesh_draw(mesh_gout);
	glPopMatrix();

	glPushMatrix();
	glMultMatrixf(ginner_xform);
	cmesh_draw(mesh_gin);
	glPopMatrix();

	/*cmesh_draw(mesh_suz);*/

	glPointSize(7);
	glBegin(GL_POINTS);
	for(i=0; i<4; i++) {
		glVertex3f(dbgvec[i].x, dbgvec[i].y, dbgvec[i].z);
	}
	glEnd();

	glPushAttrib(GL_ENABLE_BIT);
	glDisable(GL_LIGHTING);
	glLineWidth(2);
	glPointSize(5);

	rope = rsim.ropes;
	while(rope) {
		glBegin(GL_LINES);
		glColor3f(0.2, 1, 0.2);
		for(i=0; i<rope->num_masses; i++) {
			for(j=i+1; j<rope->num_masses; j++) {
				if(rsim_have_spring(rope, i, j)) {
					glVertex3f(rope->masses[i].p.x, rope->masses[i].p.y, rope->masses[i].p.z);
					glVertex3f(rope->masses[j].p.x, rope->masses[j].p.y, rope->masses[j].p.z);
				}
			}
		}
		glEnd();

		glBegin(GL_POINTS);
		glColor3f(1, 0.2, 0.2);
		for(i=0; i<rope->num_masses; i++) {
			glVertex3f(rope->masses[i].p.x, rope->masses[i].p.y, rope->masses[i].p.z);
		}
		glEnd();
		rope = rope->next;
	}
	glPopAttrib();

	glutSwapBuffers();
}
void idle(void)
{
	glutPostRedisplay();
}

void reshape(int x, int y)
{
	glViewport(0, 0, x, y);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(50.0, (float)x / (float)y, 0.5, 500.0);
}

void keyb(unsigned char key, int x, int y)
{
	switch(key) {
	case 27:
		exit(0);
	}
}

void mouse(int bn, int st, int x, int y)
{
	prev_mx = x;
	prev_my = y;
	bnstate[bn - GLUT_LEFT_BUTTON] = st == GLUT_DOWN;
	modkeys = glutGetModifiers();
}

void motion(int x, int y)
{
	int dx = x - prev_mx;
	int dy = y - prev_my;
	prev_mx = x;
	prev_my = y;

	if(!(dx | dy)) return;

	if(modkeys) {
		if(bnstate[0]) {
			grot_theta += dx * 0.5;
			grot_phi += dy * 0.5;
		}
	} else {
		if(bnstate[0]) {
			cam_theta += dx * 0.5;
			cam_phi += dy * 0.5;
			if(cam_phi < -90) cam_phi = -90;
			if(cam_phi > 90) cam_phi = 90;
		}

		if(bnstate[2]) {
			cam_dist += dy * 0.1;
			if(cam_dist < 0.0f) cam_dist = 0.0f;
		}
	}
}

void sball_motion(int x, int y, int z)
{
	gmove.x += x * 0.001f;
	gmove.y += y * 0.001f;
	gmove.z -= z * 0.001f;
}

void sball_rotate(int x, int y, int z)
{
	/*
	float axis_len, s;
	axis_len = (float)sqrt(x * x + y * y + z * z);
	s = axis_len == 0.0f ? 1.0f : 1.0f / axis_len;
	cgm_qrotate(&grot, axis_len * 0.001f, -x * s, -y * s, z * s);
	*/

	grot_theta += y * 0.03f;
	grot_phi += x * 0.03f;
}

void sball_button(int bn, int st)
{
	if(st == GLUT_DOWN) {
		/*cgm_qcons(&grot, 0, 0, 0, 1);*/
		grot_theta = grot_phi = 0.0f;
		cgm_vcons(&gmove, 0, 0, 0);
	}
}
