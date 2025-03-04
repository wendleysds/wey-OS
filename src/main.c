#define VGA_MEMORY (volatile char*)0xB8000

void clear_screen() {
	volatile char* videoMemory = VGA_MEMORY;
	for(int i = 0; i < 80 * 25 * 2; i++){
		videoMemory[i * 2] = ' ';
		videoMemory[i * 2 + 1] = 0x0F;
	}
}

void vga_write(const char* message, unsigned char color) {
	volatile char* videoMemory = VGA_MEMORY;
	int i = 0;

	while(message[i] != '\0'){
		videoMemory[i * 2] = message[i];
		videoMemory[i * 2 + 1] = color;
		i++;
	}
}

void kmain(){
	clear_screen();
	vga_write("Hello World!", 0x0F);
	while(1);
}


