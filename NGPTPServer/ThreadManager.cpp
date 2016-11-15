#include "ThreadManager.h"
#pragma comment(lib,"ws2_32")

std::vector<PlayerInfo> CThreadManager::playerVector;
std::vector<MonsterInfo> CThreadManager::monsterVector;
std::vector<BulletInfo> CThreadManager::bulletVector;
std::vector<ContainerInfo> CThreadManager::conVector;

CThreadManager::CThreadManager() {};

CThreadManager::CThreadManager(SOCKET client_socket1,SOCKET client_socket2)
{
	client_sock[0] = client_socket1;
	client_sock[1] = client_socket2;
	playerIndex = 1;
	//Init();

	//Player1 Player2 순임
	
	hThreadHandle[0] = CreateThread(
		NULL, 0, CThreadManager::ThreadFunc, (LPVOID)client_sock[0], CREATE_SUSPENDED, NULL);

	hThreadHandle[1] = CreateThread(
		NULL, 0, CThreadManager::ThreadFunc, (LPVOID)client_sock[1], CREATE_SUSPENDED, NULL);

	hMonPosUpdateHandle = CreateThread(
		NULL, 0, CThreadManager::MonsterPosUpdate, NULL, CREATE_SUSPENDED, NULL);
}

void CThreadManager::Init()
{
	
	//플레이어 위치값 공유자원들 초기화.
	playerVector.reserve(2);
	playerVector.push_back(PlayerInfo{ DataType::PLAYER,Vec3{ -1500,100,1900 },Vec3{ 0,0,0 },false });
	playerVector.push_back(PlayerInfo{ DataType::PLAYER,Vec3{ 1500,100,1900 },Vec3{ 0,0,0 },false });

	// 컨테이너 위치값 들 서버에 초기화 -> 충돌체크에 사용될것/
	conVector.reserve(4);
	conVector.push_back(ContainerInfo{ Vec3{0,300,1000} });
	conVector.push_back(ContainerInfo{ Vec3{1500,300,0} });
	conVector.push_back(ContainerInfo{ Vec3{ 0,300,-1000} });
	conVector.push_back(ContainerInfo{ Vec3{-1500,300,0} });

	//Vec3  a{ 1,2,3 };
	int hp = 3;
	// 몬스터들 위치값 초기화
	monsterVector.reserve(10);
	MonsterInfo a[10];

	a[0].MonsterPos = Vec3{ -1000,100,-1800 };
	a[1].MonsterPos = Vec3{ -1000,100,-1000 };
	a[2].MonsterPos = Vec3{ 1000,100,-1800 };
	a[3].MonsterPos = Vec3{ 1000,100,-1000 };
	a[4].MonsterPos = Vec3{ -500,100,-500 };
	a[5].MonsterPos = Vec3{ 0,100,0 };
	a[6].MonsterPos = Vec3{ 500,100,500 };
	a[7].MonsterPos = Vec3{ -1000,100,1000 };
	a[8].MonsterPos = Vec3{ 1000,100,1000 };
	a[9].MonsterPos = Vec3{ 0,100,1500 };

	for (int i = 0; i < 10; ++i)
	{
		a[i].type = DataType::MONSTER;
		a[i].hp = 3;
		monsterVector.push_back(a[i]);
	}
	//양클라이언트에 보낼 초기값들에 초기화.
	// Player1,2

	initInform.Player1Pos = Vec3{ -1500, 100, 1900 };
	initInform.Player2Pos = Vec3{ 1500, 100, 1900 };

	//Monsters
	initInform.MonsterPos[0] = Vec3{ -1000,100,-1800 };
	initInform.MonsterPos[1] = Vec3{ -1000,100,-1000 };
	initInform.MonsterPos[2] = Vec3{ 1000,100,-1800 };
	initInform.MonsterPos[3] = Vec3{ 1000,100,-1000 };
	initInform.MonsterPos[4] = Vec3{ -500,100,-500 };
	initInform.MonsterPos[5] = Vec3{ 0,100,0 };
	initInform.MonsterPos[6] = Vec3{ 500,100,500 };
	initInform.MonsterPos[7] = Vec3{ -1000,100,1000 };
	initInform.MonsterPos[8] = Vec3{ 1000,100,1000 };
	initInform.MonsterPos[9] = Vec3{ 0,100,1500 };

	initInform.playerIndex = 1;
}
void CThreadManager::err_display(char * msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPSTR)&lpMsgBuf, 0, NULL);
	printf("[%s] %s \n", msg, (char*)lpMsgBuf);
	LocalFree(lpMsgBuf);
	exit(1);
}
void CThreadManager::err_quit(char * msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPSTR)&lpMsgBuf, 0, NULL);
	MessageBox(NULL, (LPCTSTR)lpMsgBuf, msg, MB_ICONERROR);
	LocalFree(lpMsgBuf);
	exit(1);
}
DWORD WINAPI CThreadManager::MonsterPosUpdate(LPVOID)
{
	std::vector<MonsterInfo>::iterator monsterInfoIter;
	monsterInfoIter = monsterVector.begin();
	std::cout << "monsterPosUpdate" << std::endl;

	while(true)
	{
		for(int i = 0;i<10;++i)
		{
			monsterVector[i].MonsterPos.x += 1;
			std::cout << monsterVector[i].MonsterPos.x << std::endl;
		}
	}
}
DWORD WINAPI CThreadManager::ThreadFunc(LPVOID param)
{
	SOCKET client_sock = (SOCKET)param;
	SOCKADDR_IN clientaddr;
	//getpeername(client_sock,(SOCKADDR*)&clientaddr,&addrlen);

	COperator calculate = { &playerVector, &monsterVector, &bulletVector };

	int retval;			//return value
	char buf[BUFSIZE];
	
	//std::cout << thisIndex << std::endl;

	while (true)				//이 함수내에서 send,recv 작업이 이루어짐.
	{
		retval = recv(client_sock, buf, sizeof(buf), 0);

		//오류검사문 필요
		calculate.Update();
		retval = send(client_sock, buf, sizeof(buf), 0);
	}
	return 0;
}

