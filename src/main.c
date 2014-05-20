/*
 * Copyright © 2006 Novell, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include <gdk/gdk.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#define NEED_BUTTON_BISTATES
#define NEED_BUTTON_STATE_FLAGS
#define NEED_BUTTON_ACTIONS
#define NEED_BUTTON_FILE_NAMES
#include <emerald.h>
#include <engine.h>

//#define BASE_PROP_SIZE 12
//#define QUAD_PROP_SIZE 9

#ifndef DECOR_INTERFACE_VERSION
#define DECOR_INTERFACE_VERSION 0
#endif

#if defined (HAVE_LIBWNCK_2_19_4)
#define wnck_window_get_geometry wnck_window_get_client_window_geometry
#endif

void reload_all_settings(int sig);

GdkPixmap *pdeb;
static gboolean do_reload = FALSE;

static GdkPixmap *create_pixmap(int w, int h)
{
    GdkPixmap *pixmap;
    GdkVisual *visual;
    GdkColormap *colormap;

    visual = gdk_visual_get_best_with_depth(32);
    if (!visual)
	return NULL;

    pixmap = gdk_pixmap_new(NULL, w, h, 32);
    if (!pixmap)
	return NULL;

    colormap = gdk_colormap_new(visual, FALSE);
    if (!colormap)
    {
	g_object_unref (G_OBJECT (pixmap));
	return NULL;
    }

    gdk_drawable_set_colormap(GDK_DRAWABLE(pixmap), colormap);
    g_object_unref (G_OBJECT (colormap));

    return pixmap;
}

void engine_draw_frame(decor_t * d, cairo_t * cr);
gboolean load_engine(gchar * engine_name, window_settings * ws);
void load_engine_settings(GKeyFile * f, window_settings * ws);

static Atom frame_window_atom;
static Atom win_decor_atom;
static Atom win_blur_decor_atom;
static Atom wm_move_resize_atom;
static Atom restack_window_atom;
static Atom select_window_atom;
static Atom net_wm_context_help_atom;
static Atom wm_protocols_atom;
static Atom mwm_hints_atom;
static Atom switcher_fg_atom;

static Atom toolkit_action_atom;
static Atom toolkit_action_window_menu_atom;
static Atom toolkit_action_force_quit_dialog_atom;

static Atom emerald_sigusr1_atom;

static Atom utf8_string_atom;

static Time dm_sn_timestamp;

#define C(name) { 0, XC_ ## name }
#define BUTTON_NOT_VISIBLE(ddd, xxx) \
    ((ddd)->tobj_item_state[(xxx)] == 3 || !((ddd)->actions & button_actions[(xxx)]))

static struct _cursor
{
    Cursor cursor;
    unsigned int shape;
} cursor[3][3] =
{
    {
	C(top_left_corner), C(top_side), C(top_right_corner)},
    {
	C(left_side), C(left_ptr), C(right_side)},
    {
	C(bottom_left_corner), C(bottom_side), C(bottom_right_corner)}
}, button_cursor = C(hand2);


static char *program_name;

static GtkWidget *style_window;

static GHashTable *frame_table;
static GtkWidget *action_menu = NULL;
static gboolean action_menu_mapped = FALSE;
static gint double_click_timeout = 250;

static GtkWidget *tip_window;
static GtkWidget *tip_label;
static GTimeVal tooltip_last_popdown = { -1, -1 };
static gint tooltip_timer_tag = 0;

static GSList *draw_list = NULL;
static guint draw_idle_id = 0;

static gboolean enable_tooltips = TRUE;
static gchar *engine = NULL;

static gint get_b_offset(gint b)
{
    static int boffset[B_COUNT+1];
    gint i, b_t = 0;

    for (i = 0; i < B_COUNT; i++)
    {
	boffset[i] = b_t;
	if (btbistate[b_t])
	{
	    boffset[i+1] = b_t;
	    i++;
	}
	b_t++;
    }
    return boffset[b];
}
static gint get_b_t_offset(gint b_t)
{
    static int btoffset[B_T_COUNT];
    gint i, b = 0;

    for (i = 0; i < B_T_COUNT; i++)
    {
	btoffset[i] = b;
	b++;
	if (btbistate[i])
	    b++;
    }
    return btoffset[b_t];
}

window_settings *global_ws;
static gint get_real_pos(window_settings * ws, gint tobj, decor_t * d)
{
    switch (d->tobj_item_state[tobj])
    {
	case 1:
	    return ((d->width + ws->left_space - ws->right_space +
		     d->tobj_size[0] - d->tobj_size[1] - d->tobj_size[2]) / 2 +
		    d->tobj_item_pos[tobj]);
	case 2:
	    return (d->width - ws->right_space - d->tobj_size[2] +
		    d->tobj_item_pos[tobj]);
	case 3:
	    return -1;
	default:
	    return (ws->left_space + d->tobj_item_pos[tobj]);
    }
}

static void update_window_extents(window_settings * ws)
{
    //where 4 is v_corn_rad (8 is 2*4), 6 is...?
    // 0,       0,          L_EXT+4,    TT_H+4,     0,0,0,0
    // L_EXT+4  0,          -8,         T_EXT+2,    0,0,1,0
    // L_EXT-4, 0,          R_EXT+4,    TT_H+4,     1,0,0,0
    // 0,       T_EXT+6,    L_EXT,      TT_H-6,     0,0,0,1
    // L_EXT,   T_EXT+2,    0,          TT_H-2,     0,0,1,0
    // L_EXT,   T_EXT+6,    R_EXT,      TT_H-6,     1,0,0,1
    // 0,       TT_H,       L_EXT+4,    B_EXT+4,    0,1,0,0
    // L_EXT+4, TT_H+4,     -8,         B_EXT,      0,1,1,0
    // L_EXT-4, TT_H,       R_EXT+4,    B_EXT+4,    1,1,0,0
    gint l_ext = ws->win_extents.left;
    gint r_ext = ws->win_extents.right;
    gint t_ext = ws->win_extents.top;
    gint b_ext = ws->win_extents.bottom;
    gint tt_h = ws->titlebar_height;

    /*pos_t newpos[3][3] = {
      {
      {  0,  0, 10, 21,   0, 0, 0, 0 },
      { 10,  0, -8,  6,   0, 0, 1, 0 },
      {  2,  0, 10, 21,   1, 0, 0, 0 }
      }, {
      {  0, 10,  6, 11,   0, 0, 0, 1 },
      {  6,  6,  0, 15,   0, 0, 1, 0 },
      {  6, 10,  6, 11,   1, 0, 0, 1 }
      }, {
      {  0, 17, 10, 10,   0, 1, 0, 0 },
      { 10, 21, -8,  6,   0, 1, 1, 0 },
      {  2, 17, 10, 10,   1, 1, 0, 0 }
      }
      }; */
    pos_t newpos[3][3] = { {
	{0, 0, l_ext + 4, tt_h + 4, 0, 0, 0, 0},
	    {l_ext + 4, 0, -8, t_ext + 2, 0, 0, 1, 0},
	    {l_ext - 4, 0, r_ext + 4, tt_h + 4, 1, 0, 0, 0}
    }, {
	{0, t_ext + 6, l_ext, tt_h - 6, 0, 0, 0, 1},
	    {l_ext, t_ext + 2, 0, tt_h - 2, 0, 0, 1, 0},
	    {l_ext, t_ext + 6, r_ext, tt_h - 6, 1, 0, 0, 1}
    }, {
	{0, tt_h, l_ext + 4, b_ext + 4, 0, 1, 0,
	    0},
	    {l_ext + 4, tt_h + 4, -8, b_ext, 0, 1, 1,
		0},
	    {l_ext - 4, tt_h, r_ext + 4, b_ext + 4, 1,
		1, 0, 0}
    }
    };
    memcpy(ws->pos, newpos, sizeof(pos_t) * 9);
}

static int my_add_quad_row(decor_quad_t * q,
			   int width,
			   int left,
			   int right, int ypush, int vgrav, int x0, int y0)
{
    int p1y = (vgrav == GRAVITY_NORTH) ? -ypush : 0;
    int p2y = (vgrav == GRAVITY_NORTH) ? 0 : ypush - 1;

    int fwidth = width - (left + right);

    q->p1.x = -left;
    q->p1.y = p1y;				// opt: never changes
    q->p1.gravity = vgrav | GRAVITY_WEST;
    q->p2.x = 0;
    q->p2.y = p2y;
    q->p2.gravity = vgrav | GRAVITY_WEST;
    q->align = 0;				// opt: never changes
    q->clamp = 0;
    q->stretch = 0;
    q->max_width = left;
    q->max_height = ypush;		// opt: never changes
    q->m.x0 = x0;
    q->m.y0 = y0;				// opt: never changes
    q->m.xx = 1;				// opt: never changes
    q->m.xy = 0;
    q->m.yy = 1;
    q->m.yx = 0;				// opt: never changes

    q++;

    q->p1.x = 0;
    q->p1.y = p1y;
    q->p1.gravity = vgrav | GRAVITY_WEST;
    q->p2.x = 0;
    q->p2.y = p2y;
    q->p2.gravity = vgrav | GRAVITY_EAST;
    q->align = ALIGN_LEFT | ALIGN_TOP;	
    q->clamp = 0;
    q->stretch = STRETCH_X;
    q->max_width = fwidth;
    q->max_height = ypush;
    q->m.x0 = x0 + left;
    q->m.y0 = y0;
    q->m.xx = 1;
    q->m.xy = 0;
    q->m.yy = 1;
    q->m.yx = 0;

    q++;

    q->p1.x = 0;
    q->p1.y = p1y;
    q->p1.gravity = vgrav | GRAVITY_EAST;
    q->p2.x = right;
    q->p2.y = p2y;
    q->p2.gravity = vgrav | GRAVITY_EAST;
    q->max_width = right;
    q->max_height = ypush;
    q->align = 0;
    q->clamp = 0;
    q->stretch = 0;
    q->m.x0 = x0 + left + fwidth;
    q->m.y0 = y0;
    q->m.xx = 1;
    q->m.yy = 1;
    q->m.xy = 0;
    q->m.yx = 0;

    return 3;
}
static int my_add_quad_col(decor_quad_t * q,
			   int height, int xpush, int hgrav, int x0, int y0)
{
    int p1x = (hgrav == GRAVITY_WEST) ? -xpush : 0;
    int p2x = (hgrav == GRAVITY_WEST) ? 0 : xpush;

    q->p1.x = p1x;
    q->p1.y = 0;
    q->p1.gravity = GRAVITY_NORTH | hgrav;
    q->p2.x = p2x;
    q->p2.y = 0;
    q->p2.gravity = GRAVITY_SOUTH | hgrav;
    q->max_width = xpush;
    q->max_height = height;
    q->align = 0;
    q->clamp = CLAMP_VERT;
    q->stretch = STRETCH_Y;
    q->m.x0 = x0;
    q->m.y0 = y0;
    q->m.xx = 1;
    q->m.xy = 0;
    q->m.yy = 1;
    q->m.yx = 0;

    return 1;
}
static int
my_set_window_quads(decor_quad_t * q,
		    int width,
		    int height,
		    window_settings * ws,
		    gboolean max_horz, gboolean max_vert)
{
    int nq;
    int mnq = 0;

    if (!max_vert || !max_horz || !ws->use_decoration_cropping)
    {
	//TOP QUAD
	nq = my_add_quad_row(q, width, ws->left_space, ws->right_space,
			     ws->titlebar_height + ws->top_space,
			     GRAVITY_NORTH, 0, 0);
	q += nq;
	mnq += nq;


	//BOTTOM QUAD
	nq = my_add_quad_row(q, width, ws->left_space, ws->right_space,
			     ws->bottom_space, GRAVITY_SOUTH, 0,
			     height - ws->bottom_space);
	q += nq;
	mnq += nq;

	nq = my_add_quad_col(q, height -
			     (ws->titlebar_height + ws->top_space +
			      ws->bottom_space), ws->left_space, GRAVITY_WEST,
			     0, ws->top_space + ws->titlebar_height);
	q += nq;
	mnq += nq;

	nq = my_add_quad_col(q, height -
			     (ws->titlebar_height + ws->top_space +
			      ws->bottom_space), ws->right_space,
			     GRAVITY_EAST, width - ws->right_space,
			     ws->top_space + ws->titlebar_height);
	q += nq;
	mnq += nq;
    }
    else
    {
	q->p1.x = 0;
	q->p1.y = -(ws->titlebar_height + ws->win_extents.top);
	q->p1.gravity = GRAVITY_NORTH | GRAVITY_WEST;
	q->p2.x = 0;
	q->p2.y = 0;
	q->p2.gravity = GRAVITY_NORTH | GRAVITY_EAST;
	q->max_width = width - (ws->left_space + ws->right_space);
	q->max_height = ws->titlebar_height + ws->win_extents.top;
	q->align = ALIGN_LEFT | ALIGN_TOP;
	q->clamp = 0;
	q->stretch = STRETCH_X;
	q->m.x0 = ws->left_space;
	q->m.y0 = ws->top_space - ws->win_extents.top;
	q->m.xx = 1;
	q->m.xy = 0;
	q->m.yy = 1;
	q->m.yx = 0;
	q++;
	mnq++;
    }

    return mnq;
}

static void
decor_update_blur_property (decor_t *d,
			    int     width,
			    int     height,
			    Region  top_region,
			    int     top_offset,
			    Region  bottom_region,
			    int     bottom_offset,
			    Region  left_region,
			    int     left_offset,
			    Region  right_region,
			    int     right_offset)
{
    Display *xdisplay = GDK_DISPLAY_XDISPLAY (gdk_display_get_default ());
    long    *data = NULL;
    int     size = 0;
    window_settings *ws = d->fs->ws;

    if (ws->blur_type != BLUR_TYPE_ALL)
    {
	bottom_region = NULL;
	left_region   = NULL;
	right_region  = NULL;

	if (ws->blur_type != BLUR_TYPE_TITLEBAR)
	    top_region = NULL;
    }

    if (top_region)
	size += top_region->numRects;
    if (bottom_region)
	size += bottom_region->numRects;
    if (left_region)
	size += left_region->numRects;
    if (right_region)
	size += right_region->numRects;

    if (size)
	data = malloc (sizeof (long) * (2 + size * 6));

    if (data)
    {
	decor_region_to_blur_property (data, 4, 0, width, height,
				       top_region, top_offset,
				       bottom_region, bottom_offset,
				       left_region, left_offset,
				       right_region, right_offset);

	gdk_error_trap_push ();
	XChangeProperty (xdisplay, d->prop_xid,
			 win_blur_decor_atom,
			 XA_INTEGER,
			 32, PropModeReplace, (guchar *) data,
			 2 + size * 6);
	XSync(xdisplay, FALSE);
	gdk_error_trap_pop ();

	free (data);
    }
    else
    {
	gdk_error_trap_push ();
	XDeleteProperty (xdisplay, d->prop_xid, win_blur_decor_atom);
	XSync(xdisplay, FALSE);
	gdk_error_trap_pop ();
    }
}

static void decor_update_window_property(decor_t * d)
{
    long data[256];
    Display *xdisplay = GDK_DISPLAY_XDISPLAY(gdk_display_get_default());
    window_settings *ws = d->fs->ws;
    decor_extents_t maxextents;
    decor_extents_t extents = ws->win_extents;
    gint nQuad;
    decor_quad_t quads[N_QUADS_MAX];
    int		    w, h;
    gint	    stretch_offset;
    REGION	    top, bottom, left, right;

    w = d->width;
    h = d->height;

    stretch_offset = 60;

    nQuad = my_set_window_quads(quads, d->width, d->height, ws,
				d->state & WNCK_WINDOW_STATE_MAXIMIZED_HORIZONTALLY,
				d->state & WNCK_WINDOW_STATE_MAXIMIZED_VERTICALLY);

    extents.top += ws->titlebar_height;

    if (ws->use_decoration_cropping)
    {
	maxextents.left = 0;
	maxextents.right = 0;
	maxextents.top = ws->titlebar_height + ws->win_extents.top;
	maxextents.bottom = 0;
    }
    else
	maxextents = extents;

    decor_quads_to_property(data, GDK_PIXMAP_XID(d->pixmap),
			    &extents, &maxextents, 0, 0, quads, nQuad);

    gdk_error_trap_push();
    XChangeProperty(xdisplay, d->prop_xid,
		    win_decor_atom,
		    XA_INTEGER,
		    32, PropModeReplace, (guchar *) data,
		    BASE_PROP_SIZE + QUAD_PROP_SIZE * nQuad);
    XSync(xdisplay, FALSE);
    gdk_error_trap_pop();

    top.rects = &top.extents;
    top.numRects = top.size = 1;

    top.extents.x1 = -extents.left;
    top.extents.y1 = -extents.top;
    top.extents.x2 = w + extents.right;
    top.extents.y2 = 0;

    bottom.rects = &bottom.extents;
    bottom.numRects = bottom.size = 1;

    bottom.extents.x1 = -extents.left;
    bottom.extents.y1 = 0;
    bottom.extents.x2 = w + extents.right;
    bottom.extents.y2 = extents.bottom;

    left.rects = &left.extents;
    left.numRects = left.size = 1;

    left.extents.x1 = -extents.left;
    left.extents.y1 = 0;
    left.extents.x2 = 0;
    left.extents.y2 = h;

    right.rects = &right.extents;
    right.numRects = right.size = 1;

    right.extents.x1 = 0;
    right.extents.y1 = 0;
    right.extents.x2 = extents.right;
    right.extents.y2 = h;

    decor_update_blur_property (d,
				w, h,
				&top, stretch_offset,
				&bottom, w / 2,
				&left, h / 2,
				&right, h / 2);	
}

static int
set_switcher_quads(decor_quad_t * q, int width, int height, window_settings * ws)
{
    gint n, nQuad = 0;

    /* 1 top quads */
    q->p1.x = -ws->left_space;
    q->p1.y = -ws->top_space - SWITCHER_TOP_EXTRA;
    q->p1.gravity = GRAVITY_NORTH | GRAVITY_WEST;
    q->p2.x = ws->right_space;
    q->p2.y = 0;
    q->p2.gravity = GRAVITY_NORTH | GRAVITY_EAST;
    q->max_width = SHRT_MAX;
    q->max_height = SHRT_MAX;
    q->align = 0;
    q->clamp = 0;
    q->stretch = 0;
    q->m.xx = 1;
    q->m.xy = 0;
    q->m.yx = 0;
    q->m.yy = 1;
    q->m.x0 = 0;
    q->m.y0 = 0;

    q++;
    nQuad++;

    /* left quads */
    n = decor_set_vert_quad_row(q,
				0, ws->switcher_top_corner_space, 0, ws->bottom_corner_space,
				-ws->left_space, 0, GRAVITY_WEST,
				height - ws->top_space - ws->titlebar_height - ws->bottom_space,
				(ws->switcher_top_corner_space - ws->switcher_bottom_corner_space) >> 1,
				0, 0.0, ws->top_space + SWITCHER_TOP_EXTRA, FALSE);

    q += n;
    nQuad += n;

    /* right quads */
    n = decor_set_vert_quad_row(q,
				0, ws->switcher_top_corner_space, 0, ws->switcher_bottom_corner_space,
				0, ws->right_space, GRAVITY_EAST,
				height - ws->top_space - ws->titlebar_height - ws->bottom_space,
				(ws->switcher_top_corner_space - ws->switcher_bottom_corner_space) >> 1,
				0, width - ws->right_space, ws->top_space + SWITCHER_TOP_EXTRA, FALSE);

    q += n;
    nQuad += n;

    /* 1 bottom quad */
    q->p1.x = -ws->left_space;
    q->p1.y = 0;
    q->p1.gravity = GRAVITY_SOUTH | GRAVITY_WEST;
    q->p2.x = ws->right_space;
    q->p2.y = ws->bottom_space + SWITCHER_SPACE;
    q->p2.gravity = GRAVITY_SOUTH | GRAVITY_EAST;
    q->max_width = SHRT_MAX;
    q->max_height = SHRT_MAX;
    q->align = 0;
    q->clamp = 0;
    q->stretch = 0;
    q->m.xx = 1;
    q->m.xy = 0;
    q->m.yx = 0;
    q->m.yy = 1;
    q->m.x0 = 0;
    q->m.y0 = height - ws->bottom_space - SWITCHER_SPACE;

    nQuad++;

    return nQuad;
}

static int
set_shadow_quads(decor_quad_t * q, gint width, gint height, window_settings * ws)
{
    gint n, nQuad = 0;

    /* top quads */
    n = decor_set_horz_quad_line(q,
				 ws->shadow_left_space, ws->shadow_left_corner_space,
				 ws->shadow_right_space, ws->shadow_right_corner_space,
				 -ws->shadow_top_space, 0, GRAVITY_NORTH, width,
				 (ws->shadow_left_corner_space - ws->shadow_right_corner_space) >> 1,
				 0, 0.0, 0.0);

    q += n;
    nQuad += n;

    /* left quads */
    n = decor_set_vert_quad_row(q,
				0, ws->shadow_top_corner_space, 0, ws->shadow_bottom_corner_space,
				-ws->shadow_left_space, 0, GRAVITY_WEST,
				height - ws->shadow_top_space - ws->shadow_bottom_space,
				(ws->shadow_top_corner_space - ws->shadow_bottom_corner_space) >> 1,
				0, 0.0, ws->shadow_top_space, FALSE);

    q += n;
    nQuad += n;

    /* right quads */
    n = decor_set_vert_quad_row(q,
				0, ws->shadow_top_corner_space, 0, ws->shadow_bottom_corner_space, 0,
				ws->shadow_right_space, GRAVITY_EAST,
				height - ws->shadow_top_space - ws->shadow_bottom_space,
				(ws->shadow_top_corner_space - ws->shadow_bottom_corner_space) >> 1,
				0, width - ws->shadow_right_space, ws->shadow_top_space, FALSE);

    q += n;
    nQuad += n;

    /* bottom quads */
    n = decor_set_horz_quad_line(q,
				 ws->shadow_left_space, ws->shadow_left_corner_space,
				 ws->shadow_right_space, ws->shadow_right_corner_space, 0,
				 ws->shadow_bottom_space, GRAVITY_SOUTH, width,
				 (ws->shadow_left_corner_space - ws->shadow_right_corner_space) >> 1,
				 0, 0.0, ws->shadow_top_space + ws->shadow_top_corner_space +
				 ws->shadow_bottom_corner_space + 1.0);

    nQuad += n;

    return nQuad;
}

static void
gdk_cairo_set_source_color_alpha(cairo_t * cr, GdkColor * color, double alpha)
{
    cairo_set_source_rgba(cr,
			  color->red / 65535.0,
			  color->green / 65535.0,
			  color->blue / 65535.0, alpha);
}

