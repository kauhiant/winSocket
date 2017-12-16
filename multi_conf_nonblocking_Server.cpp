// multi_conf_nonblocking_Server.cpp  ���A���{��(�D�{��)
/* ���A���{���@���T��:
   multi_conf_nonblocking_Server.cpp
   multi_conf_nonblocking_Server(Client_Function).cpp
   multi_conf_nonblocking_Server(Client_Class).h

   �t�X���Ȥ�ݵ{����: multi_conf_nonblocking_Client.cpp
*/

#include <iostream>
using namespace std;
#include <winsock2.h> //wincosk2.h���twindows.h���ޤJ,��windows.h���S�twinbase.h���ޤJ
#include "multi_conf_nonblocking_Server(Client_Class).h"

/*
 * �ܼƪ��Ĥ@�Ӧr�����p�g�B�Ӧr��������ܼƪ������A�p nErrCode.����Ϊ����~�N�X
 * ��ƪ��Ĥ@�Ӧr�����j�g�B�����ƪ��\��C�pvoid AcceptThread(void).�������s���������
 */
#pragma comment(lib, "ws2_32.lib")

typedef list<CClient*> CLIENTLIST;			//���(List)

#define SERVERPORT			5556			//���A��TCP��
#define SERVER_SETUP_FAIL	1				//�Ұʦ��A������

#define TIMEFOR_THREAD_EXIT			5000	//�D������ίv�ɶ�
#define TIMEFOR_THREAD_HELP			1500	//�M�z�귽������h�X�ɶ�
#define TIMEFOR_THREAD_SLEEP		500		//���ݥΤ�ݽШD������ίv�ɶ�

HANDLE	hThreadAccept;						//�����Τ�ݳs�����������X
HANDLE	hThreadHelp;						//����귽���������X
SOCKET	sServer;							//��ť�Ȥ�ݳs�uSocket
BOOL	bServerRunning;						//���A�����ثe�u�@���A�ܼ�
HANDLE	hServerEvent;						//���A���h�X�ƥ󱱨�X
CLIENTLIST			clientlist;				//�޲z�Ȥ�ݳs�u�����(List)
CRITICAL_SECTION	csClientList;			//�O�@����{�ɰ�(Critical section)����
int     seq;                                //�Ȥ�ݧǧO

BOOL	InitSever(void);					//��l��
BOOL	StartService(void);					//�ҰʪA��
void	StopService(void);					//����A��
BOOL	CreateHelperAndAcceptThread(void);	//�Ыر����Τ�ݳs�������
void	ExitServer(void);					//���A���h�X

void	InitMember(void);					//��l�ƥ����ܼ�
BOOL	InitSocket(void);					//��l��SOCKET

void	ShowTipMsg(BOOL bFirstInput);		//��ܴ��ܸ�T
void	ShowServerStartMsg(BOOL bSuc);		//��ܦ��A���w�g�Ұ�
void	ShowServerExitMsg(void);			//��ܦ��A�����b�h�X

DWORD	__stdcall	HelperThread(void *pParam);			//����귽
DWORD	__stdcall 	AcceptThread(void *pParam);			//�����Τ�ݳs�������
void	ShowConnectNum();								//��ܥΤ�ݪ��s���ƥ�

int main(int argc, char* argv[])
{
	//��l�Ʀ��A��
	if (!InitSever())
	{
		ExitServer();
		return SERVER_SETUP_FAIL;
	}

	//�ҰʪA��
	if (!StartService())
	{
		ShowServerStartMsg(FALSE);
		ExitServer();
		return SERVER_SETUP_FAIL;
	}

	//����A��
	StopService();

	//���A���h�X
	ExitServer();

	return 0;
}

/**
 * ��l��
 */
BOOL	InitSever(void)
{

	InitMember();//��l�ƥ����ܼ�

	//��l��SOCKET
	if (!InitSocket())
		return FALSE;

	return TRUE;
}


/**
 * ��l�ƥ����ܼ�
 */
void	InitMember(void)
{
	InitializeCriticalSection(&csClientList);				//��l���{�ɰ�
	hServerEvent = CreateEvent(NULL, TRUE, FALSE, NULL);	//��ʳ]�m�ƥ�A��l�Ƭ��L��T�����A
	hThreadAccept = NULL;									//�]�m��NULL
	hThreadHelp = NULL;										//�]�m��NULL
	sServer = INVALID_SOCKET;								//�]�m���L�Ī��q�T��
	bServerRunning = FALSE;									//���A�����S���B�檬�A
	clientlist.clear();										//�M�����
	seq = 0;                                                //�Ȥ�ݧǸ�
}

/**
 *  ��l��SOCKET
 */
