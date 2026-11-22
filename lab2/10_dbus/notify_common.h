/* 실습 2-11: DBUS 알림 서버/클라이언트 공통 정의 */
#ifndef NOTIFY_COMMON_H
#define NOTIFY_COMMON_H

#define NOTIFY_BUS_NAME   "kr.ac.deu.lab2.Notify"     /* 서버가 버스에 등록하는 이름 */
#define NOTIFY_OBJ_PATH   "/kr/ac/deu/lab2/Notify"    /* 객체 경로 */
#define NOTIFY_INTERFACE  "kr.ac.deu.lab2.Notify"     /* 인터페이스 이름 */

/* 메서드
 *   Send(string sender, string message) -> (uint32 id)   알림 전달
 *   Quit()                                               서버 종료
 * 시그널
 *   Notified(uint32 id, string sender, string message)   알림이 오면 모든 구독자에게 방송
 */
#endif
