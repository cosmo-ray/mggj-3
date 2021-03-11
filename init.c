#include <yirl/all.h>

int ww;
int wh;

int old_tl;

void *ylpcsCreateHandler(void *character, void *canvas,
			 void *father, void *name);

#include "enemies.h"

#define PJ_DOWN 1
#define PJ_LEFT 2
#define PJ_RIGHT 4
#define PJ_UP 8

struct weapon {
	Entity *e;
	void (*fire)(struct weapon *);
};

void fire(struct weapon *w);
void mele_attack(struct weapon *w);

struct weapon chassepot = {NULL, fire};
struct weapon chassepot_bayonette = {NULL, mele_attack};

struct {
	int x;
	int y;
	int w;
	int h;
	Entity *s;
	struct weapon *weapon;
	Entity *bullets;
	int8_t dir0_flag;
	int8_t dir_flag;
} pc = {530, 460, 24, 24, NULL, &chassepot, NULL, 0, PJ_DOWN};

Entity *rw_c;
Entity *rw_uc;
Entity *enemies;
Entity *entries;

Entity *sprite_man_handlerSetPos;
Entity *sprite_man_handlerAdvance;
Entity *sprite_man_handlerRefresh;

static int sprit_flag_pos(void)
{
	int i = yeGetIntAt(yeGet(pc.s, "sp"), "src-pos");

	if (i == 0)
		return PJ_DOWN;
	else if (i == 24)
		return PJ_UP;
	else if (i == 48)
		return PJ_RIGHT;
	else if (i == 72)
		return PJ_LEFT;
	i = yeGetIntAt(pc.s, "x");
	if (i == 0)
		return PJ_LEFT;
	else if (i == 24)
		return PJ_DOWN;
	else if (i == 48)
		return PJ_RIGHT;
	return PJ_UP;
}

static void dead_refresh(void)
{
	yeAutoFree Entity *pos = ywPosCreate(pc.x, pc.y,
					     NULL, NULL);

	yesCall(sprite_man_handlerSetPos, pc.s, pos);
	yesCall(sprite_man_handlerRefresh, pc.s);
	Entity *c = yeGet(pc.s, "canvas");
	ywCanvasSwapObj(rw_c, c, ywCanvasLastObj(rw_c));
	ywCanvasForceSizeXY(c, 240, 240);
	ygUpdateScreen();
	usleep(300000);
}

static void repose_enemies(void)
{
	YE_FOREACH(enemies, enemy_ent) {
		struct unit *enemy = yeGetData(enemy_ent);
		yeAutoFree Entity *pos = ywPosCreate(enemy->x, enemy->y,
						     NULL, NULL);
		Entity *cobj = yeGet(enemy->s, "canvas");

		yesCall(sprite_man_handlerSetPos, enemy->s, pos);
		if (!yeGet(cobj, "enemy"))
			yePushBack(cobj, enemy_ent, "enemy");
	}
}

static void repose_cam(Entity *rw)
{
	int x, y;
	Entity *wid_pix = yeGet(rw, "wid-pix");

	ww = ywRectW(wid_pix);
	wh = ywRectH(wid_pix);

	x = pc.x - ww / 2;
	y = pc.y - wh / 2;

	ywPosSetInts(yeGet(rw_c, "cam"), x, y);
	ywPosSetInts(yeGet(rw_uc, "cam"), x, y);
	yeAutoFree Entity *pos = ywPosCreate(pc.x, pc.y,
					     NULL, NULL);

	Entity *e = pc.weapon->e;
	int gun_add_x = 0, gun_add_y = 4;
	switch(sprit_flag_pos()) {
	case PJ_DOWN:
		if (!pc.dir0_flag)
			yeSetAt(pc.s, "x", 24);
		yeSetAt(e, "x", 72);
		gun_add_x = -6;
		break;
	case PJ_LEFT:
		if (!pc.dir0_flag)
			yeSetAt(pc.s, "x", 0);
		yeSetAt(e, "x", 0);
		break;
	case PJ_UP:
		if (!pc.dir0_flag)
			yeSetAt(pc.s, "x", 72);
		yeSetAt(e, "x", 48);
		gun_add_x = 6;
		break;
	case PJ_RIGHT:
		if (!pc.dir0_flag)
			yeSetAt(pc.s, "x", 48);
		yeSetAt(e, "x", 24);
		break;
	}
	if (!pc.dir0_flag)
		yeSetAt(yeGet(pc.s, "sp"), "src-pos", 96);
	yesCall(sprite_man_handlerSetPos, pc.s, pos);
	yesCall(sprite_man_handlerRefresh, pc.s);
	ywPosAddXY(pos, gun_add_x, gun_add_y);
	yesCall(sprite_man_handlerSetPos, e, pos);
	yesCall(sprite_man_handlerRefresh, e);

	repose_enemies();
}

