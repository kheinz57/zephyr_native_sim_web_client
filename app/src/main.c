/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Native-sim keypad demo  (480 × 854 display)
 *
 * Layout:
 *
 *   ┌──────────────────────────────────────┐
 *   │          Keypad Demo                 │  title
 *   │  ┌────────────────────────────────┐  │
 *   │  │  Last key: …                   │  │  status bar
 *   │  └────────────────────────────────┘  │
 *   │                                      │
 *   │  ┌─────── Action Keys ────────────┐  │
 *   │  │  [  M  ] [  H  ] [  B  ]      │  │
 *   │  │  [  S  ] [  F  ] [  T  ]      │  │
 *   │  │  [  C  ]                       │  │
 *   │  └────────────────────────────────┘  │
 *   │                                      │
 *   │  ┌────────── D-Pad ────────────── ┐  │
 *   │  │          [  ▲  ]               │  │
 *   │  │  [  ◄ ]  [     ]  [  ► ]      │  │
 *   │  │          [  ▼  ]               │  │
 *   │  └────────────────────────────────┘  │
 *   │                                      │
 *   │  Click a button or press SDL key     │  hint
 *   └──────────────────────────────────────┘
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/input/input.h>
#include <zephyr/logging/log.h>

#include <lvgl.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

/* ── DT nodes ─────────────────────────────────────────────────────────────── */
#define NAV_KEYS_NODE   DT_NODELABEL(nav_keys)
#define KEY_M_NODE      DT_PATH(nav_keys, btn_m)
#define KEY_S_NODE      DT_PATH(nav_keys, btn_s)
#define KEY_B_NODE      DT_PATH(nav_keys, btn_b)
#define KEY_LEFT_NODE   DT_PATH(nav_keys, btn_left)
#define KEY_RIGHT_NODE  DT_PATH(nav_keys, btn_right)
#define KEY_UP_NODE     DT_PATH(nav_keys, btn_up)
#define KEY_DOWN_NODE   DT_PATH(nav_keys, btn_down)
#define KEY_H_NODE      DT_PATH(nav_keys, btn_h)
#define KEY_F_NODE      DT_PATH(nav_keys, btn_f)
#define KEY_T_NODE      DT_PATH(nav_keys, btn_t)
#define KEY_C_NODE      DT_PATH(nav_keys, btn_c)

/* ── Palette ──────────────────────────────────────────────────────────────── */
#define COL_BG         0x1A1A2E
#define COL_PANEL      0x16213E
#define COL_BTN_IDLE   0x0F3460
#define COL_BTN_PRESS  0xE94560
#define COL_DPAD_IDLE  0x1B4F72
#define COL_DPAD_PRESS 0xE94560
#define COL_ACCENT     0x533483
#define COL_TEXT       0xEEEEEE
#define COL_STATUS_BG  0x0D0D1A
#define COL_GRP_BORDER 0x2E4483

/* ── Key indices ──────────────────────────────────────────────────────────── */
#define IDX_UP    0
#define IDX_LEFT  1
#define IDX_DOWN  2
#define IDX_RIGHT 3
#define IDX_M     4
#define IDX_H     5
#define IDX_B     6
#define IDX_S     7
#define IDX_F     8
#define IDX_T     9
#define IDX_C     10

struct key_info {
	const char *symbol;
	const char *desc;
	uint16_t    code;
	lv_obj_t   *btn;
};

