#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define BUF_SIZE 100
#define MAX_CLNT 256
#define USER_ID_SIZE 21
#define MAX_ROOM 2
#define ROOM_NAME_SIZE 21
#define ANSWER_SIZE 3

typedef struct User {
	char id[USER_ID_SIZE];
	int sock;
} User;

typedef struct UserController {
	User* user_list[MAX_CLNT];
	int cnt;
} UserController;

typedef struct GameController {
	char answer_list[2][5];
	int ready_state[2];
	int turn;
} GameController;

typedef struct Stat {
	char id[USER_ID_SIZE];
	int win, draw, lose;
} Stat;

typedef struct Room {
	int id;
	char user_id[2][USER_ID_SIZE];
	char name[ROOM_NAME_SIZE];
	int clnt_sock[2];
	int cnt;
	GameController gameController;
} Room;

typedef struct MatchingRoomController {
	Room* room_list[MAX_ROOM];
	int cnt; // matching room count
} MatchingRoomController;

typedef struct CreatedRoomController {
	Room* room_list[30];
	int cnt;   // created room count
} CreatedRoomController;

void error_handling(char* msg);

// client handling thread
void* handle_clnt(void* arg);
// client enter this room and play game
void create_matching_rooms();
// needed for login_view
User* login_view(void* arg);
int login(void* arg, User* user);
int sign_up(void* arg);
int rw_login_db(char* mode, char* id, char* pw);
// needed for main_view
int main_view(User* user);
Room* get_matching_room(User* user);
Room* get_create_room(User* user);
int room_view(User* user, Room* room);
int start_game(User* user, Room* room);
int waiting_user_ready(User* user, Room* room);
int enter_created_room(User* user);
//void send_msg(User* user, char* msg, int len, int num);
void search_room(User* user);
void search_user(User* user);
void print_ranking(User* user);
int input_rank(char id[USER_ID_SIZE], int outcome);

// removing socket from socket array
void delete_room(Room* room);
void* write_log();
char* itoa(long val, char* buf, unsigned radix);

UserController userController;
MatchingRoomController mRoomController;
CreatedRoomController cRoomController;
pthread_mutex_t sock_mutx;
pthread_mutex_t login_db_mutx;
pthread_mutex_t room_mutx;
pthread_mutex_t matching_mutx;
int log_fds[2];
char log_msg[100];

int main(int argc, char* argv[])
{
	int serv_sock, clnt_sock;
	struct sockaddr_in serv_adr, clnt_adr;
	int clnt_adr_sz;
	cRoomController.cnt = 0;
	pthread_t t_id;

	if (argc != 2) {
		printf("Usage: %s <port>\n", argv[0]);
		exit(1);
	}

	serv_sock = socket(PF_INET, SOCK_STREAM, 0);

	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_adr.sin_port = htons(atoi(argv[1]));

	if (bind(serv_sock, (struct sockaddr*) & serv_adr, sizeof(serv_adr)) == -1)
		error_handling("bind() error");
	if (listen(serv_sock, 10) == -1)
		error_handling("listen() error");

	userController.cnt = 0; // userController init
	create_matching_rooms();   // matching room init
	pipe(log_fds);      // ???????? ?????? ????
	pthread_create(&t_id, NULL, write_log, NULL);   // writing log init
	pthread_detach(t_id);

	while (1) {
		clnt_adr_sz = sizeof(clnt_adr);
		clnt_sock = accept(serv_sock, (struct sockaddr*) & clnt_adr, &clnt_adr_sz);

		pthread_create(&t_id, NULL, handle_clnt, (void*)& clnt_sock);
		pthread_detach(t_id);
		printf("Connected client IP : %s \n", inet_ntoa(clnt_adr.sin_addr));
		sprintf(log_msg, "Connected client IP: %s \n", inet_ntoa(clnt_adr.sin_addr));
		write(log_fds[1], log_msg, strlen(log_msg));
		//handle_clnt(&clnt_sock);
	}
	close(serv_sock);
	return 0;
}

void create_matching_rooms() {
	char name[USER_ID_SIZE] = "matching_room";
	mRoomController.cnt = 0;   // mRoom cnt init;

	for (int i = 0; i < 2; i++) {
		Room* room = (Room*)malloc(sizeof(Room));
		room->id = i + 1;
		strcpy(room->name, name);
		room->name[strlen(name)] = (i + 1) + '0';   // int to char / "matching_room1",...
		room->cnt = 0;

		// room->gameControl init
		memset(&(room->gameController), 0, sizeof(room->gameController));

		// add room at room_list in roomController
		mRoomController.room_list[mRoomController.cnt++] = room;
	}
	sprintf(log_msg, "?????????? ????\n");
	write(log_fds[1], log_msg, strlen(log_msg));
	return;

}

