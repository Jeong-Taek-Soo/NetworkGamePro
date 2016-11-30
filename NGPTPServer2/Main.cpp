#include <WinSock2.h>
#include <iostream>
#include <vector>
#include <Windows.h>
#include <ctime>
#include "Protocol.h"
#pragma comment(lib,"ws2_32")
using namespace std;
#define SERVERPORT 9595
#define BUFSIZE 512
//-----------전역변수들----------//
SOCKET global_client_sock[2];					// 클라이언트 소켓 변수
vector<PlayerInfo> playerVector;
vector<MonsterInfo> monsterVector;
vector<BulletInfo> bulletVector;
vector<ContainerInfo> conVector;
SceneInfo changeGameScene;				//8byte 전송.  
InitInfo initInform;
HANDLE hEventMonsterUpdate;				//이벤트 핸들
HANDLE hEventPlayerThread1;				//이벤트 핸들.
HANDLE hEventPlayerThread2;

//DataPacket packet;
SendPacket packet;

double leftTime = (100 / 3);

static int NUM= 0;

//-----------함수 선언----------//
DWORD WINAPI ThreadFunc1(LPVOID);	
DWORD WINAPI ThreadFunc2(LPVOID);
void PlayerPosUpdate(PlayerInfo*);
void Init();
void MonsterUpdate();
bool CollisionBulletWithMonster(BulletInfo&, MonsterInfo&);								// 총알하고 몬스터 충돌함수
bool CollisionBulletWithMap(BulletInfo&);												// 맵전체하고 총알
bool CollisionBulletWithObstacle(BulletInfo&bullet, ContainerInfo& container);			// 총알하고 장애물 충돌함수
bool CollisionObstacleWithPlayer(PlayerInfo&, ContainerInfo&);							// 장애물과 플레이어 충돌함수
bool CollisionMapWithPlayer(PlayerInfo&player);
int main(int argc, char* argv[])
{
	WSADATA wsaData;
	SOCKADDR_IN server_addr;	// 서버 소켓 주소 구조체
	SOCKADDR_IN client_addr;	// 클라 소켓 주소 구조체
	SOCKET listen_sock;			// Listen Socket 변수
	int retval;					// return Value 값 받는 변수
	int ClientNum = 0;			// 클라 몇개 접속했는지
	int addrlen;				// 주소 길이
	int playerIndex;
	HANDLE hThreadHandle1;
	HANDLE hThreadHandle2;
	
	char buf[BUFSIZE];	
	
	//윈속 초기화 작업
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	//Socket()
	listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET)
		err_quit("listen_sock()");

	//Bind()
	ZeroMemory(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(SERVERPORT);
	retval = bind(listen_sock, (SOCKADDR*)&server_addr, sizeof(server_addr));
	if (retval == SOCKET_ERROR)
		err_quit("Bind()");

	//Listen()
	retval = listen(listen_sock, SOMAXCONN);
	if (retval == SOCKET_ERROR)
		err_quit("Listen()");    
	int option = true;
	
	while (true)
	{
		//Accept
		addrlen = sizeof(client_addr);
		global_client_sock[ClientNum] = accept(listen_sock, (SOCKADDR*)&client_addr, &addrlen);
		if (global_client_sock[ClientNum] == INVALID_SOCKET)
			err_quit("client_socket");
		printf("%d 번째 클라정보 : IP : %s, 포트번호 : %d \n",ClientNum+1, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
		if (ClientNum < 2)
			ClientNum++;
		if (ClientNum == 2)
		{
			ClientNum++;
			break;
		}
	}

	setsockopt(global_client_sock[0], IPPROTO_TCP, TCP_NODELAY, (const char*)&option, sizeof(option));
	setsockopt(global_client_sock[1], IPPROTO_TCP, TCP_NODELAY, (const char*)&option, sizeof(option));

	//쓰레드 생성.
	hThreadHandle1 = CreateThread(
		NULL, 0, ThreadFunc1, (LPVOID)global_client_sock[0], CREATE_SUSPENDED, NULL);

	hThreadHandle2 = CreateThread(
		NULL, 0, ThreadFunc2, (LPVOID)global_client_sock[1], CREATE_SUSPENDED, NULL);

	//이벤트 생성
	hEventMonsterUpdate = CreateEvent(NULL, FALSE, FALSE, 0);
	if (hEventMonsterUpdate == nullptr)
		cout << "hEventMonsterUpdate 생성 에러" << endl;
	hEventPlayerThread1 = CreateEvent(NULL, FALSE, FALSE, 0);
	if (hEventPlayerThread1 == nullptr)
		cout << "hEventPlayer1Thread 생성 에러" << endl;
	hEventPlayerThread2 = CreateEvent(NULL, FALSE, TRUE, 0);
	if (hEventPlayerThread2 == nullptr)
		cout << "hEventPlayer1Thread 생성 에러" << endl;

	Init();			//데이터들 초기화

	cout << "초기값 전송" << std::endl;
	for (int i = 0; i<2; ++i)
	{
		retval = send(global_client_sock[i], (char*)&initInform, sizeof(InitInfo), 0);         //게임 초기값 전송.
		if (retval == SOCKET_ERROR)
		{
			err_display("send()");
		}
		initInform.playerIndex += 1;
	}
	cout << "초기값 전송 완료." << std::endl;

	std::cout << "준비완료 메세지 수신" << std::endl;         // 아직은 첫번째 클라 두번째 클라 순으로 받아야함
	for (int i = 0; i<2; ++i)                           // 차후 수정 필요.
	{
		retval = recv(global_client_sock[i], buf, sizeof(buf), 0);         //준비완료 메세지 수신. 수신사이즈 생각해야됨.
		if (retval == SOCKET_ERROR)
		{
			err_display("recv()");
		}
	}
	std::cout << "준비완료 메세지 수신 완료" << std::endl;

	std::cout << "씬전환 메세지 송신" << std::endl;
 
	changeGameScene.type = DataType::SCENE;
	changeGameScene.SceneState = SceneList::INGAME;

	for (int i = 0; i < 2; ++i)            //send - 게임씬으로 넘어가라는 메세지 전송
	{
		retval = send(global_client_sock[i], (char*)&changeGameScene, sizeof(SceneInfo), 0);
		if (retval == SOCKET_ERROR)
		{
			err_display("send()");
		}
	}
	std::cout << "씬전환 메세지 보내기 완료" << std::endl;

	//OK

	//ResumeThread 해주기전에 게임 초기값들 전송해주고 게임준비완료 되면 ResumeThread 해준다.
	ResumeThread(hThreadHandle1);
	ResumeThread(hThreadHandle2);

	//MontserPosUpdate()

	MonsterUpdate();

	cout << "쓰레드 2개 종료?" << endl;
	WaitForSingleObject(hThreadHandle1, INFINITE);
	WaitForSingleObject(hThreadHandle2, INFINITE);
	cout << "쓰레드 2개 종료!" << endl;

	CloseHandle(hEventMonsterUpdate);
	CloseHandle(hEventPlayerThread1);
	CloseHandle(hEventPlayerThread2);

	closesocket(listen_sock);
	closesocket(global_client_sock[0]);
	closesocket(global_client_sock[1]);

	return 0;
}
void Init()
{
	playerVector.reserve(2);
	playerVector.push_back(PlayerInfo{ DataType::PLAYER,1,Vec3{ -1500,50,1900 },Vec3{ 0,0,0 },false });
	playerVector.push_back(PlayerInfo{ DataType::PLAYER,2,Vec3{ 1500,50,1900 },Vec3{ 0,0,0 },false });

	// 컨테이너 위치값 들 서버에 초기화 -> 충돌체크에 사용될것/
	conVector.reserve(4);
	conVector.push_back(ContainerInfo{ Vec3{ 0,300,1000 } });
	conVector.push_back(ContainerInfo{ Vec3{ 1500,300,0 } });
	conVector.push_back(ContainerInfo{ Vec3{ 0,300,-1000 } });
	conVector.push_back(ContainerInfo{ Vec3{ -1500,300,0 } });

	//Vec3  a{ 1,2,3 };
	int hp = 3;
	// 몬스터들 위치값 초기화
	monsterVector.reserve(10);
	monsterVector.push_back(MonsterInfo{ DataType::MONSTER,Vec3{ -1000,50,-1800 } ,3,0 });
	monsterVector.push_back(MonsterInfo{ DataType::MONSTER,Vec3{ -1000,50,-1000 } ,3,1 });
	monsterVector.push_back(MonsterInfo{ DataType::MONSTER,Vec3{ 1000,50,-1800 } ,3,2 });
	monsterVector.push_back(MonsterInfo{ DataType::MONSTER,Vec3{ 1000,50,-1000 } ,3,3 });
	monsterVector.push_back(MonsterInfo{ DataType::MONSTER,Vec3{ -500,50,-500 } ,3,4 });
	monsterVector.push_back(MonsterInfo{ DataType::MONSTER,Vec3{ 0,50,0 } ,3,5 });
	monsterVector.push_back(MonsterInfo{ DataType::MONSTER,Vec3{ 500,50,500 } ,3,6 });
	monsterVector.push_back(MonsterInfo{ DataType::MONSTER,Vec3{ -1000,50,1000 } ,3,7 });
	monsterVector.push_back(MonsterInfo{ DataType::MONSTER,Vec3{ 1000,50,1000 } ,3,8 });
	monsterVector.push_back(MonsterInfo{ DataType::MONSTER,Vec3{ 0,50,1500 } ,3,9 });

	//양클라이언트에 보낼 초기값들에 초기화.
	// Player1,2

	initInform.Player1Pos = Vec3{ -1500, 50, 1900 };
	initInform.Player2Pos = Vec3{ 1500, 50, 1900 };

	//Monsters
	initInform.MonsterPos[0] = Vec3{ -1000,50,-1800 };
	initInform.MonsterPos[1] = Vec3{ -1000,50,-1000 };
	initInform.MonsterPos[2] = Vec3{ 1000,50,-1800 };
	initInform.MonsterPos[3] = Vec3{ 1000,50,-1000 };
	initInform.MonsterPos[4] = Vec3{ -500,50,-500 };
	initInform.MonsterPos[5] = Vec3{ 0,50,0 };
	initInform.MonsterPos[6] = Vec3{ 500,50,500 };
	initInform.MonsterPos[7] = Vec3{ -1000,50,1000 };
	initInform.MonsterPos[8] = Vec3{ 1000,50,1000 };
	initInform.MonsterPos[9] = Vec3{ 0,50,1500 };

	initInform.playerIndex = 1;

	//Send,Recv전용 패킷 초기화
	for (int i = 0; i < 10; ++i)
	{
		packet.MonstersPosition[i].x = monsterVector[i].MonsterPos.x;
		packet.MonstersPosition[i].z = monsterVector[i].MonsterPos.z;
	}
	packet.player1.playerIndex = 1;
	packet.player1.playerPos.x = playerVector[0].PlayerPos.x;
	packet.player1.playerPos.z = playerVector[0].PlayerPos.z;

	packet.player2.playerIndex = 2;
	packet.player2.playerPos.x = playerVector[1].PlayerPos.x;
	packet.player2.playerPos.z = playerVector[1].PlayerPos.z;

}
void MonsterUpdate()
{
	std::cout << "monsterPosUpdate in Main Function" << std::endl;

	float firstPosition[10];
	float monsterDirection[10];
	float moveRange = 300;
	int retval = 0;

	char buf2[BUFSIZE];

	//몬스터들의 방향 랜덤하게 초기화.
	for (int i = 0; i < 10; ++i)
	{
		firstPosition[i] = monsterVector[i].MonsterPos.x;

		monsterDirection[i] = rand() % 2 - 1;
		if (monsterDirection[i] == 0)
			monsterDirection[i] = 1;
	}
	int monsterType = DataType::MONSTER;
	int bulletType = DataType::BULLET;
	int packetType = DataType::PACKET;
	double nextTime = 0.0f;
	double nowTime = clock();


	while (true)
	{
		WaitForSingleObject(hEventPlayerThread2, INFINITE);
		nowTime = std::clock();
		if (nowTime > nextTime)
		{
			//몬스터 위치 업데이트
			for (int i = 0; i < 10; ++i)
			{
				if (monsterVector[i].MonsterPos.x < firstPosition[i] - moveRange)
					monsterDirection[i] *= -1;
				if (monsterVector[i].MonsterPos.x > firstPosition[i] + moveRange)
					monsterDirection[i] *= -1;
			}
			for (int i = 0; i < 10; ++i)
			{
				monsterVector[i].MonsterPos.x += (5 * monsterDirection[i]);
				packet.MonstersPosition[i].x = monsterVector[i].MonsterPos.x;
				packet.MonstersPosition[i].z = monsterVector[i].MonsterPos.z;
			}

			//총알 위치 업데이트.
			//if (bulletVector.size() != 0)
			//{
			//	for (int i = 0; i < bulletVector.size(); ++i)
			//	{
			//	}
			//}

			for (int i = 0; i < 2; ++i)
			{
				retval = send(global_client_sock[i], (char*)&packet, sizeof(SendPacket), 0);
				if (retval == SOCKET_ERROR)
				{
					err_display("send1()");
				}
			}

			nextTime = clock() + leftTime;

		}
		SetEvent(hEventMonsterUpdate);
	}
}

DWORD WINAPI ThreadFunc1(LPVOID param)
{
	cout << "쓰레드 시작!" << endl;
	SOCKET client_sock = (SOCKET)param;
	int retval;         //return value
	char buf[BUFSIZE];
	int dataType = 0;

	RecvPacket recvPacket;

	int playerIndex;

	double nextTime = 0.0f;
	double nowTime = clock();

	while (true)            //이 함수내에서 send,recv 작업이 이루어짐.
	{
		WaitForSingleObject(hEventMonsterUpdate, INFINITE);
		nowTime = std::clock();
		if (nowTime > nextTime)
		{
			retval = recv(client_sock, (char*)&recvPacket, sizeof(RecvPacket), 0);
			if (retval == SOCKET_ERROR)
			{
				err_display("recv");
			}

			playerVector[0].PlayerPos.x = recvPacket.playerPos.x;
			playerVector[0].PlayerPos.z = recvPacket.playerPos.z;
			// 맵 충돌
			//CollisionMapWithPlayer(playerInfoForRS);

			//데이터패킷에 값 초기화
			packet.player1.playerPos = recvPacket.playerPos;
			packet.player1.playerCam = recvPacket.playerCam;
			
			nextTime = clock() + leftTime;
		}
		SetEvent(hEventPlayerThread1);
	}
	return 0;
}
DWORD WINAPI ThreadFunc2(LPVOID param)
{
	cout << "쓰레드 시작!" << endl;
	SOCKET client_sock = (SOCKET)param;
	int retval;         //return value
	char buf[BUFSIZE];
	int dataType = 0;

	double nextTime = 0.0f;
	double nowTime = clock();
	RecvPacket recvPacket;

	while (true)            //이 함수내에서 send,recv 작업이 이루어짐.
	{
		WaitForSingleObject(hEventPlayerThread1, INFINITE);
		nowTime = std::clock();
		if (nowTime > nextTime)
		{
			retval = recv(client_sock, (char*)&recvPacket, sizeof(RecvPacket), 0);
			if (retval == SOCKET_ERROR)
			{
				err_display("recv");
			}

			playerVector[1].PlayerPos.x = recvPacket.playerPos.x;
			playerVector[1].PlayerPos.z = recvPacket.playerPos.z;
			// 맵 충돌
			//CollisionMapWithPlayer(playerInfoForRS);
			
			//데이터패킷의 값 초기화.
			//packet.player2 = playerInfoForRS;
			
			packet.player2.playerPos = recvPacket.playerPos;
			packet.player2.playerCam = recvPacket.playerCam; 

			nextTime = clock() + leftTime;
		}
		SetEvent(hEventPlayerThread2);
	}
	return 0;
}
bool CollisionBulletWithMonster(BulletInfo & bullet, MonsterInfo& monster)
{
	return(
		bullet.BulletPos.x + 1 > monster.MonsterPos.x - 30 &&
		bullet.BulletPos.x - 1 < monster.MonsterPos.x + 30 &&
		bullet.BulletPos.y + 1 > monster.MonsterPos.y - 55 &&
		bullet.BulletPos.y - 1 < monster.MonsterPos.y + 55 &&
		bullet.BulletPos.z + 1 > monster.MonsterPos.z - 15 &&
		bullet.BulletPos.z - 1 < monster.MonsterPos.z + 15
		);
}
bool CollisionBulletWithObstacle(BulletInfo&bullet, ContainerInfo& container)
{
	return(
		bullet.BulletPos.x + 1 > container.position.x - 128 &&
		bullet.BulletPos.x - 1 < container.position.x + 128 &&
		bullet.BulletPos.y + 1 > container.position.y - 128 &&
		bullet.BulletPos.y - 1 < container.position.y + 128 &&
		bullet.BulletPos.z + 1 > container.position.z - 256 &&
		bullet.BulletPos.z - 1 < container.position.z + 256
		);
}
bool CollisionBulletWithMap(BulletInfo& bullet)
{
	if (bullet.BulletPos.x + 1 > 2000)
		return true;
	if (bullet.BulletPos.x - 1 < -2000)
		return true;
	if (bullet.BulletPos.y + 1 > 2000)
		return true;
	if (bullet.BulletPos.y - 1 < -2000)
		return true;

	return false;
}
bool CollisionObstacleWithPlayer(PlayerInfo&player, ContainerInfo& container)
{
	if (player.PlayerPos.x + 30 > container.position.x - 128 &&
		player.PlayerPos.x - 30 < container.position.x + 128 &&
		player.PlayerPos.z + 15 > container.position.z - 256 &&
		player.PlayerPos.z - 15 < container.position.z + 256)
	{
		return true;;
	}

	//트루면 이전값으로 돌려놓기.
}
bool CollisionMapWithPlayer(PlayerInfo&player)
{
	if (player.PlayerPos.x + 30 > 2000)
	{
		player.PlayerPos.x -= 30;
		return true;
	}
	if (player.PlayerPos.x - 30 < -2000)
	{
		player.PlayerPos.x += 30;
		return true;
	}
	if (player.PlayerPos.z+30>2000)
	{
		player.PlayerPos.z -= 30;
		return true;
	}
	if (player.PlayerPos.z - 30>2000)
	{
		player.PlayerPos.z += 30;
		return true;
	}
	return false;
}