static struct key_info keys[] = {
	/* D-pad — LVGL built-in arrow symbols */
	[IDX_UP]    = { LV_SYMBOL_UP,    "Up",        DT_PROP(KEY_UP_NODE,    zephyr_code) },
	[IDX_LEFT]  = { LV_SYMBOL_LEFT,  "Left",      DT_PROP(KEY_LEFT_NODE,  zephyr_code) },
	[IDX_DOWN]  = { LV_SYMBOL_DOWN,  "Down",      DT_PROP(KEY_DOWN_NODE,  zephyr_code) },
	[IDX_RIGHT] = { LV_SYMBOL_RIGHT, "Right",     DT_PROP(KEY_RIGHT_NODE, zephyr_code) },
	/* Action keys */
	[IDX_M]     = { "M", "Menu",      DT_PROP(KEY_M_NODE, zephyr_code) },
	[IDX_H]     = { "H", "Hero",      DT_PROP(KEY_H_NODE, zephyr_code) },
	[IDX_B]     = { "B", "Back",      DT_PROP(KEY_B_NODE, zephyr_code) },
	[IDX_S]     = { "S", "Settings",  DT_PROP(KEY_S_NODE, zephyr_code) },
	[IDX_F]     = { "F", "Function",  DT_PROP(KEY_F_NODE, zephyr_code) },
	[IDX_T]     = { "T", "Bluetooth", DT_PROP(KEY_T_NODE, zephyr_code) },
	[IDX_C]     = { "C", "Calculate", DT_PROP(KEY_C_NODE, zephyr_code) },
};

#define NUM_KEYS ARRAY_SIZE(keys)

static lv_obj_t *status_label;

/* ── State update ─────────────────────────────────────────────────────────── */

static void set_key_pressed(int idx, bool pressed)
{
	bool is_dpad   = (idx <= IDX_RIGHT);
	uint32_t press = is_dpad ? COL_DPAD_PRESS : COL_BTN_PRESS;
	uint32_t idle  = is_dpad ? COL_DPAD_IDLE  : COL_BTN_IDLE;

	lv_obj_set_style_bg_color(keys[idx].btn,
				  pressed ? lv_color_hex(press)
					  : lv_color_hex(idle), 0);

	lv_label_set_recolor(status_label, true);
	if (pressed) {
		lv_label_set_text_fmt(status_label,
				      "#FFFFFF %s#  (#E94560 %s# pressed)",
				      keys[idx].symbol, keys[idx].desc);
	} else {
		lv_label_set_text_fmt(status_label,
				      "#AAAAAA %s#  %s  released",
				      keys[idx].symbol, keys[idx].desc);
	}
}

/* ── Input callback (keyboard / GPIO-emul) ───────────────────────────────── */

static void input_cb(struct input_event *evt, void *user_data)
{
	ARG_UNUSED(user_data);
	if (evt->type != INPUT_EV_KEY) {
		return;
	}
	for (int i = 0; i < (int)NUM_KEYS; i++) {
		if (keys[i].code == evt->code) {
			set_key_pressed(i, evt->value != 0);
			return;
		}
	}
}

INPUT_CALLBACK_DEFINE(DEVICE_DT_GET(NAV_KEYS_NODE), input_cb, NULL);

/* ── LVGL mouse callbacks ─────────────────────────────────────────────────── */

static void btn_press_cb(lv_event_t *e)
{
	set_key_pressed((int)(intptr_t)lv_event_get_user_data(e), true);
}

static void btn_release_cb(lv_event_t *e)
{
	set_key_pressed((int)(intptr_t)lv_event_get_user_data(e), false);
}

/* ── Button factory ───────────────────────────────────────────────────────── */

static lv_obj_t *make_btn(lv_obj_t *parent, int idx,
			   int w, int h, uint32_t idle_col)
{
	lv_obj_t *btn = lv_obj_create(parent);
	lv_obj_set_size(btn, w, h);
	lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_bg_color(btn, lv_color_hex(idle_col), 0);
	lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
	lv_obj_set_style_radius(btn, 10, 0);
	lv_obj_set_style_border_color(btn, lv_color_hex(COL_ACCENT), 0);
	lv_obj_set_style_border_width(btn, 1, 0);

	void *ud = (void *)(intptr_t)idx;
	lv_obj_add_event_cb(btn, btn_press_cb,   LV_EVENT_PRESSED,  ud);
	lv_obj_add_event_cb(btn, btn_release_cb, LV_EVENT_RELEASED, ud);

	lv_obj_t *lbl = lv_label_create(btn);
	lv_label_set_text(lbl, keys[idx].symbol);
	lv_obj_set_style_text_color(lbl, lv_color_hex(COL_TEXT), 0);
	lv_obj_set_style_text_font(lbl, &lv_font_montserrat_28, 0);
	lv_obj_center(lbl);

	keys[idx].btn = btn;
	return btn;
}

