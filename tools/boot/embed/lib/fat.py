from array import array
from dataclasses import dataclass
from io import BufferedRandom
import struct

UNUSED  =  0
EOF     =  0x0FFFFFF8
READY_ONLY     =  0x01
HIDDEN         =  0x02
SYSTEM         =  0x04
VOLUME         =  0x08
DIRECTORY      =  0x10
ARCHIVE        =  0x20
LONG_NAME      =  READY_ONLY | HIDDEN | SYSTEM | VOLUME
LONG_NAME_MASK =  READY_ONLY | HIDDEN | SYSTEM | VOLUME | DIRECTORY | ARCHIVE

@dataclass
class FATBootHeader:
	jmpBoot: bytes  # 3 bytes
	OEMName: bytes  # 8 bytes
	bytesPerSec: int
	secPerClus: int
	rsvdSecCnt: int
	numFATs: int
	rootEntCnt: int
	totSec16: int
	mediaType: int
	FATSz16: int
	secPerTrk: int
	numHeads: int
	hiddSec: int
	totSec32: int

	STRUCT_FORMAT = "<3s8sHBHBHHBHHHII"

	@classmethod
	def from_bytes(cls, data: bytes) -> "FATBootHeader":
		unpacked = struct.unpack(cls.STRUCT_FORMAT, data[:struct.calcsize(cls.STRUCT_FORMAT)])
		return cls(*unpacked)

	def to_bytes(self) -> bytes:
		return struct.pack(
			self.STRUCT_FORMAT,
			self.jmpBoot,
			self.OEMName,
			self.bytesPerSec,
			self.secPerClus,
			self.rsvdSecCnt,
			self.numFATs,
			self.rootEntCnt,
			self.totSec16,
			self.mediaType,
			self.FATSz16,
			self.secPerTrk,
			self.numHeads,
			self.hiddSec,
			self.totSec32
		)
	
@dataclass
class FAT32ExtendedHeader:
	FATSz32: int
	extFlags: int
	FSVer: int
	rootClus: int
	FSInfo: int
	BkBootSec: int
	reserved: bytes  # 12 bytes
	drvNum: int
	reserved1: int
	bootSig: int
	volID: int
	volLab: bytes  # 11 bytes
	filSysType: bytes  # 8 bytes

	STRUCT_FORMAT = "<IHHIHH12sBBB I 11s8s"

	@classmethod
	def from_bytes(cls, data: bytes) -> "FAT32ExtendedHeader":
		unpacked = struct.unpack(cls.STRUCT_FORMAT, data[:struct.calcsize(cls.STRUCT_FORMAT)])
		return cls(*unpacked)

	def to_bytes(self) -> bytes:
		return struct.pack(
			self.STRUCT_FORMAT,
			self.FATSz32,
			self.extFlags,
			self.FSVer,
			self.rootClus,
			self.FSInfo,
			self.BkBootSec,
			self.reserved,
			self.drvNum,
			self.reserved1,
			self.bootSig,
			self.volID,
			self.volLab,
			self.filSysType
		)
	
@dataclass
class FATDirectoryEntry:
	name: bytes  # 11 bytes
	attr: int
	NTRes: int
	crtTimeTenth: int
	crtTime: int
	crtDate: int
	lstAccDate: int
	fstClusHI: int
	wrtTime: int
	wrtDate: int
	fstClusLO: int
	fileSize: int

	STRUCT_FORMAT = "<11sBBBHHHHHHHI"

	@classmethod
	def from_bytes(cls, data: bytes) -> "FATDirectoryEntry":
		unpacked = struct.unpack(cls.STRUCT_FORMAT, data[:struct.calcsize(cls.STRUCT_FORMAT)])
		return cls(*unpacked)

	def to_bytes(self) -> bytes:
		return struct.pack(
			self.STRUCT_FORMAT,
			self.name,
			self.attr,
			self.NTRes,
			self.crtTimeTenth,
			self.crtTime,
			self.crtDate,
			self.lstAccDate,
			self.fstClusHI,
			self.wrtTime,
			self.wrtDate,
			self.fstClusLO,
			self.fileSize
		)
	
	def __str__(self):
		print(f'name         :  {self.name.decode()}')
		print(f'attr         :  ', end='')

		if self.attr & READY_ONLY:
			print("RONLY | ", end='')
		if self.attr & HIDDEN:
			print("Hidden | ", end='')
		if self.attr & SYSTEM:
			print("System | ", end='')
		if self.attr & VOLUME:
			print("Volume | ", end='')
		if self.attr & DIRECTORY:
			print("Directory")
		if self.attr & ARCHIVE:
			print("Archive")

		print(f'NTRes        :  {self.NTRes}')
		print(f'crtTimeTenth :  {self.crtTimeTenth}')
		print(f'crtTime      :  {self.crtTime}')
		print(f'crtDate      :  {self.crtDate}')
		print(f'lstAccDate   :  {self.lstAccDate}')
		print(f'fstClusHI    :  {self.fstClusHI}')
		print(f'wrtTime      :  {self.wrtTime}')
		print(f'wrtDate      :  {self.wrtDate}')
		print(f'fstClusLO    :  {self.fstClusLO}')
		print(f'fileSize     :  {self.fileSize}', end='')
		return ''
	
