from fat import FATFS
import sys

def help():
	print(f"Usage: {sys.argv[0]} <option> <args/flags> <SRC> <DST>")

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
	print("\tstatfs: show FAT metadata")

	print("\tcp   : copy the file from <path1>(src) to <path2>(dst)")
	print("\t\t-ex: copy a external file in <path1>(local) to <path2>(img fs)")


def main() -> int:
	if len(sys.argv) < 2:
		print(f"try {sys.argv[0]} --help")
		return

	if sys.argv[1].replace("-","").lower() in ["h", "help"]:
		help()
		return
	
	op = None
	args = []
	src = None
	dst = None

	argv = sys.argv[1:]
	if argv:
		op = argv[0]
		rest = argv[1:]
	else:
		rest = []
	
	return 0

if __name__ == "__main__":
	exit(main())