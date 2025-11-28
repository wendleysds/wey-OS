#!/usr/bin/env python3

from fat import FATFS
import commands as cmd
import sys

def help():
	print(f"Usage: {sys.argv[0]} <DEVICE> <option> <args/flags> <SRC> <DST>")

	print("\tinit: format and initialize the FAT FS")

	print("\tcreat: create a file")
	print("\t\t-HI: Hidden attribute")
	print("\t\t-VO: Volume attribute")
	print("\t\t-SY: System attribute")
	print("\t\t-RO: Read only attribute")

	print("\tmkdir: create a directory")
	print("\t\t-HI: Hidden attribute")
	print("\t\t-VO: Volume attribute")
	print("\t\t-SY: System attribute")
	print("\t\t-RO: Read only attribute")

	print("\trm: remove a directory or file")
	print("\t\t-f: force delete")

	print("\tls: list directory contents")

	print("\tread : read the file content in <path1>")
	print("\t\t-s: seek into the file")
	print("\t\t-l: the amount of bytes to print")
	print("\t\t-b: read the content in hex")

	print("\tstat: show file FAT Entry metadata")
	print("\t\t-c: seek into the file")
	
	print("\tstatfs: show FAT metadata")

	print("\tcp   : copy the file from <SRC> to <DST>")
	print("\t\t-ex: copy from your fs from <SRC> to the fat <DST>")


def main() -> int:
	if len(sys.argv) < 2:
		print(f"try {sys.argv[0]} --help")
		return

	if sys.argv[1].replace("-","").lower() in ["h", "help"]:
		help()
		return
	
	dev = None
	op = None
	args = []
	src = None
	dst = None

	dev = sys.argv[1]
	op = sys.argv[2].lower()
	if len(sys.argv) >= 3:
		for arg in sys.argv[3:]:
			if src is None:
				if arg.startswith(("/", "./")):
					src = arg
					continue
			if dst is None:
				if arg.startswith(("/", "./")):
					dst = arg
					continue
			args.append(arg.lower())

	operations = {
		"creat": cmd.creat,
		"mkdir": cmd.mkdir,
		"rm": cmd.rm,
		"ls": cmd.ls,
		"read": cmd.read,
		"cp": cmd.cp,
		"statfs": cmd.statfs,
		"stat": cmd.stat
	}

	if op == 'init':
		FATFS.format_file(None, dev)
		return 0

	if op not in operations:
		print(f"Invalid option! try {sys.argv[0]} --help")
		return 1
	
	fat = FATFS(dev)
	
	return operations[op](fat, args, src, dst)

if __name__ == "__main__":
	exit(main())