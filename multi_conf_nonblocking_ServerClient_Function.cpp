// multi_conf_nonblocking_Server(Client_Function).cpp  ���A���{��(�w�q�Ȥ�ݨ禡)
/* ���A���{���@���T��:
   multi_conf_nonblocking_Server.cpp
   multi_conf_nonblocking_Server(Client_Function).cpp
   multi_conf_nonblocking_Server(Client_Class).h

   �t�X���Ȥ�ݵ{����: nonblocking_Client.cpp
*/
#include <stdio.h>
#include "multi_conf_nonblocking_Server(Client_Class).h"

/*
 * �c�y���
 */
CClient::CClient(const SOCKET sClient, const sockaddr_in &addrClient, int seq_, CRITICAL_SECTION& cs_ctlist)
{
	//��l���ܼ�
	m_hThreadRecv = NULL;
	m_hThreadSend = NULL;
	m_socket = sClient;
	m_addr = addrClient;
	m_bConning = FALSE;
	m_bExit = FALSE;
	memset(m_data.buf, 0, MAX_NUM_BUF);
	seq = seq_;
	m_cs_clientlist = cs_ctlist;

	//�Ыبƥ�
	m_hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);//��ʳ]�m�H�����A�A��l�Ƭ��L�H�����A

	//��l���{�ɰ�
	InitializeCriticalSection(&m_cs);
}


/*
 * �R�c���
 */
CClient::~CClient()
{
	closesocket(m_socket);			//�����q�T��
	m_socket = INVALID_SOCKET;		//�q�T�ݵL��
	DeleteCriticalSection(&m_cs);	//�����{�ɰϹ�H
	CloseHandle(m_hEvent);			//����ƥ󪫥�
}


/*
 * ���o�D�{��nonblocking_Server.cpp  �������ܼ� clientlist ����}
 */
BOOL CClient::GetClientList(CLIENTLIST& ctlist)
{
	m_clientlist = ctlist;
	return TRUE;
}


/*
 * �Ыصo�e�M������ư����
 */
BOOL CClient::StartRuning(void)
{
	m_bConning = TRUE;//�]�m�s�����A

	//�Ыر�����ư����
	unsigned long ulThreadId;
	m_hThreadRecv = CreateThread(NULL, 0, RecvDataThread, this, 0, &ulThreadId);
	if(NULL == m_hThreadRecv)
	{
		return FALSE;
	}else{
		CloseHandle(m_hThreadRecv);
	}

	//�Ыر����Τ�ݸ�ƪ������
	m_hThreadSend =  CreateThread(NULL, 0, SendDataThread, this, 0, &ulThreadId);
	if(NULL == m_hThreadSend)
	{
		return FALSE;
	}else{
		CloseHandle(m_hThreadSend);
	}

	return TRUE;
}


/*
 * �����Τ�ݸ��
 */
DWORD  CClient::RecvDataThread(void* pParam)
{
	CClient *pClient = (CClient*)pParam;	//�Τ�ݹ�H����
	int		reVal;							//��^��
	char	temp[MAX_NUM_BUF];				//�{���ܼ�

	memset(temp, 0, MAX_NUM_BUF);

	for (;pClient->m_bConning;)				//�s�����A
	{
		reVal = recv(pClient->m_socket, temp, MAX_NUM_BUF, 0);	//�������

		//�B�z���~��^��
		if (SOCKET_ERROR == reVal)
		{
			int nErrCode = WSAGetLastError();

			if ( WSAEWOULDBLOCK == nErrCode )	//������ƽw�İϤ��i��
			{
				continue;						//�~��j��
			}else if (WSAENETDOWN == nErrCode ||//�Τ�������F�s��
					 WSAETIMEDOUT == nErrCode ||
					WSAECONNRESET == nErrCode )
			{
				break;							//������h�X
			}
		}

		//�Τ�������F�s��
		if ( reVal == 0)
		{
			break;
		}

		//������
		if (reVal > HEADERLEN)
		{
			pClient->HandleData(temp);		//�B�z���

			SetEvent(pClient->m_hEvent);	//�q���o�e��ư����

			memset(temp, 0, MAX_NUM_BUF);	//�M���{���ܼ�
		}

		Sleep(TIMEFOR_THREAD_CLIENT);		//������ίv
	}

	pClient->m_bConning = FALSE;			//�P�Τ�ݪ��s���_�}
	SetEvent(pClient->m_hEvent);			//�q���o�e��ư�����h�X

	return 0;								//������h�X
}

/*
 *  �p��B�⦡,���]���
 */
