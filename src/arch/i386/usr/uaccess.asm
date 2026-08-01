global raw_copy_from_user
global raw_copy_to_user

section .text

; int __buffer_copy(void* to, const void* from, size_t size);
__buffer_copy:
	mov edi, [esp + 4]  ; dest
	mov esi, [esp + 8]  ; src
	mov ecx, [esp + 12] ; size

.copy:
	rep movsb

.fault:
	mov eax, ecx
    ret

; int raw_copy_from_user(void* to, const void __user* from, size_t size);
raw_copy_from_user:
	jmp __buffer_copy

; int raw_copy_to_user(void __user* to, const void* from, size_t size);
raw_copy_to_user:
	jmp __buffer_copy

section .exception_table.uaccess

; extable_entry_t { fault, fixup }
dd __buffer_copy.copy
dd __buffer_copy.fault

section .text