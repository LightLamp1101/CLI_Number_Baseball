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
int create_room(void* arg, User* user);      //�� �����
int enter_room(void* arg, User* user);      //�� ����
int search_room(void* arg);      //�� ��� ����
int search_user(void* arg);      //������ ����� ����
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
	if ((user = login_view(&sock)) != NULL) {   // �α��� ����
		while (end == 1) {
			end = main_view(&sock, user);
		}
	}
	return NULL;
}

// ���� �� ù ȭ������ �α��θ� ���� ��
User* login_view(void* arg) {
	int sock = *((int*)arg);
	User* user = (User*)malloc(sizeof(User));
	char msg[BUF_SIZE];
	char answer[ANSWER_SIZE];
	int login_result = 0;

	while (1) {
		fputs("\n* * * * * * * * * * * * * * * * *\n", stdout);
		fputs("*                               *\n", stdout);
		fputs("*        ���ڰ� ���ھ߱�        *\n", stdout);
		fputs("*                               *\n", stdout);
		fputs("* * * * * * * * * * * * * * * * *\n", stdout);
		fputs("\n\n=== �� �� ===\n", stdout);
		fputs("\n1. �α���\n", stdout);
		fputs("\n2. ���� ����\n", stdout);
		fputs("\n3. �� ��\n", stdout);
		fputs("\n�޴��� �������ּ���. (1-3). ? ", stdout);
		fflush(stdout);

		fgets(answer, sizeof(answer), stdin);
		write(sock, answer, sizeof(answer));
		fflush(stdin);

		switch (answer[0]) {
		case '1': // �α���
			login_result = login(&sock, user);   // �α��� ���

			if (login_result == 1) {   // �α��� ����
				return user;
			}
			else   // �α��� ����
				break;
		case '2': // ���� ����
			sign_up(&sock);
			break;
		case '3':
			fputs("\n������ ����˴ϴ�.\n", stdout);
			return NULL;   // ���� ��û
		default:
			system("clear");
			fputs("\n���� �� �Է����ּ���. (1-3) ? \n", stdout);
			break;
		}
	}

	return NULL;
}

// �α����� ���� ���̵�/��й�ȣ�� ����ڷκ��� �Է¹���
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
	fputs("*        ���ڰ� ���ھ߱�        *\n", stdout);
	fputs("*                               *\n", stdout);
	fputs("* * * * * * * * * * * * * * * * *\n", stdout);
	fputs("\n\n--- �α��� ---\n", stdout);

	fputs("\n���̵� : ", stdout);
	fgets(id, sizeof(id), stdin); // ���̵� �Է�

	fputs("\n��й�ȣ : ", stdout);
	fgets(pw, sizeof(pw), stdin); // ��й�ȣ �Է�

	sprintf(msg, "%s%s", id, pw);
	write(sock, msg, sizeof(msg));

	system("clear");
	printf("�α�����...\n");
	sleep(0.5);
	read(sock, answer, sizeof(answer));
	if ((answer[0]) == '1') {   // �α��� ����
		strncpy(user->id, id, strlen(id) - 1);
		fputs("\n�α��� ����!\n", stdout);
		login_result = 1;
	}
	else if (strcmp(answer, "2\n") == 0) {
		fputs("\n�̹� �������� ���̵� �Դϴ�!\n", stdout);
	}
	else {
		fputs("\n���̵�/��й�ȣ�� Ȯ�����ּ���.\n", stdout);
	}

	return login_result;   // 1 or 0
}