void* write_log() {      // ???? ???? ??????????
	int len;
	FILE* fp = NULL;
	while (1) {
		char buf[100] = { 0, };
		len = read(log_fds[0], buf, sizeof(buf));
		if ((fp = fopen("log.txt", "a")) != NULL) {
			fprintf(fp, "%s", buf);
			fclose(fp);
		}
		else {
			printf("???? ???? ????\n");
		};
	}
	return NULL;
}

void error_handling(char* msg) {
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}

void* handle_clnt(void* arg) {
	int clnt_sock = *((int*)arg);
	int room_num = 0;
	int view_mode = 1;
	int i;
	User* user;

	if ((user = login_view(&clnt_sock)) != NULL) {
		pthread_mutex_lock(&sock_mutx);   // sock mutex lock
		userController.user_list[userController.cnt++] = user;
		pthread_mutex_unlock(&sock_mutx);   // sock mutex unlock


		main_view(user);
	}
	if (user == NULL) {
		printf("?????? ???????? ????????\n");
		sprintf(log_msg, "?????? ???????? ????????\n");
		write(log_fds[1], log_msg, strlen(log_msg));
	}
	else {
		printf("%s : ????????\n", user->id);
		sprintf(log_msg, "%s : ????????\n", user->id);
		write(log_fds[1], log_msg, strlen(log_msg));

		pthread_mutex_lock(&sock_mutx);   // sock mutex lock
		if (userController.cnt == 1) {
			userController.user_list[0] = NULL;
		}
		else {
			for (i = 0; i < userController.cnt; i++) {
				if (strcmp(user->id, userController.user_list[i]->id) == 0) {

					for (; i < userController.cnt; i++) {
						userController.user_list[i] = userController.user_list[i + 1];
					}
					userController.user_list[userController.cnt] = NULL;
					break;
				}
			}
		}
		userController.cnt--;
		pthread_mutex_unlock(&sock_mutx);   // sock mutex unlock
		free(user);
	}
	close(clnt_sock);

	return NULL;
}

User* login_view(void* arg) {
	int clnt_sock = *((int*)arg);
	User* user = (User*)malloc(sizeof(User));
	char msg[BUF_SIZE];
	char answer[ANSWER_SIZE];
	int login_result = 0;
	int str_len = 0;

	memset(user, 0, sizeof(user));   // user init

	while (1) {
		str_len = read(clnt_sock, answer, sizeof(answer));
		if (str_len == -1) // read() error
			return 0;
		answer[str_len - 1] = '\0';

		switch (answer[0]) {
		case '1':
			login_result = login(&clnt_sock, user);

			if (login_result == 1)
				return user;   // user ?????? ????
			else
				break;
		case '2':
			sign_up(&clnt_sock);
			break;
		case '3':
			printf("?????????? ????\n");
			return NULL;
		default:
			break;
		}
	}

	return NULL;
}

int login(void* arg, User* user) {
	int clnt_sock = *((int*)arg);
	char mode[5] = "r";
	char uid[USER_ID_SIZE] = { 0 };
	char upw[USER_ID_SIZE] = { 0 };
	char answer[ANSWER_SIZE] = "0\n";
	char msg[BUF_SIZE] = { 0 };
	char* str = NULL;
	int verify_result = 0;
	int str_len = 0;
	int i;
	// ???????????? ?????? ???????? ????
	str_len = read(clnt_sock, msg, sizeof(msg));
	if (str_len == -1) // read() error
		return 0;
	printf("%s\n", msg);

	str = strtok(msg, "\n");   // "\n" ???? ???? ??????
	strncpy(uid, str, strlen(str));
	fprintf(stdout, "uid: %s\n", uid);

	printf("remain msg: %s\n", msg);
	str = strtok(NULL, "\n");   // ?????? ????????
	strncpy(upw, str, strlen(str));
	fprintf(stdout, "upw: %s\n", upw); // ??????/???????? ????

	if ((verify_result = rw_login_db(mode, uid, upw)) == 1) {
		for (i = 0; i < userController.cnt; i++) {
			if (strcmp(userController.user_list[i]->id, uid) == 0) {
				printf("???? ???????? ??????!\n");
				write(clnt_sock, "2\n", sizeof("2\n"));
				return 2;
			}
		}
		answer[0] = '1';
		strncpy(user->id, uid, strlen(uid));   // ???? ?????? ????
		user->sock = clnt_sock;   // ???????? ?????? ???????? ????

		printf("%s : ?????? ????\n", user->id);
		sprintf(log_msg, "%s : ?????? ????\n", user->id);
		write(log_fds[1], log_msg, strlen(log_msg));
	}

	str_len = write(clnt_sock, answer, strlen(answer));
	if (str_len == -1) // write() error
		return 0;

	return verify_result;
}

