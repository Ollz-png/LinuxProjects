#pragma once
#include <gtk/gtk.h>

// Use GSimpleAction, not GtkSimpleAction
void on_new_tab(GtkNotebook* notebook);
void on_file_dialog_response(GtkDialog *dialog, gint response_id, gpointer user_data);
void on_open_script(GSimpleAction *action, GVariant *parameter, gpointer user_data);
void on_about(GSimpleAction *action, GVariant *parameter, gpointer user_data);
void on_activate(GtkApplication *app, gpointer user_data);
