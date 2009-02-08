#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#define NEED_BUTTON_FILE_NAMES
#define NEED_BUTTON_NAMES
#define NEED_TITLEBAR_ACTION_NAMES
#include <emerald.h>
#include <engine.h>

#define LAST_COMPAT_VER "0.1.0"

typedef struct _FetcherInfo
{
    GtkWidget * dialog;
    GtkWidget * progbar;
    GPid        pd;
} FetcherInfo;

GtkWidget * ThemeSelector;
GtkListStore * ThemeList;
GtkCellRenderer * ThemeRenderer;
GtkCellRenderer * MetaRenderer;
GtkTreeViewColumn * ThemeColumn;
GtkTreeViewColumn * MetaColumn;
GtkTreeSelection * ThemeSelect;
GtkWidget * Cheating;
GtkWidget * Cheating2;
GtkWidget * mainWindow;
GtkWidget * EntryBox;
GtkWidget * Version;
GtkWidget * ThemeEnable;
GtkWidget * ReloadButton;
GtkWidget * DeleteButton;
GtkWidget * ImportButton;
GtkWidget * FetchButton;
GtkWidget * FetchButton2;
GtkWidget * ExportButton;
GtkWidget * QuitButton;
gchar * svnpath;
gchar * themecache;

static void theme_list_append(gchar * value,gchar * dir, gchar * fil)
{
    GtkTreeIter iter;
    gchar * path = g_strjoin("/",dir,fil,"theme.ini",NULL);
    gchar * imgpath = g_strjoin("/",dir,fil,"theme.screenshot.png",NULL);
    gchar * val;
    gchar * val2;
    GdkPixbuf * p;
    EngineMetaInfo emi;
    GKeyFile * f = g_key_file_new();
    g_key_file_load_from_file(f,path,G_KEY_FILE_NONE,NULL);
    gtk_list_store_append(ThemeList,&iter);
    gtk_list_store_set(ThemeList,&iter,0,value,-1);
    val = g_key_file_get_string(f,"engine","engine",NULL);
    gtk_list_store_set(ThemeList,&iter,6,val?val:"",-1);
    if (val) 
    {
        gchar * tver;
        gchar * ostr1;
        gchar * ostr2;
        gchar * ostr;
        gchar * elc;
        get_engine_meta_info(val,&emi);
        val2 = g_key_file_get_string(f,"engine_version",val,NULL);
        if (!val2)
            val2 = g_strdup("0.0.0");
        tver = g_key_file_get_string(f,"theme","version",NULL);
        if (!tver)
            tver = g_strup("0.0.0");
        elc=emi.last_compat;
        if (!emi.last_compat)
            elc="0.0.0";
        ostr1 = g_strdup_printf(strverscmp(val2,elc)>=0?
                "Engine: YES (%s)\n":"Engine: NO (%s)\n",val2);
        ostr2 = g_strdup_printf(strverscmp(tver,LAST_COMPAT_VER)>=0?
                "Emerald: YES (%s)":"Emerald: NO (%s)",tver);
        ostr = g_strdup_printf("%s%s",ostr1,ostr2);
        g_free(ostr1);
        g_free(ostr2);
        gtk_list_store_set(ThemeList,&iter,1,ostr,-1);
        g_free(ostr);
        g_free(tver);
        g_free(val2);
    }
    else
    {
        val = g_key_file_get_string(f,"theme","version",NULL);
        if (!val)
            val=g_strdup("0.0.0");
        val2 = g_strdup_printf(strverscmp(val,LAST_COMPAT_VER)>=0?
                "No Engine\nEmerald: YES (%s)":"No Engine\nEmerald: NO (%s)",val?val:"NONE");
        gtk_list_store_set(ThemeList,&iter,1,val2,-1);
        g_free(val2);
    }
    if (val) g_free(val);
    val = g_key_file_get_string(f,"theme","creator",NULL);
    gtk_list_store_set(ThemeList,&iter,2,val?val:"",-1);
    if (val) g_free(val);
    val = g_key_file_get_string(f,"theme","description",NULL);
    gtk_list_store_set(ThemeList,&iter,3,val?val:"",-1);
    if (val) g_free(val);
    val = g_key_file_get_string(f,"theme","theme_version",NULL);
    gtk_list_store_set(ThemeList,&iter,4,val?val:"",-1);
    if (val) g_free(val);
    val = g_key_file_get_string(f,"theme","suggested",NULL);
    gtk_list_store_set(ThemeList,&iter,5,val?val:"",-1);
    if (val) g_free(val);
    g_free(path);
    {
        //create the Theme column data
        gchar * format =
            _("<b><big>%s</big></b>\n"
            "<i>%s</i>\n"
            "<small>"
            "<b>Version</b> %s\n"
            "<b>Created By</b> %s\n"
            "<b>Use With</b> %s\n"
            "</small>");
        if (value[0]=='*')
        {
            value+=2;
            format=
                _("<b><big>%s</big></b> (System Theme)\n"
                "<i>%s</i>\n"
                "<small>"
                "<b>Version</b> %s\n"
                "<b>Created By</b> %s\n"
                "<b>Use With</b> %s\n"
                "</small>");
        }
        gchar * creator = g_key_file_get_string(f,"theme","creator",NULL);
        gchar * tver = g_key_file_get_string(f,"theme","theme_version",NULL);
        gchar * rwid = g_key_file_get_string(f,"theme","suggested",NULL);
        gchar * desc = g_key_file_get_string(f,"theme","description",NULL);
        if (creator && !strlen(creator)) 
        {
            g_free(creator);
            creator=NULL;
        }
        if (!creator) creator=g_strdup(_("Unknown"));
        if (tver && !strlen(tver)) 
        {
            g_free(tver);
            tver=NULL;
        }
        if (!tver) tver=g_strdup(_("Unknown"));
        if (rwid && !strlen(rwid)) 
        {
            g_free(rwid);
            rwid=NULL;
        }
        if (!rwid) rwid=g_strdup(_("Whatever (no hint)"));
        if (desc && !strlen(desc)) 
        {
            g_free(desc);
            desc=NULL;
        }
        if (!desc) desc=g_strdup(_("No Description"));
        val = g_markup_printf_escaped(format,value,desc,tver,creator,rwid);
        gtk_list_store_set(ThemeList,&iter,8,val,-1);
        g_free(val);
        g_free(tver);
        g_free(creator);
        g_free(rwid);
        g_free(desc);
    }
    g_key_file_free(f);
    p = gdk_pixbuf_new_from_file_at_size(imgpath,-1,100,NULL);
    if (p)
    {
        gtk_list_store_set(ThemeList,&iter,7,p,-1); // should own a ref to p
        g_object_unref(p);
    }
    else
    {
        //p = gdk_pixbuf_new(GDK_COLORSPACE_RGB,TRUE,8,1,1);
        //p = gdk_pixbuf_new_from_file(,NULL);
        p = gtk_widget_render_icon(ThemeSelector,GTK_STOCK_MISSING_IMAGE,GTK_ICON_SIZE_LARGE_TOOLBAR,"themelist");
        gtk_list_store_set(ThemeList,&iter,7,p,-1);
        g_object_unref(p);
    }
    g_free(imgpath);
}
static void theme_scan_dir(gchar * dir,gboolean writable)
{
    GDir * d;
    d = g_dir_open(dir,0,NULL);
    if (d)
    {
        gchar * n;
        while((n = (gchar *) g_dir_read_name(d)))
        {
            gchar * fn = g_strdup_printf("%s/%s/theme.ini",dir,n);
            if (g_file_test(fn,G_FILE_TEST_IS_REGULAR))
            {
                //actually add it here
                gchar * o;
                if (writable)
                    o = g_strdup(n);
                else
                    o = g_strdup_printf("* %s",n);
                theme_list_append(o,dir,n);
                g_free(o);
            }
            g_free(fn);
        }
        g_dir_close(d);
    }
}
static void scroll_to_theme(gchar * thn)
{
    GtkTreeIter i;
    gboolean c;
    c=gtk_tree_model_get_iter_first(GTK_TREE_MODEL(ThemeList),&i);
    while(c)
    {
        gchar * s;
        gtk_tree_model_get(GTK_TREE_MODEL(ThemeList),&i,0,&s,-1);
        if (strcmp(s,thn)==0)
        {
            GtkTreePath * p;
            p=gtk_tree_model_get_path(GTK_TREE_MODEL(ThemeList),&i);
            gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(ThemeSelector),p,NULL,TRUE,0.0,0.0);
            gtk_tree_path_free(p);
        }
        g_free(s);
        c=gtk_tree_model_iter_next(GTK_TREE_MODEL(ThemeList),&i);
    }
}
static void refresh_theme_list(gchar * thn)
{
    gchar * path;
    gtk_list_store_clear(ThemeList);
    theme_scan_dir(DATA_DIR "/emerald/themes/",FALSE); 
    path = g_strdup_printf("%s/.emerald/themes/",g_get_home_dir());
    theme_scan_dir(path,TRUE);
    g_free(path);
    if (thn)
        scroll_to_theme(thn);
}
static void cb_refresh(GtkWidget * w, gpointer p)
{
    refresh_theme_list(NULL);
}
static gboolean confirm_dialog(gchar * val, gchar * val2)
{
    GtkWidget * w;
    int ret;
    w = gtk_message_dialog_new(GTK_WINDOW(mainWindow),
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_QUESTION,
            GTK_BUTTONS_YES_NO,
            val,
            val2);
    ret=gtk_dialog_run(GTK_DIALOG(w));
    gtk_widget_destroy(w);
    return (ret == GTK_RESPONSE_YES);
}
static void info_dialog(gchar * val)
{
    GtkWidget * w;
    w = gtk_message_dialog_new(GTK_WINDOW(mainWindow),
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_INFO,
            GTK_BUTTONS_CLOSE,
            "%s",
            val);
    gtk_dialog_run(GTK_DIALOG(w));
    gtk_widget_destroy(w);
}
static void error_dialog(gchar * val)
{
    GtkWidget * w;
    w = gtk_message_dialog_new(GTK_WINDOW(mainWindow),
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            "%s",
            val);
    gtk_dialog_run(GTK_DIALOG(w));
    gtk_widget_destroy(w);
}
static void cb_load(GtkWidget *w, gpointer d)
{
    GKeyFile * f;
    GDir * dr;
    gchar * fn, *at, *mt, *xt, *gt;
    gboolean dist;
    dist=FALSE;
    GtkTreeIter iter;
    GtkTreeModel *model;
    //first normalize the name
    if (gtk_tree_selection_get_selected(ThemeSelect,&model,&iter))
    {
        gtk_tree_model_get(model,&iter,0,&at,-1);
    }
    else
        return;
    mt = at;
    if (strlen(at)>=1 && at[0] == '*')
    {
        if (strlen(at)>=2)
        {
            mt = at+2;
            dist=TRUE;
        }
        else
        {
            error_dialog(_("Invalid Theme Name"));
            g_free(at);
            return;
        }
    }
    if (strlen(mt)==0)
    {
        g_free(at);
        error_dialog(_("Invalid Theme Name"));
        return;
    }
    gtk_widget_set_sensitive(DeleteButton,!dist);
    xt = g_strdup_printf("%s/.emerald/theme/",g_get_home_dir());
    dr = g_dir_open(xt,0,NULL);
    while (dr && (gt = (gchar *)g_dir_read_name(dr)))
    {
        gchar * ft;
        ft = g_strdup_printf("%s/%s",xt,gt);
        g_unlink(ft);
        g_free(ft);
    }
    if (dr)
        g_dir_close(dr);
    if (dist)
    {
        fn = g_strdup_printf(DATA_DIR "/emerald/themes/%s/theme.ini",mt);
        at = g_strdup_printf(DATA_DIR "/emerald/themes/%s/",mt);
    }
    else
    {
        fn = g_strdup_printf("%s/.emerald/themes/%s/theme.ini",g_get_home_dir(),mt);
        at = g_strdup_printf("%s/.emerald/themes/%s/",g_get_home_dir(),mt);
    }
    dr = g_dir_open(at,0,NULL);
    while (dr && (gt = (gchar *)g_dir_read_name(dr)))
    {
        gchar * nt;
        gsize len;
        gchar * ft;
        ft = g_strdup_printf("%s/%s",at,gt);
        if (g_file_get_contents(ft,&nt,&len,NULL))
        {
            g_free(ft);
            ft = g_strdup_printf("%s/%s",xt,gt);
            g_file_set_contents(ft,nt,len,NULL);
            g_free(nt);
        }
        g_free(ft);
    }
    if (dr)
        g_dir_close(dr);
    g_free(xt);
    f = g_key_file_new();
    if (!g_key_file_load_from_file(f,fn,0,NULL))
    {
        g_free(fn);
        g_key_file_free(f);
        g_free(at);
        error_dialog("Invalid Theme File / Name");
        return;
    }
    gtk_entry_set_text(GTK_ENTRY(EntryBox),mt);
    g_free(at);
    set_changed(TRUE);
    set_apply(FALSE);
    g_slist_foreach(get_setting_list(),(GFunc) read_setting,f);
    {
        gchar * c;
        c = g_key_file_get_string(f,"theme","version",NULL);
        if (c)
            gtk_entry_set_text(GTK_ENTRY(Version),c);
        else
            gtk_entry_set_text(GTK_ENTRY(Version),"Pre-0.8");
    }
    set_apply(TRUE);
    send_reload_signal();
    g_key_file_free(f);
    g_free(fn);
}
static gchar * import_theme(gchar * file)
{
    //first make sure we have our location
    gchar * fn, * at, * ot, *pot, *rstr;
    gint ex;
    ot = g_strdup(file);
    pot=ot;
    if (!g_str_has_suffix(ot,".emerald"))
    {
        g_free(pot);
        error_dialog(_("Invalid theme file.  Does not end in .emerald"));
        return NULL;
    }
    ot[strlen(ot)-strlen(".emerald")]='\0';
    ot = g_strrstr(ot,"/");
    if (!ot)
    {
        ot=pot;
    }
    else
        ot++;
    rstr=g_strdup(ot);
    fn = g_strdup_printf("%s/.emerald/themes/%s/",g_get_home_dir(),ot);
    g_free(pot);
    g_mkdir_with_parents(fn,00755);
    at = g_shell_quote(fn);
    g_free(fn);
    fn = g_shell_quote(file);
    ot = g_strdup_printf("tar -xzf %s -C %s",fn,at);
    if (!g_spawn_command_line_sync(ot,NULL,NULL,&ex,NULL) ||
            (WIFEXITED(ex)&&WEXITSTATUS(ex)))
    {
        g_free(fn);
        g_free(ot);
        g_free(at);
        error_dialog("Error calling tar.");
        return NULL;
    }
    g_free(fn);
    g_free(ot);
    g_free(at);
 //   info_dialog(_("Theme Imported"));
    return rstr;
}
static void export_theme(gchar * file)
{
    const gchar * themename = gtk_entry_get_text(GTK_ENTRY(EntryBox));
    gchar * fn, *at, *ot;
    gint ex;
    if (!themename || !strlen(themename) || 
            themename[0]=='*' || strchr(themename,'/'))
    {
        error_dialog(_("Invalid Theme Name\nCould Not Export"));
        //these conditions should already have been handled but another check is ok
        return;
    }
    if (!g_str_has_suffix(file,".emerald"))
    {
        error_dialog(_("Invalid File Name\nMust End in .emerald"));
        return;
    }
    fn = g_strdup_printf("%s/.emerald/theme/",g_get_home_dir());
    at = g_shell_quote(fn);
    g_free(fn);
    fn = g_shell_quote(file);
    ot = g_strdup_printf("tar -czf %s -C %s ./ --exclude=*~",fn,at);
    if (!g_spawn_command_line_sync(ot,NULL,NULL,&ex,NULL) ||
            (WIFEXITED(ex)&&WEXITSTATUS(ex)))
    {
        g_free(fn);
        g_free(ot);
        g_free(at);
        error_dialog(_("Error calling tar."));
        return;
    }
    g_free(ot);
    g_free(fn);
    g_free(at);
    info_dialog(_("Theme Exported"));
}
static void cb_save(GtkWidget *w, gpointer d)
{
    GKeyFile * f;
    gchar * fn, *at, *mt, *gt;
    GDir * dr;
    //first normalize the name
    if (! (at=(gchar *) gtk_entry_get_text(GTK_ENTRY(EntryBox))))
        return;
    if (strlen(at)>=1 && at[0] == '*')
    {
        error_dialog(_("Can't save over system themes\n(or you forgot to enter a name)"));
        return;
    }
    if (strlen(at)==0 || strchr(at,'/'))
    {
        error_dialog(_("Invalid Theme Name"));
        return;
    }
    fn = g_strdup_printf("%s/.emerald/themes/%s",g_get_home_dir(),at);
    g_mkdir_with_parents(fn,00755);
    g_free(fn);
    fn = g_strdup_printf("%s/.emerald/themes/%s/theme.ini",g_get_home_dir(),at);
    f = g_key_file_new();
    g_key_file_load_from_file(f,fn,0,NULL);
    g_slist_foreach(get_setting_list(),(GFunc) write_setting,f);
    g_key_file_set_string(f,"theme","version",VERSION);
    if (g_file_test(fn,G_FILE_TEST_EXISTS))
    {
        if (!confirm_dialog(_("Overwrite Theme %s?"),at))
        {
            return;
        }
    }
    mt = at;
    at = g_key_file_to_data(f,NULL,NULL);
    //little fix since we're now copying from ~/.emerald/theme/*
    g_free(fn);
    fn = g_strdup_printf("%s/.emerald/theme/theme.ini",g_get_home_dir());
    if (at && !g_file_set_contents(fn,at,-1,NULL))
    {
        error_dialog(_("Couldn't Write Theme"));
        g_free(at);
    }
    else if (!at)
    {
        error_dialog(_("Couldn't Form Theme"));
    }
    else
    {
        gchar * xt;
        xt = g_strdup_printf("%s/.emerald/themes/%s/",g_get_home_dir(),mt);
        dr = g_dir_open(xt,0,NULL);
        while (dr && (gt = (gchar *)g_dir_read_name(dr)))
        {
            gchar * ft;
            ft = g_strdup_printf("%s/%s",xt,gt);
            g_unlink(ft);
            g_free(ft);
        }
        if (dr)
            g_dir_close(dr);
        at = g_strdup_printf("%s/.emerald/theme/",g_get_home_dir());
        dr = g_dir_open(at,0,NULL);
        while (dr && (gt = (gchar *)g_dir_read_name(dr)))
        {
            gchar * nt;
            gsize len;
            gchar * ft;
            ft = g_strdup_printf("%s/%s",at,gt);
            if (g_file_get_contents(ft,&nt,&len,NULL))
            {
                g_free(ft);
                ft = g_strdup_printf("%s/%s",xt,gt);
                g_file_set_contents(ft,nt,len,NULL);
                g_free(nt);
            }
            g_free(ft);
        }
        if (dr)
            g_dir_close(dr);
        g_free(xt);
        g_free(at);
        info_dialog(_("Theme Saved"));
    }
    gtk_entry_set_text(GTK_ENTRY(Version),VERSION);
    g_key_file_free(f);
    g_free(fn);
    refresh_theme_list(NULL);
}
static void cb_delete(GtkWidget *w, gpointer d)
{
    GtkTreeIter iter;
    GtkTreeModel *model;
    gchar * fn, *at;
    //first normalize the name
    if (gtk_tree_selection_get_selected(ThemeSelect,&model,&iter))
    {
        gtk_tree_model_get(model,&iter,0,&at,-1);
    }
    else
        return;

    if (strlen(at)>=1 && at[0] == '*')
    {
        g_free(at);
        error_dialog(_("Can't delete system themes\n(or you forgot to enter a name)"));
        return;
    }
    if (strlen(at)==0)
    {
        g_free(at);
        error_dialog(_("Invalid Theme Name"));
        return;
    }
    fn = g_strdup_printf("%s/.emerald/themes/%s/theme.ini",g_get_home_dir(),at);
    if (g_file_test(fn,G_FILE_TEST_EXISTS))
    {
        if (!confirm_dialog(_("Are you sure you want to delete %s?"),at))
        {
            g_free(at);
            g_free(fn);
            return;
        }
        else
        {
            GDir * dir;
            gchar * ot, * mt, * pt;
            pt = g_strdup_printf("%s/.emerald/themes/%s/",g_get_home_dir(),at);
            dir = g_dir_open(pt,0,NULL);
            while (dir && (ot = (gchar *)g_dir_read_name(dir)))
            {
                mt = g_strdup_printf("%s/%s",pt,ot);
                g_unlink(mt);
                g_free(mt);
            }
            if (dir)
            {
                g_dir_close(dir);
                g_rmdir(pt);
            }
            g_free(pt);
        }
        info_dialog(_("Theme deleted"));
        gtk_widget_set_sensitive(w,FALSE);
        refresh_theme_list(NULL);
    }
    else
    {
        error_dialog(_("No such theme to delete"));
    }
    g_free(at);
    g_free(fn);
}
static void cb_main_destroy (GtkWidget * w, gpointer d)
{
    gtk_main_quit();
}
static void layout_button_box(GtkWidget * vbox, gint b_t)
{
    GtkWidget * filesel;
    GtkFileFilter * imgfilter;
    GtkWidget * clearer;
    GtkWidget * image;
    SettingItem * item;

    table_append(gtk_label_new(b_names[b_t]),FALSE);
    
    filesel=gtk_file_chooser_button_new(g_strdup_printf("%s Button Pixmap",b_names[b_t]),
            GTK_FILE_CHOOSER_ACTION_OPEN);
    table_append(filesel,FALSE);
    imgfilter = gtk_file_filter_new();
    gtk_file_filter_set_name(imgfilter,"Images");
    gtk_file_filter_add_pixbuf_formats(imgfilter);
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(filesel),imgfilter);
    
    image = gtk_image_new();
    
    item = register_img_file_setting(filesel,"buttons",b_types[b_t],GTK_IMAGE(image));
    
    table_append(image,TRUE);

    clearer = gtk_button_new_from_stock(GTK_STOCK_CLEAR);
    g_signal_connect(clearer,"clicked",G_CALLBACK(cb_clear_file),item);
    table_append(clearer,FALSE);
}
void layout_general_buttons_frame(GtkWidget * hbox)
{
    GtkWidget * junk;

    junk = gtk_check_button_new_with_label(_("Use Pixmap Buttons"));
    gtk_box_pack_startC(hbox,junk,TRUE,TRUE,0);
    register_setting(junk,ST_BOOL,"buttons","use_pixmap_buttons");

    junk = gtk_check_button_new_with_label(_("Use Button Halo/Glow"));
    gtk_box_pack_startC(hbox,junk,TRUE,TRUE,0);
    register_setting(junk,ST_BOOL,"buttons","use_button_glow");
   
    junk = gtk_check_button_new_with_label(_("Use Button Halo/Glow For Inactive Windows"));
    gtk_box_pack_startC(hbox,junk,TRUE,TRUE,0);
    register_setting(junk,ST_BOOL,"buttons","use_button_inactive_glow");
}
void layout_button_pane(GtkWidget * vbox)
{
    GtkWidget * scroller;
    GtkWidget * hbox;
    gint i;
    /* Yeah, the names should probably be renamed from hbox since it's now
     * a vbox...
     */ 
    hbox = gtk_vbox_new(FALSE,2);
    gtk_box_pack_startC(vbox,hbox,FALSE,FALSE,0);
    layout_general_buttons_frame(hbox);
    
    gtk_box_pack_startC(vbox,gtk_hseparator_new(),FALSE,FALSE,0);

    gtk_box_pack_startC(vbox,gtk_label_new(_("Button Pixmaps")),FALSE,FALSE,0);
    
    scroller=gtk_scrolled_window_new(NULL,NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroller),
            GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
    gtk_box_pack_startC(vbox,scroller,TRUE,TRUE,0);
    
    table_new(4,FALSE,FALSE);
    gtk_scrolled_window_add_with_viewport(
            GTK_SCROLLED_WINDOW(scroller),GTK_WIDGET(get_current_table()));
    
    table_append(gtk_label_new(_("Button")),FALSE);
    table_append(gtk_label_new(_("File")),FALSE);
    table_append(gtk_label_new(_("Preview")),FALSE);
    table_append(gtk_label_new(_("Clear")),FALSE);
    
    for(i=0;i<BX_COUNT;i++)
    {
        layout_button_box(vbox,i);
    }
}
void layout_window_frame(GtkWidget * vbox,gboolean active)
{
    GtkWidget * scrollwin;
    scrollwin = gtk_scrolled_window_new(NULL,NULL);
    gtk_box_pack_startC(vbox,scrollwin,TRUE,TRUE,0);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollwin),
            GTK_POLICY_NEVER,GTK_POLICY_ALWAYS);
    
    table_new(3,FALSE,FALSE);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrollwin),
            GTK_WIDGET(get_current_table()));
    make_labels(_("Colors"));
    ACAV(_("Text Fill"),"text","titlebar");
    ACAV(_("Text Outline"),"text_halo","titlebar");
    table_append_separator();
    ACAV(_("Button Fill"),"button","buttons");
    ACAV(_("Button Outline"),"button_halo","buttons");
}
void add_row(GtkWidget * vbox, GtkWidget * item, gchar * title)
{
    //gtk_box_pack_startC(hbox,gtk_label_new(title),FALSE,FALSE,0);
    //gtk_box_pack_endC(hbox,item,TRUE,TRUE,0);
    table_append(gtk_label_new(title),FALSE);
    table_append(item,TRUE);
}
void add_color_button_row(GtkWidget * vbox, gchar * title, gchar * key, gchar * sect)
{
    GtkWidget * color_button;
    color_button = gtk_color_button_new();
    register_setting(color_button,ST_COLOR,sect,key);
    add_row(vbox,color_button,title);
}
void add_int_range_row(GtkWidget * vbox, gchar * title, gchar * key,
        int start, int end, gchar * sect)
{
    GtkWidget * scaler;
    scaler = scaler_new(start,end,1);
    register_setting(scaler,ST_INT,sect,key);
    add_row(vbox,scaler,title);
}
void add_float_range_row(GtkWidget * vbox, gchar * title, gchar * key,
        gdouble start, gdouble end, gdouble prec, gchar * sect)
{
    GtkWidget * scaler;
    scaler = scaler_new(start,end,prec);
    register_setting(scaler,ST_FLOAT,sect,key);
    add_row(vbox,scaler,title);
}
void layout_shadows_frame(GtkWidget * vbox)
{
    table_new(2,FALSE,FALSE);
    gtk_box_pack_startC(vbox,get_current_table(),FALSE,FALSE,0);
    add_color_button_row(vbox,_("Color"),"shadow_color","shadow");
    add_float_range_row(vbox,_("Opacity"),"shadow_opacity",0.01,6.0,0.01,"shadow");
    add_float_range_row(vbox,_("Radius"),"shadow_radius",0.0,48.0,0.1,"shadow");
    add_int_range_row(vbox,_("X Offset"),"shadow_offset_x",-16,16,"shadow");
    add_int_range_row(vbox,_("Y Offset"),"shadow_offset_y",-16,16,"shadow");
}
void layout_title_frame(GtkWidget * vbox)
{
    GtkWidget * hbox;
    GtkWidget * junk;

    hbox = gtk_hbox_new(FALSE,2);
    gtk_box_pack_startC(vbox,hbox,FALSE,FALSE,0);

    gtk_box_pack_startC(hbox,gtk_label_new(_("Title-Text Font")),FALSE,FALSE,0);
    junk = gtk_font_button_new();
    gtk_box_pack_endC(hbox,junk,FALSE,FALSE,0);
    register_setting(junk,ST_FONT,"titlebar","titlebar_font");

  
    table_new(2,FALSE,FALSE);
    gtk_box_pack_startC(vbox,get_current_table(),FALSE,FALSE,0);
    table_append(gtk_label_new(_("Minimum Title-bar Height")),FALSE);
    junk = scaler_new(0,64,1);
    gtk_range_set_value(GTK_RANGE(junk),17);
    table_append(junk,TRUE);
    register_setting(junk,ST_INT,"titlebar","min_titlebar_height");

    table_append(gtk_label_new(_("Vertical Button Offset")),FALSE);
    junk = scaler_new(0,64,1);
    gtk_range_set_value(GTK_RANGE(junk),1);
    table_append(junk,TRUE);
    register_setting(junk,ST_INT,"buttons","vertical_offset");

    table_append(gtk_label_new(_("Horizontal Button Offset")),FALSE);
    junk = scaler_new(0,64,1);
    gtk_range_set_value(GTK_RANGE(junk),1);
    table_append(junk,TRUE);
    register_setting(junk,ST_INT,"buttons","horizontal_offset");

    hbox = gtk_hbox_new(FALSE,2);
    gtk_box_pack_startC(vbox,hbox,FALSE,FALSE,0);
    gtk_box_pack_startC(hbox,gtk_label_new(_("Title-bar Object Layout")),FALSE,FALSE,0);
    junk = gtk_combo_box_entry_new_text();
    gtk_combo_box_append_text(GTK_COMBO_BOX(junk),"IT::HNXC:Normal Layout");
    gtk_combo_box_append_text(GTK_COMBO_BOX(junk),"CNX:IT:HM:OSX Layout");
    register_setting(junk,ST_STRING_COMBO,"titlebar","title_object_layout");
    gtk_box_pack_startC(hbox,junk,FALSE,FALSE,0);
    
    gtk_box_pack_startC(vbox,gtk_label_new(
                _("Use colons to separate left, center, and right.\n"
                "Anything after a third colon is ignored.\n"
                "C=Close, N=Minimize, X/R=Restore\n"
                "I=Icon, T=Title, H=Help, M=Menu\n"
                "S=Shade, U/A=On Top(Up/Above)\n"
                "Y=Sticky, (#)=# pixels space, ex: (5)\n")
                //"U=On Top, D=On Bottom, S=Shade\n"
                //"Y=Sticky (on All Desktops)\n"
                ),FALSE,FALSE,0);

    /*junk = gtk_check_button_new_with_label("Use active colors for whole active frame, not just titlebar.");
    gtk_box_pack_startC(vbox,junk,FALSE,FALSE,0);
    register_setting(junk,ST_BOOL,SECT,"use_active_colors");*/
}
void add_meta_string_value(gchar * title, gchar * key)
{
    GtkWidget * entry;
    table_append(gtk_label_new(title),FALSE);
    entry = gtk_entry_new();
    table_append(entry,TRUE);
    register_setting(entry,ST_META_STRING,"theme",key);
}
static void cb_export(GtkWidget * w, gpointer p)
{
    //get a filename
    GtkWidget * dialog = gtk_file_chooser_dialog_new(
            _("Export Theme..."),GTK_WINDOW(mainWindow),
            GTK_FILE_CHOOSER_ACTION_SAVE,
            GTK_STOCK_CANCEL,GTK_RESPONSE_CANCEL,
            GTK_STOCK_SAVE,GTK_RESPONSE_ACCEPT,
            NULL
            );
    gchar * pth = g_strdup_printf("%s/Desktop/",g_get_home_dir());
    GtkFileFilter * filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter,"Theme Packages");
    gtk_file_filter_add_pattern(filter,"*.emerald");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog),filter);
    gtk_file_chooser_set_do_overwrite_confirmation (
            GTK_FILE_CHOOSER (dialog), TRUE);
    gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER(dialog),
            pth);
    g_free(pth);
    pth = g_strdup_printf("%s.emerald",gtk_entry_get_text(
                GTK_ENTRY(EntryBox)));
    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog),
            pth);
    g_free(pth);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
    {
        gchar * filename;
        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        export_theme(filename);
        g_free(filename);
    }
    gtk_widget_destroy(dialog);
}

