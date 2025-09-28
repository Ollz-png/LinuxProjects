#include "terminal.hpp"
#include <cstring>

bool splash_shown = false;
GtkNotebook* global_notebook = nullptr;

VteTerminal* spawn_terminal(GtkWidget *parent, bool splash) {
    VteTerminal *terminal = VTE_TERMINAL(vte_terminal_new());
    vte_terminal_set_scrollback_lines(terminal, 1000);

    PangoFontDescription *font = pango_font_description_from_string("Monospace 12");
    vte_terminal_set_font(terminal, font);
    pango_font_description_free(font);

    GdkRGBA fg, bg;
    gdk_rgba_parse(&fg, "rgb(242, 242, 242)");
    gdk_rgba_parse(&bg, "#2E3440");
    vte_terminal_set_colors(terminal, &fg, &bg, NULL, 0);
    vte_terminal_set_mouse_autohide(terminal, TRUE);

    struct passwd *pw = getpwuid(getuid());
    const char* shell = pw->pw_shell ? pw->pw_shell : "/bin/bash";
    char *argv_shell[] = {(char*)shell, nullptr};

    vte_terminal_spawn_async(
        terminal,
        VTE_PTY_DEFAULT,
        nullptr,
        argv_shell,
        nullptr,
        G_SPAWN_DEFAULT,
        nullptr, nullptr,
        nullptr,
        -1,
        nullptr,
        nullptr, nullptr
    );

    if (splash && !splash_shown) {
        const char* splash_lines[] = { "Mini-T Terminal - The terminal that can" };
        for (auto &line : splash_lines) {
            vte_terminal_feed(terminal, line, strlen(line));
            vte_terminal_feed(terminal, "\n", 1);
        }
        splash_shown = true;
    }

    gtk_widget_set_hexpand(GTK_WIDGET(terminal), TRUE);
    gtk_widget_set_vexpand(GTK_WIDGET(terminal), TRUE);

    return terminal;
}
