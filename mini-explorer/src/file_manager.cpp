#include "file_manager.hpp"
#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <dirent.h>
#include <sys/stat.h>
#include <pwd.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>

// -------------------- Utility functions --------------------
const char* format_file_size(off_t size) {
    static char buffer[32];
    if (size < 1024) {
        std::snprintf(buffer, sizeof(buffer), "%ld B", size);
    } else if (size < 1024 * 1024) {
        std::snprintf(buffer, sizeof(buffer), "%.1f KB", size / 1024.0);
    } else if (size < 1024 * 1024 * 1024) {
        std::snprintf(buffer, sizeof(buffer), "%.1f MB", size / (1024.0 * 1024.0));
    } else {
        std::snprintf(buffer, sizeof(buffer), "%.1f GB", size / (1024.0 * 1024.0 * 1024.0));
    }
    return buffer;
}

std::string expand_path(const char* path) {
    if (!path) return "";
    std::string expanded_path = path;

    if (expanded_path == "~" || expanded_path.substr(0, 2) == "~/") {
        struct passwd *pw = getpwuid(getuid());
        if (pw) {
            expanded_path = (expanded_path == "~") ? pw->pw_dir
                                                  : std::string(pw->pw_dir) + expanded_path.substr(1);
        }
    }
    else if (expanded_path[0] == '~') {
        size_t slash_pos = expanded_path.find('/');
        std::string username = expanded_path.substr(1, slash_pos - 1);
        struct passwd *pw = getpwnam(username.c_str());
        if (pw) {
            expanded_path = std::string(pw->pw_dir) + expanded_path.substr(slash_pos);
        }
    }
    return expanded_path;
}

// -------------------- GUI helpers --------------------
void show_error_dialog(GtkWidget *parent, const char *title, const char *message) {
    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(parent),
                                               GTK_DIALOG_MODAL,
                                               GTK_MESSAGE_ERROR,
                                               GTK_BUTTONS_OK,
                                               "%s", title);
    gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), "%s", message);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

void load_directory(FileManagerData *data, const char *path) {
    DIR *dir = opendir(path);
    if (!dir) {
        char detailed_error[512];
        std::snprintf(detailed_error, sizeof(detailed_error),
                     "Cannot open folder: %s\nSystem error: %s (errno: %d)",
                     path, strerror(errno), errno);
        show_error_dialog(data->window, "Cannot Open Folder", detailed_error);
        return;
    }

    std::strncpy(data->current_path, path, sizeof(data->current_path)-1);
    data->current_path[sizeof(data->current_path)-1] = '\0';
    gtk_entry_set_text(GTK_ENTRY(data->path_entry), path);
    gtk_list_store_clear(data->list_store);

    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (std::strcmp(entry->d_name, ".") == 0) continue;

        GtkTreeIter iter;
        gtk_list_store_append(data->list_store, &iter);

        char full_path[PATH_MAX];
        if (std::strcmp(path, "/") == 0) {
            std::snprintf(full_path, sizeof(full_path), "/%s", entry->d_name);
        } else {
            std::snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
        }

        struct stat file_stat;
        const char *icon_name = "text-x-generic";
        char *size_str = g_strdup("--");
        bool is_directory = false;

        if (stat(full_path, &file_stat) == 0) {
            if (S_ISDIR(file_stat.st_mode)) {
                icon_name = "folder";
                g_free(size_str);
                size_str = g_strdup("Folder");
                is_directory = true;
            } else {
                g_free(size_str);
                size_str = g_strdup(format_file_size(file_stat.st_size));
            }
        } else if (entry->d_type == DT_DIR) {
            icon_name = "folder";
            g_free(size_str);
            size_str = g_strdup("Folder");
            is_directory = true;
        }

        gtk_list_store_set(data->list_store, &iter,
                           COL_ICON, icon_name,
                           COL_NAME, entry->d_name,
                           COL_SIZE, size_str,
                           COL_IS_DIR, is_directory,
                           COL_PATH, full_path,
                           -1);
        g_free(size_str);
    }
    closedir(dir);
}

