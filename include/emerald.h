#ifndef EMERALD_H
#define EMERALD_H
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <decoration.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xregion.h>
#include <X11/cursorfont.h>
#include <X11/extensions/Xrender.h>

/*
#ifndef GDK_DISABLE_DEPRECATED
#define GDK_DISABLE_DEPRECATED
#endif

#ifndef GTK_DISABLE_DEPRECATED
#define GTK_DISABLE_DEPRECATED
#endif
*/

#include <gtk/gtk.h>
#include <gtk/gtkwindow.h>
#include <gdk/gdkx.h>

#define IS_VALID_SURFACE(o) (o && cairo_surface_get_reference_count(o) > 0)

#ifdef USE_DBUS
#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#endif

#define WNCK_I_KNOW_THIS_IS_UNSTABLE
#include <libwnck/libwnck.h>

#include <cairo.h>
#include <cairo-xlib.h>

#if CAIRO_VERSION < CAIRO_VERSION_ENCODE(1, 1, 0)
#define CAIRO_EXTEND_PAD CAIRO_EXTEND_NONE
#endif

#include <pango/pango-context.h>
#include <pango/pangocairo.h>

#include <dlfcn.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include<libintl.h>
#include<locale.h>
#define _(String) gettext (String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)

#define FAKE_WINDOW_ACTION_HELP (1 << 20)

#define WM_MOVERESIZE_SIZE_TOPLEFT      0
#define WM_MOVERESIZE_SIZE_TOP          1
#define WM_MOVERESIZE_SIZE_TOPRIGHT     2
#define WM_MOVERESIZE_SIZE_RIGHT        3
#define WM_MOVERESIZE_SIZE_BOTTOMRIGHT  4
#define WM_MOVERESIZE_SIZE_BOTTOM       5
#define WM_MOVERESIZE_SIZE_BOTTOMLEFT   6
#define WM_MOVERESIZE_SIZE_LEFT         7
#define WM_MOVERESIZE_MOVE              8
#define WM_MOVERESIZE_SIZE_KEYBOARD     9
#define WM_MOVERESIZE_MOVE_KEYBOARD    10

#define SHADOW_RADIUS      8.0
#define SHADOW_OPACITY     0.5
#define SHADOW_COLOR_RED   0x0000
#define SHADOW_COLOR_GREEN 0x0000
#define SHADOW_COLOR_BLUE  0x0000
#define SHADOW_OFFSET_X    1
#define SHADOW_OFFSET_Y    1

#define MWM_HINTS_DECORATIONS (1L << 1)

#define MWM_DECOR_ALL      (1L << 0)
#define MWM_DECOR_BORDER   (1L << 1)
#define MWM_DECOR_HANDLE   (1L << 2)
#define MWM_DECOR_TITLE    (1L << 3)
#define MWM_DECOR_MENU     (1L << 4)
#define MWM_DECOR_MINIMIZE (1L << 5)
#define MWM_DECOR_MAXIMIZE (1L << 6)

#define BLUR_TYPE_NONE     0
#define BLUR_TYPE_TITLEBAR 1
#define BLUR_TYPE_ALL      2

#define PROP_MOTIF_WM_HINT_ELEMENTS 3

typedef struct {
    unsigned long flags;
    unsigned long functions;
    unsigned long decorations;
} MwmHints;

//static double decoration_alpha = 0.5; //Decoration Alpha

#define SWITCHER_SPACE     40
#define SWITCHER_TOP_EXTRA 4

typedef struct _decor_color {
    double r;
    double g;
    double b;
} decor_color_t;


#include <titlebar.h>

typedef void (*event_callback) (WnckWindow *win, XEvent *event);

#define ACOLOR(idn,zr,zg,zb,za) \
    pfs->idn.color.r = (zr);\
    pfs->idn.color.g = (zg);\
    pfs->idn.color.b = (zb);\
    pfs->idn.alpha   = (za);

#define CCOLOR(idn,zc) \
    pfs->idn.color.r = (pfs->color_contrast * pfs->zc.color.r);\
    pfs->idn.color.g = (pfs->color_contrast * pfs->zc.color.g);\
    pfs->idn.color.b = (pfs->color_contrast * pfs->zc.color.b);\
    pfs->idn.alpha   = (pfs->alpha_contrast * pfs->zc.alpha);

