#!/usr/bin/env python3.8
from __future__ import annotations # for using '|' instead of Union() on less than version 3.10

import sys
import subprocess

BEEP_PATH             = 'beep-msx.wav'

# /////////////////////////////////////////////////////////////
# // utility

def beep() -> None: # TODO: change command 'mpv' to 'aplay' after making wave data lauding more
	output = subprocess.run('mpv --volume=200 {0}'.format(BEEP_PATH), shell=True, capture_output=True, text=True)

def remove_file(file_path: str) -> None:
	output = subprocess.run('rm -f {0}'.format(file_path), shell=True, capture_output=True, text=True)

# /////////////////////////////////////////////////////////////
# // helper (exception wrapper)

def read_binary_file(file_path: str) -> bytes|None:
	try:
		with open(file_path, 'rb') as file:
			binary = file.read()
		return binary
	except FileNotFoundError:
		print('{0}: file not found'.format(file_path), file=sys.stderr)
		return None
	except Exception as e:
		print('{0}: {e}: file cannot open'.format(file_path), file=sys.stderr)
		return None

def write_binary_file(file_path: str, binary: bytes) -> bool:
	try:
		with open(file_path, 'wb') as file:
			file.write(binary)
		return True
	except Exception as e:
		print('{0}: {1}: file cannot written'.format(file_path, e), file=sys.stderr)
		return False

def append_binary_file(file_path: str, binary: bytes) -> bool:
	try:
		with open(file_path, 'ab') as file:
			file.write(binary)
		return True
	except Exception as e:
		print('{0}: {1}: file cannot written'.format(file_path, e), file=sys.stderr)
		return False

def read_text_file(file_path: str) -> str|None:
	try:
		with open(file_path, 'r') as file:
			text = file.read()
		return text
	except FileNotFoundError:
		print('{0}: file not found'.format(file_path), file=sys.stderr)
		return None
	except Exception as e:
		print('{0}: {e}: file cannot open'.format(file_path), file=sys.stderr)
		return None
