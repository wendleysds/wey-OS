#include <device/terminal.h>
#include <device/tty.h>
#include <kernel/init.h>
#include <kernel/printk.h>
#include <mm/kheap.h>
#include <mm/memblock.h>
#include <def/errno.h>
#include <def/config.h>
#include <uapi/headers.h>
#include <lib/string.h>
#include <lib/assert.h>
#include <asm/page.h>

#define FIRST_VT_INDEX 0

static struct vt terminals[TERMINALS_MAX];
static spinlock_t console_lock;
static struct tty_driver* driver = NULL;

extern struct video_info video_info;
extern const struct consw vgadrv_consw;

int cur_vt, last_vt;

static struct vt* terminal_get(int index){
	if(index >= TERMINALS_MAX || index < 0){
		return NULL;
	}

	return &terminals[index];
}

static inline int vt_is_visible(struct vt_data *vt) {
    return vt->id == cur_vt;
}

static void update_attr(struct vt_data *vt){
	vt->vt_attr = vt->vt_sw->build_attr(vt, vt->state.color, vt->state.intensity);
	vt->vt_erase_attr = vt->vt_sw->build_attr(vt, vt->state.color, TERMINAL_INTENSITY_NORMAL);
	
	vt->vt_erase_char = ' ' | (vt->vt_erase_attr << 8);
}

static int vt_data_con_setup(struct vt_data *vt){
	vt->vt_sw = &vgadrv_consw;
	vgadrv_consw.init(vt);

	vt->vt_size_row = vt->vt_cols << 1; // char + attr
	vt->screenbuffer_size = vt->vt_rows * vt->vt_size_row;
	if(!vt->screenbuffer_size){
		return -EINVAL;
	}

	return SUCCESS;
}

static void vt_data_setup(struct vt_data *vt){
	vt->vt_screen_start = (unsigned long)vt->screenbuffer;
	vt->vt_visible_origin = vt->vt_screen_start;
	vt->vt_screen_end = vt->vt_screen_start + vt->screenbuffer_size;
	vt->scrennbuffer_pos = (unsigned long)vt->screenbuffer;
	vt->vt_top = 0;
	vt->vt_bottom = vt->vt_rows;

	vt->vt_default_color = TERMINAL_DEFAULT_COLOR;
	vt->state.color = TERMINAL_DEFAULT_COLOR;
	vt->state.intensity = TERMINAL_INTENSITY_NORMAL;
	update_attr(vt);
	spinlock_init(&vt->lock);
}

static __init int vt_early_alloc(){
	struct vt* terminal = terminal_get(FIRST_VT_INDEX);
	struct vt_data *vt = (struct vt_data*)memblock_alloc(sizeof(struct vt_data), 1);

	if(!vt){
		return -ENOMEM;
	}

	vt = (struct vt_data*)__va(vt);
	memset(vt, 0x0, sizeof(struct vt_data));

	int res = vt_data_con_setup(vt);
	if(IS_ERR_VALUE(res)){
		return res;
	}

	vt->screenbuffer = (unsigned short*)memblock_alloc(vt->screenbuffer_size, 1);
	if(!vt->screenbuffer){
		return -ENOMEM;
	}

	vt->screenbuffer = (unsigned short*)__va(vt->screenbuffer);

	vt_data_setup(vt);

	vt->id = FIRST_VT_INDEX;
	terminal->data = vt;

	return SUCCESS;
}

static int vt_alloc(int currcon){
	if(currcon >= TERMINALS_MAX || currcon < 0){
		return -EINVAL;
	}

	spin_lock(&console_lock);

	struct vt* terminal = terminal_get(currcon);
	if(terminal->data){
		spin_unlock(&console_lock);
		return SUCCESS;
	}

	struct vt_data *vt = (struct vt_data*)kzalloc(sizeof(struct vt_data));
	if(!vt){
		spin_unlock(&console_lock);
		return -ENOMEM;
	}

	int res = vt_data_con_setup(vt);
	if(IS_ERR_VALUE(res)){
		kfree(vt);
		spin_unlock(&console_lock);
		return res;
	}

	vt->screenbuffer = (unsigned short*)kmalloc(vt->screenbuffer_size);
	if(!vt->screenbuffer){
		kfree(vt);
		spin_unlock(&console_lock);
		return -ENOMEM;
	}

	vt_data_setup(vt);

	vt->id = currcon;
	terminals[currcon].data = vt;

	spin_unlock(&console_lock);
	return SUCCESS;
}

