extends Node

export var win_socket_script: NodePath
var win_socket
var dataBuffer: StreamPeerBuffer

func _ready():
    dataBuffer = StreamPeerBuffer.new()
    dataBuffer.resize(16384)
    win_socket = get_node(win_socket_script)
    win_socket.connect_to_host("localhost", 2000)
    win_socket.set_message_header_size(2)
    win_socket.set_message_buffer(dataBuffer)

    print("Waiting for message...")
    win_socket.blocking_receive_message()

    print("Received message:")
    print("pos: ", dataBuffer.get_position())
    print(dataBuffer.get_string(11))

    win_socket.blocking_receive_message()
    print("pos: ", dataBuffer.get_position())
    print(dataBuffer.get_string(17))

