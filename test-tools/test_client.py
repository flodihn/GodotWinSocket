import socket
import struct
import time
import ssl

hostname = '192.168.1.211'
port = 2000

context = ssl.create_default_context(purpose = ssl.Purpose.SERVER_AUTH)
context.check_hostname = False

with socket.create_connection((hostname, port)) as tcpSocket:
	with context.wrap_socket(tcpSocket, server_hostname=hostname) as sslSocket:
		print(sslSocket.version())

		email = b'MyEmail'
		password = b'MyPassword'
		packet = struct.pack('bb7sb10s', 1, len(email), email, len(password), password)
		header = struct.pack('>H', len(packet))

		print(f"Sending header {len(packet)}...")
		bytesSent = sslSocket.send(header)
		print(f"Sent header {header} ({bytesSent}).")
		bytesSent = sslSocket.send(packet)
		print(f"Successfully sent packet {bytesSent}", packet)

		header = sslSocket.recv(2)
		if(len(header) < 1):
			print(f"Error: No header received {len(header)}.")

		(messageSize,) = struct.unpack('>H', header)
		print(messageSize)

		packet = sslSocket.recv(messageSize)
		print(packet)
		time.sleep(1)