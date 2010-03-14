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

////////////////////////////////////////////////////
//themer stuff
#include <engine.h>
#include <signal.h>
typedef enum _EngineCol
{
    ENGINE_COL_DLNAME,
    ENGINE_COL_NAME,
    ENGINE_COL_VER,
    ENGINE_COL_LAST_COMPAT,
    ENGINE_COL_MARKUP,
    ENGINE_COL_ICON,
    ENGINE_COL_COUNT
} EngineCol;
typedef struct _EngineData
{
    const gchar * canname;
    gchar * dlname;
    GtkWidget * vbox;
    EngineMetaInfo meta;
} EngineData;
typedef struct _FindEngine
{
    const gchar * canname;
    gboolean found;
    gint i;
    EngineData * d;
} FindEngine;
GSList * SettingList = NULL;
GSList * EngineList = NULL;
GtkWidget * EngineCombo;
GtkListStore * EngineModel;
GtkWidget * EngineContainer;
//GtkWidget * PreviewImage[BX_COUNT];
//GtkWidget * ButtonImage[BX_COUNT];
gboolean apply=FALSE;
gboolean changed=FALSE;
GKeyFile * global_theme_file;
GKeyFile * global_settings_file;
#ifdef USE_DBUS
DBusConnection *dbcon;
#endif
gchar * active_engine = NULL;

static gchar* display_part(const gchar *p)
{
  gchar *name = g_strdup(p);
  gchar *tmp;

  if ((tmp = g_strrstr(name,":"))) {
    *tmp++ = 0;
    tmp = g_strdup(tmp);
    g_free(name);
    name = tmp;
  }
    
  if ((tmp = g_strrstr(name,"."))) {
    *tmp = 0;
  }
    
  return name;
}

