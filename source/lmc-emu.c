#define mem m[arg]

static short pc, a;
static short m[100];

int lmc(char filename[]);
static int run();
static void input(), output();
static inline bool validate();

int lmc(char filename[]) {
	FILE *code = fopen(filename, "rb");
	if (!code) {
		puts("failed to open the file");
		return 1;
	}
	fread(m, 2, 100, code);
	fclose(code);
	
	if (run()) {
		printf("error at box %d. a = %d\n", pc - 1, a);
		return 1;
	}
	puts("done");
	
	return 0;
}

static int run() {
	short ins, arg;
	
	pc = 0;
	a = 0;
	while (pc < 100) {
		ins = m[pc++];
		switch (ins) {
			case 000:
				return 0;
			case 901:
				input();
				break;
			case 902:
				output();
				break;
			default:
				arg = ins % 100;
				ins /= 100;
				switch (ins) {
					case 1:
						a += mem; break;
					case 2:
						a -= mem; break;
					case 3:
						mem = a; break;
					case 5:
						a = mem; break;
					case 6:
						pc = arg; break;
					case 7:
						if (!a) { pc = arg; } break;
					case 8:
						if (a) { pc = arg; } break;
					default:
						return 1;
				}
		}
		if (!validate())
			return 1;
	}
	return  0;
}

static void input() {
	do {
		printf("> ");
		scanf("%hd", &a);
	} while(!validate());
}

static void output() {
	printf("  %03hd\n", a);
}

static inline bool validate() {
	return a >= 0 && a < 1000;
}
