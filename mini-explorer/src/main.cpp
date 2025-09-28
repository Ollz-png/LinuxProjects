#include <gtk/gtk.h>
#include <iostream>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <dirent.h>
#include <sys/stat.h>
#include <pwd.h>
#include <unistd.h>
#include <cstring>
#include <climits>
#include <cerrno>

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

// Format file size nicely
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

// Expand path shortcuts like ~, ~username, etc.
std::string expand_path(const char* path) {
    if (!path) return "";
    
    std::string expanded_path = path;
    
    // Handle ~ (home directory)
    if (expanded_path == "~" || expanded_path.substr(0, 2) == "~/") {
        struct passwd *pw = getpwuid(getuid());
        if (pw) {
            if (expanded_path == "~") {
                expanded_path = pw->pw_dir;
            } else {
                expanded_path = std::string(pw->pw_dir) + expanded_path.substr(1);
            }
        }
    }
    // Handle ~username
    else if (expanded_path[0] == '~' && expanded_path.find('/') != std::string::npos) {
        size_t slash_pos = expanded_path.find('/');
        std::string username = expanded_path.substr(1, slash_pos - 1);
        struct passwd *pw = getpwnam(username.c_str());
        if (pw) {
            expanded_path = std::string(pw->pw_dir) + expanded_path.substr(slash_pos);
        }
    }
    else if (expanded_path[0] == '~' && expanded_path.length() > 1 && expanded_path.find('/') == std::string::npos) {
        // Just ~username with no trailing path
        std::string username = expanded_path.substr(1);
        struct passwd *pw = getpwnam(username.c_str());
        if (pw) {
            expanded_path = pw->pw_dir;
        }
    }
    
    return expanded_path;
}

// Show error dialog
void show_error_dialog(GtkWidget *parent, const char *title, const char *message) {
    GtkWidget *dialog = gtk_message_dialog_new(
        GTK_WINDOW(parent),
        GTK_DIALOG_MODAL,
        GTK_MESSAGE_ERROR,
        GTK_BUTTONS_OK,
        "%s", title
    );
    
    gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), "%s", message);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