/* ── Panel helper ─────────────────────────────────────────────────────────── */

static lv_obj_t *make_panel(lv_obj_t *parent, const char *title,
			     int w, int h, int y_ofs)
{
	lv_obj_t *panel = lv_obj_create(parent);
	lv_obj_set_size(panel, w, h);
	lv_obj_align(panel, LV_ALIGN_TOP_MID, 0, y_ofs);
	lv_obj_set_style_bg_color(panel, lv_color_hex(COL_PANEL), 0);
	lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
	lv_obj_set_style_border_color(panel, lv_color_hex(COL_GRP_BORDER), 0);
	lv_obj_set_style_border_width(panel, 1, 0);
	lv_obj_set_style_radius(panel, 12, 0);
	lv_obj_set_style_pad_all(panel, 10, 0);
	lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

	lv_obj_t *lbl = lv_label_create(panel);
	lv_label_set_text(lbl, title);
	lv_obj_set_style_text_color(lbl, lv_color_hex(COL_ACCENT), 0);
	lv_obj_set_style_text_font(lbl, &lv_font_montserrat_22, 0);
	lv_obj_align(lbl, LV_ALIGN_TOP_LEFT, 4, 0);

	return panel;
}

/* ── D-Pad cross ──────────────────────────────────────────────────────────── */
/*
 *  3×3 LVGL grid — corners and centre left empty so only the 4 arrow
 *  directions are shown, forming a cross/plus shape:
 *
 *   col:   0         1         2
 *          ·       [ ▲ ]       ·       row 0
 *        [ ◄ ]    (empty)   [ ► ]     row 1
 *          ·       [ ▼ ]       ·       row 2
 *
 *  Gap columns/rows separate cells by COL_DPAD_GAP pixels.
 */
static void build_dpad(lv_obj_t *panel)
{
	/* All four buttons must be exactly this size */
#define DPAD_SZ  110
#define DPAD_GAP   8

	lv_obj_t *cross = lv_obj_create(panel);
	lv_obj_set_size(cross,
			3 * DPAD_SZ + 2 * DPAD_GAP,
			3 * DPAD_SZ + 2 * DPAD_GAP);
	lv_obj_align(cross, LV_ALIGN_CENTER, 0, 14);
	lv_obj_set_style_bg_opa(cross, LV_OPA_TRANSP, 0);
	lv_obj_set_style_border_width(cross, 0, 0);
	lv_obj_set_style_pad_all(cross, 0, 0);
	lv_obj_set_style_pad_gap(cross, 0, 0);
	lv_obj_clear_flag(cross, LV_OBJ_FLAG_SCROLLABLE);

	static const lv_coord_t cols[] = {
		DPAD_SZ, DPAD_GAP, DPAD_SZ, DPAD_GAP, DPAD_SZ,
		LV_GRID_TEMPLATE_LAST
	};
	static const lv_coord_t rows[] = {
		DPAD_SZ, DPAD_GAP, DPAD_SZ, DPAD_GAP, DPAD_SZ,
		LV_GRID_TEMPLATE_LAST
	};
	lv_obj_set_layout(cross, LV_LAYOUT_GRID);
	lv_obj_set_grid_dsc_array(cross, cols, rows);

	/*
	 * Logical grid positions (col 0-2, row 0-2) map to track indices
	 * 0, 2, 4 (the gap tracks are 1 and 3 and never used for buttons).
	 */
#define DPAD_BTN(idx, col, row) \
	do { \
		lv_obj_t *b = make_btn(cross, (idx), DPAD_SZ, DPAD_SZ, \
				       COL_DPAD_IDLE); \
		lv_obj_set_grid_cell(b, \
			LV_GRID_ALIGN_STRETCH, (col) * 2, 1, \
			LV_GRID_ALIGN_STRETCH, (row) * 2, 1); \
	} while (0)

	DPAD_BTN(IDX_UP,    1, 0);   /* top-centre    */
	DPAD_BTN(IDX_LEFT,  0, 1);   /* middle-left   */
	DPAD_BTN(IDX_DOWN,  1, 2);   /* bottom-centre */
	DPAD_BTN(IDX_RIGHT, 2, 1);   /* middle-right  */

	/*
	 * Transparent placeholder at the true centre (track col=2, row=2)
	 * so the grid allocates all 5×5 tracks even though the corners are
	 * empty. Without this the layout engine may shrink unused tracks.
	 */
	lv_obj_t *c = lv_obj_create(cross);
	lv_obj_set_size(c, DPAD_SZ, DPAD_SZ);
	lv_obj_set_style_bg_opa(c, LV_OPA_TRANSP, 0);
	lv_obj_set_style_border_width(c, 0, 0);
	lv_obj_set_grid_cell(c,
			     LV_GRID_ALIGN_STRETCH, 2, 1,   /* col track 2 = logical col 1 */
			     LV_GRID_ALIGN_STRETCH, 2, 1);  /* row track 2 = logical row 1 */

#undef DPAD_BTN
#undef DPAD_SZ
#undef DPAD_GAP
}

