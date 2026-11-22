/* 실습 2-11: DBUS 알림 서버
 * 세션 버스에 kr.ac.deu.lab2.Notify 이름을 등록하고
 * 클라이언트의 Send(sender, message) 메서드 호출을 받아 알림을 출력한 뒤
 * 알림 번호를 응답으로 돌려주고, Notified 시그널로 모든 구독자에게 방송한다.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <dbus/dbus.h>
#include "notify_common.h"

static dbus_uint32_t next_id = 1;
static int running = 1;

static void reply_error(DBusConnection *conn, DBusMessage *msg, const char *name, const char *text)
{
    DBusMessage *err = dbus_message_new_error(msg, name, text);
    dbus_connection_send(conn, err, NULL);
    dbus_message_unref(err);
}

static void handle_send(DBusConnection *conn, DBusMessage *msg)
{
    DBusError err;
    dbus_error_init(&err);
    const char *sender, *text;
    if (!dbus_message_get_args(msg, &err, DBUS_TYPE_STRING, &sender, DBUS_TYPE_STRING, &text,
                               DBUS_TYPE_INVALID)) {
        reply_error(conn, msg, DBUS_ERROR_INVALID_ARGS, err.message);
        dbus_error_free(&err);
        return;
    }

    dbus_uint32_t id = next_id++;
    char tbuf[32];
    time_t now = time(NULL);
    strftime(tbuf, sizeof(tbuf), "%H:%M:%S", localtime(&now));
    printf("[%s] 알림 #%u  보낸이: %s\n          내용: %s\n", tbuf, id, sender, text);
    fflush(stdout);

    /* 1) 메서드 응답: 알림 번호 */
    DBusMessage *reply = dbus_message_new_method_return(msg);
    dbus_message_append_args(reply, DBUS_TYPE_UINT32, &id, DBUS_TYPE_INVALID);
    dbus_connection_send(conn, reply, NULL);
    dbus_message_unref(reply);

    /* 2) 시그널 방송: 구독 중인 클라이언트(listen 모드)에게 전달 */
    DBusMessage *sig = dbus_message_new_signal(NOTIFY_OBJ_PATH, NOTIFY_INTERFACE, "Notified");
    dbus_message_append_args(sig, DBUS_TYPE_UINT32, &id, DBUS_TYPE_STRING, &sender,
                             DBUS_TYPE_STRING, &text, DBUS_TYPE_INVALID);
    dbus_connection_send(conn, sig, NULL);
    dbus_message_unref(sig);
    dbus_connection_flush(conn);
}

int main(void)
{
    DBusError err;
    dbus_error_init(&err);

    DBusConnection *conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
    if (!conn) {
        fprintf(stderr, "세션 버스 연결 실패: %s\n", err.message);
        fprintf(stderr, "힌트: eval $(dbus-launch --sh-syntax) 를 먼저 실행하세요.\n");
        return 1;
    }

    int ret = dbus_bus_request_name(conn, NOTIFY_BUS_NAME, DBUS_NAME_FLAG_DO_NOT_QUEUE, &err);
    if (ret != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) {
        fprintf(stderr, "버스 이름 등록 실패 (이미 서버가 실행 중인가요?) %s\n",
                dbus_error_is_set(&err) ? err.message : "");
        return 1;
    }
    printf("DBUS 알림 서버 시작: %s  (종료: 클라이언트에서 quit)\n", NOTIFY_BUS_NAME);
    fflush(stdout);

    while (running && dbus_connection_read_write(conn, -1)) {
        DBusMessage *msg;
        while ((msg = dbus_connection_pop_message(conn)) != NULL) {
            if (dbus_message_is_method_call(msg, NOTIFY_INTERFACE, "Send")) {
                handle_send(conn, msg);
            } else if (dbus_message_is_method_call(msg, NOTIFY_INTERFACE, "Quit")) {
                DBusMessage *reply = dbus_message_new_method_return(msg);
                dbus_connection_send(conn, reply, NULL);
                dbus_message_unref(reply);
                dbus_connection_flush(conn);
                running = 0;
            } else if (dbus_message_get_type(msg) == DBUS_MESSAGE_TYPE_METHOD_CALL &&
                       dbus_message_has_destination(msg, NOTIFY_BUS_NAME)) {
                reply_error(conn, msg, DBUS_ERROR_UNKNOWN_METHOD, "지원하지 않는 메서드입니다");
            }
            dbus_message_unref(msg);
        }
    }

    dbus_bus_release_name(conn, NOTIFY_BUS_NAME, NULL);
    dbus_connection_unref(conn);
    printf("서버를 종료합니다.\n");
    return 0;
}
