#define ASM_WORD_LEN 32
#define ASM_LABELS_N 100
#define ASM_LABEL_INSTANCES_N 100
#define	ASM_MNEMONICS_N 10
#define ASM_DIRECTIVES_N 2

static const struct {
	char  name[4];
	short code;
	bool arg;
} mnemonics[ASM_MNEMONICS_N] = {
	{"end", 000, false},
	{"add", 100, true},
	{"sub", 200, true},
	{"st",  300, true},
	{"ld",  500, true},
	{"br",  600, true},
	{"bz",  700, true},
	{"bp",  800, true},
	{"in",  901, false},
	{"out", 902, false}
};
static const char directives[ASM_DIRECTIVES_N][5] = {
	"code", "data"
};
static const char reserved_chars[] = ";:!?";
static const char spaces[] = " \t\n";

static int program_line;
static short buffer[100];
static unsigned char program_len;

static char word[ASM_WORD_LEN];
static unsigned char word_len;
static enum {
	PREV_NOTHING, PREV_COMMAND, PREV_OPERAND, PREV_LABEL
} prev_word;
static enum {
	MODE_CODE, MODE_DATA
} mode;

static struct label_type {
	char name[ASM_WORD_LEN];
	short place;
} labels[ASM_LABELS_N], label_instances[ASM_LABEL_INSTANCES_N];;
static short labels_n;
static short label_inst_n;

int assemble(char source_filename[], char code_filename[], enum format_type format);
int parse(FILE *source);
static void report();
static void error(const char msg[]);
static bool match_char(char c, const char source[]);
static bool is_digit(char c);
static signed char match_word(char word[], const char *list, const char n, const char m);
static short parse_number(char word[]);
static inline bool parse_word(char postfix);
static bool parse_code_word(char postfix);
static bool parse_data_word(char postfix);
static inline void put_opcode(short place, unsigned char instruction);
static inline void put_operand(short place, unsigned char n);
static inline void put_number(short place, short n);
static bool add_label(char name[]);
static bool put_label(char name[]);
static inline bool insert_labels();

int assemble(char source_filename[], char code_filename[], enum format_type format) {
	FILE *source, *code;
	
	source = fopen(source_filename, format == FOR_BIN ? "rb" : "rt");
	if (!source) {
		puts("failed to open the source code file");
		return 1;
	}
	if (parse(source))
		return 1;
	report();
	code = fopen(code_filename, "w");
	if (!code) {
		puts("failed to save the code");
		return 1;
	}
	if (format == FOR_BIN)
		fwrite(buffer, 2, program_len, code);
	else
		for(short i = 0; i < program_len; ++i)
			fprintf(code, "%03hd\n", buffer[i]);
	fclose(source);
	fclose(code);
	return 0;
}

int parse(FILE *source) {
	char read_char;
	bool is_comment;
	
	program_line = 1;
	program_len = 0;
	word_len = 0;
	is_comment = false;
	prev_word = PREV_NOTHING;
	mode = MODE_CODE;
	while (true) {
		read_char = fgetc(source);
		if (!is_comment) {
			if (!(match_char(read_char, reserved_chars) ||
				match_char(read_char, spaces))
			) {
				if (word_len > ASM_WORD_LEN) {
					error("long word");
					return 1;
				}
				word[word_len++] = read_char;
			} else if (word_len > 0) {
				word[word_len++] = '\0';
				if (parse_word(read_char))
					return 1;
				word_len = 0;
			} else {
				if (read_char == '?')
					put_number(program_len++, 000);
			}
		}
		if (read_char == '\n') {
			is_comment = false;
			prev_word = PREV_NOTHING;
			++program_line;
		} else if (read_char == ';') {
			is_comment = true;
		}
		if (feof(source))
			break;
	}
	
	if (insert_labels())
		return 1;
	
	return 0;
}

static inline bool parse_word(char postfix) {
	if (postfix == '!') {
		if ((mode = match_word(word, (char *)directives, 2, 5)) == -1) {
			error("unknown directive");
			return 1;
		}
	} else {
		bool fail;
		switch (mode) {
			case MODE_DATA:
				fail = parse_data_word(postfix);
				break;
			default:
				fail = parse_code_word(postfix);
		}
		if (fail)
			return 1;
		if (program_len > 100) {
			error("the program is too long");
			return 1;
		}
	}
	return 0;
}

