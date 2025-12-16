#include <gtk/gtk.h>
#include <iostream>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <unistd.h>
#include <cstring>

#define SIZE 4096

GtkWidget *textView;
GtkWidget *entry;

char *shared;
sem_t *semSend;
sem_t *semRecv;

void appendText(const char *msg) {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textView));
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(buffer, &end);
    gtk_text_buffer_insert(buffer, &end, msg, -1);
    gtk_text_buffer_insert(buffer, &end, "\n", -1);
}

gpointer receiveThread(gpointer data) {
    while (true) {
        sem_wait(semSend);

        if (strcmp(shared, "exit") == 0)
            break;

        appendText(shared);
    }
    return nullptr;
}

void sendMessage(GtkWidget *widget, gpointer data) {
    const char *msg = gtk_entry_get_text(GTK_ENTRY(entry));
    if (strlen(msg) == 0) return;

    sprintf(shared, "%s", msg);
    appendText(msg);

    sem_post(semRecv);
    gtk_entry_set_text(GTK_ENTRY(entry), "");
}

int main(int argc, char **argv) {

    int fd = shm_open("ChatOS", O_RDWR, 0666);
    shared = (char*)mmap(nullptr, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    semSend = sem_open("/ChatA", 0);
    semRecv = sem_open("/ChatB", 0);

    gtk_init(&argc, &argv);

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Chat – Client 2");
    gtk_window_set_default_size(GTK_WINDOW(window), 350, 300);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    textView = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(textView), false);
    gtk_box_pack_start(GTK_BOX(vbox), textView, true, true, 0);

    entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(vbox), entry, false, false, 0);

    GtkWidget *button = gtk_button_new_with_label("Send");
    gtk_box_pack_start(GTK_BOX(vbox), button, false, false, 0);

    g_signal_connect(button, "clicked", G_CALLBACK(sendMessage), NULL);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    g_thread_new("recv", receiveThread, NULL);

    gtk_widget_show_all(window);
    gtk_main();

    return 0;
}