int sign_up(void* arg) {
	int clnt_sock = *((int*)arg);
	char mode[3] = "r+";   // needed for login_db fopen
	char uid[USER_ID_SIZE] = { 0, };
	char upw[USER_ID_SIZE] = { 0, };
	int sign_up_result = 0;
	char answer[ANSWER_SIZE] = { 0, };
	char msg[BUF_SIZE] = { 0, };
	char* str;
	int str_len = 0;

	//str_len = read(clnt_sock, answer, sizeof(answer));
	//if (str_len == -1) // read() error
	   //return 0;
	//printf("str_:%d, str:%ld, %s\n", str_len, strlen(answer), answer);

	//if (!(strcmp(answer, "y\n")) || !(strcmp(answer, "Y\n"))) {
	str_len = read(clnt_sock, msg, sizeof(msg));
	if (str_len == -1) // read() error
		return 0;
	//printf("str_:%d, str:%ld\n", str_len, strlen(upw));

	str = strtok(msg, "\n");   // "\n" ???? ???? ??????
	strncpy(uid, str, strlen(str));
	fprintf(stdout, "uid: %s\n", uid);
	str = strtok(NULL, "\n");   // ?????? ????????
	strncpy(upw, str, strlen(str));
	fputs(upw, stdout);

	// DB???? ?????? ???? ???? ?? ????
	sign_up_result = rw_login_db(mode, uid, upw);

	if (sign_up_result == 1) {
		answer[0] = '1';
		printf("???? ????: %s\n", uid);
		sprintf(log_msg, "???? ???? ????: %s\n", uid);
		write(log_fds[1], log_msg, strlen(log_msg));
	}
	else {
		answer[0] = '0';
		printf("???? ???? ????\n");
		sprintf(log_msg, "???? ???? ????\n");
		write(log_fds[1], log_msg, strlen(log_msg));
	}

	str_len = write(clnt_sock, answer, strlen(answer));
	if (str_len == -1) // write() error
		return 0;

	return sign_up_result;
}

int main_view(User* user) {
	int clnt_sock = user->sock;
	char answer[ANSWER_SIZE] = { 0, };
	Room* room;
	int str_len = 0;

	printf("%s : ???? ????\n", user->id);
	sprintf(log_msg, "%s : ???? ????\n", user->id);
	write(log_fds[1], log_msg, strlen(log_msg));

	while (1) {
		str_len = read(clnt_sock, answer, sizeof(answer));
		if (str_len == -1) // read() error
			return 0;
		answer[str_len - 1] = '\0';

		switch (answer[0]) {
		case '1':      // ????
			if ((room = get_matching_room(user)) != NULL) {
				room_view(user, room);
			}
			break;
		case '2':
			if ((room = get_create_room(user)) != NULL) {
				room_view(user, room);
				if (room->cnt == 0) delete_room(room);
			}
			break;
		case '3':
			enter_created_room(user);
			break;
		case '4':
			search_room(user);
			break;
		case '5':
			search_user(user);
			break;
		case '6':
			print_ranking(user);
			break;
		case '7':
			printf("%s ????????\n", user->id);
			return 0;
		default:
			break;
		}
	}

	return 0;
}