BOOL InitSocket(void)
{
	//��^��
	int reVal;

	//��l��Windows Sockets DLL
	WSADATA  wsData;
	reVal = WSAStartup(MAKEWORD(2,2),&wsData);

	//�Ыسq�T��
	sServer = socket(AF_INET, SOCK_STREAM, 0);
	if(INVALID_SOCKET== sServer)
		return FALSE;

	//�]�m�q�T�ݫD����Ҧ�
	unsigned long ul = 1;
	reVal = ioctlsocket(sServer, FIONBIO, (unsigned long*)&ul);
	if (SOCKET_ERROR == reVal)
		return FALSE;

	//�j�w�q�T��
	sockaddr_in serAddr;
	serAddr.sin_family = AF_INET;
	serAddr.sin_port = htons(SERVERPORT);
	serAddr.sin_addr.S_un.S_addr = INADDR_ANY;
	reVal = bind(sServer, (struct sockaddr*)&serAddr, sizeof(serAddr));
	if(SOCKET_ERROR == reVal )
		return FALSE;

	//��ť
	reVal = listen(sServer, SOMAXCONN);
	if(SOCKET_ERROR == reVal)
		return FALSE;

	return TRUE;
}

/**
 *  �ҰʪA��
 */
BOOL	StartService(void)
{
	BOOL reVal = TRUE;	//��^��

	ShowTipMsg(TRUE);	//���ܥΤ��J

	char cInput;		//��J�r��
	do
	{
		cin >> cInput;
		if ('s' == cInput || 'S' == cInput)
		{
			if (CreateHelperAndAcceptThread())	//�ЫزM�z�귽�M�����Τ�ݽШD�������
			{
				ShowServerStartMsg(TRUE);		//�Ыذ�������\��T
			}else{
				reVal = FALSE;
			}
			break;//���X�`����

		}else{
			ShowTipMsg(TRUE);
		}

	} while(cInput != 's' &&//������J's'�Ϊ�'S'�r��
			cInput != 'S');

	return reVal;
}

/**
 *  ����A��
 */
void	StopService(void)
{
	BOOL reVal = TRUE;	//��^��

	ShowTipMsg(FALSE);	//���ܥΤ��J

	char cInput;		//��J���ާ@�r��
	for (;bServerRunning;)
	{
		cin >> cInput;
		if (cInput == 'E' || cInput == 'e')
		{
			if (IDOK == MessageBox(NULL, "Are you sure?", //���ݥΤ�T�{�h�X���T�����
				"Server", MB_OKCANCEL))
			{
				break;//���X�`����
			}else{
				Sleep(TIMEFOR_THREAD_EXIT);	//������ίv
			}
		}else{
			Sleep(TIMEFOR_THREAD_EXIT);		//������ίv
		}
	}

	bServerRunning = FALSE;		//���A���h�X

	ShowServerExitMsg();		//��ܦ��A���h�X��T

	Sleep(TIMEFOR_THREAD_EXIT);	//����L������ɶ��h�X

	WaitForSingleObject(hServerEvent, INFINITE);//���ݲM�z�귽������o�e���ƥ�

	return;
}

/**
 *  ��ܴ��ܸ�T
 */
void	ShowTipMsg(BOOL bFirstInput)
{
	if (bFirstInput)//�Ĥ@��
	{
		cout << endl;
		cout << endl;
		cout << "**********************" << endl;
		cout << "*                    *" << endl;
		cout << "* s(S): Start server *" << endl;
		cout << "*                    *" << endl;
		cout << "**********************" << endl;
		cout << "Please input:" << endl;

	}else{//�h�X���A��
		cout << endl;
		cout << endl;
		cout << "**********************" << endl;
		cout << "*                    *" << endl;
		cout << "* e(E): Exit  server *" << endl;
		cout << "*                    *" << endl;
		cout << "**********************" << endl;
		cout << " Please input:" << endl;
	}
}
/**
 *  ����귽
 */
void  ExitServer(void)
{
	DeleteCriticalSection(&csClientList);	//�����{�ɰϹ�H
	CloseHandle(hServerEvent);				//����ƥ󪫥󱱨�X
	closesocket(sServer);					//����SOCKET
	WSACleanup();							//����Windows Sockets DLL
}

/**
 * �Ы�����귽������M�����Τ�ݽШD�����
 */
BOOL  CreateHelperAndAcceptThread(void)
{

	bServerRunning = TRUE;//�]�m���A�����B�檬�A

	//�Ы�����귽�����
	unsigned long ulThreadId;
	hThreadHelp = CreateThread(NULL, 0, HelperThread, NULL, 0, &ulThreadId);
	if( NULL == hThreadHelp)
	{
		bServerRunning = FALSE;
		return FALSE;
	}else{
		CloseHandle(hThreadHelp);
	}

	//�Ыر����Τ�ݽШD�����
	hThreadAccept = CreateThread(NULL, 0, AcceptThread, NULL, 0, &ulThreadId);
	if( NULL == hThreadAccept)
	{
		bServerRunning = FALSE;
		return FALSE;
	}else{
		CloseHandle(hThreadAccept);
	}

	return TRUE;
}

