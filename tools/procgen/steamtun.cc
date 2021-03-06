//#define BASE_STR
//#define BASE_LEFTJ
#define BASE_RIGHTJ
//#define RIGHT_STR1
//#define RIGHT_STR2
//#define LEFT_STR1
//#define LEFT_STR2
//#define LEFT_STR3
//#define LEFT_STR4

//#define LEFT_J1
//#define LEFT_J2
//#define LEFT_J3
//#define LEFT_J4

#define STD_RAD		0.05
#define FAT_RAD		0.08
#define CON_RAD(r)	((r) * 1.4)
#define CON_WIDTH	0.04

static Object *gen_base_straight();
static Object *gen_base_left_junction();
static Object *gen_base_right_junction();
static Object *gen_pipeworks();
static Object *gen_pipe(float x, float y, float rad);
static Object *gen_pipe_inwall(float x, float y, float rad);
static Object *gen_pipe_s(float x, float y0, float y1, float rad);
static Object *gen_pipe_corner(float x, float y, float z, float rad);

static unsigned char *gen_texture(int x, int y);

static Mat4 xform;

#define YSTEP	(CON_RAD(STD_RAD) * 1.8)
#define Y0		(CON_RAD(FAT_RAD) + 0.01)
#define Y1		(Y0 + YSTEP)
#define Y2		(Y1 + YSTEP)
#define Y3		(Y2 + YSTEP)
#define Y4		(Y3 + YSTEP)
#define Y5		(Y4 + YSTEP)
#define Y6		(Y5 + YSTEP)
static const float yslots[] = {
	Y0, Y1, Y2, Y3, Y4, Y5, Y6
};

#define add_object(o) \
	do { \
		Object *tmp = o; \
		if(tmp) { \
			if(head) { \
				tail->next = tmp; \
				tail = tmp; \
			} else { \
				head = tail = tmp; \
			} \
			while(tail->next) tail = tail->next; \
		} \
	} while(0)

extern "C" Object *generate()
{
	Object *head = 0, *tail = 0;

#ifdef BASE_STR
	add_object(gen_base_straight());
#endif
#ifdef BASE_LEFTJ
	add_object(gen_base_left_junction());
#endif
#ifdef BASE_RIGHTJ
	add_object(gen_base_right_junction());
#endif
	add_object(gen_pipeworks());

	unsigned char *tex = gen_texture(256, 256);
	tail = head;
	while(tail) {
		tail->texture.pixels = tex;
		tail->texture.width = tail->texture.height = 256;
		tail->texture.fmt = PFMT_RGB;
		tail = tail->next;
	}

	return head;
}

static Object *gen_base_straight()
{
	Object *owalls = new Object;
	owalls->mesh = new Mesh;
	gen_box(owalls->mesh, -1, -2, -1, 1, 2);

	xform.rotation_x(deg_to_rad(90));
	owalls->mesh->apply_xform(xform);

	owalls->mesh->remove_faces(16, 23);

	xform.translation(0, 0.5, 0);
	owalls->mesh->apply_xform(xform);

	owalls->mesh->texcoord_gen_box();

	return owalls;
}

static Object *gen_base_left_junction()
{
	Mesh tmp;

	Object *obj = new Object;
	obj->mesh = new Mesh;

	gen_box(obj->mesh, -1, -2, -1, 1, 3);

	xform.rotation_x(deg_to_rad(90));
	xform.translate(0, 0.5, 0);
	obj->mesh->apply_xform(xform);

	obj->mesh->remove_faces(24, 35);
	obj->mesh->remove_faces(6, 11);

	for(int i=0; i<2; i++) {
		float sign = i == 0 ? 1.0f : -1.0f;
		gen_plane(&tmp, 0.5, 1, 1, 1);
		xform.rotation_y(deg_to_rad(90));
		xform.translate(-0.5, 0.5, sign * 0.75);
		tmp.apply_xform(xform);
		obj->mesh->append(tmp);
	}

	gen_box(&tmp, -1, -0.5, -1, 1, 1);
	xform.rotation_z(deg_to_rad(90));
	xform.translate(-0.75, 0.5, 0);
	tmp.apply_xform(xform);

	tmp.remove_faces(8, 11);

	obj->mesh->append(tmp);

	obj->mesh->texcoord_gen_box();

	return obj;
}

static Object *gen_base_right_junction()
{
	Object *obj = gen_base_left_junction();
	xform.rotation_y(deg_to_rad(180));
	obj->mesh->apply_xform(xform);

	obj->mesh->texcoord_gen_box();
	return obj;
}