/* ── Action keys grid ─────────────────────────────────────────────────────── */
/*
 *  Row-major, 4 columns × 2 rows (last cell empty):
 *    [ M ]  [ H ]  [ B ]  [ S ]
 *    [ F ]  [ T ]  [ C ]  [   ]
 */
static const int ACTION_IDXS[] = {
	IDX_M, IDX_H, IDX_B, IDX_S,
	IDX_F, IDX_T, IDX_C,
};
#define NUM_ACTION ARRAY_SIZE(ACTION_IDXS)

static void build_action_keys(lv_obj_t *panel)
{
#define ACT_COLS  4
#define ACT_H    120   /* button height */

	lv_obj_t *grid = lv_obj_create(panel);
	lv_obj_set_size(grid, LV_PCT(100), LV_SIZE_CONTENT);
	lv_obj_align(grid, LV_ALIGN_TOP_MID, 0, 30);
	lv_obj_set_style_bg_opa(grid, LV_OPA_TRANSP, 0);
	lv_obj_set_style_border_width(grid, 0, 0);
	lv_obj_set_style_pad_all(grid, 0, 0);
	lv_obj_set_style_pad_gap(grid, 10, 0);
	lv_obj_clear_flag(grid, LV_OBJ_FLAG_SCROLLABLE);

	static const lv_coord_t cols[] = {
		LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1),
		LV_GRID_TEMPLATE_LAST
	};
	static const lv_coord_t rows[] = {
		ACT_H, ACT_H,
		LV_GRID_TEMPLATE_LAST
	};
	lv_obj_set_layout(grid, LV_LAYOUT_GRID);
	lv_obj_set_grid_dsc_array(grid, cols, rows);

	for (int i = 0; i < (int)NUM_ACTION; i++) {
		int idx = ACTION_IDXS[i];
		lv_obj_t *btn = make_btn(grid, idx,
					 LV_PCT(100), ACT_H, COL_BTN_IDLE);

		/* description text below the symbol */
		lv_obj_t *desc = lv_label_create(btn);
		lv_label_set_text(desc, keys[idx].desc);
		lv_obj_set_style_text_color(desc, lv_color_hex(0x8899AA), 0);
		lv_obj_set_style_text_font(desc, &lv_font_montserrat_22, 0);
		lv_obj_align(desc, LV_ALIGN_BOTTOM_MID, 0, -6);

		/* nudge symbol label up so it doesn't overlap desc */
		lv_obj_align(lv_obj_get_child(btn, 0), LV_ALIGN_CENTER, 0, -14);

		lv_obj_set_grid_cell(btn,
				     LV_GRID_ALIGN_STRETCH, i % ACT_COLS, 1,
				     LV_GRID_ALIGN_STRETCH, i / ACT_COLS, 1);
	}

