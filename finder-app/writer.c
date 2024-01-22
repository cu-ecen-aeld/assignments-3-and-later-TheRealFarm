#include <stdio.h>
#include <syslog.h>

int main(int argc, char *argv[]) {
	openlog("writer", LOG_PID, LOG_USER);
	if (argc != 3) {
		syslog(LOG_ERR, "Invalid number of arguments. Usage: ./writer <writefile> <writestr>");
		closelog();
		return 1;
	}

	FILE *file;

	file = fopen(argv[1], "w");

	if (file == NULL) {
		syslog(LOG_ERR, "Unable to open file %s", argv[1]);
		closelog();
		return 1;
	}

	syslog(LOG_DEBUG, "Writing %s to %s", argv[2], argv[1]);
	fprintf(file, argv[2]);

	fclose(file);
	closelog();

	return 0;
}