static void draw_shadow_background(decor_t * d, cairo_t * cr)
{
    cairo_matrix_t matrix;
    double w, h, x2, y2;
    gint width, height;
    gint left, right, top, bottom;
    window_settings *ws = d->fs->ws;

    if (!ws->large_shadow_pixmap)
    {
	cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.0);
	cairo_paint(cr);

	return;
    }

    gdk_drawable_get_size(ws->large_shadow_pixmap, &width, &height);

    left = ws->left_space + ws->left_corner_space;
    right = ws->right_space + ws->right_corner_space;
    top = ws->top_space + ws->top_corner_space;
    bottom = ws->bottom_space + ws->bottom_corner_space;

    if (d->width - left - right < 0)
    {
	left = d->width / 2;
	right = d->width - left;
    }

    if (d->height - top - bottom < 0)
    {
	top = d->height / 2;
	bottom = d->height - top;
    }

    w = d->width - left - right;
    h = d->height - top - bottom;

    x2 = d->width - right;
    y2 = d->height - bottom;

    /* top left */
    cairo_matrix_init_identity(&matrix);
    cairo_pattern_set_matrix(ws->shadow_pattern, &matrix);
    cairo_set_source(cr, ws->shadow_pattern);
    cairo_rectangle(cr, 0.0, 0.0, left, top);
    cairo_fill(cr);

    /* top */
    if (w > 0)
    {
	cairo_matrix_init_translate(&matrix, left, 0.0);
	cairo_matrix_scale(&matrix, 1.0 / w, 1.0);
	cairo_matrix_translate(&matrix, -left, 0.0);
	cairo_pattern_set_matrix(ws->shadow_pattern, &matrix);
	cairo_set_source(cr, ws->shadow_pattern);
	cairo_rectangle(cr, left, 0.0, w, top);
	cairo_fill(cr);
    }

    /* top right */
    cairo_matrix_init_translate(&matrix, width - right - x2, 0.0);
    cairo_pattern_set_matrix(ws->shadow_pattern, &matrix);
    cairo_set_source(cr, ws->shadow_pattern);
    cairo_rectangle(cr, x2, 0.0, right, top);
    cairo_fill(cr);

    /* left */
    if (h > 0)
    {
	cairo_matrix_init_translate(&matrix, 0.0, top);
	cairo_matrix_scale(&matrix, 1.0, 1.0 / h);
	cairo_matrix_translate(&matrix, 0.0, -top);
	cairo_pattern_set_matrix(ws->shadow_pattern, &matrix);
	cairo_set_source(cr, ws->shadow_pattern);
	cairo_rectangle(cr, 0.0, top, left, h);
	cairo_fill(cr);
    }

    /* right */
    if (h > 0)
    {
	cairo_matrix_init_translate(&matrix, width - right - x2, top);
	cairo_matrix_scale(&matrix, 1.0, 1.0 / h);
	cairo_matrix_translate(&matrix, 0.0, -top);
	cairo_pattern_set_matrix(ws->shadow_pattern, &matrix);
	cairo_set_source(cr, ws->shadow_pattern);
	cairo_rectangle(cr, x2, top, right, h);
	cairo_fill(cr);
    }

    /* bottom left */
    cairo_matrix_init_translate(&matrix, 0.0, height - bottom - y2);
    cairo_pattern_set_matrix(ws->shadow_pattern, &matrix);
    cairo_set_source(cr, ws->shadow_pattern);
    cairo_rectangle(cr, 0.0, y2, left, bottom);
    cairo_fill(cr);

    /* bottom */
    if (w > 0)
    {
	cairo_matrix_init_translate(&matrix, left, height - bottom - y2);
	cairo_matrix_scale(&matrix, 1.0 / w, 1.0);
	cairo_matrix_translate(&matrix, -left, 0.0);
	cairo_pattern_set_matrix(ws->shadow_pattern, &matrix);
	cairo_set_source(cr, ws->shadow_pattern);
	cairo_rectangle(cr, left, y2, w, bottom);
	cairo_fill(cr);
    }

    /* bottom right */
    cairo_matrix_init_translate(&matrix, width - right - x2,
				height - bottom - y2);
    cairo_pattern_set_matrix(ws->shadow_pattern, &matrix);
    cairo_set_source(cr, ws->shadow_pattern);
    cairo_rectangle(cr, x2, y2, right, bottom);
    cairo_fill(cr);
}


static void draw_help_button(decor_t * d, cairo_t * cr, double s)
{
    cairo_rel_move_to(cr, 0.0, 6.0);
    cairo_rel_line_to(cr, 0.0, 3.0);
    cairo_rel_line_to(cr, 4.5, 0.0);
    cairo_rel_line_to(cr, 0.0, 4.5);
    cairo_rel_line_to(cr, 3.0, 0.0);
    cairo_rel_line_to(cr, 0.0, -4.5);

    cairo_rel_line_to(cr, 4.5, 0.0);

    cairo_rel_line_to(cr, 0.0, -3.0);
    cairo_rel_line_to(cr, -4.5, 0.0);
    cairo_rel_line_to(cr, 0.0, -4.5);
    cairo_rel_line_to(cr, -3.0, 0.0);
    cairo_rel_line_to(cr, 0.0, 4.5);

    cairo_close_path(cr);
}
static void draw_close_button(decor_t * d, cairo_t * cr, double s)
{
    cairo_rel_move_to(cr, 0.0, s);

    cairo_rel_line_to(cr, s, -s);
    cairo_rel_line_to(cr, s, s);
    cairo_rel_line_to(cr, s, -s);
    cairo_rel_line_to(cr, s, s);

    cairo_rel_line_to(cr, -s, s);
    cairo_rel_line_to(cr, s, s);
    cairo_rel_line_to(cr, -s, s);
    cairo_rel_line_to(cr, -s, -s);

    cairo_rel_line_to(cr, -s, s);
    cairo_rel_line_to(cr, -s, -s);
    cairo_rel_line_to(cr, s, -s);

    cairo_close_path(cr);
}

static void draw_max_button(decor_t * d, cairo_t * cr, double s)
{
    cairo_set_fill_rule(cr, CAIRO_FILL_RULE_EVEN_ODD);

    cairo_rel_line_to(cr, 12.0, 0.0);
    cairo_rel_line_to(cr, 0.0, 12.0);
    cairo_rel_line_to(cr, -12.0, 0.0);

    cairo_close_path(cr);

    cairo_rel_move_to(cr, 2.0, s);

    cairo_rel_line_to(cr, 8.0, 0.0);
    cairo_rel_line_to(cr, 0.0, 10.0 - s);
    cairo_rel_line_to(cr, -8.0, 0.0);

    cairo_close_path(cr);
}

static void draw_unmax_button(decor_t * d, cairo_t * cr, double s)
{
    cairo_set_fill_rule(cr, CAIRO_FILL_RULE_EVEN_ODD);

    cairo_rel_move_to(cr, 1.0, 1.0);

    cairo_rel_line_to(cr, 10.0, 0.0);
    cairo_rel_line_to(cr, 0.0, 10.0);
    cairo_rel_line_to(cr, -10.0, 0.0);

    cairo_close_path(cr);

    cairo_rel_move_to(cr, 2.0, s);

    cairo_rel_line_to(cr, 6.0, 0.0);
    cairo_rel_line_to(cr, 0.0, 8.0 - s);
    cairo_rel_line_to(cr, -6.0, 0.0);

    cairo_close_path(cr);
}

static void draw_min_button(decor_t * d, cairo_t * cr, double s)
{
    cairo_rel_move_to(cr, 0.0, 8.0);

    cairo_rel_line_to(cr, 12.0, 0.0);
    cairo_rel_line_to(cr, 0.0, s);
    cairo_rel_line_to(cr, -12.0, 0.0);

    cairo_close_path(cr);
}

typedef void (*draw_proc) (cairo_t * cr);
static void
get_button_pos(window_settings * ws, gint b_t,
	       decor_t * d, gdouble y1, gdouble * rx, gdouble * ry)
{
    //y1 - 4.0 + ws->titlebar_height / 2,
    *ry = y1 + ws->button_offset;
    *rx = get_real_pos(ws, b_t, d);
}
static void
button_state_paint(cairo_t * cr,
		   alpha_color * color, alpha_color * color_2, guint state)
{
    double alpha;

    if (state & IN_EVENT_WINDOW)
	alpha = 1.0;
    else
	alpha = color->alpha;

    if ((state & (PRESSED_EVENT_WINDOW | IN_EVENT_WINDOW))
	== (PRESSED_EVENT_WINDOW | IN_EVENT_WINDOW))
    {
	cairo_set_source_rgba(cr, color->color.r, color->color.g,
			      color->color.b, alpha);

	cairo_fill_preserve(cr);

	cairo_set_source_alpha_color(cr, color_2);

	cairo_set_line_width(cr, 1.0);
	cairo_stroke(cr);
	cairo_set_line_width(cr, 2.0);
    }
    else
    {
	cairo_set_source_alpha_color(cr, color_2);
	cairo_stroke_preserve(cr);

	cairo_set_source_rgba(cr, color->color.r, color->color.g,
			      color->color.b, alpha);

	cairo_fill(cr);
    }
}
static int get_b_state(decor_t * d, int button)
{
    int ret = d->active ? 0 : 3;

    if (d->button_states[button] & IN_EVENT_WINDOW)
    {
	ret++;
	if (d->button_states[button] & PRESSED_EVENT_WINDOW)
	    ret++;
    }
    return ret;
}
static void
draw_pixbuf(GdkPixbuf * pixbuf, cairo_t * cr,
	    gdouble x, gdouble y, gdouble x2, gdouble y2, gdouble alpha)
{
    cairo_save(cr);
    cairo_rectangle(cr, x, y, x2-x, y2-y);
    cairo_clip(cr);
    gdk_cairo_set_source_pixbuf(cr, pixbuf, x, y);
    cairo_paint_with_alpha(cr, alpha);
    cairo_restore(cr);
}
static void
draw_button_with_glow_alpha_bstate(gint b_t, decor_t * d, cairo_t * cr,
				   gint y1, gdouble button_alpha,
				   gdouble glow_alpha, int b_state)
{
    gint b = b_t;
    gdouble x, y;
    gdouble x2, y2;
    gdouble glow_x, glow_y;		// glow top left coordinates
    gdouble glow_x2, glow_y2;	// glow bottom right coordinates
    window_settings *ws = d->fs->ws;

    if (b_state < 0)
	b_state = get_b_state(d, b_t);

    b = get_b_t_offset(b_t);

    if (btbistate[b_t])
	if (d->state & btstateflag[b_t])
	    b++;

    if (BUTTON_NOT_VISIBLE(d, b_t))
	return;
    button_region_t *button_region =
	(d->active ? &d->button_region[b_t] : &d->
	 button_region_inact[b_t]);
    x = button_region->base_x1;
    y = button_region->base_y1;

    if (ws->use_pixmap_buttons)
    {
	x2 = button_region->base_x2;
	y2 = button_region->base_y2;
	draw_pixbuf(ws->ButtonPix[b_state + b * S_COUNT], cr, x, y, x2, y2,
		    button_alpha);

	if (glow_alpha > 1e-5)	// i.e. glow is on
	{
	    glow_x = button_region->glow_x1;
	    glow_y = button_region->glow_y1;
	    glow_x2 = button_region->glow_x2;
	    glow_y2 = button_region->glow_y2;
	    if (d->active)
	    {					// Draw glow
		draw_pixbuf(ws->ButtonGlowPix[b], cr, glow_x, glow_y, glow_x2,
			    glow_y2, glow_alpha);
	    }
	    else				// assume this function won't be called with glow_alpha>0
	    {					// if ws->use_inactive_glow is false
		// Draw inactive glow
		draw_pixbuf(ws->ButtonInactiveGlowPix[b], cr, glow_x, glow_y,
			    glow_x2, glow_y2, glow_alpha);
	    }
	}
    }
    else
    {
	y += 3;
	cairo_set_line_width(cr, 2.0);
	cairo_move_to(cr, x, y);
	switch (b)
	{
	    case B_CLOSE:
		draw_close_button(d, cr, 3.1);
		break;
	    case B_MAXIMIZE:
		draw_max_button(d, cr, 4.0);
		break;
	    case B_RESTORE:
		draw_unmax_button(d, cr, 4.0);
		break;
	    case B_MINIMIZE:
		draw_min_button(d, cr, 4.0);
		break;
	    case B_HELP:
		cairo_move_to(cr, x, y);
		draw_help_button(d, cr, 3.1);
		break;
	    default:
		//FIXME - do something here
		break;
	}
	button_state_paint(cr, &d->fs->button, &d->fs->button_halo,
			   d->button_states[b_t]);
    }
}
static void
draw_button_with_glow(gint b_t, decor_t * d, cairo_t * cr, gint y1,
		      gboolean with_glow)
{
    draw_button_with_glow_alpha_bstate(b_t, d, cr, y1, 1.0,
				       (with_glow ? 1.0 : 0.0), -1);
}
static void draw_button(gint b_t, decor_t * d, cairo_t * cr, gint y1)
{
    draw_button_with_glow_alpha_bstate(b_t, d, cr, y1, 1.0, 0.0, -1);
}
static void reset_buttons_bg_and_fade(decor_t * d)
{
    d->draw_only_buttons_region = FALSE;
    d->button_fade_info.cr = NULL;
    d->button_fade_info.timer = -1;
    int b_t;

    for (b_t = 0; b_t < B_T_COUNT; b_t++)
    {
	d->button_fade_info.counters[b_t] = 0;
	d->button_fade_info.pulsating[b_t] = 0;
	d->button_region[b_t].base_x1 = -100;
	d->button_region[b_t].glow_x1 = -100;
	if (d->button_region[b_t].bg_pixmap)
	    g_object_unref (G_OBJECT (d->button_region[b_t].bg_pixmap));
	d->button_region[b_t].bg_pixmap = NULL;
	d->button_region_inact[b_t].base_x1 = -100;
	d->button_region_inact[b_t].glow_x1 = -100;
	if (d->button_region_inact[b_t].bg_pixmap)
	    g_object_unref (G_OBJECT (d->button_region_inact[b_t].bg_pixmap));
	d->button_region_inact[b_t].bg_pixmap = NULL;
	d->button_last_drawn_state[b_t] = 0;
    }
}
static void stop_button_fade(decor_t * d)
{
    int j;

    if (d->button_fade_info.cr)
    {
	cairo_destroy(d->button_fade_info.cr);
	d->button_fade_info.cr = NULL;
    }
    if (d->button_fade_info.timer >= 0)
    {
	g_source_remove(d->button_fade_info.timer);
	d->button_fade_info.timer = -1;
    }
    for (j = 0; j < B_T_COUNT; j++)
	d->button_fade_info.counters[j] = 0;
}
static void draw_button_backgrounds(decor_t * d, int *necessary_update_type)
{
    int b_t;
    window_settings *ws = d->fs->ws;

    // Draw button backgrounds
    for (b_t = 0; b_t < B_T_COUNT; b_t++)
    {
	if (BUTTON_NOT_VISIBLE(d, b_t))
	    continue;
	button_region_t *button_region = (d->active ? &d->button_region[b_t] :
					  &d->button_region_inact[b_t]);
	gint src_x = 0, src_y = 0, w = 0, h = 0, dest_x = 0, dest_y = 0;

	if (necessary_update_type[b_t] == 1)
	{
	    w = button_region->base_x2 - button_region->base_x1;
	    h = button_region->base_y2 - button_region->base_y1;
	    if (ws->use_pixmap_buttons)
	    {
		dest_x = button_region->base_x1;
		dest_y = button_region->base_y1;
		if ((ws->use_button_glow && d->active) ||
		    (ws->use_button_inactive_glow && !d->active))
		{
		    src_x = button_region->base_x1 - button_region->glow_x1;
		    src_y = button_region->base_y1 - button_region->glow_y1;
		}
	    }
	    else
	    {
		dest_x = button_region->base_x1 - 2;
		dest_y = button_region->base_y1 + 1;
	    }
	}
	else if (necessary_update_type[b_t] == 2)
	{
	    dest_x = button_region->glow_x1;
	    dest_y = button_region->glow_y1;
	    w = button_region->glow_x2 - button_region->glow_x1;
	    h = button_region->glow_y2 - button_region->glow_y1;
	}
	else
	    return;
	if (button_region->bg_pixmap)
	    gdk_draw_drawable(IS_VALID(d->buffer_pixmap) ? d->buffer_pixmap :
			                                   d->pixmap,
			      d->gc, button_region->bg_pixmap, src_x, src_y,
			      dest_x, dest_y, w, h);
	d->min_drawn_buttons_region.x1 =
	    MIN(d->min_drawn_buttons_region.x1, dest_x);
	d->min_drawn_buttons_region.y1 =
	    MIN(d->min_drawn_buttons_region.y1, dest_y);
	d->min_drawn_buttons_region.x2 =
	    MAX(d->min_drawn_buttons_region.x2, dest_x + w);
	d->min_drawn_buttons_region.y2 =
	    MAX(d->min_drawn_buttons_region.y2, dest_y + h);
    }
}

