// multi_conf_nonblocking_Server(Client_Class).h  ���A���{��(�w�q�Ȥ�����O)
/* ���A���{���@���T��:
   multi_conf_nonblocking_Server.cpp
   multi_conf_nonblocking_Server(Client_Function).cpp
   multi_conf_nonblocking_Server(Client_Class).h

   �t�X���Ȥ�ݵ{����: multi_conf_nonblocking_Client.cpp
*/
#ifndef _CLIENT_H //ifndef-> �p�G�|���w�q, ���\��ΥH�קK���Ʃw�qClient.h���ҧt���F��
#define _CLIENT_H
#include <winsock2.h>
#include <list>
using namespace std;
#define TIMEFOR_THREAD_CLIENT		500		//������ίv�ɶ�

#define	MAX_NUM_CLIENT		10				//�������Τ�ݳs���̦h�ƶq
#define	MAX_NUM_BUF			48				//�w�İϪ��̤j����
#define INVALID_OPERATOR	1				//�L�Ī��ާ@��
#define INVALID_NUM			2				//�������s
#define ZERO				0				//�s

//��ƥ]����
#define EXPRESSION			'E'				//��ƹB�⦡
#define BYEBYE				'B'				//����byebye
#define CONVERSATION		'C'				//���
#define HEADERLEN			(sizeof(hdr))	//�Y����

//��ƥ]�Y���c�A�ӵ��c�bwin32�U��4byte
typedef struct _head
{
	char			type;	//����		
	unsigned short	len;	//��ƥ]������(�]�A�Y������)
}hdr, *phdr;

//��ƥ]������Ƶ��c
typedef struct _data 
{
	char	buf[MAX_NUM_BUF];//�ƾ�
}DATABUF, *pDataBuf;


class CClient
{
	public:
		typedef list<CClient*> CLIENTLIST; 
		CClient(const SOCKET sClient,const sockaddr_in &addrClient, int seq, CRITICAL_SECTION& cs_ctlist);
		virtual ~CClient();

	public:
		BOOL        GetClientList(CLIENTLIST& ctlist);
		BOOL		StartRuning(void);					//�Ыصo�e�M������ư����
		void		HandleData(const char* pExpr);		//�p��B�⦡
		BOOL		IsConning(void){					//�O�_�s���s�b
						return m_bConning;
					} 
		void		DisConning(void){					//�_�}�P�Τ�ݪ��s��
						m_bConning = FALSE;
					}
		BOOL		IsExit(void){						//�����M�o�e������O�_�w�g�h�X
						return m_bExit;
					}

	public:
		static DWORD __stdcall	 RecvDataThread(void* pParam);		//�����Τ�ݸ��
		static DWORD __stdcall	 SendDataThread(void* pParam);		//�V�Τ�ݵo�e���
		int         seq;                                //�Ȥ�ݽs��

	private:
		CClient();
	private:
		SOCKET		m_socket;			//�q�T��
		sockaddr_in	m_addr;				//�a�}
		DATABUF		m_data;				//�ƾ�
		HANDLE		m_hEvent;			//�ƥ󪫥�
		HANDLE		m_hThreadSend;		//�o�e��ư��������X
		HANDLE		m_hThreadRecv;		//������ư��������X
		CRITICAL_SECTION m_cs;			//�{�ɰϹ�H
		BOOL		m_bConning;			//�Τ�ݳs�����A
		BOOL		m_bExit;			//������h�X
		CLIENTLIST  m_clientlist;       //�޲z�Ȥ�ݳs�u�����(List)
		CRITICAL_SECTION m_cs_clientlist;	//�O�@�Ȥ�ݳs�u������{�ɰ�(Critical section)����
};
#endif
