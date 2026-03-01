#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static GtkWidget *entry;
static GtkWidget *list_box;
static GtkWidget *main_window = NULL;
static gchar *paths_file = NULL;
static gchar *pos_file = NULL;

static void open_editor_for_path(const gchar *path) {
    if (!path || *path == '\0') {
        return;
    }

    if (!g_file_test(path, G_FILE_TEST_EXISTS)) {
        return;
    }

    GError *error = NULL;
    gboolean need_root = (access(path, W_OK) != 0);

    if (need_root) {
        const gchar *display = g_getenv("DISPLAY");
        const gchar *xauth = g_getenv("XAUTHORITY");

        gchar *env_display = NULL;
        gchar *env_xauth = NULL;

        if (display && *display) {
            env_display = g_strdup_printf("DISPLAY=%s", display);
        } else {
            env_display = g_strdup("DISPLAY=:0");
        }

        if (xauth && *xauth) {
            env_xauth = g_strdup_printf("XAUTHORITY=%s", xauth);
        }

        if (env_xauth) {
            gchar *argv[] = { "pkexec", "env", env_display, env_xauth, "gedit", (gchar *)path, NULL };
            if (!g_spawn_async(NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, &error)) {
                g_clear_error(&error);
            }
        } else {
            gchar *argv[] = { "pkexec", "env", env_display, "gedit", (gchar *)path, NULL };
            if (!g_spawn_async(NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, &error)) {
                g_clear_error(&error);
            }
        }

        g_free(env_display);
        g_free(env_xauth);
    } else {
        gchar *argv[] = { "gedit", (gchar *)path, NULL };
        if (!g_spawn_async(NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, &error)) {
            g_clear_error(&error);
        }
    }
}

static void open_file_from_folder(const gchar *folder) {
    if (!folder || *folder == '\0') {
        return;
    }

    gchar *base = NULL;
    if (g_file_test(folder, G_FILE_TEST_IS_DIR)) {
        base = g_strdup(folder);
    } else {
        base = g_path_get_dirname(folder);
    }

    GtkWidget *dialog = gtk_file_chooser_dialog_new(
        "파일 선택",
        GTK_WINDOW(main_window),
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "_취소", GTK_RESPONSE_CANCEL,
        "_열기", GTK_RESPONSE_ACCEPT,
        NULL);

    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), base);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        gchar *file = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        open_editor_for_path(file);
        g_free(file);
    }

    g_free(base);
    gtk_widget_destroy(dialog);
}

static void add_path(const gchar *text) {
    if (!text) {
        return;
    }

    gchar *copy = g_strdup(text);
    g_strstrip(copy);
    if (*copy == '\0') {
        g_free(copy);
        return;
    }

    GtkWidget *row = gtk_list_box_row_new();
    GtkWidget *label = gtk_label_new(copy);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_container_add(GTK_CONTAINER(row), label);

    gtk_list_box_insert(GTK_LIST_BOX(list_box), row, -1);
    gtk_widget_show_all(row);

    g_free(copy);
}

static void on_row_activated(GtkListBox *box, GtkListBoxRow *row, gpointer user_data) {
    GtkWidget *child = gtk_bin_get_child(GTK_BIN(row));
    if (!GTK_IS_LABEL(child)) {
        return;
    }

    const gchar *text = gtk_label_get_text(GTK_LABEL(child));
    open_file_from_folder(text);
}

static gboolean on_list_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    if (event->button != 1 || event->type != GDK_2BUTTON_PRESS) {
        return FALSE;
    }

    GtkListBox *box = GTK_LIST_BOX(widget);
    GtkListBoxRow *row = gtk_list_box_get_row_at_y(box, (gint)event->y);
    if (!row) {
        return FALSE;
    }

    GtkWidget *child = gtk_bin_get_child(GTK_BIN(row));
    if (!GTK_IS_LABEL(child)) {
        return FALSE;
    }

    const gchar *text = gtk_label_get_text(GTK_LABEL(child));
    open_file_from_folder(text);

    return TRUE;
}

static void save_paths(void) {
    if (!paths_file || !list_box) {
        return;
    }

    FILE *fp = fopen(paths_file, "w");
    if (!fp) {
        return;
    }

    GList *children = gtk_container_get_children(GTK_CONTAINER(list_box));
    for (GList *l = children; l != NULL; l = l->next) {
        GtkWidget *row = GTK_WIDGET(l->data);
        GtkWidget *child = gtk_bin_get_child(GTK_BIN(row));
        if (!GTK_IS_LABEL(child)) {
            continue;
        }
        const gchar *text = gtk_label_get_text(GTK_LABEL(child));
        if (text && *text) {
            fprintf(fp, "%s\n", text);
        }
    }

    g_list_free(children);
    fclose(fp);
}

static void load_paths(void) {
    if (!paths_file) {
        return;
    }

    FILE *fp = fopen(paths_file, "r");
    if (!fp) {
        return;
    }

    char line[1024];
    while (fgets(line, sizeof(line), fp)) {
        size_t len = strlen(line);
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
            line[--len] = '\0';
        }
        if (len > 0) {
            add_path(line);
        }
    }

    fclose(fp);
}

