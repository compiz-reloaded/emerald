#ifndef EMERALD_ENGINE_H
#define EMERALD_ENGINE_H
#include <emerald.h>
#include <libengine.h>
/* init_engine */
typedef void (*init_engine_proc)(window_settings *);
/* load_engine_settings */
typedef void (*load_settings_proc)(GKeyFile * f, window_settings * ws);
/* fini_engine */
typedef void (*fini_engine_proc)(window_settings *);
/* engine_draw_frame */
typedef void (*draw_frame_proc)(decor_t * d, cairo_t * cr);
/* layout_engine_settings */
typedef void (*layout_settings_proc)(GtkWidget * vbox);
/* get meta info */
typedef void (*get_meta_info_proc)(EngineMetaInfo * d);
#endif