static inline bool insert_labels() {
	short i, j;
	for (i = 0; i < label_inst_n; ++i) {
		struct label_type label = label_instances[i];
		for (j = 0; j < labels_n; ++j)
			if(str_comp(label.name, labels[j].name))
				break;
		if (j >= labels_n) {
			printf("error: unknown label \"%s\"\n", label.name);
			return 1;
		}
		put_operand(label.place, labels[j].place);
	}
	return 0;
}

static void report() {
	short i;
	puts("labels:");
	for (i = 0; i < labels_n; ++i)
		printf("\t%s\n", labels[i].name);
	puts("program:");
	for (i = 0; i < program_len; ++i)
		printf("\t%02d - %03d\n", i, buffer[i]);
}

static void error(const char msg[]) {
	printf("error at line %d: %s\n", program_line, msg);
}

static bool match_char(char c, const char source[]) {
	int i;
	for (i = 0; c != source[i] && source[i] != '\0'; i++)
		;
	return source[i] != '\0';
}

static inline bool is_digit(char c) {
	return (c >= '0') && (c <= '9');
}

static signed char match_word(char word[], const char *list, const char n, const char m) {
	// list[n][m]
	unsigned char i, j;
	for (i = 0; i < n; ++i)
	{
		for (j = 0; j < m && word[j] == *(list + i * m + j) && word[j] != '\0'; ++j)
			;
		if (word[j] == *(list + i * m + j))
			return i;
	}
	return -1;
}

static short parse_number(char word[]) {
	unsigned char i;
	char c;
	short n = 0;
	for (i = 0; (c = word[i]) != '\0'; ++i) {
		n = n * 10 + (c - '0');
		if (n >= 1000 || c < '0' || c > '9')
			return -1;
	}
	return n;
}

static inline void put_opcode(short place, unsigned char instruction) {
	buffer[place] = mnemonics[instruction].code;
}

static inline void put_operand(short place, unsigned char n) {
	buffer[place] += n;
}

static inline void put_number(short place, short n) {
	buffer[program_len] = n;
}

static bool parse_code_word(char postfix) {
	short meaning;  // could have been a union
	for (meaning = 0; meaning < ASM_MNEMONICS_N; ++meaning)
		if (str_comp(word, (char *) mnemonics[meaning].name))
			break;
	if (meaning < ASM_MNEMONICS_N) {
		if (prev_word == PREV_COMMAND) {
			error("bad word order");
			return 1;
		}
		put_opcode(program_len++, meaning);
		prev_word = mnemonics[meaning].arg ? PREV_COMMAND : PREV_OPERAND;
	} else if (is_digit(word[0])) {
		if ((meaning = parse_number(word)) == -1) {
			error("bad number");
			return 1;
		}
		if (prev_word == PREV_COMMAND) {
			if (meaning >= 100) {
				error("the operand is too big");
				return 1;
			}
			put_operand(program_len - 1, meaning);
		} else {
			put_number(program_len, meaning);
			++program_len;
		}
		prev_word = PREV_OPERAND;
	} else if (postfix == ':') {
		if (add_label(word))
			return 1;
		prev_word = PREV_LABEL;
	} else if (prev_word == PREV_COMMAND) {
		put_label(word);
		prev_word = PREV_OPERAND;
	} else {
		error("bad word");
		return 1;
	}
	return 0;
}

static bool parse_data_word(char postfix) {
	short meaning;
	if (is_digit(word[0])) {
		if ((meaning = parse_number(word)) == -1) {
			error("bad number");
			return 1;
		}
		put_number(program_len, meaning);
		++program_len;
	} else
		add_label(word);
	return 0;
}

static bool add_label(char name[]) {
	short label;
	unsigned char i;
	for (label = 0; label < labels_n; ++label)
		if (str_comp(name, labels[label].name)) {
			error("label got redefined");
			return 1;
		}
	for (i = 0; name[i]; ++i)
		labels[labels_n].name[i] = name[i];
	labels[labels_n].place = program_len;
	if (++labels_n >= ASM_LABELS_N) {
		error("too many labels");
		return 1;
	}
	return 0;
}

static bool put_label(char name[]) {
	unsigned char i;
	for (i = 0; name[i]; ++i)
		label_instances[label_inst_n].name[i] = name[i];
	label_instances[label_inst_n].place = program_len - 1;
	if (++label_inst_n >= ASM_LABEL_INSTANCES_N) {
		error("too many labels");
		return 1;
	}
	return 0;
}