// Load directory contents
void load_directory(FileManagerData *data, const char *path) {
    DIR *dir = opendir(path);
    if (!dir) {
        // Show appropriate error message based on errno
        const char *error_msg;
        switch (errno) {
            case EACCES:
                error_msg = "Access denied. You don't have permission to view this folder.";
                break;
            case ENOENT:
                error_msg = "Folder not found or has been deleted.";
                break;
            case ENOTDIR:
                error_msg = "This is not a valid folder.";
                break;
            default:
                error_msg = "Unable to open this folder.";
                break;
        }
        
        // Create detailed error message with system error
        char detailed_error[512];
        std::snprintf(detailed_error, sizeof(detailed_error), 
                     "%s\n\nSystem error: %s (errno: %d)\nPath: %s", 
                     error_msg, strerror(errno), errno, path);
        
        show_error_dialog(data->window, "Cannot Open Folder", detailed_error);
        std::cout << "Failed to open directory: " << path << " (" << strerror(errno) << ")" << std::endl;
        return;
    }
    
    // Update current path
    std::strncpy(data->current_path, path, sizeof(data->current_path) - 1);
    data->current_path[sizeof(data->current_path) - 1] = '\0';
    
    // Update path entry
    gtk_entry_set_text(GTK_ENTRY(data->path_entry), path);
    
    // Clear existing entries
    gtk_list_store_clear(data->list_store);
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
        // Skip . but show .. for navigation
        if (std::strcmp(entry->d_name, ".") == 0) continue;
        
        GtkTreeIter iter;
        gtk_list_store_append(data->list_store, &iter);
        
        // Build full path for stat info
        char full_path[PATH_MAX];
        if (std::strcmp(path, "/") == 0) {
            // If we're at root, don't add extra slash
            std::snprintf(full_path, sizeof(full_path), "/%s", entry->d_name);
        } else {
            // Normal path concatenation
            std::snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
        }
        
        // Get file stats
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
            } else if (S_ISLNK(file_stat.st_mode)) {
                // Handle symbolic links - check what they point to
                struct stat link_stat;
                if (stat(full_path, &link_stat) == 0 && S_ISDIR(link_stat.st_mode)) {
                    icon_name = "folder-symbolic";  // Different icon for symlink dirs
                    g_free(size_str);
                    size_str = g_strdup("Folder (Link)");
                    is_directory = true;
                } else {
                    icon_name = "text-x-generic-symbolic";
                    g_free(size_str);
                    size_str = g_strdup("Link");
                }
            } else {
                g_free(size_str);
                size_str = g_strdup(format_file_size(file_stat.st_size));
            }
        } else {
            // If stat fails, fall back to d_type
            if (entry->d_type == DT_DIR) {
                icon_name = "folder";
                g_free(size_str);
                size_str = g_strdup("Folder");
                is_directory = true;
            }
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

// Handle double-click on items
void on_row_activated(GtkTreeView *tree_view, GtkTreePath *path, 
                     GtkTreeViewColumn *column, gpointer user_data) {
    FileManagerData *data = (FileManagerData*)user_data;
    GtkTreeModel *model = GTK_TREE_MODEL(data->list_store);
    GtkTreeIter iter;
    
    if (gtk_tree_model_get_iter(model, &iter, path)) {
        gboolean is_dir;
        gchar *file_path;
        
        gtk_tree_model_get(model, &iter, 
                          COL_IS_DIR, &is_dir,
                          COL_PATH, &file_path, 
                          -1);
        
        if (is_dir) {
            gchar *file_name;
            gtk_tree_model_get(model, &iter, COL_NAME, &file_name, -1);
            
            // Handle .. (parent directory) specially
            if (std::strcmp(file_name, "..") == 0) {
                // Navigate to parent directory
                char *last_slash = std::strrchr(data->current_path, '/');
                if (last_slash && last_slash != data->current_path) {
                    *last_slash = '\0';  // Truncate path
                    load_directory(data, data->current_path);
                } else if (std::strcmp(data->current_path, "/") != 0) {
                    load_directory(data, "/");
                }
            } else {
                // Regular directory navigation
                load_directory(data, file_path);
            }
            
            g_free(file_name);
        } else {
            std::cout << "Opening file: " << file_path << std::endl;
            // TODO: Open file with default application
            // For now, show a simple info dialog
            char message[512];
            std::snprintf(message, sizeof(message), "Would open: %s", file_path);
            
            GtkWidget *info_dialog = gtk_message_dialog_new(
                GTK_WINDOW(data->window),
                GTK_DIALOG_MODAL,
                GTK_MESSAGE_INFO,
                GTK_BUTTONS_OK,
                "File Selected"
            );
            gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(info_dialog), "%s", message);
            gtk_dialog_run(GTK_DIALOG(info_dialog));
            gtk_widget_destroy(info_dialog);
        }
        
        g_free(file_path);
    }
}

// Handle Enter key in path entry
void on_path_entry_activate(GtkEntry *entry, gpointer user_data) {
    FileManagerData *data = (FileManagerData*)user_data;
    const char *path = gtk_entry_get_text(entry);
    
    // Expand path shortcuts
    std::string expanded = expand_path(path);
    
    // Try to load the expanded path
    load_directory(data, expanded.c_str());
}

// Handle up/back button
void on_up_button_clicked(GtkButton *button, gpointer user_data) {
    FileManagerData *data = (FileManagerData*)user_data;
    
    // Get parent directory
    char *last_slash = strrchr(data->current_path, '/');
    if (last_slash && last_slash != data->current_path) {
        *last_slash = '\0';  // Truncate path
        load_directory(data, data->current_path);
    } else if (std::strcmp(data->current_path, "/") != 0) {
        load_directory(data, "/");
    }
}

// Handle home button
void on_home_button_clicked(GtkButton *button, gpointer user_data) {
    FileManagerData *data = (FileManagerData*)user_data;
    struct passwd *pw = getpwuid(getuid());
    if (pw) {
        load_directory(data, pw->pw_dir);
    }
}

