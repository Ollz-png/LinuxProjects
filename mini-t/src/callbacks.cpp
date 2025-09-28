#include "callbacks.hpp"
#include "terminal.hpp"
#include <fstream>
#include <string>
#include <iostream>

// New tab
void on_new_tab(GtkNotebook* notebook) {
    VteTerminal *term = spawn_terminal(GTK_WIDGET(notebook));

    GtkWidget *label_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    GtkWidget *label = gtk_label_new("Terminal");
    GtkWidget *close_btn = gtk_button_new_from_icon_name("window-close-symbolic", GTK_ICON_SIZE_MENU);
    gtk_widget_set_can_focus(close_btn, FALSE);

    gtk_box_pack_start(GTK_BOX(label_box), label, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(label_box), close_btn, FALSE, FALSE, 0);

    gtk_notebook_append_page(notebook, GTK_WIDGET(term), label_box);
    gtk_notebook_set_tab_reorderable(notebook, GTK_WIDGET(term), TRUE);
    gtk_notebook_set_current_page(notebook, gtk_notebook_get_n_pages(notebook)-1);

    g_signal_connect_swapped(close_btn, "clicked", G_CALLBACK(gtk_notebook_remove_page), notebook);
}

// File dialog response
void on_file_dialog_response(GtkDialog *dialog, gint response_id, gpointer user_data) {
    if (response_id != GTK_RESPONSE_ACCEPT) {
        gtk_widget_destroy(GTK_WIDGET(dialog));
        return;
    }

    GtkNotebook *notebook = GTK_NOTEBOOK(user_data);
    GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
    char *filename = gtk_file_chooser_get_filename(chooser);
    if (!filename) { gtk_widget_destroy(GTK_WIDGET(dialog)); return; }

    std::ifstream filestream(filename);
    if (!filestream.is_open()) { g_free(filename); gtk_widget_destroy(GTK_WIDGET(dialog)); return; }

    std::string command((std::istreambuf_iterator<char>(filestream)), std::istreambuf_iterator<char>());
    filestream.close();

    VteTerminal *term = spawn_terminal(GTK_WIDGET(notebook));
    vte_terminal_feed(term, command.c_str(), command.size());

    GtkWidget *label_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    char *basename = g_path_get_basename(filename);
    GtkWidget *label = gtk_label_new(basename);
    GtkWidget *close_btn = gtk_button_new_from_icon_name("window-close-symbolic", GTK_ICON_SIZE_MENU);
    gtk_widget_set_can_focus(close_btn, FALSE);

    gtk_box_pack_start(GTK_BOX(label_box), label, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(label_box), close_btn, FALSE, FALSE, 0);

    gtk_notebook_append_page(notebook, GTK_WIDGET(term), label_box);
    gtk_notebook_set_tab_reorderable(notebook, GTK_WIDGET(term), TRUE);
    gtk_notebook_set_current_page(notebook, gtk_notebook_get_n_pages(notebook)-1);

    g_signal_connect_swapped(close_btn, "clicked", G_CALLBACK(gtk_notebook_remove_page), notebook);

    g_free(filename);
    g_free(basename);
    gtk_widget_destroy(GTK_WIDGET(dialog));
}

// Open script
void on_open_script(GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    (void)action;    // unused
    (void)parameter; // unused

    GtkApplication *app = GTK_APPLICATION(user_data);
    GtkWindow *parent = GTK_WINDOW(gtk_application_get_active_window(app));

    GtkWidget *dialog = gtk_file_chooser_dialog_new(
        "Open Script", parent, GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Cancel", GTK_RESPONSE_CANCEL, "_Open", GTK_RESPONSE_ACCEPT, NULL
    );

    g_signal_connect(dialog, "response", G_CALLBACK(on_file_dialog_response), global_notebook);
    gtk_widget_show(dialog);
}

// About dialog
void on_about(GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    (void)action;    // unused
    (void)parameter; // unused

    GtkWindow *parent = GTK_WINDOW(gtk_application_get_active_window(GTK_APPLICATION(user_data)));
    GtkWidget *dialog = gtk_about_dialog_new();

    gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(dialog), "Mini-T Terminal Emulator");
    gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(dialog), "A minimal terminal built with GTK3 and VTE.");
    gtk_window_set_transient_for(GTK_WINDOW(dialog), parent);

    g_signal_connect(dialog, "response", G_CALLBACK(gtk_widget_destroy), NULL);
    gtk_widget_show(dialog);
}

// Activate application
void on_activate(GtkApplication *app, gpointer user_data) {
    (void)user_data; // unused

    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Mini-T");
    gtk_window_set_default_size(GTK_WINDOW(window), 900, 600);

    GtkWidget *headerbar = gtk_header_bar_new();
    gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(headerbar), TRUE);
    gtk_window_set_titlebar(GTK_WINDOW(window), headerbar);

    // File menu
    GMenu *file_menu = g_menu_new();
    g_menu_append(file_menu, "New Tab", "app.new_tab");
    g_menu_append(file_menu, "Open Script", "app.open_script");

    GtkWidget *file_button = gtk_menu_button_new();
    gtk_menu_button_set_menu_model(GTK_MENU_BUTTON(file_button), G_MENU_MODEL(file_menu));
    GtkWidget *file_label = gtk_label_new("File");
    gtk_container_add(GTK_CONTAINER(file_button), file_label);
    gtk_widget_show(file_label);
    gtk_header_bar_pack_start(GTK_HEADER_BAR(headerbar), file_button);

    // Help menu
    GMenu *help_menu = g_menu_new();
    g_menu_append(help_menu, "About", "app.about");

    GtkWidget *help_button = gtk_menu_button_new();
    gtk_menu_button_set_menu_model(GTK_MENU_BUTTON(help_button), G_MENU_MODEL(help_menu));
    GtkWidget *help_label = gtk_label_new("Help");
    gtk_container_add(GTK_CONTAINER(help_button), help_label);
    gtk_widget_show(help_label);
    gtk_header_bar_pack_end(GTK_HEADER_BAR(headerbar), help_button);

    // Notebook
    GtkWidget *notebook = gtk_notebook_new();
    gtk_widget_set_hexpand(notebook, TRUE);
    gtk_widget_set_vexpand(notebook, TRUE);
    gtk_container_add(GTK_CONTAINER(window), notebook);

    global_notebook = GTK_NOTEBOOK(notebook);
    on_new_tab(GTK_NOTEBOOK(notebook));

    gtk_widget_show(window);
}
