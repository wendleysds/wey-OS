#ifndef __AML_INTERNAL_OPCODES_H__
#define __AML_INTERNAL_OPCODES_H__

#include <stdbool.h>

enum aml_opcode {
	ZeroOp = 0x00,
	OneOp = 0x01,
	AliasOp = 0x06,
	NameOp = 0x08,
	BytePrefix = 0x0A,
	WordPrefix = 0x0B,
	DWordPrefix = 0x0C,
	StringPrefix = 0x0D,
	QWordPrefix = 0x0E,
	ScopeOp = 0x10,
	BufferOp = 0x11,
	PackageOp = 0x12,
	VarPackageOp = 0x13,
	MethodOp = 0x14,
	ExternalOp = 0x15,
	DualNamePrefix = 0x2E, 
	MultiNamePrefix = 0x2F, 

	// DigitChar = 0x30-0x39	0 - 9
	// NameChar = 0x41-0x5A	A - Z

	ExtOpPrefix = 0x5B, 
	
	// Extended opcodes (0x5B << 8 | second byte)
	MutexOp = 0x5B01,
	EventOp = 0x5B02,
	CondRefOfOp = 0x5B12,
	CreateFieldOp = 0x5B13,
	LoadTableOp = 0x5B1F,
	LoadOp = 0x5B20,
	StallOp = 0x5B21,
	SleepOp = 0x5B22,
	AcquireOp = 0x5B23,
	SignalOp = 0x5B24,
	WaitOp = 0x5B25,
	ResetOp = 0x5B26,
	ReleaseOp = 0x5B27,
	FromBCDOp = 0x5B28,
	ToBCD = 0x5B29,
	UnloadOp = 0x5B2A,
	RevisionOp = 0x5B30,
	DebugOp = 0x5B31,
	FatalOp = 0x5B32,
	TimerOp = 0x5B33,
	OpRegionOp = 0x5B80,
	FieldOp = 0x5B81,
	DeviceOpList = 0x5B82,
	ProcessorOp = 0x5B83,
	PowerResOp = 0x5B84,
	ThermalZoneOpList = 0x5B85,
	IndexFieldOp = 0x5B86,
	BankFieldOp = 0x5B87,
	DataRegionOp = 0x5B88,

	RootChar = 0x5C,
	ParentPrefixChar = 0x5E,
	NameChar_Underscore = 0x5F,
	
	Local0Op = 0x60,
	Local1Op = 0x61,
	Local2Op = 0x62,
	Local3Op = 0x63,
	Local4Op = 0x64,
	Local5Op = 0x65,
	Local6Op = 0x66,
	Local7Op = 0x67,

	Arg0Op = 0x68,
	Arg1Op = 0x69,
	Arg2Op = 0x6A,
	Arg3Op = 0x6B,
	Arg4Op = 0x6C,
	Arg5Op = 0x6D,
	Arg6Op = 0x6E,
	
	StoreOp = 0x70,
	RefOfOp = 0x71,
	AddOp = 0x72,
	ConcatOp = 0x73,
	SubtractOp = 0x74,
	IncrementOp = 0x75,
	DecrementOp = 0x76,
	MultiplyOp = 0x77,
	DivideOp = 0x78,
	ShiftLeftOp = 0x79,
	ShiftRightOp = 0x7A,
	AndOp = 0x7B,
	NandOp = 0x7C,
	OrOp = 0x7D,
	NorOp = 0x7E,
	XorOp = 0x7F,
	NotOp = 0x80,
	FindSetLeftBitOp = 0x81,
	FindSetRightBitOp = 0x82,
	DerefOfOp = 0x83,
	ConcatResOp = 0x84,
	ModOp = 0x85,
	NotifyOp = 0x86,
	SizeOfOp = 0x87,
	IndexOp = 0x88,
	MatchOp = 0x89,
	CreateDWordFieldOp = 0x8A,
	CreateWordFieldOp = 0x8B,
	CreateByteFieldOp = 0x8C,
	CreateBitFieldOp = 0x8D,
	TypeOp = 0x8E,
	CreateQWordFieldOp = 0x8F,
	LandOp = 0x90,
	LorOp = 0x91,
	LnotOp = 0x92,
	
	// Extended opcodes (0x92 << 8 | second byte)
	LNotEqualOp		= 0x9293,
	LLessEqualOp	 = 0x9294,
	LGreaterEqualOp = 0x9295,

	LEqualOp = 0x93,
	LGreaterOp = 0x94,
	LLessOp = 0x95,
	ToBufferOp = 0x96,
	ToDecimalStringOp = 0x97,
	ToHexStringOp = 0x98,
	ToIntegerOp = 0x99,
	ToStringOp = 0x9C,
	CopyObjectOp = 0x9D,
	MidOp = 0x9E,
	ContinueOp = 0x9F,
	IfOp = 0xA0,
	ElseOp = 0xA1,
	WhileOp = 0xA2,
	NoopOp = 0xA3,
	ReturnOp = 0xA4,
	BreakOp = 0xA5,
	BreakPointOp = 0xCC,
	OnesOp = 0xFF,

	// internal
	DecodeFailed = 0xFFFF,
};

static inline bool opcode_is_digit(enum aml_opcode op) {
	if (op == ZeroOp || op == OneOp || op == OnesOp ||
		op == BytePrefix || op == WordPrefix ||
		op == DWordPrefix || op == QWordPrefix)
		return true;
	
	return op >= 0x30 && op <= 0x39;
}

static inline bool opcode_is_char(enum aml_opcode op) {
	return op == '\\' || op == '^' || op == '_' || op == '.' || op == '/' || (op >= 'A' && op <= 'Z');
}

static inline bool opcode_is_arg(enum aml_opcode opcode) {
	return opcode >= 0x68 && opcode <= 0x6E;
}

static inline bool opcode_is_local(enum aml_opcode opcode) {
	return opcode >= 0x60 && opcode <= 0x67;
}

#endif