static void vt_redraw(struct vt_data *vt){
	if(!vt->vt_sw->putc){
		return;
	}

	for(int row = 0; row < vt->vt_rows; row++){
		for(int col = 0; col < vt->vt_cols; col++){
			int pos = row * vt->vt_cols + col;
			vt->vt_sw->putc(
				vt,
				vt->screenbuffer[pos],
				col,
				row
			);
		}
	}
}

static void vt_scroll(struct vt_data *vt, unsigned int top, unsigned int bottom, terminal_scroll_t scroll_dir, unsigned int lines){
	if (!lines || top >= bottom)
		return;

	if (lines > bottom - top)
		lines = bottom - top;

	uint16_t *buf = vt->screenbuffer;
	unsigned int cols = vt->vt_cols;
	unsigned int stride = cols;

	unsigned int region_rows = bottom - top;
	unsigned int copy_rows = region_rows - lines;

	if (scroll_dir == TERMINAL_SCROLL_UP) {
		memmove(
			&buf[top * stride],
			&buf[(top + lines) * stride],
			copy_rows * stride * sizeof(uint16_t)
		);

		for (unsigned int row = bottom - lines; row < bottom; row++) {
			uint16_t *line = &buf[row * stride];
			for (unsigned int col = 0; col < cols; col++)
				line[col] = vt->vt_erase_char;
		}

	} else {
		memmove(
			&buf[(top + lines) * stride],
			&buf[top * stride],
			copy_rows * stride * sizeof(uint16_t)
		);

		for (unsigned int row = top; row < top + lines; row++) {
			uint16_t *line = &buf[row * stride];
			for (unsigned int col = 0; col < cols; col++)
				line[col] = vt->vt_erase_char;
		}
	}

	if(vt_is_visible(vt) && vt->vt_sw->scroll){
		vt->vt_sw->scroll(vt, top, bottom, scroll_dir, lines);
	}else if(vt_is_visible(vt)){
		vt_redraw(vt);
	}
}

static void vt_putc(struct vt_data *vt, char ch){
	if(!vt->vt_sw->putc){
		return;
	}

	unsigned int pos = 0;

	switch (ch) {
		case '\n':
			vt->state.x = 0;
			vt->state.y++;
		break;

		case '\r':
			vt->state.x = 0; 
		break;

		case '\b':
			if(vt->state.x != 0)
				vt->state.x--;
		break;

		case '\t':
			vt->state.x = (vt->state.x + 4) & ~(4 - 1);
			if (vt->state.x >= vt->vt_cols) {
				vt->state.x = 0;
				vt->state.y++;
			}
		break;

		default:
			pos = vt->state.y * vt->vt_cols + vt->state.x;

			if (vt->state.y >= vt->vt_rows){
				return;
			}

			vt->screenbuffer[pos] =
				ch | (vt->vt_attr << 8);

			if(vt_is_visible(vt)){
				vt->vt_sw->putc(
					vt,
					vt->screenbuffer[pos],
					vt->state.x,
					vt->state.y
				);
			}

			vt->state.x++;
		break;
	}

	if (vt->state.x >= vt->vt_cols) {
		vt->state.x = 0;
		vt->state.y++;
	}

	if (vt->state.y >= vt->vt_rows) {
		vt_scroll(vt, vt->vt_top, vt->vt_bottom, TERMINAL_SCROLL_UP, 1);
		vt->state.y--;
	}
}

static void vt_switch(int new_vt){
	if (new_vt == cur_vt)
		return;

	spin_lock(&console_lock);

	struct vt_data *old = terminal_get(cur_vt)->data;
	struct vt_data *new = terminal_get(new_vt)->data;

	if (!new){
		spin_unlock(&console_lock);
		return;
	}

	last_vt = cur_vt;
	cur_vt = new_vt;

	old->vt_sw->cursor(old, 0);

	vt_redraw(new);

	new->vt_sw->cursor(new, 1);

	spin_unlock(&console_lock);
}

void vt_puts(struct vt_data *vt, const char *s){
	spin_lock(&vt->lock);

	vt->vt_sw->cursor(vt, 0);
	while (*s) {
		vt_putc(vt, *s++);
	}
	vt->vt_sw->cursor(vt, 1);

	spin_unlock(&vt->lock);
}