GSList * get_setting_list()
{
    return SettingList;
}
GtkWidget * scaler_new(gdouble low, gdouble high, gdouble prec)
{
    GtkWidget * w;
    w = gtk_hscale_new_with_range(low,high,prec);
    gtk_scale_set_value_pos(GTK_SCALE(w),GTK_POS_RIGHT);
    gtk_range_set_update_policy(GTK_RANGE(w),GTK_UPDATE_DISCONTINUOUS);
    gtk_widget_set_size_request(w,100,-1);
    return w;
}
void add_color_alpha_value(gchar * caption, gchar * basekey, gchar * sect, gboolean active) 
{
    GtkWidget * w;
    gchar * colorkey;
    gchar * alphakey;
    colorkey = g_strdup_printf(active?"active_%s":"inactive_%s",basekey);
    alphakey = g_strdup_printf(active?"active_%s_alpha":"inactive_%s_alpha",
            basekey);
    
    w = gtk_label_new(caption);
    table_append(w,FALSE);

    w = gtk_color_button_new();
    table_append(w,FALSE);
    register_setting(w,ST_COLOR,sect,colorkey);

    w = scaler_new(0.0,1.0,0.01);
    table_append(w,TRUE);
    register_setting(w,ST_FLOAT,sect,alphakey);
    //we don't g_free because they are registered with register_setting
}
void make_labels(gchar * header)
{
    table_append(gtk_label_new(header),FALSE);
    table_append(gtk_label_new("Color"),FALSE);
    table_append(gtk_label_new("Opacity"),FALSE);
}
GtkWidget * build_frame(GtkWidget * vbox, gchar * title, gboolean is_hbox)
{
    GtkWidget * frame;
    GtkWidget * box;
    frame = gtk_frame_new(title);
    gtk_box_pack_startC(vbox,frame,TRUE,TRUE,0);
    box = is_hbox?gtk_hbox_new(FALSE,2):gtk_vbox_new(FALSE, 2);
    gtk_container_set_border_widthC(box,8);
    gtk_container_addC(frame,box);
    return box;
}
SettingItem * register_img_file_setting(GtkWidget * widget, gchar * section, gchar * key, GtkImage * image)
{
    SettingItem * item = register_setting(widget,ST_IMG_FILE,section,key);
    gtk_file_chooser_button_set_width_chars(GTK_FILE_CHOOSER_BUTTON(widget),0);
    item->image = image;
    item->preview = GTK_IMAGE(gtk_image_new());
    gtk_file_chooser_set_preview_widget(GTK_FILE_CHOOSER(widget),GTK_WIDGET(item->preview));
    g_signal_connect(widget,"update-preview",G_CALLBACK(update_preview_cb),
            item->preview);
    return item;
}
SettingItem * register_setting(GtkWidget * widget, SettingType type, gchar * section, gchar * key)
{
    SettingItem * item;
    item = malloc(sizeof(SettingItem));
    item->type = type;
    item->key = g_strdup(key);
    item->section = g_strdup(section);
    item->widget = widget;
    item->fvalue = g_strdup("");
    SettingList = g_slist_append(SettingList,item);
    switch(item->type)
    {
        case ST_BOOL:
        case ST_SFILE_BOOL:
            g_signal_connect(widget,"toggled",
                    G_CALLBACK(cb_apply_setting),
                    item);
            break;
        case ST_INT:
        case ST_SFILE_INT:
            g_signal_connect(widget,"value-changed",
                    G_CALLBACK(cb_apply_setting),
                    item);
            break;
        case ST_FLOAT:
            g_signal_connect(widget,"value-changed",
                    G_CALLBACK(cb_apply_setting),
                    item);
            break;
        case ST_COLOR:
            g_signal_connect(widget,"color-set",
                    G_CALLBACK(cb_apply_setting),
                    item);
            break;
        case ST_FONT:
            g_signal_connect(widget,"font-set",
                    G_CALLBACK(cb_apply_setting),
                    item);
            break;
        case ST_IMG_FILE:
            g_signal_connect(widget,"selection-changed",
                    G_CALLBACK(cb_apply_setting),
                    item);
            break;
        case ST_STRING_COMBO:
            g_signal_connect(gtk_bin_get_child(GTK_BIN(widget)),"changed",
                    G_CALLBACK(cb_apply_setting),
                    item);
            break;
        case ST_SFILE_INT_COMBO:
            g_signal_connect(widget,"changed",
                    G_CALLBACK(cb_apply_setting),
                    item);
            break;
        case ST_ENGINE_COMBO:
            g_signal_connect(widget,"changed",
                    G_CALLBACK(cb_apply_setting),
                    item);
        default:
            break;
            //unconnected types
    }
    return item;
}

static gint current_table_width;
static GtkTable * current_table;
static gint current_table_col;
static gint current_table_row;