gint draw_buttons_timer_func(gpointer data)
{
    button_fade_info_t *fade_info = (button_fade_info_t *) data;
    decor_t *d = (decor_t *) (fade_info->d);
    window_settings *ws = d->fs->ws;
    int num_steps = ws->button_fade_num_steps;

    /* decorations no longer available? */
    if (!d->buffer_pixmap && !d->pixmap)
    {
	stop_button_fade(d);
	return FALSE;
    }

    d->min_drawn_buttons_region.x1 = 10000;
    d->min_drawn_buttons_region.y1 = 10000;
    d->min_drawn_buttons_region.x2 = -100;
    d->min_drawn_buttons_region.y2 = -100;

    if (!fade_info->cr)
    {
	fade_info->cr =
	    gdk_cairo_create(GDK_DRAWABLE
			     (IS_VALID(d->buffer_pixmap) ? d->buffer_pixmap :
			                                   d->pixmap));
	cairo_set_operator(fade_info->cr, CAIRO_OPERATOR_OVER);
    }

    // Determine necessary updates
    int b_t;
    int necessary_update_type[B_T_COUNT];	// 0: none, 1: only base, 2: base+glow

    for (b_t = 0; b_t < B_T_COUNT; b_t++)
	necessary_update_type[b_t] = (ws->use_button_glow && d->active) || 
	    (ws->use_button_inactive_glow && !d->active) ? 2:1;
    draw_button_backgrounds(d, necessary_update_type);

    // Draw the buttons that are in "non-hovered" or pressed state
    for (b_t = 0; b_t < B_T_COUNT; b_t++)
    {
	if (BUTTON_NOT_VISIBLE(d, b_t) || fade_info->counters[b_t] ||
	    necessary_update_type[b_t] == 0)
	    continue;
	int b_state = get_b_state(d, b_t);
	int toBeDrawnState =
	    (d->
	     active ? (b_state == S_ACTIVE_PRESS ? 2 : 0) : (b_state ==
							     S_INACTIVE_PRESS
							     ? 5 : 3));
	draw_button_with_glow_alpha_bstate(b_t, d, fade_info->cr, fade_info->y1, 1.0, 0.0, toBeDrawnState);	// no glow here
    }

    // Draw the buttons that are in "hovered" state (fading in/out or at max fade)
    double button_alphas[B_T_COUNT];

    for (b_t = 0; b_t < B_T_COUNT; b_t++)
    {
	button_alphas[b_t] = 0;
	if (BUTTON_NOT_VISIBLE(d, b_t) ||
	    (!fade_info->pulsating[b_t] && !fade_info->counters[b_t]))
	    continue;

	if (ws->button_fade_pulse_len_steps > 0 && fade_info->counters[b_t] &&
	    fade_info->pulsating[b_t])
	{
	    // If it is time, reverse the fade
	    if (fade_info->counters[b_t] ==
		-num_steps + ws->button_fade_pulse_len_steps)
		fade_info->counters[b_t] = 1 - fade_info->counters[b_t];
	    if (fade_info->counters[b_t] ==
		num_steps + 1 + ws->button_fade_pulse_wait_steps)
		fade_info->counters[b_t] =
		    1 - MIN(fade_info->counters[b_t], num_steps + 1);
	}
	if (ws->button_fade_pulse_len_steps > 0 &&
	    fade_info->counters[b_t] == num_steps)
	    fade_info->pulsating[b_t] = TRUE;	// start pulse

	if (fade_info->counters[b_t] != num_steps + 1 ||	// unless fade is at max
	    (ws->button_fade_pulse_len_steps > 0 &&	// or at pulse max
	     fade_info->counters[b_t] !=
	     num_steps + 1 + ws->button_fade_pulse_wait_steps))
	{
	    fade_info->counters[b_t]++;	// increment fade counter
	}
	d->button_last_drawn_state[b_t] = fade_info->counters[b_t];

	gdouble alpha;

	if (fade_info->counters[b_t] > 0)
	    alpha = (MIN(fade_info->counters[b_t], num_steps + 1) -
		     1) / (gdouble) num_steps;
	else
	    alpha = -fade_info->counters[b_t] / (gdouble) num_steps;

	if (fade_info->counters[b_t] < num_steps + 1)	// not at max fade
	{
	    // Draw button's non-hovered version (with 1-alpha)
	    draw_button_with_glow_alpha_bstate(b_t, d, fade_info->cr,
					       fade_info->y1, pow(1 - alpha,
								  0.4), 0.0,
					       d->active ? 0 : 3);
	}
	button_alphas[b_t] = alpha;
    }
    for (b_t = 0; b_t < B_T_COUNT; b_t++)
    {
	if (button_alphas[b_t] > 1e-4)
	{
	    gdouble glow_alpha = 0.0;

	    if ((ws->use_button_glow && d->active) ||
		(ws->use_button_inactive_glow && !d->active))
		glow_alpha = button_alphas[b_t];

	    // Draw button's hovered version (with alpha)
	    draw_button_with_glow_alpha_bstate(b_t, d, fade_info->cr,
					       fade_info->y1,
					       button_alphas[b_t], glow_alpha,
					       d->active ? 1 : 4);
	}
    }

    // Check if the fade has come to an end
    gboolean any_active_buttons = FALSE;

    for (b_t = 0; b_t < B_T_COUNT; b_t++)
	if (!BUTTON_NOT_VISIBLE(d, b_t) &&
	    ((fade_info->counters[b_t] &&
	      fade_info->counters[b_t] < num_steps + 1) ||
	     fade_info->pulsating[b_t]))
	{
	    any_active_buttons = TRUE;
	    break;
	}

    if (IS_VALID(d->buffer_pixmap) && !d->button_fade_info.first_draw &&
	d->min_drawn_buttons_region.x1 < 10000)
    {
	// if region is updated at least once
	gdk_draw_drawable(d->pixmap,
			  d->gc,
			  d->buffer_pixmap,
			  d->min_drawn_buttons_region.x1,
			  d->min_drawn_buttons_region.y1,
			  d->min_drawn_buttons_region.x1,
			  d->min_drawn_buttons_region.y1,
			  d->min_drawn_buttons_region.x2 -
			  d->min_drawn_buttons_region.x1,
			  d->min_drawn_buttons_region.y2 -
			  d->min_drawn_buttons_region.y1);
    }
    fade_info->first_draw = FALSE;
    if (!any_active_buttons)
    {
	cairo_destroy(fade_info->cr);
	fade_info->cr = NULL;
	if (fade_info->timer >= 0)
	{
	    g_source_remove(fade_info->timer);
	    fade_info->timer = -1;
	}
	return FALSE;
    }
    return TRUE;
}
static void draw_buttons_with_fade(decor_t * d, cairo_t * cr, double y1)
{
    window_settings *ws = d->fs->ws;
    int b_t;

    for (b_t = 0; b_t < B_T_COUNT; b_t++)
    {
	if (BUTTON_NOT_VISIBLE(d, b_t))
	    continue;
	if (!(d->active ? d->button_region[b_t] : d->button_region_inact[b_t]).bg_pixmap)	// don't draw if bg_pixmaps are not valid
	    return;
    }
    button_fade_info_t *fade_info = &(d->button_fade_info);
    gboolean button_pressed = FALSE;

    for (b_t = 0; b_t < B_T_COUNT; b_t++)
    {
	if (BUTTON_NOT_VISIBLE(d, b_t))
	    continue;
	int b_state = get_b_state(d, b_t);

	if (fade_info->counters[b_t] != 0 &&
	    (b_state == S_ACTIVE_PRESS || b_state == S_INACTIVE_PRESS))
	{
	    // Button pressed, stop fade
	    fade_info->counters[b_t] = 0;
	    button_pressed = TRUE;
	}
	else if (fade_info->counters[b_t] > 0 && (b_state == S_ACTIVE || b_state == S_INACTIVE))	// moved out
	{
	    // Change fade in -> out and proceed 1 step
	    fade_info->counters[b_t] =
		1 - MIN(fade_info->counters[b_t],
			ws->button_fade_num_steps + 1);
	}
	else if (fade_info->counters[b_t] < 0 &&
		 (b_state == S_ACTIVE_HOVER || b_state == S_INACTIVE_HOVER))
	{
	    // Change fade out -> in and proceed 1 step
	    fade_info->counters[b_t] = 1 - fade_info->counters[b_t];
	}
	else if (fade_info->counters[b_t] == 0 &&
		 (b_state == S_ACTIVE_HOVER || b_state == S_INACTIVE_HOVER))
	{
	    // Start fade in
	    fade_info->counters[b_t] = 1;
	}
	if (fade_info->pulsating[b_t] &&
	    b_state != S_ACTIVE_HOVER && b_state != S_INACTIVE_HOVER)
	{
	    // Stop pulse
	    fade_info->pulsating[b_t] = FALSE;
	}
    }

    if (fade_info->timer == -1 || button_pressed)
	// button_pressed is needed because sometimes after a button is pressed,
	// this function is called twice, first with S_(IN)ACTIVE, then with S_(IN)ACTIVE_PRESS
	// where it should have been only once with S_(IN)ACTIVE_PRESS
    {
	fade_info->d = (gpointer) d;
	fade_info->y1 = y1;
	if (draw_buttons_timer_func((gpointer) fade_info) == TRUE)	// call once now
	{
	    // and start a new timer for the next step
	    fade_info->timer =
		g_timeout_add(ws->button_fade_step_duration,
			      draw_buttons_timer_func,
			      (gpointer) fade_info);
	}
    }
}
static void draw_buttons_without_fade(decor_t * d, cairo_t * cr, double y1)
{
    window_settings *ws = d->fs->ws;

    d->min_drawn_buttons_region.x1 = 10000;
    d->min_drawn_buttons_region.y1 = 10000;
    d->min_drawn_buttons_region.x2 = -100;
    d->min_drawn_buttons_region.y2 = -100;

    int b_t;
    int necessary_update_type[B_T_COUNT];	// 0: none, 1: only base, 2: base+glow

    for (b_t = 0; b_t < B_T_COUNT; b_t++)
	necessary_update_type[b_t] = (ws->use_button_glow && d->active) || 
	    (ws->use_button_inactive_glow && !d->active) ? 2:1;
    //necessary_update_type[b_t] = 2;

    draw_button_backgrounds(d, necessary_update_type);

    // Draw buttons
    gint button_hovered_on = -1;

    for (b_t = 0; b_t < B_T_COUNT; b_t++)
    {
	if (necessary_update_type[b_t] == 0)
	    continue;
	int b_state = get_b_state(d, b_t);

	if (ws->use_pixmap_buttons &&
	    ((ws->use_button_glow && b_state == S_ACTIVE_HOVER) ||
	     (ws->use_button_inactive_glow && b_state == S_INACTIVE_HOVER)))
	{
	    // skip the one being hovered on, if any
	    button_hovered_on = b_t;
	}
	else
	    draw_button(b_t, d, cr, y1);
    }
    if (button_hovered_on >= 0)
    {
	// Draw the button and the glow for the button hovered on
	draw_button_with_glow(button_hovered_on, d, cr, y1, TRUE);
    }
}
static void update_button_regions(decor_t * d)
{
    window_settings *ws = d->fs->ws;
    gint y1 = ws->top_space - ws->win_extents.top;

    gint b_t, b_t2;
    gdouble x, y;
    gdouble glow_x, glow_y;		// glow top left coordinates

    for (b_t = 0; b_t < B_T_COUNT; b_t++)
    {
	if (BUTTON_NOT_VISIBLE(d, b_t))
	    continue;
	button_region_t *button_region = &(d->button_region[b_t]);

	if (button_region->bg_pixmap)
	{
	    g_object_unref (G_OBJECT (button_region->bg_pixmap));
	    button_region->bg_pixmap = NULL;
	}
	if (d->button_region_inact[b_t].bg_pixmap)
	{
	    g_object_unref (G_OBJECT (d->button_region_inact[b_t].bg_pixmap));
	    d->button_region_inact[b_t].bg_pixmap = NULL;
	}
	// Reset overlaps
	for (b_t2 = 0; b_t2 < b_t; b_t2++)
	    if (!BUTTON_NOT_VISIBLE(d, b_t2))
		d->button_region[b_t].overlap_buttons[b_t2] = FALSE;
	for (b_t2 = 0; b_t2 < b_t; b_t2++)
	    if (!BUTTON_NOT_VISIBLE(d, b_t2))
		d->button_region_inact[b_t].overlap_buttons[b_t2] = FALSE;
    }
    d->button_fade_info.first_draw = TRUE;

    if (ws->use_pixmap_buttons)
    {
	if ((d->active && ws->use_button_glow) ||
	    (!d->active && ws->use_button_inactive_glow))
	{
	    for (b_t = 0; b_t < B_T_COUNT; b_t++)
	    {
		if (BUTTON_NOT_VISIBLE(d, b_t))
		    continue;
		get_button_pos(ws, b_t, d, y1, &x, &y);
		button_region_t *button_region = &(d->button_region[b_t]);

		glow_x = x - (ws->c_glow_size.w - ws->c_icon_size[b_t].w) / 2;
		glow_y = y - (ws->c_glow_size.h - ws->c_icon_size[b_t].h) / 2;

		button_region->base_x1 = x;
		button_region->base_y1 = y;
		button_region->base_x2 = x + ws->c_icon_size[b_t].w;
		button_region->base_y2 = MIN(y + ws->c_icon_size[b_t].h,
					     ws->top_space +
					     ws->titlebar_height);

		button_region->glow_x1 = glow_x;
		button_region->glow_y1 = glow_y;
		button_region->glow_x2 = glow_x + ws->c_glow_size.w;
		button_region->glow_y2 = MIN(glow_y + ws->c_glow_size.h,
					     ws->top_space +
					     ws->titlebar_height);

		// Update glow overlaps of each pair

		for (b_t2 = 0; b_t2 < b_t; b_t2++)
		{				// coordinates for these b_t2's will be ready for this b_t here
		    if (BUTTON_NOT_VISIBLE(d, b_t2))
			continue;
		    if ((button_region->base_x1 > d->button_region[b_t2].base_x1 &&	//right of b_t2
			 button_region->glow_x1 <= d->button_region[b_t2].base_x2) || (button_region->base_x1 < d->button_region[b_t2].base_x1 &&	//left of b_t2
										       button_region->
										       glow_x2
										       >=
										       d->
										       button_region
										       [b_t2].
										       base_x1))
		    {
			button_region->overlap_buttons[b_t2] = TRUE;
		    }
		    else
			button_region->overlap_buttons[b_t2] = FALSE;

		    // buttons' protruding glow length might be asymmetric
		    if ((d->button_region[b_t2].base_x1 > button_region->base_x1 &&	//left of b_t2
			 d->button_region[b_t2].glow_x1 <= button_region->base_x2) || (d->button_region[b_t2].base_x1 < button_region->base_x1 &&	//right of b_t2
										       d->
										       button_region
										       [b_t2].
										       glow_x2
										       >=
										       button_region->
										       base_x1))
		    {
			d->button_region[b_t2].overlap_buttons[b_t] = TRUE;
		    }
		    else
			d->button_region[b_t2].overlap_buttons[b_t] = FALSE;
		}
	    }
	}
	else
	{
	    for (b_t = 0; b_t < B_T_COUNT; b_t++)
	    {
		if (BUTTON_NOT_VISIBLE(d, b_t))
		    continue;
		get_button_pos(ws, b_t, d, y1, &x, &y);
		button_region_t *button_region = &(d->button_region[b_t]);

		button_region->base_x1 = x;
		button_region->base_y1 = y;
		button_region->base_x2 = x + ws->c_icon_size[b_t].w;
		button_region->base_y2 = MIN(y + ws->c_icon_size[b_t].h,
					     ws->top_space +
					     ws->titlebar_height);
	    }
	}
    }
    else
    {
	for (b_t = 0; b_t < B_T_COUNT; b_t++)
	{
	    if (BUTTON_NOT_VISIBLE(d, b_t))
		continue;
	    get_button_pos(ws, b_t, d, y1, &x, &y);
	    button_region_t *button_region = &(d->button_region[b_t]);

	    button_region->base_x1 = x;
	    button_region->base_y1 = y;
	    button_region->base_x2 = x + 16;
	    button_region->base_y2 = y + 16;
	}
    }
    for (b_t = 0; b_t < B_T_COUNT; b_t++)
    {
	button_region_t *button_region = &(d->button_region[b_t]);
	button_region_t *button_region_inact = &(d->button_region_inact[b_t]);

	memcpy(button_region_inact, button_region, sizeof(button_region_t));
    }
}
static void draw_window_decoration_real(decor_t * d, gboolean shadow_time)
{
    cairo_t *cr;
    double x1, y1, x2, y2, h;
    int top;
    frame_settings *fs = d->fs;
    window_settings *ws = fs->ws;

    if (!d->pixmap)
	return;

    top = ws->win_extents.top + ws->titlebar_height;

    x1 = ws->left_space - ws->win_extents.left;
    y1 = ws->top_space - ws->win_extents.top;
    x2 = d->width - ws->right_space + ws->win_extents.right;
    y2 = d->height - ws->bottom_space + ws->win_extents.bottom;

    h = d->height - ws->top_space - ws->titlebar_height - ws->bottom_space;

    if (!d->draw_only_buttons_region)	// if not only drawing buttons
    {
	cr = gdk_cairo_create(GDK_DRAWABLE
			      (IS_VALID(d->buffer_pixmap) ? d->buffer_pixmap :
			                                    d->pixmap));
	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	cairo_set_line_width(cr, 1.0);
	cairo_save(cr);
	draw_shadow_background(d, cr);
	engine_draw_frame(d, cr);
	cairo_restore(cr);
	cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
	cairo_set_line_width(cr, 2.0);

	/*color.r = 1;
	  color.g = 1;
	  color.b = 1; */

	// the buttons were previously drawn here, so we need to save the cairo state here
	cairo_save(cr);

	if (d->layout && d->tobj_item_state[TBT_TITLE] != 3)
	{
	    pango_layout_set_alignment(d->layout, ws->title_text_align);
	    cairo_move_to(cr,
			  get_real_pos(ws, TBT_TITLE, d),
			  y1 + 2.0 + (ws->titlebar_height -
				      ws->text_height) / 2.0);

	    /* ===================active text colors */
	    cairo_set_source_alpha_color(cr, &fs->text_halo);
	    pango_cairo_layout_path(cr, d->layout);
	    cairo_stroke(cr);

	    cairo_set_source_alpha_color(cr, &fs->text);

	    cairo_move_to(cr,
			  get_real_pos(ws, TBT_TITLE, d),
			  y1 + 2.0 + (ws->titlebar_height -
				      ws->text_height) / 2.0);

	    pango_cairo_show_layout(cr, d->layout);
	}
	if (d->icon && d->tobj_item_state[TBT_ICON] != 3)
	{
	    cairo_translate(cr, get_real_pos(ws, TBT_ICON, d),
			    y1 - 5.0 + ws->titlebar_height / 2);

	    cairo_set_source(cr, d->icon);
	    cairo_rectangle(cr, 0.0, 0.0, 16.0, 16.0);
	    cairo_clip(cr);
	    cairo_paint(cr);
	}
	// Copy button region backgrounds to buffers
	// for fast drawing of buttons from now on
	// when drawing is done for buttons
	gboolean bg_pixmaps_update_needed = FALSE;
	int b_t;

	for (b_t = 0; b_t < B_T_COUNT; b_t++)
	{
	    button_region_t *button_region =
		(d->active ? &d->button_region[b_t] : &d->
		 button_region_inact[b_t]);
	    if (BUTTON_NOT_VISIBLE(d, b_t))
		continue;
	    if (!button_region->bg_pixmap && button_region->base_x1 >= 0)	// if region is valid
	    {
		bg_pixmaps_update_needed = TRUE;
		break;
	    }
	}
	if (bg_pixmaps_update_needed && !shadow_time)
	{
	    for (b_t = 0; b_t < B_T_COUNT; b_t++)
	    {
		if (BUTTON_NOT_VISIBLE(d, b_t))
		    continue;

		button_region_t *button_region =
		    (d->active ? &d->button_region[b_t] : &d->
		     button_region_inact[b_t]);
		gint rx, ry, rw, rh;

		if (ws->use_pixmap_buttons &&
		    ((ws->use_button_glow && d->active) ||
		     (ws->use_button_inactive_glow && !d->active)))
		{
		    if (button_region->glow_x1 == -100)	// skip uninitialized regions
			continue;
		    rx = button_region->glow_x1;
		    ry = button_region->glow_y1;
		    rw = button_region->glow_x2 - button_region->glow_x1;
		    rh = button_region->glow_y2 - button_region->glow_y1;
		}
		else
		{
		    if (button_region->base_x1 == -100)	// skip uninitialized regions
			continue;
		    rx = button_region->base_x1;
		    ry = button_region->base_y1;
		    if (!ws->use_pixmap_buttons)	// offset: (-2,1)
		    {
			rx -= 2;
			ry++;
		    }
		    rw = button_region->base_x2 - button_region->base_x1;
		    rh = button_region->base_y2 - button_region->base_y1;
		}
		if (!button_region->bg_pixmap)
		    button_region->bg_pixmap = create_pixmap(rw, rh);
		if (!button_region->bg_pixmap)
		{
		    fprintf(stderr,
			    "%s: Error allocating buffer.\n", program_name);
		}
		else
		{
		    gdk_draw_drawable(button_region->bg_pixmap, d->gc,
				      IS_VALID(d->buffer_pixmap) ?
				      d->buffer_pixmap : d->pixmap,
				      rx, ry, 0, 0,
				      rw, rh);
		}
	    }
	}
	cairo_restore(cr);		// and restore the state for button drawing
	/*if (!shadow_time)
	  {
	//workaround for slowness, will grab and rotate the two side-pieces
	gint w, h;
	cairo_surface_t * csur;
	cairo_pattern_t * sr;
	cairo_matrix_t cm;
	cairo_destroy(cr);
	gint topspace = ws->top_space + ws->titlebar_height;
	cr = gdk_cairo_create (GDK_DRAWABLE (d->buffer_pixmap ? d->buffer_pixmap : d->pixmap));
	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);

	gdk_drawable_get_size(pbuff,&w,&h);
	csur = cairo_xlib_surface_create(
	GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()),
	GDK_PIXMAP_XID(pbuff),
	GDK_VISUAL_XVISUAL(gdk_drawable_get_visual(pbuff)),
	w,h);

	cairo_set_source_surface(cr, csur, 0, 0);
	sr = cairo_get_source(cr);
	cairo_pattern_get_matrix(sr, &cm);

	//draw all four quads from the old one to the new one
	//first top quad
	cairo_save(cr);
	cairo_rectangle(cr, 0, 0, d->width, topspace);
	cairo_clip(cr);
	cairo_pattern_set_matrix(sr, &cm);
	cairo_paint(cr);
	cairo_restore(cr);

	//then bottom, easiest this way
	cairo_save(cr);
	cairo_rectangle(cr, 0, topspace, d->width, ws->bottom_space);
	cairo_clip(cr);
	cm.y0 = d->height - (top_space + ws->bottom_space);
	cm.x0 = 0;
	cairo_pattern_set_matrix(sr,&cm);
	cairo_paint(cr);
	cairo_restore(cr);

	//now left
	cairo_save(cr);
	cairo_rectangle(cr, 0, topspace + ws->bottom_space,
	d->height-(topspace + ws->bottom_space), ws->left_space);
	cairo_clip(cr);
	cm.xx=0;
	cm.xy=1;
	cm.yx=1;
	cm.yy=0;
	cm.x0 = - topspace - ws->bottom_space;
	cm.y0 = topspace;
	cairo_pattern_set_matrix(sr,&cm);
	cairo_paint(cr);
	cairo_restore(cr);

	//now right
	cairo_save(cr);
	cairo_rectangle(cr, 0, topspace + ws->bottom_space + ws->left_space,
	d->height-(topspace + ws->bottom_space), ws->right_space);
	cairo_clip(cr);
	cm.y0 = topspace;
	cm.x0 = d->width-
	(topspace + ws->bottom_space + ws->left_space + ws->right_space);
	cairo_pattern_set_matrix(sr,&cm);
	cairo_paint(cr);
	cairo_restore(cr);


	cairo_destroy(cr);
	g_object_unref (G_OBJECT (pbuff));
	cairo_surface_destroy(csur);
    }
    */
    }
    // Draw buttons

    cr = gdk_cairo_create(GDK_DRAWABLE (IS_VALID(d->buffer_pixmap) ?
					d->buffer_pixmap : d->pixmap));

    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

    if (ws->use_button_fade && ws->use_pixmap_buttons)
	draw_buttons_with_fade(d, cr, y1);
    else
	draw_buttons_without_fade(d, cr, y1);

    cairo_destroy(cr);

    if (IS_VALID(d->buffer_pixmap))
    {
	/*if (d->draw_only_buttons_region && d->min_drawn_buttons_region.x1 < 10000)	// if region is updated at least once
	  {
	  gdk_draw_drawable(d->pixmap,
	  d->gc,
	  d->buffer_pixmap,
	  d->min_drawn_buttons_region.x1,
	  d->min_drawn_buttons_region.y1,
	  d->min_drawn_buttons_region.x1,
	  d->min_drawn_buttons_region.y1,
	  d->min_drawn_buttons_region.x2 -
	  d->min_drawn_buttons_region.x1,
	  d->min_drawn_buttons_region.y2 -
	  d->min_drawn_buttons_region.y1);
	  }
	  else*/
	{
	    gdk_draw_drawable(d->pixmap,
			      d->gc,
			      d->buffer_pixmap,
			      0,
			      0,
			      0,
			      0,
			      d->width,
			      d->height);
	    //ws->top_space + ws->bottom_space +
	    //ws->titlebar_height + 2);
	}
    }
}

static void draw_window_decoration(decor_t * d)
{
    if (d->active)
    {
	d->pixmap = d->p_active;
	d->buffer_pixmap = d->p_active_buffer;
    }
    else
    {
	d->pixmap = d->p_inactive;
	d->buffer_pixmap = d->p_inactive_buffer;
    }
    if (d->draw_only_buttons_region)
	draw_window_decoration_real(d, FALSE);
    if (!d->only_change_active)
    {
	gboolean save = d->active;
	frame_settings *fs = d->fs;

	d->active = TRUE;
	d->fs = d->fs->ws->fs_act;
	d->pixmap = d->p_active;
	d->buffer_pixmap = d->p_active_buffer;
	draw_window_decoration_real(d, FALSE);
	d->active = FALSE;
	d->fs = d->fs->ws->fs_inact;
	d->pixmap = d->p_inactive;
	d->buffer_pixmap = d->p_inactive_buffer;
	draw_window_decoration_real(d, FALSE);
	d->active = save;
	d->fs = fs;
    }
    else
    {
	d->only_change_active = FALSE;
    }
    if (d->active)
    {
	d->pixmap = d->p_active;
	d->buffer_pixmap = d->p_active_buffer;
    }
    else
    {
	d->pixmap = d->p_inactive;
	d->buffer_pixmap = d->p_inactive_buffer;
    }
    if (d->prop_xid)
    {
	decor_update_window_property(d);
	d->prop_xid = 0;
    }
    d->draw_only_buttons_region = FALSE;
}
static void draw_shadow_window(decor_t * d)
{
    draw_window_decoration_real(d, TRUE);
}

#define SWITCHER_ALPHA 0xa0a0

static void decor_update_switcher_property(decor_t * d)
{
    long data[256];
    Display *xdisplay = GDK_DISPLAY_XDISPLAY(gdk_display_get_default());
    gint nQuad;
    decor_quad_t quads[N_QUADS_MAX];
    window_settings *ws = d->fs->ws;
    decor_extents_t extents = ws->switcher_extents;
    GtkStyle *style;
    long fgColor[4];

    nQuad = set_switcher_quads(quads, d->width, d->height, ws);

    decor_quads_to_property(data, GDK_PIXMAP_XID(d->pixmap),
			    &extents, &extents, 0, 0, quads, nQuad);

    style = gtk_widget_get_style (style_window);

    fgColor[0] = style->fg[GTK_STATE_NORMAL].red;
    fgColor[1] = style->fg[GTK_STATE_NORMAL].green;
    fgColor[2] = style->fg[GTK_STATE_NORMAL].blue;
    fgColor[3] = SWITCHER_ALPHA;

    gdk_error_trap_push();
    XChangeProperty(xdisplay, d->prop_xid,
		    win_decor_atom,
		    XA_INTEGER,
		    32, PropModeReplace, (guchar *) data,
		    BASE_PROP_SIZE + QUAD_PROP_SIZE * nQuad);
    XChangeProperty (xdisplay, d->prop_xid, switcher_fg_atom,
		     XA_INTEGER, 32, PropModeReplace, (guchar *) fgColor, 4);
    XSync(xdisplay, FALSE);
    gdk_error_trap_pop();
}

