// multi_conf_nonblocking_Client.cpp   �Ȥ�ݵ{��

/* �t�X�T�Ӧ��A���{��:
   multi_conf_nonblocking_Server.cpp
   multi_conf_nonblocking_Server(Client_Function).cpp
   multi_conf_nonblocking_Server(Client_Class).h
*/

#include <string.h>
#include <iostream>
#include <winsock2.h>//wincosk2.h���twindows.h���ޤJ,��windows.h���S�twinbase.h���ޤJ

#pragma comment(lib, "WS2_32.lib")
using namespace std;

#define CLIENT_SETUP_FAIL			1		//�ҰʥΤ�ݥ���
#define CLIENT_CREATETHREAD_FAIL	2		//�Ыذ��������
#define CLIENT_CONNECT_FAIL			3		//�����s���_�}
#define TIMEFOR_THREAD_EXIT			1000	//�l������h�X�ɶ�
#define TIMEFOR_THREAD_SLEEP		500		//������ίv�ɶ�

#define	SERVERIP			"127.0.0.1"		//���A��IP
#define	SERVERPORT			5556			//���A��TCP��
#define	MAX_NUM_BUF			48				//�w�İϪ��̤j����
#define ADD					'+'				//+
#define SUB					'-'				//- 
#define MUT					'*'				//*
#define DIV					'/'				///
#define EQU					'='				//=

//��ƥ]����
#define EXPRESSION			'E'				//��ƹB�⦡
#define BYEBYE				'B'				//����byebye
#define CONVERSATION		'C'				//���
#define HEADERLEN			(sizeof(hdr))	//�Y����

//��ƥ]�Y���c�ӵ��c�bwin32xp�U��4byte
typedef struct _head
{
	char			type;//����		
	unsigned short	len;//��ƥ]������(�]�A�Y������)
}hdr, *phdr;

//��ƥ]������Ƶ��c
typedef struct _data 
{
	char	buf[MAX_NUM_BUF];
}DATABUF, *pDataBuf;

SOCKET	sClient;							//�Ȥ��Socket
HANDLE	hThreadSend;						//�ǰe��ư��������X
HANDLE	hThreadRecv;						//������ư��������X
DATABUF bufSend;							//�ǰe��ƽw�İ�
DATABUF bufRecv;							//������ƽw�İ�
CRITICAL_SECTION csSend;					//�{�ɰϪ���A�T�O�D������M�ǰe��ư������bufSend�ܼƪ������s��
CRITICAL_SECTION csRecv;					//�{�ɰϪ���A�T�O�D������M�ǰe��ư������bufRecv�ܼƪ������s��
BOOL	bSendData;							//�q���ǰe��ư�����ǰe��ƪ����L�ܼ�
HANDLE	hEventShowDataResult;				//��ܭp�⵲�G���ƥ�
BOOL	bConnecting;						//�P���A�����s�����A
HANDLE	arrThread[2];						//�l������}�C


BOOL	InitClient(void);					//��l��
BOOL	ConnectServer(void);				//�s�����A��
BOOL	CreateSendAndRecvThread(void);		//�Ыصo�e�M������ư����
void	InputAndOutput(void);				//�ϥΪ̿�J���
void	ExitClient(void);					//�h�X

void	InitMember(void);					//��l�ƥ����ܼ�
BOOL    InitSockt(void);					//�Ы�SOCKET

DWORD __stdcall	RecvDataThread(void* pParam);		//������ư����
DWORD __stdcall	SendDataThread(void* pParam);		//�o�e��ư����

BOOL	PackByebye(const char* pExpr);		//�N��J��"Byebye" "byebye"���r�ꥴ�]
BOOL	PackExpression(const char *pExpr);	//�N��J����ƹB�⦡���]
BOOL	Conversation(const char* pExpr);

void	ShowConnectMsg(BOOL bSuc);			//��ܳs�����A������
void	ShowDataResultMsg(void);			//��ܳs�p�⵲�G
void	ShowTipMsg(BOOL bFirstInput);		//��ܴ��ܸ�T


int main(int argc, char* argv[])
{	
	//��l��
	if (!InitClient())
	{	
		ExitClient();
		return CLIENT_SETUP_FAIL;
	}	
	
	//�s�����A��
	if (ConnectServer())
	{
		ShowConnectMsg(TRUE);	
	}else{
		ShowConnectMsg(FALSE);		
		ExitClient();
		return CLIENT_SETUP_FAIL;		
	}
	
	//�Ыصo�e�M������ư����
	if (!CreateSendAndRecvThread())
	{
		ExitClient();
		return CLIENT_CREATETHREAD_FAIL;
	}
	
	//�ϥΪ̿�J��ƩM��ܵ��G
	InputAndOutput();
	
	//�h�X
	ExitClient();
	
	return 0;
}