// Setup the tree view with columns
void setup_tree_view(FileManagerData *data) {
    // Create list store
    data->list_store = gtk_list_store_new(N_COLUMNS,
                                         G_TYPE_STRING,  // Icon
                                         G_TYPE_STRING,  // Name
                                         G_TYPE_STRING,  // Size
                                         G_TYPE_BOOLEAN, // Is directory
                                         G_TYPE_STRING); // Full path
    
    // Set model
    gtk_tree_view_set_model(GTK_TREE_VIEW(data->tree_view), 
                           GTK_TREE_MODEL(data->list_store));
    
    // Icon column
    GtkCellRenderer *icon_renderer = gtk_cell_renderer_pixbuf_new();
    GtkTreeViewColumn *icon_column = gtk_tree_view_column_new_with_attributes(
        "", icon_renderer, "icon-name", COL_ICON, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(data->tree_view), icon_column);
    
    // Name column
    GtkCellRenderer *text_renderer = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *name_column = gtk_tree_view_column_new_with_attributes(
        "Name", text_renderer, "text", COL_NAME, NULL);
    gtk_tree_view_column_set_resizable(name_column, TRUE);
    gtk_tree_view_column_set_expand(name_column, TRUE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(data->tree_view), name_column);
    
    // Size column
    GtkCellRenderer *size_renderer = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *size_column = gtk_tree_view_column_new_with_attributes(
        "Size", size_renderer, "text", COL_SIZE, NULL);
    gtk_tree_view_column_set_resizable(size_column, TRUE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(data->tree_view), size_column);
    
    // Connect double-click signal
    g_signal_connect(data->tree_view, "row-activated", 
                    G_CALLBACK(on_row_activated), data);
}

// Create the main window
GtkWidget* create_main_window(FileManagerData *data) {
    data->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(data->window), "File Manager");
    gtk_window_set_default_size(GTK_WINDOW(data->window), 800, 600);
    
    // Main container
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(data->window), vbox);
    
    // Toolbar
    GtkWidget *toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_set_homogeneous(GTK_BOX(toolbar), FALSE);
    gtk_container_set_border_width(GTK_CONTAINER(toolbar), 5);
    gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 0);
    
    // Up button
    GtkWidget *up_button = gtk_button_new_with_label("↑ Up");
    g_signal_connect(up_button, "clicked", G_CALLBACK(on_up_button_clicked), data);
    gtk_box_pack_start(GTK_BOX(toolbar), up_button, FALSE, FALSE, 0);
    
    // Home button
    GtkWidget *home_button = gtk_button_new_with_label("Home");
    g_signal_connect(home_button, "clicked", G_CALLBACK(on_home_button_clicked), data);
    gtk_box_pack_start(GTK_BOX(toolbar), home_button, FALSE, FALSE, 0);
    
    // Path entry (like a terminal pwd)
    data->path_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(data->path_entry), "Enter path...");
    g_signal_connect(data->path_entry, "activate", G_CALLBACK(on_path_entry_activate), data);
    gtk_box_pack_start(GTK_BOX(toolbar), data->path_entry, TRUE, TRUE, 10);
    
    // Scrolled window for tree view
    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled, TRUE, TRUE, 0);
    
    // Tree view
    data->tree_view = gtk_tree_view_new();
    gtk_container_add(GTK_CONTAINER(scrolled), data->tree_view);
    
    // Setup tree view
    setup_tree_view(data);
    
    return data->window;
}

// Destroy callback
void on_destroy(GtkWidget *widget, gpointer user_data) {
    gtk_main_quit();
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);
    
    FileManagerData data = {0};
    
    // Create main window
    create_main_window(&data);
    
    // Connect destroy signal
    g_signal_connect(data.window, "destroy", G_CALLBACK(on_destroy), NULL);
    
    // Load home directory initially
    struct passwd *pw = getpwuid(getuid());
    if (pw) {
        load_directory(&data, pw->pw_dir);
    }
    
    // Show everything
    gtk_widget_show_all(data.window);
    
    // Main loop
    gtk_main();
    
    return 0;
}