static void draw_switcher_background(decor_t * d)
{
    Display *xdisplay = GDK_DISPLAY_XDISPLAY(gdk_display_get_default());
    cairo_t *cr;
    GtkStyle *style;
    decor_color_t color;
    alpha_color acolor;
    alpha_color acolor2;
    double alpha = SWITCHER_ALPHA / 65535.0;
    double x1, y1, x2, y2, h;
    int top;
    unsigned long pixel;
    ushort a = SWITCHER_ALPHA;
    window_settings *ws = d->fs->ws;

    if (!IS_VALID(d->buffer_pixmap))
	return;

    style = gtk_widget_get_style(style_window);

    color.r = style->bg[GTK_STATE_NORMAL].red / 65535.0;
    color.g = style->bg[GTK_STATE_NORMAL].green / 65535.0;
    color.b = style->bg[GTK_STATE_NORMAL].blue / 65535.0;
    acolor.color = acolor2.color = color;
    acolor.alpha = alpha;
    acolor2.alpha = alpha * 0.75;

    cr = gdk_cairo_create(GDK_DRAWABLE(d->buffer_pixmap));

    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);

    top = ws->win_extents.bottom;

    x1 = ws->left_space - ws->win_extents.left;
    y1 = ws->top_space - ws->win_extents.top;
    x2 = d->width - ws->right_space + ws->win_extents.right;
    y2 = d->height - ws->bottom_space + ws->win_extents.bottom;

    h = y2 - y1 - ws->win_extents.bottom - ws->win_extents.bottom;

    cairo_set_line_width(cr, 1.0);

    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    if (d->prop_xid || !IS_VALID(d->buffer_pixmap))
    {
	draw_shadow_background(d, cr);
    }

    fill_rounded_rectangle(cr,
			   x1 + 0.5,
			   y1 + 0.5,
			   ws->win_extents.left - 0.5,
			   top - 0.5,
			   CORNER_TOPLEFT,
			   &acolor, &acolor2,
			   SHADE_TOP | SHADE_LEFT, ws, 5.0);

    fill_rounded_rectangle(cr,
			   x1 + ws->win_extents.left,
			   y1 + 0.5,
			   x2 - x1 - ws->win_extents.left -
			   ws->win_extents.right,
			   top - 0.5,
			   0, &acolor, &acolor2, SHADE_TOP, ws, 5.0);

    fill_rounded_rectangle(cr,
			   x2 - ws->win_extents.right,
			   y1 + 0.5,
			   ws->win_extents.right - 0.5,
			   top - 0.5,
			   CORNER_TOPRIGHT,
			   &acolor, &acolor2,
			   SHADE_TOP | SHADE_RIGHT, ws, 5.0);

    fill_rounded_rectangle(cr,
			   x1 + 0.5,
			   y1 + top,
			   ws->win_extents.left - 0.5,
			   h, 0, &acolor, &acolor2, SHADE_LEFT, ws, 5.0);

    fill_rounded_rectangle(cr,
			   x2 - ws->win_extents.right,
			   y1 + top,
			   ws->win_extents.right - 0.5,
			   h, 0, &acolor, &acolor2, SHADE_RIGHT, ws, 5.0);

    fill_rounded_rectangle(cr,
			   x1 + 0.5,
			   y2 - ws->win_extents.bottom,
			   ws->win_extents.left - 0.5,
			   ws->win_extents.bottom - 0.5,
			   CORNER_BOTTOMLEFT,
			   &acolor, &acolor2,
			   SHADE_BOTTOM | SHADE_LEFT, ws, 5.0);

    fill_rounded_rectangle(cr,
			   x1 + ws->win_extents.left,
			   y2 - ws->win_extents.bottom,
			   x2 - x1 - ws->win_extents.left -
			   ws->win_extents.right,
			   ws->win_extents.bottom - 0.5,
			   0, &acolor, &acolor2, SHADE_BOTTOM, ws, 5.0);

    fill_rounded_rectangle(cr,
			   x2 - ws->win_extents.right,
			   y2 - ws->win_extents.bottom,
			   ws->win_extents.right - 0.5,
			   ws->win_extents.bottom - 0.5,
			   CORNER_BOTTOMRIGHT,
			   &acolor, &acolor2,
			   SHADE_BOTTOM | SHADE_RIGHT, ws, 5.0);

    cairo_rectangle(cr, x1 + ws->win_extents.left,
		    y1 + top,
		    x2 - x1 - ws->win_extents.left - ws->win_extents.right,
		    h);
    gdk_cairo_set_source_color_alpha(cr, &style->bg[GTK_STATE_NORMAL], alpha);
    cairo_fill(cr);

    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
    cairo_identity_matrix(cr);
    cairo_rectangle(cr, x1 + 1, y1 + 1,
		    x2 - x1 - 1.0,
		    y2 - y1 - 1.0);



    cairo_clip(cr);

    cairo_translate(cr, 1.0, 1.0);

    rounded_rectangle(cr,
		      x1 + 0.5, y1 + 0.5,
		      x2 - x1 - 1.0, y2 - y1 - 1.0,
		      CORNER_TOPLEFT | CORNER_TOPRIGHT | CORNER_BOTTOMLEFT |
		      CORNER_BOTTOMRIGHT, ws, 5.0);

    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.4);
    cairo_stroke(cr);

    cairo_translate(cr, -2.0, -2.0);

    rounded_rectangle(cr,
		      x1 + 0.5, y1 + 0.5,
		      x2 - x1 - 1.0, y2 - y1 - 1.0,
		      CORNER_TOPLEFT | CORNER_TOPRIGHT | CORNER_BOTTOMLEFT |
		      CORNER_BOTTOMRIGHT, ws, 5.0);

    cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.1);
    cairo_stroke(cr);

    cairo_translate(cr, 1.0, 1.0);

    cairo_reset_clip(cr);

    rounded_rectangle(cr,
		      x1 + 0.5, y1 + 0.5,
		      x2 - x1 - 1.0, y2 - y1 - 1.0,
		      CORNER_TOPLEFT | CORNER_TOPRIGHT | CORNER_BOTTOMLEFT |
		      CORNER_BOTTOMRIGHT, ws, 5.0);

    gdk_cairo_set_source_color_alpha(cr, &style->fg[GTK_STATE_NORMAL], alpha);

    cairo_stroke(cr);

    cairo_destroy(cr);

    gdk_draw_drawable(d->pixmap,
		      d->gc,
		      d->buffer_pixmap, 0, 0, 0, 0, d->width, d->height);

    pixel = ((((a * style->bg[GTK_STATE_NORMAL].red) >> 24) & 0x0000ff) |
	     (((a * style->bg[GTK_STATE_NORMAL].green) >> 16) & 0x00ff00) |
	     (((a * style->bg[GTK_STATE_NORMAL].blue) >> 8) & 0xff0000) |
	     (((a & 0xff00) << 16)));

    decor_update_switcher_property(d);

    gdk_error_trap_push();
    XSetWindowBackground(xdisplay, d->prop_xid, pixel);
    XClearWindow(xdisplay, d->prop_xid);
    XSync(xdisplay, FALSE);
    gdk_error_trap_pop();

    d->prop_xid = 0;
}

static void draw_switcher_foreground(decor_t * d)
{
    cairo_t *cr;
    GtkStyle *style;
    decor_color_t color;
    double alpha = SWITCHER_ALPHA / 65535.0;
    double x1, y1, x2;
    int top;
    window_settings *ws = d->fs->ws;

    if (!IS_VALID(d->pixmap) || !IS_VALID(d->buffer_pixmap))
	return;

    style = gtk_widget_get_style(style_window);

    color.r = style->bg[GTK_STATE_NORMAL].red / 65535.0;
    color.g = style->bg[GTK_STATE_NORMAL].green / 65535.0;
    color.b = style->bg[GTK_STATE_NORMAL].blue / 65535.0;

    top = ws->win_extents.bottom;

    x1 = ws->left_space - ws->win_extents.left;
    y1 = ws->top_space - ws->win_extents.top;
    x2 = d->width - ws->right_space + ws->win_extents.right;

    cr = gdk_cairo_create(GDK_DRAWABLE(d->buffer_pixmap));

    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);

    cairo_rectangle(cr, x1 + ws->win_extents.left,
		    y1 + top + ws->switcher_top_corner_space,
		    x2 - x1 - ws->win_extents.left - ws->win_extents.right,
		    SWITCHER_SPACE);

    gdk_cairo_set_source_color_alpha(cr, &style->bg[GTK_STATE_NORMAL], alpha);
    cairo_fill(cr);

    if (d->layout)
    {
	int w;
	int text_width;
	int text_len = 0;
	const char *text = NULL;
	const int SWITCHER_SUBST_FONT_SIZE = 10;

	pango_layout_get_pixel_size(d->layout, &text_width, NULL);
	text = pango_layout_get_text(d->layout);
	if (text != NULL)
	    text_len = strlen(text);
	else
	    return;

	// some themes ("frame" e.g.) set the title text font to 0.0pt, try to fix it
	if (text_width == 0)
	{
	    if (text_len)
	    {
		// if the font size is set to 0 indeed, set it to 10, otherwise don't draw anything

		if (!pango_layout_get_font_description(d->layout))
		{
		    PangoContext *context =
			pango_layout_get_context(d->layout);
		    PangoFontDescription *font =
			pango_context_get_font_description(context);
		    if (font == NULL)
			return;

		    int font_size = pango_font_description_get_size(font);

		    if (font_size == 0)
		    {
			pango_font_description_set_size(font,
							SWITCHER_SUBST_FONT_SIZE
							* PANGO_SCALE);
			pango_layout_get_pixel_size(d->layout, &text_width,
						    NULL);
		    }
		    else
			return;
		}
	    }
	    else
		return;
	}

	// fix too long title text in switcher
	// using ellipsize instead of cutting off text
	pango_layout_set_ellipsize(d->layout, PANGO_ELLIPSIZE_END);
	pango_layout_set_width(d->layout, (x2 - x1) * PANGO_SCALE);

	cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

	gdk_cairo_set_source_color_alpha(cr,
					 &style->fg[GTK_STATE_NORMAL], 1.0);

	pango_layout_get_pixel_size(d->layout, &w, NULL);

	cairo_move_to(cr, d->width / 2 - w / 2,
		      y1 + top + ws->switcher_top_corner_space +
		      SWITCHER_SPACE / 2 - ws->text_height / 2);

	pango_cairo_show_layout(cr, d->layout);
    }

    cairo_destroy(cr);

    gdk_draw_drawable(d->pixmap,
		      d->gc,
		      d->buffer_pixmap, 0, 0, 0, 0, d->width, d->height);
}

static void draw_switcher_decoration(decor_t * d)
{
    if (d->prop_xid)
	draw_switcher_background(d);

    draw_switcher_foreground(d);
}

static gboolean draw_decor_list(void *data)
{
    GSList *list;
    decor_t *d;

    draw_idle_id = 0;

    for (list = draw_list; list; list = list->next)
    {
	d = (decor_t *) list->data;
	(*d->draw) (d);
    }

    g_slist_free(draw_list);
    draw_list = NULL;

    return FALSE;
}

static void queue_decor_draw_for_buttons(decor_t * d, gboolean for_buttons)
{
    if (g_slist_find(draw_list, d))
    {
	// handle possible previously queued drawing
	if (d->draw_only_buttons_region)
	{
	    // the old drawing request is only for buttons, so override it
	    d->draw_only_buttons_region = for_buttons;
	}
	return;
    }

    d->draw_only_buttons_region = for_buttons;

    draw_list = g_slist_append(draw_list, d);

    if (!draw_idle_id)
	draw_idle_id = g_idle_add(draw_decor_list, NULL);
}
static void queue_decor_draw(decor_t * d)
{
    d->only_change_active = FALSE;
    queue_decor_draw_for_buttons(d, FALSE);
}

static GdkPixmap *pixmap_new_from_pixbuf(GdkPixbuf * pixbuf)
{
    GdkPixmap *pixmap;
    guint width, height;
    cairo_t *cr;

    width = gdk_pixbuf_get_width(pixbuf);
    height = gdk_pixbuf_get_height(pixbuf);

    pixmap = create_pixmap(width, height);
    if (!pixmap)
	return NULL;

    cr = (cairo_t *) gdk_cairo_create(GDK_DRAWABLE(pixmap));
    gdk_cairo_set_source_pixbuf(cr, pixbuf, 0, 0);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_paint(cr);
    cairo_destroy(cr);

    return pixmap;
}

static void
update_default_decorations(GdkScreen * screen, frame_settings * fs_act,
			   frame_settings * fs_inact)
{
    long data[256];
    Window xroot;
    GdkDisplay *gdkdisplay = gdk_display_get_default();
    Display *xdisplay = gdk_x11_display_get_xdisplay(gdkdisplay);
    Atom bareAtom, normalAtom, activeAtom;
    decor_t d;
    gint nQuad;
    decor_quad_t quads[N_QUADS_MAX];
    window_settings *ws = fs_act->ws;	// hackish, I know, FIXME
    decor_extents_t extents = ws->win_extents;

    bzero(&d, sizeof(decor_t));

    xroot = RootWindowOfScreen(gdk_x11_screen_get_xscreen(screen));

    bareAtom = XInternAtom(xdisplay, DECOR_BARE_ATOM_NAME, FALSE);
    normalAtom = XInternAtom(xdisplay, DECOR_NORMAL_ATOM_NAME, FALSE);
    activeAtom = XInternAtom(xdisplay, DECOR_ACTIVE_ATOM_NAME, FALSE);

    if (ws->shadow_pixmap)
    {
	int width, height;

	gdk_drawable_get_size(ws->shadow_pixmap, &width, &height);

	nQuad = set_shadow_quads(quads, width, height, ws);

	decor_quads_to_property(data, GDK_PIXMAP_XID(ws->shadow_pixmap),
				&ws->shadow_extents, &ws->shadow_extents, 0, 0,
				quads, nQuad);

	XChangeProperty(xdisplay, xroot,
			bareAtom,
			XA_INTEGER,
			32, PropModeReplace, (guchar *) data,
			BASE_PROP_SIZE + QUAD_PROP_SIZE * nQuad);

    }
    else
	XDeleteProperty(xdisplay, xroot, bareAtom);

    d.width =
	ws->left_space + ws->left_corner_space + 200 +
	ws->right_corner_space + ws->right_space;
    d.height =
	ws->top_space + ws->titlebar_height +
	ws->normal_top_corner_space + 2 + ws->bottom_corner_space +
	ws->bottom_space;

    extents.top += ws->titlebar_height;

    d.buffer_pixmap = NULL;
    d.layout = NULL;
    d.icon = NULL;
    d.state = 0;
    d.actions = 0;
    d.prop_xid = 0;
    d.draw = draw_window_decoration;
    d.only_change_active = FALSE;

    reset_buttons_bg_and_fade(&d);

    if (ws->decor_normal_pixmap)
	g_object_unref (G_OBJECT (ws->decor_normal_pixmap));
    if (ws->decor_active_pixmap)
	g_object_unref (G_OBJECT (ws->decor_active_pixmap));

    nQuad = my_set_window_quads(quads, d.width, d.height, ws, FALSE, FALSE);

    ws->decor_active_pixmap = ws->decor_normal_pixmap =
	create_pixmap(MAX(d.width, d.height),
		      ws->top_space + ws->left_space + ws->right_space +
		      ws->bottom_space + ws->titlebar_height);

    g_object_ref (G_OBJECT (ws->decor_active_pixmap));

    if (ws->decor_normal_pixmap && ws->decor_active_pixmap)
    {
	d.p_inactive = ws->decor_normal_pixmap;
	d.p_active = ws->decor_active_pixmap;
	d.p_active_buffer = NULL;
	d.p_inactive_buffer = NULL;
	d.active = FALSE;
	d.fs = fs_inact;

	(*d.draw) (&d);

	decor_quads_to_property(data, GDK_PIXMAP_XID(d.p_inactive),
				&extents, &extents, 0, 0, quads, nQuad);

	XChangeProperty(xdisplay, xroot,
			normalAtom,
			XA_INTEGER,
			32, PropModeReplace, (guchar *) data,
			BASE_PROP_SIZE + QUAD_PROP_SIZE * nQuad);

	decor_quads_to_property(data, GDK_PIXMAP_XID(d.p_active),
				&extents, &extents, 0, 0, quads, nQuad);

	XChangeProperty(xdisplay, xroot,
			activeAtom,
			XA_INTEGER,
			32, PropModeReplace, (guchar *) data,
			BASE_PROP_SIZE + QUAD_PROP_SIZE * nQuad);
    }
}

static gboolean get_window_prop(Window xwindow, Atom atom, Window * val)
{
    Atom type;
    int format;
    gulong nitems;
    gulong bytes_after;
    Window *w;
    int err, result;

    *val = 0;

    gdk_error_trap_push();

    type = None;
    result = XGetWindowProperty(GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()),
				xwindow,
				atom,
				0, G_MAXLONG,
				False, XA_WINDOW, &type, &format, &nitems,
				&bytes_after, (void *)&w);
    err = gdk_error_trap_pop();
    if (err != Success || result != Success)
	return FALSE;

    if (type != XA_WINDOW)
    {
	XFree(w);
	return FALSE;
    }

    *val = *w;
    XFree(w);

    return TRUE;
}

static unsigned int get_mwm_prop(Window xwindow)
{
    Display *xdisplay;
    Atom actual;
    int err, result, format;
    unsigned long n, left;
    unsigned char *hints_ret;
    MwmHints *mwm_hints;
    unsigned int decor = MWM_DECOR_ALL;

    xdisplay = GDK_DISPLAY_XDISPLAY(gdk_display_get_default());

    gdk_error_trap_push();

    result = XGetWindowProperty(xdisplay, xwindow, mwm_hints_atom,
				0L, 20L, FALSE, mwm_hints_atom,
				&actual, &format, &n, &left, &hints_ret);
    mwm_hints = (MwmHints *) hints_ret;

    err = gdk_error_trap_pop();
    if (err != Success || result != Success)
	return decor;

    if (n && mwm_hints)
    {
	if (n >= PROP_MOTIF_WM_HINT_ELEMENTS &&
	    mwm_hints->flags & MWM_HINTS_DECORATIONS)
	    decor = mwm_hints->decorations;

	XFree(mwm_hints);
    }

    return decor;
}

gint get_title_object_type(gchar obj)
{
    switch (obj)
    {
	case ':':					// state separator
	    return -1;
	case 'C':					// close
	    return TBT_CLOSE;
	case 'N':					// miNimize
	    return TBT_MINIMIZE;
	case 'X':					// maXimize/Restore
	case 'R':					// ""
	    return TBT_MAXIMIZE;
	case 'H':					// Help
	    return TBT_HELP;
	case 'M':					// not implemented menu
	    return TBT_MENU;
	case 'T':					// Text
	    return TBT_TITLE;
	case 'I':					// Icon
	    return TBT_ICON;
	case 'S':					// Shade
	    return TBT_SHADE;
	case 'U':					// on-top(Up)
	case 'A':					// (Above)
	    return TBT_ONTOP;
	case 'D':					// not implemented on-bottom(Down)
	    return TBT_ONBOTTOM;
	case 'Y':
	    return TBT_STICKY;
	default:
	    return -2;
    }
    return -2;
}

gint get_title_object_width(gchar obj, window_settings * ws, decor_t * d)
{
    int i = get_title_object_type(obj);

    switch (i)
    {
	case -1:					// state separator
	    return -1;
	case TBT_TITLE:
	    if (d->layout)
	    {
		gint w;

		pango_layout_get_pixel_size(d->layout, &w, NULL);
		return w + 2;
	    }
	    else
		return 2;
	case TBT_ICON:
	    return 18;
	default:
	    if (i < B_T_COUNT)
		return (d->actions & button_actions[i]) ?
		    (ws->use_pixmap_buttons ? ws->c_icon_size[i].w : 18) : 0;
	    else
		return 0;
    }

}
void position_title_object(gchar obj, WnckWindow * win, window_settings * ws,
			   gint x, gint s)
{
    decor_t *d = g_object_get_data(G_OBJECT(win), "decor");
    gint i = get_title_object_type(obj);

    if (i < 0)
	return;
    if (i < B_T_COUNT)
    {
	Display *xdisplay;
	gint w = ws->use_pixmap_buttons ? ws->c_icon_size[i].w : 16;
	gint h = ws->use_pixmap_buttons ? ws->c_icon_size[i].h : 16;
	gint y = ws->button_offset;

	xdisplay = GDK_DISPLAY_XDISPLAY(gdk_display_get_default());
	gdk_error_trap_push();
	if (d->actions & button_actions[i])
	{
	    XMoveResizeWindow(xdisplay, d->button_windows[i], x -
			      ((ws->use_decoration_cropping &&
				(d->state & WNCK_WINDOW_STATE_MAXIMIZED_HORIZONTALLY)) ?
			       ws->win_extents.left : 0), y, w, h);
	    if (button_cursor.cursor && ws->button_hover_cursor == 1)
		XDefineCursor(xdisplay,
			      d->button_windows[i], button_cursor.cursor);
	    else
		XUndefineCursor(xdisplay, d->button_windows[i]);
	}
	XSync(xdisplay, FALSE);
	gdk_error_trap_pop();
    }
    d->tobj_item_pos[i] = x - d->tobj_pos[s];
    d->tobj_item_state[i] = s;
}

void layout_title_objects(WnckWindow * win)
{
    decor_t *d = g_object_get_data(G_OBJECT(win), "decor");
    window_settings *ws = d->fs->ws;
    gint x0, y0;
    gint width, height;
    guint i;
    gint state = 0;
    gint owidth;
    gint x;
    gboolean removed = FALSE;

    d->tobj_size[0] = 0;
    d->tobj_size[1] = 0;
    d->tobj_size[2] = 0;
    wnck_window_get_geometry(win, &x0, &y0, &width, &height);
    for (i = 0; i < strlen(ws->tobj_layout); i++)
    {
	if (ws->tobj_layout[i] == '(')
	{
	    i++;
	    d->tobj_size[state] +=
		g_ascii_strtoull(&ws->tobj_layout[i], NULL, 0);
	    while (ws->tobj_layout[i] && g_ascii_isdigit(ws->tobj_layout[i]))
		i++;
	    continue;
	}
	if ((owidth =
	     get_title_object_width(ws->tobj_layout[i], ws, d)) == -1)
	{
	    state++;
	    if (state > 2)
		break;
	}
	else
	    d->tobj_size[state] += owidth;

	d->tobj_item_width[state] = owidth;
    }
    state = 0;
    d->tobj_pos[0] = ws->win_extents.left;	// always true
    d->tobj_pos[2] = width - d->tobj_size[2] + d->tobj_pos[0];
    d->tobj_pos[1] =
	MAX((d->tobj_pos[2] + d->tobj_size[0] - d->tobj_size[1]) / 2,
	    0) + d->tobj_pos[0];
    x = d->tobj_pos[0] + ws->button_hoffset;
    for (i = 0; i < TBT_COUNT; i++)
	d->tobj_item_state[i] = 3;

    for (i = 0; i < strlen(ws->tobj_layout); i++)
    {
	if (ws->tobj_layout[i] == '(')
	{
	    i++;
	    x += g_ascii_strtoull(&ws->tobj_layout[i], NULL, 0);
	    while (ws->tobj_layout[i] && g_ascii_isdigit(ws->tobj_layout[i]))
		i++;
	    continue;
	}
	if (get_title_object_type(ws->tobj_layout[i]) == -1)
	{
	    state++;
	    if (state > 2)
		break;
	    else
		x = d->tobj_pos[state];
	}
	else
	{
	    if (state == 2 && !removed)
	    {
		x -= ws->button_hoffset;
		removed = TRUE;
	    }
	    position_title_object(ws->tobj_layout[i], win, ws, x, state);
	    x += get_title_object_width(ws->tobj_layout[i], ws, d);
	}
    }
}
static void update_event_windows(WnckWindow * win)
{
    Display *xdisplay;
    decor_t *d = g_object_get_data(G_OBJECT(win), "decor");
    gint x0, y0, width, height, x, y, w, h;
    gint i, j, k, l;
    window_settings *ws = d->fs->ws;

    xdisplay = GDK_DISPLAY_XDISPLAY(gdk_display_get_default());

    wnck_window_get_geometry(win, &x0, &y0, &width, &height);

    if (d->state & WNCK_WINDOW_STATE_SHADED)
    {
	height = 0;
	k = l = 1;
    }
    else
    {
	k = 0;
	l = 2;
    }

    gdk_error_trap_push();

    for (i = 0; i < 3; i++)
    {
	static guint event_window_actions[3][3] = {
	    {
		WNCK_WINDOW_ACTION_RESIZE,
		WNCK_WINDOW_ACTION_RESIZE,
		WNCK_WINDOW_ACTION_RESIZE}, {
		    WNCK_WINDOW_ACTION_RESIZE,
		    WNCK_WINDOW_ACTION_MOVE,
		    WNCK_WINDOW_ACTION_RESIZE}, {
			WNCK_WINDOW_ACTION_RESIZE,
			WNCK_WINDOW_ACTION_RESIZE,
			WNCK_WINDOW_ACTION_RESIZE}
	};

	for (j = 0; j < 3; j++)
	{
	    if (d->actions & event_window_actions[i][j] && i >= k && i <= l)
	    {
		x = ws->pos[i][j].x + ws->pos[i][j].xw * width;
		y = ws->pos[i][j].y + ws->pos[i][j].yh * height;
		w = ws->pos[i][j].w + ws->pos[i][j].ww * width;
		h = ws->pos[i][j].h + ws->pos[i][j].hh * height;

		XMapWindow(xdisplay, d->event_windows[i][j]);
		XMoveResizeWindow(xdisplay, d->event_windows[i][j], x -
				  ((ws->use_decoration_cropping &&
				    (d->state & WNCK_WINDOW_STATE_MAXIMIZED_HORIZONTALLY)) ?
				   ws->win_extents.left : 0), y, w, h);
	    }
	    else
		XUnmapWindow(xdisplay, d->event_windows[i][j]);
	}
    }

    for (i = 0; i < B_T_COUNT; i++)
    {
	if (d->actions & button_actions[i])
	    XMapWindow(xdisplay, d->button_windows[i]);
	else
	    XUnmapWindow(xdisplay, d->button_windows[i]);
    }
    layout_title_objects(win);
    XSync(xdisplay, FALSE);
    gdk_error_trap_pop();
}

