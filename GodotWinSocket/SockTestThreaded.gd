extends Node
signal message_received

var thread
export var win_socket_script: NodePath
var win_socket
var messageBuffer: StreamPeerBuffer
var messagesReceived = 0
var run = true

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
    win_socket = get_node(win_socket_script)
    win_socket.set_ssl(true)
    # Print debug output in the console.
    win_socket.set_debug(true)
    win_socket.set_message_header_size(2)
    win_socket.set_message_buffer(messageBuffer)
    var connectResult = win_socket.connect_to_host("localhost", 2000)

    var sendBuffer = PoolByteArray()
    sendBuffer.append(1)
    var sendMessage = "Msg from client"
    sendBuffer.append_array(sendMessage.to_ascii())
    win_socket.send_message(sendBuffer);

func on_message_received(message):
    print(message)

func receive_data(userdata):
    while true:
        var messageSize = win_socket.receive_message()
        var message = messageBuffer.get_string(messageSize)
        call_deferred("emit_signal", "message_received", message)

func _process(_delta):
    pass
        
