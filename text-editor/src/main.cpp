// src/main.cpp
#include <gtk/gtk.h>
#include <gtksourceview/gtksource.h>
#include <vte/vte.h>

#include <string>
#include <vector>
#include <memory>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>

struct TabData {
    GtkWidget* scrolled;
    GtkSourceBuffer* buffer;
    GtkWidget* view;
    std::string path;
    bool dirty;
};

static std::vector<std::unique_ptr<TabData>> tabs;
static GtkWidget* notebook;
static GtkWidget* statusbar;
static guint status_ctx;
static GtkWidget* terminal; // VTE terminal
static GtkWidget* paned;     // vertical paned (top: notebook, bottom: terminal)

// Silence unused parameters
#define UNUSED(x) (void)(x)

static void update_status_for_buffer(TabData *t) {
    GtkTextIter iter;
    gtk_text_buffer_get_iter_at_mark(GTK_TEXT_BUFFER(t->buffer),
                                     &iter,
                                     gtk_text_buffer_get_insert(GTK_TEXT_BUFFER(t->buffer)));
    int line = gtk_text_iter_get_line(&iter) + 1;
    int col = gtk_text_iter_get_line_offset(&iter);
    std::string label = (t->dirty ? "*" : "") + (t->path.empty() ? "Untitled" : t->path) +
                        " — Ln " + std::to_string(line) + ", Col " + std::to_string(col);
    gtk_statusbar_pop(GTK_STATUSBAR(statusbar), status_ctx);
    gtk_statusbar_push(GTK_STATUSBAR(statusbar), status_ctx, label.c_str());
}

static void mark_tab_dirty(TabData *t, bool dirty) {
    if (t->dirty == dirty) return;
    t->dirty = dirty;

    gint page = gtk_notebook_page_num(GTK_NOTEBOOK(notebook), t->scrolled);
    if (page != -1) {
        const char *base = t->path.empty() ? "Untitled" : std::filesystem::path(t->path).filename().c_str();
        std::string lab = (dirty ? "*" : "") + std::string(base);
        GtkWidget *label = gtk_label_new(lab.c_str());
        gtk_notebook_set_tab_label(GTK_NOTEBOOK(notebook), t->scrolled, label);
        gtk_widget_show(label);
    }
    update_status_for_buffer(t);
}

static TabData* get_current_tab() {
    gint page = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));
    if (page < 0 || (size_t)page >= tabs.size()) return nullptr;
    return tabs[page].get();
}

static void on_buffer_changed(GtkTextBuffer* buf, gpointer user_data) {
    UNUSED(buf);
    TabData* t = (TabData*)user_data;
    mark_tab_dirty(t, true);
    update_status_for_buffer(t);
}

static void on_cursor_moved(GtkTextBuffer* buf, const GtkTextIter *location, GtkTextMark *mark, gpointer user_data) {
    UNUSED(buf);
    UNUSED(location);
    UNUSED(mark);
    TabData* t = (TabData*)user_data;
    update_status_for_buffer(t);
}

static GtkWidget* make_source_view(TabData* t) {
    GtkSourceBuffer *buf = GTK_SOURCE_BUFFER(gtk_source_buffer_new(nullptr));
    t->buffer = buf;

    GtkWidget *view = gtk_source_view_new_with_buffer(buf);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(view), GTK_WRAP_NONE);
    gtk_widget_set_hexpand(view, TRUE);
    gtk_widget_set_vexpand(view, TRUE);
    g_object_set(G_OBJECT(view), "show-line-numbers", TRUE, NULL);

    t->view = view;
    return view;
}

static void ensure_tab_label(TabData* t) {
    const char *base = t->path.empty() ? "Untitled" : std::filesystem::path(t->path).filename().c_str();
    std::string lab = (t->dirty ? "*" : "") + std::string(base);
    GtkWidget *label = gtk_label_new(lab.c_str());
    gtk_notebook_set_tab_label(GTK_NOTEBOOK(notebook), t->scrolled, label);
    gtk_widget_show(label);
}

