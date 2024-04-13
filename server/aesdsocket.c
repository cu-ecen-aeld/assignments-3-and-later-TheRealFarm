#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <syslog.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

volatile sig_atomic_t termination_flag = 0;

void signal_handler(int signum) {
	termination_flag = 1;
	syslog(LOG_INFO, "Caught signal, exiting");
}

int accept_connections(int server_fd) {
	int new_socket_fd;
	struct sockaddr_in client_addr;
	int addrln = sizeof(client_addr);

	while (!termination_flag) {
		if (new_socket_fd = accept(server_fd, (struct sockaddr*)&client_addr, (socklen_t*)&addrln) == -1) {
			closelog();
			return -1;
		}

		char client_ip[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
		syslog(LOG_INFO, "Accepted connection from %s", client_ip);

		int file_fd = open("/var/tmp/aesdsocketdata", O_WRONLY | O_APPEND | O_CREAT, 0644);
		if (file_fd == -1) {
			syslog(LOG_ERR, "Failed to open file");
			closelog();
			return -1;
		}

		char buffer[BUFFER_SIZE];
		ssize_t bytes_received;
		ssize_t total_bytes_received;
		int write_error = 0;

		while((bytes_received = recv(new_socket_fd, buffer, sizeof(buffer), 0)) > 0) {
			total_bytes_received += bytes_received;
			char* newline_pos;
			while((newline_pos = memchr(buffer, '\n', total_bytes_received)) != NULL) {
				ssize_t packet_length = newline_pos - buffer + 1;

				if (write(file_fd, buffer, packet_length) == -1) {
					write_error = 1;
					break;
				}
				memmove(buffer, newline_pos + 1, total_bytes_received - packet_length);
				total_bytes_received -= packet_length;
			}
		}

		if (write_error == 1 || bytes_received == -1) {
			syslog(LOG_ERR, "No bytes received");
			closelog();
			return -1;
		}

		close(file_fd);

		int file_fd_read = open("/var/tmp/aesdsocketdata", O_RDONLY);
		if (file_fd_read == -1) {
			closelog();
			return -1;
		}
	
		char file_buffer[BUFFER_SIZE];
		ssize_t bytes_read;
		int send_error = 0;
		while((bytes_read = read(file_fd_read, file_buffer, BUFFER_SIZE)) > 0) {
			if (send(new_socket_fd, file_buffer, bytes_read, 0) == -1) {
				send_error = 1;
				break;
			}
		}

		if (send_error == 1 || bytes_read == -1) {
			closelog();
			return -1;
		}

		close(new_socket_fd);
	}
	
	return 0;
}

int main() {
	int server_fd;
	struct sockaddr_in address;
	int opt = 1;

	openlog("aesdsocket", LOG_PID, LOG_USER);

	syslog(LOG_INFO, "registering signals");
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);

	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		closelog();
		return -1;
	}

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(9000);

	if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == -1) {
		closelog();
		return -1;
	}
	syslog(LOG_INFO, "Server socket binded");

	if (listen(server_fd, 3) == -1) {
		closelog();
		return -1;
	}
	syslog(LOG_INFO, "Listening for connections");
	
	int return_code = accept_connections(server_fd);

	close(server_fd);

	closelog();
	return return_code;
}
