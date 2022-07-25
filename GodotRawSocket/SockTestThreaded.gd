extends Node
signal message_received

var thread
export var win_socket_script: NodePath
var raw_socket
var messageBuffer: StreamPeerBuffer
var messagesReceived = 0
var run = true
export (NodePath) var message_input

func _ready():
	thread = Thread.new()
	self.connect("message_received", self, "on_message_received")
	init_socket()
	thread.start(self, "receive_data", "ignored")

func init_socket():
	messageBuffer = StreamPeerBuffer.new()
	messageBuffer.resize(16384)
	
	# As an alternative to setting big_endian to true or false, values can be
	# converted from network to host byte order by calling 
	# win_sock.ntohs(short) and win_sock.htonl(int).
	messageBuffer.big_endian = true
	raw_socket = get_node(win_socket_script)
	raw_socket.set_ssl(true)
	# Print debug output in the console.
	raw_socket.set_debug(true)
	raw_socket.set_message_header_size(2)
	raw_socket.set_message_buffer(messageBuffer)
	var connectResult = raw_socket.connect_to_host("192.168.1.211", 2000)

func on_message_received(message):
	print(message)

func receive_data(userdata):
	while true:
		var messageSize = raw_socket.receive_message()
		var message = messageBuffer.get_string(messageSize)
		call_deferred("emit_signal", "message_received", message)

func _process(_delta):
	pass

func send_message1():
	var sendBuffer = PoolByteArray()
	sendBuffer.append(1)
	var sendMessage = "Msg from client"
	sendBuffer.append_array(sendMessage.to_ascii())
	raw_socket.send_message(sendBuffer)

func send_message2():
	var sendBuffer = PoolByteArray()
	sendBuffer.append(1)
	var sendMessage = "Msg from client2"
	sendBuffer.append_array(sendMessage.to_ascii())
	raw_socket.send_message(sendBuffer)

func send_message3():
	var sendBuffer = PoolByteArray()
	sendBuffer.append(1)
	var sendMessage = get_node(message_input).text
	sendBuffer.append_array(sendMessage.to_ascii())
	raw_socket.send_message(sendBuffer)

func send_string(msg):
	var sendBuffer = PoolByteArray()
	sendBuffer.append(1)
	var sendMessage = msg
	sendBuffer.append_array(sendMessage.to_ascii())
	raw_socket.send_message(sendBuffer)

func make_string(msg):
	var sendBuffer = PoolByteArray()
	sendBuffer.append(len(msg))
	sendBuffer.append_array(msg.to_ascii())
	raw_socket.send_message(sendBuffer)
