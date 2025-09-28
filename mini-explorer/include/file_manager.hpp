#pragma once
#include <gtk/gtk.h>
#include <string>
#include <climits>

typedef struct {
    GtkWidget *window;
    GtkWidget *tree_view;
    GtkListStore *list_store;
    GtkWidget *path_entry;
    char current_path[PATH_MAX];
} FileManagerData;

enum {
    COL_ICON,
    COL_NAME,
    COL_SIZE,
    COL_IS_DIR,
    COL_PATH,
    N_COLUMNS
};

// Utility
const char* format_file_size(off_t size);
std::string expand_path(const char* path);

// GUI helpers
void show_error_dialog(GtkWidget *parent, const char *title, const char *message);
void load_directory(FileManagerData *data, const char *path);
GtkWidget* create_main_window(FileManagerData *data);
void setup_tree_view(FileManagerData *data);

// Callbacks
void on_row_activated(GtkTreeView *tree_view, GtkTreePath *path,
                      GtkTreeViewColumn *column, gpointer user_data);
void on_path_entry_activate(GtkEntry *entry, gpointer user_data);
void on_up_button_clicked(GtkButton *button, gpointer user_data);
void on_home_button_clicked(GtkButton *button, gpointer user_data);
void on_destroy(GtkWidget *widget, gpointer user_data);
