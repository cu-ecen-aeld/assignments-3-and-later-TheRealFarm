#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <syslog.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

volatile sig_atomic_t termination_flag = 0;

void signal_handler(int signum) {
	termination_flag = 1;
	syslog(LOG_INFO, "Caught signal, exiting");
}

int setup_signal_handler() {
	struct sigaction action;
	memset(&action, 0, sizeof(action));
	action.sa_handler = signal_handler;
	if (sigaction(SIGINT, &action, NULL) == -1) {
		syslog(LOG_ERR, "Failed to set up SIGINT signal handler");
		return -1;
	}
	if (sigaction(SIGTERM, &action, NULL) == -1) {
		syslog(LOG_ERR, "Failed to set up SIGTERM signal handler");
		return -1;
	}
	return 0;
}

ssize_t send_data(int client_fd, char* temp_file_name) {
	syslog(LOG_INFO, "Opening read file...");
	int file_fd_read = open(temp_file_name, O_RDONLY);
	if (file_fd_read == -1) {
		closelog();
		return -1;
	}
	syslog(LOG_INFO, "Read file opened: '%s' fd: %d",temp_file_name, file_fd_read);
	
	char file_buffer[BUFFER_SIZE];
	ssize_t bytes_read;
	ssize_t total_bytes_read = 0;
	while((bytes_read = read(file_fd_read, file_buffer, BUFFER_SIZE)) > 0) {
		syslog(LOG_INFO, "Bytes read: %zd", bytes_read);
		syslog(LOG_INFO, "Data sent to client: %.*s", (int)bytes_read, file_buffer);
		if (send(client_fd, file_buffer, bytes_read, 0) == -1) {
			syslog(LOG_ERR, "send() failed: %s", strerror(errno));
			return -1;
		}
		total_bytes_read += bytes_read;
	}
	syslog(LOG_INFO, "Closing read file. fd: %d", file_fd_read);
	if (close(file_fd_read) == -1) {
		syslog(LOG_ERR, "Failed to close read file");
	};

	return bytes_read < 0 ? bytes_read : total_bytes_read;
}

int accept_connections(int server_fd, char* temp_file_name) {
	struct sockaddr_in client_addr;
	int addrln = sizeof(client_addr);

	while (!termination_flag) {
		int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, (socklen_t*)&addrln);
		if (client_fd == -1) {
			if (errno == EINTR && termination_flag) {
				break;
			}
			closelog();
			return -1;
		}

		char client_ip[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
		syslog(LOG_INFO, "Accepted connection from %s", client_ip);

		int file_fd = open(temp_file_name, O_WRONLY | O_APPEND | O_CREAT, 0644);
		if (file_fd == -1) {
			syslog(LOG_ERR, "Failed to open file");
			closelog();
			return -1;
		}

		char buffer[BUFFER_SIZE];
		ssize_t bytes_received = 0;
		int write_error = 0;
		int send_error = 0;

		while((bytes_received = recv(client_fd, buffer, sizeof(buffer), 0)) > 0) {
			syslog(LOG_INFO, "Bytes received: %zd", bytes_received);
			char* newline_pos = NULL;
			
			while((newline_pos = memchr(buffer, '\n', bytes_received)) != NULL) {
				ssize_t packet_length = newline_pos - buffer + 1;
				syslog(LOG_INFO, "packet length: %zd", packet_length);

				if (write(file_fd, buffer, packet_length) == -1) {
					write_error = 1;
					break;
				}
				memmove(buffer, newline_pos + 1, bytes_received - packet_length);
				bytes_received -= packet_length;
				fsync(file_fd);
				if (send_data(client_fd, temp_file_name) < 0) {
					send_error = 1;
					break;
				}
			}
			if (newline_pos == NULL) {
				if (write(file_fd, buffer, bytes_received) == -1) {
					write_error = 1;
					break;
				}
			}
		}

		if (write_error == 1) {
			syslog(LOG_ERR, "Error during write");
			closelog();
			return -1;
		} else if (bytes_received == -1) {
			syslog(LOG_ERR, "recv() failed: %s", strerror(errno));
			closelog();
			return -1;
		}

		syslog(LOG_INFO, "Closing write file descriptor. fd: %d", file_fd);
		if (close(file_fd) == -1) {
			syslog(LOG_ERR, "Error closing write file: %s", strerror(errno));
			closelog();
			return -1;
		}

		syslog(LOG_INFO, "Closing client socket. fd: %d", client_fd);
		syslog(LOG_INFO, "Closed connection from %s", client_ip);
		close(client_fd);
	}
	
	return 0;
}

int main(int argc, char* argv[]) {
	int server_fd;
	struct sockaddr_in address;
	int opt = 1;
	int daemon_mode = (argc > 1 && strcmp(argv[1], "-d") == 0) ? 1 : 0;

	openlog("aesdsocket", LOG_PID, LOG_USER);

	syslog(LOG_INFO, "registering signals");
	if (setup_signal_handler() == -1) {
		closelog();
		return -1;
	}

	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		syslog(LOG_ERR, "could not open socket");
		closelog();
		return -1;
	}

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(9000);

	if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == -1) {
		syslog(LOG_ERR, "could not bind socket");
		closelog();
		return -1;
	}
	
	if (daemon_mode) {
		pid_t pid = fork();
		if (pid < 0) {
			syslog(LOG_ERR, "Fork failed");
			closelog();
			return -1;
		} else if (pid > 0) {
			syslog(LOG_INFO, "Fork successful");
			closelog();
			exit(EXIT_SUCCESS);
		}

		umask(0);
		if(chdir("/") == -1) {
			syslog(LOG_ERR, "Failed to change working directory to root");
			return -1;
		}
		close(STDIN_FILENO);
		close(STDOUT_FILENO);
		close(STDERR_FILENO);
	}

	if (listen(server_fd, 3) == -1) {
		closelog();
		return -1;
	}
	
	char* temp_file_name = "/var/tmp/aesdsocketdata";
	int return_code = accept_connections(server_fd, temp_file_name);
	if (remove(temp_file_name) == 0) {
		syslog(LOG_INFO, "Successfully deleted file: %s", temp_file_name);
	} else {
		syslog(LOG_ERR, "Failed to delete file: %s", temp_file_name);
		return_code = -1;
	}

	close(server_fd);

	closelog();
	return return_code;
}