// ȸ�������� ���� ���̵�/��й�ȣ�� ����ڷκ��� �Է¹���
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
	fputs("*        ���ڰ� ���ھ߱�        *\n", stdout);
	fputs("*                               *\n", stdout);
	fputs("* * * * * * * * * * * * * * * * *\n", stdout);
	fputs("\n\n--- �������� ---\n", stdout);

	fputs("\n���̵� : ", stdout);
	fgets(id, sizeof(id), stdin);

	fputs("\n��й�ȣ : ", stdout);
	fgets(pw, sizeof(pw), stdin);

	fputs("\n���� ������ �����Ͻðڽ��ϱ� (Y/N) ? ", stdout);
	fgets(answer, sizeof(answer), stdin);
	system("clear");

	if (strcmp(answer, "Y\n") || strcmp(answer, "y\n")) {
		sprintf(id_pw, "%s%s", id, pw);
		write(sock, id_pw, strlen(id_pw));
		printf("���� ������...\n");
		read(sock, answer, sizeof(answer));
		if ((answer[0]) == '1') {   // �������� ����
			fputs("\n�������� ����!\n", stdout);
			sign_up_result = 1;
		}
		else {
			fputs("\n�̹� �����ϴ� ���̵��Դϴ�.\n", stdout);
		}
	}

	return sign_up_result;
}



