print "Start"

import sys

print "SYS libraries imported"


sys.path.extend([
    "E:\\Python27",
    "E:\\Python27\\DLLs",
    "E:\\Python27\\Lib",
    "E:\\Python27\\Lib\\plat-win",
    "E:\\Python27\\Lib\\site-packages",
    "C:\\WINDOWS\\SYSTEM32\\python27.zip"
])
sys.path = list(set(sys.path))

from struct import pack
print "Struct.pack imported"
import socket
print "Socket imported"
import math
print "Math imported"
import thread
print "Threads imported"
import time
print "Time imported"

print "Running"

# This is used to close everything down safely
shutdown = False

"""
	Utility for converting string to array of numbers
"""
def buffer(string_input) :
	ret = []
	for i in string_input :
		ret.append(ord(i))
	return ret

"""
	A thread for listening on a TCP port and parsing the commands received
"""
def receive_command_thread() :
	global shutdown

	server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	server.bind(('127.0.0.1', 1337))
	server.listen(5)

	conn, addr = server.accept()

	while not shutdown :
		incoming_packet_size = conn.recv(1)
		size = ord(incoming_packet_size[0])
		incoming_packet = conn.recv(size)
		data = buffer(incoming_packet)
		func = data[0]

		print "Received command: ", data

		if func == 1 :
			try :
				servo = data[1]
				position = data[2] * 1000
				MAV.doCommand(MAV.MAV_CMD.DO_SET_SERVO, servo,
					position, 0, 0, 0, 0, 0)
			except Exception as e :
				print "Could not set servo"

		if func == 2 :
			try :
				MAV.doARM(not cs.armed)
			except Exception as e :
				print "Could not toggle arm"

		if func == 3 :
			print "Shutting down"
			shutdown = True

	server.close()
	print "Server gone. Good bye, client"

"""
	A simple thread to wait for a short time then send the current state over
"""
def send_packets_thread(socket) :
	global shutdown

	while not shutdown :
		time.sleep(0.00833333)

		if should_send_packet() :
			serialized_cs = serialize_current_state()
			data = socket.sendto(serialized_cs, ('127.0.0.1', 54248))

			if data != len(serialized_cs) :
				print "Packet failed to send"

	print "Client gone. Good bye, server"

"""
	Used to determine if a packet should be sent

	If it returns False, serialize_current_state() will throw a divide by 0 error if called
"""
def should_send_packet() :
	descent = Script.GetParam('LAND_SPEED_HIGH')
	if descent == 0 :
		descent = Script.GetParam('WPNAV_SPEED_DN')

	return not (
		Script.GetParam('LAND_SPEED') == 0 or \
			Script.GetParam('WPNAV_SPEED') == 0 or \
				descent == 0
	)

"""
	Serializes the current state as a packet recognized by the C++ backend

	Basically copies numbers over, but uses a header and footer to help be sure that everything is ok
"""
def serialize_current_state() :
	descent = Script.GetParam('LAND_SPEED_HIGH')
	if descent == 0 :
		descent = Script.GetParam('WPNAV_SPEED_DN')

	rtl_land_speed = (
		(Script.GetParam('LAND_SPEED') + float(descent) + float(descent)) / 3
	) / 100

	packed = [67, 83, 69, 78, 68]
	packed.extend(pack(
		"<i<d<d<f<f<d<f<f<f<f<f<f<d<d<d<f<?",
		cs.timeInAir,
		cs.lat,
		cs.lng,
		cs.battery_voltage,
		cs.battery_remaining,
		cs.alt,
		cs.groundspeed,
		cs.ch3percent,
		cs.DistToHome,
		cs.verticalspeed,
		Script.GetParam('WPNAV_SPEED') / 100,
		rtl_land_speed,
		cs.roll,
		cs.yaw,
		cs.pitch,
        cs.DistToHome / (Script.GetParam('WPNAV_SPEED')/100) + 20 + (((cs.alt - 10) / Script.GetParam('LAND_SPEED')) \
                                                                         if cs.alt > 10 else 0) + (10 if cs.alt > 10 else cs.alt) / Script.GetParam('LAND_SPEED'),
		cs.armed
	))
	packed.extend([67, 83, 69, 78, 68])

	return packed

sending = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

try :
	thread.start_new_thread(receive_command_thread, tuple([]))
except Exception as e :
	print "Thread error: {0}".format(e)

send_packets_thread(sending)