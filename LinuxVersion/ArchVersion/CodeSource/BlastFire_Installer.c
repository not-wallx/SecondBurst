// BlastFire_Installer.c
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h> // For mkdir, stat
#include <errno.h>    // For errno

// --- Global Variables ---
static GtkWidget *window;
static GtkWidget *stack; // To manage different installer pages
static GtkWidget *install_progress_bar;
static GtkWidget *status_label;
static GtkWidget *install_button;
static GtkWidget *next_button;
static GtkWidget *back_button;
static GtkWidget *cancel_button;
static GtkWidget *finish_button;
static GtkWidget *install_path_entry;
static GtkWidget *install_path_button;
static char *install_path = NULL;
static gboolean install_complete = FALSE;
static GtkApplication *app_instance = NULL; // Keep a reference to the app

// --- Structure Definition (moved to global scope) ---
// Define the structure here so all functions can see it.
struct ProgressUpdateData {
    gdouble frac;
    gchar *txt;
};

// --- Function Prototypes ---
static void setup_installer_pages(void);
static GtkWidget* create_welcome_page(void);
static GtkWidget* create_license_page(void);
static GtkWidget* create_install_path_page(void);
static GtkWidget* create_installation_page(void);
static GtkWidget* create_finish_page(void);
static void on_install_button_clicked(GtkButton *button, gpointer user_data);
static void on_next_button_clicked(GtkButton *button, gpointer user_data);
static void on_back_button_clicked(GtkButton *button, gpointer user_data);
static void on_cancel_button_clicked(GtkButton *button, gpointer user_data);
static void on_finish_button_clicked(GtkButton *button, gpointer user_data);
static void on_browse_path_clicked(GtkButton *button, gpointer user_data);
static void run_installation_thread(void);
static gpointer install_thread_func(gpointer data);
static void update_install_progress(gdouble fraction, const char *text);
// Declare update_ui_progress BEFORE update_install_progress
static gboolean update_ui_progress(gpointer user_data);
static gboolean update_ui_after_install(gpointer user_data);
static void activate(GtkApplication *app, gpointer user_data);
static void cleanup_install_path(gchar *path);
// New helper functions for installation logic
static gboolean check_and_install_package(const char* package_name);
static gboolean copy_executable(const char* source, const char* destination);
static gboolean create_desktop_entry(const char* install_dir);

// --- Installer Page Creation ---

static GtkWidget* create_welcome_page(void) {
    GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(page), 50);

    GtkWidget *title_label = gtk_label_new("");
    gtk_label_set_markup(GTK_LABEL(title_label), "<span font_weight=\"bold\" size=\"x-large\">Welcome to OptimizeBrawlhalla Setup</span>");
    gtk_box_pack_start(GTK_BOX(page), title_label, FALSE, FALSE, 0);

    GtkWidget *description_label = gtk_label_new("\nThis setup will install OptimizeBrawlhalla, a performance optimizer for Brawlhalla, on your computer.\n\nClick \"Next\" to continue or \"Cancel\" to exit.");
    gtk_label_set_line_wrap(GTK_LABEL(description_label), TRUE);
    gtk_label_set_max_width_chars(GTK_LABEL(description_label), 50);
    gtk_box_pack_start(GTK_BOX(page), description_label, FALSE, FALSE, 0);

    return page;
}

static GtkWidget* create_license_page(void) {
    GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(page), 20);

    GtkWidget *title_label = gtk_label_new("");
    gtk_label_set_markup(GTK_LABEL(title_label), "<span font_weight=\"bold\" size=\"large\">License Agreement</span>");
    gtk_box_pack_start(GTK_BOX(page), title_label, FALSE, FALSE, 0);

    GtkWidget *license_text = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(license_text), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(license_text), FALSE);
    gtk_text_buffer_insert_at_cursor(gtk_text_view_get_buffer(GTK_TEXT_VIEW(license_text)),
                                     "MIT License\n\nCopyright (c) 2025 Bekkali (iodx2004)\n\n"
                                     "Permission is hereby granted, free of charge, to any person obtaining a copy\n"
                                     "of this software and associated documentation files (the \"Software\"), to deal\n"
                                     "in the Software without restriction, including without limitation the rights\n"
                                     "to use, copy, modify, merge, publish, distribute, sublicense, and/or sell\n"
                                     "copies of the Software, and to permit persons to whom the Software is\n"
                                     "furnished to do so, subject to the following conditions:\n\n"
                                     "The above copyright notice and this permission notice shall be included in all\n"
                                     "copies or substantial portions of the Software.\n\n"
                                     "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\n"
                                     "IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\n"
                                     "FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE\n"
                                     "AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER\n"
                                     "LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,\n"
                                     "OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE\n"
                                     "SOFTWARE.\n\n"
                                     "Do you accept the terms of this license agreement?", -1);

    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scroll), license_text);
    gtk_box_pack_start(GTK_BOX(page), scroll, TRUE, TRUE, 0);

    return page;
}