typedef struct _alpha_color
{
    decor_color_t color;
    double alpha;
} alpha_color;

typedef struct _pos_t {
    int x, y, w, h;
    int xw, yh, ww, hh;
} pos_t;

typedef struct _frame_settings frame_settings;

typedef struct _window_settings
{
    void * engine_ws;
    gint button_offset;
    gint button_hoffset;
    gchar * tobj_layout;

    gint double_click_action;
    gint button_hover_cursor;
    gboolean titlebar_no_scroll_shade;

    gboolean round_top_left;
    gboolean round_top_right;
    gboolean round_bottom_left;
    gboolean round_bottom_right;

    frame_settings * fs_act;
    frame_settings * fs_inact;
    gint min_titlebar_height;
    gboolean use_pixmap_buttons;// = FALSE;
    double	corner_radius;//	=	5.0;
    PangoAlignment title_text_align;// = PANGO_ALIGN_CENTER;
    GdkPixbuf * ButtonPix[S_COUNT*B_COUNT];
    GdkPixbuf * ButtonArray[B_COUNT];

    gboolean    use_button_glow;
    gboolean    use_button_inactive_glow;
    gboolean    use_decoration_cropping;
    gboolean    use_button_fade;
    GdkPixbuf * ButtonGlowPix[B_COUNT];
    GdkPixbuf * ButtonGlowArray;
    GdkPixbuf * ButtonInactiveGlowArray;
    GdkPixbuf * ButtonInactiveGlowPix[B_COUNT];
    int         button_fade_num_steps;        // number of steps
    int         button_fade_step_duration;    // step duration in milliseconds
    int         button_fade_pulse_len_steps;  // length of pulse (number of steps)
    int         button_fade_pulse_wait_steps; // how much pulse waits before fade out
    /* = {
    { 0, 6, 16, 16,   1, 0, 0, 0 },
    { 0, 6, 16, 16,   1, 0, 0, 0 },
    { 0, 6, 16, 16,   1, 0, 0, 0 },
    { 0, 6, 16, 16,   1, 0, 0, 0 },
    };*/
    gdouble shadow_radius;
    gdouble shadow_opacity;
    gint    shadow_color[3];
    gint    shadow_offset_x;
    gint    shadow_offset_y;
    decor_extents_t shadow_extents;//   = { 0, 0, 0, 0 };
    decor_extents_t win_extents;//      = { 6, 6, 4, 6 };
    pos_t pos[3][3];
    gint left_space;//   = 6;
    gint right_space;//  = 6;
    gint top_space;//    = 4;
    gint bottom_space;// = 6;

    gint left_corner_space;//   = 0;
    gint right_corner_space;//  = 0;
    gint top_corner_space;//    = 0;
    gint bottom_corner_space;// = 0;

    gint titlebar_height;// = 17; //Titlebar Height

    gint normal_top_corner_space;//      = 0;

    gint shadow_left_space;//   = 0;
    gint shadow_right_space;//  = 0;
    gint shadow_top_space;//    = 0;
    gint shadow_bottom_space;// = 0;

    gint shadow_left_corner_space;//   = 0;
    gint shadow_right_corner_space;//  = 0;
    gint shadow_top_corner_space;//    = 0;
    gint shadow_bottom_corner_space;// = 0;


    cairo_surface_t *shadow_surface;// = NULL;
    cairo_surface_t *large_shadow_surface;// = NULL;
    cairo_surface_t *decor_normal_surface;// = NULL;
    cairo_surface_t *decor_active_surface;// = NULL;

    cairo_pattern_t *shadow_pattern;// = NULL;

    gint	    text_height;

    PangoFontDescription *font_desc;
    PangoContext * pango_context;

    decor_extents_t switcher_extents;// = { 0, 0, 0, 0 };
    gint switcher_top_corner_space;//    = 0;
    gint switcher_bottom_corner_space;// = 0;

    struct _icon_size {
        int w, h;
    } c_icon_size[B_T_COUNT],
      c_glow_size; // one glow size for all buttons
                   // (buttons will be centered in their glows)
                   // active and inactive glow pixmaps are assumed to be of same size
    gboolean stretch_sides;
    gint blur_type;// = BLUR_TYPE_NONE;

} window_settings;