Room* get_matching_room(User* user) {
	int clnt_sock = user->sock;
	Room* room;
	int entered = 0;
	int in_room_cnt = 0;

	printf("%s : ???? ????\n", user->id);
	sprintf(log_msg, "%s : ???? ????\n", user->id);
	write(log_fds[1], log_msg, strlen(log_msg));

	// mutex lock
	pthread_mutex_lock(&matching_mutx);
	for (int i = 0; i < mRoomController.cnt; i++) {
		in_room_cnt = mRoomController.room_list[i]->cnt;
		if (in_room_cnt < 2) {   // empty space in room
			room = mRoomController.room_list[i];
			room->clnt_sock[in_room_cnt] = user->sock;
			memcpy(room->user_id[in_room_cnt], user->id, sizeof(user->id));
			room->cnt++;
			entered = 1;
			printf("???? ???? ???? %d?? %d??\n", i, room->cnt);
			break;
		}
	}
	pthread_mutex_unlock(&matching_mutx);
	// mutex unlock
	if (entered == 1) {
		printf("%s : ???? ????\n", user->id);
		sprintf(log_msg, "%s : ???? ????\n", user->id);
		write(log_fds[1], log_msg, strlen(log_msg));
		return room;
	}

	return NULL;
}

Room* get_create_room(User* user) {
	Room* room = (Room*)malloc(sizeof(Room));   //?? ????
	char name[21];
	char str_len;
	room->cnt = 1;
	room->clnt_sock[0] = user->sock;
	memcpy(room->user_id[0], user->id, sizeof(user->id));

	str_len = read(user->sock, name, sizeof(name));
	if (str_len == -1)
		return 0;
	sprintf(room->name, "%s", name);

	pthread_mutex_lock(&room_mutx);
	room->id = cRoomController.cnt++;
	cRoomController.room_list[room->id] = room;
	pthread_mutex_unlock(&room_mutx);
	printf("%s : ?? ???? -> %d %s", user->id, room->id, room->name);
	sprintf(log_msg, "%s : ?? ???? -> %d %s", user->id, room->id, room->name);
	write(log_fds[1], log_msg, strlen(log_msg));

	return room;
}

int enter_created_room(User* user) {
	char msg[21] = { 0 };
	char* str;
	Room* room;
	int num = -1, str_len = 0;
	int in_room_cnt = 0;
	int i, entered = 0;

	str_len = read(user->sock, msg, sizeof(msg));      // ?? ???? ???? ???? ????
	if (str_len == -1)   // read() error
		return 0;
	printf("%s : ?? ???? ???? -> %s", user->id, msg);
	sprintf(log_msg, "%s : ?? ???? ???? -> %s", user->id, msg);
	write(log_fds[1], log_msg, strlen(log_msg));

	str = strtok(msg, "\n");
	msg[str_len] = 0;
	if ((int)msg[0] < 58 && (int)msg[0] > 47)   // ?????? ?????????? int?? ????
		num = atoi(str);
	strcat(msg, "\n");

	pthread_mutex_lock(&room_mutx);
	for (i = 0; i < cRoomController.cnt; i++) {
		if ((strcmp(msg, cRoomController.room_list[i]->name) == 0) ||
			(num == cRoomController.room_list[i]->id)) {      // ???? ?????? ????
			in_room_cnt = cRoomController.room_list[i]->cnt;
			if (in_room_cnt >= 2) {
				printf("???? ??????\n");
				break;
			}
			else {
				printf("???? ????\n");
				room = cRoomController.room_list[i];
				room->clnt_sock[in_room_cnt] = user->sock;
				memcpy(room->user_id[1], user->id, sizeof(user->id));
				room->cnt++;
				entered = 1;
				break;
			}
		}
	}
	pthread_mutex_unlock(&room_mutx);
	if (entered == 1) {
		printf("????????\n");
		sprintf(log_msg, "%s : ?? ???? ???? -> %s", user->id, room->name);
		write(log_fds[1], log_msg, strlen(log_msg));
		str_len = write(user->sock, "1\n", sizeof("1\n"));
		room_view(user, room);
		if (room->cnt == 0) delete_room(room);
	}
	else {
		printf("???? ????\n");
		sprintf(log_msg, "%s : ???? ???? -> %s", user->id, msg);
		write(log_fds[1], log_msg, strlen(log_msg));
		str_len = write(user->sock, "0\n", sizeof("0\n"));
	}
	return 0;
}

void search_room(User* user) {
	int i;
	char buf[100];
	char list[1000] = { 0 };
	printf("%s : ?? ???? ???? : %d??\n", user->id, cRoomController.cnt);
	sprintf(log_msg, "%s : ?? ???? ???? : %d??\n", user->id, cRoomController.cnt);
	write(log_fds[1], log_msg, strlen(log_msg));
	if (cRoomController.cnt == 0) {
		write(user->sock, " ", sizeof(" "));
	}
	else {
		for (i = 0; i < cRoomController.cnt; i++) {
			strcat(list, itoa((cRoomController.room_list[i]->id), buf, 10));
			strcat(list, " ");
			strcat(list, cRoomController.room_list[i]->name);
		}
		write(user->sock, list, strlen(list));
	}
	return;
}