@dataclass
class FAT32FSInfo:
	leadSignature: int
	reserved1: bytes  # 480 bytes
	structSignature: int
	freeClusterCount: int
	nextFreeCluster: int
	reserved2: bytes  # 12 bytes
	trailSignature: int

	STRUCT_FORMAT = "<I480sIII12sI"

	@classmethod
	def from_bytes(cls, data: bytes) -> "FAT32FSInfo":
		unpacked = struct.unpack(cls.STRUCT_FORMAT, data[:struct.calcsize(cls.STRUCT_FORMAT)])
		return cls(*unpacked)

	def to_bytes(self) -> bytes:
		return struct.pack(
			self.STRUCT_FORMAT,
			self.leadSignature,
			self.reserved1,
			self.structSignature,
			self.freeClusterCount,
			self.nextFreeCluster,
			self.reserved2,
			self.trailSignature
		)
	
class FAT:
	from lib.mbr import MBREntry
	header_boot: FATBootHeader
	mbr: MBREntry
	dev: BufferedRandom
	table: array
	totalClusters: int
	firstDataSector: int
	clusterSize: int

	def lba_to_sector(self, lba):
		return (self.mbr.startLBA + lba) * self.header_boot.bytesPerSec

	def __init__(self, header_boot, dev, mbr, table, totalClusters, firstDataSector, clusterSize):
		self.header_boot = header_boot
		self.dev = dev
		self.mbr = mbr
		self.table = table
		self.totalClusters = totalClusters
		self.firstDataSector = firstDataSector
		self.clusterSize = clusterSize

	def update(): raise NotImplementedError
	def next_cluster(self, current: int) -> int: raise NotImplementedError
	def free_cluster(self) -> int: raise NotImplementedError
	def append_cluster(self, cluster: int ) -> int: raise NotImplementedError
	def free_chain(self, start: int) -> int: raise NotImplementedError


class FAT32(FAT):
	header_extended: FAT32ExtendedHeader
	fsInfo: FAT32FSInfo

	def __init__(self, header_boot, dev, mbr, table, totalClusters, firstDataSector, clusterSize, fsinfo, header_extended):
		super().__init__(header_boot, dev, mbr, table, totalClusters, firstDataSector, clusterSize)
		self.fsInfo = fsinfo
		self.header_extended = header_extended

	def update(self):
		fatStartSector = self.header_boot.rsvdSecCnt
		f = self.dev
		
		if self.fsInfo.freeClusterCount != -1 and self.fsInfo.nextFreeCluster != -1:
			f.seek(self.lba_to_sector(self.header_extended.FSInfo))
			f.write(self.fsInfo.to_bytes())

		fatStartSector = self.header_boot.rsvdSecCnt

		f.seek(self.lba_to_sector(fatStartSector))
		f.write(struct.pack("<" + "I" * len(self.table), *self.table))

	def next_cluster(self, current: int) -> int:
		next = self.table[current]
		return next & 0x0FFFFFFF
	
	def free_cluster(self) -> int:
		if self.fsInfo.nextFreeCluster >= self.header_extended.rootClus and self.fsInfo.nextFreeCluster < self.totalClusters:
			cluster = self.fsInfo.nextFreeCluster
			for cluster in range(cluster, self.totalClusters):
				if (self.table[cluster] & 0x0FFFFFFF) == UNUSED:
					return cluster
				
		cluster = self.header_extended.rootClus
		for cluster in range(cluster, self.totalClusters):
			if (self.table[cluster] & 0x0FFFFFFF) == UNUSED:
				return cluster
			
		return EOF

	def append_cluster(self, cluster: int ) -> int:
		findedCluster = self.free_cluster()

		if findedCluster == -1:
			return -1
		
		self.table[cluster] = findedCluster
		self.table[findedCluster] = EOF

		if self.fsInfo.nextFreeCluster != -1:
			self.fsInfo.nextFreeCluster = findedCluster + 1

		if self.fsInfo.freeClusterCount != -1:
			self.fsInfo.freeClusterCount -= 1
		
		return findedCluster

	def free_chain(self, start: int) -> int:
		count = 0
		cur = start
		while not cur >= EOF:
			if 0 <= cur or cur > len(self.table):
				break # out fat limits

			next = self.table[cur] & 0x0FFFFFFF
			self.table[cur] = UNUSED
			cur = next
			count += 1

		if self.fsInfo.freeClusterCount != -1:
			self.fsInfo.freeClusterCount += count
		
		if self.fsInfo.nextFreeCluster != -1:
			self.fsInfo.nextFreeCluster = start

		return count

