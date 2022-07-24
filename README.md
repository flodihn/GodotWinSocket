# WinSocket plugin for the Godot engine

## Introduction
At the time of writing 2021-08, trying to use the StreamPeerTCP class, it did 
non manage to connect to my server for unknown reason. It felt very bugggy
so I decided to create my own plugin for low level socket communciation.

This plugin is only meant to be used as a client, it does not expose any
socket listen functions, required to act as a server.

## Quickstart receive message using blocking socket:
```
extends Node

export var win_socket_script: NodePath
var win_socket
var dataBuffer: StreamPeerBuffer
var messagesReceived = 0
var run = true

func _ready():
    dataBuffer = StreamPeerBuffer.new()
    # This is the max packet size you want to receive.
    dataBuffer.resize(16384)
    win_socket = get_node(win_socket_script)
    win_socket.set_debug(false)
    win_socket.winsock_init()
    var connectResult = win_socket.connect_to_host("localhost", 2000)
    win_socket.set_blocking(true) # This is the default, it can be left out
    
    if connectResult > 0:
        print("Connection error: ", connectResult)
        return
        
    win_socket.set_message_header_size(2)
    win_socket.set_message_buffer(dataBuffer)
    
    # Receive first message.
    var messageSize = win_socket.receive_message()
    print(dataBuffer.get_string(messageSize))
    ¨
    # Receive another message.
    messageSize = win_socket.receive_message()
    print(dataBuffer.get_string(messageSize))

    win_socket.disconnect()
    win_socket.winsock_cleanup()
```

## Quickstart receive message using non blocking socket:
```
extends Node

export var win_socket_script: NodePath
var win_socket
var dataBuffer: StreamPeerBuffer
var messagesReceived = 0
var run = true

func _ready():
    dataBuffer = StreamPeerBuffer.new()
    # This is the max packet size you want to receive.
    dataBuffer.resize(16384)
    win_socket = get_node(win_socket_script)
    win_socket.set_debug(false)
    win_socket.winsock_init()
    var connectResult = win_socket.connect_to_host("localhost", 2000)
    win_socket.set_blocking(false)
    
    if connectResult > 0:
        print("Connection error: ", connectResult)
        return
        
    win_socket.set_message_header_size(2)
    win_socket.set_message_buffer(dataBuffer)
    
func _process(delta):
    
    if run == false:
        return

    var messageSize = win_socket.receive_message()
    
    if(messageSize > 0):
        print("Received message:")
        print(dataBuffer.get_string(messageSize))
        messagesReceived += 1

    if messagesReceived == 2:
        run = false
        win_socket.disconnect()
        win_socket.winsock_cleanup()
        print("Disconnected.")
```

## Quick start low level receive:
```
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
    var connectResult = win_socket.connect_to_host("localhost", 2000)
    win_socket.set_blocking(true)
    
    if connectResult > 0:
        print("Connection error: ", connectResult)
        return
    
    # Before calling the win_sock.receive, we need to create a large enough
    # receive buffer to contain the packet.
    win_socket.set_receive_buffer(1024)
    win_socket.receive(dataBuffer, 0, 2)
    dataBuffer.seek(0)
    var header = dataBuffer.get_u16()
    print("Got header: ", header)
```

## How to use
Copy the "bin\win64\WinSocket.dll" into your godot project and add it as a GDNative Library by creating a gdnlib resource, for details check Godot tutorial here: https://docs.godotengine.org/en/stable/tutorials/plugins/gdnative/gdnative-c-example.html#creating-the-gdnativelibrary-gdnlib-file

This is demonstrated by the GodotWinSocket example project found in this repository.