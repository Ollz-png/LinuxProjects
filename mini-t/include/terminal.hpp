#pragma once
#include <gtk/gtk.h>
#include <vte/vte.h>
#include <pwd.h>
#include <unistd.h>
#include <string>

VteTerminal* spawn_terminal(GtkWidget *parent, bool splash = true);
extern bool splash_shown;
extern GtkNotebook* global_notebook;