static void img_down(Entity *arg)
{
	pc.dir0_flag = (pc.dir0_flag | PJ_DOWN) & ~PJ_UP;
	yeSetAt(yeGet(pc.s, "sp"), "src-pos", 0);
}

static void img_up(Entity *arg)
{
	pc.dir0_flag = (pc.dir0_flag | PJ_UP) & ~PJ_DOWN;
	yeSetAt(yeGet(pc.s, "sp"), "src-pos", 24);
}

static void img_right(Entity *arg)
{
	pc.dir0_flag = (pc.dir0_flag | PJ_RIGHT) & ~PJ_LEFT;
	yeSetAt(yeGet(pc.s, "sp"), "src-pos", 48);
}

static void img_left(Entity *arg)
{
	pc.dir0_flag = (pc.dir0_flag | PJ_LEFT) & ~PJ_RIGHT;
	yeSetAt(yeGet(pc.s, "sp"), "src-pos", 72);
}


static void callback(Entity *a, int k, int is_up)
{
	switch (k) {
	case Y_UP_KEY:
		if (!is_up)
			img_up(a);
		else
			pc.dir0_flag &= ~PJ_UP;
		break;
	case Y_DOWN_KEY:
		if (!is_up)
			img_down(a);
		else
			pc.dir0_flag &= ~PJ_DOWN;
		break;
	case Y_LEFT_KEY:
		if (!is_up)
			img_left(a);
		else
			pc.dir0_flag &= ~PJ_LEFT;
		break;
	case Y_RIGHT_KEY:
		if (!is_up)
			img_right(a);
		else
			pc.dir0_flag &= ~PJ_RIGHT;
		break;
	}
	if ((pc.dir0_flag))
		pc.dir_flag = pc.dir0_flag;
	if (!(sprit_flag_pos() & pc.dir_flag)) {
		int f = pc.dir_flag;
		int pix_dst = 0;

		if (f & PJ_UP)
			pix_dst = 24;
		else if (f & PJ_LEFT)
			pix_dst = 72;
		else if (f & PJ_RIGHT)
			pix_dst = 48;
		yeSetAt(yeGet(pc.s, "sp"), "src-pos", pix_dst);
	}
	/* img_up, img_down, img_right, img_left */
}

void create_bullet(Entity *mouse_pos)
{
	Entity *b = yeCreateArray(pc.bullets, NULL);
	int xadd = 0, yadd = 11;

	switch (sprit_flag_pos()) {
	case PJ_UP:
		xadd = 20;
		break;
	case PJ_RIGHT:
		xadd = 15;
		break;
	}

	yeAutoFree Entity *cp_pos = ywPosCreate(pc.x, pc.y, NULL, NULL);

	ywPosSub(cp_pos, yeGet(rw_c, "cam"));
	Entity *bc = ywCanvasNewRectangle(rw_c, pc.x + xadd, pc.y + yadd, 5,  5,
					  "rgba: 255 100 20 200");
	yeAutoFree Entity *seg = ywSegmentFromPos(cp_pos, mouse_pos, NULL, NULL);
	int dis = ywPosDistance(cp_pos, mouse_pos);
	Entity *dir = yeCreateArray(b, "dir");

	yeCreateFloat((1.0 * ywSizeW(seg) / dis), dir, "x");
	yeCreateFloat((1.0 * ywSizeH(seg) / dis), dir, "y");
	yePushBack(b, bc, NULL);
	yeCreateFloat(420, b, "life");
	yeCreateInt(6, b, "px_per_ms");
}

