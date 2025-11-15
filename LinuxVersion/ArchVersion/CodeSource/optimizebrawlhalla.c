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
    } else if (g_strcmp0(current_page, "installation") == 0) {
        gtk_stack_set_visible_child_name(GTK_STACK(stack), "install_path");
        gtk_widget_set_sensitive(next_button, TRUE);
        gtk_widget_set_sensitive(back_button, TRUE);
    }
    // Update button visibility
    gtk_widget_set_visible(finish_button, FALSE);
    gtk_widget_set_visible(next_button, TRUE);
}

static void on_cancel_button_clicked(GtkButton *button, gpointer user_data) {
    // Use g_application_quit to properly quit the GApplication
    if (app_instance) {
        g_application_quit(G_APPLICATION(app_instance));
    }
}

static void on_finish_button_clicked(GtkButton *button, gpointer user_data) {
    // Use g_application_quit to properly quit the GApplication
    if (app_instance) {
        g_application_quit(G_APPLICATION(app_instance));
    }
}

static void on_browse_path_clicked(GtkButton *button, gpointer user_data) {
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Select Install Folder",
                                                     GTK_WINDOW(window),
                                                     GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
                                                     "Cancel", GTK_RESPONSE_CANCEL,
                                                     "Select", GTK_RESPONSE_ACCEPT,
                                                     NULL);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        gchar *path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        gtk_entry_set_text(GTK_ENTRY(install_path_entry), path);
        if (install_path) g_free(install_path);
        install_path = g_strdup(path);
        g_free(path);
    }
    gtk_widget_destroy(dialog);
}

// --- Installation Logic ---

static void run_installation_thread(void) {
    GThread *install_thread = g_thread_new("install_thread", install_thread_func, NULL);
    if (install_thread) {
        g_thread_unref(install_thread); // Let it run in the background
    } else {
        g_printerr("Failed to create install thread.\n");
        // Update UI directly on main thread if thread creation fails
        update_install_progress(1.0, "Error: Thread Creation Failed");
    }
}

// --- Helper Functions for Installation ---
// This function attempts to install a package using pacman or yay.
static gboolean check_and_install_package(const char* package_name) {
    // Check if already installed using pacman
    char check_cmd[256];
    snprintf(check_cmd, sizeof(check_cmd), "pacman -Q %s >/dev/null 2>&1", package_name);
    if (system(check_cmd) == 0) {
        g_print("Package %s is already installed via pacman.\n", package_name);
        return TRUE;
    }

    // If not found via pacman, try yay (assuming it's available)
    char install_cmd[256];
    snprintf(install_cmd, sizeof(install_cmd), "yay -S --noconfirm %s", package_name);
    g_print("Attempting to install %s using yay...\n", package_name);
    int result = system(install_cmd);
    if (result == 0) {
        g_print("Successfully installed %s using yay.\n", package_name);
        return TRUE;
    }

    // If yay fails, try pacman directly (might need sudo privileges here too)
    snprintf(install_cmd, sizeof(install_cmd), "sudo pacman -S --noconfirm %s", package_name);
    g_print("Attempting to install %s using pacman...\n", package_name);
    result = system(install_cmd);
    if (result == 0) {
        g_print("Successfully installed %s using pacman.\n", package_name);
        return TRUE;
    }

    g_printerr("Failed to install package %s using pacman or yay.\n", package_name);
    return FALSE;
}

// This function copies the executable to the destination directory.
// It assumes the executable is named 'OptimizeBrawlhalla' and is in the same directory as the installer.
static gboolean copy_executable(const char* source, const char* destination) {
    char copy_cmd[512];
    snprintf(copy_cmd, sizeof(copy_cmd), "cp '%s' '%s'", source, destination);
    g_print("Copying executable: %s\n", copy_cmd);
    int result = system(copy_cmd);
    if (result == 0) {
        // Make the copied file executable
        char chmod_cmd[512];
        snprintf(chmod_cmd, sizeof(chmod_cmd), "chmod +x '%s'", destination);
        g_print("Setting executable permissions: %s\n", chmod_cmd);
        result = system(chmod_cmd);
        return result == 0;
    }
    return FALSE;
}

