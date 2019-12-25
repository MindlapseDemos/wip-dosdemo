#include <stdio.h>
#include <stdlib.h>
#include <GL/glut.h>
#include "cmesh.h"
#include "cgmath/cgmath.h"

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

struct cmesh *scn;
struct cmesh *mesh_gout, *mesh_gin, *mesh_suz;
cgm_vec3 ganchor[4];

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

	glutMainLoop();
	return 0;
}


int init(void)
{
	static const char *meshnames[] = {"suzanne", "gimbal_outer", "gimbal_inner"};
	static struct cmesh **meshes[] = {&mesh_suz, &mesh_gout, &mesh_gin};
	static const float amb[] = {0.05, 0.05, 0.08, 1};
	int i;

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

	return 0;
}

void cleanup(void)
{
	cmesh_free(mesh_suz);
	cmesh_free(mesh_gout);
	cmesh_free(mesh_gin);
	cmesh_free(scn);
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
	int i;

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

	cmesh_draw(scn);

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
}

void motion(int x, int y)
{
	int dx = x - prev_mx;
	int dy = y - prev_my;
	prev_mx = x;
	prev_my = y;

	if(!(dx | dy)) return;

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

void sball_motion(int x, int y, int z)
{
}

void sball_rotate(int rx, int ry, int rz)
{
}

void sball_button(int bn, int st)
{
}