#include "terminal.hpp"
#include <cstring>
#include <pwd.h>
#include <vte/vte.h>

bool splash_shown = false;
GtkNotebook* global_notebook = nullptr;

VteTerminal* spawn_terminal(GtkWidget *parent, bool splash) {
    VteTerminal *terminal = VTE_TERMINAL(vte_terminal_new());
    vte_terminal_set_scrollback_lines(terminal, 1000);

    // Set font
    PangoFontDescription *font = pango_font_description_from_string("Monospace 12");
    vte_terminal_set_font(terminal, font);
    pango_font_description_free(font);

    // Nord-inspired colors
    GdkRGBA fg, bg, palette[16];
    gdk_rgba_parse(&fg, "#E5E9F0");     // normal text
    gdk_rgba_parse(&bg, "#1E2127");     // background

	gdk_rgba_parse(&palette[0], "#434C5E");  // black → slightly lighter
	gdk_rgba_parse(&palette[1], "#BF616A");  // red
	gdk_rgba_parse(&palette[2], "#A3BE8C");  // green
	gdk_rgba_parse(&palette[3], "#EBCB8B");  // yellow
	gdk_rgba_parse(&palette[4], "#81A1C1");  // blue
	gdk_rgba_parse(&palette[5], "#B48EAD");  // magenta
	gdk_rgba_parse(&palette[6], "#8FBCBB");  // cyan
	gdk_rgba_parse(&palette[7], "#E5E9F0");  // white
	gdk_rgba_parse(&palette[8], "#5E6579");  // bright black → lighter than before
	gdk_rgba_parse(&palette[9], "#BF616A");  // bright red
	gdk_rgba_parse(&palette[10], "#A3BE8C"); // bright green
	gdk_rgba_parse(&palette[11], "#EBCB8B"); // bright yellow
	gdk_rgba_parse(&palette[12], "#81A1C1"); // bright blue
	gdk_rgba_parse(&palette[13], "#B48EAD"); // bright magenta
	gdk_rgba_parse(&palette[14], "#8FBCBB"); // bright cyan
	gdk_rgba_parse(&palette[15], "#ECEFF4"); // bright white


    // Apply colors and palette
    vte_terminal_set_colors(terminal, &fg, &bg, palette, 16);
    vte_terminal_set_mouse_autohide(terminal, TRUE);

    // Spawn shell
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

    // Splash screen
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