#if HAVE_WNCK_WINDOW_HAS_NAME
static const char *wnck_window_get_real_name(WnckWindow * win)
{
    return wnck_window_has_name(win) ? wnck_window_get_name(win) : NULL;
}
#define wnck_window_get_name(w) wnck_window_get_real_name(w)
#endif

gint max_window_name_width(WnckWindow * win)
{
    decor_t *d = g_object_get_data(G_OBJECT(win), "decor");
    const gchar *name;
    gint w;
    window_settings *ws = d->fs->ws;

    name = wnck_window_get_name(win);
    if (!name)
	return 0;

    if (!d->layout)
    {
	d->layout = pango_layout_new(ws->pango_context);
	if (!d->layout)
	    return 0;

	pango_layout_set_wrap(d->layout, PANGO_WRAP_CHAR);
    }

    pango_layout_set_auto_dir (d->layout, FALSE);
    pango_layout_set_width(d->layout, -1);
    pango_layout_set_text(d->layout, name, strlen(name));
    pango_layout_get_pixel_size(d->layout, &w, NULL);

    if (d->name)
	pango_layout_set_text(d->layout, d->name, strlen(d->name));

    return w + 6;
}

static void update_window_decoration_name(WnckWindow * win)
{
    decor_t *d = g_object_get_data(G_OBJECT(win), "decor");
    const gchar *name;
    glong name_length;
    PangoLayoutLine *line;
    window_settings *ws = d->fs->ws;

    if (d->name)
    {
	g_free(d->name);
	d->name = NULL;
    }

    name = wnck_window_get_name(win);
    if (name && (name_length = strlen(name)))
    {
	gint w, n_line;

	pango_layout_set_auto_dir (d->layout, FALSE);
	pango_layout_set_text(d->layout, "", 0);
	pango_layout_set_width(d->layout, 0);
	layout_title_objects(win);
	w = d->width - ws->left_space - ws->right_space - d->tobj_size[0]
	    - d->tobj_size[1] - d->tobj_size[2];
	if (w < 1)
	    w = 1;

	pango_layout_set_width(d->layout, w * PANGO_SCALE);
	pango_layout_set_text(d->layout, name, name_length);

	n_line = pango_layout_get_line_count(d->layout);

	line = pango_layout_get_line(d->layout, 0);

	name_length = line->length;
	if (pango_layout_get_line_count(d->layout) > 1)
	{
	    if (name_length < 4)
	    {
		g_object_unref(G_OBJECT(d->layout));
		d->layout = NULL;
		return;
	    }

	    d->name = g_strndup(name, name_length);
	    strcpy(d->name + name_length - 3, "...");
	}
	else
	    d->name = g_strndup(name, name_length);

	pango_layout_set_text(d->layout, d->name, name_length);
	layout_title_objects(win);
    }
    else if (d->layout)
    {
	g_object_unref(G_OBJECT(d->layout));
	d->layout = NULL;
    }
}

static void update_window_decoration_icon(WnckWindow * win)
{
    decor_t *d = g_object_get_data(G_OBJECT(win), "decor");

    if (d->icon)
    {
	cairo_pattern_destroy(d->icon);
	d->icon = NULL;
    }

    if (d->icon_pixmap)
    {
	g_object_unref (G_OBJECT (d->icon_pixmap));
	d->icon_pixmap = NULL;
    }
    if (d->icon_pixbuf) 
      g_object_unref (G_OBJECT (d->icon_pixbuf));

    d->icon_pixbuf = wnck_window_get_mini_icon(win);
    if (d->icon_pixbuf)
    {
	cairo_t *cr;

        g_object_ref (G_OBJECT (d->icon_pixbuf));

	d->icon_pixmap = pixmap_new_from_pixbuf(d->icon_pixbuf);
	cr = gdk_cairo_create(GDK_DRAWABLE(d->icon_pixmap));
	d->icon = cairo_pattern_create_for_surface(cairo_get_target(cr));
	cairo_destroy(cr);
    }
}

static void update_window_decoration_state(WnckWindow * win)
{
    decor_t *d = g_object_get_data(G_OBJECT(win), "decor");

    d->state = wnck_window_get_state(win);
}

static void update_window_decoration_actions(WnckWindow * win)
{
    decor_t *d = g_object_get_data(G_OBJECT(win), "decor");

    /* code to check for context help protocol */
    Atom actual;
    int result, format;
    unsigned long n, left;
    unsigned long offset;
    unsigned char *data;
    Window id = wnck_window_get_xid(win);
    Display *xdisplay;
    GdkDisplay *gdkdisplay;

    //GdkScreen   *screen;
    //Window      xroot;
    //XEvent      ev;

    gdkdisplay = gdk_display_get_default();
    xdisplay = GDK_DISPLAY_XDISPLAY(gdkdisplay);
    //screen     = gdk_display_get_default_screen (gdkdisplay);
    //xroot      = RootWindowOfScreen (gdk_x11_screen_get_xscreen (screen));

    d->actions = wnck_window_get_actions(win);
    data = NULL;
    left = 1;
    offset = 0;
    while (left)
    {
	result = XGetWindowProperty(xdisplay, id, wm_protocols_atom,
				    offset, 1L, FALSE, XA_ATOM, &actual,
				    &format, &n, &left, &data);
	offset++;
	if (result == Success && n && data)
	{
	    Atom a;

	    memcpy(&a, data, sizeof(Atom));
	    XFree((void *)data);
	    if (a == net_wm_context_help_atom)
		d->actions |= FAKE_WINDOW_ACTION_HELP;
	    data = NULL;
	}
	else if (result != Success)
	{
	    /* Closes #161 */
	    fprintf(stderr,
		    "XGetWindowProperty() returned non-success value (%d) for window '%s'.\n",
		    result, wnck_window_get_name(win));
	    break;
	}
    }
}

static gboolean update_window_decoration_size(WnckWindow * win)
{
    decor_t *d = g_object_get_data(G_OBJECT(win), "decor");
    GdkPixmap *pixmap, *buffer_pixmap = NULL;
    GdkPixmap *ipixmap, *ibuffer_pixmap = NULL;
    gint width, height;
    gint w;
    gint h;

    window_settings *ws = d->fs->ws;

    max_window_name_width(win);
    layout_title_objects(win);

    wnck_window_get_geometry(win, NULL, NULL, &w, &h);

    width = ws->left_space + MAX(w, 1) + ws->right_space;
    height = ws->top_space + ws->bottom_space + ws->titlebar_height;

    if (ws->stretch_sides)
	height += ws->normal_top_corner_space + ws->bottom_corner_space + 2;
    else
	height += MAX(h, 1);

    if (width == d->width && height == d->height)
    {
	update_window_decoration_name(win);
	return FALSE;
    }
    reset_buttons_bg_and_fade(d);

    pixmap = create_pixmap(MAX(width, height),
			   ws->top_space + ws->titlebar_height +
			   ws->left_space + ws->right_space +
			   ws->bottom_space);
    if (!pixmap)
	return FALSE;

    buffer_pixmap =
	create_pixmap(MAX(width, height),
		      ws->top_space + ws->titlebar_height +
		      ws->left_space + ws->right_space +
		      ws->bottom_space);
    if (!buffer_pixmap)
    {
	g_object_unref (G_OBJECT (pixmap));
	return FALSE;
    }

    ipixmap = create_pixmap(MAX(width, height),
			    ws->top_space + ws->titlebar_height +
			    ws->left_space + ws->right_space +
			    ws->bottom_space);

    ibuffer_pixmap = create_pixmap(MAX(width, height),
				   ws->top_space + ws->titlebar_height +
				   ws->left_space + ws->right_space +
				   ws->bottom_space);

    if (d->p_active)
	g_object_unref (G_OBJECT (d->p_active));

    if (d->p_active_buffer)
	g_object_unref (G_OBJECT (d->p_active_buffer));

    if (d->p_inactive)
	g_object_unref (G_OBJECT (d->p_inactive));

    if (d->p_inactive_buffer)
	g_object_unref (G_OBJECT (d->p_inactive_buffer));

    if (d->gc)
      g_object_unref (G_OBJECT (d->gc));

    d->only_change_active = FALSE;

    d->pixmap = d->active ? pixmap : ipixmap;
    d->buffer_pixmap = d->active ? buffer_pixmap : ibuffer_pixmap;
    d->p_active = pixmap;
    d->p_active_buffer = buffer_pixmap;
    d->p_inactive = ipixmap;
    d->p_inactive_buffer = ibuffer_pixmap;
    d->gc = gdk_gc_new(pixmap);

    d->width = width;
    d->height = height;

    d->prop_xid = wnck_window_get_xid(win);

    update_window_decoration_name(win);

    update_button_regions(d);
    stop_button_fade(d);

    queue_decor_draw(d);

    return TRUE;
}

static void add_frame_window(WnckWindow * win, Window frame)
{
    Display *xdisplay;
    XSetWindowAttributes attr;
    gulong xid = wnck_window_get_xid(win);
    decor_t *d = g_object_get_data(G_OBJECT(win), "decor");
    gint i, j;

    xdisplay = GDK_DISPLAY_XDISPLAY(gdk_display_get_default());

    bzero(&attr, sizeof(XSetWindowAttributes));
    attr.event_mask = ButtonPressMask | EnterWindowMask | LeaveWindowMask;
    attr.override_redirect = TRUE;

    gdk_error_trap_push();

    for (i = 0; i < 3; i++)
    {
	for (j = 0; j < 3; j++)
	{
	    d->event_windows[i][j] =
		XCreateWindow(xdisplay,
			      frame,
			      0, 0, 1, 1, 0,
			      CopyFromParent, CopyFromParent,
			      CopyFromParent,
			      CWOverrideRedirect | CWEventMask, &attr);

	    if (cursor[i][j].cursor)
		XDefineCursor(xdisplay, d->event_windows[i][j],
			      cursor[i][j].cursor);
	}
    }

    attr.event_mask |= ButtonReleaseMask;

    for (i = 0; i < B_T_COUNT; i++)
    {
	d->button_windows[i] =
	    XCreateWindow(xdisplay,
			  frame,
			  0, 0, 1, 1, 0,
			  CopyFromParent, CopyFromParent, CopyFromParent,
			  CWOverrideRedirect | CWEventMask, &attr);

	d->button_states[i] = 0;
    }

    XSync(xdisplay, FALSE);
    if (!gdk_error_trap_pop())
    {
	if (get_mwm_prop(xid) & (MWM_DECOR_ALL | MWM_DECOR_TITLE))
	    d->decorated = TRUE;

	for (i = 0; i < 3; i++)
	    for (j = 0; j < 3; j++)
		g_hash_table_insert(frame_table,
				    GINT_TO_POINTER(d->event_windows[i][j]),
				    GINT_TO_POINTER(xid));

	for (i = 0; i < B_T_COUNT; i++)
	    g_hash_table_insert(frame_table,
				GINT_TO_POINTER(d->button_windows[i]),
				GINT_TO_POINTER(xid));


	update_window_decoration_state(win);
	update_window_decoration_actions(win);
	update_window_decoration_icon(win);
	update_window_decoration_size(win);

	update_event_windows(win);
    }
    else
	memset(d->event_windows, 0, sizeof(d->event_windows));
}

static gboolean update_switcher_window(WnckWindow * win, Window selected)
{
    decor_t *d = g_object_get_data(G_OBJECT(win), "decor");
    GdkPixmap *pixmap, *buffer_pixmap = NULL;
    gint height, width = 0;
    WnckWindow *selected_win;
    window_settings *ws = d->fs->ws;

    wnck_window_get_geometry(win, NULL, NULL, &width, NULL);

    width += ws->left_space + ws->right_space;
    height = ws->top_space + SWITCHER_TOP_EXTRA +
	ws->switcher_top_corner_space + SWITCHER_SPACE +
	ws->switcher_bottom_corner_space + ws->bottom_space;

    d->decorated = FALSE;
    d->draw = draw_switcher_decoration;

    if (!IS_VALID(d->pixmap) && IS_VALID(ws->switcher_pixmap))
    {
	g_object_ref (G_OBJECT (ws->switcher_pixmap));
	d->pixmap = ws->switcher_pixmap;
    }

    if (!IS_VALID(d->buffer_pixmap) && IS_VALID(ws->switcher_buffer_pixmap))
    {
	g_object_ref (G_OBJECT (ws->switcher_buffer_pixmap));
	d->buffer_pixmap = ws->switcher_buffer_pixmap;
    }

    if (!d->width)
	d->width = ws->switcher_width;

    if (!d->height)
	d->height = ws->switcher_height;

    selected_win = wnck_window_get(selected);
    if (selected_win)
    {
	glong name_length;
	PangoLayoutLine *line;
	const gchar *name;

	if (d->name)
	{
	    g_free(d->name);
	    d->name = NULL;
	}

	name = wnck_window_get_name(selected_win);
	if (name && (name_length = strlen(name)))
	{
	    gint n_line;

	    if (!d->layout)
	    {
		d->layout = pango_layout_new(ws->pango_context);
		if (d->layout)
		    pango_layout_set_wrap(d->layout, PANGO_WRAP_CHAR);
	    }

	    if (d->layout)
	    {
		pango_layout_set_auto_dir (d->layout, FALSE);
		pango_layout_set_width(d->layout, -1);
		pango_layout_set_text(d->layout, name, name_length);

		n_line = pango_layout_get_line_count(d->layout);

		line = pango_layout_get_line(d->layout, 0);

		name_length = line->length;
		if (pango_layout_get_line_count(d->layout) > 1)
		{
		    if (name_length < 4)
		    {
			g_object_unref(G_OBJECT(d->layout));
			d->layout = NULL;
		    }
		    else
		    {
			d->name = g_strndup(name, name_length);
			strcpy(d->name + name_length - 3, "...");
		    }
		}
		else
		    d->name = g_strndup(name, name_length);

		if (d->layout)
		    pango_layout_set_text(d->layout, d->name, name_length);
	    }
	}
	else if (d->layout)
	{
	    g_object_unref(G_OBJECT(d->layout));
	    d->layout = NULL;
	}
    }

    if (width == d->width && height == d->height)
    {
	if (!d->gc)
	{
	    if (d->pixmap->parent_instance.ref_count) 
		d->gc = gdk_gc_new(d->pixmap);
	    else 
		d->pixmap = NULL;
	}

	if (d->pixmap)
	{
	    queue_decor_draw(d);
	    return FALSE;
	}
    }

    pixmap = create_pixmap(width, height);
    if (!pixmap)
	return FALSE;

    buffer_pixmap = create_pixmap(width, height);
    if (!buffer_pixmap)
    {
	g_object_unref (G_OBJECT (pixmap));
	return FALSE;
    }

    if (ws->switcher_pixmap)
	g_object_unref (G_OBJECT (ws->switcher_pixmap));

    if (ws->switcher_buffer_pixmap)
	g_object_unref (G_OBJECT (ws->switcher_buffer_pixmap));

    if (d->pixmap)
	g_object_unref (G_OBJECT (d->pixmap));

    if (d->buffer_pixmap)
	g_object_unref (G_OBJECT (d->buffer_pixmap));

    if (d->gc)
        g_object_unref (G_OBJECT (d->gc));

    ws->switcher_pixmap = pixmap;
    ws->switcher_buffer_pixmap = buffer_pixmap;

    ws->switcher_width = width;
    ws->switcher_height = height;

    g_object_ref (G_OBJECT (pixmap));
    g_object_ref (G_OBJECT (buffer_pixmap));

    d->pixmap = pixmap;
    d->buffer_pixmap = buffer_pixmap;
    d->gc = gdk_gc_new(pixmap);

    d->width = width;
    d->height = height;

    d->prop_xid = wnck_window_get_xid(win);

    reset_buttons_bg_and_fade(d);

    queue_decor_draw(d);

    return TRUE;
}

static void remove_frame_window(WnckWindow * win)
{
    decor_t *d = g_object_get_data(G_OBJECT(win), "decor");

    if (d->p_active)
    {
	g_object_unref (G_OBJECT (d->p_active));
	d->p_active = NULL;
    }

    if (d->p_active_buffer)
    {
	g_object_unref (G_OBJECT (d->p_active_buffer));
	d->p_active_buffer = NULL;
    }

    if (d->p_inactive)
    {
	g_object_unref (G_OBJECT (d->p_inactive));
	d->p_inactive = NULL;
    }

    if (d->p_inactive_buffer)
    {
	g_object_unref (G_OBJECT (d->p_inactive_buffer));
	d->p_inactive_buffer = NULL;
    }

    if (d->gc)
    {
        g_object_unref (G_OBJECT (d->gc));
	d->gc = NULL;
    }

    int b_t;

    for (b_t = 0; b_t < B_T_COUNT; b_t++)
    {
	if (d->button_region[b_t].bg_pixmap)
	{
	    g_object_unref (G_OBJECT (d->button_region[b_t].bg_pixmap));
	    d->button_region[b_t].bg_pixmap = NULL;
	}
	if (d->button_region_inact[b_t].bg_pixmap)
	{
	    g_object_unref (G_OBJECT (d->button_region_inact[b_t].bg_pixmap));
	    d->button_region_inact[b_t].bg_pixmap = NULL;
	}
    }
    if (d->button_fade_info.cr)
    {
	cairo_destroy(d->button_fade_info.cr);
	d->button_fade_info.cr = NULL;
    }

    if (d->button_fade_info.timer >= 0)
    {
	g_source_remove(d->button_fade_info.timer);
	d->button_fade_info.timer = -1;
    }

    if (d->name)
    {
	g_free(d->name);
	d->name = NULL;
    }

    if (d->layout)
    {
	g_object_unref(G_OBJECT(d->layout));
	d->layout = NULL;
    }

    if (d->icon)
    {
	cairo_pattern_destroy(d->icon);
	d->icon = NULL;
    }

    if (d->icon_pixmap)
    {
	g_object_unref (G_OBJECT (d->icon_pixmap));
	d->icon_pixmap = NULL;
    }

    if (d->icon_pixbuf)
    {
        g_object_unref (G_OBJECT (d->icon_pixbuf));
        d->icon_pixbuf = NULL;
    }


    if (d->force_quit_dialog)
    {
	GtkWidget *dialog = d->force_quit_dialog;

	d->force_quit_dialog = NULL;
	gtk_widget_destroy(dialog);
    }

    d->width = 0;
    d->height = 0;

    d->decorated = FALSE;

    d->state = 0;
    d->actions = 0;

    draw_list = g_slist_remove(draw_list, d);
}

static void window_name_changed(WnckWindow * win)
{
    decor_t *d = g_object_get_data(G_OBJECT(win), "decor");

    if (d->decorated && !update_window_decoration_size(win))
    {
	update_button_regions(d);
	queue_decor_draw(d);
    }
}

static void window_geometry_changed(WnckWindow * win)
{
    decor_t *d = g_object_get_data(G_OBJECT(win), "decor");

    if (d->decorated)
    {
	int width, height;

	wnck_window_get_geometry(win, NULL, NULL, &width, &height);
	if ((width != d->client_width) || (height != d->client_height)) 
	{
	    d->client_width = width;
	    d->client_height = height;

	    update_window_decoration_size(win);
	    update_event_windows(win);
	}
    }
}

static void window_icon_changed(WnckWindow * win)
{
    decor_t *d = g_object_get_data(G_OBJECT(win), "decor");

    if (d->decorated)
    {
	update_window_decoration_icon(win);
	update_button_regions(d);
	queue_decor_draw(d);
    }
}

static void window_state_changed(WnckWindow * win)
{
    decor_t *d = g_object_get_data(G_OBJECT(win), "decor");

    if (d->decorated)
    {
	update_window_decoration_state(win);
	update_button_regions(d);
	stop_button_fade(d);
	update_window_decoration_size(win);
	update_event_windows(win);

	d->prop_xid = wnck_window_get_xid(win);
	queue_decor_draw(d);
    }
}

static void window_actions_changed(WnckWindow * win)
{
    decor_t *d = g_object_get_data(G_OBJECT(win), "decor");

    if (d->decorated)
    {
	update_window_decoration_actions(win);
	update_window_decoration_size(win);
	update_event_windows(win);

	d->prop_xid = wnck_window_get_xid(win);
	queue_decor_draw(d);
    }
}

static void connect_window(WnckWindow * win)
{
    g_signal_connect_object(win, "name_changed",
			    G_CALLBACK(window_name_changed), 0, 0);
    g_signal_connect_object(win, "geometry_changed",
			    G_CALLBACK(window_geometry_changed), 0, 0);
    g_signal_connect_object(win, "icon_changed",
			    G_CALLBACK(window_icon_changed), 0, 0);
    g_signal_connect_object(win, "state_changed",
			    G_CALLBACK(window_state_changed), 0, 0);
    g_signal_connect_object(win, "actions_changed",
			    G_CALLBACK(window_actions_changed), 0, 0);
}