void fire(struct weapon *w)
{
	int xadd = (pc.dir_flag & PJ_LEFT) ? -1 :
		((pc.dir_flag & PJ_RIGHT) ? 1 : 0);
	int yadd = (pc.dir_flag & PJ_DOWN) ? 1 :
		((pc.dir_flag & PJ_UP) ? -1 : 0);
	yeAutoFree Entity *p =
		ywPosCreate(pc.x + xadd, pc.y + yadd, NULL, NULL);
	ywPosSub(p, yeGet(rw_c, "cam"));
	yePrint(p);
	create_bullet(p);
}

void mele_attack(struct weapon *weapon)
{
	int x, y, w, h;
	int p = sprit_flag_pos();

	switch (p) {
	case PJ_DOWN:
	case PJ_UP:
		w = 10;
		h = 24;
		x = pc.x + 5;
		if (p == PJ_DOWN) {
			y = pc.y + 24;
		} else {
			y = pc.y - 24;
		}
		break;
	case PJ_LEFT:
	case PJ_RIGHT:
		w = 24;
		h = 10;
		y = pc.y + 5;
		if (p == PJ_LEFT)
			x = pc.x - 24;
		else
			x = pc.x + 24;
		break;
	}
	yeAutoFree Entity *r = ywRectCreateInts(x, y, w, h, NULL, NULL);
	yeAutoFree Entity *cols = ywCanvasNewCollisionsArrayWithRectangle(rw_c, r);


	YE_FOREACH(cols, c) {
		/* yePrint(c); */
		if (yeGet(c, "enemy")) {
			Entity *enemy = yeGet(c, "enemy");
			ywCanvasRemoveObj(rw_c, c);
			yeRemoveChild(enemies, enemy);
		}
	}

}