// This function creates a .desktop file for the application in ~/.local/share/applications
static gboolean create_desktop_entry(const char* install_dir) {
    char desktop_file_path[512];
    snprintf(desktop_file_path, sizeof(desktop_file_path), "%s/.local/share/applications/optimizebrawlhalla.desktop", g_get_home_dir());

    FILE *file = fopen(desktop_file_path, "w");
    if (!file) {
        g_printerr("Failed to create desktop file: %s\n", desktop_file_path);
        return FALSE;
    }

    fprintf(file, "[Desktop Entry]\n");
    fprintf(file, "Type=Application\n");
    fprintf(file, "Name=OptimizeBrawlhalla\n");
    fprintf(file, "Comment=Optimize Brawlhalla Performance\n");
    fprintf(file, "Exec=gnome-terminal -- bash -c 'sudo %s/OptimizeBrawlhalla --optimize-brawlhalla; read -p \"Press Enter to exit...\"'\n"); // Example command to run with terminal and prompt
    fprintf(file, "Icon=applications-games\n"); // Use a generic icon, or place your own
    fprintf(file, "Terminal=false\n"); // We handle terminal via Exec line
    fprintf(file, "Categories=Game;\n");

    fclose(file);
    g_print("Desktop entry created: %s\n", desktop_file_path);
    return TRUE;
}


static gpointer install_thread_func(gpointer data) {
    g_print("Starting installation thread...\n");
    update_install_progress(0.05, "Checking system requirements...");

    // 1. Check for and install pkg-config (needed for compilation if source is included)
    //    For the standalone executable, we might not need this, but GTK does.
    //    Let's keep it for the installer itself if it was compiled with pkg-config.
    //    We'll focus on the target executable dependencies later if needed.
    //    For now, let's just ensure the target directory can be created (which often requires basic tools).
    update_install_progress(0.15, "Checking basic tools...");

    // 2. Create installation directory
    if (install_path) {
        char mkdir_cmd[512];
        snprintf(mkdir_cmd, sizeof(mkdir_cmd), "mkdir -p '%s'", install_path);
        g_print("Creating install directory: %s\n", mkdir_cmd);
        int result = system(mkdir_cmd);
        if (result != 0) {
            update_install_progress(1.0, "Error: Failed to create install directory. Aborting.");
            return NULL;
        }
        update_install_progress(0.25, "Installation directory created.");
    } else {
        update_install_progress(1.0, "Error: No install path specified. Aborting.");
        return NULL;
    }

    // 3. Copy the OptimizeBrawlhalla executable to the install directory
    //    IMPORTANT: This assumes 'OptimizeBrawlhalla' is in the current working directory when the installer runs.
    //    You might need to adjust the source path based on how the installer is packaged.
    char source_executable[512];
    snprintf(source_executable, sizeof(source_executable), "./OptimizeBrawlhalla"); // Source path
    char destination_executable[512];
    snprintf(destination_executable, sizeof(destination_executable), "%s/OptimizeBrawlhalla", install_path);

    if (!copy_executable(source_executable, destination_executable)) {
        update_install_progress(1.0, "Error: Failed to copy executable. Aborting.");
        return NULL;
    }
    update_install_progress(0.50, "OptimizeBrawlhalla executable copied.");

    // 4. Create Desktop Entry
    if (!create_desktop_entry(install_path)) {
        g_printerr("Warning: Failed to create desktop entry.\n");
        // Don't abort installation just because of the desktop entry
    } else {
        update_install_progress(0.70, "Desktop entry created.");
    }

    // 5. Simulate remaining steps (Compilation, etc.)
    //    In a real scenario, these would be actual operations.
    g_usleep(500000); // Simulate work (0.5 seconds)
    update_install_progress(0.85, "Finalizing installation...");
    g_usleep(500000);
    update_install_progress(1.0, "Installation Complete!");

    install_complete = TRUE;

    // Signal the main thread to update UI
    g_idle_add(update_ui_after_install, NULL);

    return NULL;
}

static void update_install_progress(gdouble fraction, const char *text) {
    // This function is called from the install thread.
    // We need to update the UI from the main thread.
    // We use g_idle_add to schedule the update on the main thread.
    struct ProgressUpdateData *data = g_malloc(sizeof(struct ProgressUpdateData));
    data->frac = fraction;
    data->txt = g_strdup(text);
    // Use the function declared earlier
    g_idle_add((GSourceFunc)update_ui_progress, data);
}

// --- UI Update Functions (Must be called from main thread) ---

// This function must be defined AFTER the struct definition
static gboolean update_ui_progress(gpointer user_data) {
    struct ProgressUpdateData *data = (struct ProgressUpdateData*)user_data;
    if (install_progress_bar) {
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(install_progress_bar), data->frac);
        gtk_progress_bar_set_text(GTK_PROGRESS_BAR(install_progress_bar), data->txt);
    }
    if (status_label) {
        gtk_label_set_text(GTK_LABEL(status_label), data->txt);
    }
    g_free(data->txt);
    g_free(data);
    return G_SOURCE_REMOVE; // Run only once
}