static Object *gen_pipeworks()
{
	Object *head = 0, *tail = 0;

	// --- right straight pipes ---
	float start_y = CON_RAD(FAT_RAD) + 0.01;

#if defined(RIGHT_STR1) || defined(RIGHT_STR2)
	add_object(gen_pipe(0.5 - FAT_RAD, start_y, FAT_RAD));
	start_y += (CON_RAD(FAT_RAD) + CON_RAD(STD_RAD)) * 0.9;

	for(int i=0; i<3; i++) {
		float x = 0.5 - CON_RAD(STD_RAD);
		float y = start_y + i * (CON_RAD(STD_RAD) * 1.8);
#ifdef RIGHT_STR2
		if(i == 1)
			add_object(gen_pipe_inwall(x, y, STD_RAD));
		else
#endif
			add_object(gen_pipe(x, y, STD_RAD));
	}
#endif

	// --- left straight pipes ---
#if defined(LEFT_STR1) || defined(LEFT_STR2) || defined(LEFT_STR3) || defined(LEFT_STR4)
	//add_object(gen_pipe(-0.5 + FAT_RAD, start_y, FAT_RAD));
	add_object(gen_pipe(-0.5 + CON_RAD(STD_RAD), 0.8, STD_RAD));
#endif
#ifdef LEFT_STR1
	add_object(gen_pipe(-0.5 + CON_RAD(STD_RAD), 0.68, STD_RAD));
#endif
#ifdef LEFT_STR2
	add_object(gen_pipe(-0.5 + CON_RAD(STD_RAD), 0.3, STD_RAD));
#endif
#ifdef LEFT_STR3
	add_object(gen_pipe_s(-0.5 + CON_RAD(STD_RAD), 0.3, 0.67, STD_RAD));
#endif
#ifdef LEFT_STR4
	add_object(gen_pipe_s(-0.5 + CON_RAD(STD_RAD), 0.68, 0.3, STD_RAD));
#endif

	// --- left junction pipes ---
#if defined(LEFT_J1) || defined(LEFT_J2) || defined(LEFT_J3) || defined(LEFT_J4)
	add_object(gen_pipe_corner(-0.5, 0.8, 0.5, STD_RAD));
	add_object(gen_pipe_corner(-0.5, 0.8, -0.5, STD_RAD));
#endif
#ifdef LEFT_J1
	add_object(gen_pipe_corner(-0.5, 0.68, 0.5, STD_RAD));
	add_object(gen_pipe_corner(-0.5, 0.68, -0.5, STD_RAD));
#endif
#ifdef LEFT_J2
	add_object(gen_pipe_corner(-0.5, 0.3, 0.5, STD_RAD));
	add_object(gen_pipe_corner(-0.5, 0.3, -0.5, STD_RAD));
#endif
#ifdef LEFT_J3
	add_object(gen_pipe_corner(-0.5, 0.3, 0.5, STD_RAD));
	add_object(gen_pipe_corner(-0.5, 0.68, -0.5, STD_RAD));
#endif
#ifdef LEFT_J4
	add_object(gen_pipe_corner(-0.5, 0.68, 0.5, STD_RAD));
	add_object(gen_pipe_corner(-0.5, 0.3, -0.5, STD_RAD));
#endif

	// --- right junction pipes ---
	return head;
}

static void gen_conn(Mesh *mesh, float rad)
{
	gen_cylinder(mesh, CON_RAD(rad), CON_WIDTH, 7, 1, 1);

	xform.scaling(1, 0.05, 1);
	mesh->texcoord_apply_xform(xform);
}

static Object *gen_pipe(float x, float y, float rad)
{
	Object *opipe = new Object;
	opipe->mesh = new Mesh;


	for(int i=0; i<2; i++) {
		Mesh tmp;

		float pipelen = 1 - CON_WIDTH * 2;
		gen_cylinder(&tmp, rad, pipelen, 6, 1);
		xform.translation(0, i - pipelen / 2 - CON_WIDTH, 0);
		tmp.apply_xform(xform);
		opipe->mesh->append(tmp);

		gen_conn(&tmp, rad);
		xform.translation(0, 1 - i - CON_WIDTH / 2, 0);
		tmp.apply_xform(xform);
		opipe->mesh->append(tmp);

		gen_conn(&tmp, rad);
		xform.translation(0, 1 - i - CON_WIDTH * 1.5 - pipelen, 0);
		tmp.apply_xform(xform);
		opipe->mesh->append(tmp);
	}

	xform.rotation_x(deg_to_rad(90));
	opipe->mesh->apply_xform(xform);
	opipe->xform.translation(x, y, 0);

	return opipe;
}

static Object *gen_pipe_inwall(float x, float y, float rad)
{
	Object *opipe = new Object;
	opipe->mesh = new Mesh;

	const float pipelen = 0.2;
	float zoffs = 1.0 - pipelen - CON_WIDTH;

	for(int i=0; i<2; i++) {
		Mesh tmp;
		float sign = i == 0 ? 1.0f : -1.0f;

		gen_torus(&tmp, rad * 2.0, rad, 4, 7, 0.25);
		xform = Mat4::identity;
		if(i > 0) xform.rotate_y(deg_to_rad(90));
		xform.translate(rad * 2.0, 0, sign * zoffs);
		tmp.apply_xform(xform);
		opipe->mesh->append(tmp);

		gen_cylinder(&tmp, rad, pipelen, 7, 1);
		xform.rotation_x(deg_to_rad(90));
		xform.rotate_z(deg_to_rad(90));
		xform.translate(0, 0, sign * (zoffs + pipelen / 2.0));
		tmp.apply_xform(xform);
		opipe->mesh->append(tmp);

		gen_conn(&tmp, rad);
		xform.rotation_x(deg_to_rad(90));
		xform.translate(0, 0, sign * (1 - CON_WIDTH / 2.0));
		tmp.apply_xform(xform);
		opipe->mesh->append(tmp);
	}

	opipe->xform.translation(x, y, 0);

	return opipe;
}