void layout_file_frame(GtkWidget * vbox)
{
    GtkWidget * junk;
    GtkWidget * hbox;

    hbox = gtk_hbox_new(FALSE,2);
    gtk_box_pack_startC(vbox,hbox,FALSE,FALSE,0);
    
    gtk_box_pack_startC(hbox,gtk_label_new(_("Name:")),FALSE,FALSE,0);
    EntryBox = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(EntryBox),_("Untitled Theme"));
    gtk_box_pack_startC(hbox,EntryBox,TRUE,TRUE,0);

    junk = gtk_button_new_from_stock(GTK_STOCK_SAVE);
    gtk_box_pack_startC(hbox,junk,FALSE,FALSE,0);
    g_signal_connect(junk,"clicked",G_CALLBACK(cb_save),NULL);
    
    ExportButton = gtk_button_new_with_label(_("Export..."));
    gtk_button_set_image(GTK_BUTTON(ExportButton),
            gtk_image_new_from_stock(GTK_STOCK_SAVE_AS,GTK_ICON_SIZE_BUTTON));
    gtk_box_pack_startC(hbox,ExportButton,FALSE,FALSE,0);
    g_signal_connect(ExportButton,"clicked",G_CALLBACK(cb_export),NULL);
}
void layout_info_frame(GtkWidget * vbox)
{
    table_new(2,FALSE,FALSE);
    gtk_box_pack_startC(vbox,get_current_table(),FALSE,FALSE,0);
    add_meta_string_value(_("Creator"),"creator");
    add_meta_string_value(_("Description"),"description");
    add_meta_string_value(_("Theme Version"),"theme_version");
    add_meta_string_value(_("Suggested Widget Theme"), "suggested");

    table_append_separator();

    table_append(gtk_label_new(_("Themer Version")),FALSE);

    Version = gtk_entry_new();
    gtk_editable_set_editable(GTK_EDITABLE(Version),FALSE);
    gtk_widget_set_sensitive(Version,FALSE);
    table_append(Version,TRUE);
}
void add_border_slider(gchar * text,gchar * key,gint value)
{
    GtkWidget * w;
    table_append(gtk_label_new(text),FALSE);

    w = scaler_new(0,20,1);
    table_append(w,TRUE);
    gtk_range_set_value(GTK_RANGE(w),value);
    register_setting(w,ST_INT,"borders",key);
}
void layout_borders_frame(GtkWidget * vbox)
{
    table_new(2,FALSE,FALSE);
    gtk_box_pack_startC(vbox,get_current_table(),FALSE,FALSE,0);
    table_append(gtk_label_new(_("Border")),FALSE);
    table_append(gtk_label_new(_("Size")),FALSE);
    add_border_slider(_("Top"),"top",4);
    add_border_slider(_("Bottom"),"bottom",6);
    add_border_slider(_("Left"),"left",6);
    add_border_slider(_("Right"),"right",6);
    gtk_box_pack_startC(vbox,gtk_label_new(
                _("Note, when changing these values,\n"
                "it is advised that you do something\n"
                "like change viewports, as this will\n"
                "cause the position of the 'event windows'\n"
                "to update properly, and thus make\n"
                "moving/resizing windows work properly.\n"
                "This also applies to when you change\n"
                "the title-bar height through either\n"
                "the titlebar-min-height slider\n"
                "or the titlebar font setting.\n")
                ),FALSE,FALSE,0);
}
void layout_left_frame_pane(GtkWidget * hbox)
{
    GtkWidget * vbox;
    vbox = gtk_vbox_new(FALSE,2);
    gtk_box_pack_startC(hbox,vbox,TRUE,TRUE,0);

    layout_shadows_frame(build_frame(vbox,_("Shadows"),FALSE));
}
void layout_right_frame_pane(GtkWidget * hbox)
{
    GtkWidget * vbox;
    vbox = gtk_vbox_new(FALSE,2);
    gtk_box_pack_startC(hbox,vbox,TRUE,TRUE,0);

    layout_borders_frame(build_frame(vbox,_("Frame Borders"),FALSE));
}
void layout_frame_pane(GtkWidget * vbox)
{
    GtkWidget * hbox;

    hbox = gtk_hbox_new(TRUE,2);
    gtk_box_pack_startC(vbox,hbox,TRUE,TRUE,0);
    
    layout_left_frame_pane(hbox);
    layout_right_frame_pane(hbox);
}
void layout_left_global_pane(GtkWidget * hbox)
{
    GtkWidget * vbox;
    vbox = gtk_vbox_new(FALSE,2);
    gtk_box_pack_startC(hbox,vbox,TRUE,TRUE,0);

    layout_window_frame(build_frame(vbox,_("Active Window"),FALSE),TRUE);
    layout_window_frame(build_frame(vbox,_("Inactive Window"),FALSE),FALSE);
    //layout stuff here.
}
void layout_right_global_pane(GtkWidget * hbox)
{
    GtkWidget * vbox;
    vbox = gtk_vbox_new(FALSE,2);
    gtk_box_pack_startC(hbox,vbox,TRUE,TRUE,0);
    
    layout_title_frame(build_frame(vbox,_("Title Bar"),FALSE));
}
void layout_global_pane(GtkWidget * vbox)
{
    GtkWidget * hbox;

    hbox = gtk_hbox_new(TRUE,2);
    gtk_box_pack_startC(vbox,hbox,TRUE,TRUE,0);
    
    layout_left_global_pane(hbox);
    layout_right_global_pane(hbox);
}
void layout_screenshot_frame(GtkWidget * vbox)
{
    GtkWidget * filesel;
    GtkFileFilter * imgfilter;
    GtkWidget * clearer;
    GtkWidget * image;
    SettingItem * item;
    GtkWidget * hbox;
    GtkWidget * scrollwin;

    image = gtk_image_new();

    scrollwin = gtk_scrolled_window_new(NULL,NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollwin),
            GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrollwin),image);
    gtk_widget_set_size_request(scrollwin,300,160);
    
    gtk_box_pack_startC(vbox,scrollwin,TRUE,TRUE,0);

    hbox = gtk_hbox_new(TRUE,2);
    gtk_box_pack_startC(vbox,hbox,FALSE,FALSE,0);

    filesel=gtk_file_chooser_button_new(_("Screenshot"),
            GTK_FILE_CHOOSER_ACTION_OPEN);
    gtk_box_pack_startC(hbox,filesel,TRUE,TRUE,0);
    imgfilter = gtk_file_filter_new();
    gtk_file_filter_set_name(imgfilter,_("Images"));
    gtk_file_filter_add_pixbuf_formats(imgfilter);
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(filesel),imgfilter);
    
    item = register_img_file_setting(filesel,"theme","screenshot",GTK_IMAGE(image));
    
    clearer = gtk_button_new_from_stock(GTK_STOCK_CLEAR);
    g_signal_connect(clearer,"clicked",G_CALLBACK(cb_clear_file),item);
    gtk_box_pack_startC(hbox,clearer,TRUE,TRUE,0);
}
void layout_left_theme_pane(GtkWidget * hbox)
{
    GtkWidget * vbox;
    vbox = gtk_vbox_new(FALSE,2);
    gtk_box_pack_startC(hbox,vbox,TRUE,TRUE,0);
    layout_screenshot_frame(build_frame(vbox,_("Screenshot"),FALSE));
}
void layout_right_theme_pane(GtkWidget * hbox)
{
    GtkWidget * vbox;
    vbox = gtk_vbox_new(FALSE,2);
    gtk_box_pack_startC(hbox,vbox,TRUE,TRUE,0);
    layout_info_frame(build_frame(vbox,_("Information"),FALSE));
}
void layout_theme_pane(GtkWidget * vbox)
{
    GtkWidget * hbox;
    hbox = gtk_hbox_new(FALSE,2);
    gtk_box_pack_startC(vbox,hbox,TRUE,TRUE,0);
    layout_left_theme_pane(hbox);
    layout_right_theme_pane(hbox);
}
void layout_settings_pane(GtkWidget * vbox)
{
    GtkWidget * combo;
    GtkWidget * junk;
    gint i;
    gtk_box_pack_startC(vbox,gtk_label_new(
                _("NOTE - These settings are not part of themes, "
                "they are stored separately, and control various UI "
                "preferences for Emerald.")
                ),FALSE,FALSE,0);
    gtk_box_pack_startC(vbox,gtk_hseparator_new(),FALSE,FALSE,0);

    junk = gtk_check_button_new_with_label(_("Show Tooltips for Buttons"));
    gtk_box_pack_startC(vbox,junk,FALSE,FALSE,0);
    register_setting(junk,ST_SFILE_BOOL,"buttons","enable_tooltips");

    junk = gtk_check_button_new_with_label(_("Use Decoration Cropping"));
    gtk_box_pack_startC(vbox,junk,FALSE,FALSE,0);
    register_setting(junk,ST_SFILE_BOOL,"decorations","use_decoration_cropping");
	
    junk = gtk_check_button_new_with_label(_("Use Button Fade"));
    gtk_box_pack_startC(vbox,junk,FALSE,FALSE,0);
    register_setting(junk,ST_SFILE_BOOL,"buttons","use_button_fade");

    junk = gtk_check_button_new_with_label(_("Use Button Fade Pulse"));
    gtk_box_pack_startC(vbox,junk,FALSE,FALSE,0);
    register_setting(junk,ST_SFILE_BOOL,"buttons","use_button_fade_pulse");

    table_new(2,FALSE,FALSE);
    gtk_box_pack_startC(vbox,get_current_table(),FALSE,FALSE,0);

    table_append(gtk_label_new(_("Button Fade Total Duration")),FALSE);
    junk = scaler_new(1,4000,1);
    gtk_range_set_value(GTK_RANGE(junk),250);
    table_append(junk,TRUE);
    register_setting(junk,ST_SFILE_INT,"buttons","button_fade_total_duration");

    table_append(gtk_label_new(_("Button Fade Step Duration")),FALSE);
    junk = scaler_new(1,2000,1);
    gtk_range_set_value(GTK_RANGE(junk),50);
    table_append(junk,TRUE);
    register_setting(junk,ST_SFILE_INT,"buttons","button_fade_step_duration");

    table_append(gtk_label_new(_("Button Pulse Wait Duration")),FALSE);
    junk = scaler_new(0,4000,1);
    gtk_range_set_value(GTK_RANGE(junk),0);
    table_append(junk,TRUE);
    register_setting(junk,ST_SFILE_INT,"buttons","button_fade_pulse_wait_duration");

    table_append(gtk_label_new(_("Button Pulse Min Opacity %")),FALSE);
    junk = scaler_new(0,100,1);
    gtk_range_set_value(GTK_RANGE(junk),0);
    table_append(junk,TRUE);
    register_setting(junk,ST_SFILE_INT,"buttons","button_fade_pulse_min_opacity");

    table_append(gtk_label_new(_("Titlebar Double-Click Action")),FALSE);
    combo = gtk_combo_box_new_text();
    for (i=0;i<TITLEBAR_ACTION_COUNT;i++)
    {
        gtk_combo_box_append_text(GTK_COMBO_BOX(combo),titlebar_action_name[i]);
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo),0);
    table_append(combo,TRUE);
    register_setting(combo,ST_SFILE_INT_COMBO,"titlebars",
            "double_click_action");

    table_append(gtk_label_new(_("Button Hover Cursor")),FALSE);
    combo = gtk_combo_box_new_text();
    gtk_combo_box_append_text(GTK_COMBO_BOX(combo),_("Normal"));
    gtk_combo_box_append_text(GTK_COMBO_BOX(combo),_("Pointing Finger"));
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo),1);
    table_append(combo,TRUE);
    register_setting(combo,ST_SFILE_INT_COMBO,"buttons",
            "hover_cursor");

    table_append(gtk_label_new(_("Compiz Decoration Blur Type")),FALSE);
    combo = gtk_combo_box_new_text();
    gtk_combo_box_append_text(GTK_COMBO_BOX(combo),_("None"));
    gtk_combo_box_append_text(GTK_COMBO_BOX(combo),_("Titlebar only"));
    gtk_combo_box_append_text(GTK_COMBO_BOX(combo),_("All decoration"));    
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo), BLUR_TYPE_NONE);
    table_append(combo,TRUE);
    register_setting(combo,ST_SFILE_INT_COMBO,"decorations",
            "blur_type");
    
    /*table_append(gtk_label_new("Icon Click Action"),FALSE);
    combo = gtk_combo_box_new_text();
    gtk_combo_box_append_text(GTK_COMBO_BOX(combo),"None");
    gtk_combo_box_append_text(GTK_COMBO_BOX(combo),"Window Menu");
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo),0);
    table_append(combo,FALSE);
    register_setting(combo,ST_SFILE_INT_COMBO,"window_icon",
            "click_action");
    
    table_append(gtk_label_new("Icon Double-Click Action"),FALSE);
    combo = gtk_combo_box_new_text();
    gtk_combo_box_append_text(GTK_COMBO_BOX(combo),"None");
    gtk_combo_box_append_text(GTK_COMBO_BOX(combo),"Close Window");
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo),0);
    table_append(combo,FALSE);
    register_setting(combo,ST_SFILE_INT_COMBO,"window_icon",
            "double_click_action");*/
    //TODO - implement the emerald side of these, so for now they won't be here.
}
void layout_engine_pane(GtkWidget * vbox)
{
    GtkWidget * nvbox, * scwin;
    nvbox = gtk_vbox_new(FALSE,2);
    scwin = gtk_scrolled_window_new(NULL,NULL);
    gtk_box_pack_startC(vbox,scwin,TRUE,TRUE,0);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scwin),
            GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scwin),nvbox);
    layout_engine_list(nvbox);
    init_engine_list();
}
void layout_lower_pane(GtkWidget * vbox)
{
    GtkWidget * notebook;
    
    //layout_mid_pane(vbox);

    notebook = gtk_notebook_new();
    gtk_box_pack_startC(vbox,notebook,TRUE,TRUE,0);

    layout_engine_pane(build_notebook_page(_("Frame Engine"),notebook));
    layout_button_pane(build_notebook_page(_("Buttons"),notebook));
    layout_frame_pane(build_notebook_page(_("Frame/Shadows"),notebook));
    layout_global_pane(build_notebook_page(_("Titlebar"),notebook));
    layout_theme_pane(build_notebook_page(_("Theme"),notebook));

    layout_file_frame(vbox);
}

