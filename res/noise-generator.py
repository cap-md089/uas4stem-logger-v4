from struct import pack, unpack
import socket
import math
import thread
import time
import random
import os

class CurrentState :
	timeInAir = 0
	lat = (random.random() - 1) * 360
	lng = (random.random() - 1) * 360
	battery_voltage = 12.5
	battery_remaining = 90
	alt = 0.5
	groundspeed = 0
	ch3percent = 0
	DistToHome = 0.00
	roll = 0
	pitch = 0
	yaw = 0
	verticalspeed = 0
	armed = False

	flying = False

	def update(self) :
		if self.flying :
			self.timeInAir += 1
			self.lat += ((random.random() - 0.5) / 10000.0)
			self.lng += ((random.random() - 0.5) / 10000.0)
			self.alt += ((random.random() - 0.5) / 10000.0)
			self.battery_voltage -= ((random.random() - 0.1) / 100.0)
		else :
			self.alt = 0.5

	def takeoff(self) :
		if self.armed :
			self.flying = True
			self.ch3percent = 50
			self.alt = 30
			self.groundspeed = 5

	def land(self) :
		self.flying = False
		self.ch3percent = 0
		self.groundspeed = 0

cs = CurrentState()

class ScriptRunner :
	def GetParam(self, arg) :
		return 10.00

	def ChangeMode(self, mode) :
		if mode == 'AUTO' and not cs.flying :
			cs.takeoff()

		if mode == 'RTL' and cs.flying :
			cs.land()

Script = ScriptRunner()

class MavUtility :
	class MAV_CMD :
		DO_SET_SERVO = 1

	def doARM(self, arm) :
		cs.armed = arm

	def doCommand(self, a1, a2, a3, a4, a5, a6, a7, a8) :
		pass

MAV = MavUtility();

import sys

sys.path.extend([
	"E:\\Python27",
	"E:\\Python27\\DLLs",
	"E:\\Python27\\Lib",
	"E:\\Python27\\Lib\\plat-win",
	"E:\\Python27\\Lib\\site-packages",
	"C:\\WINDOWS\\SYSTEM32\\python27.zip"
])
sys.path = list(set(sys.path))

# This is used to close everything down safely
shutdown = False

# Used to signal there is a connection
has_connection = False

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
	global MAV
	global cs
	global has_connection
	global Script

	server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	server.bind(('127.0.0.1', 1337))
	server.listen(5)

	while not shutdown :
		has_connection = False
		conn, addr = server.accept()

		has_connection = True

		while not shutdown :
			try :
				incoming_packet_size = conn.recv(1)
			except Exception as e :
				break
			size = ord(incoming_packet_size[0])
			incoming_packet = conn.recv(size)
			data = buffer(incoming_packet)
			func = data[0]

			os.system('cls')
			print "Received command: ", data
			print_cs()
			print ' > ',

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

			if func == 4 :
				# Maybe auto open a file specified, for the setup?
				# Will need to pass a file path for that...
				path = conn.recv(data[1])
				print path

			if func == 5 :
				Script.ChangeMode('RTL')

			if func == 6 :
				Script.ChangeMode('AUTO')

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
	global Script

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
	global Script
	global cs
	global MAV

	descent = Script.GetParam('LAND_SPEED_HIGH')
	if descent == 0 :
		descent = Script.GetParam('WPNAV_SPEED_DN')

	rtl_land_speed = (
		(Script.GetParam('LAND_SPEED') + float(descent) + float(descent)) / 3
	) / 100

	time_required_to_get_home = cs.DistToHome / (Script.GetParam('WPNAV_SPEED') / 100)

	time_required_to_land = \
		(((cs.alt - 10) / Script.GetParam('LAND_SPEED'))) \
		if cs.alt > 10 else \
		0

	time_required_to_land += 20

	time_required_to_land2 = (10 if cs.alt > 10 else cs.alt) / Script.GetParam('LAND_SPEED')

	time_required_to_rtl = time_required_to_land + time_required_to_get_home + time_required_to_land2

	packed = ['C', 'S', 'D', 'A', 'T']
	packed.extend(pack(
		"<ddddddifffffffff?",
		cs.lat,
		cs.lng,
		cs.alt,
		cs.roll,
		cs.yaw,
		cs.pitch,
		int(cs.timeInAir),
		cs.battery_voltage,
		cs.battery_remaining,
		cs.groundspeed,
		cs.ch3percent,
		cs.DistToHome,
		cs.verticalspeed,
		Script.GetParam('WPNAV_SPEED') / 100,
		rtl_land_speed,
		time_required_to_rtl,
		cs.armed
	))
	packed.extend(['C', 'S', 'E', 'N', 'D'])

	return ''.join(packed)

sending = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

def update_cs() :
	global cs

	while not shutdown :
		time.sleep(1)
		cs.update()

def print_cs() :
	global has_connection

	print 'Flying:', ('YES' if cs.flying else 'NO')
	print 'Armed:', ('YES' if cs.armed else 'NO')
	print 'Lat,lng:', str(cs.lat) + ',' + str(cs.lng)
	print 'Time in air:', str(cs.timeInAir)
	print 'Altitude:', str(cs.alt)
	print 'Connected:', ('YES' if has_connection else 'NO')
	print 'Battery voltage:', str(cs.battery_voltage)

try :
	thread.start_new_thread(receive_command_thread, tuple([]))
	thread.start_new_thread(send_packets_thread, tuple([sending]))
	thread.start_new_thread(update_cs, tuple([]))
except Exception as e :
	print "Thread error: {0}".format(e)
	
while True :
	print_cs()
	cmd = raw_input(' > ');
	cmds = cmd.lower().split(' ')

	os.system('cls')

	if cmds[0] == 'stop' :
		shutdown = True
		break
	elif cmds[0] == 'takeoff' :
		if not cs.armed :
			print 'Cannot takeoff when not armed'
		else :
			cs.takeoff()
	elif cmds[0] == 'land' :
		cs.land()
	elif cmds[0] == 'arm' :
		cs.armed = True
	elif cmds[0] == 'disarm' :
		if cs.flying :
			print 'Cannot disarm while flying'
		else :
			cs.armed = False
	elif cmds[0] == 'status' :
		pass
	elif cmds[0] == 'help' :
		print """
Available commands:

stop
	stops program execution

takeoff
	makes the virtual UAV takeoff and hover

	only works if UAV is armed

land
	makes the virtual UAV land on the ground

arm
	arms the virtual UAV

disarm
	disarms the virtual UAV

status
	updates the status display

help
	displays this help message

Each command updates the status display

		"""
	else :
		print 'Not a valid command:', cmds[0]

time.sleep(0.5)

print "Shut down complete"