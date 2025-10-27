global tss_load
global idt_load
global gdt_load

; void tss_load(int tss_segment);
tss_load:
	mov ax, [esp+4]
	ltr ax
	ret

; void idt_load(struct IDTr_ptr* ptr);
idt_load:
	mov eax, [esp+4]
	lidt [eax]
	ret

; void gdt_load(struct GDT* gdt, int size);
gdt_load:
	mov eax, [esp+4]
	mov [gdt_descriptor + 2], eax
	mov ax, [esp+8]
	mov [gdt_descriptor], ax
	lgdt [gdt_descriptor]
	ret

section .data
gdt_descriptor:
	dw 0x00 ; Size
	dw 0x00 ; GDT Start Address