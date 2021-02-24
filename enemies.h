struct type {
	const char *spath;
	int w;
	int h;
	int sprite_len;
	int hp;
};

struct unit {
	struct type *t;
	int x;
	int y;
	int hp;
	Entity *s;
};

struct type rat = {"rat.png", 32, 32, 3, 1};

struct unit enemies[] = {
	{&rat, 20, 40},
	{&rat, 200, 40},
	{&rat, 20, 400},
	{&rat, 500, 40},
	{&rat, 300, 430},
};
