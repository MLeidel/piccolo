/*
* piccolo.c
* Small Text Editor
* Linux GTK3
* Michael Leidel
* Feb 2021
* NOTE: winmet.txt and piccolo.glade files must
* reside on /home/USER/.config/piccolo/ directory
*/
#include <gtk/gtk.h>
#include "stralt.h"

// GtkWidget Pointers go here
// GtkWidget *g_;
GtkWidget *g_fc_dlg;
GtkWidget *g_about_dlg;
GtkWidget *g_txbuf;
GtkWidget *g_tview;
GtkWidget *g_window;
GtkWidget *g_about;
GtkWidget *g_msgdlg;

FILE *fh;
char fbuf[512];
int fc_action = 2;  // 0=open, 1=Save As, 2=New(Save As), 3=command line
char winmet_path[128] = "\0";
char glade_path[128] = "\0";
int modified = 0;


int main(int argc, char *argv[]) {
    GtkBuilder      *builder;
    GtkWidget       *window1;
    gint             w_top;
    gint             w_left;
    gint             w_width;
    gint             w_height;

    gtk_init(&argc, &argv);

    sprintf(winmet_path, "%s/.config/piccolo/%s", getenv("HOME"), "winmet.txt");
    sprintf(glade_path, "%s/.config/piccolo/%s", getenv("HOME"), "piccolo.glade");

    builder = gtk_builder_new();
    gtk_builder_add_from_file (builder, glade_path, NULL);
    //gtk_builder_new_from_string (glade_layout, -1);

    window1 = GTK_WIDGET(gtk_builder_get_object(builder, "window1"));
    gtk_builder_connect_signals(builder, NULL);

    // set global GtkWidget Pointers to their builder objects
    // g_ = GTK_WIDGET(gtk_builder_get_object(builder, "WIDGET_ID"));
    g_fc_dlg = GTK_WIDGET(gtk_builder_get_object(builder, "fcdlg"));
    g_msgdlg = GTK_WIDGET(gtk_builder_get_object(builder, "msgdlg"));
    g_about_dlg = GTK_WIDGET(gtk_builder_get_object(builder, "dlg_about"));
    g_txbuf = GTK_WIDGET(gtk_builder_get_object(builder, "textbuff"));
    g_tview = GTK_WIDGET(gtk_builder_get_object(builder, "textview"));
    g_window = window1;
    g_about = GTK_WIDGET(gtk_builder_get_object(builder, "dlg_about"));

    g_object_unref(builder);

    // load the window metrics
    char line[64];
    fh = open_for_read(winmet_path);
    fgets(line, 64, fh);
    fclose(fh);
    removen(line);  // remove new line character
    fields(line, ",");
    w_left      = atol(_fields[0]);
    w_top       = atol(_fields[1]);
    w_width     = atol(_fields[2]);
    w_height    = atol(_fields[3]);

    gtk_widget_show(window1);
    gtk_window_move(GTK_WINDOW(window1), w_left, w_top);  // set metrics ...
    gtk_window_resize(GTK_WINDOW(window1), w_width, w_height);

    if (argc > 1) {  // load file from command line
        strcpy(fbuf, argv[1]);
        // read the file and insert it into the textview
        fh = open_for_read(fbuf);
        fseek(fh, 0, SEEK_END);
        long fsize = ftell(fh);
        fseek(fh, 0, SEEK_SET);
        char *string = malloc(fsize + 1);
        fread(string, 1, fsize, fh);
        fclose(fh);
        string[fsize] = 0;
        gtk_text_buffer_set_text(GTK_TEXT_BUFFER(g_txbuf), string, -1);
        free(string);
        gtk_window_set_title(GTK_WINDOW(g_window), fbuf);
        fc_action = 3;
        gtk_text_buffer_set_modified (GTK_TEXT_BUFFER(g_txbuf) , FALSE);
        modified = 0;
    }

    gtk_main();
    return 0;
}


// callback functions for GtkWidget events


