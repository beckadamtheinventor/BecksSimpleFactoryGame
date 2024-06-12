/* This program is here because Windows COPY command can't copy a file from path/A to path/B */
#include <stdio.h>
int main(int argc, char **argv) {
    FILE *fd, *fd2;
    for (int i=2; i<argc; i+=2) {
        if ((fd = fopen(argv[i-1], "rb"))) {
			fseek(fd, 0, SEEK_END);
			size_t flen = ftell(fd);
			fseek(fd, 0, SEEK_SET);
			if ((fd2 = fopen(argv[i], "wb"))) {
				void *buffer = malloc(flen);
				if (buffer == NULL) {
					printf("Failed to malloc copy buffer of %llu bytes\n", flen);
					return 1;
				}
				fread(buffer, 1, flen, fd);
				fwrite(buffer, 1, flen, fd2);
				fclose(fd2);
				fclose(fd);
			} else {
				fclose(fd);
				printf("Failed to write destination file %s\n", argv[i]);
				return -2;
			}
		} else {
			printf("Failed to read source file %s\n", argv[i-1]);
			return -1;
        }
    }
	return 0;
}