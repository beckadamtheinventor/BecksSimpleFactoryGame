/* This program is here because Windows DEL command is dumb and errors if a file doesn't exist,
 * and none of the solutions I found online worked. */
#include <stdio.h>
int main(int argc, char **argv) {
    FILE *fd;
    for (int i=1; i<argc; i++) {
        if ((fd = fopen(argv[i], "rb"))) {
            fclose(fd);
            remove(argv[i]);
        }
    }
	return 0;
}