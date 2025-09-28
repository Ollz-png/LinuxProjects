#include "file_manager.hpp"
#include <pwd.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    FileManagerData data = {0};
    create_main_window(&data);
    g_signal_connect(data.window, "destroy", G_CALLBACK(on_destroy), NULL);

    struct passwd *pw = getpwuid(getuid());
    if (pw) load_directory(&data, pw->pw_dir);

    gtk_widget_show_all(data.window);
    gtk_main();
    return 0;
}