void search_user(User* user) {      //???????? ?????? ????
	int i;
	char list[1000] = { 0 };

	printf("%s : ???? ???? ???? : %d??\n", user->id, userController.cnt);
	sprintf(log_msg, "%s : ???? ???? ???? : %d??\n", user->id, userController.cnt);
	write(log_fds[1], log_msg, strlen(log_msg));

	for (i = 0; i < userController.cnt; i++) {      //???? ?????? ????
		strcat(list, userController.user_list[i]->id);
		strcat(list, "\n");
	}
	write(user->sock, list, strlen(list));      //?????????????? ????
	return;
}

void print_ranking(User* user) {
	Stat stat[MAX_CLNT] = { 0 };
	Stat temp;
	char rankingme[100] = { 0 };
	char rankingall[1000] = { 0 };
	int record = 0;
	int i, j;
	FILE* fp = NULL;

	if ((fp = fopen("ranking_db.txt", "r")) == NULL) {
		error_handling("fopen() error");
	}
	else {
		while ((fscanf(fp, "%s %d %d %d\n", stat[record].id, &stat[record].win, &stat[record].draw, &stat[record].lose)) != EOF) {
			record++;
		}
	}
	fclose(fp);

	for (i = record - 1; i > 0; i--) {
		for (j = 0; j < i; j++) {
			if (stat[j].win < stat[j + 1].win) {   // ?????? ???????? ????????
				temp = stat[j + 1];
				stat[j + 1] = stat[j];
				stat[j] = temp;
			}
			// ?????? ?????? ???????? ??????
			else if (stat[j].win == stat[j + 1].win && stat[j].lose > stat[j + 1].lose) {
				temp = stat[j + 1];
				stat[j + 1] = stat[j];
				stat[j] = temp;
			}
			// ?????? ???? ?????? ???????? ??????
			else if (stat[j].win == stat[j + 1].win && stat[j].lose == stat[j + 1].lose && stat[j].draw < stat[j + 1].draw)
			{
				temp = stat[j + 1];
				stat[j + 1] = stat[j];
				stat[j] = temp;
			}
		}
	}
	for (i = 0; i < record; i++) {      //???? ?????? ????
		sprintf(rankingme, "%3d?? %15s %5d %6d %6d", i + 1, stat[i].id, stat[i].win, stat[i].draw, stat[i].lose);
		strcat(rankingall, rankingme);
		strcat(rankingall, "\n");
	}
	write(user->sock, rankingall, strlen(rankingall));      //?????????????? ????
}

int room_view(User* user, Room* room) {
	int buf = 0;
	printf("?? ????\n");
	printf("%d??\n", room->cnt);

	while (1) {
		buf = (waiting_user_ready(user, room));
		if (buf == 1) {
			start_game(user, room);
		}
		else if (buf == 2) {
			while (room->gameController.ready_state[0] == 1) {
				sleep(1);
			}
		}
		else
			break;
	}

	return 0;
}

