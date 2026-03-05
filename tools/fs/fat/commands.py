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
	
	if('-c' in args):
		cluster_chain = []
		cur = cluster
		while 0x2 <= cur < 0x0FFFFFF8:
			cluster_chain.append(cur)
			next = fat.fat.next_cluster(cur)
			cur = next
		
		print(" -> ".join(map(str, cluster_chain)) + " -> [EOF]")
		return 0
	
	lba = cluster_to_lba(fat, cluster)
	off_rel = lba * fat.fat.header_boot.bytesPerSec
	off_rea = (fat.fat.mbr.startLBA + lba) * fat.fat.header_boot.bytesPerSec
	print(entry)
	print("data LBA")
	print(f"   relative: {lba} - {off_rel}:{hex(off_rel)}")
	print(f"   real: {fat.fat.mbr.startLBA + lba} - {off_rea}:{hex(off_rea)}")

	return 0

def statfs(fat: FATFS, args: list[str], src: str, dst: str) -> int: pass

def cp(fat: FATFS, args: list[str], src: str, dst: str) -> int:
	import os
	is_external = '-ex' in args

	if is_external:
		if not os.path.isfile(src):
			print("No such file or directory")
			return 1
	else:
		if not fat.walk(src):
			print("No such file or directory")
			return 1

	# dest is a folder        ./ext/foo.txt -> /home or /home/ (search and replace or create)
	# dest is a existent file ./ext/foo.txt -> /home/bar.txt (rewrite/replace)

	# create the destination if it does not exist
	dest_exists = fat.walk(dst)
	if not dest_exists:
		dirs = dst.removesuffix('/' + os.path.basename(dst))
		dir_entry = fat.walk(dirs if dirs != '' else '/')

		fat.create(get_cluster_entry(dir_entry), os.path.basename(dst), attr.ARCHIVE)

	# create the file if it does not exist
	if dest_exists and dest_exists.attr & attr.DIRECTORY:
		target_exists = fat.search(get_cluster_entry(dest_exists), os.path.basename(src))

		# create if not exists
		if not target_exists:
			fat.create(get_cluster_entry(dest_exists), os.path.basename(src), attr.ARCHIVE)

		dst = os.path.join(dst, os.path.basename(src))

	if is_external:
		with open(src, '+rb') as f:
			fd = fat.open(dst)

			if not fd:
				print("No such file or directory")
				return 1

			if fd.entry.fileSize > 0:
				fat.fat.free_chain(fd.firstCluster)
				fat.fat.table[fd.firstCluster] = 0x0FFFFFF8
				fd.entry.fileSize = 0

			data = f.read(fat.fat.clusterSize)
			while data:
				fat.write(fd, data, len(data))
				data = f.read(fat.fat.clusterSize)
	else:
		srcFd = fat.open(src)
		dstFd = fat.open(dst)

		if not srcFd or not dstFd:
			print("No such file or directory")
			return 1
		
		if dstFd.entry.fileSize > 0:
			fat.fat.free_chain(dstFd.firstCluster)
			fat.fat.table[dstFd.firstCluster] = 0x0FFFFFF8
			dstFd.entry.fileSize = 0

		data = fat.read(srcFd, fat.fat.clusterSize)
		while data:
			fat.write(dstFd, data, len(data))
			data = fat.read(srcFd, fat.fat.clusterSize)

	return 0
