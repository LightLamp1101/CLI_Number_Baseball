#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define BUF_SIZE 100
#define USER_ID_SIZE 21
#define ANSWER_SIZE 3

typedef struct User {
	char id[USER_ID_SIZE];
} User;

typedef struct MultipleArg {
	int sock;
	char id[USER_ID_SIZE];
} MultipleArg;

void* clnt_view(void* arg);
// needed for login_view
User* login_view(void* arg);
int login(void* arg, User* user);
int sign_up(void* arg);
// needed for main_view
int main_view(void* arg, User* user);
int room_view(void* arg, User* user);
int chatting(void* arg, User* user);
int start_game(void* arg, User* user);
int enter_matching_room(void* arg, User* user);
int create_room(void* arg, User* user);      //방 만들기
int enter_room(void* arg, User* user);      //방 들어가기
int search_room(void* arg);      //방 목록 보기
int search_user(void* arg);      //접속한 사용자 보기
int ranking(void* arg);
void* send_msg(void* multiple_arg);
void* recv_msg(void* arg);
void error_handling(char* msg);

int game = 0;

int main(int argc, char* argv[])
{
	int sock;
	struct sockaddr_in serv_addr;
	pthread_t snd_thread, rcv_thread;
	void* thread_return;

	if (argc != 3) {
		printf("Usage : %s <IP> <PORT> \n", argv[0]);
		exit(1);
	}

	sock = socket(PF_INET, SOCK_STREAM, 0);

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
	serv_addr.sin_port = htons(atoi(argv[2]));

	if (connect(sock, (struct sockaddr*) & serv_addr, sizeof(serv_addr)) == -1)
		error_handling("connect() error");

	clnt_view(&sock);

	close(sock);
	return 0;
}

void* clnt_view(void* arg) {
	int sock = *((int*)arg);
	int end = 1;
	User* user = (User*)malloc(sizeof(User));
	system("clear");
	if ((user = login_view(&sock)) != NULL) {   // 로그인 성공
		while (end == 1) {
			end = main_view(&sock, user);
		}
	}
	return NULL;
}

// 접속 시 첫 화면으로 로그인를 위한 뷰
User* login_view(void* arg) {
	int sock = *((int*)arg);
	User* user = (User*)malloc(sizeof(User));
	char msg[BUF_SIZE];
	char answer[ANSWER_SIZE];
	int login_result = 0;

	while (1) {
		fputs("\n* * * * * * * * * * * * * * * * *\n", stdout);
		fputs("*                               *\n", stdout);
		fputs("*        다자간 숫자야구        *\n", stdout);
		fputs("*                               *\n", stdout);
		fputs("* * * * * * * * * * * * * * * * *\n", stdout);
		fputs("\n\n=== 메 뉴 ===\n", stdout);
		fputs("\n1. 로그인\n", stdout);
		fputs("\n2. 계정 생성\n", stdout);
		fputs("\n3. 종 료\n", stdout);
		fputs("\n메뉴를 선택해주세요. (1-3). ? ", stdout);
		fflush(stdout);

		fgets(answer, sizeof(answer), stdin);
		write(sock, answer, sizeof(answer));
		fflush(stdin);

		switch (answer[0]) {
		case '1': // 로그인
			login_result = login(&sock, user);   // 로그인 결과

			if (login_result == 1) {   // 로그인 성공
				return user;
			}
			else   // 로그인 실패
				break;
		case '2': // 계정 생성
			sign_up(&sock);
			break;
		case '3':
			fputs("\n연결이 종료됩니다.\n", stdout);
			return NULL;   // 종료 요청
		default:
			system("clear");
			fputs("\n보기 중 입력해주세요. (1-3) ? \n", stdout);
			break;
		}
	}

	return NULL;
}