static gboolean update_ui_after_install(gpointer user_data) {
    if (install_complete) {
        gtk_stack_set_visible_child_name(GTK_STACK(stack), "finish");
        gtk_widget_set_visible(finish_button, TRUE);
        gtk_widget_set_visible(next_button, FALSE);
        gtk_widget_set_visible(back_button, FALSE);
        gtk_widget_set_visible(cancel_button, FALSE);
        gtk_widget_set_sensitive(install_button, FALSE);
    }
    return G_SOURCE_REMOVE; // Run only once
}

// --- Main Application Setup ---

static void setup_installer_pages(void) {
    gtk_stack_add_named(GTK_STACK(stack), create_welcome_page(), "welcome");
    gtk_stack_add_named(GTK_STACK(stack), create_license_page(), "license");
    gtk_stack_add_named(GTK_STACK(stack), create_install_path_page(), "install_path");
    gtk_stack_add_named(GTK_STACK(stack), create_installation_page(), "installation");
    gtk_stack_add_named(GTK_STACK(stack), create_finish_page(), "finish");

    gtk_stack_set_visible_child_name(GTK_STACK(stack), "welcome");
}

static void activate(GtkApplication *app, gpointer user_data) {
    app_instance = app; // Store the application instance

    // --- Main Window ---
    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "OptimizeBrawlhalla Setup");
    gtk_window_set_default_size(GTK_WINDOW(window), 500, 400);
    gtk_window_set_resizable(GTK_WINDOW(window), FALSE); // Keep fixed size for installer

    // --- Main Vertical Box ---
    GtkWidget *main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(window), main_vbox);

    // --- Header (Optional) ---
    GtkWidget *header_label = gtk_label_new("");
    gtk_label_set_markup(GTK_LABEL(header_label), "<span font_weight=\"bold\" size=\"large\">OptimizeBrawlhalla Setup</span>");
    gtk_widget_set_halign(header_label, GTK_ALIGN_START);
    gtk_widget_set_valign(header_label, GTK_ALIGN_START);
    gtk_widget_set_margin_start(header_label, 10);
    gtk_widget_set_margin_top(header_label, 5);
    gtk_box_pack_start(GTK_BOX(main_vbox), header_label, FALSE, FALSE, 0);

    // --- Stack for Pages ---
    stack = gtk_stack_new();
    setup_installer_pages();
    gtk_box_pack_start(GTK_BOX(main_vbox), stack, TRUE, TRUE, 0);

    // --- Button Box ---
    GtkWidget *button_box = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_button_box_set_layout(GTK_BUTTON_BOX(button_box), GTK_BUTTONBOX_END); // Pack buttons to the right
    gtk_widget_set_margin_bottom(button_box, 10);
    gtk_widget_set_margin_end(button_box, 10);
    gtk_widget_set_margin_start(button_box, 10);

    // --- Buttons ---
    install_button = gtk_button_new_with_label("Install");
    g_signal_connect(install_button, "clicked", G_CALLBACK(on_install_button_clicked), NULL);
    gtk_box_pack_end(GTK_BOX(button_box), install_button, FALSE, FALSE, 0);

    finish_button = gtk_button_new_with_label("Finish");
    g_signal_connect(finish_button, "clicked", G_CALLBACK(on_finish_button_clicked), NULL);
    gtk_box_pack_end(GTK_BOX(button_box), finish_button, FALSE, FALSE, 0);
    gtk_widget_hide(finish_button); // Hide initially

    next_button = gtk_button_new_with_label("Next >");
    g_signal_connect(next_button, "clicked", G_CALLBACK(on_next_button_clicked), NULL);
    gtk_box_pack_end(GTK_BOX(button_box), next_button, FALSE, FALSE, 0);

    back_button = gtk_button_new_with_label("< Back");
    g_signal_connect(back_button, "clicked", G_CALLBACK(on_back_button_clicked), NULL);
    gtk_box_pack_end(GTK_BOX(button_box), back_button, FALSE, FALSE, 0);

    cancel_button = gtk_button_new_with_label("Cancel");
    g_signal_connect(cancel_button, "clicked", G_CALLBACK(on_cancel_button_clicked), NULL);
    gtk_box_pack_end(GTK_BOX(button_box), cancel_button, FALSE, FALSE, 0);

    gtk_box_pack_end(GTK_BOX(main_vbox), button_box, FALSE, FALSE, 0);

    // --- Show All Widgets ---
    gtk_widget_show_all(window);
}

// --- Main Function ---
int main(int argc, char **argv) {
    GtkApplication *app;
    int status;

    app = gtk_application_new("com.blastfire.installer", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    // Cleanup
    if (install_path) g_free(install_path);

    return status;
}