void *redwall_action(int nb, void **args)
{
	Entity *rw = args[0];
	Entity *evs = args[1];
	static int lr = 0, ud = 0;
	static double mv_acc;
	double mv_pix = 2 * ywidGetTurnTimer() / (double)10000;
	static int time_acc;

	time_acc += ywidGetTurnTimer();
	mv_acc += mv_pix - floor(mv_pix);
	if (mv_acc > 1) {
		mv_pix += floor(mv_acc);
		mv_acc -= floor(mv_acc);
	}
	/* printf("ywidGetTurnTimer: %f %f %f\n", mv_pix, floor(mv_pix), mv_acc); */

	yeveDirFromDirGrp(evs, yeGet(rw, "u_grp"), yeGet(rw, "d_grp"),
			  yeGet(rw, "l_grp"), yeGet(rw, "r_grp"),
			  &ud, &lr, callback, NULL);

	int ox = pc.x, oy = pc.y;
	pc.x += mv_pix * lr;
	if (pc.x + pc.w > yeGetIntAt(rw_c, "tiled-wpix")) {
		pc.x = yeGetIntAt(rw_c, "tiled-wpix") - pc.w;
	} else if (pc.x < 0) {
		pc.x = 0;
	}
	pc.y += mv_pix * ud;
	if (pc.y + pc.h > yeGetIntAt(rw_c, "tiled-hpix")) {
		pc.y = yeGetIntAt(rw_c, "tiled-wpix") - pc.h;
	} else if (pc.y < 0) {
		pc.y = 0;
	}

	int fire = yevIsGrpDown(evs, yeGet(rw, "fire_grp"));

	if (fire) {
		pc.weapon->fire(&pc.weapon);
	}

	yeAutoFree Entity *pc_rect = ywRectCreateInts(pc.x + 6, pc.y, 10,
						      pc.h, NULL, NULL);
	yeAutoFree Entity *col =
		ywCanvasNewCollisionsArrayWithRectangle(rw_c, pc_rect);

	YE_FOREACH(col, c) {
		if (yeGetIntAt(c, "Collision")) {
			pc.x = ox;
			pc.y = oy;
			break;
		}
	}

	if ((lr || ud) && time_acc > 100000 && pc.dir0_flag) {
		yesCall(sprite_man_handlerAdvance, pc.s);
	}

	/* move bulets */
	YE_FOREACH(pc.bullets, b) {
		Entity *dir = yeGet(b, 0);
		Entity *obj = yeGet(b, 1);
		Entity *life = yeGet(b, 2);
		yeAutoFree Entity *cols = ywCanvasNewCollisionsArray(rw_c, obj);

		YE_FOREACH(cols, c) {
			if (yeGetIntAt(c, "Collision")) {
				goto remove;
			}
			/* yePrint(c); */
			if (yeGet(c, "enemy")) {
				Entity *enemy = yeGet(c, "enemy");
				ywCanvasRemoveObj(rw_c, c);
				yeRemoveChild(enemies, enemy);
				goto remove;
			}
		}
		if (yeGetFloat(life) < 0) {
		remove:
			ywCanvasRemoveObj(rw_c, obj);
			yeRemoveChild(pc.bullets, b);
			continue;
		}
		double px_per_ms = yeGetIntAt(b, 3) / 2;
		double advence_x = px_per_ms * yeGetFloatAt(dir, 0) *
			ywidGetTurnTimer() / (double)10000;
		double advence_y = px_per_ms * yeGetFloatAt(dir, 1) *
			ywidGetTurnTimer() / (double)10000;

		ywCanvasMoveObjXY(yeGet(b, 1), advence_x, advence_y);
		yeSubFloat(life, yuiAbs(advence_y) +
			   yuiAbs(advence_x));
	}

	YE_FOREACH(enemies, e) {
		struct unit *enemy = yeGetData(e);
		Entity *cobj = yeGet(enemy->s, "canvas");

		if (ywCanvasObjDistanceXY( cobj,
			    pc.x, pc.y) < 300) {
			int x = 0, y = 0, s;
			int advance = 1 * ywidGetTurnTimer() / (double)10000;


			if (ywCanvasObjDistanceXY(cobj,
						  pc.x, pc.y) < 30) {
				ywCntPopLastEntry(rw);

				ywCanvasNewRectangle(rw_c, 0, 0, 2048, 2048, "rgba: 0 0 0 255");
				ywCanvasNewRectangle(rw_c, 0, 0, ww, wh, "rgba: 0 0 0 0");
				Entity *die_fnc = ygGet(yeGetStringAt(rw, "die"));

				lr = 0;
				ud = 0;
				mv_acc = 0;

				pc.x -= 110;
				pc.y -= 110;

				dead_refresh();
				yeSetAt(pc.s, "x", 0);
				yeSetAt(yeGet(pc.s, "sp"), "src-pos", 288);
				dead_refresh();
				yeSetAt(pc.s, "x", 24);
				yeSetAt(yeGet(pc.s, "sp"), "src-pos", 288);
				dead_refresh();
				yeSetAt(pc.s, "x", 48);
				yeSetAt(yeGet(pc.s, "sp"), "src-pos", 288);
				dead_refresh();

				if (die_fnc) {
					yesCall(die_fnc, rw);
				} else {
					ygTerminate();
				}
				return (void *)ACTION;
			}
			x = pc.x - enemy->x;
			s = x;
			x = yuiAbs(x) > advance ? advance : yuiAbs(x);
			x = s < 0 ? -x : x;

			y = pc.y - enemy->y;
			s = y;
			y = yuiAbs(y) > advance ? advance : yuiAbs(y);
			y = s < 0 ? -y : y;
			enemy->x += x;
			enemy->y += y;
			yeAutoFree Entity *rect =
				ywRectReCreateInts(enemy->x, enemy->y,
						   enemy->t->w,
						   enemy->t->h,
						   NULL, NULL);
			yeAutoFree Entity *col =
				ywCanvasNewCollisionsArrayWithRectangle(rw_c,
									rect);

			if (x < 0) {
				yeSetAt(yeGet(enemy->s, "sp"), "src-pos", 72);
			} else if (x > 0) {
				yeSetAt(yeGet(enemy->s, "sp"), "src-pos", 48);
			} else if (y < 0) {
				yeSetAt(yeGet(enemy->s, "sp"), "src-pos", 24);
			} else if (y > 0) {
				yeSetAt(yeGet(enemy->s, "sp"), "src-pos", 0);
			}
			YE_FOREACH(col, c) {
				if (yeGetIntAt(c, "Collision")) {
					enemy->x -= x;
					enemy->y -= y;
					goto continue_loop;
				}
			}
			if (time_acc > 100000) {
				yesCall(sprite_man_handlerAdvance, enemy->s);
				yesCall(sprite_man_handlerRefresh, enemy->s);
			}

		continue_loop:

		}
	}
	if (time_acc > 100000)
		time_acc = 0;
	repose_cam(rw);
	return (void *)ACTION;
}

