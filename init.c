#include <yirl/all.h>

int ww;
int wh;

int old_tl;

void *ylpcsCreateHandler(void *character, void *canvas,
			 void *father, void *name);

#include "enemies.h"

struct {
	int x;
	int y;
	int w;
	int h;
	Entity *s;
	Entity *bullets;
} pc = {530, 460, 24, 24, NULL};

Entity *rw_c;
Entity *rw_uc;
Entity *enemies;

static void repose_enemies(void)
{
	YE_FOREACH(enemies, enemy_ent) {
		struct unit *enemy = yeGetData(enemy_ent);
		yeAutoFree Entity *pos = ywPosCreate(enemy->x, enemy->y,
						     NULL, NULL);

		yesCall(ygGet("sprite-man.handlerSetPos"), enemy->s, pos);
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
	yesCall(ygGet("sprite-man.handlerSetPos"), pc.s, pos);
	yesCall(ygGet("sprite-man.handlerRefresh"), pc.s);

	repose_enemies();
}

static void img_down(Entity *arg)
{
	yeSetAt(yeGet(pc.s, "sp"), "src-pos", 0);
}

static void img_up(Entity *arg)
{
	yeSetAt(yeGet(pc.s, "sp"), "src-pos", 24);
}

static void img_right(Entity *arg)
{
	yeSetAt(yeGet(pc.s, "sp"), "src-pos", 48);
}

static void img_left(Entity *arg)
{
	yeSetAt(yeGet(pc.s, "sp"), "src-pos", 72);
}

static void (*callbacks[4])(Entity *) = {img_up, img_down, img_right, img_left};

void create_bullet(Entity *mouse_pos)
{
	Entity *b = yeCreateArray(pc.bullets, NULL);
	yeAutoFree Entity *cp_pos = ywPosCreate(pc.x, pc.y, NULL, NULL);

	ywPosSub(cp_pos, yeGet(rw_c, "cam"));
	Entity *bc = ywCanvasNewRectangle(rw_c, pc.x, pc.y, 5,  5,
					  "rgba: 255 100 20 200");
	yeAutoFree Entity *seg = ywSegmentFromPos(cp_pos, mouse_pos, NULL, NULL);
	int dis = ywPosDistance(cp_pos, mouse_pos );
	Entity *dir = yeCreateArray(b, "dir");

	yeCreateFloat((1.0 * ywSizeW(seg) / dis), dir, "x");
	yeCreateFloat((1.0 * ywSizeH(seg) / dis), dir, "y");
	yePushBack(b, bc, NULL);
	yeCreateFloat(420, b, "life");
	yeCreateInt(6, b, "px_per_ms");
}

void *redwall_action(int nb, void **args)
{
	Entity *rw = args[0];
	Entity *evs = args[1];
	static int lr = 0, ud = 0;
	static double mv_acc;
	double mv_pix = 2 * ywidGetTurnTimer() / (double)10000;

	mv_acc += mv_pix - floor(mv_pix);
	if (mv_acc > 1) {
		mv_pix += floor(mv_acc);
		mv_acc -= floor(mv_acc);
	}
	/* printf("ywidGetTurnTimer: %f %f %f\n", mv_pix, floor(mv_pix), mv_acc); */

	yeveDirFromDirGrp(evs, yeGet(rw, "u_grp"), yeGet(rw, "d_grp"),
			  yeGet(rw, "l_grp"), yeGet(rw, "r_grp"),
			  &ud, &lr, callbacks, NULL);

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

	int btn = 0;
	if (yevMouseDown(evs, &btn)) {
		create_bullet(yevMousePos(evs));
	}

	Entity *pc_rect = ywRectReCreateInts(pc.x + 6, pc.y, 10,
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

	if (lr || ud)
		yesCall(ygGet("sprite-man.handlerAdvance"), pc.s);

	/* move bulets */
	YE_FOREACH(pc.bullets, b) {
		Entity *dir = yeGet(b, 0);
		Entity *obj = yeGet(b, 1);
		Entity *life = yeGet(b, 2);
		yeAutoFree Entity *cols = ywCanvasNewCollisionsArray(rw_c, obj);

		YE_FOREACH(cols, c) {
			if (yeGetIntAt(c, "Collision")) {
				goto next;
			}
		}
		if (yeGetFloat(life) < 0) {
		next:
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
	repose_cam(rw);
	return (void *)ACTION;
}

void* redwall_destroy(int nb, void **args)
{
	ywSetTurnLengthOverwrite(old_tl);
	yeDestroy(pc.s);
	yeDestroy(pc.bullets);
	return NULL;
}

void *redwall_init(int nb, void **args)
{
	Entity *rw = args[0];
	yeAutoFree Entity *down_grp, *up_grp, *left_grp, *right_grp;

	yesCall(ygGet("tiled.setAssetPath"), "./");

	down_grp = yevCreateGrp(0, Y_DOWN_KEY);
	up_grp = yevCreateGrp(0, Y_UP_KEY);
	left_grp = yevCreateGrp(0, Y_LEFT_KEY);
	right_grp = yevCreateGrp(0, Y_RIGHT_KEY);

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
	}

	enemies = yeCreateArray(rw, "enemies");

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

	Entity *objects = yeGet(rw_c, "objects");
	YE_FOREACH (objects, o) {
		yeAutoFree Entity *s = yeCreateArray(NULL, NULL);
		Entity *sprite = yeCreateArray(s, "sprite");
		const char *name = yeGetStringAt(o, "name");
		struct type *t = NULL;
		Entity *rect = yeGet(o, "rect");

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
		yeCreateInt(32, sprite, "src-pos");
		yeCreateString(enemy->t->spath, sprite, "path");

		enemy->s = yesCall(ygGet("sprite-man.createHandler"), s, rw_c);

		yeSetFreeAdDestroy(yeCreateData(enemy, enemies, NULL));
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