int waiting_user_ready(User* user, Room* room) {
	int clnt_sock = user->sock;
	char msg[BUF_SIZE];
	int str_len = 0;
	char* str;
	char start_msg[] = "???? ????: /r\t ?? ??????: /q\t ??????: /?\n";

	write(user->sock, start_msg, sizeof(start_msg));

	while (1) {
		str_len = read(clnt_sock, msg, sizeof(msg));
		if (str_len == -1) // read() error
			return 0;
		msg[str_len] = '\0';
		printf("msg : %s", msg);
		// message control
		if (msg[0] == '/') {
			if (strncmp(msg, "/q\n", strlen("/q\n")) == 0 || strncmp(msg, "/Q\n", strlen("/Q\n")) == 0) {
				if (room->clnt_sock[0] == clnt_sock) {
					write(room->clnt_sock[1], "???????? ???? ??????????.\n", sizeof("???????? ???? ??????????.\n"));
					room->clnt_sock[0] = room->clnt_sock[1];
				}
				else {
					write(room->clnt_sock[0], "???????? ???? ??????????.\n", sizeof("???????? ???? ??????????.\n"));
				}
				room->clnt_sock[1] = -1;
				room->cnt--;
				str_len = write(clnt_sock, "/q\n", strlen("/q\n"));
				printf("???? ?? ?????? ????\n");
				return 0;
			}
			else if (strcmp(msg, "/?\n") == 0) {
				write(user->sock, start_msg, sizeof(start_msg));      //??????
			}
			else if (strncmp(msg, "/r\n", strlen("r\n")) == 0 || strncmp(msg, "/R\n", strlen("/R\n")) == 0) {
				if (clnt_sock == room->clnt_sock[0]) {
					room->gameController.ready_state[0] = 1;
					if (room->gameController.ready_state[1] != 1) {
						write(room->clnt_sock[1], "???????? ??????????????\n", strlen("???????? ??????????????\n"));
					}
					while (room->gameController.ready_state[1] != 1) {
						sleep(1);
					}
					return 2;
				}
				else {
					room->gameController.ready_state[1] = 1;
					if (room->gameController.ready_state[0] != 1) {
						write(room->clnt_sock[0], "???????? ??????????????\n", strlen("???????? ??????????????\n"));
					}
					while (room->gameController.ready_state[0] != 1) {
						sleep(1);
					}
				}

				if (room->gameController.ready_state[0] && room->gameController.ready_state[1]) {
					write(room->clnt_sock[0], "/r\n", strlen("/r\n"));
					write(room->clnt_sock[1], "/r\n", strlen("/r\n"));
					printf("%s ???? ???? -> ??:%d %s\n", user->id, room->id, room->name);
					sprintf(log_msg, "???? ???? -> ??:%d %s\n", room->id, room->name);
					write(log_fds[1], log_msg, strlen(log_msg));
					return 1;
				}
			}

			continue;
		}

		if (room->cnt == 2) {   // ???? ???????? ???? ?? ???? ???????? ?????? ????
			if (clnt_sock == room->clnt_sock[0] && room->gameController.ready_state[1] != 1) {
				str_len = write(room->clnt_sock[1], msg, str_len);
				if (str_len == -1) // write() error
					return 0;
			}
			else if (clnt_sock == room->clnt_sock[1] && room->gameController.ready_state[0] != 1) {
				str_len = write(room->clnt_sock[0], msg, str_len);
				if (str_len == -1) // write() error
					return 0;
			}
		}
	}

	return 0;
}