void* redwall_destroy(int nb, void **args)
{
	ywSetTurnLengthOverwrite(old_tl);
	yeDestroy(pc.s);
	yeDestroy(pc.bullets);
	yeDestroy(chassepot.e);
	yeDestroy(enemies);
	enemies = NULL;
	yeDestroy(entries);
	entries = NULL;
	return NULL;
}

void *redwall_init(int nb, void **args)
{
	Entity *rw = args[0];
	yeAutoFree Entity *down_grp, *up_grp, *left_grp, *right_grp;
	yeAutoFree Entity *fire_grp;

	yesCall(ygGet("tiled.setAssetPath"), "./");

	down_grp = yevCreateGrp(0, 's', Y_DOWN_KEY);
	up_grp = yevCreateGrp(0, 'w', Y_UP_KEY);
	left_grp = yevCreateGrp(0, 'a', Y_LEFT_KEY);
	right_grp = yevCreateGrp(0, 'd', Y_RIGHT_KEY);
	fire_grp = yevCreateGrp(0, ' ');

	YEntityBlock {
		rw.entries = {};
		rw.background = "rgba: 0 0 0 255";
		rw["cnt-type"] = "stack";
		rw.action = redwall_action;
		rw.destroy = redwall_destroy;
		rw.u_grp = up_grp;
		rw.d_grp = down_grp;
		rw.l_grp = left_grp;
		rw.r_grp = right_grp;
		rw.fire_grp = fire_grp;
	}

	enemies = yeCreateArray(rw, "enemies");
	entries = yeCreateArray(rw, "entries");
	sprite_man_handlerSetPos = ygGet("sprite-man.handlerSetPos");
	sprite_man_handlerAdvance = ygGet("sprite-man.handlerAdvance");
	sprite_man_handlerRefresh = ygGet("sprite-man.handlerRefresh");

	rw_c = ywCreateCanvasEnt(yeGet(rw, "entries"), NULL);
	yeCreateInt(2, rw_c, "mergable");
	ywSizeCreate(2048, 2048, rw_c, "merge_surface_size");
	yeCreateArray(rw_c, "objects");
	rw_uc = ywCreateCanvasEnt(yeGet(rw, "entries"), NULL);
	ywPosCreateInts(0, 0, rw_c, "cam");
	ywPosCreateInts(0, 0, rw_uc, "cam");
	void *ret = ywidNewWidget(rw, "container");
	ww = ywRectW(yeGet(rw, "wid-pix"));
	wh = ywRectH(yeGet(rw, "wid-pix"));

	void *rr = yesCall(ygGet("tiled.fileToCanvas"),
			   "./pere-lachaise.json", rw_c, rw_uc, 1);

	yeAutoFree Entity *pcs = yeCreateArray(NULL, NULL);

	YEntityBlock {
		pcs.sex = "female";
		pcs.sprite = {};
		pcs.sprite.path = "mc_placeholder.png";
		pcs.sprite.length = 6;
		pcs.sprite.size = 24;
		pcs.sprite["src-pos"] = 0;
	}
	pc.s = yesCall(ygGet("sprite-man.createHandler"), pcs, rw_c);
	pc.bullets = yeCreateArray(NULL, NULL);

	yeAutoFree Entity *chassepot_e = yeCreateArray(NULL, NULL);

	YEntityBlock {
		chassepot_e.sprite = {};
		chassepot_e.sprite.path = "mc_placeholder.png";
		chassepot_e.sprite.length = 4;
		chassepot_e.sprite.size = 24;
		chassepot_e.sprite["src-pos"] = 240;
	}
	chassepot.e = yesCall(ygGet("sprite-man.createHandler"),
			      chassepot_e, rw_c);

	yeAutoFree Entity *chassepot_bayonette_e = yeCreateArray(NULL, NULL);

	YEntityBlock {
		chassepot_e.sprite = {};
		chassepot_e.sprite.path = "enemy_placeholder_melee.png";
		chassepot_e.sprite.length = 4;
		chassepot_e.sprite.size = 24;
		chassepot_e.sprite["src-pos"] = 240;
	}

	chassepot_bayonette.e = yesCall(ygGet("sprite-man.createHandler"),
			      chassepot_e, rw_c);

	pc.weapon = &chassepot;

	Entity *objects = yeGet(rw_c, "objects");
	YE_FOREACH (objects, o) {
		const char *layer_name = yeGetStringAt(o, "layer_name");
		struct type *t = NULL;
		Entity *rect = yeGet(o, "rect");
		const char *name = yeGetStringAt(o, "name");

		if (!strcmp(layer_name, "Entries")) {
			printf("push entry: %s\n", name);
			yePushBack(entries, rect, name);
			continue;
		}

		yeAutoFree Entity *s = yeCreateArray(NULL, NULL);
		Entity *sprite = yeCreateArray(s, "sprite");
		if (!name)
			continue;
		for (int i = 0;
		     i < sizeof(enemies_types) / sizeof(enemies_types[0]); ++i) {
			if (!strcmp(enemies_types[i].name, name)) {
				t = enemies_types[i].t;
				break;
			}
		}

		if (!t) {
			continue;
		}
		struct unit *enemy = malloc(sizeof *enemy);

		enemy->t = t;
		enemy->x = ywRectX(rect);
		enemy->y = ywRectY(rect);
		yeCreateString(enemy->t->spath, sprite, "path");
		yeCreateInt(enemy->t->sprite_len, sprite, "length");
		yeCreateInt(enemy->t->h, sprite, "size");
		yeCreateInt(0, sprite, "src-pos");
		yeCreateString(enemy->t->spath, sprite, "path");

		enemy->s = yesCall(ygGet("sprite-man.createHandler"), s, rw_c);

		yeSetFreeAdDestroy(yeCreateData(enemy, enemies, NULL));
	}

	Entity *in = yeGet(entries, yeGetStringAt(rw, "in"));
	if (in) {
		pc.y = ywRectY(in);
		pc.x = ywRectX(in);
	}
	old_tl = ywGetTurnLengthOverwrite();
	ywSetTurnLengthOverwrite(-1);
	return ret;
}

void *init_redwall(int nb, void **args)
{
	Entity *mod = args[0];
	Entity *init = yeCreateArray(NULL, NULL);

	YEntityBlock {
		init.name = "redwall";
		init.callback = redwall_init;

	}

	ywidAddSubType(init);
	return mod;
}