static TabData* create_new_tab(const std::string &initial_text = "", const std::string &path = "") {
    auto tab = std::make_unique<TabData>();
    tab->path = path;
    tab->dirty = false;

    tab->view = make_source_view(tab.get());
    tab->scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(tab->scrolled), tab->view);
    gtk_widget_show_all(tab->scrolled);

    gint page = gtk_notebook_append_page(GTK_NOTEBOOK(notebook), tab->scrolled, NULL);
    tabs.push_back(std::move(tab));
    TabData* t = tabs.back().get();

    if (!initial_text.empty()) {
        gtk_text_buffer_set_text(GTK_TEXT_BUFFER(t->buffer), initial_text.c_str(), initial_text.size());
    }

    ensure_tab_label(t);

    g_signal_connect(t->buffer, "changed", G_CALLBACK(on_buffer_changed), t);
    g_signal_connect(t->buffer, "mark-set", G_CALLBACK(on_cursor_moved), t);

    gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), page);
    return t;
}

static bool load_file_to_tab(TabData* t, const std::string &path) {
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) return false;
    std::ostringstream ss;
    ss << ifs.rdbuf();
    std::string content = ss.str();
    gtk_text_buffer_set_text(GTK_TEXT_BUFFER(t->buffer), content.c_str(), content.size());
    t->path = path;
    mark_tab_dirty(t, false);
    ensure_tab_label(t);
    return true;
}

static bool save_tab_to_path(TabData* t, const std::string &path) {
    GtkTextIter start, end;
    gtk_text_buffer_get_start_iter(GTK_TEXT_BUFFER(t->buffer), &start);
    gtk_text_buffer_get_end_iter(GTK_TEXT_BUFFER(t->buffer), &end);
    gchar *text = gtk_text_buffer_get_text(GTK_TEXT_BUFFER(t->buffer), &start, &end, FALSE);

    std::ofstream ofs(path, std::ios::binary);
    if (!ofs) {
        g_free(text);
        return false;
    }
    ofs << text;
    ofs.close();
    g_free(text);
    t->path = path;
    mark_tab_dirty(t, false);
    ensure_tab_label(t);
    return true;
}

// File menu actions
static void action_new(GtkWidget*, gpointer) { create_new_tab(); }

static void action_open(GtkWidget*, gpointer) {
    GtkWidget *dialog = gtk_file_chooser_dialog_new(
        "Open File",
        GTK_WINDOW(gtk_widget_get_toplevel(notebook)),
        GTK_FILE_CHOOSER_ACTION_OPEN,
        ("_Cancel"), GTK_RESPONSE_CANCEL,
        ("_Open"), GTK_RESPONSE_ACCEPT,
        NULL);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        TabData* t = create_new_tab();
        if (!load_file_to_tab(t, filename)) {
            GtkWidget *err = gtk_message_dialog_new(
                GTK_WINDOW(gtk_widget_get_toplevel(notebook)),
                GTK_DIALOG_MODAL,
                GTK_MESSAGE_ERROR,
                GTK_BUTTONS_CLOSE,
                "Failed to open %s", filename);
            gtk_dialog_run(GTK_DIALOG(err));
            gtk_widget_destroy(err);
        }
        g_free(filename);
    }
    gtk_widget_destroy(dialog);
}

static void action_save_as(GtkWidget*, gpointer) {
    TabData* t = get_current_tab();
    if (!t) return;

    GtkWidget *dialog = gtk_file_chooser_dialog_new(
        "Save File As",
        GTK_WINDOW(gtk_widget_get_toplevel(notebook)),
        GTK_FILE_CHOOSER_ACTION_SAVE,
        ("_Cancel"), GTK_RESPONSE_CANCEL,
        ("_Save"), GTK_RESPONSE_ACCEPT,
        NULL);

    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        if (!save_tab_to_path(t, filename)) {
            GtkWidget *err = gtk_message_dialog_new(
                GTK_WINDOW(gtk_widget_get_toplevel(notebook)),
                GTK_DIALOG_MODAL,
                GTK_MESSAGE_ERROR,
                GTK_BUTTONS_CLOSE,
                "Failed to save %s", filename);
            gtk_dialog_run(GTK_DIALOG(err));
            gtk_widget_destroy(err);
        }
        g_free(filename);
    }
    gtk_widget_destroy(dialog);
}