// 로그인을 위해 아이디/비밀번호를 사용자로부터 입력받음
int login(void* arg, User* user) {
	int sock = *((int*)arg);
	int id_len = USER_ID_SIZE, pw_len = USER_ID_SIZE;
	char id[id_len + 1], pw[pw_len + 1];
	char msg[BUF_SIZE];
	char answer[ANSWER_SIZE];
	int login_result = 0;
	system("clear");
	fputs("\n* * * * * * * * * * * * * * * * *\n", stdout);
	fputs("*                               *\n", stdout);
	fputs("*        다자간 숫자야구        *\n", stdout);
	fputs("*                               *\n", stdout);
	fputs("* * * * * * * * * * * * * * * * *\n", stdout);
	fputs("\n\n--- 로그인 ---\n", stdout);

	fputs("\n아이디 : ", stdout);
	fgets(id, sizeof(id), stdin); // 아이디 입력

	fputs("\n비밀번호 : ", stdout);
	fgets(pw, sizeof(pw), stdin); // 비밀번호 입력

	sprintf(msg, "%s%s", id, pw);
	write(sock, msg, sizeof(msg));

	system("clear");
	printf("로그인중...\n");
	sleep(0.5);
	read(sock, answer, sizeof(answer));
	if ((answer[0]) == '1') {   // 로그인 성공
		strncpy(user->id, id, strlen(id) - 1);
		fputs("\n로그인 성공!\n", stdout);
		login_result = 1;
	}
	else if (strcmp(answer, "2\n") == 0) {
		fputs("\n이미 접속중인 아이디 입니다!\n", stdout);
	}
	else {
		fputs("\n아이디/비밀번호를 확인해주세요.\n", stdout);
	}

	return login_result;   // 1 or 0
}

// 회원가입을 위해 아이디/비밀번호를 사용자로부터 입력받음
int sign_up(void* arg) {
	int sock = *((int*)arg);
	int id_len = USER_ID_SIZE;
	int pw_len = USER_ID_SIZE;
	char id[id_len], pw[pw_len], id_pw[id_len + pw_len];
	int sign_up_result = 0;
	char answer[ANSWER_SIZE];
	system("clear");
	fputs("\n* * * * * * * * * * * * * * * * *\n", stdout);
	fputs("*                               *\n", stdout);
	fputs("*        다자간 숫자야구        *\n", stdout);
	fputs("*                               *\n", stdout);
	fputs("* * * * * * * * * * * * * * * * *\n", stdout);
	fputs("\n\n--- 계정생성 ---\n", stdout);

	fputs("\n아이디 : ", stdout);
	fgets(id, sizeof(id), stdin);

	fputs("\n비밀번호 : ", stdout);
	fgets(pw, sizeof(pw), stdin);

	fputs("\n위의 내용대로 생성하시겠습니까 (Y/N) ? ", stdout);
	fgets(answer, sizeof(answer), stdin);
	system("clear");

	if (strcmp(answer, "Y\n") || strcmp(answer, "y\n")) {
		sprintf(id_pw, "%s%s", id, pw);
		write(sock, id_pw, strlen(id_pw));
		printf("계정 생성중...\n");
		read(sock, answer, sizeof(answer));
		if ((answer[0]) == '1') {   // 계정생성 성공
			fputs("\n계정생성 성공!\n", stdout);
			sign_up_result = 1;
		}
		else {
			fputs("\n이미 존재하는 아이디입니다.\n", stdout);
		}
	}

	return sign_up_result;
}



int main_view(void* arg, User* user) {
	int sock = *((int*)arg);
	static int stat = 0;
	char answer[ANSWER_SIZE];
	char msg[] = "\n = = = 메  뉴 = = =\n1. 매칭으로 게임 시작하기\n2. 방 생성하기\n3. 방 참가하기\n4. 입장 가능한 방 조회\n5. 현재 사용자 조회\n6. 랭킹 조회\n7. 로그아웃\n0. 도움말\n";
	fputs("\n* * * * * * * * * * * * * * * * *\n", stdout);
	fputs("*                               *\n", stdout);
	fputs("*        다자간 숫자야구        *\n", stdout);
	fputs("*                               *\n", stdout);
	fputs("* * * * * * * * * * * * * * * * *\n", stdout);
	if (stat++ == 0) printf("\n%s 님 환영합니다!\n", user->id);
	printf("%s", msg);

	while (1) {
		fputs("\n메뉴를 선택해주세요. (0-7)\n", stdout);
		printf("%s > ", user->id);
		fgets(answer, sizeof(answer), stdin);
		write(sock, answer, sizeof(answer));
		fflush(stdin);

		switch (answer[0]) {
		case '1':
			enter_matching_room(&sock, user);
			return 1;
		case '2':
			create_room(&sock, user);
			return 1;
		case '3':
			enter_room(&sock, user);
			return 1;
		case '4':
			search_room(&sock);
			break;
		case '5':
			search_user(&sock);
			break;
		case '6':
			ranking(&sock);
			break;
		case '7':
			printf("접속을 종료합니다.\n\n");
			return 0;
		case '0':
			system("clear");
			fputs("\n* * * * * * * * * * * * * * * * *\n", stdout);
			fputs("*                               *\n", stdout);
			fputs("*        다자간 숫자야구        *\n", stdout);
			fputs("*                               *\n", stdout);
			fputs("* * * * * * * * * * * * * * * * *\n", stdout);
			printf("%s", msg);
		default:
			break;
		}
	}
	return 1;
}