// -------------------- Callbacks --------------------
void on_row_activated(GtkTreeView *tree_view, GtkTreePath *path,
                      GtkTreeViewColumn *column, gpointer user_data) {
    FileManagerData *data = (FileManagerData*)user_data;
    GtkTreeModel *model = GTK_TREE_MODEL(data->list_store);
    GtkTreeIter iter;

    if (!gtk_tree_model_get_iter(model, &iter, path)) return;

    gboolean is_dir;
    gchar *file_path;
    gtk_tree_model_get(model, &iter,
                       COL_IS_DIR, &is_dir,
                       COL_PATH, &file_path,
                       -1);

    if (is_dir) load_directory(data, file_path);
    else {
        char message[512];
        std::snprintf(message, sizeof(message), "Would open: %s", file_path);
        GtkWidget *info_dialog = gtk_message_dialog_new(GTK_WINDOW(data->window),
                                                        GTK_DIALOG_MODAL,
                                                        GTK_MESSAGE_INFO,
                                                        GTK_BUTTONS_OK,
                                                        "File Selected");
        gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(info_dialog), "%s", message);
        gtk_dialog_run(GTK_DIALOG(info_dialog));
        gtk_widget_destroy(info_dialog);
    }

    g_free(file_path);
}

void on_path_entry_activate(GtkEntry *entry, gpointer user_data) {
    FileManagerData *data = (FileManagerData*)user_data;
    std::string path = expand_path(gtk_entry_get_text(entry));
    load_directory(data, path.c_str());
}

void on_up_button_clicked(GtkButton *button, gpointer user_data) {
    FileManagerData *data = (FileManagerData*)user_data;
    char *last_slash = strrchr(data->current_path, '/');
    if (last_slash && last_slash != data->current_path) {
        *last_slash = '\0';
        load_directory(data, data->current_path);
    } else load_directory(data, "/");
}

void on_home_button_clicked(GtkButton *button, gpointer user_data) {
    FileManagerData *data = (FileManagerData*)user_data;
    struct passwd *pw = getpwuid(getuid());
    if (pw) load_directory(data, pw->pw_dir);
}

void on_destroy(GtkWidget *widget, gpointer user_data) {
    gtk_main_quit();
}

// -------------------- GUI Setup --------------------
void setup_tree_view(FileManagerData *data) {
    data->list_store = gtk_list_store_new(N_COLUMNS,
                                         G_TYPE_STRING,
                                         G_TYPE_STRING,
                                         G_TYPE_STRING,
                                         G_TYPE_BOOLEAN,
                                         G_TYPE_STRING);

    gtk_tree_view_set_model(GTK_TREE_VIEW(data->tree_view),
                            GTK_TREE_MODEL(data->list_store));

    GtkCellRenderer *icon_renderer = gtk_cell_renderer_pixbuf_new();
    GtkTreeViewColumn *icon_column = gtk_tree_view_column_new_with_attributes(
        "", icon_renderer, "icon-name", COL_ICON, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(data->tree_view), icon_column);

    GtkCellRenderer *text_renderer = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *name_column = gtk_tree_view_column_new_with_attributes(
        "Name", text_renderer, "text", COL_NAME, NULL);
    gtk_tree_view_column_set_resizable(name_column, TRUE);
    gtk_tree_view_column_set_expand(name_column, TRUE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(data->tree_view), name_column);

    GtkCellRenderer *size_renderer = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *size_column = gtk_tree_view_column_new_with_attributes(
        "Size", size_renderer, "text", COL_SIZE, NULL);
    gtk_tree_view_column_set_resizable(size_column, TRUE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(data->tree_view), size_column);

    g_signal_connect(data->tree_view, "row-activated",
                     G_CALLBACK(on_row_activated), data);
}

GtkWidget* create_main_window(FileManagerData *data) {
    data->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(data->window), "File Manager");
    gtk_window_set_default_size(GTK_WINDOW(data->window), 800, 600);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(data->window), vbox);

    GtkWidget *toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_set_homogeneous(GTK_BOX(toolbar), FALSE);
    gtk_container_set_border_width(GTK_CONTAINER(toolbar), 5);
    gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 0);

    GtkWidget *up_button = gtk_button_new_with_label("↑ Up");
    g_signal_connect(up_button, "clicked", G_CALLBACK(on_up_button_clicked), data);
    gtk_box_pack_start(GTK_BOX(toolbar), up_button, FALSE, FALSE, 0);

    GtkWidget *home_button = gtk_button_new_with_label("Home");
    g_signal_connect(home_button, "clicked", G_CALLBACK(on_home_button_clicked), data);
    gtk_box_pack_start(GTK_BOX(toolbar), home_button, FALSE, FALSE, 0);

    data->path_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(data->path_entry), "Enter path...");
    g_signal_connect(data->path_entry, "activate", G_CALLBACK(on_path_entry_activate), data);
    gtk_box_pack_start(GTK_BOX(toolbar), data->path_entry, TRUE, TRUE, 10);

    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled, TRUE, TRUE, 0);

    data->tree_view = gtk_tree_view_new();
    gtk_container_add(GTK_CONTAINER(scrolled), data->tree_view);

    setup_tree_view(data);

    return data->window;
}
