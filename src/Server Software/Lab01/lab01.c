#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>

const int maxn = 105;

int main () {
	FILE* file;
	char buffer[maxn];
	int result;

	int pid = fork();

	if (pid == 0) {

		// Child process open the PIPE file.
		file = fopen("PIPE", "r");
		if (file == NULL) {

			printf("**ERROR**: Child process failed to open PIPE file, %s!\n", strerror(errno));
			exit(EXIT_FAILURE);
		}

		// Child process read message from PIPE file.
		while (!feof(file)) {

			if (ferror(file)) {

				printf("**ERROR**: Child process failed to read PIPE file, %s!\n", strerror(errno));
				exit(EXIT_FAILURE);
			}

			if (fgets(buffer, 100, file) != NULL) {

				printf("Child got: %s", buffer);
			}
		}

		// Child process close the PIPE file.
		if (fclose(file) != 0) {

			printf("**ERROR**: Child process failed to close PIPE file, %s!\n", strerror(errno));
			exit(EXIT_FAILURE);
		}

		printf("child done\n");

	} else if (pid > 0) {

		// Parent process open PIPE file.
		file = fopen("PIPE", "w");
		if (file == NULL) {

			printf("**ERROR**: Parent process failed to open PIPE file, %s!\n", strerror(errno));
			exit(EXIT_FAILURE);
		}

		// Parent process writh content.
		fputs("parent writing to pipe\n", file);
		fputs("hello 1\n", file);
		fputs("hello 2\n", file);

		// Parent process close the PIPE file.
		if (fclose(file) != 0) {

			printf("**ERROR**: Parent process failed to close PIPE file, %s!\n", strerror(errno));
			exit(EXIT_FAILURE);
		}

		// Parent process wait the child process.
		int err = waitpid(pid, &result, WNOHANG);
		if (err == pid) {

			printf("Subprocess %d is done - returned %i.\n", pid, result); 

		} else {

			printf("**ERROR**: Parent process failed to wait child process!\n");
			exit(EXIT_FAILURE);
		}

	} else {

		printf("**ERROR**: Parent process failed to create a child process!\n");
		exit(EXIT_FAILURE);
	}

	return EXIT_SUCCESS;
}
