#include <device/terminal.h>
#include <device/tty.h>
#include <kernel/init.h>
#include <mm/kheap.h>
#include <mm/memblock.h>
#include <def/errno.h>
#include <def/config.h>
#include <lib/string.h>
#include <lib/assert.h>
#include <asm/page.h>

#define FIRST_VT_INDEX 0

static struct vt terminals[TERMINALS_MAX];
static spinlock_t console_lock;
static struct tty_driver* driver = NULL;

static const uint8_t ansi_to_color[8] = { 0, 4, 2, 6, 1, 5, 3, 7 };

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
	uint8_t color = vt->state.color;
	if (vt->state.reverse) {
		color = VT_COLOR(VT_BG(color), VT_FG(color));
	}

	vt->vt_attr = vt->vt_sw->build_attr(vt, color, vt->state.intensity);
	vt->vt_erase_attr = vt->vt_sw->build_attr(vt, color, TERMINAL_INTENSITY_NORMAL);
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

static void vt_gotoxy(struct vt_data *vt, int x, int y)
{
	vt->need_wrap = 0;
	if (x < 0) x = 0;
	if (y < 0) y = 0;
	if (x >= (int)vt->vt_cols) x = vt->vt_cols - 1;
	if (y >= (int)vt->vt_rows) y = vt->vt_rows - 1;
	vt->state.x = x;
	vt->state.y = y;
}

static void vt_clear_cells(struct vt_data *vt, unsigned int y, unsigned int x, unsigned int count)
{
	vt->need_wrap = 0;
	unsigned int pos = y * vt->vt_cols + x;
	unsigned int total = vt->vt_rows * vt->vt_cols;

	if (pos >= total) return;
	if (count > total - pos) count = total - pos;

	for (unsigned int i = 0; i < count; i++, pos++) {
		vt->screenbuffer[pos] = vt->vt_erase_char;
		if (vt_is_visible(vt))
			vt->vt_sw->putc(vt, vt->vt_erase_char,
				pos % vt->vt_cols, pos / vt->vt_cols);
	}
}

static void vt_csi_sgr(struct vt_data *vt)
{
	for (unsigned int i = 0; i <= vt->esc_nparams; i++) {
		unsigned int p = vt->esc_params[i];
		uint8_t fg = VT_FG(vt->state.color);
		uint8_t bg = VT_BG(vt->state.color);

		if (p == 0) {
			vt->state.reverse = 0;
			vt->state.color = vt->vt_default_color;
			vt->state.intensity = TERMINAL_INTENSITY_NORMAL;
			continue;
		}

		if (p == 1)                    vt->state.intensity = TERMINAL_INTENSITY_BOLD;
		else if (p == 2)               vt->state.intensity = TERMINAL_INTENSITY_HALF_BRIGHT;
		else if (p == 7)               vt->state.reverse = 1;
		else if (p == 22)              vt->state.intensity = TERMINAL_INTENSITY_NORMAL;
		else if (p == 27)              vt->state.reverse = 0;
		else if (p >= 30 && p <= 37)   fg = ansi_to_color[p - 30];
		else if (p == 39)              fg = VT_FG(vt->vt_default_color);
		else if (p >= 40 && p <= 47)   bg = ansi_to_color[p - 40];
		else if (p == 49)              bg = VT_BG(vt->vt_default_color);
		else if (p >= 90 && p <= 97)   fg = ansi_to_color[p - 90] | 0x8;

		vt->state.color = VT_COLOR(fg, bg);
	}

	update_attr(vt);
}

static void vt_csi_dispatch(struct vt_data *vt, char final)
{
	unsigned int p0 = vt->esc_params[0];
	unsigned int p1 = vt->esc_nparams >= 1 ? vt->esc_params[1] : 0;
	int arg = p0 ? (int)p0 : 1;               /* default 1 nos movimentos */
	int x = vt->state.x, y = vt->state.y;

	if (vt->esc_private) {
		if (p0 == 25 && (final == 'h' || final == 'l'))
			vt->cursor_hidden = (final == 'l');
		return;
	}

	switch (final) {
	case 'A': vt_gotoxy(vt, x, y - arg); break;
	case 'B': vt_gotoxy(vt, x, y + arg); break;
	case 'C': vt_gotoxy(vt, x + arg, y); break;
	case 'D': vt_gotoxy(vt, x - arg, y); break;
	case 'G': vt_gotoxy(vt, arg - 1, y); break;
	case 'd': vt_gotoxy(vt, x, arg - 1); break;
	case 'H':
	case 'f':
		vt_gotoxy(vt, (p1 ? (int)p1 : 1) - 1, arg - 1);
		break;

	case 'J':
		if (p0 == 0)
			vt_clear_cells(vt, y, x, vt->vt_rows * vt->vt_cols);
		else if (p0 == 1)
			vt_clear_cells(vt, 0, 0, y * vt->vt_cols + x + 1);
		else
			vt_clear_cells(vt, 0, 0, vt->vt_rows * vt->vt_cols);
		break;

	case 'K':
		if (p0 == 0)      vt_clear_cells(vt, y, x, vt->vt_cols - x);
		else if (p0 == 1) vt_clear_cells(vt, y, 0, x + 1);
		else              vt_clear_cells(vt, y, 0, vt->vt_cols);
		break;

	case 'm': vt_csi_sgr(vt); break;
	case 's': vt->saved_state = vt->state; break;
	case 'u': vt->state = vt->saved_state; update_attr(vt); break;
	}
}