/**
 * ��ܱҰʦ��A�����\�P���Ѯ���
 */
void  ShowServerStartMsg(BOOL bSuc)
{
	if (bSuc)
	{
		cout << "**********************" << endl;
		cout << "*                    *" << endl;
		cout << "* Server succeeded!  *" << endl;
		cout << "*                    *" << endl;
		cout << "**********************" << endl;
	}else{
		cout << "**********************" << endl;
		cout << "*                    *" << endl;
		cout << "* Server failed   !  *" << endl;
		cout << "*                    *" << endl;
		cout << "**********************" << endl;
	}

}

/**
 * ��ܰh�X���A������
 */
void  ShowServerExitMsg(void)
{

	cout << "**********************" << endl;
	cout << "*                    *" << endl;
	cout << "* Server exit...     *" << endl;
	cout << "*                    *" << endl;
	cout << "**********************" << endl;
}

/**
 * �����Τ�ݳs��
 */
DWORD __stdcall AcceptThread(void* pParam)
{
	SOCKET  sAccept;							//�����Τ�ݳs�����q�T��
	sockaddr_in addrClient;						//�Τ��SOCKET�a�}

	for (;bServerRunning;)						//���A�������A
	{
		memset(&addrClient, 0, sizeof(sockaddr_in));					//��l��
		int			lenClient = sizeof(sockaddr_in);					//�a�}����
		sAccept = accept(sServer, (sockaddr*)&addrClient, &lenClient);	//�����Ȥ�ШD

		if(INVALID_SOCKET == sAccept )
		{
			int nErrCode = WSAGetLastError();
			if(nErrCode == WSAEWOULDBLOCK)	//�L�k�ߧY�����@�ӫD���שʳq�T�ݾާ@
			{
				Sleep(TIMEFOR_THREAD_SLEEP);
				continue;//�~�򵥫�
			}else {
				return 0;//������h�X
			}

		}
		else//�����Τ�ݪ��ШD
		{
/*[mark]HEAD
	�I�s�غc�l�i��Ȥ�ݸ`�I����l�ơC
	�N���`�I�[�J���clientlist�C
	�����clientlist�����C�ӫȤ�ݸ`�I���o���clientlist (�ǥѫȤ�ݸ`�I��GetClientList()�禡)
�y�{:
    1.�̷Ӧ���쪺 sAccept �I�s�غc�l�i�� CClient �`�I����l�ơA�`�I�W�� pClient
    2.�i�J clientlist ���{�ɰ�
    3.�N pClient �[�J clientlist
    4.�� clientlist �����C�� CClient �`�I�� GetCLientList(clientlist) �N CClient �`�I���� m_clientlist ���V�� clientlist
    5.���} clientlist ���{�ɰ�
    6.���� CClient �`�I�Ұ� StartRunning()
*/
			CClient *pClient = new CClient(sAccept,addrClient, seq++, csClientList);	//[1]�ЫإΤ�ݹ�H

			EnterCriticalSection(&csClientList);			//[2]�i�J�b�{�ɰ�

			clientlist.push_back(pClient);					//[3]�[�J���

			CLIENTLIST::iterator iter = clientlist.begin(); //[4]
			for (iter; iter != clientlist.end();)
			{
				CClient *pClient_tmp = (CClient*)*iter;     //[���D4]
				BOOL r = pClient_tmp->GetClientList(clientlist);
					iter++;						            //���w�U��
			}

			LeaveCriticalSection(&csClientList);			//[5]���}�{�ɰ�

			pClient->StartRuning();							//[6]���������Τ�ݫإ߱�����ƩM�o�e��ư����

/*[mark]END
���D:
        1.CClient �� m_clientlist �ä��O���ЦӬO�@���ܼơA���ӷ|���B�~���ܦh�Ŷ�
        2.�p�G CClient �� m_clientlist �O���Ъ��ܡA�N���ΨC���n���ܥ��� clientlist �����s�I�s�Ҧ��`�I GetClientList() �A�ٮɶ�
        3.GetClientList() �u�|�^�� true �A�ҥH bool r �@�w�O true
        4.�]�� clientlist �O list<CClient*> �A�L���`�I�O���Ы��A�A�� iter �O���V CClient* ������ *iter �N�O CClient* �`�I�A�������٭n�૬
*/
		}
	}

	return 0;//������h�X
}

/**
 * �M�z�귽
 */
