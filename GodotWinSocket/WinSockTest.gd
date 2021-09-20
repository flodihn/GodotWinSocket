extends Node

export var win_socket_script: NodePath
var win_socket
var dataBuffer: StreamPeerBuffer
var messagesReceived = 0
var run = true

func _ready():
    dataBuffer = StreamPeerBuffer.new()
    dataBuffer.resize(16384)
    
    # As an alternative to setting bid_endian to true or false, values can be
    # converted from network to host byte order by calling 
    # win_sock.ntohs(short) and win_sock.htonl(int).
    dataBuffer.big_endian = true
    win_socket = get_node(win_socket_script)
    win_socket.set_debug(true)
    win_socket.winsock_init()
    win_socket.set_message_header_size(2)
    
    win_socket.set_message_buffer(dataBuffer)
    var connectResult = win_socket.secure_connect_to_host("localhost", 2000)

    win_socket.set_blocking(true)
    
    if connectResult > 0:
        print("Connection error: ", connectResult)
        return
    
    # Before calling the win_sock.receive, we need to create a large enough
    # receive buffer to contain the packet.
    print("Receiving message...")
    var messageSize = win_socket.receive_message()
    #var message = dataBuffer.get_string(messageSize)
    #print(message)
    
func _process(_delta):
    pass
        
