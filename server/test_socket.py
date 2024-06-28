import socket

def main():
	server_address = ('localhost', 9000)

	client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

	try:
		client_socket.connect(server_address)
		client_socket.settimeout(5)

		data = b"Hello server!\nThis is my server\nMy bad didn't mean to be administrator"
		client_socket.sendall(data)

		print("Data sent to server:", data)

		response = client_socket.recv(1024)

		print("Response from server:", response.decode())

	except socket.timeout:
		print("Timeout: No response from server")
	except Exception as e:
		print("Error:", e)
	finally:
		client_socket.close()

if __name__ == "__main__":
	main()