GtkWidget * build_lower_pane(GtkWidget * vbox)
{
    GtkWidget * expander;
    GtkWidget * my_vbox;

    expander = gtk_expander_new(_("Edit"));
    gtk_box_pack_startC(vbox,expander,FALSE,FALSE,0);
    gtk_expander_set_expanded(GTK_EXPANDER(expander),FALSE);

    my_vbox = gtk_vbox_new(FALSE,2);
    gtk_container_addC(expander,my_vbox);

    return my_vbox;
}


void cb_refilter(GtkWidget * w, gpointer p)
{
    GtkTreeModelFilter * filt = p;
    gtk_tree_model_filter_refilter(filt);
}

gboolean is_visible(GtkTreeModel * model, GtkTreeIter *iter, gpointer p)
{
    GtkEntry * e = p;
    static gint cols[]={0,2,3,5,-1};
    gint i;
    const gchar * ch;
    if (strlen(ch=gtk_entry_get_text(e))==0)
        return TRUE;
    ch = g_ascii_strup(ch,-1);
    for (i=0;cols[i]>=0;i++)
    {
        gchar * at;
        gtk_tree_model_get(model,iter,i,&at,-1);
        at = g_ascii_strup(at,-1);
        if (strlen(at) && strstr(at,ch))
        {
            g_free(at);
            return TRUE;
        }
        g_free(at);
    }
    //0, 2, 3, 5
    return FALSE;
}
void cb_clearbox(GtkWidget * w, gpointer p)
{
    gtk_entry_set_text(GTK_ENTRY(p),"");
}
static void cb_import(GtkWidget * w, gpointer p)
{
    //get a filename
    GtkWidget * dialog = gtk_file_chooser_dialog_new(
            _("Import Theme..."),GTK_WINDOW(mainWindow),
            GTK_FILE_CHOOSER_ACTION_OPEN,
            GTK_STOCK_CANCEL,GTK_RESPONSE_CANCEL,
            GTK_STOCK_OPEN,GTK_RESPONSE_ACCEPT,
            NULL);
    gchar * pth = g_strdup_printf("%s/Desktop/",g_get_home_dir());
    GtkFileFilter * filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter,"Theme Packages");
    gtk_file_filter_add_pattern(filter,"*.emerald");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog),filter);
    gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER(dialog),
            pth);
    g_free(pth);
    if (gtk_dialog_run(GTK_DIALOG(dialog))==GTK_RESPONSE_ACCEPT)
    {
        gchar * filename;
        gchar * thn;
        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        thn = import_theme(filename);
        g_free(filename);
        refresh_theme_list(thn);
        if (thn) g_free(thn);
    }
    gtk_widget_destroy(dialog);
}

