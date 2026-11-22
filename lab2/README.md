# 실습 2. 파일 처리, 프로세스 및 프로세스간 통신 (lab2)

## 준비 (Ubuntu / WSL)
```bash
sudo apt update
sudo apt install -y build-essential libdbus-1-dev dbus-x11 git
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh   # Rust (cargo)
source ~/.cargo/env
```

## 빌드
```bash
make          # 전체 빌드 (C 프로그램 + myshell + Rust)
make clean    # 정리
```

## 디렉토리 구성과 실행 방법

| 순서 | 디렉토리 | 내용 | 실행 |
|---|---|---|---|
| 2 | `01_matrix_rust` | Vec 으로 두 행렬 동적 할당 후 덧셈 (Rust) | `cd 01_matrix_rust && cargo run --release` |
| 3 | `02_file` | open/read/write/lseek/stat/chmod/link/symlink/rename/mkdir/opendir, 장치 파일 | `./02_file/file_ops` |
| 4 | `03_lsr` | `ls -R` 과 같은 재귀 목록 | `./03_lsr/myls_r [-1] [디렉토리]` |
| 5 | `04_typing` | 타자 연습 (오타 수, 분당 타수) | `./04_typing/typing` |
| 6 | `05_process_signal` | fork/exec/wait/system, sigaction/kill/alarm/sigprocmask | `./proc_demo`, `./signal_demo` |
| 7 | `06_ipc` | pipe+dup2, FIFO, System V 메시지큐·공유메모리·세마포어 | `./pipe_demo`, `./fifo_demo`, `./sysv_ipc_demo` |
| 8 | `07_pipe_chat` | FIFO 2개로 양방향 채팅 | 터미널1 `./pipe_chat 1 철수` / 터미널2 `./pipe_chat 2 영희` |
| 9 | `08_mq_chat` | POSIX 메시지 큐 채팅 | 터미널1 `./mq_chat 철수 영희` / 터미널2 `./mq_chat 영희 철수` |
| 10 | `09_shm_copy` | POSIX 공유메모리 + mmap 파일 복사 | `./shm_copy 원본 대상` |
| 11 | `10_dbus` | DBUS 알림 서버/클라이언트 | 아래 참고 |
| 12 | `11_myshell` | 팀과제 쉘 | `./11_myshell/myshell` |

채팅 프로그램 종료: `/quit`, Ctrl-D 또는 Ctrl-C

### DBUS (WSL 은 세션 버스가 없으므로 먼저 띄운다)
```bash
cd 10_dbus
eval $(dbus-launch --sh-syntax)      # 세션 버스 시작 (같은 터미널에서 아래 실행)
./notify_server &                    # 서버
./notify_client listen &             # (선택) 시그널 구독자
./notify_client send 교수님 "보고서 제출은 금요일까지"
./notify_client quit                 # 서버 종료
```
다른 터미널에서 클라이언트를 실행하려면 그 터미널에서 `export DBUS_SESSION_BUS_ADDRESS=...` 로 같은 주소를 지정한다.

### myshell 기능
- `exit [n]`, Ctrl-D 로 종료
- `명령 &` 백그라운드 실행, `jobs`, 끝나면 `[n]+ Done` 출력
- Ctrl-C(SIGINT), Ctrl-\(SIGQUIT): 전경 명령만 종료 / Ctrl-Z(SIGTSTP): 일시정지 → `fg`
- `>`, `>>`, `<`, `|` (여러 단계 파이프), 따옴표, `~`, 와일드카드(`*.c`)
- 내부 명령(`commands/`): ls pwd cd mkdir rmdir ln cp rm mv cat grep

| 팀원 | 담당 파일 |
|---|---|
| A | `commands/ls.c`, `pwd.c`, `cd.c`, `mkdir.c` |
| B | `commands/rmdir.c`, `ln.c`, `cp.c`, `rm.c` |
| C | `commands/mv.c`, `cat.c`, `grep.c` |

## GitHub 업로드
```bash
cd ~/lab2
git init
git add .
git commit -m "lab2: 파일 처리, 프로세스, IPC, myshell"
git branch -M main
git remote add origin https://github.com/mnmn05/lab2.git   # GitHub 에서 lab2 저장소를 먼저 생성
git push -u origin main
```