static void active_window_changed(WnckScreen * screen)
{
    WnckWindow *win;
    decor_t *d;

    win = wnck_screen_get_previously_active_window(screen);
    if (win)
    {
	d = g_object_get_data(G_OBJECT(win), "decor");
	if (d && d->pixmap && d->decorated)
	{
	    d->active = wnck_window_is_active(win);
	    d->fs = (d->active ? d->fs->ws->fs_act : d->fs->ws->fs_inact);
	    if (!g_slist_find(draw_list, d))
		d->only_change_active = TRUE;
	    d->prop_xid = wnck_window_get_xid(win);
	    stop_button_fade(d);
	    queue_decor_draw_for_buttons(d, TRUE);
	}
    }

    win = wnck_screen_get_active_window(screen);
    if (win)
    {
	d = g_object_get_data(G_OBJECT(win), "decor");
	if (d && d->pixmap && d->decorated)
	{
	    d->active = wnck_window_is_active(win);
	    d->fs = (d->active ? d->fs->ws->fs_act : d->fs->ws->fs_inact);
	    if (!g_slist_find(draw_list, d))
		d->only_change_active = TRUE;
	    d->prop_xid = wnck_window_get_xid(win);
	    stop_button_fade(d);
	    queue_decor_draw_for_buttons(d, TRUE);
	}
    }
}

static void window_opened(WnckScreen * screen, WnckWindow * win)
{
    decor_t *d;
    Window window;
    gulong xid;

    d = g_malloc(sizeof(decor_t));
    if (!d)
	return;
    bzero(d, sizeof(decor_t));

    wnck_window_get_geometry(win, NULL, NULL, &d->client_width, &d->client_height);
    d->active = wnck_window_is_active(win);

    d->draw = draw_window_decoration;
    d->fs = d->active ? global_ws->fs_act : global_ws->fs_inact;

    reset_buttons_bg_and_fade(d);

    g_object_set_data(G_OBJECT(win), "decor", d);

    connect_window(win);

    xid = wnck_window_get_xid(win);

    if (get_window_prop(xid, select_window_atom, &window))
    {
	d->prop_xid = wnck_window_get_xid(win);
	update_switcher_window(win, window);
    }
    else if (get_window_prop(xid, frame_window_atom, &window))
	add_frame_window(win, window);
}

static void window_closed(WnckScreen * screen, WnckWindow * win)
{
    Display *xdisplay = GDK_DISPLAY_XDISPLAY(gdk_display_get_default());
    decor_t *d = g_object_get_data(G_OBJECT(win), "decor");
    Window window;

    gdk_error_trap_push();
    XDeleteProperty(xdisplay, wnck_window_get_xid(win), win_decor_atom);
    XSync(xdisplay, FALSE);
    gdk_error_trap_pop();

    if (!get_window_prop(wnck_window_get_xid(win), select_window_atom, &window))
	remove_frame_window(win);

    g_object_set_data (G_OBJECT (win), "decor", NULL);

    g_free(d);
}

static void connect_screen(WnckScreen * screen)
{
    GList *windows;

    g_signal_connect_object(G_OBJECT(screen), "active_window_changed",
			    G_CALLBACK(active_window_changed), 0, 0);
    g_signal_connect_object(G_OBJECT(screen), "window_opened",
			    G_CALLBACK(window_opened), 0, 0);
    g_signal_connect_object(G_OBJECT(screen), "window_closed",
			    G_CALLBACK(window_closed), 0, 0);

    windows = wnck_screen_get_windows(screen);
    while (windows != NULL)
    {
	window_opened(screen, windows->data);
	windows = windows->next;
    }
}

static void
move_resize_window(WnckWindow * win, int direction, XEvent * xevent)
{
    Display *xdisplay;
    GdkDisplay *gdkdisplay;
    GdkScreen *screen;
    Window xroot;
    XEvent ev;

    gdkdisplay = gdk_display_get_default();
    xdisplay = GDK_DISPLAY_XDISPLAY(gdkdisplay);
    screen = gdk_display_get_default_screen(gdkdisplay);
    xroot = RootWindowOfScreen(gdk_x11_screen_get_xscreen(screen));

    if (action_menu_mapped)
    {
	gtk_object_destroy(GTK_OBJECT(action_menu));
	action_menu_mapped = FALSE;
	action_menu = NULL;
	return;
    }

    ev.xclient.type = ClientMessage;
    ev.xclient.display = xdisplay;

    ev.xclient.serial = 0;
    ev.xclient.send_event = TRUE;

    ev.xclient.window = wnck_window_get_xid(win);
    ev.xclient.message_type = wm_move_resize_atom;
    ev.xclient.format = 32;

    ev.xclient.data.l[0] = xevent->xbutton.x_root;
    ev.xclient.data.l[1] = xevent->xbutton.y_root;
    ev.xclient.data.l[2] = direction;
    ev.xclient.data.l[3] = xevent->xbutton.button;
    ev.xclient.data.l[4] = 1;

    XUngrabPointer(xdisplay, xevent->xbutton.time);
    XUngrabKeyboard(xdisplay, xevent->xbutton.time);

    XSendEvent(xdisplay, xroot, FALSE,
	       SubstructureRedirectMask | SubstructureNotifyMask, &ev);

    XSync(xdisplay, FALSE);
}

static void restack_window(WnckWindow * win, int stack_mode)
{
    Display *xdisplay;
    GdkDisplay *gdkdisplay;
    GdkScreen *screen;
    Window xroot;
    XEvent ev;

    gdkdisplay = gdk_display_get_default();
    xdisplay = GDK_DISPLAY_XDISPLAY(gdkdisplay);
    screen = gdk_display_get_default_screen(gdkdisplay);
    xroot = RootWindowOfScreen(gdk_x11_screen_get_xscreen(screen));

    if (action_menu_mapped)
    {
	gtk_object_destroy(GTK_OBJECT(action_menu));
	action_menu_mapped = FALSE;
	action_menu = NULL;
	return;
    }

    ev.xclient.type = ClientMessage;
    ev.xclient.display = xdisplay;

    ev.xclient.serial = 0;
    ev.xclient.send_event = TRUE;

    ev.xclient.window = wnck_window_get_xid(win);
    ev.xclient.message_type = restack_window_atom;
    ev.xclient.format = 32;

    ev.xclient.data.l[0] = 2;
    ev.xclient.data.l[1] = None;
    ev.xclient.data.l[2] = stack_mode;
    ev.xclient.data.l[3] = 0;
    ev.xclient.data.l[4] = 0;

    XSendEvent(xdisplay, xroot, FALSE,
	       SubstructureRedirectMask | SubstructureNotifyMask, &ev);

    XSync(xdisplay, FALSE);
}

/* stolen from gtktooltip.c */

#define DEFAULT_DELAY 500		/* Default delay in ms */
#define STICKY_DELAY 0			/* Delay before popping up next tip
					 * if we're sticky
					 */
#define STICKY_REVERT_DELAY 1000	/* Delay before sticky tooltips revert
					 * to normal
					 */

static void show_tooltip(const char *text)
{
    GdkDisplay *gdkdisplay;
    GtkRequisition requisition;
    gint x, y, w, h;
    GdkScreen *screen;
    gint monitor_num;
    GdkRectangle monitor;

    if (enable_tooltips)
    {

	gdkdisplay = gdk_display_get_default();

	gtk_label_set_text(GTK_LABEL(tip_label), text);

	gtk_widget_size_request(tip_window, &requisition);

	w = requisition.width;
	h = requisition.height;

	gdk_display_get_pointer(gdkdisplay, &screen, &x, &y, NULL);

	x -= (w / 2 + 4);

	monitor_num = gdk_screen_get_monitor_at_point(screen, x, y);
	gdk_screen_get_monitor_geometry(screen, monitor_num, &monitor);

	if ((x + w) > monitor.x + monitor.width)
	    x -= (x + w) - (monitor.x + monitor.width);
	else if (x < monitor.x)
	    x = monitor.x;

	if ((y + h + 16) > monitor.y + monitor.height)
	    y = y - h - 16;
	else
	    y = y + 16;

	gtk_window_move(GTK_WINDOW(tip_window), x, y);
	gtk_widget_show(tip_window);
    }
}

static void hide_tooltip(void)
{
    if (gtk_widget_get_visible(tip_window))
	g_get_current_time(&tooltip_last_popdown);

    gtk_widget_hide(tip_window);

    if (tooltip_timer_tag)
    {
	g_source_remove(tooltip_timer_tag);
	tooltip_timer_tag = 0;
    }
}

static gboolean tooltip_recently_shown(void)
{
    GTimeVal now;
    glong msec;

    g_get_current_time(&now);

    msec = (now.tv_sec - tooltip_last_popdown.tv_sec) * 1000 +
	(now.tv_usec - tooltip_last_popdown.tv_usec) / 1000;

    return (msec < STICKY_REVERT_DELAY);
}

static gint tooltip_timeout(gpointer data)
{
    tooltip_timer_tag = 0;

    show_tooltip((const char *)data);

    return FALSE;
}

static void tooltip_start_delay(const char *text)
{
    guint delay = DEFAULT_DELAY;

    if (tooltip_timer_tag)
	return;

    if (tooltip_recently_shown())
	delay = STICKY_DELAY;

    tooltip_timer_tag = g_timeout_add(delay,
				      tooltip_timeout, (gpointer) text);
}

static gint tooltip_paint_window(GtkWidget * tooltip)
{
    GtkRequisition req;

    gtk_widget_size_request(tip_window, &req);
    gtk_paint_flat_box(tip_window->style, tip_window->window,
		       GTK_STATE_NORMAL, GTK_SHADOW_OUT,
		       NULL, GTK_WIDGET(tip_window), "tooltip",
		       0, 0, req.width, req.height);

    return FALSE;
}

static gboolean create_tooltip_window(void)
{
    tip_window = gtk_window_new(GTK_WINDOW_POPUP);

    gtk_widget_set_app_paintable(tip_window, TRUE);
    gtk_window_set_resizable(GTK_WINDOW(tip_window), FALSE);
    gtk_widget_set_name(tip_window, "gtk-tooltips");
    gtk_container_set_border_width(GTK_CONTAINER(tip_window), 4);

#if GTK_CHECK_VERSION (2, 10, 0)
    if (!gtk_check_version(2, 10, 0))
	gtk_window_set_type_hint(GTK_WINDOW(tip_window),
				 GDK_WINDOW_TYPE_HINT_TOOLTIP);
#endif

    g_signal_connect_swapped(tip_window,
			     "expose_event",
			     G_CALLBACK(tooltip_paint_window), 0);

    tip_label = gtk_label_new(NULL);
    gtk_label_set_line_wrap(GTK_LABEL(tip_label), TRUE);
    gtk_misc_set_alignment(GTK_MISC(tip_label), 0.5, 0.5);
    gtk_widget_show(tip_label);

    gtk_container_add(GTK_CONTAINER(tip_window), tip_label);

    gtk_widget_ensure_style(tip_window);

    return TRUE;
}

static void
handle_tooltip_event(WnckWindow * win,
		     XEvent * xevent, guint state, const char *tip)
{
    switch (xevent->type)
    {
	case ButtonPress:
	    hide_tooltip();
	    break;
	case ButtonRelease:
	    break;
	case EnterNotify:
	    if (!(state & PRESSED_EVENT_WINDOW))
	    {
		if (wnck_window_is_active(win))
		    tooltip_start_delay(tip);
	    }
	    break;
	case LeaveNotify:
	    hide_tooltip();
	    break;
    }
}
static void action_menu_unmap(GObject * object)
{
    action_menu_mapped = FALSE;
}

static void action_menu_map(WnckWindow * win, long button, Time time)
{
    GdkDisplay *gdkdisplay;
    GdkScreen *screen;

    gdkdisplay = gdk_display_get_default();
    screen = gdk_display_get_default_screen(gdkdisplay);

    if (action_menu)
    {
	if (action_menu_mapped)
	{
	    gtk_widget_destroy(action_menu);
	    action_menu_mapped = FALSE;
	    action_menu = NULL;
	    return;
	}
	else
	    gtk_widget_destroy(action_menu);
    }

    switch (wnck_window_get_window_type(win))
    {
	case WNCK_WINDOW_DESKTOP:
	case WNCK_WINDOW_DOCK:
	    /* don't allow window action */
	    return;
	case WNCK_WINDOW_NORMAL:
	case WNCK_WINDOW_DIALOG:
#ifndef HAVE_LIBWNCK_2_19_3
	case WNCK_WINDOW_MODAL_DIALOG:
#endif
	case WNCK_WINDOW_TOOLBAR:
	case WNCK_WINDOW_MENU:
	case WNCK_WINDOW_UTILITY:
	case WNCK_WINDOW_SPLASHSCREEN:
	    /* allow window action menu */
	    break;
    }

    action_menu = wnck_create_window_action_menu(win);

    gtk_menu_set_screen(GTK_MENU(action_menu), screen);

    g_signal_connect_object(G_OBJECT(action_menu), "unmap",
			    G_CALLBACK(action_menu_unmap), 0, 0);

    gtk_widget_show(action_menu);
    gtk_menu_popup(GTK_MENU(action_menu),
		   NULL, NULL, NULL, NULL, button, time);

    action_menu_mapped = TRUE;
}

/* generic_button_event returns:
 * 0: nothing, hover, ButtonPress
 * XEvent Button code: ButtonRelease (mouse click)
 */
static gint generic_button_event(WnckWindow * win, XEvent * xevent,
				 gint button, gint bpict)
{
    // Display *xdisplay = GDK_DISPLAY_XDISPLAY (gdk_display_get_default ());
    const gchar *tooltips[B_COUNT] = {
	_("Close Window"),
	_("Maximize Window"),
	_("Restore Window"),
	_("Minimize Window"),
	_("Context Help"),
	_("Window Menu"),
	_("Shade Window"),
	_("UnShade Window"),
	_("Set Above"),
	_("UnSet Above"),
	_("Stick Window"),
	_("UnStick Window"),
    };

    decor_t *d = g_object_get_data(G_OBJECT(win), "decor");
    guint state = d->button_states[button];
    gint ret = 0;

    // window_settings * ws = d->fs->ws;

    handle_tooltip_event(win, xevent, state, tooltips[bpict]);

    /*
     * "!(xevent->xbutton.button & -4)" tests whether event button is one of
     * three major mouse buttons
     * "!(xevent->xbutton.button & -4)" evaluates to:
     * 0: xevent->xbutton.button < 0, xevent->xbutton.button > 3
     * 1: -1 < xevent->xbutton.button < 4
     */
    switch (xevent->type)
    {
	case ButtonPress:
	    if (((button == B_T_MAXIMIZE) && !(xevent->xbutton.button & -4)) ||
		(xevent->xbutton.button == Button1))
		d->button_states[button] |= PRESSED_EVENT_WINDOW;
	    break;
	case ButtonRelease:
	    if (((button == B_T_MAXIMIZE) && !(xevent->xbutton.button & -4)) ||
		(xevent->xbutton.button == Button1))
	    {
		if (d->button_states[button] ==
		    (PRESSED_EVENT_WINDOW | IN_EVENT_WINDOW))
		    ret = xevent->xbutton.button;

		d->button_states[button] &= ~PRESSED_EVENT_WINDOW;
	    }
	    break;
	case EnterNotify:
	    d->button_states[button] |= IN_EVENT_WINDOW;
	    break;
	case LeaveNotify:
	    d->button_states[button] &= ~IN_EVENT_WINDOW;
	    break;
    }

    if (state != d->button_states[button])
	queue_decor_draw_for_buttons(d, TRUE);

    return ret;
}

static void close_button_event(WnckWindow * win, XEvent * xevent)
{
    if (generic_button_event(win, xevent, B_T_CLOSE, B_CLOSE))
	wnck_window_close(win, xevent->xbutton.time);
}

static void max_button_event(WnckWindow * win, XEvent * xevent)
{
    gboolean maximized = wnck_window_is_maximized(win);

    switch (generic_button_event(win,
				 xevent,
				 B_T_MAXIMIZE,
				 (maximized ? B_RESTORE : B_MAXIMIZE)))
    {
	case Button1:
	    maximized ? wnck_window_unmaximize(win) : wnck_window_maximize(win);
	    break;
	case Button2:
	    if (wnck_window_is_maximized_vertically(win))
		wnck_window_unmaximize_vertically(win);
	    else
		wnck_window_maximize_vertically(win);
	    break;
	case Button3:
	    if (wnck_window_is_maximized_horizontally(win))
		wnck_window_unmaximize_horizontally(win);
	    else
		wnck_window_maximize_horizontally(win);
	    break;
    }
}

static void min_button_event(WnckWindow * win, XEvent * xevent)
{
    if (generic_button_event(win, xevent, B_T_MINIMIZE, B_MINIMIZE))
	wnck_window_minimize(win);
}

static void above_button_event(WnckWindow * win, XEvent * xevent)
{
    if (wnck_window_is_above(win))
    {
	if (generic_button_event(win, xevent, B_T_ABOVE, B_UNABOVE))
	    wnck_window_unmake_above(win);
    }
    else
    {
	if (generic_button_event(win, xevent, B_T_ABOVE, B_ABOVE))
	    wnck_window_make_above(win);
    }
}

static void sticky_button_event(WnckWindow * win, XEvent * xevent)
{
    if (wnck_window_is_sticky(win))
    {
	if (generic_button_event(win, xevent, B_T_STICKY, B_UNSTICK))
	    wnck_window_unstick(win);
    }
    else
    {
	if (generic_button_event(win, xevent, B_T_STICKY, B_STICK))
	    wnck_window_stick(win);
    }
}

static void send_help_message(WnckWindow * win)
{
    Display *xdisplay;
    GdkDisplay *gdkdisplay;

    //GdkScreen  *screen;
    Window id;
    XEvent ev;

    gdkdisplay = gdk_display_get_default();
    xdisplay = GDK_DISPLAY_XDISPLAY(gdkdisplay);
    //screen     = gdk_display_get_default_screen (gdkdisplay);
    //xroot      = RootWindowOfScreen (gdk_x11_screen_get_xscreen (screen));
    id = wnck_window_get_xid(win);

    ev.xclient.type = ClientMessage;
    //ev.xclient.display = xdisplay;

    //ev.xclient.serial     = 0;
    //ev.xclient.send_event = TRUE;

    ev.xclient.window = id;
    ev.xclient.message_type = wm_protocols_atom;
    ev.xclient.data.l[0] = net_wm_context_help_atom;
    ev.xclient.data.l[1] = 0L;
    ev.xclient.format = 32;

    XSendEvent(xdisplay, id, FALSE, 0L, &ev);

    XSync(xdisplay, FALSE);
}

static void help_button_event(WnckWindow * win, XEvent * xevent)
{
    if (generic_button_event(win, xevent, B_T_HELP, B_HELP))
	send_help_message(win);
}
static void menu_button_event(WnckWindow * win, XEvent * xevent)
{
    if (generic_button_event(win, xevent, B_T_MENU, B_MENU))
	action_menu_map(win, xevent->xbutton.button, xevent->xbutton.time);
}
static void shade_button_event(WnckWindow * win, XEvent * xevent)
{
    if (generic_button_event(win, xevent, B_T_SHADE, B_SHADE))
    {
	if (wnck_window_is_shaded(win))
	    wnck_window_unshade(win);
	else
	    wnck_window_shade(win);
    }
}


static void top_left_event(WnckWindow * win, XEvent * xevent)
{
    if (xevent->xbutton.button == 1)
	move_resize_window(win, WM_MOVERESIZE_SIZE_TOPLEFT, xevent);
}

static void top_event(WnckWindow * win, XEvent * xevent)
{
    if (xevent->xbutton.button == 1)
	move_resize_window(win, WM_MOVERESIZE_SIZE_TOP, xevent);
}

static void top_right_event(WnckWindow * win, XEvent * xevent)
{
    if (xevent->xbutton.button == 1)
	move_resize_window(win, WM_MOVERESIZE_SIZE_TOPRIGHT, xevent);
}

static void left_event(WnckWindow * win, XEvent * xevent)
{
    if (xevent->xbutton.button == 1)
	move_resize_window(win, WM_MOVERESIZE_SIZE_LEFT, xevent);
}

static void title_event(WnckWindow * win, XEvent * xevent)
{
    static unsigned int last_button_num = 0;
    static Window last_button_xwindow = None;
    static Time last_button_time = 0;
    decor_t *d = g_object_get_data(G_OBJECT(win), "decor");
    window_settings *ws = d->fs->ws;

    if (xevent->type != ButtonPress)
	return;

    if (xevent->xbutton.button == 1)
    {
	if (xevent->xbutton.button == last_button_num &&
	    xevent->xbutton.window == last_button_xwindow &&
	    xevent->xbutton.time < last_button_time + double_click_timeout)
	{
	    switch (ws->double_click_action)
	    {
		case DOUBLE_CLICK_SHADE:
		    if (wnck_window_is_shaded(win))
			wnck_window_unshade(win);
		    else
			wnck_window_shade(win);
		    break;
		case DOUBLE_CLICK_MAXIMIZE:
		    if (wnck_window_is_maximized(win))
			wnck_window_unmaximize(win);
		    else
			wnck_window_maximize(win);
		    break;
		case DOUBLE_CLICK_MINIMIZE:
		    wnck_window_minimize(win);
		    break;
		default:
		    break;
	    }

	    last_button_num = 0;
	    last_button_xwindow = None;
	    last_button_time = 0;
	}
	else
	{
	    last_button_num = xevent->xbutton.button;
	    last_button_xwindow = xevent->xbutton.window;
	    last_button_time = xevent->xbutton.time;

	    restack_window(win, Above);

	    move_resize_window(win, WM_MOVERESIZE_MOVE, xevent);
	}
    }
    else if (xevent->xbutton.button == 2)
	restack_window(win, Below);
    else if (xevent->xbutton.button == 3)
    {
	action_menu_map(win, xevent->xbutton.button, xevent->xbutton.time);
    }
    else if (xevent->xbutton.button == 4)
    {
	if (!wnck_window_is_shaded(win))
	    wnck_window_shade(win);
    }
    else if (xevent->xbutton.button == 5)
    {
	if (wnck_window_is_shaded(win))
	    wnck_window_unshade(win);
    }
}

static void right_event(WnckWindow * win, XEvent * xevent)
{
    if (xevent->xbutton.button == 1)
	move_resize_window(win, WM_MOVERESIZE_SIZE_RIGHT, xevent);
}

static void bottom_left_event(WnckWindow * win, XEvent * xevent)
{
    if (xevent->xbutton.button == 1)
	move_resize_window(win, WM_MOVERESIZE_SIZE_BOTTOMLEFT, xevent);
}

static void bottom_event(WnckWindow * win, XEvent * xevent)
{
    if (xevent->xbutton.button == 1)
	move_resize_window(win, WM_MOVERESIZE_SIZE_BOTTOM, xevent);
}