int enter_matching_room(void* arg, User* user) {
	int sock = *((int*)arg);

	system("clear");
	printf("매칭방 입장\n");
	room_view(&sock, user);

	return 0;
}

int create_room(void* arg, User* user) {      //방 생성
	int sock = *(int*)arg;
	char name[21];
	int str_len = 0;
	fputs("\n생성할 방 이름 : ", stdout);
	fgets(name, sizeof(name), stdin);
	name[strlen(name)] = 0;
	system("clear");
	printf("방 생성중... -> %s", name);
	str_len = write(sock, name, sizeof(name));
	if (str_len == -1)
		return 0;

	printf("생성 성공\n");
	room_view(&sock, user);

	return 0;
}

int enter_room(void* arg, User* user) {
	int sock = *(int*)arg;
	char name[21];
	char msg[BUF_SIZE];
	char answer[ANSWER_SIZE];
	int str_len = 0;

	fputs("\n입장할 방의 번호 또는 이름을 입력해주세요. ", stdout);
	fgets(name, sizeof(name), stdin);
	str_len = write(sock, name, sizeof(name));
	if (str_len == -1)
		return 0;

	system("clear");
	printf("방 입장중... -> %s", name);

	read(sock, answer, sizeof(answer));   // 입장 결과 수신

	if (strcmp(answer, "1\n") == 0) {   // 방 입장
		printf(" 입장 성공!\n");
		room_view(&sock, user);
	}
	else {      // 로비로
		printf(" 입장 실패!\n");
	}
	return 0;
}

int search_room(void* arg) {      // 방 id name 출력
	int sock = *(int*)arg;
	char msg[1000];
	int str_len = 0;

	fputs("\n방 조회\n", stdout);
	str_len = read(sock, msg, sizeof(msg));      // 방 목록 읽기
	if (str_len == -1)   // read() error
		return 0;
	msg[str_len] = 0;
	fputs(msg, stdout);
	return 0;
}

int search_user(void* arg) {   // 접속중인 사용자 조회
	int sock = *(int*)arg;
	char msg[1000];
	int str_len = 0;

	printf("\n사용자 조회\n");
	str_len = read(sock, msg, sizeof(msg));      // 사용자 목록 읽기
	if (str_len == -1)   // read() error
		return 0;

	msg[str_len] = 0;
	fputs(msg, stdout);
	return 0;
}

int ranking(void* arg) {
	int sock = *(int*)arg;
	char msg[1000];

	fputs("\n랭킹조회\n", stdout);
	fputs(" 랭킹            ID     승리  무승부  패배\n", stdout);
	fputs("", stdout);
	read(sock, msg, sizeof(msg) - 1);
	fputs(msg, stdout);
	return 0;
}


int room_view(void* arg, User* user) { //
	int sock = *((int*)arg);

	while ((chatting(&sock, user)) == 1) {
		start_game(&sock, user);
	}

	printf("방 퇴장\n");
	return 0;
}


int chatting(void* arg, User* user) {
	int sock = *((int*)arg);
	pthread_t snd_thread, rcv_thread;
	void* rcv_thread_return, * snd_thread_return;
	MultipleArg* multiple_arg;

	multiple_arg = (MultipleArg*)malloc(sizeof(MultipleArg)); // init
	multiple_arg->sock = sock;
	memcpy(multiple_arg->id, user->id, strlen(user->id));

	printf("채팅시작\n");

	pthread_create(&snd_thread, NULL, send_msg, (void*)multiple_arg);
	pthread_create(&rcv_thread, NULL, recv_msg, (void*)& sock);
	pthread_join(snd_thread, &snd_thread_return);
	pthread_join(rcv_thread, &rcv_thread_return);

	printf("채팅종료\n");

	if ((int*)snd_thread_return && (int*)rcv_thread_return) {
		return 1;
	}

	return 0;
}

