from fat import FATFS, FATDirectoryEntry
import fat as attr

def cluster_to_lba(fat: FATFS, cluster: int) -> int:
    return fat.firstDataSector + ((cluster - 2) * fat.headers.boot.secPerClus)

def get_cluster_entry(entry: FATDirectoryEntry) -> int:
    return (entry.fstClusHI << 16) | entry.fstClusLO

def init(args: list[str], src: str, dst: str) -> int: pass

def creat(fat: FATFS, args: list[str], src: str, dst: str) -> int:
	tokens = src.rstrip('/').split('/')
	name = tokens[len(tokens)-1]
	dirs = src[:-len('/' + name)]

	entry = fat.walk('/' if dirs == '' else dirs)
	if not entry:
		return 1

	cluster = get_cluster_entry(entry)

	ATTRIBUTES = {
		'-HI': attr.HIDDEN,
		'-VO': attr.VOLUME,
		'-SY': attr.SYSTEM,
		'-RO': attr.READY_ONLY
	}

	attrs = 0
	for arg in args:
		attrs |= ATTRIBUTES[arg]

	r = fat.create(cluster, name, attr.ARCHIVE | attrs)
	if r == -2:
		print("File exists")
		return 1

	if r == -1:
		print("FAT Full")
		return 1

	return 0


def mkdir(args: list[str], src: str, dst: str) -> int: pass
def rm(fat: FATFS, args: list[str], src: str, dst: str) -> int: pass
def ls(fat: FATFS, args: list[str], src: str, dst: str) -> int: pass
def read(fat: FATFS, args: list[str], src: str, dst: str) -> int: pass
def stat(fat: FATFS, args: list[str], src: str, dst: str) -> int: pass
def statfs(fat: FATFS, args: list[str], src: str, dst: str) -> int: pass
def cp(fat: FATFS, args: list[str], src: str, dst: str) -> int: pass
