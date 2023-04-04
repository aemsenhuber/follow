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
import curses
import socket
import subprocess
import argparse

parser = argparse.ArgumentParser()

parser.add_argument( "command", nargs = argparse.REMAINDER, help = "Command to watch" )
parser.add_argument( "--interval", "-n", type = int, default = 1, help = "Update interval time (seconds)" )
parser.add_argument( "--shell", "-s", action = "store_true", help = "Execute command through a shell" )

args = parser.parse_args()

title = socket.gethostname() + " " + args.command[0] + " output"

stdscr = curses.initscr()
curses.noecho()
curses.curs_set( 0 )
curses.cbreak()
stdscr.keypad( True )

v_offset = 0
h_offset = 0
v_end = False

try:
	while True:
		stdscr.erase()
		screen_height, screen_width = stdscr.getmaxyx()
		title_height = 1
		title_width = len( title )
		try:
			stdscr.addstr( 0, int( ( screen_width - title_width ) / 2. ), title, curses.A_REVERSE )
		except:
			pass

		# Size of the zone where the output of the command will be display
		# One line less because of the title
		display_height = screen_height - title_height
		display_width = screen_width

		# Execute the command
		if args.shell:
			process = subprocess.Popen( " ".join( args.command ), stdout = subprocess.PIPE, stderr = subprocess.STDOUT, shell = True )
		else:
			process = subprocess.Popen( args.command, stdout = subprocess.PIPE, stderr = subprocess.STDOUT )
		stdout, stderr = process.communicate()
		del process

		# Process the result
		res_lines = stdout.decode( "ascii" ).split( "\n" )

		# Size of the result in both directions
		res_max_height = len( res_lines )
		res_max_width = max( len( line ) for line in res_lines )

		if res_max_height <= display_height:
			v_offset = 0
			display_lines = res_lines
		else:
			if v_offset < 0:
				v_offset = 0
			elif v_offset + display_height > res_max_height or v_end:
				v_offset = res_max_height - display_height
			display_lines = res_lines[v_offset:v_offset+display_height]

		if res_max_width <= display_width:
			h_offset = 0
		else:
			if h_offset < 0:
				h_offset = 0
			elif h_offset + display_width > res_max_width:
				h_offset = res_max_width - display_width

		curline = 1
		for line in display_lines:
			if h_offset > 0:
				if len( line ) <= h_offset:
					line = ""
				else:
					line = line[h_offset:]
			if len( line ) > display_width:
				line = line[:display_width]
			try:
				stdscr.addstr( curline, 0, line )
			except:
				pass
			curline = curline + 1

		stdscr.refresh()

		# We want to refresh the window every interval;
		# this is done by waiting for a key to be pressed.
		# Once a key has been pressed, get the whole content from STDIN
		# without waiting again for one second.
		begin = True
		while True:
			if begin:
				curses.halfdelay( args.interval * 10 )
				stdscr.nodelay( False )
				begin = False
			else:
				stdscr.nodelay( True )

			# Get one character from STDIN
			c = stdscr.getch()

			# Handle key
			if c == ord( 'q' ):
				raise KeyboardInterrupt
			elif c == curses.KEY_LEFT:
				h_offset = h_offset - 1
			elif c == curses.KEY_RIGHT:
				h_offset = h_offset + 1
			elif c == curses.KEY_UP:
				v_offset = v_offset - 1
			elif c == curses.KEY_DOWN:
				v_offset = v_offset + 1
			elif c == ord( 'b' ):
				v_offset = v_offset - display_height
			elif c == ord( ' ' ):
				v_offset = v_offset + display_height
			elif c == ord( 'd' ):
				v_offset = v_offset + display_height // 2
			elif c == ord( 'u' ):
				v_offset = v_offset - display_height // 2
			elif c == ord( 'f' ):
				v_end = not v_end
			else:
				break
except KeyboardInterrupt:
	pass
finally:
	curses.nocbreak()
	stdscr.keypad( False )
	curses.echo()
	curses.endwin()