DWORD __stdcall HelperThread(void* pParam)
{
	for (;bServerRunning;)//���A�����b�B��
	{
/*[mark]HEAD
    �M�z�w�g�_�}�s�����Ȥ᪺�O����Ŷ��C
    �p�G���Ȥ��_�}�s�u�A�h�������s�����clientlist�����C�ӫȤ�ݸ`�I���o���clientlist (�ǥѫȤ�ݸ`�I��GetClientList()�禡)
�y�{:
    1.�ŧi Erase bool�ܼ� �A�����O�_���Ȥ��_�}�s�u
    2.�i�J clientlist ���{�ɰ�
    3.�M��w�g�_�}�s�u���Τ�ݡA��L�q clientlist �������A������L���O����Ŷ��A�A�NErase��אּtrue
    4.�p�G���Ȥ���_�u�A��Ҧ����Ȥ�ݳ��I�s GetClientList()
    5.���} clientlist ���{�ɰ�
    6.���ݳB�z�ɶ��A�קK����
*/
		BOOL Erase = FALSE; //[1]

		EnterCriticalSection(&csClientList);//[2]�i�J�{�ɰ�

		//[3]�M�z�w�g�_�}���s���Τ�ݰO����Ŷ�
		CLIENTLIST::iterator iter = clientlist.begin();
		for (iter; iter != clientlist.end();)
		{
			CClient *pClient = (CClient*)*iter;
			if (pClient->IsExit())			//�Τ�ݰ�����w�g�h�X
			{
				clientlist.erase(iter++);	//�R���`�I�A[���D3]
				delete pClient;				//����O����
				pClient = NULL;             //[���D2]
				Erase = TRUE;               //[���D1]
			}else{
				iter++;						//���w�U��
			}
		}

		if(Erase)   //[4] [���D1]
		{
			CLIENTLIST::iterator iter = clientlist.begin();
			for (iter; iter != clientlist.end();)
			{
				CClient *pClient_tmp = (CClient*)*iter;
				BOOL r = pClient_tmp->GetClientList(clientlist);
					iter++;					//���w�U��
			}
		}

		LeaveCriticalSection(&csClientList);    //[5]���}�{�ɰ�

		Sleep(TIMEFOR_THREAD_HELP);             //[6]
/*[mark]END
���D:
    1.Erase�ܼ����Ӥ��ݭn�A�]���i�H��[�y�{4]���ʧ@���[�y�{3]�� if �̭�
    2.[�y�{3]�� pClient = NULL ���Ӥ��ݭn�A�]���᭱���S�Ψ� pClient
    3.[�y�{3]�� clientlist.erase(iter++) ���Ӥ]��g�� iter = clientlist.erase(iter)
*/
	}


	//���A������u�@
	if (!bServerRunning)
	{
		//�_�}�C�ӳs��,������h�X
		EnterCriticalSection(&csClientList);
		CLIENTLIST::iterator iter = clientlist.begin();
		for (iter; iter != clientlist.end();)
		{
			CClient *pClient = (CClient*)*iter;
			//�p�G�Τ�ݪ��s���٦s�b�A�h�_�}�s���A������h�X
			if (pClient->IsConning())
			{
				pClient->DisConning();
			}
			++iter;
		}
		//���}�{�ɰ�
		LeaveCriticalSection(&csClientList);

		//���s���Τ�ݰ�����ɶ��A�Ϩ�۰ʰh�X
		Sleep(TIMEFOR_THREAD_SLEEP);

		//�i�J�{�ɰ�
		EnterCriticalSection(&csClientList);

		//�T�O���C�ӥΤ�ݤ��t���O����Ŷ����^���C
		//�p�G���[�Jwhile�o�h�j��A�i��s�b�o�˪����p�C��pClient->IsExit()�ɡA�Ӱ�����٨S���h�X�C
		//����N�ݭn�q����}�l�������s�P�_�C
		while ( 0 != clientlist.size())
		{
			iter = clientlist.begin();
			for (iter; iter != clientlist.end();)
			{
				CClient *pClient = (CClient*)*iter;
				if (pClient->IsExit())			//�Τ�ݰ�����w�g�h�X
				{
					clientlist.erase(iter++);	//�R���`�I
					delete pClient;				//����O����Ŷ�
					pClient = NULL;
				}else{
					iter++;						//���w�U��
				}
			}
			//���s���Τ�ݰ�����ɶ��A�Ϩ�۰ʰh�X
			Sleep(TIMEFOR_THREAD_SLEEP);
		}
		LeaveCriticalSection(&csClientList);//���}�{�ɰ�

	}

	clientlist.clear();		//�M�����

	SetEvent(hServerEvent);	//�q���D������h�X

	return 0;
}
