from struct import pack, unpack
import socket
import math
import thread
import time
import random
import os
import curses
import array

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
	wpno = 0
	mode = 'Loiter'

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
		cs.mode = mode

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

error = ""

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
def receive_command_thread(stdscr) :
	global shutdown
	global MAV
	global cs
	global has_connection
	global Script
	global error

	conn = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

	while not shutdown :
		has_connection = False
		conn.connect(('127.0.0.1', 1337))
		conn.setblocking(0)
		conn.settimeout(0.1)

		has_connection = True

		while not shutdown :
			try :
				incoming_packet_size = conn.recv(1)
			except socket.timeout as e :
				continue
			except Exception as e :
				error = "Lost connection"
				break
			size = ord(incoming_packet_size[0])
			incoming_packet = conn.recv(size)
			data = buffer(incoming_packet)
			func = data[0]

			print_cs(stdscr)

			if func == 1 :
				try :
					servo = data[1]
					position = data[2] * 1000
					MAV.doCommand(MAV.MAV_CMD.DO_SET_SERVO, servo,
						position, 0, 0, 0, 0, 0)
				except Exception as e :
					error = "Could not set servo"

			if func == 2 :
				try :
					MAV.doARM(not cs.armed)
				except Exception as e :
					error = "Could not toggle arm"

			if func == 3 :
				error = "Shutting down"
				shutdown = True

			if func == 4 :
				# Maybe auto open a file specified, for the setup?
				# Will need to pass a file path for that...
				path = conn.recv(data[1])
				error = path

			if func == 5 :
				Script.ChangeMode('RTL')

			if func == 6 :
				Script.ChangeMode('AUTO')

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
				error = "Packet failed to send"

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
	Takes the current mode and puts it into something like an enum for the C++ side
"""
def get_mode_number() :
	if cs.mode.lower() == 'auto' :
		return 1
	elif cs.mode.lower() == 'loiter' :
		return 2
	elif cs.mode.lower() == 'althold' :
		return 3
	elif cs.mode.lower() == 'guided' :
		return 4
	elif cs.mode.lower() == 'rtl' :
		return 5

	return 0

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
		"<ddddddifffffffff?BB",
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
		cs.armed,
		cs.wpno,
		get_mode_number()
	))
	packed.extend(['C', 'S', 'E', 'N', 'D'])

	return ''.join(packed)

sending = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

def update_cs() :
	global cs

	while not shutdown :
		time.sleep(1)
		cs.update()

def print_cs(stdscr) :
	global has_connection
	global error

	stdscr.addstr(0, 0, 'Flying: %s' % ('YES' if cs.flying else 'NO '))
	stdscr.addstr(1, 0, 'Armed: %s' % ('YES' if cs.armed else 'NO '))
	stdscr.addstr(2, 0, 'Lat,lng: %f, %f        ' % (cs.lat, cs.lng))
	stdscr.addstr(3, 0, 'Time in air: %d    ' % cs.timeInAir)
	stdscr.addstr(4, 0, 'Altitude: %f      ' % cs.alt)
	stdscr.addstr(5, 0, 'Connected: %s' % ('YES' if has_connection else 'NO '))
	stdscr.addstr(6, 0, 'Mode: %s       ' % cs.mode)
	stdscr.addstr(7, 0, 'Battery voltage: %f      ' % cs.battery_voltage)
	stdscr.addstr(8, 0, ' ' * stdscr.getmaxyx()[1])
	stdscr.addstr(8, 0, error)


def main(stdscr) :
	global shutdown
	global MAV
	global cs
	global has_connection
	global Script
	global error

	try :
		thread.start_new_thread(receive_command_thread, tuple([stdscr]))
		thread.start_new_thread(send_packets_thread, tuple([sending]))
		thread.start_new_thread(update_cs, tuple([]))
	except Exception as e :
		print "Thread error: {0}".format(e)


	stdscr.nodelay(True)
	current_input = []

	stdscr.addstr(9, 0, ' > ')

	while not shutdown :
		print_cs(stdscr)
		stdscr.move(9, 3 + len(current_input))

		c = stdscr.getch()

		if c != -1 :
			if c == 127 :
				if len(current_input) > 0 :
					stdscr.addstr(9, 0, ' > %s' % (' ' * (len(current_input))))
					current_input.pop()
					cmd = array.array('B', current_input).tostring()
					stdscr.addstr(9, 0, ' > %s' % cmd)
			elif c == 10 :
				cmd = array.array('B', current_input).tostring()
				current_input = []
				stdscr.addstr(9, 0, ' > %s' % (' ' * len(cmd)))

				cmds = cmd.lower().strip().split(' ')

				if cmds[0] == 'stop' :
					shutdown = True
					break
				elif cmds[0] == 'takeoff' :
					if not cs.armed :
						error = 'Cannot takeoff when not armed'
					else :
						cs.takeoff()
				elif cmds[0] == 'land' :
					cs.land()
				elif cmds[0] == 'arm' :
					cs.armed = True
				elif cmds[0] == 'disarm' :
					if cs.flying :
						error = 'Cannot disarm while flying'
					else :
						cs.armed = False
				else :
					error = 'Not a valid command: ' + cmds[0]

			else :
				current_input.append(c)
				cmd = array.array('B', current_input).tostring()
				stdscr.addstr(9, 0, ' > %s' % cmd)

curses.wrapper(main)

time.sleep(0.5)
print "Shut down complete"
