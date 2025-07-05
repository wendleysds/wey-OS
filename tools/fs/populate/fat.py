from dataclasses import dataclass
from typing import List
import array
import struct

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
    DIR_Name: bytes  # 11 bytes
    DIR_Attr: int
    NTRes: int
    DIR_CrtTimeTenth: int
    DIR_CrtTime: int
    DIR_CrtDate: int
    DIR_LstAccDate: int
    DIR_FstClusHI: int
    DIR_WrtTime: int
    DIR_WrtDate: int
    DIR_FstClusLO: int
    DIR_FileSize: int

    STRUCT_FORMAT = "<11sBBBHHHHHHHI"

    @classmethod
    def from_bytes(cls, data: bytes) -> "FAT32DirectoryEntry":
        unpacked = struct.unpack(cls.STRUCT_FORMAT, data[:struct.calcsize(cls.STRUCT_FORMAT)])
        return cls(*unpacked)

    def to_bytes(self) -> bytes:
        return struct.pack(
            self.STRUCT_FORMAT,
            self.DIR_Name,
            self.DIR_Attr,
            self.NTRes,
            self.DIR_CrtTimeTenth,
            self.DIR_CrtTime,
            self.DIR_CrtDate,
            self.DIR_LstAccDate,
            self.DIR_FstClusHI,
            self.DIR_WrtTime,
            self.DIR_WrtDate,
            self.DIR_FstClusLO,
            self.DIR_FileSize
        )


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
class FAT:
    headers: FATHeaders
    fsInfo: FAT32FSInfo
    table: array.array
    totalClusters: int
    firstDataSector: int

    def __init__(self, img_path, startClusters = 3000):
        with open(img_path, "+rb") as f:
            content = f.read(512)
            headersBoot = FAT32HeaderBoot.from_bytes(content[0:36])
            headersExtended = FAT32HeaderExtended.from_bytes(content[36:90])    

            fatStartSector = headersBoot.rsvdSecCnt
            fatSize = headersExtended.FATSz32
            fatBytes = fatSize * headersBoot.bytesPerSec

            fatTotalClusters = fatSize * headersBoot.bytesPerSec / 4
            firstDataSector = fatStartSector + (headersBoot.numFATs * fatSize)

            f.seek(fatStartSector * headersBoot.bytesPerSec, 0)
            table = array.array("I")
            table.frombytes(f.read(fatBytes))

            f.seek(headersExtended.FSInfo * headersBoot.bytesPerSec, 0)
            content = f.read(512)

            fsInfo = FAT32FSInfo.from_bytes(content)
            if fsInfo.leadSignature != 0x41615252 or fsInfo.structSignature != 0x61417272 or fsInfo.trailSignature != 0xAA550000:
                print("FSINFO invalid signature or Missing!")
                fsInfo.freeClusterCount = 0xFFFFFFFF
                fsInfo.nextFreeCluster = 0xFFFFFFFF
            else:
                if fsInfo.freeClusterCount == 0xFFFFFFFF:
                    fsInfo.freeClusterCount = fatTotalClusters

                if fsInfo.nextFreeCluster == 0xFFFFFFFF:
                    fsInfo.nextFreeCluster = startClusters

            self.headers = FATHeaders(headersBoot, headersExtended)
            self.fsInfo = fsInfo
            self.table = table
            self.totalClusters = fatTotalClusters
            self.firstDataSector = firstDataSector

    @classmethod
    def get_next_cluster(cls, current: int) -> int:
        next = cls.table[current]
        return next & 0x0FFFFFFF
    
    @classmethod
    def get_free_cluster(cls) -> int:
        if cls.fsInfo.nextFreeCluster >= cls.headers.extended.rootClus and cls.fsInfo.nextFreeCluster < cls.totalClusters:
            cluster = cls.fsInfo.nextFreeCluster
        else:
            for c in range(cls.headers.extended.rootClus, cls.totalClusters):
                if (cls.table[c] & 0x0FFFFFFF) == 0:
                    return c
            
            return -1

        for c in range(cluster, cls.totalClusters):
            if (cls.table[c] & 0x0FFFFFFF) == 0:
                return c
            
        return -1
    
    @classmethod
    def reserve_next_free_cluster(cls) -> int:
        cluster = cls.get_free_cluster()
        if cluster < 0:
            return cluster
        
        cls.fsInfo.nextFreeCluster = cluster + 1
        if cls.fsInfo.freeClusterCount != 0xFFFFFFFF:
            cls.fsInfo.freeClusterCount -= 1

        return cluster
        