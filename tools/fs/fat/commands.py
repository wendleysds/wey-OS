from fat import FATFS, FATDirectoryEntry
import fat as attr

ATTRIBUTES = {
	'-hi': attr.HIDDEN,
	'-vo': attr.VOLUME,
	'-sy': attr.SYSTEM,
	'-ro': attr.READY_ONLY
}

def cluster_to_lba(fat: FATFS, cluster: int) -> int:
    return fat.fat.firstDataSector + ((cluster - 2) * fat.fat.header_boot.secPerClus)

def get_cluster_entry(entry: FATDirectoryEntry) -> int:
    return (entry.fstClusHI << 16) | entry.fstClusLO


def _create_entry(fat: FATFS, src: str, args: list[str], entry_type: int) -> int:
	tokens = src.rstrip('/').split('/')
	name = tokens[-1]
	dirs = src[:-len('/' + name)]

	entry = fat.walk('/' if dirs == '' else dirs)
	if not entry:
		print("No such file or directory")
		return 1

	cluster = get_cluster_entry(entry)

	attrs = 0
	for arg in args:
		attrs |= ATTRIBUTES[arg]

	r = fat.create(cluster, name, entry_type | attrs)
	if r == -2:
		print("File exists")
		return 1

	if r == -1:
		print("FAT Full")
		return 1

	return 0

def creat(fat: FATFS, args: list[str], src: str, dst: str) -> int:
	return _create_entry(fat, src, args, attr.ARCHIVE)

def mkdir(fat: FATFS, args: list[str], src: str, dst: str) -> int:
	return _create_entry(fat, src, args, attr.DIRECTORY)

def rm(fat: FATFS, args: list[str], src: str, dst: str) -> int:
	tokens = src.rstrip('/').split('/')
	name = tokens[len(tokens)-1]
	dirs = src[:-len('/' + name)]

	if dirs == '':
		print("Invalid file")
		return 0

	entry = fat.walk('/' if dirs == '' else dirs)
	if not entry:
		print("No such file or directory")
		return 1

	cluster = get_cluster_entry(entry)
	target = fat.search(cluster, name)

	if not target:
		print("No such file or directory")
		return 1

	if target.attr & attr.DIRECTORY:
		if len(fat.ls(get_cluster_entry(target))) - 2 != 0:
			print("Directory not empty")
			return 1

	res = fat.remove(cluster, name)
	if res != 0:
		print("No such file or directory")
		return 1


def ls(fat: FATFS, args: list[str], src: str, dst: str) -> int:
	entry = fat.walk(src)

	if not entry:
		print("No such file or directory")
		return 1

	if not entry.attr & attr.DIRECTORY:
		print(src)
		return

	entries = fat.ls(get_cluster_entry(entry))

	for e in entries:
		name = e.name.decode().strip()
		if e.attr & attr.DIRECTORY:
			print(f"\033[1;34m{name}\033[0m/", end='')
		else:
			print(f"{name[:8].rstrip()}.{name[8:11].rstrip()}", end='')

		print("   ", end='')
	print('')

def read(fat: FATFS, args: list[str], src: str, dst: str) -> int: pass
def stat(fat: FATFS, args: list[str], src: str, dst: str) -> int:
	entry = fat.walk(src)
	if not entry:
		print("No such file or directory")
		return 1

	cluster = get_cluster_entry(entry)
	lba = cluster_to_lba(fat, cluster)
	off = (fat.fat.mbr.startLBA + lba) * fat.fat.header_boot.bytesPerSec
	print(entry)
	print(f"data LBA     :  {lba} - {off}:{hex(off)}")

def statfs(fat: FATFS, args: list[str], src: str, dst: str) -> int: pass
def cp(fat: FATFS, args: list[str], src: str, dst: str) -> int: pass