int start_game(User* user, Room* room) {
	int str_len = 0;
	char user_num[5] = { 0 };
	char msg[BUF_SIZE] = { 0 };
	char remain_msg[BUF_SIZE * 2] = { 0 };
	int ball = 0, strike = 0;
	int user1_win = 0, user2_win = 0;
	int cnt = 1;
	char result[BUF_SIZE] = { 0 };

	room->gameController.turn = 0;

	str_len = read(room->clnt_sock[0], user_num, sizeof(user_num));
	strncpy(room->gameController.answer_list[0], user_num, sizeof(room->gameController.answer_list[0]));
	printf("user1_num: %s\n", room->gameController.answer_list[0]);

	str_len = read(room->clnt_sock[1], user_num, sizeof(user_num));
	strncpy(room->gameController.answer_list[1], user_num, sizeof(room->gameController.answer_list[1]));
	printf("user2_num: %s\n", room->gameController.answer_list[1]);



	while (1) {
		if (room->gameController.turn == 0) {
			printf("turn1\n");
			str_len = write(room->clnt_sock[0], "/turn1\n", strlen("/turn1\n"));
		}
		else {
			printf("turn2\n");
			str_len = write(room->clnt_sock[1], "/turn2\n", strlen("/turn2\n"));
		}

		str_len = read(room->clnt_sock[room->gameController.turn], user_num, sizeof(user_num));
		user_num[str_len] = '\0';
		if (str_len == 0)   // read() error
			return 0;
		printf("attack num: %s", user_num);

		ball = 0; strike = 0;   // ball strike init

		for (int i = 0; i < 3; i++) {
			for (int j = 0; j < 3; j++) {
				if (user_num[i] == room->gameController.answer_list[!room->gameController.turn][j]) {
					if (i == j)
						strike++;
					else
						ball++;
				}
			}
		}
		printf("????: %dS %dB\n", strike, ball);
		sprintf(result, "[????] %dS %dB\n", strike, ball);

		if (room->gameController.turn == 0) {
			if (strike == 3) {
				user1_win = 1;
				// ?????? ???????? ???? ???????? ????
				sprintf(msg, "%s??????????\n???????? ?????? ???? ?? ??????????\n", result);
				write(room->clnt_sock[0], msg, strlen(msg));
			}
			else {
				write(room->clnt_sock[0], result, strlen(result));
			}

			sprintf(remain_msg, "\n???? %d????\n?????? ????:%s%s", cnt, user_num, result);
			if (user1_win)
				sprintf(remain_msg, "%s???????? ?????? ????????????\n?????? ??????????\n", remain_msg);
			write(room->clnt_sock[1], remain_msg, strlen(remain_msg));
			str_len = read(room->clnt_sock[1], remain_msg, sizeof(2));

		}
		else if (room->gameController.turn == 1) {
			if (strike == 3) {
				user2_win = 1;
				sprintf(msg, "%s??????????\n", result);
				write(room->clnt_sock[1], msg, strlen(msg));
			}
			else {
				write(room->clnt_sock[1], result, strlen(result));
			}

			sprintf(remain_msg, "\n???? %d????\n?????? ????:%s%s", cnt++, user_num, result);
			if (user2_win)
				sprintf(remain_msg, "%s???????? ?????? ????????????\n", remain_msg);
			write(room->clnt_sock[0], remain_msg, strlen(remain_msg));
			str_len = read(room->clnt_sock[0], remain_msg, sizeof(2));
			if (user1_win == 1 || user2_win == 1)
				break;
		}

		room->gameController.turn = !room->gameController.turn;
	}

	printf("???? ????. -> ??:%d %s", room->id, room->name);
	sprintf(log_msg, "???? ????. -> ??:%d %s", room->id, room->name);
	write(log_fds[1], log_msg, strlen(log_msg));

	if (user2_win) {
		if (user1_win) {
			write(room->clnt_sock[0], "/draw\n", strlen("/draw\n"));
			write(room->clnt_sock[1], "/draw\n", strlen("/draw\n"));
			printf("%s %s??????\n", room->user_id[0], room->user_id[1]);
			sprintf(log_msg, "%s %s??????\n", room->user_id[0], room->user_id[1]);
			write(log_fds[1], log_msg, strlen(log_msg));
			input_rank(room->user_id[0], 2);
			input_rank(room->user_id[1], 2);
		}
		else {
			write(room->clnt_sock[1], "/win\n", strlen("/win\n"));
			write(room->clnt_sock[0], "/lose\n", strlen("/lose\n"));
			printf("%s ???? %s ????\n", room->user_id[1], room->user_id[0]);
			sprintf(log_msg, "%s ???? %s ????\n", room->user_id[1], room->user_id[0]);
			write(log_fds[1], log_msg, strlen(log_msg));
			input_rank(room->user_id[1], 1);
			input_rank(room->user_id[0], 3);
		}
	}
	else if (user1_win) {
		write(room->clnt_sock[1], "/lose\n", strlen("/lose\n"));
		write(room->clnt_sock[0], "/win\n", strlen("/win\n"));
		printf("%s ???? %s ????\n", room->user_id[0], room->user_id[1]);
		sprintf(log_msg, "%s ???? %s ????\n", room->user_id[0], room->user_id[1]);
		write(log_fds[1], log_msg, strlen(log_msg));
		input_rank(room->user_id[1], 3);
		input_rank(room->user_id[0], 1);
	}
	memset(&(room->gameController), 0, sizeof(room->gameController));   // ???? ?????? ???????? ??????
	return 0;
}

int input_rank(char id[USER_ID_SIZE], int outcome) {

	FILE* fp = fopen("ranking_db.txt", "rt+");
	if (fp == NULL) error_handling("fopen() error");
	else
	{
		long seek;
		char temp_id[USER_ID_SIZE] = { 0 };
		int w = 0, d = 0, l = 0;
		while (1)
		{
			seek = ftell(fp);
			if (fscanf(fp, "%s %d %d %d\n", temp_id, &w, &d, &l) == EOF) break;
			if (!strcmp(temp_id, id)) {
				switch (outcome) {
				case 1: w++; break;
				case 2:   d++; break;
				case 3:   l++; break;
				default: break;
				}
				fseek(fp, seek, SEEK_SET);
				fprintf(fp, "%s %d %d %d\n", temp_id, w, d, l);
				fflush(fp);
			}
		}
		rewind(fp);
		fclose(fp);
	}
}