int main_view(void* arg, User* user) {
	int sock = *((int*)arg);
	static int stat = 0;
	char answer[ANSWER_SIZE];
	char msg[] = "\n = = = ��  �� = = =\n1. ��Ī���� ���� �����ϱ�\n2. �� �����ϱ�\n3. �� �����ϱ�\n4. ���� ������ �� ��ȸ\n5. ���� ����� ��ȸ\n6. ��ŷ ��ȸ\n7. �α׾ƿ�\n0. ����\n";
	fputs("\n* * * * * * * * * * * * * * * * *\n", stdout);
	fputs("*                               *\n", stdout);
	fputs("*        ���ڰ� ���ھ߱�        *\n", stdout);
	fputs("*                               *\n", stdout);
	fputs("* * * * * * * * * * * * * * * * *\n", stdout);
	if (stat++ == 0) printf("\n%s �� ȯ���մϴ�!\n", user->id);
	printf("%s", msg);

	while (1) {
		fputs("\n�޴��� �������ּ���. (0-7)\n", stdout);
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
			printf("������ �����մϴ�.\n\n");
			return 0;
		case '0':
			system("clear");
			fputs("\n* * * * * * * * * * * * * * * * *\n", stdout);
			fputs("*                               *\n", stdout);
			fputs("*        ���ڰ� ���ھ߱�        *\n", stdout);
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
	printf("��Ī�� ����\n");
	room_view(&sock, user);

	return 0;
}

int create_room(void* arg, User* user) {      //�� ����
	int sock = *(int*)arg;
	char name[21];
	int str_len = 0;
	fputs("\n������ �� �̸� : ", stdout);
	fgets(name, sizeof(name), stdin);
	name[strlen(name)] = 0;
	system("clear");
	printf("�� ������... -> %s", name);
	str_len = write(sock, name, sizeof(name));
	if (str_len == -1)
		return 0;

	printf("���� ����\n");
	room_view(&sock, user);

	return 0;
}

int enter_room(void* arg, User* user) {
	int sock = *(int*)arg;
	char name[21];
	char msg[BUF_SIZE];
	char answer[ANSWER_SIZE];
	int str_len = 0;

	fputs("\n������ ���� ��ȣ �Ǵ� �̸��� �Է����ּ���. ", stdout);
	fgets(name, sizeof(name), stdin);
	str_len = write(sock, name, sizeof(name));
	if (str_len == -1)
		return 0;

	system("clear");
	printf("�� ������... -> %s", name);

	read(sock, answer, sizeof(answer));   // ���� ��� ����

	if (strcmp(answer, "1\n") == 0) {   // �� ����
		printf(" ���� ����!\n");
		room_view(&sock, user);
	}
	else {      // �κ��
		printf(" ���� ����!\n");
	}
	return 0;
}

int search_room(void* arg) {      // �� id name ���
	int sock = *(int*)arg;
	char msg[1000];
	int str_len = 0;

	fputs("\n�� ��ȸ\n", stdout);
	str_len = read(sock, msg, sizeof(msg));      // �� ��� �б�
	if (str_len == -1)   // read() error
		return 0;
	msg[str_len] = 0;
	fputs(msg, stdout);
	return 0;
}

int search_user(void* arg) {   // �������� ����� ��ȸ
	int sock = *(int*)arg;
	char msg[1000];
	int str_len = 0;

	printf("\n����� ��ȸ\n");
	str_len = read(sock, msg, sizeof(msg));      // ����� ��� �б�
	if (str_len == -1)   // read() error
		return 0;

	msg[str_len] = 0;
	fputs(msg, stdout);
	return 0;
}

int ranking(void* arg) {
	int sock = *(int*)arg;
	char msg[1000];

	fputs("\n��ŷ��ȸ\n", stdout);
	fputs(" ��ŷ            ID     �¸�  ���º�  �й�\n", stdout);
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

	printf("�� ����\n");
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

	printf("ä�ý���\n");

	pthread_create(&snd_thread, NULL, send_msg, (void*)multiple_arg);
	pthread_create(&rcv_thread, NULL, recv_msg, (void*)& sock);
	pthread_join(snd_thread, &snd_thread_return);
	pthread_join(rcv_thread, &rcv_thread_return);

	printf("ä������\n");

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

	strncpy(uid, mArg.id, sizeof(mArg.id)); // id ����
	sprintf(start_msg, "%s���� �����Ͽ����ϴ�.\n", mArg.id);
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
				printf("�غ�Ǿ����ϴ�.\n");
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
				printf("���� �����ϴ�.\n");
				return 0;
			}
			else if (strncmp(name_msg, "/r\n", strlen("/r\n")) == 0) {
				printf("���ӽ���\n");
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

	while (1) {   // ������ ���ߴ� ����ڰ� ���ö����� �ݺ�
		if (turn) {
			while (1) { // �������� ���ڸ� �Է¹ޱ� ���� �ݺ�
				if (first == 1) {
					fprintf(stdout, "%s�� 1���� 9������ 3�ڸ� ���ڸ� �Է����ּ���. (ex: 123) : ", user->id);
					first--;
				}
				else {
					printf("������ ���ڸ� �Է����ּ��� : ");
				}
				fgets(user_num, sizeof(user_num), stdin);
				if (user_num[0] < '1' || user_num[0] > '9' || user_num[1] < '1' || user_num[1] > '9' || user_num[2] < '1' || user_num[2] > '9')
				{ // �Է��� ���ڰ� 1 ~ 9 ���ڰ� �ƴϸ� �ٽ� �Է¹޵��� 
					printf("���� ���� ���ڸ� �Է��Ͻø� �ȵ˴ϴ�.\n");
					continue;
				}
				else if (user_num[0] == user_num[1] || user_num[0] == user_num[2] || user_num[1] == user_num[2])
				{ // �Է��� ���� �߿� �ߺ��� �� ������ �ٽ� �Է¹޵��� 
					printf("�ߺ��� ���ڸ� �Է��Ͻø� �ȵ˴ϴ�.\n");
					continue;
				}
				break; // �ƹ� ���� ���� ��� �ݺ� ���� 
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
					printf("\n�¸��ϼ̽��ϴ�!!\n");
					return 0;
				}
				else if (strncmp(msg, "/draw\n", strlen("/draw\n")) == 0) {
					printf("\n�����ϴ�!!\n");
					return 0;
				}
				else if (strncmp(msg, "/lose\n", strlen("/lose\n")) == 0) {
					printf("\n�й��ϼ̽��ϴ�!!\n");
					return 0;
				}
				else if (strncmp(msg, "/turn1\n", strlen("/turn1\n")) == 0) {
					turn = 1;
					printf("\n���� %dȸ��\n", cnt);
					cnt++;
					break;
				}
				else if (strncmp(msg, "/turn2\n", strlen("/turn2\n")) == 0) {
					turn = 1;
					printf("\n���� %dȸ��\n", cnt);
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