#undef ACT_COLS
#undef ACT_H
}

/* ── Top-level UI ─────────────────────────────────────────────────────────── */

static void create_ui(void)
{
	lv_obj_t *scr = lv_scr_act();
	lv_obj_set_style_bg_color(scr, lv_color_hex(COL_BG), 0);
	lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

	/* Title */
	lv_obj_t *title = lv_label_create(scr);
	lv_label_set_text(title, "Keypad Demo");
	lv_obj_set_style_text_color(title, lv_color_hex(COL_TEXT), 0);
	lv_obj_set_style_text_font(title, &lv_font_montserrat_28, 0);
	lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

	/* Status bar */
	lv_obj_t *sbar = lv_obj_create(scr);
	lv_obj_set_size(sbar, 460, 54);
	lv_obj_align(sbar, LV_ALIGN_TOP_MID, 0, 50);
	lv_obj_set_style_bg_color(sbar, lv_color_hex(COL_STATUS_BG), 0);
	lv_obj_set_style_border_color(sbar, lv_color_hex(COL_ACCENT), 0);
	lv_obj_set_style_border_width(sbar, 1, 0);
	lv_obj_set_style_radius(sbar, 8, 0);
	lv_obj_set_style_pad_all(sbar, 8, 0);
	lv_obj_clear_flag(sbar, LV_OBJ_FLAG_SCROLLABLE);

	status_label = lv_label_create(sbar);
	lv_label_set_recolor(status_label, true);
	lv_label_set_text(status_label, "#888888 Waiting for input...#");
	lv_obj_set_style_text_color(status_label, lv_color_hex(COL_TEXT), 0);
	lv_obj_set_style_text_font(status_label, &lv_font_montserrat_22, 0);
	lv_obj_center(status_label);

	/*
	 * Pixel budget (480 × 854):
	 *   title        y=10,  h=36  → bottom= 46
	 *   status bar   y=50,  h=54  → bottom=104
	 *   gap=8
	 *   Action panel y=112, h=296 → bottom=408
	 *     4 cols × 2 rows × 120px + 1×gap(10) + title(28) + 2×pad(10) = 296
	 *   gap=8
	 *   D-pad panel  y=416, h=426 → bottom=842
	 *     3×110 + 2×8 + title(28) + 2×pad(10) = 386 → round to 394
	 *   hint BOTTOM_MID -6       → ~848
	 */
	lv_obj_t *act  = make_panel(scr, "Action Keys", 460, 296, 112);
	build_action_keys(act);

	lv_obj_t *dpad = make_panel(scr, "D-Pad",       460, 394, 416);
	build_dpad(dpad);

	/* Hint footer */
	lv_obj_t *hint = lv_label_create(scr);
	lv_label_set_text(hint,
			  LV_SYMBOL_KEYBOARD
			  "  M H B S F T C  |  "
			  LV_SYMBOL_LEFT " " LV_SYMBOL_RIGHT
			  " " LV_SYMBOL_UP " " LV_SYMBOL_DOWN
			  "  arrow keys");
	lv_obj_set_style_text_font(hint, &lv_font_montserrat_22, 0);
	lv_obj_set_style_text_color(hint, lv_color_hex(0x556677), 0);
	lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -6);
}

/* ── Entry point ──────────────────────────────────────────────────────────── */

int main(void)
{
	const struct device *display_dev =
		DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

	if (!device_is_ready(display_dev)) {
		LOG_ERR("Display device not ready");
		return -ENODEV;
	}

	create_ui();

	lv_task_handler();
	display_blanking_off(display_dev);

	while (1) {
		lv_task_handler();
		k_sleep(K_MSEC(10));
	}

	return 0;
}