static GtkWidget* create_install_path_page(void) {
    GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(page), 20);

    GtkWidget *title_label = gtk_label_new("");
    gtk_label_set_markup(GTK_LABEL(title_label), "<span font_weight=\"bold\" size=\"large\">Choose Install Location</span>");
    gtk_box_pack_start(GTK_BOX(page), title_label, FALSE, FALSE, 0);

    GtkWidget *path_label = gtk_label_new("Install folder:");
    gtk_box_pack_start(GTK_BOX(page), path_label, FALSE, FALSE, 0);

    GtkWidget *path_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    install_path_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(install_path_entry), "/opt/optimizebrawlhalla"); // Default path
    install_path = g_strdup("/opt/optimizebrawlhalla");
    gtk_box_pack_start(GTK_BOX(path_hbox), install_path_entry, TRUE, TRUE, 0);

    install_path_button = gtk_button_new_with_label("Browse...");
    g_signal_connect(install_path_button, "clicked", G_CALLBACK(on_browse_path_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(path_hbox), install_path_button, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(page), path_hbox, FALSE, FALSE, 0);

    GtkWidget *note_label = gtk_label_new("\nSelect the folder where you want to install OptimizeBrawlhalla.");
    gtk_label_set_line_wrap(GTK_LABEL(note_label), TRUE);
    gtk_box_pack_start(GTK_BOX(page), note_label, FALSE, FALSE, 0);

    return page;
}

static GtkWidget* create_installation_page(void) {
    GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(page), 20);

    GtkWidget *title_label = gtk_label_new("");
    gtk_label_set_markup(GTK_LABEL(title_label), "<span font_weight=\"bold\" size=\"large\">Installing OptimizeBrawlhalla</span>");
    gtk_box_pack_start(GTK_BOX(page), title_label, FALSE, FALSE, 0);

    install_progress_bar = gtk_progress_bar_new();
    gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(install_progress_bar), TRUE);
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(install_progress_bar), "Ready");
    gtk_box_pack_start(GTK_BOX(page), install_progress_bar, FALSE, FALSE, 0);

    status_label = gtk_label_new("Preparing for installation...");
    gtk_box_pack_start(GTK_BOX(page), status_label, FALSE, FALSE, 0);

    GtkWidget *note_label = gtk_label_new("\nThe application is being installed. Please wait...");
    gtk_label_set_line_wrap(GTK_LABEL(note_label), TRUE);
    gtk_box_pack_start(GTK_BOX(page), note_label, FALSE, FALSE, 0);

    return page;
}

static GtkWidget* create_finish_page(void) {
    GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(page), 50);

    GtkWidget *title_label = gtk_label_new("");
    gtk_label_set_markup(GTK_LABEL(title_label), "<span font_weight=\"bold\" size=\"x-large\">OptimizeBrawlhalla Setup Complete</span>");
    gtk_box_pack_start(GTK_BOX(page), title_label, FALSE, FALSE, 0);

    GtkWidget *description_label = gtk_label_new("\nThe installation of OptimizeBrawlhalla has been completed successfully.\n\nClick \"Finish\" to exit the setup.");
    gtk_label_set_line_wrap(GTK_LABEL(description_label), TRUE);
    gtk_label_set_max_width_chars(GTK_LABEL(description_label), 50);
    gtk_box_pack_start(GTK_BOX(page), description_label, FALSE, FALSE, 0);

    return page;
}

// --- Button Callbacks ---

static void on_install_button_clicked(GtkButton *button, gpointer user_data) {
    gtk_stack_set_visible_child_name(GTK_STACK(stack), "installation");
    gtk_widget_set_sensitive(install_button, FALSE);
    gtk_widget_set_sensitive(next_button, FALSE);
    gtk_widget_set_sensitive(back_button, FALSE);
    run_installation_thread();
}

static void on_next_button_clicked(GtkButton *button, gpointer user_data) {
    const gchar *current_page = gtk_stack_get_visible_child_name(GTK_STACK(stack));
    if (g_strcmp0(current_page, "welcome") == 0) {
        gtk_stack_set_visible_child_name(GTK_STACK(stack), "license");
    } else if (g_strcmp0(current_page, "license") == 0) {
        gtk_stack_set_visible_child_name(GTK_STACK(stack), "install_path");
    } else if (g_strcmp0(current_page, "install_path") == 0) {
        gtk_stack_set_visible_child_name(GTK_STACK(stack), "installation");
        gtk_widget_set_sensitive(next_button, FALSE);
        gtk_widget_set_sensitive(back_button, FALSE);
        run_installation_thread();
    }
    // Update button sensitivity based on page
    if (g_strcmp0(gtk_stack_get_visible_child_name(GTK_STACK(stack)), "finish") == 0) {
        gtk_widget_set_visible(finish_button, TRUE);
        gtk_widget_set_visible(next_button, FALSE);
    }
}

static void on_back_button_clicked(GtkButton *button, gpointer user_data) {
    const gchar *current_page = gtk_stack_get_visible_child_name(GTK_STACK(stack));
    if (g_strcmp0(current_page, "license") == 0) {
        gtk_stack_set_visible_child_name(GTK_STACK(stack), "welcome");
    } else if (g_strcmp0(current_page, "install_path") == 0) {
        gtk_stack_set_visible_child_name(GTK_STACK(stack), "license");
    } else if (