static void vt_write(struct vt_data *vt, const char *buf, int length){
	spin_lock(&vt->lock);

	vt->vt_sw->cursor(vt, 0);
	for(int i = 0; i < length; i++){
		vt_putc(vt, buf[i]);
	}
	vt->vt_sw->cursor(vt, 1);

	spin_unlock(&vt->lock);
}

static void vt_clean(struct vt_data* vt){
	spin_lock(&vt->lock);
	if(vt_is_visible(vt) && vt->vt_sw->clean){
		vt->vt_sw->clean(vt);
	}

	uint16_t *buf = vt->screenbuffer;
	unsigned int cells = vt->screenbuffer_size / sizeof(uint16_t);

	for (unsigned int i = 0; i < cells; i++)
		buf[i] = vt->vt_erase_char;


	if(!vt->vt_sw->clean && vt_is_visible(vt)){
		vt_redraw(vt);
	}

	spin_unlock(&vt->lock);
}

static void vt_write_current_active(const char *buf, int length){
	vt_write(terminal_get(cur_vt)->data, buf, length);
}

static int __init terminal_common_init(int early){
	memset(terminals, 0x0, sizeof(terminals));

	int res = vgadrv_consw.setup(&video_info);
	if(IS_ERR_VALUE(res)){
		return res;
	}

	if (early) {
		spinlock_init(&console_lock);
		res = vt_early_alloc();
	} else {
		res = vt_alloc(FIRST_VT_INDEX);
	}

	if(IS_ERR_VALUE(res)){
		return res;
	}

	if(video_info.bpp == 0)
		terminals[FIRST_VT_INDEX].data->vt_is_text_mode = 1;

	if(!early) {
		vgadrv_consw.exit_early();
	}

	cur_vt = last_vt = FIRST_VT_INDEX;

	printk_set_echo(vt_write_current_active);

	printk_show_buffer();

	return SUCCESS;
}

int __init terminal_early_init(){
	return terminal_common_init(1);
}

int __init terminal_init(){
	if(terminal_get(FIRST_VT_INDEX)->data){
		vgadrv_consw.exit_early();
		return SUCCESS;
	}

	return terminal_common_init(0);
}

struct vt* terminal_get_current(){
	return terminal_get(cur_vt);
}

int terminal_switch(int index){
	struct vt* vt = terminal_get(index);
	if(!vt) return -EINVAL;

	if(!vt->data){
		int res = vt_alloc(index);
		if(res) return res;
	}

	if(!vt->tty){
		void* res_ptr = tty_ensure_created(driver, index);
		if(IS_ERR(res_ptr)){
			kfree(driver);
			return PTR_ERR(res_ptr);
		}
	}

	vt_switch(index);
	return SUCCESS;
}

static int vt_tty_install(struct tty_driver *self, struct tty_struct *new_tty){
	if(!self || !new_tty){
		return -EINVAL;
	}

	if(new_tty->index < 0 || new_tty->index >= TERMINALS_MAX){
		return -EINVAL;
	}

	int res = vt_alloc(new_tty->index);
	if(res) return res;

	struct vt* vt = terminal_get(new_tty->index);
	BUG_ON(vt->tty);

	vt->tty = new_tty;

	return OK;
}

static int vt_tty_write(struct tty_struct *self, const u8 *buffer, size_t count){
	if(!self || !buffer){
		return -EINVAL;
	}

	if(count == 0){
		return 0;
	}

	struct vt *terminal = terminal_get(self->index);
	if(!terminal){
		return -EINVAL;
	}

	if(!terminal->data){
		int res = vt_alloc(self->index);
		if(res < 0){
			return res;
		}
	}

	vt_write(terminal->data, (char*)buffer, count);

	return count;
}

static const struct tty_ops vt_tty_ops = {
	.install = vt_tty_install,
	.write = vt_tty_write
};

static int __init vt_tty_init(void){
	driver = tty_alloc_driver();
	if(!driver){
		return -ENOMEM;
	}

	driver->name = "tty",
	driver->ops = &vt_tty_ops;
	driver->major = 4;
	driver->minor_start = 0;
	driver->num = TERMINALS_MAX;

	int res = tty_register_driver(driver);
	if(res){
		kfree(driver);
	}

	void* res_ptr = tty_ensure_created(driver, 0);
	if(IS_ERR(res_ptr)){
		kfree(driver);
		return PTR_ERR(res_ptr);
	}

	return SUCCESS;
}

static int __init vt_init(void){
	spinlock_init(&console_lock);
	return vt_tty_init();
}

device_initcall(vt_init);
