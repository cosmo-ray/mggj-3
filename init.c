/*
**Copyright (C) 2021 Matthias Gatto
**
**This program is free software: you can redistribute it and/or modify
**it under the terms of the GNU Lesser General Public License as published by
**the Free Software Foundation, either version 3 of the License, or
**(at your option) any later version.
**
**This program is distributed in the hope that it will be useful,
**but WITHOUT ANY WARRANTY; without even the implied warranty of
**MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**GNU General Public License for more details.
**
**You should have received a copy of the GNU Lesser General Public License
**along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <yirl/all.h>

int ww;
int wh;

int old_tl;

void *ylpcsCreateHandler(void *character, void *canvas,
			 void *father, void *name);

static const int TARGET_WIN_W = 640;
static const int TARGET_WIN_H = 480;

double w_mult;
double h_mult;

struct type {
	const char *spath;
	int w;
	int h;
	int sprite_len;
	int hp;
	double invulnerable;
	int (*ai)(struct unit *);
	void (*init)(struct unit *);
};

struct unit {
	struct type *t;
	int x;
	int y;
	int hp;
	double reload;
	double x_speed;
	double y_speed;
	double save_x;
	double save_y;
	Entity *s;
	Entity *self;
	const char *name;
	int is_last;
};

static void init_ranger(struct unit *enemy)
{
	enemy->reload = 5;
}

static int mele_ai(struct unit *enemy);

static int bullet_ai(struct unit *enemy);
static int range_ai(struct unit *enemy);

static int macmahon_ai(struct unit *enemy);

struct type rat = {
	"enemy_placeholder_melee.png", 24, 24,
	6, 1,
	0,
	mele_ai, NULL
};

struct type shooter = {
	"enemy_placeholder.png",
	24, 24,
	6, 1,
	0,
	range_ai, init_ranger
};

struct type bullet = {
	"bullet.png",
	24, 24,
	1, -1,
	0,
	bullet_ai, NULL
};

struct type macmahon = {
	"boss_macmahon.png",
	48, 48,
	6, 25,
	0,
	macmahon_ai, NULL
};

static int time_acc;
static int score;
static double slow_power;
static struct unit *current_boss;

struct {
	struct type *t;
	char *name;
} enemies_types[] = {
	{ &rat, "rat" },
	{ &shooter, "shooter" }
};


#define PJ_DOWN 1
#define PJ_LEFT 2
#define PJ_RIGHT 4
#define PJ_UP 8

struct weapon {
	Entity *e;
	void (*fire)(struct weapon *);
	const char *name;
};

void fire(struct weapon *w);
void mele_attack(struct weapon *w);

struct weapon chassepot = {NULL, fire, "chassepot"};
struct weapon chassepot_bayonette = {NULL, mele_attack, "bayonette"};

struct {
	int x;
	int y;
	int w;
	int h;
	struct weapon *weapon;
	Entity *s;
	Entity *bullets;
	int8_t dir0_flag;
	int8_t dir_flag;
	int hp;
	double invulnerable;
	double knokback_x;
	double knokback_y;
	double power_reload;
} pc = {
	530, 460, 24, 24,
	&chassepot,
	0
};

static Entity *rw;
static Entity *rw_c;
static Entity *rw_uc;
static Entity *enemies;
static Entity *entries;
static Entity *rw_text;
static Entity *boss;
static Entity *boss_nfo;

static Entity *sprite_man_handlerSetPos;
static Entity *sprite_man_handlerAdvance;
static Entity *sprite_man_handlerRefresh;

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

static int wait_threshold;
static int turn_timer;

static void pow_refresh(Entity *h)
{
	yesCall(sprite_man_handlerRefresh, h);
	Entity *c = yeGet(h, "canvas");
	ywCanvasForceSizeXY(c, 240, 240);
	ygUpdateScreen();
	wait_threshold += 300000;
	usleep(300000);
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

	ywCanvasObjSetPos(rw_text, x + ww - 230, y + 10);

	ywCanvasObjSetPos(boss_nfo, x + 30, y + 25);

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
		xadd = 15;
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

static struct unit *create_enemy(struct type *t, Entity *pos)
{
	if (!t)
		return NULL;

	struct unit *enemy = malloc(sizeof *enemy);
	yeAutoFree Entity *s = yeCreateArray(NULL, NULL);
	Entity *sprite = yeCreateArray(s, "sprite");

	enemy->t = t;
	enemy->x = ywPosX(pos);
	enemy->y = ywPosY(pos);
	enemy->hp = t->hp;
	enemy->save_y = 0;
	enemy->save_x = 0;
	yeCreateString(enemy->t->spath, sprite, "path");
	yeCreateInt(enemy->t->sprite_len, sprite, "length");
	yeCreateInt(enemy->t->h, sprite, "size");
	yeCreateInt(0, sprite, "src-pos");
	yeCreateString(enemy->t->spath, sprite, "path");

	enemy->s = yesCall(ygGet("sprite-man.createHandler"), s, rw_c);
	enemy->self = yeCreateData(enemy, enemies, NULL);
	yeSetFreeAdDestroy(enemy->self);
	enemy->is_last = 0;
	if (t->init)
		t->init(enemy);
	return enemy;
}

static int bullet_ai(struct unit *enemy)
{
	double speed_x = enemy->x_speed;
	double speed_y = enemy->y_speed;

	enemy->reload -= turn_timer / (double)10000;
	if (slow_power > 0) {
		speed_x /= 3;
		speed_y /= 3;
	}
	enemy->x += speed_x * turn_timer / (double)10000;
	enemy->y += speed_y * turn_timer / (double)10000;

	if (enemy->reload <= 0) {
		return 2;
	}
	Entity *cobj = yeGet(enemy->s, "canvas");
	if (ywCanvasObjDistanceXY(cobj,
				  pc.x, pc.y) < 50 &&
	    ywCanvasObjectsCheckColisions(cobj, yeGet(pc.s, "canvas"))) {
		return 3;
	}

	yeAutoFree Entity *cols = ywCanvasNewCollisionsArray(rw_c, cobj);
	YE_FOREACH(cols, c) {
		if (yeGetIntAt(c, "Collision")) {
			return 2;
		}
	}

	return 0;
}

static int macmahon_ai(struct unit *enemy)
{
	Entity *cobj = yeGet(enemy->s, "canvas");

	if (ywCanvasObjDistanceXY(cobj, pc.x, pc.y) < 700) {
		if (enemy->reload <= 0) {
			yeAutoFree Entity *pos = ywPosCreate(enemy->x, enemy->y,
							     NULL, NULL);
			enemy->reload = 111;
			struct unit *b = create_enemy(&bullet, pos);
			yeAutoFree Entity *pc_pos = ywPosCreate(pc.x + 12,
								pc.y + 12,
								NULL, NULL);
			yeAutoFree Entity *seg = ywSegmentFromPos(pos, pc_pos,
								  NULL, NULL);
			int dis = ywPosDistance(pos, pc_pos);

			b->reload = 150;
			b->x_speed = 4.0 * ywSizeW(seg) / dis;
			b->y_speed = 4.0 * ywSizeH(seg) / dis;
		} else {
			enemy->reload -= turn_timer / (double)10000;
		}
	}
	return 0;
}

static int range_ai(struct unit *enemy)
{
	Entity *cobj = yeGet(enemy->s, "canvas");

	if (ywCanvasObjDistanceXY(cobj, pc.x, pc.y) < 500) {
		if (enemy->reload <= 0) {
			yeAutoFree Entity *pos = ywPosCreate(enemy->x, enemy->y,
							     NULL, NULL);
			enemy->reload = 555;
			struct unit *b = create_enemy(&bullet, pos);
			yeAutoFree Entity *pc_pos = ywPosCreate(pc.x + 12,
								pc.y + 12,
								NULL, NULL);
			yeAutoFree Entity *seg = ywSegmentFromPos(pos, pc_pos,
								  NULL, NULL);
			int dis = ywPosDistance(pos, pc_pos);

			b->reload = 150;
			b->x_speed = 4.0 * ywSizeW(seg) / dis;
			b->y_speed = 4.0 * ywSizeH(seg) / dis;
		} else {
			enemy->reload -= turn_timer / (double)10000;
		}
	}
	return 0;
}

static int mele_ai(struct unit *enemy)
{
	Entity *cobj = yeGet(enemy->s, "canvas");

	if (pc.invulnerable > 0)
		return 0;
	if (ywCanvasObjDistanceXY(cobj, pc.x, pc.y) < 300) {
		double x = 0, y = 0, s;
		double speed = slow_power > 0 ? 0.4 : 1.6;
		double advance = speed * turn_timer / (double)10000;


		if (ywCanvasObjDistanceXY(cobj,
					  pc.x, pc.y) < 30 &&
		    ywCanvasObjectsCheckColisions(cobj, yeGet(pc.s, "canvas"))) {
			return 1;
		}
		x = pc.x - enemy->x;
		y = pc.y - enemy->y;
		advance = y && x ? advance / 2 : advance;

		s = x;
		x = yuiAbs(x) > advance ? advance : yuiAbs(x);
		enemy->save_x += x - floor(x);
		if (enemy->save_x > 1) {
			x += floor(enemy->save_x);
			enemy->save_x -= floor(enemy->save_x);
		}
		x = s < 0 ? -x : x;

		s = y;
		y = yuiAbs(y) > advance ? advance : yuiAbs(y);
		enemy->save_y += y - floor(y);
		if (enemy->save_y > 1) {
			y += floor(enemy->save_y);
			enemy->save_y -= floor(enemy->save_y);
		}
		y = s < 0 ? -y : y;
		enemy->x += x;
		enemy->x_speed = x;
		enemy->y += y;
		enemy->y_speed = y;
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
				return 0;
			}
		}
		if (time_acc > 100000) {
			yesCall(sprite_man_handlerAdvance, enemy->s);
			yesCall(sprite_man_handlerRefresh, enemy->s);
		}
	}
	return 0;

}

static void pc_move(int x, int y)
{
	int ox = pc.x, oy = pc.y;
	pc.x += x;
	if (pc.x + pc.w > yeGetIntAt(rw_c, "tiled-wpix")) {
		pc.x = yeGetIntAt(rw_c, "tiled-wpix") - pc.w;
	} else if (pc.x < 0) {
		pc.x = 0;
	}
	pc.y += y;
	if (pc.y + pc.h > yeGetIntAt(rw_c, "tiled-hpix")) {
		pc.y = yeGetIntAt(rw_c, "tiled-wpix") - pc.h;
	} else if (pc.y < 0) {
		pc.y = 0;
	}

	yeAutoFree Entity *pc_rect = ywRectCreateInts(pc.x + 6, pc.y, 10,
						      pc.h, NULL, NULL);
	yeAutoFree Entity *col =
		ywCanvasNewCollisionsArrayWithRectangle(rw_c, pc_rect);

	YE_FOREACH(col, c) {
		if (!yeGetIntAt(c, "Collision"))
			continue;
		yeAutoFree Entity *pos = ywPosCreate(pc.x, pc.y,
						     NULL, NULL);

		yesCall(sprite_man_handlerSetPos, pc.s, pos);
		Entity *pc_c = yeGet(pc.s, "canvas");
		if (ywCanvasObjectsCheckColisions(pc_c, c)) {
			pc.x = ox;
			pc.y = oy;
			break;
		}
	}

}

void *redwall_action(int nb, void **args)
{
	Entity *evs = args[1];
	static int lr = 0, ud = 0;
	static double mv_acc;
	static int is_blocked;

	turn_timer = ywidGetTurnTimer() - wait_threshold;
	wait_threshold = 0;
	double mv_pix = 2 * turn_timer / (double)10000;
	yeAutoFree Entity *txt = yeCreateString("Life: ", NULL, NULL);
	yeStringAddInt(txt, pc.hp);
	yeStringAdd(txt, "\nweapon: ");
	yeStringAdd(txt, pc.weapon->name);
	yeStringAdd(txt, "\nscore: ");
	yeStringAddInt(txt, score);

	if (slow_power > 0) {
		slow_power -= turn_timer / (double)10000;
	}

	if (pc.power_reload > 0) {
		pc.power_reload -= turn_timer / (double)10000;
	}
	if (pc.power_reload < 0)
		pc.power_reload = 0;

	yeStringAdd(txt, "\nslow device ready: ");
	yeStringAddInt(txt, 100 - pc.power_reload / 10);


	if (pc.invulnerable > 0) {
		pc.invulnerable -= turn_timer / (double)10000;
		if (pc.invulnerable <= 0)
			yeSetAt(pc.s, "text_idx", 0);
		else if (pc.invulnerable > 50) {
			pc_move(pc.knokback_x, pc.knokback_y);
			/* skipp movement, but stil get input */
			yeveDirFromDirGrp(evs, yeGet(rw, "u_grp"), yeGet(rw, "d_grp"),
					  yeGet(rw, "l_grp"), yeGet(rw, "r_grp"),
					  &ud, &lr, callback, NULL);

			goto skipp_movement;
		}
	}
	ywCanvasStringSet(rw_text, txt);
	if (current_boss) {
		yeAutoFree Entity *boss_txt =
			yeCreateString(current_boss->name, NULL, NULL);
		yeStringAdd(boss_txt, " [");
		for (int i = 0; i < current_boss->t->hp; ++i) {
			if (i < current_boss->hp)
				yeStringAddCh(boss_txt, '*');
			else
				yeStringAddCh(boss_txt, '.');
		}
		yeStringAdd(boss_txt, "]");
		ywCanvasStringSet(boss_nfo, boss_txt);
	}
	time_acc += turn_timer;
	mv_acc += mv_pix - floor(mv_pix);
	if (mv_acc > 1) {
		mv_pix += floor(mv_acc);
		mv_acc -= floor(mv_acc);
	}
	/* printf("ywidGetTurnTimer: %f %f %f\n", mv_pix, floor(mv_pix), mv_acc); */

	yeveDirFromDirGrp(evs, yeGet(rw, "u_grp"), yeGet(rw, "d_grp"),
			  yeGet(rw, "l_grp"), yeGet(rw, "r_grp"),
			  &ud, &lr, callback, NULL);

	pc_move(mv_pix * lr, mv_pix * ud);

