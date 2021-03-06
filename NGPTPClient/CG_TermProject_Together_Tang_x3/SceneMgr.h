#pragma once
#include "stdafx.h"
#include "Common.h"
class CScene;
class CBitmapMgr;
class CGameObject;
class CCharacter;
class CSceneMgr
{
	static CSceneMgr*		m_pInst;
	CScene*					m_pScene;
	CGameObject*			m_pCharacter;
	// 윈속 초기화
	WSADATA					wsa;
	// socket()
	SOCKET					sock;
	// connect()
	SOCKADDR_IN				serveraddr;
	int						retval;
	/////////////////////////////////////////////
	static InitInfo*		strInitInfo;
	InitInfo				tempInitInfo;

	static SceneInfo*		strSceneInfo;
	SceneInfo				tempSceneInfo;

	static MonsterPosForRecv* strMonsterPos;

	static PlayerInfo*		strPlayerInfo;


	static unsigned char	charKey;
	PlayerInfo				tempPlayerInfo;
public:
	void SetScene(int _eID, CBitmapMgr* _pBitmapMgr);

public:
	static CSceneMgr* GetInst(void);

	void DestroyInst(void);
public:
	GLvoid	Render(GLvoid);
	GLvoid	KeyboardUp(unsigned char key, int x, int y);
	GLvoid	Keyboard(unsigned char key, int x, int y);
	GLvoid	SpecialKeyboard(int key, int x, int y);
	GLvoid	Update(int value);
	GLvoid	Mouse(int button, int state, int x, int y);
	GLvoid	MouseMotionFunc(int x, int y);
	GLvoid	Release();
	GLvoid	SpecialKeyboardUp(int key, int x, int y);
	// 소켓 함수 오류 출력 후 종료
	void err_quit(char *msg);
	// 소켓 함수 오류 출력
	void err_display(char *msg);
	GLvoid ConnectServer();
	void RecvInitInfo();
	static InitInfo* GetInitInfo();
	
	void SendSpace();
	void RecvSpace();
	static SceneInfo* GetSceneInfo();

	void RecvServer();
	static MonsterPosForRecv* GetMonsterPos();

	static PlayerInfo* GetPlayerPos();
	int recvn(SOCKET s, char* buf, int len, int flags);

	static void SetKey(unsigned char _Key);
	void SendKey(Vec3 _Position);
	void RecvKey();
	void SetCharClass(CGameObject* _pChar)
	{
		m_pCharacter = _pChar;
	}
	//void PlayerRender();
	//void MonsterRender();
	//void ContainerRender();
public:
	CSceneMgr();
	~CSceneMgr();
	
};