void table_new(gint width, gboolean same, gboolean labels)
{
    //WARNING - clobbers all the current_table_ vars.
    current_table = GTK_TABLE(gtk_table_new(width,1,same));
    gtk_table_set_row_spacings(current_table,8);
    gtk_table_set_col_spacings(current_table,8);
    current_table_col = labels?1:0;
    current_table_row = 0;
    current_table_width = width;
}
void table_append(GtkWidget * child,gboolean stretch)
{
    gtk_table_attach(current_table,child,current_table_col,current_table_col+1,
            current_table_row,current_table_row+1,
            (stretch?GTK_EXPAND:GTK_SHRINK)|GTK_FILL,
            (stretch?GTK_EXPAND:GTK_SHRINK)|GTK_FILL,
            0,0);
    current_table_col++;
    if (current_table_col == current_table_width)
    {
        current_table_col=0;
        current_table_row++;
//        gtk_table_resize(current_table,current_table_width,current_table_row+1);
    }
}
void table_append_separator()
{
    current_table_col=0;
    current_table_row++;
//    gtk_table_resize(current_table,current_table_width,current_table_row+1);
    gtk_table_attach_defaults(current_table,
            gtk_hseparator_new(),
            0,current_table_width,
            current_table_row,
            current_table_row+1);
    current_table_row++;
//    gtk_table_resize(current_table,current_table_width,current_table_row+1);
}
GtkTable * get_current_table()
{
    return current_table;
}
void send_reload_signal()
{
#ifdef USE_DBUS
    DBusMessage *message;
    message = dbus_message_new_signal("/","org.metascape.emerald.dbus.Signal","Reload");
    dbus_connection_send(dbcon,message,NULL);
    dbus_message_unref(message);
#else
    Atom wmAtom = 0;
    Display *dpy = GDK_DISPLAY_XDISPLAY (gdk_display_get_default ());

    char buffer[128];
    char *part = display_part(getenv("DISPLAY"));

    sprintf(buffer, "_COMPIZ_DM_S%s", part);
    free(part);

    if (dpy)
        wmAtom = XInternAtom(dpy,buffer,0);

    if (wmAtom) {
        XEvent clientEvent;
	Status missed;
        Window w = XGetSelectionOwner(dpy,wmAtom);
	Atom ReloadIt = XInternAtom(dpy, "emerald-sigusr1", 0);
	clientEvent.xclient.type = ClientMessage;
	clientEvent.xclient.window = w;
	clientEvent.xclient.message_type = ReloadIt;
	clientEvent.xclient.format = 32;
	clientEvent.xclient.display = dpy;
	clientEvent.xclient.data.l[0]    = 0;
	clientEvent.xclient.data.l[1]    = 0;
	clientEvent.xclient.data.l[2]    = 0;
	clientEvent.xclient.data.l[3]    = 0;
	clientEvent.xclient.data.l[4]    = 0;
	missed = XSendEvent(dpy,w, 
			    False,
			    NoEventMask, 
			    &clientEvent);
	XSync (dpy, False);
    } else {
        /* The old way */
        gchar * args[]=
	    {"killall","-u",(gchar *)g_get_user_name(),"-SIGUSR1","emerald",NULL};
	gchar * ret=NULL;
	if (!g_spawn_sync(NULL,args,NULL,G_SPAWN_STDERR_TO_DEV_NULL | G_SPAWN_SEARCH_PATH,
			  NULL,NULL,&ret,NULL,NULL,NULL) || !ret)
	    g_warning("Couldn't find running emerald, no reload signal sent.");
    }
#endif
}
void apply_settings()
{
    gchar * file = g_strjoin("/",g_get_home_dir(),".emerald/theme/theme.ini",NULL);
    gchar * path = g_strjoin("/",g_get_home_dir(),".emerald/theme/",NULL);
    gchar * at;
    g_slist_foreach(SettingList,(GFunc) write_setting,global_theme_file);
    g_key_file_set_string(global_theme_file,"theme","version",VERSION);
    g_mkdir_with_parents(path,00755);
    at = g_key_file_to_data(global_theme_file,NULL,NULL);
    if (at)
    {
        g_file_set_contents(file,at,-1,NULL);
        g_free(at);
    }
    g_free(file);
    g_free(path);
    send_reload_signal();
}
void cb_apply_setting(GtkWidget * w, gpointer p)
{
    SettingItem * item = p;
    if (item->type == ST_IMG_FILE)
    {
        gchar * s;
        if (!(s=gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(item->widget))))
            return; // for now just ignore setting it to an invalid name
        if (!strcmp(s,item->fvalue))
        {
            g_free(s);
            return;
        }
        g_free(item->fvalue);
        item->fvalue=s;
        check_file(item,s);
    }
    write_setting(p,(gpointer) global_theme_file);
    if (apply)
        apply_settings();
    else
    {
        changed=TRUE;
    }
}
#ifdef USE_DBUS
void setup_dbus()
{
    dbcon = dbus_bus_get (DBUS_BUS_SESSION,NULL);
    dbus_connection_setup_with_g_main(dbcon,NULL);    
}
#endif
void write_setting(SettingItem * item, gpointer p)
{
    GKeyFile * f = (GKeyFile *) p;
    switch(item->type)
    {
        case ST_BOOL:
            g_key_file_set_boolean(f,item->section,item->key,get_bool(item));
            break;
        case ST_INT:
            g_key_file_set_integer(f,item->section,item->key,get_int(item));
            break;
        case ST_FLOAT:
            g_key_file_set_string(f,item->section,item->key,get_float_str(item));
            break;
        case ST_COLOR:
            g_key_file_set_string(f,item->section,item->key,get_color(item));
            break;
        case ST_FONT:
            g_key_file_set_string(f,item->section,item->key,get_font(item));
            break;
        case ST_META_STRING:
            g_key_file_set_string(f,item->section,item->key,get_string(item));
            break;
        case ST_STRING_COMBO:
            g_key_file_set_string(f,item->section,item->key,get_string_combo(item));
            break;
        case ST_IMG_FILE:
            //g_key_file_set_string(f,item->section,item->key,get_img_file(item));
            {
                gchar * s = g_strdup_printf("%s/.emerald/theme/%s.%s.png",g_get_home_dir(),item->section,item->key);
                GdkPixbuf * pbuf = gtk_image_get_pixbuf(item->image);
                if (pbuf)
                {
                    gdk_pixbuf_savev(pbuf,s,"png",NULL,NULL,NULL);
                }
                else
                {
                    g_unlink(s); // to really clear out a clear'd image
                }
                g_free(s);
            }
            break;
        case ST_ENGINE_COMBO:
            {
                EngineMetaInfo emi;
                const gchar * active_engine = get_engine_combo(item);
                if (get_engine_meta_info(active_engine,&emi))
                    g_key_file_set_string(f,"engine_version",active_engine,emi.version);
                g_key_file_set_string(f,item->section,item->key,active_engine);
                do_engine(active_engine);
            }
            break;
        case ST_SFILE_INT:
            if (f==global_theme_file)
            {
                g_key_file_set_integer(global_settings_file,item->section,
                        item->key,get_int(item));
                write_setting_file();
            }
            break;
        case ST_SFILE_BOOL:
            if (f==global_theme_file)
            {
                g_key_file_set_boolean(global_settings_file,item->section,
                        item->key,get_bool(item));
                write_setting_file();
            }
            break;
        case ST_SFILE_INT_COMBO:
            if (f==global_theme_file)
            {
                g_key_file_set_integer(global_settings_file,item->section,
                        item->key,get_sf_int_combo(item));
                write_setting_file();
            }
            break;
        default:
            break;
            //unhandled types
    }
}
void write_setting_file()
{
    gchar * file = g_strjoin("/",g_get_home_dir(),".emerald/settings.ini",NULL);
    gchar * path = g_strjoin("/",g_get_home_dir(),".emerald/",NULL);
    gchar * at;
    g_mkdir_with_parents(path,00755);
    at = g_key_file_to_data(global_settings_file,NULL,NULL);
    if (at)
    {
        g_file_set_contents(file,at,-1,NULL);
        g_free(at);
    }
    g_free(file);
    g_free(path);
}