/**
 *	��l��
 */
BOOL	InitClient(void)
{
	//��l�ƥ����ܼ�
	InitMember();

	//�Ы�SOCKET
	if (!InitSockt())
	{
		return FALSE;
	}

	return TRUE;	
}


/**
 * ��l�ƥ����ܼ�
 */
void	InitMember(void)
{
	//��l���{�ɰ�
	InitializeCriticalSection(&csSend);
	InitializeCriticalSection(&csRecv);

	sClient = INVALID_SOCKET;	//�q�T��
	hThreadRecv = NULL;			//������ư��������X
	hThreadSend = NULL;			//�o�e��ư��������X
	bConnecting = FALSE;		//���s�����A
	bSendData = FALSE;			//���o�e��ƪ��A	

	//��l�Ƽƾڽw�İ�
	memset(bufSend.buf, 0, MAX_NUM_BUF);
	memset(bufRecv.buf, 0, MAX_NUM_BUF);
	memset(arrThread, 0, 2);
	
	//��ʳ]�m�ƥ�A��l�Ƭ��L�H�����A
	hEventShowDataResult = (HANDLE)CreateEvent(NULL, TRUE, FALSE, NULL);
}	
	

/**
 * �ЫثD����q�T��
 */
BOOL    InitSockt(void)
{
	int			reVal;	//��^��
	WSADATA		wsData;	//WSADATA�ܼ�	
	reVal = WSAStartup(MAKEWORD(2,2),&wsData);//��l��Windows Sockets Dll
	
	//�Ыسq�T��		
	sClient = socket(AF_INET, SOCK_STREAM, 0);
	if(INVALID_SOCKET == sClient)
		return FALSE;

	
	//�]�m�q�T�ݫD����Ҧ�
	unsigned long ul = 1;
	reVal = ioctlsocket(sClient, FIONBIO, (unsigned long*)&ul);
	if (reVal == SOCKET_ERROR)
		return FALSE;

	return TRUE;
}	
	

/**
 * �s�����A��
 */
BOOL	ConnectServer(void)
{
	int reVal;			//��^��
	sockaddr_in serAddr;//���A���a�}
	
	serAddr.sin_family = AF_INET;
	serAddr.sin_port = htons(SERVERPORT);
	serAddr.sin_addr.S_un.S_addr = inet_addr(SERVERIP);

	cout << "�s�u��...\n";
	for (;;)
	{
		//�s�����A��
		reVal = connect(sClient, (struct sockaddr*)&serAddr, sizeof(serAddr));
		
		//�B�z�s�����~
		if(SOCKET_ERROR == reVal)
		{
			int nErrCode = WSAGetLastError();
			if( WSAEWOULDBLOCK == nErrCode ||//�s���٨S������
					 WSAEINVAL == nErrCode || 
				   WSAEALREADY == nErrCode )
			{
				continue;
			}else if (WSAEISCONN == nErrCode)//�s���w�g����
			{
				break;
			}else //�䥦��]�A�s������ 
			{
				return FALSE;
			}
		}	
		
		if ( reVal == 0 )//�s�����\
			break;		
	}

	bConnecting = TRUE;
	return TRUE;
}

/**
 * ��ܳs�����A�����Ѹ�T
 */
void	ShowConnectMsg(BOOL bSuc)
{
	if (bSuc)
	{
		cout << "******************************" << endl;
		cout << "*                            *" << endl;
		cout << "* Succeed to connect server! *" << endl;
		cout << "*                            *" << endl;
		cout << "******************************" << endl;
	}else{
		cout << "***************************" << endl;
		cout << "*                         *" << endl;
		cout << "* Fail to connect server! *" << endl;
		cout << "*                         *" << endl;
		cout << "***************************" << endl;
	}
	
    return;
}

/**
 * �Ыصo�e�M������ư����
 */