static Object *gen_pipe_s(float x, float y0, float y1, float rad)
{
	Mesh tmp;
	float dist = fabs(y0 - y1);
	float pipelen = 1.0f - rad * 2.0 - CON_WIDTH / 2.0;

	Object *obj = new Object;
	obj->mesh = new Mesh;

	for(int i=0; i<2; i++) {
		float zsign = i == 0 ? 1.0f : -1.0f;
		float ysign = zsign * (y0 > y1 ? 1.0f : -1.0f);

		gen_torus(&tmp, rad * 2.0, rad, 4, 6, 0.25);
		xform.rotation_z(deg_to_rad(90) * ysign);
		if(i != 0) xform.rotate_x(deg_to_rad(y0 < y1 ? -90 : 90));
		xform.translate(0, ysign * (-dist / 2.0 + rad * 2.0), zsign * rad * 2.0);
		tmp.apply_xform(xform);
		obj->mesh->append(tmp);

		gen_cylinder(&tmp, rad, pipelen, 6, 1);
		xform.rotation_x(deg_to_rad(90));
		xform.translate(0, ysign * -dist / 2.0, zsign * (pipelen / 2.0 + rad * 2.0));
		tmp.apply_xform(xform);
		obj->mesh->append(tmp);

		gen_conn(&tmp, rad);
		xform.rotation_x(deg_to_rad(90));
		xform.translate(0, ysign * -dist / 2.0, zsign * (1 - CON_WIDTH / 2.0));
		tmp.apply_xform(xform);
		obj->mesh->append(tmp);

		gen_conn(&tmp, rad);
		xform.rotation_x(deg_to_rad(90));
		xform.translate(0, ysign * -dist / 2.0, zsign * (rad * 2.0 + CON_WIDTH / 2.0));
		tmp.apply_xform(xform);
		obj->mesh->append(tmp);
	}

	gen_cylinder(&tmp, rad, dist - rad * 4.0, 6, 1);
	obj->mesh->append(tmp);

	obj->xform.translation(x, (y0 + y1) / 2.0, 0);
	return obj;
}

static Object *gen_pipe_corner(float x, float y, float z, float rad)
{
	Mesh tmp;
	Object *obj = new Object;
	obj->mesh = new Mesh;

	float xoffs = CON_RAD(rad);
	float pipelen = 0.5 - CON_WIDTH - (rad * 2.0 - xoffs + CON_WIDTH);

	float sign = z >= 0.0f ? 1.0f : -1.0f;

	gen_torus(&tmp, rad * 2.0, rad, 4, 6, 0.25);
	xform.rotation_y(deg_to_rad(sign >= 0.0f ? -90 : 180));
	xform.translate(xoffs - rad * 2.0, 0, sign * (0.5 + rad * 2.0 - xoffs));
	tmp.apply_xform(xform);
	obj->mesh->append(tmp);

	for(int i=0; i<2; i++) {
		Mesh pipe;
		gen_cylinder(&tmp, rad, pipelen, 6, 1);
		xform.rotation_x(deg_to_rad(90));
		xform.rotate_z(deg_to_rad(90));
		xform.translate(xoffs, 0, sign * (1 - pipelen / 2 - CON_WIDTH));
		tmp.apply_xform(xform);
		pipe.append(tmp);

		gen_conn(&tmp, rad);
		xform.rotation_x(deg_to_rad(90));
		xform.translate(xoffs, 0, sign * (1 - CON_WIDTH / 2));
		tmp.apply_xform(xform);
		pipe.append(tmp);

		gen_conn(&tmp, rad);
		xform.rotation_x(deg_to_rad(90));
		xform.translate(xoffs, 0, sign * (0.5 + rad * 2.0 - xoffs + CON_WIDTH / 2.0));
		tmp.apply_xform(xform);
		pipe.append(tmp);

		if(i > 0) {
			xform = Mat4::identity;
			xform.translate(-xoffs, 0, sign * -0.5);
			xform.rotate_y(deg_to_rad(sign * -90));
			xform.translate(xoffs, 0, sign * 0.5);
			xform.translate(-xoffs, 0, sign * -xoffs);
			pipe.apply_xform(xform);
		}

		obj->mesh->append(pipe);
	}

	obj->xform.translation(x, y, 0);
	return obj;
}

static unsigned char *gen_texture(int x, int y)
{
	unsigned char *pixels = new unsigned char[x * y * 3];
	unsigned char *ptr = pixels;

	for(int i=0; i<y; i++) {
		for(int j=0; j<x; j++) {
			*ptr++ = i ^ j;
			*ptr++ = (i ^ j) << 1;
			*ptr++ = (i ^ j) << 2;
		}
	}
	return pixels;
}