void delete_room(Room* room) {
	int i;
	for (i = 0; i < mRoomController.cnt; i++) {
		if (mRoomController.room_list[i] == room) {

		}
	}
	printf("?? ???? -> %d %s", room->id, room->name);
	sprintf(log_msg, "?? ???? -> %d %s", room->id, room->name);
	write(log_fds[1], log_msg, strlen(log_msg));
	pthread_mutex_lock(&room_mutx);
	if (cRoomController.cnt == 1) {
		cRoomController.room_list[0] = NULL;
	}
	else {
		for (i = room->id; i < cRoomController.cnt - 1; i++) {      // ?? ?????? ????
			cRoomController.room_list[i] = cRoomController.room_list[i + 1];
			cRoomController.room_list[i]->id--;
		}
		cRoomController.room_list[cRoomController.cnt - 1] = NULL;   //?? ???? ???? ????
	}
	cRoomController.cnt--;
	pthread_mutex_unlock(&room_mutx);
	free(room);      //?? ????
	return;
}

int rw_login_db(char* rw, char* id, char* pw) {
	FILE* fp = NULL;
	FILE* rank_fp = NULL;
	char mode[5];
	char uid[USER_ID_SIZE] = { 0, };
	char upw[USER_ID_SIZE] = { 0, };
	char get_id[USER_ID_SIZE] = { 0, };
	char get_pw[USER_ID_SIZE] = { 0, };
	int is_duplicated_id = 0;
	int result = 0;

	memcpy(mode, rw, strlen(rw));
	memcpy(uid, id, strlen(id));
	memcpy(upw, pw, strlen(pw));

	pthread_mutex_lock(&login_db_mutx); // login_db.txt mutx lock
	if ((fp = fopen("login_db.txt", mode)) == NULL) {
		error_handling("fopen() error");
	}
	printf("\ndb???? ????\n");
	if (strncmp(mode, "r", strlen(mode)) == 0) {   // ???????? ?????????? mode?? read ????
	   // login_db.txt ???? ?????? ?????? "id pw\n"
		while (fscanf(fp, "%s %s\n", get_id, get_pw) != EOF) {
			if (strncmp(uid, get_id, strlen(get_id)) == 0) {   // ?????? ????
				printf("?????? ????\n");
				if (strncmp(upw, get_pw, strlen(get_pw)) == 0) {   // ???????? ????
					printf("???????? ????\n");
					result = 1;   // true
				}
			}
		}
	}
	else if (strncmp(mode, "r+", strlen(mode)) == 0) {   // ?????????? ?????????? mode?? rw
	   // login_db.txt ???? ?????? ??????
		while (fscanf(fp, "%s %s\n", get_id, get_pw) != EOF) {
			if (strncmp(uid, get_id, strlen(get_id)) == 0) {   // ?????? ????
				printf("?????? ????\n");
				is_duplicated_id = 1;            // ?????? ???? ????
				break;
			}
		}
		// ???????? ???????? ?????? login_db.txt?? ?????? ???? ????
		if (!(is_duplicated_id)) {
			printf("???? ????\n");
			fprintf(fp, "%s %s\n", uid, upw);   // login_db.txt.?? ???? ????
			if ((rank_fp = fopen("ranking_db.txt", "at+")) == NULL) {   // rankig_db.txt open
				error_handling("fopen() error");
			}
			fprintf(rank_fp, "%s 0 0 0\n", uid);   // ???? ?????? ????
			fclose(rank_fp);            // ranking_db.txt close
			result = 1;   // true
		}
	}
	fclose(fp);                     // login_db.txt close
	pthread_mutex_unlock(&login_db_mutx); // login_db.txt mutx unlock

	return result;   // Y == 1 or N == 0
}

char* itoa(long val, char* buf, unsigned radix)
{
	char* p;
	char* firstdig;
	char temp;
	unsigned digval;
	p = buf;

	if (radix == 10 && val < 0) {
		*p++ = '-';
		val = (unsigned long)(-(long)val);
	}

	firstdig = p;    /* save pointer to first digit */

	do {
		digval = (unsigned)(val % radix);
		val /= radix;
		if (digval > 9)
			* p++ = (char)(digval - 10 + 'a');
		else
			*p++ = (char)(digval + '0');
	} while (val > 0);

	*p-- = '\0';

	do {
		temp = *p;
		*p = *firstdig;
		*firstdig = temp;
		--p;
		++firstdig;
	} while (firstdig < p);

	return buf;
}