static void vt_putc(struct vt_data *vt, char ch){
	if(!vt->vt_sw->putc){
		return;
	}

	if ((unsigned char)ch < 32) {
		if (ch != 7 && ch != 8 && ch != 9 && ch != 10 && ch != 13) {
			return;
		}
	}

	switch (ch) {
		case '\a':
			break;

		case '\n':
			vt->need_wrap = 0;
			vt->state.x = 0;
			vt->state.y++;
		break;

		case '\r':
			vt->need_wrap = 0;
			vt->state.x = 0; 
		break;

		case '\b':
			vt->need_wrap = 0;
			if(vt->state.x != 0)
				vt->state.x--;
		break;

		case '\t':
			vt->need_wrap = 0;
			vt->state.x = (vt->state.x + 4) & ~(4 - 1);
			if (vt->state.x >= vt->vt_cols) {
				vt->state.x = 0;
				vt->state.y++;
			}
		break;

		default:
			if (vt->need_wrap) {
				vt->need_wrap = 0;
				vt->state.x = 0;
				vt->state.y++;
				if (vt->state.y >= vt->vt_rows) {
					vt_scroll(vt, vt->vt_top, vt->vt_bottom, TERMINAL_SCROLL_UP, 1);
					vt->state.y--;
				}
			}

			const unsigned int pos = vt->state.y * vt->vt_cols + vt->state.x;
			if (vt->state.y >= vt->vt_rows){
				return;
			}

			vt->screenbuffer[pos] =
				(unsigned char)ch | (vt->vt_attr << 8);

			if(vt_is_visible(vt)){
				vt->vt_sw->putc(
					vt,
					vt->screenbuffer[pos],
					vt->state.x,
					vt->state.y
				);
			}

			if (vt->state.x + 1 >= vt->vt_cols) {
				vt->need_wrap = 1;
			} else {
				vt->state.x++;
			}
		break;
	}

	if (vt->state.y >= vt->vt_rows) {
		vt_scroll(vt, vt->vt_top, vt->vt_bottom, TERMINAL_SCROLL_UP, 1);
		vt->state.y--;
	}
}

static void vt_feed(struct vt_data *vt, char ch)
{
	switch (vt->esc_state) {
	case ESnormal:
		if (ch == 0x1B) vt->esc_state = ESesc;
		else vt_putc(vt, ch);
		return;

	case ESesc:
		vt->esc_state = ESnormal;
		if (ch == '[') {
			memset(vt->esc_params, 0, sizeof(vt->esc_params));
			vt->esc_nparams = 0;
			vt->esc_private = 0;
			vt->esc_state = EScsi;
		} else if (ch == '7') {
			vt->saved_state = vt->state;
		} else if (ch == '8') {
			vt->state = vt->saved_state;
			update_attr(vt);
		}
		return;

	case EScsi:
		if (ch == '?') {
			vt->esc_private = 1;
		} else if (ch >= '0' && ch <= '9') {
			unsigned int *p = &vt->esc_params[vt->esc_nparams];
			*p = *p * 10 + (ch - '0');
		} else if (ch == ';') {
			if (vt->esc_nparams < VT_MAX_PARAMS - 1)
				vt->esc_nparams++;
		} else if (ch >= 0x40 && ch <= 0x7E) {
			vt_csi_dispatch(vt, ch);
			vt->esc_state = ESnormal;
		} else if (ch == 0x1B) {
			vt->esc_state = ESesc;
		} else if ((unsigned char)ch < 0x20) {
			vt_putc(vt, ch);
		}
		return;
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
		vt_feed(vt, *s++);
	}
	vt->vt_sw->cursor(vt, !vt->cursor_hidden);

	spin_unlock(&vt->lock);
}

static void vt_write(struct vt_data *vt, const char *buf, int length){
	spin_lock(&vt->lock);

	vt->vt_sw->cursor(vt, 0);
	for(int i = 0; i < length; i++){
		vt_feed(vt, buf[i]);
	}
	vt->vt_sw->cursor(vt, !vt->cursor_hidden);

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
	new_tty->winsize.ws_row = vt->data->vt_rows;
	new_tty->winsize.ws_col = vt->data->vt_cols;
	new_tty->winsize.ws_xpixel = 0;
	new_tty->winsize.ws_ypixel = 0;

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
