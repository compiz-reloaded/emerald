/*
 * Line theme engine
 *
 * line.c
 *
 * Copyright (C) 2006 Quinn Storm <livinglatexkali@gmail.com>
 * Copyright (C) 2007 Patrick Niklaus <patrick.niklaus@googlemail.com>
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

#include <emerald.h>
#include <engine.h>

#include <legacy_icon.h>

#define SECT "line_settings"

/*
 * color privates
 */
typedef struct _private_fs
{
    alpha_color border;
	alpha_color title_bar;
} private_fs;

/*
 * settings privates
 */
typedef struct _private_ws
{
} private_ws;

void get_meta_info (EngineMetaInfo * emi)
{
    emi->version = g_strdup("0.1");
    emi->description = g_strdup(_("Based on original legacy"));
    emi->last_compat = g_strdup("0.0"); // old themes still compatible
    emi->icon = gdk_pixbuf_new_from_inline(-1, my_pixbuf, TRUE, NULL);
}

void engine_draw_frame (decor_t * d, cairo_t * cr)
{
    frame_settings *fs = d->fs;
    private_fs *pfs = fs->engine_fs;
    window_settings *ws = fs->ws;
    private_ws *pws = ws->engine_ws;

	double x1, y1, x2, y2, h;
    x1 = ws->left_space - ws->win_extents.left;
    y1 = ws->top_space + ws->titlebar_height;
    x2 = d->width  - ws->right_space  + ws->win_extents.right;
    y2 = d->height - ws->bottom_space + ws->win_extents.bottom;

	double m1 = MIN(ws->win_extents.left, ws->win_extents.right);
	double m2 = MIN(ws->win_extents.top,  ws->win_extents.bottom);
	double border_width = MIN(m1, m2);
	double border_offset = border_width/2.0;

/*	cairo_save(cr);
	cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
	rounded_rectangle (cr,
			x1 - ws->left_space,
			y1 - ws->titlebar_height - ws->top_space,
			x2 - x1 + ws->left_space + ws->right_space,
			y1 - border_offset*2.0,
			0, ws, 0);
	cairo_fill(cr);
	cairo_restore(cr);*/
	ws->top_space = ws->titlebar_height; // FIXME bad work-a-round

    cairo_set_line_width (cr, border_width);
    cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

	rounded_rectangle (cr,
            x1,
			y1 - ws->titlebar_height,
            x2 - x1,
			y1 - border_offset*2.0,
            0, ws, 0);
	cairo_set_source_alpha_color(cr, &pfs->title_bar);
	cairo_fill(cr);

    rounded_rectangle (cr,
            x1 + border_offset,
			y1 - border_offset,
            x2 - x1 - border_offset*2,
			y2 - y1,
            0, ws, 0);
	cairo_set_source_alpha_color(cr, &pfs->border);
	cairo_stroke (cr);
}

void load_engine_settings(GKeyFile * f, window_settings * ws)
{
    private_ws * pws = ws->engine_ws;
    PFACS(border);
	PFACS(title_bar);
}

void init_engine(window_settings * ws)
{
    private_fs * pfs;
    private_ws * pws;

    pws = malloc(sizeof(private_ws));
    ws->engine_ws = pws;
    bzero(pws,sizeof(private_ws));

    pfs = malloc(sizeof(private_fs));
    ws->fs_act->engine_fs = pfs;
    bzero(pfs, sizeof(private_fs));
    ACOLOR(border, 0.0, 0.0, 0.0, 1.0);
	ACOLOR(title_bar, 0.0, 0.0, 0.0, 0.3);

	pfs = malloc(sizeof(private_fs));
    ws->fs_inact->engine_fs = pfs;
    bzero(pfs,sizeof(private_fs));
    ACOLOR(border, 0.0, 0.0, 0.0, 1.0);
	ACOLOR(title_bar, 0.0, 0.0, 0.0, 0.0);
}

void fini_engine(window_settings * ws)
{
    free(ws->fs_act->engine_fs);
    free(ws->fs_inact->engine_fs);
}

void my_engine_settings(GtkWidget * hbox, gboolean active)
{
    GtkWidget * vbox;
    GtkWidget * scroller;
    vbox = gtk_vbox_new(FALSE,2);
    gtk_box_pack_startC(hbox, vbox, TRUE, TRUE, 0);
    gtk_box_pack_startC(vbox, gtk_label_new(active?"Active Window":"Inactive Window"), FALSE, FALSE, 0);
    gtk_box_pack_startC(vbox, gtk_hseparator_new(), FALSE, FALSE, 0);
    scroller = gtk_scrolled_window_new(NULL,NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroller), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_startC(vbox, scroller, TRUE, TRUE, 0);
    
    table_new(3, FALSE, FALSE);

    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scroller), GTK_WIDGET(get_current_table()));
    
    make_labels(_("Colors"));
    table_append_separator();
    ACAV(_("Outer Frame Blend"), "border", SECT);
	ACAV(_("Title Bar"), "title_bar", SECT);
}

void layout_engine_colors(GtkWidget * vbox)
{
    GtkWidget * hbox;
    hbox = gtk_hbox_new(FALSE, 2);
    gtk_box_pack_startC(vbox, hbox, TRUE, TRUE, 0);
    my_engine_settings(hbox, TRUE);
    gtk_box_pack_startC(hbox, gtk_vseparator_new(), FALSE, FALSE, 0);
    my_engine_settings(hbox, FALSE);
}

void layout_engine_settings(GtkWidget * vbox)
{
    GtkWidget * note;
    note = gtk_notebook_new();
    gtk_box_pack_startC(vbox, note, TRUE, TRUE, 0);
    layout_engine_colors(build_notebook_page(_("Colors"), note));
}