// called when window is closed
void on_window1_delete_event() {
    FILE *fh;
    gint w_top;
    gint w_left;
    gint w_width;
    gint w_height;

    gtk_window_get_position (GTK_WINDOW(g_window), &w_left, &w_top);
    gtk_window_get_size (GTK_WINDOW(g_window), &w_width, &w_height);
    fh = open_for_write(winmet_path);
    fprintf(fh, "%d,%d,%d,%d\n", w_left, w_top, w_width, w_height);
    fclose(fh);
    gtk_main_quit();
}

// User selected "Quit" from menu
void on_mu_quit_activate() {
    if (modified == 1) {
        gtk_dialog_run(GTK_DIALOG(g_msgdlg));
    }
    on_window1_delete_event();
}


// Clear Editor contents and set to New(Save As)
void on_mu_new_activate() {
    fc_action = 2;
     gtk_text_buffer_set_text(GTK_TEXT_BUFFER(g_txbuf), "", -1);
     gtk_window_set_title(GTK_WINDOW(g_window), "Piccolo");
}


// Select a file to open
void on_mu_open_activate() {
    if (modified == 1) {
        // Not saving altered textbuffer !
    }
    fc_action = 0;
    gtk_dialog_run(GTK_DIALOG(g_fc_dlg));
}


// Just close the FileChooserDialog
void on_btn_fc_cancel_clicked() {
    gtk_widget_hide(g_fc_dlg);
}


// Signal the dialog to Hide not Destroy
void on_fcdlg_delete_event() {
    gtk_widget_hide_on_delete(g_fc_dlg);
}

void on_dlg_about_delete_event() {
    gtk_widget_hide_on_delete(g_about);
}

// Show FileChooserDialog for Save As
void on_mu_saveas_activate() {
    fc_action = 1;  // signal activate handler to process "saveas"
    gtk_file_chooser_set_action (GTK_FILE_CHOOSER(g_fc_dlg),
                                 GTK_FILE_CHOOSER_ACTION_SAVE);
    gtk_dialog_run(GTK_DIALOG(g_fc_dlg));
}


// Save the current editor content
void on_mu_save_activate() {
    if (fc_action == 2) {
        on_mu_saveas_activate();  // save after "New"
        return;
    }

    GtkTextIter start, end;
    GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW(g_tview));
    gchar *text;

    gtk_text_buffer_get_bounds (buffer, &start, &end);
    text = gtk_text_buffer_get_text (buffer, &start, &end, FALSE);
    //printf("%s\n", text);
    fh = open_for_write(fbuf);
    fprintf(fh, "%s", text);
    fclose(fh);
    g_free(text);
    gtk_text_buffer_set_modified (buffer , FALSE);
    modified = 0;
}


// User done with filechooser, now process user's request
void on_btn_fc_ok_clicked() {

    strcpy(fbuf, gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(g_fc_dlg)));

    gtk_window_set_title(GTK_WINDOW(g_window), fbuf);

    if (fbuf == NULL) {
        gtk_widget_hide(g_fc_dlg);  // user cancelled filechooser
        return;
    }
    if (fc_action == 0) {
        // read the file and insert it into the textview
        fh = open_for_read(fbuf);
        fseek(fh, 0, SEEK_END);
        long fsize = ftell(fh);
        fseek(fh, 0, SEEK_SET);
        char *string = malloc(fsize + 1);
        fread(string, 1, fsize, fh);
        fclose(fh);
        string[fsize] = 0;
        gtk_text_buffer_set_text(GTK_TEXT_BUFFER(g_txbuf), string, -1);
        free(string);
        gtk_text_buffer_set_modified (GTK_TEXT_BUFFER(g_txbuf) , FALSE);
        modified = 0;
    } else {
        // get text from textview and save it to a file
        on_mu_save_activate();
    }
    gtk_widget_hide(g_fc_dlg);
}

// User double clicked to the selected file
void on_fcdlg_file_activated() {
    on_btn_fc_ok_clicked();
}

void on_mu_about_activate() {
    gtk_dialog_run(GTK_DIALOG(g_about));
}

void on_dlg_about_response() {
    gtk_widget_hide(g_about);
}

void on_textbuff_modified_changed() {
    modified = 1;
}

void on_btn_msgdlg_cancel_clicked() {
    gtk_widget_hide(g_msgdlg);
}

void on_btn_msgdlg_ok_clicked() {
    on_mu_save_activate();
    gtk_widget_hide(g_msgdlg);
}