BOOL	CreateSendAndRecvThread(void)
{
	//�Ыر�����ƪ������
	unsigned long ulThreadId;
	hThreadRecv = CreateThread(NULL, 0, RecvDataThread, NULL, 0, &ulThreadId);
	if (NULL == hThreadRecv)
		return FALSE;
	
	//�Ыصo�e��ƪ������
	hThreadSend = CreateThread(NULL, 0, SendDataThread, NULL, 0, &ulThreadId);
	if (NULL == hThreadSend)
		return FALSE;

	//�K�[�������}�C
	arrThread[0] = hThreadRecv;
	arrThread[1] = hThreadSend;	
	return TRUE;
}

/**
 * ��J��ƩM��ܵ��G
 */
void	InputAndOutput(void)
{
	char cInput[MAX_NUM_BUF];	//�Τ��J�w�İ�	
	BOOL bFirstInput = TRUE;	//�Ĥ@���u���J��ƹB�⦡
	
	ShowTipMsg(bFirstInput);		//���ܿ�J��T
	for (;bConnecting;)			//�s�����A
	{
		memset(cInput, 0, MAX_NUM_BUF);		
		cin.getline(cInput,MAX_NUM_BUF);
		char *pTemp = cInput;
		if (bFirstInput)				//�Ĥ@����J
		{
			if (!PackExpression(pTemp))	//��ƹB�⦡���]
			{
				if(!Conversation(pTemp))
				continue;				//���s��J
			}
			bFirstInput = FALSE;		//���\��J�Ĥ@�Ӻ�ƹB�⦡
			
		}else if (!PackByebye(pTemp))	//��Byebye����byebye�����]
		{			
			if (!PackExpression(pTemp))	//��ƹB�⦡���]
			{
				if(!Conversation(pTemp))
				continue;				//���s��J
			}
		}
		
	}

	if (!bConnecting)			//�P���A���s���w�g�_�}
	{
		ShowConnectMsg(FALSE);	//��ܸ�T
		Sleep(TIMEFOR_THREAD_EXIT);
	}

	//���ݸ�Ƶo�e�M����������h�X
	DWORD reVal = WaitForMultipleObjects(2, arrThread, TRUE, INFINITE);
	if (WAIT_ABANDONED_0 == reVal)
	{
		int nErrCode = GetLastError();
	}
}


/**
 * ���]�p��B�⦡�����
 */
BOOL	PackExpression(const char *pExpr)
{
	
	char* pTemp = (char*)pExpr;	//��ƹB�⦡�Ʀr�}�l����m

	while (*pTemp == ' ')//while (!*pTemp)				//�Ĥ@�ӼƦr��m
		pTemp++;		
	
	char* pos1 = pTemp;	//�Ĥ@�ӼƦr��m
	char* pos2 = NULL;	//�B��l��m
	char* pos3 = NULL;	//�ĤG�ӼƦr��m
	int len1 = 0;		//�Ĥ@�ӼƦr����
	int len2 = 0;		//�B��l����
	int len3 = 0;		//�ĤG�ӼƦr����

	//�Ĥ@�Ӧr�������O+ - �Ϊ̬O�Ʀr
	if ((*pTemp != '+') && 
		(*pTemp != '-') &&
		((*pTemp < '0') || (*pTemp > '9')))
	{
		return FALSE;
	}

	
	if ((*pTemp++ == '+')&&(*pTemp < '0' || *pTemp > '9'))	//�Ĥ@�Ӧr���O'+'�ɡA�ĤG�ӥ����O�Ʀr	
		return FALSE;										//���s��J
	--pTemp;												//�W�����w
	
	
	if ((*pTemp++ == '-')&&(*pTemp < '0' || *pTemp > '9'))	//�Ĥ@�Ӧr���O'-'��,�ĤG�ӥ����O�Ʀr	
		return FALSE;										//���s��J
	--pTemp;												//�W�����w
	
	char* pNum = pTemp;						//�Ʀr�}�l����m					
	if (*pTemp == '+'||*pTemp == '-')		//+ -
		pTemp++;
	
	while (*pTemp >= '0' && *pTemp <= '9')	//�Ʀr
		pTemp++;							
	
	len1 = pTemp - pNum;//�Ʀr����						
	
	//�i�঳�Ů�
	while(*pTemp == ' ')//while(!*pTemp)							
		pTemp++;
	
	//��ƹB��l
	if ((ADD != *pTemp)&&			
		(SUB != *pTemp)&&
		(MUT != *pTemp)&&
		(DIV != *pTemp))
		return FALSE;
	
	pos2 = pTemp;
	len2 = 1;
	
	//�U�����w
	pTemp++;
	//�i�঳�Ů�
	while(*pTemp == ' ')//while(!*pTemp)
		pTemp++;
	
	//��2�ӼƦr��m
	pos3 = pTemp;
	if (*pTemp < '0' || *pTemp > '9')
		return FALSE;//���s��J			
	
	while (*pTemp >= '0' && *pTemp <= '9')//�Ʀr
		pTemp++;
	
	while(*pTemp == ' ')//while(!*pTemp)
		pTemp++;

	if (EQU != *pTemp)	//�̫�O����
		return FALSE;	//���s��J
	
	len3 = pTemp - pos3;//�Ʀr����

	int nExprlen = len1 + len2 + len3;	//��ƪ�ܪ���

	//�B�⦡Ū�J�o�e��ƽw�İ�
	EnterCriticalSection(&csSend);		//�i�J�{�ɰ�
	//�ƾڥ]�Y
	phdr pHeader = (phdr)(bufSend.buf);
	pHeader->type = EXPRESSION;			//����
	pHeader->len = nExprlen + HEADERLEN;//�ƾڥ]����
	//�������
	memcpy(bufSend.buf + HEADERLEN, pos1, len1);
	memcpy(bufSend.buf + HEADERLEN + len1, pos2, len2);
	memcpy(bufSend.buf + HEADERLEN + len1 + len2 , pos3,len3);
	LeaveCriticalSection(&csSend);		//���}�{�ɰ�
	pHeader = NULL;

	bSendData = TRUE;					//�q���o�e��ư�����o�e���

	return TRUE;
}