gchar * globalStr = NULL;
gchar globalFloatStr[G_ASCII_DTOSTR_BUF_SIZE+1];

gboolean get_bool(SettingItem * item)
{
    return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(item->widget));
}
gdouble get_float(SettingItem * item)
{
    if(!strcmp(G_OBJECT_TYPE_NAME(item->widget),"GtkSpinButton")) {
         return gtk_spin_button_get_value((GtkSpinButton *)item->widget);
    }
    else {
        return gtk_range_get_value(GTK_RANGE(item->widget));
    }
}
gint get_int(SettingItem * item)
{
    return get_float(item);
}
const gchar * get_float_str(SettingItem * item)
{
    g_ascii_dtostr(globalFloatStr,G_ASCII_DTOSTR_BUF_SIZE,
            get_float(item));
    return globalFloatStr;
}
const gchar * get_color(SettingItem * item)
{
    GdkColor c;
    if (globalStr)
        g_free(globalStr);
    gtk_color_button_get_color(GTK_COLOR_BUTTON(item->widget),&c);
    globalStr = g_strdup_printf("#%02x%02x%02x",c.red>>8,c.green>>8,c.blue>>8);
    return globalStr;
}
const gchar * get_font(SettingItem * item)
{
    return gtk_font_button_get_font_name(GTK_FONT_BUTTON(item->widget));
}
const gchar * get_string(SettingItem * item)
{
    return gtk_entry_get_text(GTK_ENTRY(item->widget));
}
void check_file(SettingItem * item,gchar * f)
{
    GdkPixbuf * p;
    p = gdk_pixbuf_new_from_file(f,NULL);
    if(p)
    {
        gtk_image_set_from_pixbuf(item->image,p);
        gtk_image_set_from_pixbuf(item->preview,p);
    }
    else
    {
        gtk_image_clear(item->image);
        gtk_image_clear(item->preview);
    }
    if(p)
        g_object_unref(p);
}
const gchar * get_img_file(SettingItem * item)
{
    return item->fvalue;
}
const gchar * get_string_combo(SettingItem * item)
{
    const gchar * s;
    s= gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(item->widget))));
    if (strlen(s))
        return s;
    s="IT::HNXC:Default Layout (Blank Entry)";
    return s;
}
void show_engine_named(EngineData * d, gpointer p)
{
    gchar * nam = p;
    if (!strcmp(nam,d->canname))
    {
        gtk_container_add(GTK_CONTAINER(EngineContainer),d->vbox);
        gtk_widget_show_all(EngineContainer);
    }
}
void do_engine(const gchar * nam)
{
    GtkWidget * w;
    if (active_engine && !strcmp(active_engine,nam))
        return;
    if (active_engine)
        g_free(active_engine);
    active_engine = g_strdup(nam);
    if ((w=gtk_bin_get_child(GTK_BIN(EngineContainer))))
        gtk_container_remove(GTK_CONTAINER(EngineContainer),w);
    g_slist_foreach(EngineList,(GFunc) show_engine_named, (gpointer) nam);

}
void search_engine(EngineData * d, gpointer p)
{
    FindEngine * fe = p;
    if (!fe->found)
    {
        if (!strcmp(d->canname,fe->canname))
        {
            fe->d = d;
            fe->found=TRUE;
        }
        else
            fe->i++;
    }
}
gboolean get_engine_meta_info(const gchar * engine, EngineMetaInfo * inf)
{
    FindEngine fe;
    fe.canname = engine;
    fe.found = FALSE;
    fe.i=0;
    fe.d=NULL;
    g_slist_foreach(EngineList,(GFunc) search_engine, &fe);
    if (fe.found)
        memcpy(inf,&(fe.d->meta),sizeof(EngineMetaInfo));
    return fe.found;
}
void set_engine_combo(SettingItem * item, gchar * val)
{
    FindEngine fe;
    fe.canname = val;
    fe.found=FALSE;
    fe.i = 0;
    g_slist_foreach(EngineList,(GFunc) search_engine,&fe);
    if (fe.found)
    {
        gtk_combo_box_set_active(GTK_COMBO_BOX(item->widget),fe.i);
    }
    else
    {
        fe.canname = "legacy";
        fe.found=FALSE;
        fe.i=0;
        g_slist_foreach(EngineList,(GFunc) search_engine,&fe);
        if (fe.found)
            gtk_combo_box_set_active(GTK_COMBO_BOX(item->widget),fe.i);
    }
    do_engine(fe.canname);
}
const gchar * get_engine_combo(SettingItem * item)
{
    static gchar * s = NULL;
    GtkTreeIter i;
    if(s) g_free(s);
    //s = gtk_combo_box_get_active_text(GTK_COMBO_BOX(item->widget));
    if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX(item->widget),&i))
    {
        gtk_tree_model_get(GTK_TREE_MODEL(EngineModel),&i,ENGINE_COL_NAME,&s,-1);
        if (!strlen(s))
        {
            g_free(s);
            s=g_strdup("legacy");
        }
    }
    return s;
}
gint get_sf_int_combo(SettingItem * item)
{
    return gtk_combo_box_get_active(GTK_COMBO_BOX(item->widget));
}
void update_preview(GtkFileChooser * fc, gchar * filename, GtkImage * img)
{
    GdkPixbuf * pixbuf;
    gboolean have_preview;
    pixbuf = gdk_pixbuf_new_from_file(filename,NULL);
    have_preview = (pixbuf != NULL);
    gtk_image_set_from_pixbuf(GTK_IMAGE(img),pixbuf);
    if (pixbuf)
        g_object_unref(pixbuf);
    gtk_file_chooser_set_preview_widget_active(fc,have_preview);
}
void update_preview_cb(GtkFileChooser * file_chooser, gpointer data)
{
    gchar * filename;
    filename = gtk_file_chooser_get_preview_filename(file_chooser);
    update_preview(file_chooser,filename,GTK_IMAGE(data));
    g_free(filename);
}
void set_img_file(SettingItem * item,gchar * f)
{
    g_free(item->fvalue);
    item->fvalue = g_strdup(f);
    gtk_file_chooser_select_filename(GTK_FILE_CHOOSER(item->widget),f);
    check_file(item,f);
}
void set_bool(SettingItem * item, gboolean b)
{
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(item->widget),b);
}
void set_float(SettingItem * item, gdouble f)
{
    if(!strcmp(G_OBJECT_TYPE_NAME(item->widget),"GtkSpinButton")) {
         gtk_spin_button_set_value((GtkSpinButton *)item->widget, f);
    } 
    else {
         gtk_range_set_value(GTK_RANGE(item->widget),f);
    }
}
void set_int(SettingItem * item, gint i)
{
    set_float(item,i);
}
void set_float_str(SettingItem * item, gchar * s)
{
    set_float(item,g_ascii_strtod(s,NULL));
}
void set_color(SettingItem * item, gchar * s)
{
    GdkColor c;
    gdk_color_parse(s,&c);
    gtk_color_button_set_color(GTK_COLOR_BUTTON(item->widget),&c);
}
void set_font(SettingItem * item, gchar * f)
{
    gtk_font_button_set_font_name(GTK_FONT_BUTTON(item->widget),f);
}
void set_string(SettingItem * item, gchar * s)
{
    gtk_entry_set_text(GTK_ENTRY(item->widget),s);
}
void set_string_combo(SettingItem * item, gchar * s)
{
    gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(item->widget))),s);
}
void set_sf_int_combo(SettingItem * item, gint i)
{
    gtk_combo_box_set_active(GTK_COMBO_BOX(item->widget),i);
}
void read_setting(SettingItem * item, gpointer * p)
{
    GKeyFile * f = (GKeyFile *) p;
    GError * e = NULL;
    gboolean b;
    gint i;
    gchar * s;
    switch(item->type)
    {
        case ST_BOOL:
            b = g_key_file_get_boolean(f,item->section,item->key,&e);
            if (!e)
                set_bool(item,b);
            break;
        case ST_INT:
            i = g_key_file_get_integer(f,item->section,item->key,&e);
            if (!e)
                set_int(item,i);
            break;
        case ST_FLOAT:
            s = g_key_file_get_string(f,item->section,item->key,&e);
            if (!e && s)
            {
                set_float_str(item,s);
                g_free(s);
            }
            break;
        case ST_COLOR:
            s = g_key_file_get_string(f,item->section,item->key,&e);
            if (!e && s)
            {
                set_color(item,s);
                g_free(s);
            }
            break;
        case ST_FONT:
            s = g_key_file_get_string(f,item->section,item->key,&e);
            if (!e && s)
            {
                set_font(item,s);
                g_free(s);
            }
            break;
        case ST_META_STRING:
            s = g_key_file_get_string(f,item->section,item->key,&e);
            if (!e && s)
            {
                set_string(item,s);
                g_free(s);
            }
            break;
        case ST_STRING_COMBO:
            s = g_key_file_get_string(f,item->section,item->key,&e);
            if (!e && s)
            {
                set_string_combo(item,s);
                g_free(s);
            }
            break;
        case ST_IMG_FILE:
            /*s = g_key_file_get_string(f,item->section,item->key,&e);
            if (!e && s)
            {
                set_img_file(item,s);
                g_free(s);
            }*/
            s = g_strdup_printf("%s/.emerald/theme/%s.%s.png",g_get_home_dir(),item->section,item->key);
            set_img_file(item,s);
            g_free(s);
            break;
        case ST_ENGINE_COMBO:
            s = g_key_file_get_string(f,item->section,item->key,&e);
            if (!e && s)
            {
                set_engine_combo(item,s);
                g_free(s);
            }
            break;
        case ST_SFILE_INT:
            if (f==global_theme_file)
            {
                i = g_key_file_get_integer(global_settings_file,
                        item->section,item->key,&e);
                if (!e)
                    set_int(item,i);
            }
            break;
        case ST_SFILE_BOOL:
            if (f==global_theme_file)
            {
                b = g_key_file_get_boolean(global_settings_file,
                        item->section,item->key,&e);
                if (!e)
                    set_bool(item,b);
            }
            break;
        case ST_SFILE_INT_COMBO:
            if (f==global_theme_file)
            {
                i = g_key_file_get_integer(global_settings_file,
                        item->section,item->key,&e);
                if (!e)
                    set_sf_int_combo(item,i);
            }
            break;
        default:
            break;
            //unhandled types
    }
}
void init_settings()
{
    gchar * file = g_strjoin("/",g_get_home_dir(),".emerald/theme/theme.ini",NULL);
    g_key_file_load_from_file(global_theme_file,file,G_KEY_FILE_KEEP_COMMENTS,NULL);
    g_free(file);
    file = g_strjoin("/",g_get_home_dir(),".emerald/settings.ini",NULL);
    g_key_file_load_from_file(global_settings_file,file,G_KEY_FILE_KEEP_COMMENTS,NULL);
    g_free(file);
    g_slist_foreach(SettingList,(GFunc) read_setting,global_theme_file);
}

