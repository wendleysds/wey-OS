from dataclasses import dataclass
from typing import List

from lib.fat.namegen import fat_format_name
import lib.fat.attributes as attr

import struct
import array

EOF     =  0x0FFFFFF8
UNUSED  =  0

@dataclass
class FAT32HeaderBoot:
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
    def from_bytes(cls, data: bytes) -> "FAT32HeaderBoot":
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
class FAT32HeaderExtended:
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
    def from_bytes(cls, data: bytes) -> "FAT32HeaderExtended":
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
class FAT32DirectoryEntry:
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
    def from_bytes(cls, data: bytes) -> "FAT32DirectoryEntry":
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

        if self.attr & attr.READY_ONLY:
            print("RONLY | ", end='')
        if self.attr & attr.HIDDEN:
            print("Hidden | ", end='')
        if self.attr & attr.SYSTEM:
            print("System | ", end='')
        if self.attr & attr.VOLUME:
            print("Volume | ", end='')
        if self.attr & attr.DIRECTORY:
            print("Directory")
        if self.attr & attr.ARCHIVE:
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

@dataclass
class FATLongDirectoryEntry:
    LDIR_Ord: int
    LDIR_Name1: List[int]  # 5 * uint16_t
    LDIR_Attr: int
    LDIR_Type: int
    LDIR_Chksum: int
    LDIR_Name2: List[int]  # 6 * uint16_t
    LDIR_FstClusLO: int
    LDIR_Name3: List[int]  # 2 * uint16_t

    STRUCT_FORMAT = "<B5HBBB6HH2H"

    @classmethod
    def from_bytes(cls, data: bytes) -> "FATLongDirectoryEntry":
        unpacked = struct.unpack(cls.STRUCT_FORMAT, data[:struct.calcsize(cls.STRUCT_FORMAT)])
        LDIR_Ord = unpacked[0]
        LDIR_Name1 = list(unpacked[1:6])
        LDIR_Attr = unpacked[6]
        LDIR_Type = unpacked[7]
        LDIR_Chksum = unpacked[8]
        LDIR_Name2 = list(unpacked[9:15])
        LDIR_FstClusLO = unpacked[15]
        LDIR_Name3 = list(unpacked[16:18])
        return cls(
            LDIR_Ord,
            LDIR_Name1,
            LDIR_Attr,
            LDIR_Type,
            LDIR_Chksum,
            LDIR_Name2,
            LDIR_FstClusLO,
            LDIR_Name3
        )

    def to_bytes(self) -> bytes:
        return struct.pack(
            self.STRUCT_FORMAT,
            self.LDIR_Ord,
            *self.LDIR_Name1,
            self.LDIR_Attr,
            self.LDIR_Type,
            self.LDIR_Chksum,
            *self.LDIR_Name2,
            self.LDIR_FstClusLO,
            *self.LDIR_Name3
        )

@dataclass
class FATHeaders:
    boot: FAT32HeaderBoot
    extended: FAT32HeaderExtended

@dataclass
class FAT32:
    headers: FATHeaders
    fsInfo: FAT32FSInfo
    table: array.array
    totalClusters: int
    firstDataSector: int
    clusterSize: int
    image_path: str

    def update(self):
        fatStartSector = self.headers.boot.rsvdSecCnt
        sectorSize = self.headers.boot.bytesPerSec
        with open(self.image_path, "+rb") as f:
            if self.fsInfo.freeClusterCount != -1 and self.fsInfo.nextFreeCluster != -1:
                f.seek(int(self.headers.extended.FSInfo * sectorSize))
                f.write(self.fsInfo.to_bytes())

            fatStartSector = self.headers.boot.rsvdSecCnt

            f.seek(int(fatStartSector * sectorSize), 0)
            f.write(struct.pack("<" + "I" * len(self.table), *self.table))

    def next_cluster(self, current: int) -> int:
        next = self.table[current]
        return next & 0x0FFFFFFF

    def next_free_cluster(self) -> int:
        if self.fsInfo.nextFreeCluster >= self.headers.extended.rootClus and self.fsInfo.nextFreeCluster < self.totalClusters:
            cluster = self.fsInfo.nextFreeCluster
            for cluster in range(cluster, self.totalClusters):
                if (self.table[cluster] & 0x0FFFFFFF) == UNUSED:
                    return cluster
                
        cluster = self.headers.extended.rootClus
        for cluster in range(cluster, self.totalClusters):
            if (self.table[cluster] & 0x0FFFFFFF) == UNUSED:
                return cluster
            
        return -1

    def append_cluster(self, cluster: int ) -> int:
        findedCluster = self.next_free_cluster()

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
    
