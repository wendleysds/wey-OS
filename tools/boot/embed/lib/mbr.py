import struct
from dataclasses import dataclass

@dataclass
class MBREntry:
	bootInd: int

	startHead: int
	startSec: int
	startCyl: int

	partType: int

	endHead: int
	endSec: int
	endCyl: int

	startLBA: int
	sizeInLBA: int

	STRUCT_FORMAT = "<BBBBBBBBII"

	@classmethod
	def from_bytes(cls, data: bytes) -> "MBREntry":
		unpacked = struct.unpack(cls.STRUCT_FORMAT, data[:struct.calcsize(cls.STRUCT_FORMAT)])
		return cls(*unpacked)