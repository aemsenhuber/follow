#!/usr/bin/env python3
#
# follow - watch-like program with pager capabilities
#
# Copyright 2023 Alexandre Emsenhuber
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import sys
import time
import curses
import socket
import subprocess
import argparse

def positive_float( value ):
	parsed = float( value )
	if parsed > 0.:
		return parsed
	else:
		raise ValueError( "Value must be strictly positive" )

parser = argparse.ArgumentParser()

parser.add_argument( "command", help = "Command to watch" )
parser.add_argument( "arguments", nargs = argparse.REMAINDER, help = "Command to watch" )
parser.add_argument( "--interval", "-n", type = positive_float, default = 1., help = "Update interval time (seconds)" )
parser.add_argument( "--shell", "-s", action = "store_true", help = "Execute command through a shell" )

args = parser.parse_args()

command_args = [ args.command ] + args.arguments
if args.shell:
	command_args = " ".join( command_args )

title = socket.gethostname() + " " + args.command + " output"
title_height = 1
title_width = len( title )

stdscr = curses.initscr()
curses.noecho()
curses.curs_set( 0 )
curses.cbreak()
stdscr.keypad( True )

res_lines = None
res_max_height = 0
res_max_width = 0
elapsed = True
last_time = None

v_offset = 0
h_offset = 0
v_end = False
v_diff = 0
h_diff = 0
past = False

try:
	while True:
		stdscr.erase()
		screen_height, screen_width = stdscr.getmaxyx()

		try:
			stdscr.addstr( 0, int( ( screen_width - title_width ) / 2. ), title, curses.A_REVERSE )
		except:
			pass

		# Size of the zone where the output of the command will be display
		# One line less because of the title
		display_height = screen_height - title_height
		display_width = screen_width

		if elapsed:
			last_time = time.monotonic()

			# Execute the command
			process = subprocess.Popen( command_args, stdout = subprocess.PIPE, stderr = subprocess.STDOUT, shell = args.shell )
			stdout, stderr = process.communicate()
			del process

			# Process the result
			res_lines = stdout.decode( stdscr.encoding ).split( "\n" )

			# Size of the result in both directions
			res_max_height = len( res_lines )
			res_max_width = max( len( line ) for line in res_lines )

		if v_end:
			v_offset = max( res_max_height - display_height, 0 )
		elif v_diff != 0 and past:
			v_offset += v_diff
		elif v_diff > 0:
			v_offset = max( v_offset, min( v_offset + v_diff, max( res_max_height - display_height, 0 ) ) )
		elif v_diff < 0:
			v_offset = min( v_offset, max( v_offset + v_diff, 0 ) )

		if h_diff != 0 and past:
			h_offset += h_diff
		elif h_diff > 0:
			h_offset = max( h_offset, min( h_offset + h_diff, max( res_max_width - display_width, 0 ) ) )
		elif h_diff < 0:
			h_offset = min( h_offset, max( h_offset + h_diff, 0 ) )

		if v_offset > -display_height and v_offset < res_max_height and h_offset > -display_width and h_offset < res_max_width:
			if v_offset < 0:
				v_disp_off = -v_offset
				v_start = 0
			else:
				v_disp_off = 0
				v_start = v_offset

			if h_offset < 0:
				h_disp_off = -h_offset
				h_start = 0
			else:
				h_disp_off = 0
				h_start = h_offset

			for num, line in enumerate( res_lines[v_start:v_offset+display_height] ):
				line_len = len( line )
				if line_len <= h_offset:
					continue
				h_end = min( line_len - h_start, h_offset + display_width )
				try:
					stdscr.addstr( v_disp_off + num + title_height, h_disp_off, line[h_start:h_end] )
				except:
					pass

		stdscr.refresh()

		# We want to refresh the window every interval; this is done by waiting for a key to be pressed.
		# The wait interval is recomputed so that the time interval between two command execution is
		# as close as possible to the provided value.
		new_time = time.monotonic()
		# Remaining time to wait in seconds
		rem_time = args.interval - ( new_time - last_time )
		# Convert into tenths of seconds, which is the unit used by curses.halfdelay()
		tenths = int( rem_time * 10 + 0.5 )
		# curses.halfdelay takes a number between 1 and 255
		if tenths > 255:
			last = False
			tenths = 255
		else:
			last = True
		curses.halfdelay( max( tenths, 1 ) )

		# Reset these ones
		elapsed = False
		past = False

		# Get one character from STDIN
		c = stdscr.getch()

		# Handle key
		if c == -1:
			elapsed = last
		elif c == ord( 'r' ) or c == ord( 'R' );
			elapsed = True
		elif c == ord( 'q' ):
			raise KeyboardInterrupt
		elif c == curses.KEY_LEFT:
			h_diff = -1
		elif c == curses.KEY_RIGHT:
			h_diff = +1
		elif c == curses.KEY_UP or c == ord( 'k' ) or c == ord( 'y' ):
			v_diff = -1
		elif c == curses.KEY_DOWN or c == ord( 'e' ) or c == ord( 'j' ):
			v_diff = +1
		elif c == ord( 'E' ) or c == ord( 'J' ):
			v_diff = +1
			past = True
		elif c == ord( 'K' ) or c == ord( 'Y' ):
			v_diff = -1
			past = True
		elif c == ord( 'b' ):
			v_diff = -display_height
		elif c == ord( ' ' ) or c == ord( 'f' ):
			v_diff = +display_height
		elif c == ord( 'd' ):
			v_diff = +display_height // 2
		elif c == ord( 'u' ):
			v_diff = -display_height // 2
		elif c == ord( 'g' ):
			v_offset = 0
		elif c == ord( 'G' ):
			v_offset = 0
			v_diff = sys.maxsize
		elif c == ord( 'F' ):
			v_end = not v_end
except KeyboardInterrupt:
	pass
finally:
	curses.nocbreak()
	stdscr.keypad( False )
	curses.echo()
	curses.endwin()
