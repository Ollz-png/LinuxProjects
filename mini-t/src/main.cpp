#include "callbacks.hpp"
#include <gtk/gtk.h>

int main(int argc, char *argv[]) {
    GtkApplication *app = gtk_application_new("com.example.mini-t", G_APPLICATION_DEFAULT_FLAGS);

    // Actions
    GSimpleAction *new_tab_action = g_simple_action_new("new_tab", nullptr);
    g_signal_connect(new_tab_action, "activate", G_CALLBACK(on_new_tab), nullptr);
    g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(new_tab_action));

    GSimpleAction *open_script_action = g_simple_action_new("open_script", nullptr);
    g_signal_connect(open_script_action, "activate", G_CALLBACK(on_open_script), app);
    g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(open_script_action));

    GSimpleAction *about_action = g_simple_action_new("about", nullptr);
    g_signal_connect(about_action, "activate", G_CALLBACK(on_about), app);
    g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(about_action));

    g_signal_connect(app, "activate", G_CALLBACK(on_activate), nullptr);

    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}