/**
 * ���]�o�ebyebye���
 */
BOOL	PackByebye(const char* pExpr)
{
	BOOL reVal = FALSE;
	
	if(!strcmp("Byebye", pExpr)||!strcmp("byebye", pExpr))		//�p�G�O"Byebye" "byebye"
	{
		EnterCriticalSection(&csSend);							//�i�J�{�ɰ�
		phdr pHeader = (phdr)bufSend.buf;						//�j���ഫ
		pHeader->type = BYEBYE;									//����
		pHeader->len = HEADERLEN + strlen("Byebye");			//�ƾڥ]����
		memcpy(bufSend.buf + HEADERLEN, pExpr, strlen(pExpr));	//�ƻs���
		LeaveCriticalSection(&csSend);							//���}�{�ɰ�		
		
		pHeader = NULL;											//null
		bSendData = TRUE;										//�q���o�e��ư����
		reVal = TRUE;		
	}
	
	return reVal;
}


/**
 * ���]�o�e �|�� ���
 */
BOOL	Conversation(const char* pExpr)
{
	BOOL reVal = FALSE;

	if(strlen(pExpr)>0)
	{
		EnterCriticalSection(&csSend);							//�i�J�{�ɰ�
		phdr pHeader = (phdr)bufSend.buf;						//�j���ഫ
		pHeader->type = CONVERSATION;									//����
		pHeader->len = HEADERLEN + strlen(pExpr);			//�ƾڥ]����
		memcpy(bufSend.buf + HEADERLEN, pExpr, strlen(pExpr));	//�ƻs���
		LeaveCriticalSection(&csSend);							//���}�{�ɰ�		
		
		pHeader = NULL;											//null
		bSendData = TRUE;										//�q���o�e��ư����
		reVal = TRUE;
	}
		return reVal;
}



/**
 * �B�⦡���G
 */
void	ShowDataResultMsg(void)
{
	EnterCriticalSection(&csRecv);
	cout <<  "\t" << bufRecv.buf <<endl;
	LeaveCriticalSection(&csRecv);
		
}

/**
 * ���ܸ�T
 */
void	ShowTipMsg(BOOL bFirstInput)
{
	if (bFirstInput)//�������
	{
		cout << "**********************************" << endl;
		cout << "*                                *" << endl;
		cout << "* Please input expression.       *" << endl;
		cout << "* Usage:NumberOperatorNumber=    *" << endl;
		cout << "*                                *" << endl;
		cout << "* If you want to exit.           *" << endl;
		cout << "* Usage: Byebye or byebye        *" << endl;
		cout << "**********************************" << endl;
	}else{
		cout << "**********************************" << endl;
		cout << "*                                *" << endl;
		cout << "* Please input: expression       *" << endl;
		cout << "* Usage:NumberOperatorNumber=    *" << endl;
		cout << "*                                *" << endl;
		cout << "* If you want to exit.           *" << endl;
		cout << "* Usage: Byebye or byebye        *" << endl;
		cout << "*                                *" << endl;
		cout << "**********************************" << endl;
	}	
	
}

