/* 실습 2-11: DBUS 알림 클라이언트
 * 사용법:
 *   ./notify_client send <보낸이> <메시지>   알림 보내기 (메서드 호출, 응답으로 번호 수신)
 *   ./notify_client listen                    Notified 시그널 구독 (방송되는 알림 수신)
 *   ./notify_client quit                      서버 종료 요청
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dbus/dbus.h>
#include "notify_common.h"

static DBusConnection *connect_bus(void)
{
    DBusError err;
    dbus_error_init(&err);
    DBusConnection *conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
    if (!conn) {
        fprintf(stderr, "세션 버스 연결 실패: %s\n", err.message);
        exit(1);
    }
    return conn;
}

/* 메서드를 호출하고 응답을 기다린다 (최대 3초) */
static DBusMessage *call(DBusConnection *conn, DBusMessage *msg)
{
    DBusError err;
    dbus_error_init(&err);
    DBusMessage *reply = dbus_connection_send_with_reply_and_block(conn, msg, 3000, &err);
    dbus_message_unref(msg);
    if (!reply) {
        fprintf(stderr, "호출 실패: %s\n  (서버가 실행 중인지 확인하세요)\n", err.message);
        dbus_error_free(&err);
        exit(1);
    }
    return reply;
}

static int do_send(DBusConnection *conn, const char *sender, const char *text)
{
    DBusMessage *msg = dbus_message_new_method_call(NOTIFY_BUS_NAME, NOTIFY_OBJ_PATH,
                                                    NOTIFY_INTERFACE, "Send");
    dbus_message_append_args(msg, DBUS_TYPE_STRING, &sender, DBUS_TYPE_STRING, &text,
                             DBUS_TYPE_INVALID);
    DBusMessage *reply = call(conn, msg);

    DBusError err;
    dbus_error_init(&err);
    dbus_uint32_t id;
    if (!dbus_message_get_args(reply, &err, DBUS_TYPE_UINT32, &id, DBUS_TYPE_INVALID)) {
        fprintf(stderr, "응답 해석 실패: %s\n", err.message);
        return 1;
    }
    printf("알림 전송 성공 (서버가 부여한 번호 #%u)\n", id);
    dbus_message_unref(reply);
    return 0;
}

static int do_listen(DBusConnection *conn)
{
    DBusError err;
    dbus_error_init(&err);
    dbus_bus_add_match(conn, "type='signal',interface='" NOTIFY_INTERFACE "',member='Notified'", &err);
    dbus_connection_flush(conn);
    if (dbus_error_is_set(&err)) { fprintf(stderr, "구독 실패: %s\n", err.message); return 1; }
    printf("알림 구독 중... (Ctrl-C 로 종료)\n");
    fflush(stdout);

    while (dbus_connection_read_write(conn, -1)) {
        DBusMessage *msg;
        while ((msg = dbus_connection_pop_message(conn)) != NULL) {
            if (dbus_message_is_signal(msg, NOTIFY_INTERFACE, "Notified")) {
                dbus_uint32_t id;
                const char *sender, *text;
                if (dbus_message_get_args(msg, &err, DBUS_TYPE_UINT32, &id, DBUS_TYPE_STRING, &sender,
                                          DBUS_TYPE_STRING, &text, DBUS_TYPE_INVALID)) {
                    printf("[수신] 알림 #%u from %s: %s\n", id, sender, text);
                    fflush(stdout);
                } else {
                    dbus_error_free(&err);
                }
            }
            dbus_message_unref(msg);
        }
    }
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc >= 4 && strcmp(argv[1], "send") == 0) {
        DBusConnection *conn = connect_bus();
        return do_send(conn, argv[2], argv[3]);
    }
    if (argc == 2 && strcmp(argv[1], "listen") == 0)
        return do_listen(connect_bus());
    if (argc == 2 && strcmp(argv[1], "quit") == 0) {
        DBusConnection *conn = connect_bus();
        DBusMessage *msg = dbus_message_new_method_call(NOTIFY_BUS_NAME, NOTIFY_OBJ_PATH,
                                                        NOTIFY_INTERFACE, "Quit");
        dbus_message_unref(call(conn, msg));
        printf("서버 종료 요청 완료\n");
        return 0;
    }
    fprintf(stderr, "사용법:\n  %s send <보낸이> <메시지>\n  %s listen\n  %s quit\n",
            argv[0], argv[0], argv[0]);
    return 1;
}