class FATFD:
    entry: FATDirectoryEntry
    firstCluster: int
    currentCluster: int
    dirCluster: int
    cursor: int

    def __init__(self, entry: FATDirectoryEntry, dirCluster: int):
        cluster = (entry.fstClusHI << 16) | entry.fstClusLO
        self.entry = entry
        self.firstCluster = cluster
        self.currentCluster = cluster
        self.cursor = 0
        self.dirCluster = dirCluster

def fat_format_name(filename: str) -> str:
    out = [' '] * 11

    if not filename or len(filename) == 0:
        return '\0'
    
    dot = filename.rfind('.')

    if len(filename) > 11:
        out[:8] = list(filename[:8])    
        if dot != -1 and dot + 1 < len(filename):
            ext = filename[dot + 1:]
            if len(ext) > 3:
                ext = ext[-3:]
            out[8:11] = list(ext.ljust(3))
        else:
            out[8:11] = list('   ')

        out[6] = '~'
        out[7] = '1'

        return ''.join(out).upper()
    
    name_length = 0
    ext_length = 0

    p = 0
    while p < len(filename) and (dot == -1 or p < dot) and name_length < 8:
        out[name_length] = filename[p]
        name_length += 1
        p += 1

    if dot != -1 and dot + 1 < len(filename):
        ext = filename[dot + 1:]
        while ext_length < len(ext) and ext_length < 3:
            out[8 + ext_length] = ext[ext_length]
            ext_length += 1

    return ''.join(out).upper()