/**
 * �Τ�ݰh�X
 */
void	ExitClient(void)			
{
	DeleteCriticalSection(&csSend);
	DeleteCriticalSection(&csRecv);
	CloseHandle(hThreadRecv);
	CloseHandle(hThreadSend);
	closesocket(sClient);
	WSACleanup();
	return;
}

/**
 * �o�e��ư����
 */
DWORD __stdcall	SendDataThread(void* pParam)			
{	
	while(bConnecting)						//�s�����A
	{
		if (bSendData)						//�o�e���
		{
			EnterCriticalSection(&csSend);	//�i�J�{�ɰ�
			for (;;)
			{
				int nBuflen = ((phdr)(bufSend.buf))->len;		
				int val = send(sClient, bufSend.buf, nBuflen,0);
				
				//�B�z��^���~
				if (SOCKET_ERROR == val)
				{
					int nErrCode = WSAGetLastError();
					if(WSAEWOULDBLOCK == nErrCode)		//�o�e�w�İϤ��i��
					{
						continue;						//�~��j��
					}else {
						LeaveCriticalSection(&csSend);	//���}�{�ɰ�
						bConnecting = FALSE;			//�_�}���A
						SetEvent(hEventShowDataResult);	//�q���D������A����b�L���������ݪ�^���G�C
						return 0;
					}				
				}				
			
				bSendData = FALSE;			//�o�e���A
				break;						//���Xfor
			}			
			LeaveCriticalSection(&csSend);	//���}�{�ɰ�
		}
			
		Sleep(TIMEFOR_THREAD_SLEEP);		//������ίv
	}
	
	return 0;
}


/**
 * ������ư����
 */
DWORD __stdcall	RecvDataThread(void* pParam)				
{
	int		reVal;				//��^��
	char	temp[MAX_NUM_BUF];	//�ϰ��ܼ�
	memset(temp, 0, MAX_NUM_BUF);

	while(bConnecting)			//�s�����A		
	{	
		reVal = recv(sClient, temp, MAX_NUM_BUF, 0);//�������
		
		if (SOCKET_ERROR == reVal)
		{
			int nErrCode = WSAGetLastError();
			if (WSAEWOULDBLOCK == nErrCode)			//������ƽw�İϤ��i��
			{
				Sleep(TIMEFOR_THREAD_SLEEP);		//������ίv
				continue;							//�~�򱵦����
			}else{
				bConnecting = FALSE;
				SetEvent(hEventShowDataResult);		//�q���D������A����b�L����������
				return 0;							//������h�X				
			}
		}
		
		if ( reVal == 0)							//���A�������F�s��
		{
			bConnecting = FALSE;					//������h�X
			SetEvent(hEventShowDataResult);			//�q���D������A����b�L����������
			return 0;								//������h�X 
		}
		
		if (reVal > HEADERLEN && -1 != reVal)		//������
		{
			//��ƾڸѥ]
 			phdr header = (phdr)(temp);
			if (BYEBYE != ((phdr)header)->type)	    
			{
					EnterCriticalSection(&csRecv);		
					memset(bufRecv.buf, 0, MAX_NUM_BUF);	
					memcpy(bufRecv.buf, temp + HEADERLEN, header->len - HEADERLEN);	//�N��Ƶ��G�ƻs�챵����ƽw�İ�
					LeaveCriticalSection(&csRecv);
					ShowDataResultMsg();				//��ܸ��
			}
			else //�p�G�O��byebye���Ϊ̡�Byebye��
			{
					EnterCriticalSection(&csRecv);		
					memset(bufRecv.buf, 0, MAX_NUM_BUF);	
					memcpy(bufRecv.buf, temp + HEADERLEN, header->len - HEADERLEN);	//�N��Ƶ��G�ƻs�챵����ƽw�İ�
					LeaveCriticalSection(&csRecv);
					ShowDataResultMsg();				//��ܸ��

					if (0 == strcmp(bufRecv.buf, "OK"))	//�Τ�ݥD�ʰh�X
					{
						bConnecting = FALSE;
						Sleep(TIMEFOR_THREAD_EXIT);		//����Ʊ����M�o�e������h�X�ɶ�
						exit(0);
					}			
			}

			memset(temp, 0, MAX_NUM_BUF);
		}
		
		Sleep(TIMEFOR_THREAD_SLEEP);//������ίv
	}
	return 0;
}