GtkWidget * build_tree_view()
{
    GtkWidget * scrollwin;
    GtkTreeModelFilter * filt;
    GtkTreeModelSort * sort;
    GtkWidget * searchbox;
    GtkWidget * vbox;
    GtkWidget * hbox;
    GtkWidget * clearbut;

    vbox = gtk_vbox_new(FALSE,2);
    hbox = gtk_hbox_new(FALSE,2);
    gtk_box_pack_startC(vbox,hbox,FALSE,FALSE,0);

    gtk_box_pack_startC(hbox,gtk_label_new(_("Search:")),FALSE,FALSE,0);

    ThemeList = gtk_list_store_new(9,G_TYPE_STRING,G_TYPE_STRING,
            G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,
            G_TYPE_STRING,GDK_TYPE_PIXBUF,G_TYPE_STRING);

    searchbox = gtk_entry_new();
    gtk_box_pack_startC(hbox,searchbox,TRUE,TRUE,0);

    clearbut = gtk_button_new_from_stock(GTK_STOCK_CLEAR);
    g_signal_connect(clearbut,"clicked",G_CALLBACK(cb_clearbox),searchbox);
    gtk_box_pack_startC(hbox,clearbut,FALSE,FALSE,0);

	gtk_box_pack_startC(hbox,gtk_vseparator_new(),FALSE,FALSE,0);

    ReloadButton = gtk_button_new_from_stock(GTK_STOCK_REFRESH);
    gtk_box_pack_startC(hbox,ReloadButton,FALSE,FALSE,0);
    g_signal_connect(ReloadButton,"clicked",G_CALLBACK(cb_refresh),NULL);

    DeleteButton = gtk_button_new_from_stock(GTK_STOCK_DELETE);
    gtk_box_pack_startC(hbox,DeleteButton,FALSE,FALSE,0);
    gtk_widget_set_sensitive(DeleteButton,FALSE);
    g_signal_connect(DeleteButton,"clicked",G_CALLBACK(cb_delete),NULL);

    ImportButton = gtk_button_new_with_label("Import...");
    gtk_button_set_image(GTK_BUTTON(ImportButton),
            gtk_image_new_from_stock(GTK_STOCK_OPEN,GTK_ICON_SIZE_BUTTON));
    gtk_box_pack_startC(hbox,ImportButton,FALSE,FALSE,0);
    g_signal_connect(ImportButton,"clicked",G_CALLBACK(cb_import),NULL);
 
	gtk_box_pack_startC(vbox,gtk_hseparator_new(),FALSE,FALSE,0);

    filt = GTK_TREE_MODEL_FILTER(
            gtk_tree_model_filter_new(GTK_TREE_MODEL(ThemeList),NULL));
    gtk_tree_model_filter_set_visible_func(filt,
            (GtkTreeModelFilterVisibleFunc)is_visible,searchbox,NULL);

    sort = GTK_TREE_MODEL_SORT(
            gtk_tree_model_sort_new_with_model(GTK_TREE_MODEL(filt)));

    g_signal_connect(searchbox,"changed",G_CALLBACK(cb_refilter),filt);

    ThemeSelector = gtk_tree_view_new_with_model(GTK_TREE_MODEL(sort));
    gtk_tree_view_set_headers_clickable(GTK_TREE_VIEW(ThemeSelector),TRUE);
    gtk_tree_view_set_reorderable(GTK_TREE_VIEW(ThemeSelector),TRUE);

    ThemeRenderer = gtk_cell_renderer_text_new();
    ThemeColumn = gtk_tree_view_column_new_with_attributes
        ("Theme",ThemeRenderer,"markup",8,NULL);
    g_object_set(ThemeRenderer,"ellipsize",PANGO_ELLIPSIZE_END,"ellipsize-set",TRUE,NULL);
    gtk_tree_view_column_set_sort_column_id(ThemeColumn,8);
    gtk_tree_view_append_column(GTK_TREE_VIEW(ThemeSelector),ThemeColumn);
    gtk_tree_view_column_set_resizable(ThemeColumn,TRUE);
    gtk_tree_view_column_set_reorderable(ThemeColumn,TRUE);
    gtk_tree_view_column_set_expand(ThemeColumn,TRUE);

    MetaRenderer = gtk_cell_renderer_pixbuf_new();
    g_object_set(MetaRenderer,"xalign",(gfloat)0.0,NULL);
    MetaColumn = gtk_tree_view_column_new_with_attributes
        ("Screenshot",MetaRenderer,"pixbuf",7,NULL);
    //gtk_tree_view_column_set_sort_column_id(MetaColumn,7);
    gtk_tree_view_append_column(GTK_TREE_VIEW(ThemeSelector),MetaColumn);
    gtk_tree_view_column_set_max_width(MetaColumn,400);
    gtk_tree_view_column_set_resizable(MetaColumn,TRUE);
    gtk_tree_view_column_set_reorderable(MetaColumn,TRUE);

    /*MetaRenderer = gtk_cell_renderer_text_new();
    g_object_set(MetaRenderer,"wrap-mode",PANGO_WRAP_WORD,"wrap-width",30,NULL);
    MetaColumn = gtk_tree_view_column_new_with_attributes
        ("Version",MetaRenderer,"text",4,NULL);
    gtk_tree_view_column_set_sort_column_id(MetaColumn,4);
    gtk_tree_view_append_column(GTK_TREE_VIEW(ThemeSelector),MetaColumn);
    gtk_tree_view_column_set_resizable(MetaColumn,FALSE);
    gtk_tree_view_column_set_reorderable(MetaColumn,TRUE);*/
   
    MetaRenderer = gtk_cell_renderer_text_new();
    MetaColumn = gtk_tree_view_column_new_with_attributes
        ("Up-to-Date",MetaRenderer,"text",1,NULL);
    gtk_tree_view_column_set_sort_column_id(MetaColumn,1);
    gtk_tree_view_append_column(GTK_TREE_VIEW(ThemeSelector),MetaColumn);
    gtk_tree_view_column_set_resizable(MetaColumn,TRUE);
    gtk_tree_view_column_set_reorderable(MetaColumn,TRUE);

    MetaRenderer = gtk_cell_renderer_text_new();
    MetaColumn = gtk_tree_view_column_new_with_attributes
        ("Engine",MetaRenderer,"text",6,NULL);
    gtk_tree_view_column_set_sort_column_id(MetaColumn,6);
    gtk_tree_view_append_column(GTK_TREE_VIEW(ThemeSelector),MetaColumn);
    gtk_tree_view_column_set_resizable(MetaColumn,TRUE);
    gtk_tree_view_column_set_reorderable(MetaColumn,TRUE);

    /*MetaRenderer = gtk_cell_renderer_text_new();
    g_object_set(MetaRenderer,"wrap-mode",PANGO_WRAP_WORD,"wrap-width",80,NULL);
    MetaColumn = gtk_tree_view_column_new_with_attributes
        ("Suggested Widget Theme",MetaRenderer,"text",5,NULL);
    gtk_tree_view_column_set_sort_column_id(MetaColumn,5);
    gtk_tree_view_append_column(GTK_TREE_VIEW(ThemeSelector),MetaColumn);
    gtk_tree_view_column_set_resizable(MetaColumn,TRUE);
    gtk_tree_view_column_set_reorderable(MetaColumn,TRUE);

    MetaRenderer = gtk_cell_renderer_text_new();
    g_object_set(MetaRenderer,"wrap-mode",PANGO_WRAP_WORD,"wrap-width",80,NULL);
    MetaColumn = gtk_tree_view_column_new_with_attributes
        ("Creator",MetaRenderer,"text",2,NULL);
    gtk_tree_view_column_set_sort_column_id(MetaColumn,2);
    gtk_tree_view_append_column(GTK_TREE_VIEW(ThemeSelector),MetaColumn);
    gtk_tree_view_column_set_resizable(MetaColumn,TRUE);
    gtk_tree_view_column_set_reorderable(MetaColumn,TRUE);*/

    /*MetaRenderer = gtk_cell_renderer_text_new();
    g_object_set(MetaRenderer,"wrap-mode",PANGO_WRAP_WORD,"wrap-width",100,NULL);
    MetaColumn = gtk_tree_view_column_new_with_attributes
        ("Description",MetaRenderer,"text",3,NULL);
    gtk_tree_view_column_set_sort_column_id(MetaColumn,3);
    gtk_tree_view_append_column(GTK_TREE_VIEW(ThemeSelector),MetaColumn);
    gtk_tree_view_column_set_resizable(MetaColumn,TRUE);*/

    ThemeSelect = gtk_tree_view_get_selection(
            GTK_TREE_VIEW(ThemeSelector));
    gtk_tree_selection_set_mode(ThemeSelect,GTK_SELECTION_SINGLE);
    g_signal_connect(ThemeSelect,"changed",
            G_CALLBACK(cb_load),NULL);

    scrollwin = gtk_scrolled_window_new(NULL,NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollwin),
            GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
    gtk_container_addC(scrollwin,ThemeSelector);
    //gtk_widget_set_size_request(scrollwin,500,200);

    gtk_box_pack_startC(vbox,scrollwin,TRUE,TRUE,0);
    return vbox;
}
void import_cache(GtkWidget * progbar)
{
    GDir * d;
    d = g_dir_open(themecache,0,NULL);
    if (d)
    {
        gchar * n;
        while((n = (gchar *) g_dir_read_name(d)))
        {
	   
            gchar * fn;
            if (g_str_has_suffix(n,".emerald"))
            {
              fn = g_strconcat(themecache,"/",NULL);
	      fn = g_strconcat(fn,n,NULL);
                import_theme(fn);
                g_free(fn);
                gtk_progress_bar_pulse(GTK_PROGRESS_BAR(progbar));
            }
        }
        g_free(n);
        g_dir_close(d);
    }
}