void CThreadManager::Update()
{
	Init();

	std::cout << "초기값 전송" << std::endl;
	for(int i =0 ;i<2;++i)
	{
		retval = send(client_sock[i], (char*)&initInform, sizeof(InitInfo), 0);			//게임 초기값 전송.
		if (retval == SOCKET_ERROR)
		{
			err_display("send()");
		}
		initInform.playerIndex += 1;
	}
	std::cout << "초기값 전송 완료." << std::endl;


	std::cout << "준비완료 메세지 수신" << std::endl;			// 아직은 첫번째 클라 두번째 클라 순으로 받아야함
	for (int i = 0; i<2; ++i)									// 차후 수정 필요.
	{
		retval = recv(client_sock[i], buf, sizeof(buf), 0);			//준비완료 메세지 수신. 수신사이즈 생각해야됨.
		if (retval == SOCKET_ERROR)
		{
			err_display("recv()");
		}
	}
	std::cout << "준비완료 메세지 수신 완료" << std::endl;

	std::cout << "씬전환 메세지 송신" << std::endl;

	SceneInfo changeGameScene;			//8byte 전송.	
	changeGameScene.type = DataType::SCENE;
	changeGameScene.SceneState = SceneList::INGAME;

	for (int i = 0; i < 2; ++i)				//send - 게임씬으로 넘어가라는 메세지 전송
	{
		retval = send(client_sock[i], (char*)&changeGameScene, sizeof(SceneInfo), 0);
		if (retval == SOCKET_ERROR)
		{
			err_display("send()");
		}
	}
	std::cout << "씬전환 메세지 보내기 완료" << std::endl;

	//ResumeThread 해주기전에 게임 초기값들 전송해주고 게임준비완료 되면 ResumeThread 해준다.
	ResumeThread(hThreadHandle[0]);
	ResumeThread(hThreadHandle[1]);

	ResumeThread(hMonPosUpdateHandle);

	std::cout << "쓰레드 3개 종료?" << std::endl;
	WaitForMultipleObjects(2,hThreadHandle,TRUE,INFINITE);
	WaitForSingleObject(hMonPosUpdateHandle, INFINITE);
	std::cout << "쓰레드 3개 종료!" << std::endl;
}

CThreadManager::~CThreadManager()
{}
