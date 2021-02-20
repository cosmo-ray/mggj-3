#include <yirl/all.h>

int ww;
int wh;

int old_tl;

struct {
	int x;
	int y;
	int w;
	int h;
} pc = {530, 400, 32, 50};

Entity *rw_c;
Entity *rw_uc;

void repose_cam(Entity *rw)
{
	int x, y;
	Entity *wid_pix = yeGet(rw, "wid-pix");

	ww = ywRectW(wid_pix);
	wh = ywRectH(wid_pix);

	x = pc.x - ww / 2 + pc.w / 2;
	y = pc.y - wh / 2 - pc.h / 2;
	ywPosSetInts(yeGet(rw_c, "cam"), x, y);
	ywPosSetInts(yeGet(rw_uc, "cam"), x, y);
}

void *redwall_action(int nb, void **args)
{
	Entity *rw = args[0];
	Entity *evs = args[1];
	static int lr = 0, ud = 0;

	yeveDirFromDirGrp(evs, yeGet(rw, "u_grp"), yeGet(rw, "d_grp"),
			  yeGet(rw, "l_grp"), yeGet(rw, "r_grp"),
			  &ud, &lr);

	pc.x += 5 * lr;
	pc.y += 5 * ud;
	printf("%d %d\n", lr, ud);
	repose_cam(rw);
	ywPosPrint(yeGet(rw_c, "cam"));
	return (void *)ACTION;
}

void* redwall_destroy(int nb, void **args)
{
	ywSetTurnLengthOverwrite(old_tl);
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
		rw.background = "rgba: 127 127 127 255";
		rw["cnt-type"] = "stack";
		rw.action = redwall_action;
		rw.destroy = redwall_destroy;
		rw.u_grp = up_grp;
		rw.d_grp = down_grp;
		rw.l_grp = left_grp;
		rw.r_grp = right_grp;
	}

	rw_c = ywCreateCanvasEnt(yeGet(rw, "entries"), NULL);
	rw_uc = ywCreateCanvasEnt(yeGet(rw, "entries"), NULL);
	ywPosCreateInts(0, 0, rw_c, "cam");
	ywPosCreateInts(0, 0, rw_uc, "cam");
	yePrint(rw);
	void *ret = ywidNewWidget(rw, "container");

	void *rr = yesCall(ygGet("tiled.fileToCanvas"),
			   "./pere-lachaise.json", rw_c, rw_uc);

	printf("rr: %p - %d - %d\n", rr, ww, wh);
	ww = ywRectW(yeGet(rw, "wid-pix"));
	wh = ywRectH(yeGet(rw, "wid-pix"));

	old_tl = ywGetTurnLengthOverwrite();
	ywSetTurnLengthOverwrite(-1);
	return ret;
}

void *init_redwall(int nb, void **args)
{
	Entity *mod = args[0];
	Entity *init = yeCreateArray(NULL, NULL);

	YEntityBlock {
		mod.main = [];
		mod.main["<type>"] = "redwall";

		init.name = "redwall";
		init.callback = redwall_init;

	}

	ywidAddSubType(init);
	return mod;
}
