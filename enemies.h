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

struct {
	struct type *t;
	char *name;
} enemies_types[] = {
	{ &rat, "rat" }
};