void CClient::HandleData(const char* pExpr)
{
	memset(m_data.buf, 0, MAX_NUM_BUF);//�M��m_data
	//�p�G�O��byebye���Ϊ̡�Byebye��
	if (BYEBYE == ((phdr)pExpr)->type)
	{
		EnterCriticalSection(&m_cs);
		phdr pHeaderSend = (phdr)m_data.buf;				//�o�e�����
		pHeaderSend->type = BYEBYE;							//�������
		pHeaderSend->len = HEADERLEN + strlen("OK");		//�ƾڥ]����
		memcpy(m_data.buf + HEADERLEN, "OK", strlen("OK"));	//�ƻs��ƨ�m_data"
		LeaveCriticalSection(&m_cs);

	}

	//��ƹB�⦡
	if (EXPRESSION == ((phdr)pExpr)->type)
	{
		int nFirNum;		//�Ĥ@�ӼƦr
		int nSecNum;		//�ĤG�ӼƦr
		char cOper;			//��ƹB��l
		int nResult;		//�p�⵲�G
		//�榡��Ū�J���
		sscanf(pExpr + HEADERLEN, "%d%c%d", &nFirNum, &cOper, &nSecNum);

		//�p��
		switch(cOper)
		{
			case '+'://�[
				{
					nResult = nFirNum + nSecNum;
					break;
				}
			case '-'://��
				{
					nResult = nFirNum - nSecNum;
					break;
				}
			case '*'://��
				{
					nResult = nFirNum * nSecNum;
					break;
				}
			case '/'://��
				{
					if (ZERO == nSecNum)//�L�Ī��Ʀr
					{
						nResult = INVALID_NUM;
					}else
					{
						nResult = nFirNum / nSecNum;
					}
					break;
				}
			default:
				nResult = INVALID_OPERATOR;//�L�ľާ@��
				break;
		}

		//�N��ƹB�⦡�M�p�⪺���G�g�J�r���}�C��
		char temp[MAX_NUM_BUF];
		char cEqu = '=';
		sprintf(temp, "%d%c%d%c%d",nFirNum, cOper, nSecNum,cEqu, nResult);

		//���]�ƾ�
		EnterCriticalSection(&m_cs);
		phdr pHeaderSend = (phdr)m_data.buf;				//�o�e�����
		pHeaderSend->type = EXPRESSION;						//�����������ƹB�⦡
		pHeaderSend->len = HEADERLEN + strlen(temp);		//�ƾڥ]������
		memcpy(m_data.buf + HEADERLEN, temp, strlen(temp));	//�ƻs��ƨ�m_data
		LeaveCriticalSection(&m_cs);

	}

	//�|��
	if (CONVERSATION == ((phdr)pExpr)->type)
	{
		//���]�ƾ�
/*[mark]HEAD
    �Ч������]��ܪ��ƾڨ�m_data.buf
�y�{:
    1.�i�J �ʥ] ���{�ɰ�
    2.�� pHeaderSend ���Шӭק� m_data.buf �����e
    3.�� type �]�� CONVERSATION
    4.�� len �]�� pExpr�ʥ] �����e������
    5.���Ʊq pExpr��buf �ƻs�� m_data��buf
    6.���} �ʥ] ���{�ɰ�
*/
		EnterCriticalSection(&m_cs);
		phdr pHeaderSend = (phdr)m_data.buf;		//�o�e�����
		pHeaderSend->type = CONVERSATION;
		pHeaderSend->len = ((phdr)pExpr)->len;		//�ƾڥ]������
		memcpy(m_data.buf + HEADERLEN, pExpr + HEADERLEN, ((phdr)pExpr)->len - HEADERLEN);	//�ƻs��ƨ�m_data.buf
		LeaveCriticalSection(&m_cs);
//[mark]END
	}
}


/*
 * //�V�Τ�ݵo�e���
 */