void* send_msg(void* multiple_arg)
{
	MultipleArg mArg = *((MultipleArg*)multiple_arg);
	char start_msg[100];
	char msg[BUF_SIZE];
	char name_msg[USER_ID_SIZE + BUF_SIZE];
	char uid[USER_ID_SIZE];

	strncpy(uid, mArg.id, sizeof(mArg.id)); // id 복사
	sprintf(start_msg, "%s님이 입장하였습니다.\n", mArg.id);
	write(mArg.sock, start_msg, sizeof(start_msg));

	while (1)
	{
		fgets(msg, sizeof(msg), stdin);
		sprintf(name_msg, "%s: %s", uid, msg);
		if (msg[0] == '/') {
			if (strcmp(msg, "/q\n") == 0) {
				write(mArg.sock, msg, strlen(msg));
				return 0;
			}
			else if (strcmp(msg, "/r\n") == 0) {
				write(mArg.sock, msg, strlen(msg));
				printf("준비되었습니다.\n");
				return 1;
			}
			else if (strcmp(msg, "/?\n") == 0) {
				write(mArg.sock, msg, strlen(msg));
			}
		}
		else {
			write(mArg.sock, name_msg, strlen(name_msg));
		}
	}
	return NULL;
}

void* recv_msg(void* arg)
{
	MultipleArg mArg = *((MultipleArg*)arg);
	char name_msg[USER_ID_SIZE + BUF_SIZE];
	int str_len = 0;

	while (1)
	{
		str_len = read(mArg.sock, name_msg, USER_ID_SIZE + BUF_SIZE);
		if (str_len == -1)
			return (void*)-1;
		name_msg[str_len] = 0;
		if (name_msg[0] == '/') {
			if ((strncmp(name_msg, "/q\n", strlen("/q\n"))) == 0) {
				printf("방을 나갑니다.\n");
				return 0;
			}
			else if (strncmp(name_msg, "/r\n", strlen("/r\n")) == 0) {
				printf("게임시작\n");
				return 1;
			}
		}
		else {
			fputs(name_msg, stdout);
		}

	}
	return NULL;
}

int start_game(void* arg, User* user) {
	int sock = *((int*)arg);
	char user_num[5];
	char msg[BUF_SIZE];
	char set_turn[5];
	int str_len = 0;
	int first = 1;
	int turn = 1;
	int cnt = 1;

	while (1) {   // 정답을 맞추는 사용자가 나올때까지 반복
		if (turn) {
			while (1) { // 정상적인 숫자만 입력받기 위한 반복
				if (first == 1) {
					fprintf(stdout, "%s님 1부터 9사이의 3자리 숫자를 입력해주세요. (ex: 123) : ", user->id);
					first--;
				}
				else {
					printf("공격할 숫자를 입력해주세요 : ");
				}
				fgets(user_num, sizeof(user_num), stdin);
				if (user_num[0] < '1' || user_num[0] > '9' || user_num[1] < '1' || user_num[1] > '9' || user_num[2] < '1' || user_num[2] > '9')
				{ // 입력한 숫자가 1 ~ 9 숫자가 아니면 다시 입력받도록 
					printf("범위 외의 숫자를 입력하시면 안됩니다.\n");
					continue;
				}
				else if (user_num[0] == user_num[1] || user_num[0] == user_num[2] || user_num[1] == user_num[2])
				{ // 입력한 숫자 중에 중복된 게 있으면 다시 입력받도록 
					printf("중복된 숫자를 입력하시면 안됩니다.\n");
					continue;
				}
				break; // 아무 문제 없을 경우 반복 종료 
			}
			write(sock, user_num, strlen(user_num));
			turn = !turn;
		}

		while (1) {
			if (!(str_len = read(sock, msg, sizeof(msg) - 1)))
				break;

			msg[str_len] = '\0';
			if (msg[0] == '/') {
				if (strncmp(msg, "/win\n", strlen("/win\n")) == 0) {
					printf("\n승리하셨습니다!!\n");
					return 0;
				}
				else if (strncmp(msg, "/draw\n", strlen("/draw\n")) == 0) {
					printf("\n비겼습니다!!\n");
					return 0;
				}
				else if (strncmp(msg, "/lose\n", strlen("/lose\n")) == 0) {
					printf("\n패배하셨습니다!!\n");
					return 0;
				}
				else if (strncmp(msg, "/turn1\n", strlen("/turn1\n")) == 0) {
					turn = 1;
					printf("\n공격 %d회초\n", cnt);
					cnt++;
					break;
				}
				else if (strncmp(msg, "/turn2\n", strlen("/turn2\n")) == 0) {
					turn = 1;
					printf("\n공격 %d회말\n", cnt);
					cnt++;
					break;
				}
			}
			else if (msg[0] == '\n') {
				fputs(msg, stdout);
				write(sock, "\n", strlen("\n"));
				break;
			}
			fputs(msg, stdout);
		}
	}

	return 0;
}

void error_handling(char* msg)
{
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}