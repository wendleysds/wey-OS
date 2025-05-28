; FATSignature.bin -  FAT32 Signarute
db 'RRaA'
times 484-($ - $$) db 0
db 'rrAa'

dd 0xFFFFFFFF ; Next free cluster (Unknow)
dd 0xFFFFFFFF ; Total free cluster (Unknow)

times 510-($ - $$) db 0
dw 0xAA55
