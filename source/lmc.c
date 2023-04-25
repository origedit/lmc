/* 
 * the little man computer was designed by stuard madnick
 * implemented by origedit :^)
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

enum format_type {FOR_DEC, FOR_BIN};
bool str_comp(char a[], char b[]);  // non-stadard function for fun

#include "lmc-asm.c"
#include "lmc-emu.c"

static int argc2;
static char **argv2;
static int argi;
static char *arg();

int main(int argc, char **argv) {
	argc2 = argc;
	argv2 = argv;
	argi = 1;
	char *argt;
	
	if (str_comp(arg(), "-a")) {
		char *source_filename = arg();
		char *dest_filename = arg();
		enum format_type format = FOR_BIN;
		argt = arg();
		if (str_comp(argt, "-d"))
			format = FOR_DEC;
		else if (!str_comp(argt, "-b"))
			--argi;
		if (assemble(source_filename, dest_filename, format))
			return 1;
	} else
		--argi;

	if ((argt = arg())) {
		if (str_comp(argt, "-r")){
			if (lmc(arg()))
				return 1;
		} else {
			puts("weird arguments");
			return 1;
		}
	}
	return 0;
}

static char *arg() {
	if (argi < argc2)
		return argv2[argi++];
	return NULL;
}

bool str_comp(char a[], char b[]) {
	short i;
	if (!(a && b))
		return false;
	for (i = 0; a[i] == b[i] && a[i] && b[i]; ++i)
		;
	if (a[i] == b[i])
		return true;
	return false;
}