void set_changed(gboolean schanged)
{
    changed=schanged;
}
void set_apply(gboolean sapply)
{
    apply=sapply;
}
void cb_clear_file(GtkWidget * button, gpointer p)
{
    SettingItem * item = p;
    check_file(item,"");
    item->fvalue="";
    gtk_file_chooser_unselect_all(GTK_FILE_CHOOSER(item->widget));
    write_setting(p,global_theme_file);
    if (apply) apply_settings();
}
void init_key_files()
{
    global_theme_file = g_key_file_new();
    global_settings_file = g_key_file_new();
}
void layout_engine_list(GtkWidget * vbox)
{
    GtkWidget * hbox;
    EngineCombo = gtk_combo_box_new();
    hbox = gtk_hbox_new(FALSE,2);
    gtk_box_pack_startC(vbox,hbox,FALSE,FALSE,0);
    gtk_box_pack_startC(hbox,gtk_label_new(_("Select\nEngine")),FALSE,FALSE,0);
    gtk_box_pack_startC(hbox,EngineCombo,FALSE,FALSE,0);
    gtk_box_pack_startC(vbox,gtk_hseparator_new(),FALSE,FALSE,0);
    EngineContainer = gtk_alignment_new(0,0,1,1); // really only needed for the bin-ness
    gtk_box_pack_startC(vbox,EngineContainer,TRUE,TRUE,0);
}
static gchar * canonize_name(gchar * dlname)
{
    gchar * end;
    gchar * begin = g_strrstr(dlname,"/lib");
    if (!begin)
        return g_strdup("");
    begin+=4;
    begin = g_strdup(begin);
    end = g_strrstr(begin,".so");
    end[0]='\0';
    return begin;
}
static void engine_comp(EngineData * d,gpointer p)
{
    FindEngine * e = p;
    if (!strcmp(e->canname, d->canname))
        e->found=TRUE;
}
static gboolean engine_is_unique (gchar * canname)
{
    FindEngine e;
    e.canname=canname;
    e.found=FALSE;
    g_slist_foreach(EngineList,(GFunc)engine_comp,&e);
    return !e.found;
}
static void append_engine(gchar * dlname)
{
    gchar * can;
    gchar * err;
    (void) dlerror();
    void * hand = dlopen(dlname,RTLD_NOW);
    err = dlerror();
    if (!hand || err)
    {
        g_warning("%s", err);
        if (hand)
            dlclose(hand);
        return;
    }
    can = canonize_name(dlname);
    if (engine_is_unique(can))
    {
        layout_settings_proc lay;
        lay = dlsym(hand,"layout_engine_settings");
        if ((err=dlerror()))
            g_warning("%s", err);
        if (lay)
        {
            get_meta_info_proc meta;
            EngineData * d = malloc(sizeof(EngineData));
            GtkTreeIter i;
            const gchar * format =
                "<b>%s</b> (%s)\n"
                "<i><small>%s</small></i>";
            meta = dlsym(hand,"get_meta_info");
            if ((err=dlerror()))
                g_warning("%s", err);
                d->meta.description=g_strdup("No Description");
                d->meta.version=g_strdup("0.0");
                d->meta.last_compat=g_strdup("0.0");
                d->meta.icon=gtk_widget_render_icon(EngineCombo,GTK_STOCK_MISSING_IMAGE,
                        GTK_ICON_SIZE_LARGE_TOOLBAR,"themeengine");
            if (meta)
                meta(&(d->meta));
            else
                g_warning("Engine %s has no meta info, please update it, using defaults.",dlname);

            d->dlname = dlname;
            d->canname = can;
            d->vbox = gtk_vbox_new(FALSE,2);
            g_object_ref(d->vbox);
            lay(d->vbox);
            EngineList = g_slist_append(EngineList,d);
            gtk_list_store_append(EngineModel,&i);

            gtk_list_store_set(EngineModel,&i,ENGINE_COL_DLNAME,d->dlname,ENGINE_COL_NAME,d->canname,
                    ENGINE_COL_VER,d->meta.version,ENGINE_COL_LAST_COMPAT,d->meta.last_compat,
                    ENGINE_COL_ICON,d->meta.icon,ENGINE_COL_MARKUP,
                    g_markup_printf_escaped(format,d->canname,d->meta.version,d->meta.description),
                    -1);
            //gtk_combo_box_prepend_text(GTK_COMBO_BOX(EngineCombo),d->canname);
        }
    }
    dlclose(hand);
}
static void engine_scan_dir(gchar * dir)
{
    GDir * d;
    d = g_dir_open(dir,0,NULL);
    if (d)
    {
        gchar * n;
        GPatternSpec * ps;
        ps = g_pattern_spec_new("lib*.so");
        while ((n = (gchar *) g_dir_read_name(d)))
        {
            if (g_pattern_match_string(ps,n))
            {
                gchar * dln = g_strjoin("/",dir,n,NULL);
                append_engine(dln);
            }
        }
        g_pattern_spec_free(ps);
        g_dir_close(d);
    }
}
void init_engine_list()
{
    //presumes the container & combo are created
    //presumes the combo is NOT registered
    GtkCellRenderer * r;
    
    EngineModel = gtk_list_store_new(ENGINE_COL_COUNT,G_TYPE_STRING,
            G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,GDK_TYPE_PIXBUF);
    gchar * local_engine_dir = g_strjoin("/",g_get_home_dir(),".emerald/engines",NULL);
    gtk_combo_box_set_model(GTK_COMBO_BOX(EngineCombo),GTK_TREE_MODEL(EngineModel));
    r = gtk_cell_renderer_pixbuf_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(EngineCombo),r,FALSE);
    gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(EngineCombo),r,"pixbuf",ENGINE_COL_ICON);
    r = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(EngineCombo),r,TRUE);
    gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(EngineCombo),r,"markup",ENGINE_COL_MARKUP);
    engine_scan_dir(local_engine_dir);
    g_free(local_engine_dir);
    engine_scan_dir(ENGINE_DIR);
    
    register_setting(EngineCombo,ST_ENGINE_COMBO,"engine","engine");
}
GtkWidget * build_notebook_page(gchar * title, GtkWidget * notebook)
{
    GtkWidget * vbox;
    vbox = gtk_vbox_new(FALSE,2);
    gtk_container_set_border_widthC(vbox,8);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook),vbox,
            gtk_label_new(title));
    return vbox;
}