static gboolean on_window_delete(GtkWidget *widget, GdkEvent *event, gpointer user_data) {
    save_paths();

    if (pos_file && GTK_IS_WINDOW(widget)) {
        gint x, y;
        gtk_window_get_position(GTK_WINDOW(widget), &x, &y);

        FILE *fp = fopen(pos_file, "w");
        if (fp) {
            fprintf(fp, "%d %d\n", x, y);
            fclose(fp);
        }
    }

    return FALSE;
}

static void on_help_clicked(GtkButton *button, gpointer user_data) {
    if (!GTK_IS_WINDOW(main_window)) {
        return;
    }

    const gchar *msg =
        "사용 방법\n"
        "\n"
        "1. 폴더 경로를 직접 입력하거나 '폴더 선택...' 버튼으로 폴더를 추가합니다.\n"
        "   - 추가된 폴더 목록은 종료해도 자동으로 저장됩니다.\n"
        "   - 저장 위치는 홈 폴더의 .gtk_ford_paths 파일\n"
        "\n"
        "2. 목록에서 위치정보을 클릭하면 해당 폴더 기준으로 파일 선택 창이 열립니다.\n"
        "   - 원하는 파일을 선택하면 gedit로 파일이 열립니다.\n"
        "\n"
        "3. 선택한 파일에 관리자 권한이 필요하면\n"
        "   - gedit를 열기 전에 관리자 비밀번호 입력 창이 먼저 나타납니다.\n"
        "\n"
        "4. 입력 후 추가 버튼을 누르면 위치 정보가 입력됩니다.\n";

    GtkWidget *dialog = gtk_message_dialog_new(
        GTK_WINDOW(main_window),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_MESSAGE_INFO,
        GTK_BUTTONS_CLOSE,
        "%s",
        msg);

    gtk_window_set_title(GTK_WINDOW(dialog), "도움말");
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

static void on_add_clicked(GtkButton *button, gpointer user_data) {
    const gchar *text = gtk_entry_get_text(GTK_ENTRY(entry));
    add_path(text);
    gtk_entry_set_text(GTK_ENTRY(entry), "");
}

static void on_add_from_fs_clicked(GtkButton *button, gpointer user_data) {
    GtkWidget *parent = GTK_WIDGET(user_data);

    GtkWidget *dialog = gtk_file_chooser_dialog_new(
        "폴더 선택",
        GTK_WINDOW(parent),
        GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
        "_취소", GTK_RESPONSE_CANCEL,
        "_선택", GTK_RESPONSE_ACCEPT,
        NULL);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        gchar *folder = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        add_path(folder);
        g_free(folder);
    }

    gtk_widget_destroy(dialog);
}

static void on_activate(GtkApplication *app, gpointer user_data) {
    GtkWidget *window = gtk_application_window_new(app);
    main_window = window;
    gtk_window_set_title(GTK_WINDOW(window), "폴더/파일 위치 편집기");
    gtk_window_set_default_size(GTK_WINDOW(window), 600, 400);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 5);

    entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "폴더 경로를 직접 입력하세요...");
    gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 0);

    GtkWidget *add_button = gtk_button_new_with_label("추가");
    gtk_box_pack_start(GTK_BOX(hbox), add_button, FALSE, FALSE, 0);

    GtkWidget *browse_button = gtk_button_new_with_label("폴더 선택...");
    gtk_box_pack_start(GTK_BOX(hbox), browse_button, FALSE, FALSE, 0);

    GtkWidget *help_button = gtk_button_new_with_label("도움말");
    gtk_box_pack_start(GTK_BOX(hbox), help_button, FALSE, FALSE, 0);

    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled, TRUE, TRUE, 0);

    list_box = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(list_box), GTK_SELECTION_SINGLE);
    gtk_container_add(GTK_CONTAINER(scrolled), list_box);

    g_signal_connect(add_button, "clicked", G_CALLBACK(on_add_clicked), NULL);
    g_signal_connect(browse_button, "clicked", G_CALLBACK(on_add_from_fs_clicked), window);
    g_signal_connect(help_button, "clicked", G_CALLBACK(on_help_clicked), NULL);
    g_signal_connect(list_box, "row-activated", G_CALLBACK(on_row_activated), NULL);
    g_signal_connect(list_box, "button-press-event", G_CALLBACK(on_list_button_press), NULL);
    g_signal_connect(window, "delete-event", G_CALLBACK(on_window_delete), NULL);

    if (pos_file) {
        FILE *fp = fopen(pos_file, "r");
        if (fp) {
            gint x, y;
            if (fscanf(fp, "%d %d", &x, &y) == 2) {
                gtk_window_move(GTK_WINDOW(window), x, y);
            } else {
                gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
            }
            fclose(fp);
        } else {
            gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
        }
    } else {
        gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    }

    load_paths();

    gtk_widget_show_all(window);
}

int main(int argc, char **argv) {
    GtkApplication *app;
    int status;

    paths_file = g_build_filename(g_get_home_dir(), ".gtk_ford_paths", NULL);
    pos_file = g_build_filename(g_get_home_dir(), ".gtk_ford_position", NULL);

    app = gtk_application_new("com.example.fordpaths", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);

    status = g_application_run(G_APPLICATION(app), argc, argv);

    g_object_unref(app);
    g_free(paths_file);
    g_free(pos_file);
    return status;
}
