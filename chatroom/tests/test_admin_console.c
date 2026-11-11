#include "test_util.h"

#include <fcntl.h>

static pid_t active_pid = 0;
static void cleanup_child(void) {
    if (active_pid > 0) {
        kill(active_pid, SIGKILL);
        waitpid(active_pid, NULL, 0);
    }
}

#define CHECK(cond)                                                           \
    do {                                                                      \
        if (!(cond)) {                                                        \
            fprintf(stderr, "FAILED %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            exit(1);                                                          \
        }                                                                     \
    } while (0)

typedef struct {
    pid_t pid;
    int in_fd;  /* write commands here */
    int out_fd; /* non-blocking server stdout */
    char out[1 << 16];
    size_t out_len;
} proc_t;

static void start_piped(proc_t *s, const char *bin, int port, const char *log) {
    memset(s, 0, sizeof(*s));
    int in_p[2];
    int out_p[2];
    CHECK(pipe(in_p) == 0 && pipe(out_p) == 0);
    s->pid = fork();
    active_pid = s->pid;
    if (s->pid == 0) {
        dup2(in_p[0], STDIN_FILENO);
        dup2(out_p[1], STDOUT_FILENO);
        close(in_p[0]);
        close(in_p[1]);
        close(out_p[0]);
        close(out_p[1]);
        char p[16];
        snprintf(p, sizeof(p), "%d", port);
        execl(bin, bin, TC_IP, p, log, (char *)NULL);
        _exit(127);
    }
    close(in_p[0]);
    close(out_p[1]);
    s->in_fd = in_p[1];
    s->out_fd = out_p[0];
    fcntl(s->out_fd, F_SETFL, O_NONBLOCK);
    for (int i = 0; i < 100; i++) {
        int fd = tc_connect(port);
        if (fd >= 0) {
            close(fd);
            return;
        }
        usleep(50000);
    }
    CHECK(0);
}

static void pump(proc_t *s) {
    ssize_t n;
    while (s->out_len + 1 < sizeof(s->out) &&
           (n = read(s->out_fd, s->out + s->out_len, sizeof(s->out) - 1 - s->out_len)) > 0) {
        s->out_len += (size_t)n;
        s->out[s->out_len] = '\0';
    }
}

static void cmd(proc_t *s, const char *line) {
    CHECK(write(s->in_fd, line, strlen(line)) == (ssize_t)strlen(line));
}

static int wait_out(proc_t *s, const char *needle) {
    for (int i = 0; i < 60; i++) {
        pump(s);
        if (strstr(s->out, needle) != NULL) {
            return 1;
        }
        usleep(50000);
    }
    fprintf(stderr, "timed out waiting for '%s' in:\n%s\n", needle, s->out);
    return 0;
}

int main(int argc, char **argv) {
    setvbuf(stdout, NULL, _IONBF, 0);
    if (argc != 2) {
        fprintf(stderr, "usage: %s <path-to-chatroom_server>\n", argv[0]);
        return 2;
    }
    signal(SIGPIPE, SIG_IGN);
    atexit(cleanup_child);
    char log_path[64];
    snprintf(log_path, sizeof(log_path), "/tmp/admin_test_%d.log", (int)getpid());
    remove(log_path);
    int port = tc_port(5);

    /* ---- session 1: every console command + quit ---- */
    proc_t srv;
    start_piped(&srv, argv[1], port, log_path);
    int a = tc_login(port, "alice");
    int b = tc_login(port, "bob");
    int c = tc_login(port, "carol");
    CHECK(a >= 0 && b >= 0 && c >= 0);

    cmd(&srv, "help\n");
    CHECK(wait_out(&srv, "Admin commands:"));
    cmd(&srv, "notacommand\n");
    CHECK(wait_out(&srv, "Unknown command"));

    cmd(&srv, "users\n");
    CHECK(wait_out(&srv, "Total active clients: 3"));
    CHECK(strstr(srv.out, "alice") != NULL && strstr(srv.out, "bob") != NULL);

    /* generate traffic of every kind */
    CHECK(tc_send_text(a, PKT_MSG_PUBLIC, "alice", "", "seen on monitor"));
    CHECK(tc_wait_for(b, PKT_MSG_PUBLIC, "alice", "seen on monitor", 2000));
    CHECK(wait_out(&srv, "alice: seen on monitor"));
    CHECK(tc_send_text(a, PKT_MSG_PRIVATE, "alice", "bob", "secret1"));
    CHECK(tc_wait_for(b, PKT_MSG_PRIVATE, "alice", "secret1", 2000));
    CHECK(tc_send_text(b, PKT_CMD_JOIN_GRP, "bob", "ops", NULL));
    CHECK(tc_wait_for(b, PKT_SERVER_INFO, NULL, "Joined group", 2000));
    CHECK(tc_send_text(b, PKT_MSG_GROUP, "bob", "ops", "group1"));

    /* monitor off silences traffic echoing; monitor on brings it back */
    cmd(&srv, "monitor off\n");
    CHECK(wait_out(&srv, "Traffic monitor is off."));
    CHECK(tc_send_text(a, PKT_MSG_PUBLIC, "alice", "", "quiet message"));
    CHECK(tc_wait_for(b, PKT_MSG_PUBLIC, "alice", "quiet message", 2000));
    usleep(100000);
    pump(&srv);
    CHECK(strstr(srv.out, "quiet message") == NULL);
    cmd(&srv, "monitor on\n");
    CHECK(wait_out(&srv, "Traffic monitor is on."));
    CHECK(tc_send_text(a, PKT_MSG_PUBLIC, "alice", "", "loud again"));
    CHECK(tc_wait_for(b, PKT_MSG_PUBLIC, "alice", "loud again", 2000));
    CHECK(wait_out(&srv, "alice: loud again"));

    cmd(&srv, "broadcast Maintenance in 10 minutes\n");
    CHECK(tc_wait_for(a, PKT_SERVER_INFO, "server", "[ADMIN] Maintenance in 10 minutes", 2000));
    CHECK(tc_wait_for(b, PKT_SERVER_INFO, "server", "[ADMIN] Maintenance in 10 minutes", 2000));

    /* kick: victim gets an error + EOF; survivors get a notice */
    cmd(&srv, "kick nobody\n");
    CHECK(wait_out(&srv, "User 'nobody' not found."));
    cmd(&srv, "kick carol\n");
    CHECK(tc_wait_for(c, PKT_SERVER_ERROR, NULL, "kicked by an administrator", 2000));
    packet_header_t h;
    char p[PROTO_MAX_PAYLOAD + 1];
    tc_set_timeout(c, 2000);
    CHECK(packet_recv(c, &h, p) == 0); /* socket closed cleanly by the server */
    close(c);
    CHECK(tc_wait_for(a, PKT_SERVER_INFO, NULL, "carol was kicked", 2000));

    cmd(&srv, "stats\n");
    CHECK(wait_out(&srv, "Active clients      : 2 (peak: 3)"));
    CHECK(strstr(srv.out, "Messages relayed    : 6 (broadcast: 4, private: 1, group: 1)") != NULL);
    CHECK(strstr(srv.out, "Bytes TX / RX") != NULL);

    /* graceful shutdown via `quit`: clients receive SERVER_SHUTDOWN, process exits 0 */
    cmd(&srv, "quit\n");
    CHECK(tc_wait_for(a, PKT_SERVER_INFO, NULL, "SERVER_SHUTDOWN", 2000));
    CHECK(tc_wait_for(b, PKT_SERVER_INFO, NULL, "SERVER_SHUTDOWN", 2000));
    int status = 0;
    CHECK(waitpid(srv.pid, &status, 0) == srv.pid);
    CHECK(WIFEXITED(status) && WEXITSTATUS(status) == 0);
    active_pid = 0;
    pump(&srv);
    CHECK(strstr(srv.out, "Server stopped.") != NULL);
    close(a);
    close(b);
    close(srv.in_fd);
    close(srv.out_fd);

    /* ---- session 2: SIGINT also triggers a clean exit 0 and flushes the log ---- */
    start_piped(&srv, argv[1], port, log_path);
    a = tc_login(port, "alice");
    CHECK(a >= 0);
    kill(srv.pid, SIGINT);
    CHECK(tc_wait_for(a, PKT_SERVER_INFO, NULL, "SERVER_SHUTDOWN", 2000));
    CHECK(waitpid(srv.pid, &status, 0) == srv.pid);
    CHECK(WIFEXITED(status) && WEXITSTATUS(status) == 0);
    active_pid = 0;
    close(a);
    close(srv.in_fd);
    close(srv.out_fd);

    FILE *f = fopen(log_path, "r");
    CHECK(f != NULL);
    char log[16384];
    size_t n = fread(log, 1, sizeof(log) - 1, f);
    log[n] = '\0';
    fclose(f);
    CHECK(strstr(log, "[WARN] [ADMIN] Kicked user 'carol'") != NULL);
    CHECK(strstr(log, "[INFO] [ADMIN] Broadcast: \"Maintenance in 10 minutes\"") != NULL);
    CHECK(strstr(log, "[INFO] [SERV] Server stopped cleanly") != NULL);
    remove(log_path);

    printf("All admin console tests passed.\n");
    return 0;
}