struct _frame_settings
{
    void * engine_fs;
    window_settings *ws;
    alpha_color button;
    alpha_color button_halo;
    alpha_color text;
    alpha_color text_halo;
};

typedef struct _rectangle {
    gint        x1, y1, x2, y2;
} rectangle_t;

typedef struct _button_fade_info
{
    gpointer * d; // needed by the timer function
    cairo_t * cr;
    double    y1;
    int  counters[B_T_COUNT]; // 0: not fading, > 0: fading in, < 0: fading out
                              // max value:  ws->button_fade_num_steps+1 (1 is reserved to indicate
                              //                                          fade-in initiation)
                              // min value: -ws->button_fade_num_steps
    gboolean pulsating[B_T_COUNT];
    gint    timer;
    gboolean first_draw;
} button_fade_info_t;

typedef struct _button_region_t {
    gint        base_x1, base_y1, base_x2, base_y2; // button coords with no glow
    gint        glow_x1, glow_y1, glow_x2, glow_y2; // glow coordinates

    // holds whether this button's glow overlap with the other button's non-glow (base) area
    gboolean    overlap_buttons[B_T_COUNT];
    cairo_surface_t *bg_surface;
} button_region_t;

typedef struct _decor
{
    Window	      event_windows[3][3];
    Window	      button_windows[B_T_COUNT];
    guint	      button_states[B_T_COUNT];
    gint tobj_pos[3];
    gint tobj_size[3];
    gint tobj_item_pos[TBT_COUNT];
    gint tobj_item_state[TBT_COUNT];
    gint tobj_item_width[TBT_COUNT];
    cairo_surface_t   *surface;
    cairo_surface_t   *buffer_surface;
    gint	      width;
    gint	      height;
    gint              client_width;
    gint              client_height;
    gboolean	      decorated;
    gboolean	      active;
    PangoLayout	      *layout;
    gchar	      *name;
    cairo_pattern_t   *icon;
    cairo_surface_t   *icon_surface;
    GdkPixbuf	      *icon_pixbuf;
    WnckWindowState   state;
    WnckWindowActions actions;
    XID		      prop_xid;
    GtkWidget	      *force_quit_dialog;
    frame_settings * fs;
    void	      (*draw) (struct _decor *d);
    button_region_t   button_region[B_T_COUNT];
    rectangle_t       min_drawn_buttons_region; // minimal rectangle enclosing all drawn regions
    gboolean          draw_only_buttons_region;
    gint              button_last_drawn_state[B_T_COUNT]; // last drawn state or fade counter
    button_fade_info_t button_fade_info;
    cairo_surface_t   *p_active_surface, *p_active_buffer_surface;
    cairo_surface_t   *p_inactive_surface, *p_inactive_buffer_surface;
    button_region_t   button_region_inact[B_T_COUNT];
    gboolean only_change_active;
} decor_t;

#define LFACSS(zc,sec) \
    load_color_setting(f,&ws->fs_act->zc.color,"active_" #zc , #sec);\
    load_color_setting(f,&ws->fs_inact->zc.color,"inactive_" #zc , #sec);\
    load_float_setting(f,&ws->fs_act->zc.alpha,"active_" #zc "_alpha", #sec);\
    load_float_setting(f,&ws->fs_inact->zc.alpha,"inactive_" #zc "_alpha", #sec);

#define SHADE_LEFT   (1 << 0)
#define SHADE_RIGHT  (1 << 1)
#define SHADE_TOP    (1 << 2)
#define SHADE_BOTTOM (1 << 3)

#define CORNER_TOPLEFT     (1 << 0)
#define CORNER_TOPRIGHT    (1 << 1)
#define CORNER_BOTTOMRIGHT (1 << 2)
#define CORNER_BOTTOMLEFT  (1 << 3)

#endif