DWORD CClient::SendDataThread(void* pParam)
{
	int val;
	CClient *pClient = (CClient*)pParam;//�ഫ���������CClient���w

	for (;pClient->m_bConning;)//�s�����A
	{
		//����ƥ�q��
		if (WAIT_OBJECT_0 == WaitForSingleObject(pClient->m_hEvent, INFINITE))
		{
			//��Τ�ݪ��s���_�}�ɡA������ư�������h�X�A�M��Ӱ������h�X�A�ó]�m�h�X�лx
			if (!pClient->m_bConning)
			{
				pClient->m_bExit = TRUE;
				break ;
			}

			//�ǰe�����Ȥ��
			//�i�J�{�ɰ� (�w�� m_data.buf)
			EnterCriticalSection(&pClient->m_cs);
			phdr pHeader = (phdr)pClient->m_data.buf;
			int nSendlen = pHeader->len;
/*[mark]HEAD
    �Юھڸ�ƫ��A�P�_�p��ǰe��ƨ쥻�Ȥ��,�P�ɤ]�n��send()�禡����^���~�i��B�z�C
�y�{:
    1.�p�G�ʥ]�������O CONVERSATION �~�|�ǰe���ӫȤ��
    2.�ǰe�ʥ]���ӫȤ��
    3.�B�z���~�A�p�G�s���W�ӫȤ�A�N��L����Ƴ]�� ���}
*/
			if (CONVERSATION != ((phdr)pHeader)->type)      //[1]
			{
				val = send(pClient->m_socket, pClient->m_data.buf, nSendlen,0); //[2]

				if (SOCKET_ERROR == val)//[3]�B�z��^���~
				{
					int nErrCode = WSAGetLastError();
					if (nErrCode == WSAEWOULDBLOCK)//�o�e��ƽw�İϤ��i��
					{
						continue;
					}else if ( WSAENETDOWN == nErrCode ||
							  WSAETIMEDOUT == nErrCode ||
							  WSAECONNRESET == nErrCode)//�Τ�������F�s��
					{
						pClient->m_bConning = FALSE;	//�s���_�}   [���D1]
						pClient->m_bExit = TRUE;		//������h�X [���D1]
					}else {
						pClient->m_bConning = FALSE;	//�s���_�}   [���D1]
						pClient->m_bExit = TRUE;		//������h�X [���D1]
					}
				}
			}
/*[mark]END
���D:
    1.�b[�y�{3]�U����� else �����Ʊ����@�ˡA�����򤣯�g�b�@�_
*/

			//�s���ܨ�L�Ȥ�
			EnterCriticalSection(&pClient->m_cs_clientlist);//�i�J�{�ɰ� (�w�� m_clientlist)
			CLIENTLIST::iterator iter = pClient->m_clientlist.begin();
/*[mark]HEAD
    �Юھڸ�ƫ��A�P�_�p��s����ƨ��L�Ȥ�
�y�{:
    1.�w��C�ӫȤ�i��A���p�G�O�W�����ӫȤ�N���L
    2.�p�G�o�ӫʥ]���O byebye �N�ǰe����L�Τ�
    3.�p�G�O byebye �q����L�Ȥ�ݦ��Ȥ�n�h�X
    [�y�{3]:
        1.�إߤ@�ӷs�ʥ] m_data
        2.�N�L�����A�]��CONVERSATION
        3.�إߤ@�Ӧr���}�C temp
        4.�N�ۤv�]�w���T�����Ӱ}�C�A�T�����e�H�A���g
        5.�̰T�����e�]�w�o�ӫʥ]������
        6.�N�T���ƻs��ʥ]��buf
        7.�ǰe�ʥ]
*/
			for (iter; iter != pClient->m_clientlist.end();)
			{
				CClient *pClient_tmp = (CClient*)*iter;

				if (pClient_tmp->seq != pClient->seq)       //[1]
				{
					if (BYEBYE != ((phdr)pHeader)->type)    //[2]
					    val = send(pClient_tmp->m_socket, pClient->m_data.buf, nSendlen, 0);

					else if (BYEBYE == ((phdr)pHeader)->type) //[3]�q����L�Ȥ��, ���Ȥ�n�h�X
					{
						DATABUF		m_data;
						phdr pHeader = (phdr)m_data.buf;
						pHeader->type = CONVERSATION;
						char temp[20];
						sprintf(temp, "%s%d%s","Client ", pClient->seq, " exits\n");
						pHeader->len = HEADERLEN + strlen(temp);			//�ƾڥ]����
						memcpy(m_data.buf + HEADERLEN, temp, strlen(temp));	//�ƻs���
						val = send(pClient_tmp->m_socket, m_data.buf, MAX_NUM_BUF, 0);
					}
				}

				iter++;
			}
/*[mark]END
���D:
    [�y�{3]��ı�o�n��o�Ӭy�{�g�b�j�餧�~�A�o�̭��u�n�ǰe�T���N�n�A���M�ܮ��O�ɶ�
*/
			LeaveCriticalSection(&pClient->m_cs_clientlist);//���}�{�ɰ�

			//���\�o�e���
			LeaveCriticalSection(&pClient->m_cs);//���}�{�ɰ�
			ResetEvent(&pClient->m_hEvent);//�]�m�ƥ󬰵L�H�����A

			if(pClient->m_bExit == TRUE)//���Ȥ�ݫD���`���u
				break;
		}
	}

	return 0;
}