static void action_save(GtkWidget*, gpointer) {
    TabData* t = get_current_tab();
    if (!t) return;
    if (t->path.empty()) action_save_as(nullptr, nullptr);
    else save_tab_to_path(t, t->path);
}

static void action_close_tab(GtkWidget*, gpointer) {
    gint page = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));
    if (page < 0) return;

    gtk_notebook_remove_page(GTK_NOTEBOOK(notebook), page);
    tabs.erase(tabs.begin() + page);

    TabData *cur = get_current_tab();
    if (cur) update_status_for_buffer(cur);
    else {
        gtk_statusbar_pop(GTK_STATUSBAR(statusbar), status_ctx);
        gtk_statusbar_push(GTK_STATUSBAR(statusbar), status_ctx, "No file");
    }
}

static void action_quit(GtkWidget*, gpointer) { gtk_main_quit(); }

// Modern VTE terminal
static void create_terminal() {
    terminal = vte_terminal_new();

	const char *shell = g_getenv("SHELL");
	if (!shell) shell = "bash";
	char *argv[] = { (char*)shell, nullptr }; // cast to char* for argv

    GPid child_pid;
    GError *error = nullptr;

    gboolean success = vte_terminal_spawn_sync(
        VTE_TERMINAL(terminal),
        VTE_PTY_DEFAULT,
        nullptr,
        argv,
        nullptr,
        G_SPAWN_DEFAULT,
        nullptr,
        nullptr,
        &child_pid,
        nullptr,
        &error
    );

    if (!success) {
        g_printerr("Failed to spawn terminal: %s\n", error->message);
        g_error_free(error);
    }

    gtk_widget_set_size_request(terminal, -1, 200);
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(GTK_WINDOW(window), 1000, 700);
    gtk_window_set_title(GTK_WINDOW(window), "Text Editor");

    GtkAccelGroup *accel = gtk_accel_group_new();
    gtk_window_add_accel_group(GTK_WINDOW(window), accel);

    GtkWidget *menubar = gtk_menu_bar_new();
    GtkWidget *filemenu = gtk_menu_new();
    GtkWidget *filemi = gtk_menu_item_new_with_label("File");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(filemi), filemenu);

	auto make_item = [&](const char* label, GCallback cb, const char* accel_key) {
    	GtkWidget *item = gtk_menu_item_new_with_label(label);
    	g_signal_connect(item, "activate", cb, nullptr);
    	if (accel_key) {
        	guint key = gdk_keyval_from_name(accel_key);
        	gtk_widget_add_accelerator(item, "activate", accel, key, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    	}
    	gtk_menu_shell_append(GTK_MENU_SHELL(filemenu), item);
	};


    make_item("New", G_CALLBACK(action_new), "n");
    make_item("Open", G_CALLBACK(action_open), "o");
    make_item("Save", G_CALLBACK(action_save), "s");
    make_item("Save As", G_CALLBACK(action_save_as), nullptr);
    make_item("Close Tab", G_CALLBACK(action_close_tab), "w");
    make_item("Quit", G_CALLBACK(action_quit), "q");

    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), filemi);

    paned = gtk_paned_new(GTK_ORIENTATION_VERTICAL);
    gtk_container_add(GTK_CONTAINER(window), paned);

    notebook = gtk_notebook_new();
    gtk_widget_set_vexpand(notebook, TRUE);
    gtk_paned_pack1(GTK_PANED(paned), notebook, TRUE, FALSE);

    create_terminal();
    gtk_paned_pack2(GTK_PANED(paned), terminal, FALSE, TRUE);
    gtk_widget_hide(terminal);

    statusbar = gtk_statusbar_new();
    status_ctx = gtk_statusbar_get_context_id(GTK_STATUSBAR(statusbar), "status");
    gtk_paned_pack2(GTK_PANED(paned), statusbar, FALSE, TRUE);

    create_new_tab();

    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), nullptr);
    gtk_widget_show_all(window);

    gtk_main();
    return 0;
}