@dataclass
class FATFD:
    entry: FAT32DirectoryEntry
    firstCluster: int
    currentCluster: int
    dirCluster: int
    cursor: int

    def __init__(self, entry: FAT32DirectoryEntry, dirCluster: int):
        cluster = (entry.fstClusHI << 16) | entry.fstClusLO
        self.entry = entry
        self.firstCluster = cluster
        self.currentCluster = cluster
        self.cursor = 0
        self.dirCluster = dirCluster

class FATFS:
    fat: FAT32

    def __init__(self, img_path, startClusters = 3000):
        with open(img_path, "+rb") as f:
            content = f.read(512)
            hBoot = FAT32HeaderBoot.from_bytes(content[0:36])
            hExtend = FAT32HeaderExtended.from_bytes(content[36:90])

            fatStartSector = hBoot.rsvdSecCnt
            fatSize = hExtend.FATSz32
            fatBytes = fatSize * hBoot.bytesPerSec

            fatTotalClusters = fatSize * hBoot.bytesPerSec / 4
            firstDataSector = fatStartSector + (hBoot.numFATs * fatSize)

            f.seek(fatStartSector * hBoot.bytesPerSec, 0)
            table = array.array("I")
            table.frombytes(f.read(fatBytes))

            f.seek(hExtend.FSInfo * hBoot.bytesPerSec, 0)
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
                FATHeaders(hBoot, hExtend),
                fsInfo,
                table,
                int(fatTotalClusters),
                int(firstDataSector),
                int(hBoot.bytesPerSec * hBoot.secPerClus),
                img_path
            )

    def __cluster_to_lba(self, cluster: int):
        fat = self.fat
        return fat.firstDataSector + ((cluster - 2) * fat.headers.boot.secPerClus)

    def ls(self, dirCluster: int) -> list[FAT32DirectoryEntry]:
        fat = self.fat
        entries = []
        with open(fat.image_path, "+rb") as f:
            cluster = dirCluster
            lba = self.__cluster_to_lba(cluster)
            f.seek(lba * fat.headers.boot.bytesPerSec)
            for _ in range(int(fat.clusterSize / 32)):
                data = f.read(32)
                entry = FAT32DirectoryEntry.from_bytes(data)

                if entry.name[0] == 0x0:
                    return entries

                if entry.name[0] == 0xE5:
                    continue

                entries.append(entry)

            cluster = self.fat.next_cluster(cluster)
            if cluster >= EOF:
                return entries
            
    def search(self, dirCluster: int, name: str, formatName: bool = True) -> FAT32DirectoryEntry | None:
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
        with open(fat.image_path, "+rb") as f:
            while True:
                cluster = dirCluster
                lba = self.__cluster_to_lba(cluster)
                f.seek(lba * fat.headers.boot.bytesPerSec)
                for _ in range(int(fat.clusterSize / 32)):
                    data = f.read(32)
                    entry = FAT32DirectoryEntry.from_bytes(data)

                    if entry.name.decode(errors='ignore') == name:
                        return -2

                    if entry.name[0] != 0x0 and entry.name[0] != 0xE5:
                        continue

                    cluster = self.fat.next_free_cluster()
                    fat.table[cluster] = EOF

                    entry.name = name.encode()
                    entry.attr = attr
                    
                    entry.fstClusHI = cluster >> 16
                    entry.fstClusLO = cluster
                    entry.fileSize = 0

                    f.seek(-32, 1)
                    f.write(entry.to_bytes())

                    if attr & 0x10:
                        lba = self.__cluster_to_lba(cluster)
                        f.seek(lba * fat.headers.boot.bytesPerSec)
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
        with open(fat.image_path, "+rb") as f:
            while True:
                cluster = dirCluster
                lba = self.__cluster_to_lba(cluster)
                f.seek(lba * fat.headers.boot.bytesPerSec)
                for _ in range(int(fat.clusterSize / 32)):
                    data = f.read(32)
                    entry = FAT32DirectoryEntry.from_bytes(data)

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

    def format_file(self, img_path, createFsInfo: bool = False):
        with open(img_path, "+rb") as f:
            content = f.read(512)
            hboot = FAT32HeaderBoot.from_bytes(content[0:36])
            hextended = FAT32HeaderExtended.from_bytes(content[36:90])

            fatStartSector = hboot.rsvdSecCnt
            fatSize = hextended.FATSz32
            fatBytes = fatSize * hboot.bytesPerSec

            table = bytearray(fatBytes)

            table[0:4]  = b'\xF8\xFF\xFF\x0F'  # cluster 0
            table[4:8]  = b'\xFF\xFF\xFF\x0F'  # cluster 1
            table[8:12] = b'\xFF\xFF\xFF\x0F'  # cluster 2 (root)

            def _SEC(sector):
                return sector * hboot.bytesPerSec

            f.seek(_SEC(fatStartSector))
            f.write(table)

            f.seek(_SEC(fatStartSector + fatSize))
            f.write(table)

            if hextended.BkBootSec != 0:
                f.seek(0)
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

    def walk(self, path: str) -> FAT32DirectoryEntry | None:
        itens = path.split("/")

        cluster = self.fat.headers.extended.rootClus

        if len(path) == 1:
            root = FAT32DirectoryEntry(0,0,0,0,0,0,0,0,0,0,0,0)
            root.name = b'ROOT       '
            root.attr = attr.DIRECTORY | attr.SYSTEM
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
                if not entry.attr & attr.DIRECTORY:
                    print(entry)
                    return None

            entry = self.search(cluster, name, False)
            if not entry:
                return None
            
            cluster = (entry.fstClusHI << 16) | entry.fstClusLO

        return entry
    
    def __entry_lba(self, dirCluster: int, entry: FAT32DirectoryEntry) -> int:
        fat = self.fat
        lba = self.__cluster_to_lba(dirCluster)
        entrySize = 32
        entryIndex = 0

        with open(fat.image_path, "+rb") as f:
            f.seek(lba * fat.headers.boot.bytesPerSec)
            for _ in range(int(fat.clusterSize / entrySize)):
                data = f.read(entrySize)
                if not data:
                    break

                currentEntry = FAT32DirectoryEntry.from_bytes(data)
                if currentEntry.name.decode() == entry.name.decode():
                    return lba * fat.headers.boot.bytesPerSec + (entryIndex * entrySize)

                entryIndex += 1

        return -1
    
    def open(self, path: str) -> FATFD | None:
        tokens = path.rstrip('/').split('/')
        name = tokens[len(tokens)-1]
        dirs = path[:-len('/' + name)]

        if dirs == '':
            return None

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
        lba = self.__cluster_to_lba(fd.currentCluster)

        remaining = count
        totalReaded = 0

        if fd.cursor == fd.entry.fileSize:
            return b''

        clusterOffset = cursor % self.fat.clusterSize
        bytesLeftInCluster = self.fat.clusterSize - clusterOffset

        content = bytearray()

        def _SEC(x):
            return x * self.fat.headers.boot.bytesPerSec

        with open(self.fat.image_path, "+rb") as f:
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
        lba = self.__cluster_to_lba(fd.currentCluster)

        remaining = count
        totalWritten = 0

        clusterOffset = cursor % self.fat.clusterSize
        bytesLeftInCluster = self.fat.clusterSize - clusterOffset

        def _SEC(x):
            return x * self.fat.headers.boot.bytesPerSec

        with open(self.fat.image_path, "+rb") as f:
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

                        lba = self.__cluster_to_lba(nextCluster)
                        f.seek(_SEC(lba) + clusterOffset)

            fd.cursor = cursor

            if fd.cursor > fd.entry.fileSize:
                fd.entry.fileSize = fd.cursor

            f.seek(self.__entry_lba(fd.dirCluster, fd.entry), 0)
            f.write(fd.entry.to_bytes())

            self.fat.update()
            
            return totalWritten
    