static void bottom_right_event(WnckWindow * win, XEvent * xevent)
{
    if (xevent->xbutton.button == 1)
	move_resize_window(win, WM_MOVERESIZE_SIZE_BOTTOMRIGHT, xevent);
}

static void force_quit_dialog_realize(GtkWidget * dialog, void *data)
{
    WnckWindow *win = data;

    gdk_error_trap_push();
    XSetTransientForHint(GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()),
			 GDK_WINDOW_XID(dialog->window),
			 wnck_window_get_xid(win));
    XSync(GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), FALSE);
    gdk_error_trap_pop();
}

static char *get_client_machine(Window xwindow)
{
    Atom atom, type;
    gulong nitems, bytes_after;
    gchar *str = NULL;
    unsigned char *sstr = NULL;
    int format, result;
    char *retval;

    atom = XInternAtom(GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), "WM_CLIENT_MACHINE", FALSE);

    gdk_error_trap_push();

    result = XGetWindowProperty(GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()),
				xwindow, atom,
				0, G_MAXLONG,
				FALSE, XA_STRING, &type, &format, &nitems,
				&bytes_after, &sstr);

    str = (gchar *) sstr;
    gdk_error_trap_pop();

    if (result != Success)
	return NULL;

    if (type != XA_STRING)
    {
	XFree(str);
	return NULL;
    }

    retval = g_strdup(str);

    XFree(str);

    return retval;
}

static void kill_window(WnckWindow * win)
{
    WnckApplication *app;

    app = wnck_window_get_application(win);
    if (app)
    {
	gchar buf[257], *client_machine;
	int pid;

	pid = wnck_application_get_pid(app);
	client_machine = get_client_machine(wnck_application_get_xid(app));

	if (client_machine && pid > 0)
	{
	    if (gethostname(buf, sizeof(buf) - 1) == 0)
	    {
		if (strcmp(buf, client_machine) == 0)
		    kill(pid, 9);
	    }
	}

	if (client_machine)
	    g_free(client_machine);
    }

    gdk_error_trap_push();
    XKillClient(GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), wnck_window_get_xid(win));
    XSync(GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), FALSE);
    gdk_error_trap_pop();
}

static void
force_quit_dialog_response(GtkWidget * dialog, gint response, void *data)
{
    WnckWindow *win = data;
    decor_t *d = g_object_get_data(G_OBJECT(win), "decor");

    if (response == GTK_RESPONSE_ACCEPT)
	kill_window(win);

    if (d->force_quit_dialog)
    {
	d->force_quit_dialog = NULL;
	gtk_widget_destroy(dialog);
    }
}

static void show_force_quit_dialog(WnckWindow * win, Time timestamp)
{
    decor_t *d = g_object_get_data(G_OBJECT(win), "decor");
    GtkWidget *dialog;
    gchar *str, *tmp;
    const gchar *name;

    if (d->force_quit_dialog)
	return;

    name = wnck_window_get_name(win);
    if (!name)
	name = "";

    tmp = g_markup_escape_text(name, -1);
    str = g_strdup_printf(_("The window \"%s\" is not responding."), tmp);

    g_free(tmp);

    dialog = gtk_message_dialog_new(NULL, 0,
				    GTK_MESSAGE_WARNING,
				    GTK_BUTTONS_NONE,
				    "<b>%s</b>\n\n%s",
				    str,
				    _("Forcing this application to "
				      "quit will cause you to lose any "
				      "unsaved changes."));
    g_free(str);

    gtk_window_set_icon_name(GTK_WINDOW(dialog), "force-quit");

    gtk_label_set_use_markup(GTK_LABEL(GTK_MESSAGE_DIALOG(dialog)->label),
			     TRUE);
    gtk_label_set_line_wrap(GTK_LABEL(GTK_MESSAGE_DIALOG(dialog)->label),
			    TRUE);

    gtk_dialog_add_buttons(GTK_DIALOG(dialog),
			   GTK_STOCK_CANCEL,
			   GTK_RESPONSE_REJECT,
			   _("_Force Quit"), GTK_RESPONSE_ACCEPT, NULL);

    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_REJECT);

    g_signal_connect(G_OBJECT(dialog), "realize",
		     G_CALLBACK(force_quit_dialog_realize), win);

    g_signal_connect(G_OBJECT(dialog), "response",
		     G_CALLBACK(force_quit_dialog_response), win);

    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);

    gtk_widget_realize(dialog);

    gdk_x11_window_set_user_time(dialog->window, timestamp);

    gtk_widget_show(dialog);

    d->force_quit_dialog = dialog;
}

static void hide_force_quit_dialog(WnckWindow * win)
{
    decor_t *d = g_object_get_data(G_OBJECT(win), "decor");

    if (d->force_quit_dialog)
    {
	gtk_widget_destroy(d->force_quit_dialog);
	d->force_quit_dialog = NULL;
    }
}

static GdkFilterReturn
event_filter_func(GdkXEvent * gdkxevent, GdkEvent * event, gpointer data)
{
    Display *xdisplay;
    GdkDisplay *gdkdisplay;
    XEvent *xevent = gdkxevent;
    gulong xid = 0;

    gdkdisplay = gdk_display_get_default();
    xdisplay = GDK_DISPLAY_XDISPLAY(gdkdisplay);

    switch (xevent->type)
    {
	case ButtonPress:
	case ButtonRelease:
	    xid = (gulong)
		g_hash_table_lookup(frame_table,
				    GINT_TO_POINTER(xevent->xbutton.window));
	    break;
	case EnterNotify:
	case LeaveNotify:
	    xid = (gulong)
		g_hash_table_lookup(frame_table,
				    GINT_TO_POINTER(xevent->xcrossing.
						    window));
	    break;
	case MotionNotify:
	    xid = (gulong)
		g_hash_table_lookup(frame_table,
				    GINT_TO_POINTER(xevent->xmotion.window));
	    break;
	case PropertyNotify:
	    if (xevent->xproperty.atom == frame_window_atom)
	    {
		WnckWindow *win;

		xid = xevent->xproperty.window;

		win = wnck_window_get(xid);
		if (win)
		{
		    Window frame;

		    if (get_window_prop(xid, frame_window_atom, &frame))
			add_frame_window(win, frame);
		    else
			remove_frame_window(win);
		}
	    }
	    else if (xevent->xproperty.atom == mwm_hints_atom)
	    {
		WnckWindow *win;

		xid = xevent->xproperty.window;

		win = wnck_window_get(xid);
		if (win)
		{
		    decor_t *d = g_object_get_data(G_OBJECT(win), "decor");
		    gboolean decorated = FALSE;

		    if (get_mwm_prop(xid) & (MWM_DECOR_ALL | MWM_DECOR_TITLE))
			decorated = TRUE;

		    if (decorated != d->decorated)
		    {
			d->decorated = decorated;
			if (decorated)
			{
			    d->width = d->height = 0;

			    update_window_decoration_size(win);
			    update_event_windows(win);
			}
			else
			{
			    gdk_error_trap_push();
			    XDeleteProperty(xdisplay, xid, win_decor_atom);
			    XSync(xdisplay, FALSE);
			    gdk_error_trap_pop();
			}
		    }
		}
	    }
	    else if (xevent->xproperty.atom == select_window_atom)
	    {
		WnckWindow *win;

		xid = xevent->xproperty.window;

		win = wnck_window_get(xid);
		if (win)
		{
		    Window select;

		    if (get_window_prop(xid, select_window_atom, &select))
			update_switcher_window(win, select);
		}
	    }
	    break;
	case DestroyNotify:
	    g_hash_table_remove(frame_table,
				GINT_TO_POINTER(xevent->xproperty.window));
	    break;
	case ClientMessage:
	    if (xevent->xclient.message_type == toolkit_action_atom)
	    {
		unsigned long action;

		action = xevent->xclient.data.l[0];
		if (action == toolkit_action_window_menu_atom)
		{
		    WnckWindow *win;

		    win = wnck_window_get(xevent->xclient.window);
		    if (win)
		    {
			action_menu_map(win,
					xevent->xclient.data.l[2],
					xevent->xclient.data.l[1]);
		    }
		}
		else if (action == toolkit_action_force_quit_dialog_atom)
		{
		    WnckWindow *win;

		    win = wnck_window_get(xevent->xclient.window);
		    if (win)
		    {
			if (xevent->xclient.data.l[2])
			    show_force_quit_dialog(win,
						   xevent->xclient.data.l[1]);
			else
			    hide_force_quit_dialog(win);
		    }
		}
	    }
	    else if (xevent->xclient.message_type == emerald_sigusr1_atom)
	    {
		reload_all_settings(SIGUSR1);
	    }

	default:
	    break;
    }

    if (xid)
    {
	WnckWindow *win;

	win = wnck_window_get(xid);
	if (win)
	{
	    static event_callback callback[3][3] = {
		{top_left_event, top_event, top_right_event},
		{left_event, title_event, right_event},
		{bottom_left_event, bottom_event, bottom_right_event}
	    };
	    static event_callback button_callback[B_T_COUNT] = {
		close_button_event,
		max_button_event,
		min_button_event,
		help_button_event,
		menu_button_event,
		shade_button_event,
		above_button_event,
		sticky_button_event,
	    };
	    decor_t *d = g_object_get_data(G_OBJECT(win), "decor");

	    if (d->decorated)
	    {
		gint i, j;

		for (i = 0; i < 3; i++)
		    for (j = 0; j < 3; j++)
			if (d->event_windows[i][j] == xevent->xany.window)
			    (*callback[i][j]) (win, xevent);

		for (i = 0; i < B_T_COUNT; i++)
		    if (d->button_windows[i] == xevent->xany.window)
			(*button_callback[i]) (win, xevent);
	    }
	}
    }

    return GDK_FILTER_CONTINUE;
}

static GdkFilterReturn
selection_event_filter_func(GdkXEvent * gdkxevent,
			    GdkEvent * event, gpointer data)
{
    Display *xdisplay;
    GdkDisplay *gdkdisplay;
    XEvent *xevent = gdkxevent;
    int status;

    gdkdisplay = gdk_display_get_default();
    xdisplay = GDK_DISPLAY_XDISPLAY(gdkdisplay);

    switch (xevent->type)
    {
	case SelectionRequest:
	    decor_handle_selection_request(xdisplay, xevent, dm_sn_timestamp);
	    break;
	case SelectionClear:
	    status = decor_handle_selection_clear(xdisplay, xevent, 0);
	    if (status == DECOR_SELECTION_GIVE_UP)
		exit(0);
	    break;
	default:
	    break;
    }

    return GDK_FILTER_CONTINUE;
}

#if G_MAXINT != G_MAXLONG
/* XRenderSetPictureFilter used to be broken on LP64. This
 * works with either the broken or fixed version.
 */
static void
XRenderSetPictureFilter_wrapper(Display * dpy,
				Picture picture,
				char *filter, XFixed * params, int nparams)
{
    gdk_error_trap_push();
    XRenderSetPictureFilter(dpy, picture, filter, params, nparams);
    XSync(dpy, False);
    if (gdk_error_trap_pop())
    {
	long *long_params = g_new(long, nparams);
	int i;

	for (i = 0; i < nparams; i++)
	    long_params[i] = params[i];

	XRenderSetPictureFilter(dpy, picture, filter,
				(XFixed *) long_params, nparams);
	g_free(long_params);
    }
}

#define XRenderSetPictureFilter XRenderSetPictureFilter_wrapper
#endif

static void
set_picture_transform(Display * xdisplay, Picture p, int dx, int dy)
{
    XTransform transform = {
	{
	    {1 << 16, 0, -dx << 16},
	    {0, 1 << 16, -dy << 16},
	    {0, 0, 1 << 16},
	}
    };

    XRenderSetPictureTransform(xdisplay, p, &transform);
}

static XFixed *create_gaussian_kernel(double radius,
				      double sigma,
				      double alpha,
				      double opacity, int *r_size)
{
    XFixed *params;
    double *amp, scale, x_scale, fx, sum;
    int size, x, i, n;

    scale = 1.0f / (2.0f * M_PI * sigma * sigma);

    if (alpha == -0.5f)
	alpha = 0.5f;

    size = alpha * 2 + 2;

    x_scale = 2.0f * radius / size;

    if (size < 3)
	return NULL;

    n = size;

    amp = g_malloc(sizeof(double) * n);
    if (!amp)
	return NULL;

    n += 2;

    params = g_malloc(sizeof(XFixed) * n);
    if (!params)
	return NULL;

    i = 0;
    sum = 0.0f;

    for (x = 0; x < size; x++)
    {
	fx = x_scale * (x - alpha - 0.5f);

	amp[i] = scale * exp((-1.0f * (fx * fx)) / (2.0f * sigma * sigma));

	sum += amp[i];

	i++;
    }

    /* normalize */
    if (sum)
	sum = 1.0 / sum;

    params[0] = params[1] = 0;

    for (i = 2; i < n; i++)
	params[i] = XDoubleToFixed(amp[i - 2] * sum * opacity * 1.2);

    g_free(amp);

    *r_size = size;

    return params;
}

/* to save some memory, value is specific to current decorations */
#define CORNER_REDUCTION 3