skipp_movement:;

	int fire = yevIsGrpDown(evs, yeGet(rw, "fire_grp"));

	if (fire) {
		pc.weapon->fire(pc.weapon);
	}

	int pow = yevIsGrpDown(evs, yeGet(rw, "power_grp"));

	if (pow && pc.power_reload == 0) {
		yeAutoFree Entity *pow_sprite = yeCreateArray(NULL, NULL);
		yeAutoFree Entity *pow_handler;
		YEntityBlock {
			pow_sprite.sprite = {};
			pow_sprite.sprite.path = "slow_bomb.png";
			pow_sprite.sprite.length = 6;
			pow_sprite.sprite.size = 24;
			pow_sprite.sprite["src-pos"] = 0;
		}
		pow_handler = yesCall(ygGet("sprite-man.createHandler"),
				      pow_sprite, rw_uc);

		yeAutoFree Entity *pos = ywPosCreate(pc.x - 110, pc.y - 110,
						     NULL, NULL);
		yesCall(sprite_man_handlerSetPos, pow_handler, pos);
		pow_refresh(pow_handler);
		for (int i = 0; i < 5; ++i) {
			yesCall(sprite_man_handlerAdvance, pow_handler);
			pow_refresh(pow_handler);
		}

		yesCall(ygGet("sprite-man.handlerNullify"), pow_handler);
		pc.power_reload = 1000;
		slow_power = 200;
		YE_FOREACH(enemies, e) {
			struct unit *enemy = yeGetData(e);

			if (enemy->t == &bullet) {
				ywCanvasRemoveObj(rw_c, yeGet(enemy->s, "canvas"));
				yeRemoveChild(enemies, e);
			}
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
			Entity *enemy = yeGet(c, "enemy");
			if (enemy &&
			    ywCanvasObjectsCheckColisions(c, obj)) {
				struct unit *e = yeGetData(enemy);

				if (e->hp < 0)
					continue;
				e->hp -= 1;
				if (e->hp > 0) {
					goto remove;
				}
				/* kill enemy */
				ywCanvasRemoveObj(rw_c, c);
				yeRemoveChild(enemies, enemy);
				++score;
				if (e->is_last) {
					Entity *win_fnc = ygGet(yeGetStringAt(rw, "win"));
					printf("you win !!\n");
					if (win_fnc) {
						yesCall(win_fnc, rw);
					} else {
						ygTerminate();
					}
					return (void *)ACTION;

				}
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
			turn_timer / (double)10000;
		double advence_y = px_per_ms * yeGetFloatAt(dir, 1) *
			turn_timer / (double)10000;

		ywCanvasMoveObjXY(yeGet(b, 1), advence_x, advence_y);
		yeSubFloat(life, yuiAbs(advence_y) +
			   yuiAbs(advence_x));
	}

	int i = 0;
	YE_FOREACH(boss, bb) {
		int score_req = yeGetIntAt(bb, 0);
		int been_invocked = yeGetIntAt(bb, 1);

		if (!been_invocked && score_req <= score) {
			struct unit *mh = create_enemy(&macmahon, yeGet(bb, 2));

			yeSetAt(bb, 1, 1);
			mh->is_last = 1;
			mh->name = yeGetKeyAt(boss, i);
			current_boss = mh;
		}
		i++;
	}

	YE_FOREACH(enemies, e) {
		struct unit *enemy = yeGetData(e);
		int ai_ret = enemy->t->ai(enemy);

		if (ai_ret & 1) {
			if (pc.invulnerable > 0)
				goto not_dead;

			pc.hp -= 1;
			if (pc.hp) {
				yeSetAt(pc.s, "text_idx", 1);
				pc.knokback_x = enemy->x_speed * 2;
				pc.knokback_y = enemy->y_speed * 2;
				pc.invulnerable = 75;
				goto not_dead;
			}

			/* remove up layer */
			ywCntPopLastEntry(rw);

			ywCanvasNewRectangle(rw_c, 0, 0, 2048, 2048, "rgba: 0 0 0 255");
			ywCanvasNewRectangle(rw_c, 0, 0, ww, wh, "rgba: 0 0 0 0");

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

			Entity *die_fnc = ygGet(yeGetStringAt(rw, "die"));
			if (die_fnc) {
				yesCall(die_fnc, rw);
			} else {
				ygTerminate();
			}
			return (void *)ACTION;
		}
	not_dead:

		if (ai_ret & 2) {
			ywCanvasRemoveObj(rw_c, yeGet(enemy->s, "canvas"));
			yeRemoveChild(enemies, e);
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
	yeDestroy(rw_text);
	rw_text = NULL;
	yeDestroy(boss_nfo);
	boss_nfo = NULL;
	yeDestroy(boss);
	boss = NULL;
	current_boss = NULL;
	return NULL;
}

void *redwall_init(int nb, void **args)
{
	rw = args[0];
	yeAutoFree Entity *down_grp, *up_grp, *left_grp, *right_grp;
	yeAutoFree Entity *fire_grp, *power_grp;

	score = 0;
	current_boss = NULL;
	slow_power = 0;
	yesCall(ygGet("tiled.setAssetPath"), "./");

	down_grp = yevCreateGrp(0, 's', Y_DOWN_KEY);
	up_grp = yevCreateGrp(0, 'w', Y_UP_KEY);
	left_grp = yevCreateGrp(0, 'a', Y_LEFT_KEY);
	right_grp = yevCreateGrp(0, 'd', Y_RIGHT_KEY);
	fire_grp = yevCreateGrp(0, ' ');
	power_grp = yevCreateGrp(0, 'x');

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
		rw.power_grp = power_grp;
	}

	enemies = yeCreateArray(rw, "enemies");
	entries = yeCreateArray(rw, "entries");
	boss = yeCreateArray(rw, "boss");

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

	printf("w size: %d %d\n", ww, wh);

	void *rr = yesCall(ygGet("tiled.fileToCanvas"),
			   "./pere-lachaise.json", rw_c, rw_uc, 1);

	yeAutoFree Entity *useless = yeCreateString("", NULL, NULL);
	rw_text = ywCanvasNewTextExt(rw_uc, 0, 0, useless,
		"rgba: 255 255 255 255");

	boss_nfo = ywCanvasNewTextExt(rw_uc, 0, 0, useless,
				      "rgba: 255 255 255 255");
	yeAutoFree Entity *pcs = yeCreateArray(NULL, NULL);

	YEntityBlock {
		pcs.sex = "female";
		pcs.sprite = {};
		pcs.sprite.paths = {};
		pcs.sprite.paths[0] = "mc_placeholder.png";
		pcs.sprite.paths[1] = "mc_hurt.png";
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
			yePushBack(entries, rect, name);
			continue;
		} else if (!strcmp(layer_name, "Boss")) {
			Entity *b = yeCreateArray(boss, name);

			yeCreateInt(yeGetIntAt(o, "score_require"),
				    b, "score_req");
			yeCreateInt(0, b, "been_invocked");
			yePushBack(b, rect, "rect");
		}

		if (!name)
			continue;
		for (int i = 0;
		     i < sizeof(enemies_types) / sizeof(enemies_types[0]); ++i) {
			if (!strcmp(enemies_types[i].name, name)) {
				t = enemies_types[i].t;
				break;
			}
		}

		create_enemy(t, rect);
	}
	yePrint(boss);

	Entity *in = yeGet(entries, yeGetStringAt(rw, "in"));
	if (in) {
		pc.y = ywRectY(in);
		pc.x = ywRectX(in);
	}
	pc.hp = 6;
	pc.invulnerable = 0;
	pc.dir_flag = PJ_DOWN;
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