class FATFS:
	fat: FAT

	def __init__(self, device: BufferedRandom | str, startClusters = 3000):
		from lib.mbr import MBREntry
		
		f = open(device, '+rb') if isinstance(device, str) else device
		f.seek(0)
		content = f.read(512)
		mbr = MBREntry.from_bytes(content[0x1BE:(0x1BE+16)])
		
		f.seek(mbr.startLBA * 512)
		content = f.read(512)

		hBoot = FATBootHeader.from_bytes(content[0:36])
		hExtend = FAT32ExtendedHeader.from_bytes(content[36:90])

		fatStartSector = hBoot.rsvdSecCnt
		fatSize = hExtend.FATSz32
		fatBytes = fatSize * hBoot.bytesPerSec

		fatTotalClusters = fatSize * hBoot.bytesPerSec / 4
		firstDataSector = fatStartSector + (hBoot.numFATs * fatSize)

		f.seek((mbr.startLBA + fatStartSector) * hBoot.bytesPerSec)
		table = array("I")

		table.frombytes(f.read(fatBytes))

		f.seek((mbr.startLBA + hExtend.FSInfo) * hBoot.bytesPerSec)
		content = f.read(512)

		fsInfo = FAT32FSInfo.from_bytes(content)
		if fsInfo.leadSignature != 0x41615252 or fsInfo.structSignature != 0x61417272 or fsInfo.trailSignature != 0xAA550000:
			print("FSInfo invalid signature or Missing!")
			fsInfo.freeClusterCount = -1
			fsInfo.nextFreeCluster = -1
		else:
			if fsInfo.freeClusterCount == 0xFFFFFFFF:
				fsInfo.freeClusterCount = int(fatTotalClusters)

			if fsInfo.nextFreeCluster == 0xFFFFFFFF:
				fsInfo.nextFreeCluster = int(startClusters)

		self.fat = FAT32(
			hBoot,
			f,
			mbr,
			table,
			int(fatTotalClusters),
			int(firstDataSector),
			int(hBoot.bytesPerSec * hBoot.secPerClus),
			fsInfo,
			hExtend,
		)

	def cluster_to_lba(self, cluster: int):
		fat = self.fat
		return fat.firstDataSector + ((cluster - 2) * fat.header_boot.secPerClus)

	def ls(self, dirCluster: int) -> list[FATDirectoryEntry]:
		fat = self.fat
		entries = []
		f = self.fat.dev

		cluster = dirCluster
		lba = self.cluster_to_lba(cluster)
		f.seek(self.fat.lba_to_sector(lba))
		for _ in range(int(fat.clusterSize / 32)):
			data = f.read(32)
			entry = FATDirectoryEntry.from_bytes(data)

			if entry.name[0] == 0x0:
				return entries

			if entry.name[0] == 0xE5:
				continue

			entries.append(entry)

		cluster = self.fat.next_cluster(cluster)
		if cluster >= EOF:
			return entries
			
	def search(self, dirCluster: int, name: str, formatName: bool = True) -> FATDirectoryEntry | None:
		entries = self.ls(dirCluster)

		if len(entries) == 0:
			return None

		if formatName:
			name = fat_format_name(name)

		for entry in entries:
			if entry.name.decode() == name:
				return entry

		return None

	def create(self, dirCluster: int, name: str, attr: int) -> int:
		fat = self.fat
		name = fat_format_name(name)
		f = self.fat.dev
		while True:
			cluster = dirCluster
			lba = self.cluster_to_lba(cluster)
			f.seek(self.fat.lba_to_sector(lba))
			for _ in range(int(fat.clusterSize / 32)):
				data = f.read(32)
				entry = FATDirectoryEntry.from_bytes(data)

				if entry.name.decode(errors='ignore') == name:
					return -2

				if entry.name[0] != 0x0 and entry.name[0] != 0xE5:
					continue

				cluster = self.fat.free_cluster()
				fat.table[cluster] = EOF

				entry.name = name.encode()
				entry.attr = attr
				
				entry.fstClusHI = cluster >> 16
				entry.fstClusLO = cluster
				entry.fileSize = 0

				f.seek(-32, 1)
				f.write(entry.to_bytes())

				if attr & 0x10:
					lba = self.cluster_to_lba(cluster)
					f.seek(self.fat.lba_to_sector(lba))
					dot = entry

					dot.name = '.          '.encode()
					f.write(dot.to_bytes())

					dot.name = '..         '.encode()
					dot.fstClusHI = dirCluster >> 16
					dot.fstClusLO = dirCluster
					f.write(dot.to_bytes())

				self.fat.update()
				return 0

			cluster = self.fat.append_cluster(cluster)
			if cluster == -1:
				return -1

	def remove(self, dirCluster: int, name: str) -> int:
		fat = self.fat
		name = fat_format_name(name)
		f = self.fat.dev
		while True:
			cluster = dirCluster
			lba = self.cluster_to_lba(cluster)
			f.seek(self.fat.lba_to_sector(lba))
			for _ in range(int(fat.clusterSize / 32)):
				data = f.read(32)
				entry = FATDirectoryEntry.from_bytes(data)

				if entry.name[0] == 0x0:
					return -2

				if entry.name[0] == 0xE5:
					continue

				if entry.name.decode() != name:
					continue

				newName = bytearray(entry.name)
				newName[0] = 0xE5

				entry.name = bytes(newName)
				self.fat.free_chain((entry.fstClusHI << 16) | entry.fstClusLO)

				f.seek(-32, 1)
				f.write(entry.to_bytes())
				self.fat.update()
				return 0

			cluster = self.fat.next_cluster(cluster)
			if cluster >= EOF:
				return -1

	def format_file(self, device: BufferedRandom | str, createFsInfo: bool = True):
		f = open(device, '+rb') if isinstance(device, str) else device

		from lib.mbr import MBREntry

		f.seek(0)
		sec0 = f.read(512)
		mbr = MBREntry.from_bytes(sec0[0x1BE:(0x1BE+16)])

		# calc totsec32
		hboot = FATBootHeader(
			jmpBoot=b'\xEB\x58\x90', # jmp short + nop
			OEMName=b'MSWIN4.1',
			bytesPerSec=512,
			secPerClus=8,
			rsvdSecCnt=32,
			numFATs=2,
			rootEntCnt=0,
			totSec16=0,
			mediaType=0xF8,
			FATSz16=0,
			secPerTrk=63,
			numHeads=16,
			hiddSec=mbr.startLBA,
			totSec32= mbr.sizeInLBA
		)

		hextended = FAT32ExtendedHeader(
			FATSz32=0,
			extFlags=0,
			FSVer=0,
			rootClus=2,
			FSInfo=1,
			BkBootSec=6,
			reserved=b'\x00' * 12,
			drvNum=0x80,
			reserved1=0,
			bootSig=0x29,
			volID=0xCAFE,
			volLab=b'WEY-OS VOL ',
			filSysType=b'FAT32   '
		)

		# RootDirSectors = ((BPB_RootEntCnt * 32) + (BPB_BytsPerSec – 1)) / BPB_BytsPerSec;
		# TmpVal1 = DskSize – (BPB_ResvdSecCnt + RootDirSectors);
		# TmpVal2 = (256 * BPB_SecPerClus) + BPB_NumFATs;
		# If(FATType == FAT32)
		# TmpVal2 = TmpVal2 / 2;
		# FATSz = (TMPVal1 + (TmpVal2 – 1)) / TmpVal2;

		# If(FATType == FAT32) {
			# BPB_FATSz16 = 0;
			# BPB_FATSz32 = FATSz;
		# } else {
			# BPB_FATSz16 = LOWORD(FATSz);
			# /* there is no BPB_FATSz32 in a FAT16 BPB */
		# }

		tmpVal1 = mbr.sizeInLBA - hboot.rsvdSecCnt
		tmpVal2 = (256 * hboot.secPerClus) + hboot.numFATs
		FATSz = (tmpVal1 + (tmpVal2 - 1)) // tmpVal2

		hextended.FATSz32 = FATSz

		fatStartSector = hboot.rsvdSecCnt
		fatSize = hextended.FATSz32
		fatBytes = fatSize * hboot.bytesPerSec

		table = bytearray(fatBytes)

		table[0:4]  = b'\xF8\xFF\xFF\x0F'  # cluster 0
		table[4:8]  = b'\xFF\xFF\xFF\x0F'  # cluster 1
		table[8:12] = b'\xFF\xFF\xFF\x0F'  # cluster 2 (root)

		def _SEC(sector):
			return (sector + mbr.startLBA) * hboot.bytesPerSec
		
		f.seek(_SEC(0))
		boot_sector = bytearray(512)
		boot_sector[0:90] = hboot.to_bytes() + hextended.to_bytes()
		boot_sector[510:512] = b'\x55\xAA'

		f.write(boot_sector)

		f.seek(_SEC(fatStartSector))
		f.write(table)

		f.seek(_SEC(fatStartSector + fatSize))
		f.write(table)

		if hextended.BkBootSec != 0:
			f.seek(_SEC(0))
			secbuffer = f.read(512)

			f.seek(_SEC(hextended.BkBootSec))
			f.write(secbuffer)

		if createFsInfo:
			fsinfo = FAT32FSInfo(
				leadSignature=0x41615252,
				reserved1=b"\x00" * 480,
				structSignature=0x61417272,
				freeClusterCount=0xFFFFFFFF,
				nextFreeCluster=0xFFFFFFFF,
				reserved2=b"\x00" * 12,
				trailSignature=0xAA550000
			)

			f.seek(_SEC(hextended.FSInfo))
			f.write(fsinfo.to_bytes())

	def walk(self, path: str) -> FATDirectoryEntry | None:
		itens = path.split("/")

		cluster = self.fat.header_extended.rootClus

		if len(path) == 1:
			root = FATDirectoryEntry(0,0,0,0,0,0,0,0,0,0,0,0)
			root.name = b'ROOT       '
			root.attr = DIRECTORY | SYSTEM
			root.fstClusHI = (cluster << 16) & 0xFFFF
			root.fstClusLO = cluster & 0xFFFF
			return root
		else:
			itens.remove('')

		entry = None
		for s in itens:
			name = fat_format_name(s)

			if s == '..':
				name = '..         '
			elif s == '.':
				name = '.          '

			if s == '':
				continue

			if entry:
				if not entry.attr & DIRECTORY:
					print(entry)
					return None

			entry = self.search(cluster, name, False)
			if not entry:
				return None
			
			cluster = (entry.fstClusHI << 16) | entry.fstClusLO

		return entry

	def __entry_lba(self, dirCluster: int, entry: FATDirectoryEntry) -> int:
		fat = self.fat
		lba = self.cluster_to_lba(dirCluster)
		entrySize = 32
		entryIndex = 0

		f = self.fat.dev

		f.seek(self.fat.lba_to_sector(lba))
		for _ in range(int(fat.clusterSize / entrySize)):
			data = f.read(entrySize)
			if not data:
				break

			currentEntry = FATDirectoryEntry.from_bytes(data)
			if currentEntry.name.decode() == entry.name.decode():
				return self.fat.lba_to_sector(lba) + (entryIndex * entrySize)

			entryIndex += 1

		return -1

	def open(self, path: str) -> FATFD | None:
		tokens = path.rstrip('/').split('/')
		name = tokens[len(tokens)-1]
		dirs = path[:-len('/' + name)]

		entry = self.walk('/' if dirs == '' else dirs)
		if not entry:
			return None
		
		cluster = (entry.fstClusHI << 16) | entry.fstClusLO
		target = self.search(cluster, name)

		if not target:
			return None
		
		return FATFD(target, cluster)

	def seek(self, fd: FATFD, offset: int, whence: int = 0) -> int:
		filesize = fd.entry.fileSize
		if whence == 0:
			target = offset
		elif whence == 1:
			target = fd.cursor + offset
		elif whence == 2:
			target = filesize + offset
		else:
			return -1
		
		if target > filesize:
			return -1
		
		clusterOffset = int(target / self.fat.clusterSize)

		cluster = fd.firstCluster
		for _ in range(clusterOffset):
			cluster = self.fat.next_cluster(cluster)
			if cluster >= EOF:
				return -1
			
		fd.currentCluster = cluster
		fd.cursor = target

		return filesize - target

	def read(self, fd: FATFD, count: int) -> bytes:
		cursor = fd.cursor
		lba = self.cluster_to_lba(fd.currentCluster)

		remaining = count
		totalReaded = 0

		if fd.cursor == fd.entry.fileSize:
			return b''

		clusterOffset = cursor % self.fat.clusterSize
		bytesLeftInCluster = self.fat.clusterSize - clusterOffset

		content = bytearray()

		def _SEC(x):
			return x * self.fat.header_boot.bytesPerSec
		
		f = self.fat.dev

		f.seek(_SEC(lba) + clusterOffset)
		while remaining > 0:
			toRead = remaining if remaining < bytesLeftInCluster else bytesLeftInCluster
			data = f.read(toRead)
			readed = len(data)

			content.extend(data)

			cursor += readed
			totalReaded += readed
			remaining -= readed
			bytesLeftInCluster -= readed

			if cursor % self.fat.clusterSize == 0:
				nextCluster = self.fat.next_cluster(fd.currentCluster)
				if nextCluster >= EOF:
					fd.cursor = fd.entry.fileSize
					return b''

		return bytes(content)
		
	def write(self, fd: FATFD, buffer: str | bytes, count: int) -> int:
		cursor = fd.cursor
		lba = self.cluster_to_lba(fd.currentCluster)

		remaining = count
		totalWritten = 0

		clusterOffset = cursor % self.fat.clusterSize
		bytesLeftInCluster = self.fat.clusterSize - clusterOffset

		def _SEC(x):
			return x * self.fat.header_boot.bytesPerSec
		
		f = self.fat.dev

		f.seek(_SEC(lba) + clusterOffset)
		while remaining > 0:
			toWrite = remaining if remaining < bytesLeftInCluster else bytesLeftInCluster
			writed = f.write(buffer[totalWritten:totalWritten + toWrite])

			cursor += writed
			totalWritten += writed
			remaining -= writed
			bytesLeftInCluster -= writed

			if cursor % self.fat.clusterSize == 0:
				nextCluster = self.fat.next_cluster(fd.currentCluster)
				if nextCluster >= EOF:
					nextCluster = self.fat.append_cluster(fd.currentCluster)
					if nextCluster < 0:
						return nextCluster

					fd.currentCluster = nextCluster

					clusterOffset = cursor % self.fat.clusterSize
					bytesLeftInCluster = self.fat.clusterSize - clusterOffset

					lba = self.cluster_to_lba(nextCluster)
					f.seek(_SEC(lba) + clusterOffset)

		fd.cursor = cursor

		if fd.cursor > fd.entry.fileSize:
			fd.entry.fileSize = fd.cursor

		f.seek(self.__entry_lba(fd.dirCluster, fd.entry), 0)
		f.write(fd.entry.to_bytes())

		self.fat.update()
		
		return totalWritten
