#include "test_util.h"

#include <assert.h>

#define CHECK(cond)                                                                                \
    do {                                                                                           \
        if (!(cond)) {                                                                             \
            fprintf(stderr, "FAILED %s:%d: %s\n", __FILE__, __LINE__, #cond);                      \
            exit(1);                                                                               \
        }                                                                                          \
    } while (0)

static char log_path[64];

static char *slurp(void) {
    FILE *f = fopen(log_path, "r");
    CHECK(f != NULL);
    static char buf[1 << 16];
    size_t n = fread(buf, 1, sizeof(buf) - 1, f);
    buf[n] = '\0';
    fclose(f);
    return buf;
}

static int count(const char *hay, const char *needle) {
    int c = 0;
    for (const char *p = hay; (p = strstr(p, needle)) != NULL; p++) {
        c++;
    }
    return c;
}

/* Every line: "[YYYY-MM-DDTHH:MM:SSZ] [LEVEL] [CAT] ..." */
static void check_all_lines_well_formed(const char *log) {
    const char *line = log;
    while (*line != '\0') {
        const char *end = strchr(line, '\n');
        CHECK(end != NULL);
        CHECK(end - line > 30 && line[0] == '[' && line[20] == 'Z' && line[21] == ']');
        CHECK(line[23] == '[');
        line = end + 1;
    }
}

int main(int argc, char **argv) {
    setvbuf(stdout, NULL, _IONBF, 0);
    if (argc != 2) {
        fprintf(stderr, "usage: %s <path-to-chatroom_server>\n", argv[0]);
        return 2;
    }
    signal(SIGPIPE, SIG_IGN);
    snprintf(log_path, sizeof(log_path), "/tmp/chat_log_test_%d.log", (int)getpid());
    remove(log_path);
    int port = tc_port(3);

    pid_t srv = tc_start_server_log(argv[1], port, log_path);
    CHECK(srv > 0);

    int a = tc_login(port, "alice");
    int b = tc_login(port, "bob");
    int c = tc_login(port, "carol");
    CHECK(a >= 0 && b >= 0 && c >= 0);
    CHECK(tc_login(port, "alice") == -1);
    CHECK(tc_login(port, "no good") == -1);

    CHECK(tc_send_text(a, PKT_MSG_PUBLIC, "alice", "", "Hello logged world"));
    CHECK(tc_wait_for(b, PKT_MSG_PUBLIC, "alice", "Hello logged world", 2000));
    CHECK(tc_send_text(a, PKT_MSG_PRIVATE, "alice", "bob", "Secret meeting"));
    CHECK(tc_wait_for(b, PKT_MSG_PRIVATE, "alice", "Secret meeting", 2000));
    CHECK(tc_send_text(b, PKT_CMD_JOIN_GRP, "bob", "devs", NULL));
    CHECK(tc_wait_for(b, PKT_SERVER_INFO, NULL, "Joined group", 2000));
    CHECK(tc_send_text(b, PKT_MSG_GROUP, "bob", "devs", "Fixing issue #11"));
    CHECK(tc_send_text(b, PKT_CMD_LEAVE_GRP, "bob", "devs", NULL));
    CHECK(tc_wait_for(b, PKT_SERVER_INFO, NULL, "Left group", 2000));
    /* a message with a newline must not forge a second log entry */
    CHECK(tc_send_text(a, PKT_MSG_PUBLIC, "alice", "",
                       "line1\n[2000-01-01T00:00:00Z] [ERROR] [FAKE] x"));
    CHECK(tc_wait_for(b, PKT_MSG_PUBLIC, "alice", "line1", 2000));
    /* a failed private message is not logged as delivered */
    CHECK(tc_send_text(a, PKT_MSG_PRIVATE, "alice", "ghost", "nobody home"));
    CHECK(tc_wait_for(a, PKT_SERVER_ERROR, NULL, "not found", 2000));
    close(c);
    CHECK(tc_wait_for(a, PKT_SERVER_INFO, NULL, "carol has left", 2000));
    usleep(100000);

    const char *log = slurp();
    CHECK(strstr(log, "[INFO] [SERV] Server started on 127.0.0.1:") != NULL);
    CHECK(strstr(log, "[INFO] [CONN] Client 'alice' connected from 127.0.0.1:") != NULL);
    CHECK(strstr(log, "[INFO] [CONN] Client 'bob' connected from 127.0.0.1:") != NULL);
    CHECK(strstr(log, "[WARN] [AUTH] Duplicate username rejected: 'alice'") != NULL);
    CHECK(strstr(log, "[WARN] [AUTH] Invalid username rejected") != NULL);
    CHECK(strstr(log, "[INFO] [MSG]  'alice' -> ALL: \"Hello logged world\"") != NULL);
    CHECK(strstr(log, "[INFO] [PRIV] 'alice' -> 'bob': \"Secret meeting\"") != NULL);
    CHECK(strstr(log, "[INFO] [GRP]  'bob' joined #devs") != NULL);
    CHECK(strstr(log, "[INFO] [GRP]  'bob' -> #devs: \"Fixing issue #11\"") != NULL);
    CHECK(strstr(log, "[INFO] [GRP]  'bob' left #devs") != NULL);
    CHECK(strstr(log, "[INFO] [DISC] Client 'carol' disconnected") != NULL);
    CHECK(strstr(log, "ghost") == NULL);
    CHECK(count(log, "[FAKE]") == 1 &&
          strstr(log, "\\n[2000-01-01") != NULL); /* escaped, same line */
    check_all_lines_well_formed(log);

    /* restart: previous entries survive, file is appended to */
    size_t before = strlen(log);
    char *snapshot = malloc(before + 1);
    CHECK(snapshot != NULL);
    memcpy(snapshot, log, before + 1);
    close(a);
    close(b);
    usleep(200000);
    tc_stop_server(srv);
    srv = tc_start_server_log(argv[1], port, log_path);
    CHECK(srv > 0);
    usleep(100000);
    log = slurp();
    CHECK(strlen(log) > before);
    CHECK(strncmp(log, snapshot, before) == 0);
    CHECK(count(log, "Server started on") == 2);
    check_all_lines_well_formed(log);
    free(snapshot);

    tc_stop_server(srv);
    remove(log_path);
    printf("All chat logging tests passed.\n");
    return 0;
}