static int update_shadow(frame_settings * fs)
{
    Display *xdisplay = GDK_DISPLAY_XDISPLAY (gdk_display_get_default ());
    GdkVisual *visual = gdk_visual_get_best_with_depth(32);
    XRenderPictFormat *format;
    GdkPixmap *pixmap;
    Picture src, dst, tmp;
    XFixed *params;
    XFilters *filters;
    char *filter = NULL;
    int size, n_params = 0;
    cairo_t *cr;
    decor_t d;

    bzero(&d, sizeof(decor_t));
    window_settings *ws = fs->ws;

    // TODO shadows show strong artefacts with 30-bit setups
    // meanwhile disable here as a workaround
    if (gdk_visual_get_bits_per_rgb(visual) == 10)
	ws->shadow_radius = 0;

    //    double        save_decoration_alpha;
    static XRenderColor color;
    static XRenderColor clear = { 0x0000, 0x0000, 0x0000, 0x0000 };
    static XRenderColor white = { 0xffff, 0xffff, 0xffff, 0xffff };

    color.red = ws->shadow_color[0];
    color.green = ws->shadow_color[1];
    color.blue = ws->shadow_color[2];
    color.alpha = 0xffff;

    /* compute a gaussian convolution kernel */
    params = create_gaussian_kernel(ws->shadow_radius, ws->shadow_radius / 2.0,	// SIGMA
				    ws->shadow_radius,	// ALPHA
				    ws->shadow_opacity, &size);
    if (!params)
	ws->shadow_offset_x = ws->shadow_offset_y = size = 0;

    if (ws->shadow_radius <= 0.0 && ws->shadow_offset_x == 0 &&
	ws->shadow_offset_y == 0)
	size = 0;

    n_params = size + 2;
    size = size / 2;

    ws->left_space = ws->win_extents.left + size - ws->shadow_offset_x;
    ws->right_space = ws->win_extents.right + size + ws->shadow_offset_x;
    ws->top_space = ws->win_extents.top + size - ws->shadow_offset_y;
    ws->bottom_space = ws->win_extents.bottom + size + ws->shadow_offset_y;


    ws->left_space = MAX(ws->win_extents.left, ws->left_space);
    ws->right_space = MAX(ws->win_extents.right, ws->right_space);
    ws->top_space = MAX(ws->win_extents.top, ws->top_space);
    ws->bottom_space = MAX(ws->win_extents.bottom, ws->bottom_space);

    ws->shadow_left_space = MAX(0, size - ws->shadow_offset_x);
    ws->shadow_right_space = MAX(0, size + ws->shadow_offset_x);
    ws->shadow_top_space = MAX(0, size - ws->shadow_offset_y);
    ws->shadow_bottom_space = MAX(0, size + ws->shadow_offset_y);

    ws->shadow_left_corner_space = MAX(0, size + ws->shadow_offset_x);
    ws->shadow_right_corner_space = MAX(0, size - ws->shadow_offset_x);
    ws->shadow_top_corner_space = MAX(0, size + ws->shadow_offset_y);
    ws->shadow_bottom_corner_space = MAX(0, size - ws->shadow_offset_y);

    ws->left_corner_space =
	MAX(0, ws->shadow_left_corner_space - CORNER_REDUCTION);
    ws->right_corner_space =
	MAX(0, ws->shadow_right_corner_space - CORNER_REDUCTION);
    ws->top_corner_space =
	MAX(0, ws->shadow_top_corner_space - CORNER_REDUCTION);
    ws->bottom_corner_space =
	MAX(0, ws->shadow_bottom_corner_space - CORNER_REDUCTION);

    ws->normal_top_corner_space =
	MAX(0, ws->top_corner_space - ws->titlebar_height);
    ws->switcher_top_corner_space =
	MAX(0, ws->top_corner_space - SWITCHER_TOP_EXTRA);
    ws->switcher_bottom_corner_space =
	MAX(0, ws->bottom_corner_space - SWITCHER_SPACE);

    d.buffer_pixmap = NULL;
    d.layout = NULL;
    d.icon = NULL;
    d.state = 0;
    d.actions = 0;
    d.prop_xid = 0;
    d.draw = draw_shadow_window;
    d.active = TRUE;
    d.fs = fs;

    reset_buttons_bg_and_fade(&d);

    d.width =
	ws->left_space + ws->left_corner_space + 1 +
	ws->right_corner_space + ws->right_space;
    d.height =
	ws->top_space + ws->titlebar_height +
	ws->normal_top_corner_space + 2 + ws->bottom_corner_space +
	ws->bottom_space;

    /* all pixmaps are ARGB32 */
    format = XRenderFindStandardFormat(xdisplay, PictStandardARGB32);

    /* shadow color */
    src = XRenderCreateSolidFill(xdisplay, &color);

    if (ws->large_shadow_pixmap)
    {
	g_object_unref (G_OBJECT (ws->large_shadow_pixmap));
	ws->large_shadow_pixmap = NULL;
    }

    if (ws->shadow_pattern)
    {
	cairo_pattern_destroy(ws->shadow_pattern);
	ws->shadow_pattern = NULL;
    }

    if (ws->shadow_pixmap)
    {
	g_object_unref (G_OBJECT (ws->shadow_pixmap));
	ws->shadow_pixmap = NULL;
    }

    /* no shadow */
    if (size <= 0)
    {
	if (params)
	    g_free(params);

	return 1;
    }

    pixmap = create_pixmap(d.width, d.height);
    if (!pixmap)
    {
	g_free(params);
	return 0;
    }

    /* query server for convolution filter */
    filters = XRenderQueryFilters(xdisplay, GDK_PIXMAP_XID(pixmap));
    if (filters)
    {
	int i;

	for (i = 0; i < filters->nfilter; i++)
	{
	    if (strcmp(filters->filter[i], FilterConvolution) == 0)
	    {
		filter = FilterConvolution;
		break;
	    }
	}

	XFree(filters);
    }

    if (!filter)
    {
	fprintf(stderr, "can't generate shadows, X server doesn't support "
		"convolution filters\n");

	g_free(params);
	g_object_unref (G_OBJECT (pixmap));
	return 1;
    }

    /* WINDOWS WITH DECORATION */

    d.pixmap = create_pixmap(d.width, d.height);
    if (!d.pixmap)
    {
	g_free(params);
	g_object_unref (G_OBJECT (pixmap));
	return 0;
    }

    /* draw decorations */
    (*d.draw) (&d);

    dst = XRenderCreatePicture(xdisplay, GDK_PIXMAP_XID(d.pixmap),
			       format, 0, NULL);
    tmp = XRenderCreatePicture(xdisplay, GDK_PIXMAP_XID(pixmap),
			       format, 0, NULL);

    /* first pass */
    params[0] = (n_params - 2) << 16;
    params[1] = 1 << 16;

    set_picture_transform(xdisplay, dst, ws->shadow_offset_x, 0);
    XRenderSetPictureFilter(xdisplay, dst, filter, params, n_params);
    XRenderComposite(xdisplay,
		     PictOpSrc,
		     src, dst, tmp, 0, 0, 0, 0, 0, 0, d.width, d.height);

    /* second pass */
    params[0] = 1 << 16;
    params[1] = (n_params - 2) << 16;

    set_picture_transform(xdisplay, tmp, 0, ws->shadow_offset_y);
    XRenderSetPictureFilter(xdisplay, tmp, filter, params, n_params);
    XRenderComposite(xdisplay,
		     PictOpSrc,
		     src, tmp, dst, 0, 0, 0, 0, 0, 0, d.width, d.height);

    XRenderFreePicture(xdisplay, tmp);
    XRenderFreePicture(xdisplay, dst);

    g_object_unref (G_OBJECT (pixmap));

    ws->large_shadow_pixmap = d.pixmap;

    cr = gdk_cairo_create(GDK_DRAWABLE(ws->large_shadow_pixmap));
    ws->shadow_pattern =
	cairo_pattern_create_for_surface(cairo_get_target(cr));
    cairo_pattern_set_filter(ws->shadow_pattern, CAIRO_FILTER_NEAREST);
    cairo_destroy(cr);


    /* WINDOWS WITHOUT DECORATIONS */

    d.width = ws->shadow_left_space + ws->shadow_left_corner_space + 1 +
	ws->shadow_right_space + ws->shadow_right_corner_space;
    d.height = ws->shadow_top_space + ws->shadow_top_corner_space + 1 +
	ws->shadow_bottom_space + ws->shadow_bottom_corner_space;

    pixmap = create_pixmap(d.width, d.height);
    if (!pixmap)
    {
	g_free(params);
	return 0;
    }

    d.pixmap = create_pixmap(d.width, d.height);
    if (!d.pixmap)
    {
	g_object_unref (G_OBJECT (pixmap));
	g_free(params);
	return 0;
    }

    dst = XRenderCreatePicture(xdisplay, GDK_PIXMAP_XID(d.pixmap),
			       format, 0, NULL);

    /* draw rectangle */
    XRenderFillRectangle(xdisplay, PictOpSrc, dst, &clear,
			 0, 0, d.width, d.height);
    XRenderFillRectangle(xdisplay, PictOpSrc, dst, &white,
			 ws->shadow_left_space,
			 ws->shadow_top_space,
			 d.width - ws->shadow_left_space -
			 ws->shadow_right_space,
			 d.height - ws->shadow_top_space -
			 ws->shadow_bottom_space);

    tmp = XRenderCreatePicture(xdisplay, GDK_PIXMAP_XID(pixmap),
			       format, 0, NULL);

    /* first pass */
    params[0] = (n_params - 2) << 16;
    params[1] = 1 << 16;

    set_picture_transform(xdisplay, dst, ws->shadow_offset_x, 0);
    XRenderSetPictureFilter(xdisplay, dst, filter, params, n_params);
    XRenderComposite(xdisplay,
		     PictOpSrc,
		     src, dst, tmp, 0, 0, 0, 0, 0, 0, d.width, d.height);

    /* second pass */
    params[0] = 1 << 16;
    params[1] = (n_params - 2) << 16;

    set_picture_transform(xdisplay, tmp, 0, ws->shadow_offset_y);
    XRenderSetPictureFilter(xdisplay, tmp, filter, params, n_params);
    XRenderComposite(xdisplay,
		     PictOpSrc,
		     src, tmp, dst, 0, 0, 0, 0, 0, 0, d.width, d.height);

    XRenderFreePicture(xdisplay, tmp);
    XRenderFreePicture(xdisplay, dst);
    XRenderFreePicture(xdisplay, src);

    g_object_unref (G_OBJECT (pixmap));

    g_free(params);

    ws->shadow_pixmap = d.pixmap;

    return 1;
}
static void titlebar_font_changed(window_settings * ws)
{
    PangoFontMetrics *metrics;
    PangoLanguage *lang;

    pango_context_set_font_description(ws->pango_context, ws->font_desc);
    lang = pango_context_get_language(ws->pango_context);
    metrics =
	pango_context_get_metrics(ws->pango_context, ws->font_desc, lang);

    ws->text_height = PANGO_PIXELS(pango_font_metrics_get_ascent(metrics) +
				   pango_font_metrics_get_descent(metrics));

    ws->titlebar_height = ws->text_height;
    if (ws->titlebar_height < ws->min_titlebar_height)
	ws->titlebar_height = ws->min_titlebar_height;

    pango_font_metrics_unref(metrics);

}
static void load_buttons_image(window_settings * ws, gint y)
{
    gchar *file;
    int x, pix_width, pix_height, rel_button;

    rel_button = get_b_offset(y);



    if (ws->ButtonArray[y])
	g_object_unref(ws->ButtonArray[y]);
    file = make_filename("buttons", b_types[y], "png");
    if (!file || !(ws->ButtonArray[y] = gdk_pixbuf_new_from_file(file, NULL)))
	ws->ButtonArray[y] = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, 16 * S_COUNT, 16);	// create a blank pixbuf
    g_free(file);

    pix_width = gdk_pixbuf_get_width(ws->ButtonArray[y]) / S_COUNT;
    pix_height = gdk_pixbuf_get_height(ws->ButtonArray[y]);
    ws->c_icon_size[rel_button].w = pix_width;
    ws->c_icon_size[rel_button].h = pix_height;
    for (x = 0; x < S_COUNT; x++)
    {
	if (ws->ButtonPix[x + y * S_COUNT])
	    g_object_unref(ws->ButtonPix[x + y * S_COUNT]);

	ws->ButtonPix[x + y * S_COUNT] =
	    gdk_pixbuf_new_subpixbuf(ws->ButtonArray[y], x * pix_width, 0,
				     pix_width, pix_height);
    }
}
static void load_buttons_glow_images(window_settings * ws)
{
    gchar *file1 = NULL;
    gchar *file2 = NULL;
    int x, pix_width, pix_height;
    int pix_width2, pix_height2;
    gboolean success1 = FALSE;
    gboolean success2 = FALSE;

    if (ws->use_button_glow)
    {
	if (ws->ButtonGlowArray)
	    g_object_unref(ws->ButtonGlowArray);
	file1 = make_filename("buttons", "glow", "png");
	if (file1 &&
	    (ws->ButtonGlowArray = gdk_pixbuf_new_from_file(file1, NULL)))
	    success1 = TRUE;
    }
    if (ws->use_button_inactive_glow)
    {
	if (ws->ButtonInactiveGlowArray)
	    g_object_unref(ws->ButtonInactiveGlowArray);
	file2 = make_filename("buttons", "inactive_glow", "png");
	if (file2 &&
	    (ws->ButtonInactiveGlowArray =
	     gdk_pixbuf_new_from_file(file2, NULL)))
	    success2 = TRUE;
    }
    if (success1 && success2)
    {
	pix_width = gdk_pixbuf_get_width(ws->ButtonGlowArray) / B_COUNT;
	pix_height = gdk_pixbuf_get_height(ws->ButtonGlowArray);
	pix_width2 =
	    gdk_pixbuf_get_width(ws->ButtonInactiveGlowArray) / B_COUNT;
	pix_height2 = gdk_pixbuf_get_height(ws->ButtonInactiveGlowArray);

	if (pix_width != pix_width2 || pix_height != pix_height2)
	{
	    g_warning
		("Choose same size glow images for active and inactive windows."
		 "\nInactive glow image is scaled for now.");
	    // Scale the inactive one
	    GdkPixbuf *tmp_pixbuf =
		gdk_pixbuf_new(gdk_pixbuf_get_colorspace
			       (ws->ButtonGlowArray),
			       TRUE,
			       gdk_pixbuf_get_bits_per_sample(ws->
							      ButtonGlowArray),
			       pix_width * B_COUNT,
			       pix_height);

	    gdk_pixbuf_scale(ws->ButtonInactiveGlowArray, tmp_pixbuf,
			     0, 0,
			     pix_width * B_COUNT, pix_height,
			     0, 0,
			     pix_width / (double)pix_width2,
			     pix_height / (double)pix_height2,
			     GDK_INTERP_BILINEAR);
	    g_object_unref(ws->ButtonInactiveGlowArray);
	    ws->ButtonInactiveGlowArray = tmp_pixbuf;
	}
    }
    else
    {
	pix_width = 16;
	pix_height = 16;
	if (success1)
	{
	    pix_width = gdk_pixbuf_get_width(ws->ButtonGlowArray) / B_COUNT;
	    pix_height = gdk_pixbuf_get_height(ws->ButtonGlowArray);
	}
	else if (success2)
	{
	    pix_width =
		gdk_pixbuf_get_width(ws->ButtonInactiveGlowArray) /
		B_COUNT;
	    pix_height = gdk_pixbuf_get_height(ws->ButtonInactiveGlowArray);
	}
	if (!success1 && ws->use_button_glow)
	    ws->ButtonGlowArray = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, pix_width * B_COUNT, pix_height);	// create a blank pixbuf
	if (!success2 && ws->use_button_inactive_glow)
	    ws->ButtonInactiveGlowArray = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, pix_width * B_COUNT, pix_height);	// create a blank pixbuf
    }
    ws->c_glow_size.w = pix_width;
    ws->c_glow_size.h = pix_height;

    if (ws->use_button_glow)
    {
	g_free(file1);
	for (x = 0; x < B_COUNT; x++)
	{
	    if (ws->ButtonGlowPix[x])
		g_object_unref(ws->ButtonGlowPix[x]);
	    ws->ButtonGlowPix[x] =
		gdk_pixbuf_new_subpixbuf(ws->ButtonGlowArray,
					 x * pix_width, 0, pix_width,
					 pix_height);
	}
    }
    if (ws->use_button_inactive_glow)
    {
	g_free(file2);
	for (x = 0; x < B_COUNT; x++)
	{
	    if (ws->ButtonInactiveGlowPix[x])
		g_object_unref(ws->ButtonInactiveGlowPix[x]);
	    ws->ButtonInactiveGlowPix[x] =
		gdk_pixbuf_new_subpixbuf(ws->ButtonInactiveGlowArray,
					 x * pix_width, 0, pix_width,
					 pix_height);
	}
    }
}
void load_button_image_setting(window_settings * ws)
{
    gint i;

    for (i = 0; i < B_COUNT; i++)
	load_buttons_image(ws, i);

    // load active and inactive glow image
    if (ws->use_button_glow || ws->use_button_inactive_glow)
	load_buttons_glow_images(ws);
}
static void load_settings(window_settings * ws)
{
    gchar *path =
	g_strjoin("/", g_get_home_dir(), ".emerald/settings.ini", NULL);
    GKeyFile *f = g_key_file_new();

    copy_from_defaults_if_needed();

    //settings
    g_key_file_load_from_file(f, path, 0, NULL);
    g_free(path);
    load_int_setting(f, &ws->double_click_action, "double_click_action",
		     "titlebars");
    load_int_setting(f, &ws->button_hover_cursor, "hover_cursor", "buttons");
    load_bool_setting(f, &ws->use_decoration_cropping,
		      "use_decoration_cropping", "decorations");
    load_bool_setting(f, &ws->use_button_fade, "use_button_fade", "buttons");
    gint button_fade_step_duration = ws->button_fade_step_duration;

    load_int_setting(f, &button_fade_step_duration,
		     "button_fade_step_duration", "buttons");
    if (button_fade_step_duration > 0)
	ws->button_fade_step_duration = button_fade_step_duration;
    gint button_fade_total_duration = 250;

    load_int_setting(f, &button_fade_total_duration,
		     "button_fade_total_duration", "buttons");
    if (button_fade_total_duration > 0)
	ws->button_fade_num_steps =
	    button_fade_total_duration / ws->button_fade_step_duration;
    if (ws->button_fade_num_steps == 0)
	ws->button_fade_num_steps = 1;
    gboolean use_button_fade_pulse = FALSE;

    load_bool_setting(f, &use_button_fade_pulse, "use_button_fade_pulse",
		      "buttons");

    ws->button_fade_pulse_wait_steps = 0;
    if (use_button_fade_pulse)
    {
	gint button_fade_pulse_min_opacity = 0;

	load_int_setting(f, &button_fade_pulse_min_opacity,
			 "button_fade_pulse_min_opacity", "buttons");
	ws->button_fade_pulse_len_steps =
	    ws->button_fade_num_steps * (100 -
					 button_fade_pulse_min_opacity) /
	    100;
	gint button_fade_pulse_wait_duration = 0;

	load_int_setting(f, &button_fade_pulse_wait_duration,
			 "button_fade_pulse_wait_duration", "buttons");
	if (button_fade_pulse_wait_duration > 0)
	    ws->button_fade_pulse_wait_steps =
		button_fade_pulse_wait_duration /
		ws->button_fade_step_duration;
    }
    else
	ws->button_fade_pulse_len_steps = 0;

    load_bool_setting(f, &enable_tooltips, "enable_tooltips", "buttons");
    load_int_setting(f, &ws->blur_type,
		     "blur_type", "decorations");

    //theme
    path = g_strjoin("/", g_get_home_dir(), ".emerald/theme/theme.ini", NULL);
    g_key_file_load_from_file(f, path, 0, NULL);
    g_free(path);
    load_string_setting(f, &engine, "engine", "engine");
    if (!load_engine(engine, ws))
    {
	if (engine)
	    g_free(engine);
	engine = g_strdup("legacy");
	load_engine(engine, ws);
    }
    LFACSS(text, titlebar);
    LFACSS(text_halo, titlebar);
    LFACSS(button, buttons);
    LFACSS(button_halo, buttons);
    load_engine_settings(f, ws);
    load_font_setting(f, &ws->font_desc, "titlebar_font", "titlebar");
    load_bool_setting(f, &ws->use_pixmap_buttons, "use_pixmap_buttons",
		      "buttons");
    load_bool_setting(f, &ws->use_button_glow, "use_button_glow", "buttons");
    load_bool_setting(f, &ws->use_button_inactive_glow,
		      "use_button_inactive_glow", "buttons");

    if (ws->use_pixmap_buttons)
	load_button_image_setting(ws);
    load_shadow_color_setting(f, ws->shadow_color, "shadow_color", "shadow");
    load_int_setting(f, &ws->shadow_offset_x, "shadow_offset_x", "shadow");
    load_int_setting(f, &ws->shadow_offset_y, "shadow_offset_y", "shadow");
    load_float_setting(f, &ws->shadow_radius, "shadow_radius", "shadow");
    load_float_setting(f, &ws->shadow_opacity, "shadow_opacity", "shadow");
    load_string_setting(f, &ws->tobj_layout, "title_object_layout",
			"titlebar");
    load_int_setting(f, &ws->button_offset, "vertical_offset", "buttons");
    load_int_setting(f, &ws->button_hoffset, "horizontal_offset", "buttons");
    load_int_setting(f, &ws->win_extents.top, "top", "borders");
    load_int_setting(f, &ws->win_extents.left, "left", "borders");
    load_int_setting(f, &ws->win_extents.right, "right", "borders");
    load_int_setting(f, &ws->win_extents.bottom, "bottom", "borders");
    load_int_setting(f, &ws->min_titlebar_height, "min_titlebar_height",
		     "titlebar");
    g_key_file_free(f);
}
static void update_settings(window_settings * ws)
{
    //assumes ws is fully allocated

    GdkDisplay *gdkdisplay;

    // Display    *xdisplay;
    GdkScreen *gdkscreen;
    WnckScreen *screen = wnck_screen_get_default();
    GList *windows;

    load_settings(ws);

    gdkdisplay = gdk_display_get_default();
    // xdisplay   = gdk_x11_display_get_xdisplay (gdkdisplay);
    gdkscreen = gdk_display_get_default_screen(gdkdisplay);

    titlebar_font_changed(ws);
    update_window_extents(ws);
    update_shadow(ws->fs_act);
    update_default_decorations(gdkscreen, ws->fs_act, ws->fs_inact);

    windows = wnck_screen_get_windows(screen);
    while (windows)
    {
	decor_t *d = g_object_get_data(G_OBJECT(windows->data), "decor");

	if (d->decorated)
	{
	    d->width = d->height = 0;
	    update_window_decoration_size(WNCK_WINDOW(windows->data));
	    update_event_windows(WNCK_WINDOW(windows->data));
	}
	windows = windows->next;
    }
}

#ifdef USE_DBUS
static DBusHandlerResult
dbus_signal_filter(DBusConnection * connection, DBusMessage * message,
		   void *user_data)
{
    if (dbus_message_is_signal
	(message, "org.metascape.emerald.dbus.Signal", "Reload"))
    {
	puts("Reloading...");
	update_settings(global_ws);
	return DBUS_HANDLER_RESULT_HANDLED;
    }
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

void dbc(DBusError * err)
{
    if (dbus_error_is_set(err))
    {
	fprintf(stderr, "emerald: Connection Error (%s)\n", err->message);
	dbus_error_free(err);
    }
}
#else
void reload_all_settings(int sig)
{
    if (sig == SIGUSR1)
	do_reload = TRUE;
}
#endif
gboolean reload_if_needed(gpointer p)
{
    if (do_reload)
    {
	do_reload = FALSE;
	puts("Reloading...");
	update_settings(global_ws);
    }
    return TRUE;
}

int main(int argc, char *argv[])
{
    GdkDisplay *gdkdisplay;
    Display *xdisplay;
    WnckScreen *screen;
    int status;

    gint i, j;
    gboolean replace = FALSE;
    PangoFontMetrics *metrics;
    PangoLanguage *lang;
    frame_settings *pfs;
    window_settings *ws;

    ws = malloc(sizeof(window_settings));
    bzero(ws, sizeof(window_settings));
    global_ws = ws;
    setlocale(LC_ALL, "");
    bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    textdomain(GETTEXT_PACKAGE);

    ws->win_extents.left = 6;
    ws->win_extents.top = 4;
    ws->win_extents.right = 6;
    ws->win_extents.bottom = 6;
    ws->corner_radius = 5;
    ws->shadow_radius = 15;
    ws->shadow_opacity = .8;
    ws->min_titlebar_height = 17;
    ws->double_click_action = DOUBLE_CLICK_SHADE;
    ws->button_hover_cursor = 1;
    ws->button_offset = 1;
    ws->button_hoffset = 1;
    ws->button_fade_step_duration = 50;
    ws->button_fade_num_steps = 5;
    ws->blur_type = BLUR_TYPE_NONE;

    ws->tobj_layout = g_strdup("IT::HNXC");	// DEFAULT TITLE OBJECT LAYOUT, does not use any odd buttons
    //ws->tobj_layout=g_strdup("CNX:IT:HM");

    pfs = malloc(sizeof(frame_settings));
    bzero(pfs, sizeof(frame_settings));
    pfs->ws = ws;
    ACOLOR(text, 1.0, 1.0, 1.0, 1.0);
    ACOLOR(text_halo, 0.0, 0.0, 0.0, 0.2);
    ACOLOR(button, 1.0, 1.0, 1.0, 0.8);
    ACOLOR(button_halo, 0.0, 0.0, 0.0, 0.2);
    ws->fs_act = pfs;

    pfs = malloc(sizeof(frame_settings));
    bzero(pfs, sizeof(frame_settings));
    pfs->ws = ws;
    ACOLOR(text, 0.8, 0.8, 0.8, 0.8);
    ACOLOR(text_halo, 0.0, 0.0, 0.0, 0.2);
    ACOLOR(button, 0.8, 0.8, 0.8, 0.8);
    ACOLOR(button_halo, 0.0, 0.0, 0.0, 0.2);
    ws->fs_inact = pfs;

    ws->round_top_left = TRUE;
    ws->round_top_right = TRUE;
    ws->round_bottom_left = TRUE;
    ws->round_bottom_right = TRUE;

    engine = g_strdup("legacy");
    load_engine(engine, ws);	// assumed to always return TRUE

    program_name = argv[0];

    //ws->ButtonBase = NULL;
    for (i = 0; i < (S_COUNT * B_COUNT); i++)
    {
	ws->ButtonPix[i] = NULL;
    }
    gtk_init(&argc, &argv);
    gdk_error_trap_push();
#ifdef USE_DBUS
    if (!g_thread_supported())
	g_thread_init(NULL);
    dbus_g_thread_init();
#endif

    for (i = 0; i < argc; i++)
    {
	if (strcmp(argv[i], "--replace") == 0)
	{
	    replace = TRUE;
	}
	else if (strcmp(argv[i], "--version") == 0)
	{
	    printf("%s: %s version %s\n", program_name, PACKAGE, VERSION);
	    return 0;
	}
	else if (strcmp(argv[i], "--help") == 0)
	{
	    fprintf(stderr, "%s [--replace] [--help] [--version]\n",
		    program_name);
	    return 0;
	}
    }

#ifdef USE_DBUS
    {
	DBusConnection *dbcon;
	DBusError err;

	dbus_error_init(&err);
	dbcon = dbus_bus_get(DBUS_BUS_SESSION, &err);
	dbc(&err);
	dbus_connection_setup_with_g_main(dbcon, NULL);
	dbc(&err);
	dbus_bus_request_name(dbcon, "org.metascape.emerald.dbus",
			      DBUS_NAME_FLAG_REPLACE_EXISTING |
			      DBUS_NAME_FLAG_ALLOW_REPLACEMENT, &err);
	dbc(&err);
	dbus_bus_add_match(dbcon,
			   "type='signal',interface='org.metascape.emerald.dbus.Signal'",
			   &err);
	dbc(&err);
	dbus_connection_add_filter(dbcon, dbus_signal_filter, NULL, NULL);
    }
#endif
    signal(SIGUSR1, reload_all_settings);


    gdkdisplay = gdk_display_get_default();
    xdisplay = gdk_x11_display_get_xdisplay(gdkdisplay);

    frame_window_atom = XInternAtom(xdisplay, "_NET_FRAME_WINDOW", FALSE);
    win_decor_atom = XInternAtom(xdisplay, DECOR_WINDOW_ATOM_NAME, FALSE);
    win_blur_decor_atom = XInternAtom (xdisplay, DECOR_BLUR_ATOM_NAME, FALSE);
    wm_move_resize_atom = XInternAtom(xdisplay, "_NET_WM_MOVERESIZE", FALSE);
    restack_window_atom = XInternAtom(xdisplay, "_NET_RESTACK_WINDOW", FALSE);
    select_window_atom = XInternAtom(xdisplay, DECOR_SWITCH_WINDOW_ATOM_NAME,
				     FALSE);
    mwm_hints_atom = XInternAtom(xdisplay, "_MOTIF_WM_HINTS", FALSE);
    switcher_fg_atom = XInternAtom (xdisplay,
				    DECOR_SWITCH_FOREGROUND_COLOR_ATOM_NAME,
				    FALSE);
    wm_protocols_atom = XInternAtom(xdisplay, "WM_PROTOCOLS", FALSE);
    net_wm_context_help_atom =
	XInternAtom(xdisplay, "_NET_WM_CONTEXT_HELP", FALSE);

    toolkit_action_atom =
	XInternAtom(xdisplay, "_COMPIZ_TOOLKIT_ACTION", FALSE);
    toolkit_action_window_menu_atom =
	XInternAtom(xdisplay, "_COMPIZ_TOOLKIT_ACTION_WINDOW_MENU",
		    FALSE);
    toolkit_action_force_quit_dialog_atom =
	XInternAtom(xdisplay, "_COMPIZ_TOOLKIT_ACTION_FORCE_QUIT_DIALOG",
		    FALSE);

    emerald_sigusr1_atom = XInternAtom(xdisplay, "emerald-sigusr1", FALSE);

    utf8_string_atom = XInternAtom(xdisplay, "UTF8_STRING", FALSE);

    status = decor_acquire_dm_session (xdisplay, DefaultScreen(xdisplay),
				       "emerald", replace, &dm_sn_timestamp);

    if (status != DECOR_ACQUIRE_STATUS_SUCCESS)
    {
	if (status == DECOR_ACQUIRE_STATUS_FAILED)
	{
	    fprintf (stderr,
		     "%s: Could not acquire decoration manager "
		     "selection on screen %d display \"%s\"\n",
		     program_name, DefaultScreen(xdisplay),
		     DisplayString (xdisplay));
	}
	else if (status == DECOR_ACQUIRE_STATUS_OTHER_DM_RUNNING)
	{
	    fprintf (stderr,
		     "%s: Screen %d on display \"%s\" already "
		     "has a decoration manager; try using the "
		     "--replace option to replace the current "
		     "decoration manager.\n",
		     program_name, DefaultScreen(xdisplay),
		     DisplayString (xdisplay));
	}

	return 1;
    }

    for (i = 0; i < 3; i++)
    {
	for (j = 0; j < 3; j++)
	{
	    if (cursor[i][j].shape != XC_left_ptr)
		cursor[i][j].cursor =
		    XCreateFontCursor(xdisplay, cursor[i][j].shape);
	}
    }
    if (button_cursor.shape != XC_left_ptr)
	button_cursor.cursor =
	    XCreateFontCursor(xdisplay, button_cursor.shape);

    frame_table = g_hash_table_new(NULL, NULL);

    if (!create_tooltip_window())
    {
	fprintf(stderr, "%s, Couldn't create tooltip window\n", argv[0]);
	return 1;
    }

    screen = wnck_screen_get_default();

    gdk_window_add_filter(NULL, selection_event_filter_func, NULL);

    gdk_window_add_filter(NULL, event_filter_func, NULL);

    connect_screen(screen);

    style_window = gtk_window_new(GTK_WINDOW_POPUP);
    gtk_widget_realize(style_window);
    ws->pango_context = gtk_widget_create_pango_context(style_window);
    ws->font_desc = pango_font_description_from_string("Sans Bold 12");
    pango_context_set_font_description(ws->pango_context, ws->font_desc);
    lang = pango_context_get_language(ws->pango_context);
    metrics =
	pango_context_get_metrics(ws->pango_context, ws->font_desc, lang);

    ws->text_height = PANGO_PIXELS(pango_font_metrics_get_ascent(metrics) +
				   pango_font_metrics_get_descent(metrics));

    ws->titlebar_height = ws->text_height;
    if (ws->titlebar_height < ws->min_titlebar_height)
	ws->titlebar_height = ws->min_titlebar_height;

    pango_font_metrics_unref(metrics);

    update_window_extents(ws);
    update_shadow(pfs);

    decor_set_dm_check_hint(xdisplay, DefaultScreen(xdisplay));

    update_settings(ws);

    g_timeout_add(500, reload_if_needed, NULL);

    gtk_main();
    gdk_error_trap_pop();

    return 0;
}