gboolean watcher_func(gpointer p)
{
    FetcherInfo * f = p;

    gtk_progress_bar_pulse(GTK_PROGRESS_BAR(f->progbar));
    if (waitpid(f->pd,NULL,WNOHANG)!=0)
    {
	import_cache(f->progbar);
	refresh_theme_list(NULL);
        gtk_widget_destroy(f->dialog);
	g_free(themecache);
        free(p);
        return FALSE;
    }
    return TRUE;
}
void fetch_svn()
{
    gchar* themefetcher[] = {g_strdup("svn"), g_strdup("co"), g_strdup(svnpath), g_strdup(themecache), NULL };
    GtkWidget * w;
    GtkWidget * l;
    GPid pd;
    FetcherInfo * fe = malloc(sizeof(FetcherInfo));
    w = gtk_dialog_new_with_buttons(_("Fetching Themes"),
            GTK_WINDOW(mainWindow),
            GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL, NULL);
    l = gtk_label_new(_("Fetching themes... \n"
                       "This may take time depending on \n"
                       "internet connection speed."));
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(w)->vbox),l,FALSE,FALSE,0);
    l = gtk_progress_bar_new();
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(w)->action_area),l,TRUE,TRUE,0);
    gtk_widget_show_all(w);
    g_spawn_async(NULL,themefetcher,NULL,
            G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD,
            NULL,NULL,&pd,NULL);
    g_free(themefetcher[4]);
    fe->dialog=w;
    fe->progbar=l;
    fe->pd=pd;
    g_timeout_add(100,watcher_func,fe);
}
void fetch_gpl_svn()
{
	svnpath="http://emerald-themes.googlecode.com/svn/trunk/emerald-themes-repo";
	themecache=g_strconcat(g_get_home_dir(),"/.emerald/themecache",NULL);
	fetch_svn();
}
void fetch_ngpl_svn()
{
	svnpath="https://svn.generation.no/emerald-themes";
	themecache=g_strconcat(g_get_home_dir(),"/.emerald/ngplthemecache",NULL);
	fetch_svn();
}
void cb_quit(GtkWidget * w, gpointer p)
{
    gtk_widget_destroy(mainWindow);
}
void layout_upper_pane(GtkWidget * vbox)
{
    //GtkWidget * hbox;

    //hbox = gtk_hbox_new(FALSE,2);
    //gtk_box_pack_startC(vbox,hbox,TRUE,TRUE,0);

    gtk_box_pack_startC(vbox,build_tree_view(),TRUE,TRUE,0);

    //table_new(1,TRUE,FALSE);
    //gtk_box_pack_startC(hbox,get_current_table(),FALSE,FALSE,0);
    

}
void layout_repo_pane(GtkWidget * vbox)
{
	GtkWidget * hbox;
	GtkWidget * rlabel;
	gtk_box_pack_startC(vbox,gtk_label_new(
		_("Here are the repositories that you can fetch Emerald Themes from. \n"
	"Fetching themes would fetch and import themes from SVN repositories \n"
	"You need Subversion package installed to use this feature."
	)),FALSE,FALSE,0);
    gtk_box_pack_startC(vbox,gtk_hseparator_new(),FALSE,FALSE,0);
    hbox = gtk_hbox_new(FALSE,2);
    gtk_box_pack_startC(vbox,hbox,TRUE,TRUE,0);


    table_new(2,TRUE,FALSE);
    gtk_box_pack_startC(hbox,get_current_table(),FALSE,FALSE,0);
       
    FetchButton = gtk_button_new_with_label("Fetch GPL'd Themes");
    gtk_button_set_image(GTK_BUTTON(FetchButton),
            gtk_image_new_from_stock(GTK_STOCK_CONNECT,GTK_ICON_SIZE_BUTTON));
    table_append(FetchButton,FALSE);
    g_signal_connect(FetchButton,"clicked",G_CALLBACK(fetch_gpl_svn),NULL);
	
	rlabel = gtk_label_new(
		_("This repository contains GPL'd themes that can be used under \n"
	"the terms of GNU GPL licence v2.0 or later \n"));
	table_append(rlabel,FALSE);
	
	FetchButton2 = gtk_button_new_with_label("Fetch non GPL'd Themes");
    gtk_button_set_image(GTK_BUTTON(FetchButton2),
            gtk_image_new_from_stock(GTK_STOCK_CONNECT,GTK_ICON_SIZE_BUTTON));
    table_append(FetchButton2,FALSE);
    g_signal_connect(FetchButton2,"clicked",G_CALLBACK(fetch_ngpl_svn),NULL);
	
	rlabel = gtk_label_new(
		_("This repository contains non-GPL'd themes. They might infringe \n"
		"copyrights and patent laws in some countries."));
	table_append(rlabel,FALSE);
	
	gtk_box_pack_startC(vbox,gtk_hseparator_new(),FALSE,FALSE,0);
	
		gtk_box_pack_startC(vbox,gtk_label_new(
		_("To activate Non-GPL repository please run the following in shell and accept the server certificate permanently: \n"
		"svn ls https://svn.generation.no/emerald-themes."
	)),FALSE,FALSE,0);
}
void layout_themes_pane(GtkWidget * vbox)
{
    GtkWidget * notebook;  
    notebook = gtk_notebook_new();
    gtk_box_pack_startC(vbox,notebook,TRUE,TRUE,0);
    layout_upper_pane(build_notebook_page(_("Themes"),notebook));
    layout_lower_pane(build_notebook_page(_("Edit Themes"),notebook));
//	layout_repo_pane(build_notebook_page(_("Repositories"),notebook));
	
}
GtkWidget* create_filechooserdialog1 (char *input)
{
   	
    //get a filename
    GtkWidget * dialog_startup = gtk_file_chooser_dialog_new(
            _("Import Theme..."),NULL,
            GTK_FILE_CHOOSER_ACTION_OPEN,
            GTK_STOCK_CANCEL,GTK_RESPONSE_CANCEL,
            GTK_STOCK_OPEN,GTK_RESPONSE_ACCEPT,
            NULL);

    gchar * pth = g_strdup_printf("%s",input);
    GtkFileFilter * filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter,"Theme Packages");
    gtk_file_filter_add_pattern(filter,"*.emerald");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog_startup),filter);
    gtk_file_chooser_set_filename (GTK_FILE_CHOOSER(dialog_startup),
            pth);
    g_free(pth);
    if (gtk_dialog_run(GTK_DIALOG(dialog_startup))==GTK_RESPONSE_ACCEPT)
    {
        gchar * filename;
        gchar * thn;
        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog_startup));
        thn=import_theme(filename);
        g_free(filename);
        refresh_theme_list(thn);
        if (thn) g_free(thn);
    }
  
        gtk_widget_destroy(dialog_startup);
        return dialog_startup;
}
void layout_main_window()
{
    GtkWidget * notebook;
    GtkWidget * vbox;
    GtkWidget * hbox;

    vbox = gtk_vbox_new(FALSE,2);
    
    notebook = gtk_notebook_new();
    gtk_box_pack_startC(vbox,notebook,TRUE,TRUE,0);

    layout_themes_pane(build_notebook_page(_("Themes Settings"),notebook));
    layout_settings_pane(build_notebook_page(_("Emerald Settings"),notebook));

    hbox = gtk_hbox_new(FALSE,2);
    gtk_box_pack_startC(vbox,hbox,FALSE,FALSE,0);
    QuitButton = gtk_button_new_from_stock(GTK_STOCK_QUIT);
    gtk_box_pack_endC(hbox,QuitButton,FALSE,FALSE,0);
    g_signal_connect(QuitButton,"clicked",G_CALLBACK(cb_quit),NULL);
 
    gtk_container_addC(GTK_CONTAINER (mainWindow), vbox);
}
int main (int argc, char * argv[])
{
    set_changed(FALSE);
    set_apply(FALSE);
    gchar *input_file=NULL;
    setlocale (LC_ALL, "");
    bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    textdomain (GETTEXT_PACKAGE);
    int install_file = 0;
    int loop_count = 0;

    while (loop_count < argc) {
	if (strcmp(argv[loop_count],"-i") == 0) {
	 //-i arg found so next option should be the file to install
	    if (loop_count + 1 < argc) { 	
	     input_file = argv[loop_count + 1]; 
             printf("File To Install %s\n", input_file); 
             install_file = 1;
	    } else {
	     printf("Usage: -i /file/to/install\n");  
	    }	
	}	
    loop_count++;
    }


    gtk_init (&argc, &argv);
#ifdef USE_DBUS
    if (!g_thread_supported()) g_thread_init(NULL);
    dbus_g_thread_init();
#endif

    g_mkdir_with_parents(g_strdup_printf("%s/.emerald/theme/",g_get_home_dir()),00755);
    g_mkdir_with_parents(g_strdup_printf("%s/.emerald/themes/",g_get_home_dir()),00755);

    init_key_files();
    
    mainWindow = gtk_window_new (GTK_WINDOW_TOPLEVEL);

    if (install_file == 1) {
    		GtkWidget *filechooserdialog1;
    		filechooserdialog1 = create_filechooserdialog1 (input_file);
    		gtk_widget_show (filechooserdialog1);
    }

#ifdef USE_DBUS
    setup_dbus();
#endif

    gtk_window_set_title (GTK_WINDOW (mainWindow), "Emerald Themer " VERSION);

    gtk_window_set_resizable (GTK_WINDOW (mainWindow), TRUE);
    gtk_window_set_default_size (GTK_WINDOW(mainWindow), 700, 500);

    g_signal_connect(G_OBJECT (mainWindow), "destroy", G_CALLBACK(cb_main_destroy), NULL);
    
    gtk_window_set_default_icon_from_file (PIXMAPS_DIR "/emerald-theme-manager-icon.png", NULL);

    gtk_container_set_border_widthC (GTK_CONTAINER (mainWindow), 5);

    layout_main_window();
    gtk_widget_show_all(mainWindow);

    refresh_theme_list(NULL);
    copy_from_defaults_if_needed();
    init_settings();
    set_changed(FALSE);
    set_apply(TRUE);

    gtk_main();
    return 0;
}
