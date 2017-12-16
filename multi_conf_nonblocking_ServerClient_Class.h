// multi_conf_nonblocking_Server(Client_Class).h  伺服器程式(定義客戶端類別)
/* 伺服器程式共有三個:
   multi_conf_nonblocking_Server.cpp
   multi_conf_nonblocking_Server(Client_Function).cpp
   multi_conf_nonblocking_Server(Client_Class).h

   配合的客戶端程式為: multi_conf_nonblocking_Client.cpp
*/
#ifndef _CLIENT_H //ifndef-> 如果尚未定義, 此功能用以避免重複定義Client.h內所含的東西
#define _CLIENT_H
#include <winsock2.h>
#include <list>
using namespace std;
#define TIMEFOR_THREAD_CLIENT		500		//執行緒睡眠時間

#define	MAX_NUM_CLIENT		10				//接受的用戶端連接最多數量
#define	MAX_NUM_BUF			48				//緩衝區的最大長度
#define INVALID_OPERATOR	1				//無效的操作符
#define INVALID_NUM			2				//分母為零
#define ZERO				0				//零

//資料包類型
#define EXPRESSION			'E'				//算數運算式
#define BYEBYE				'B'				//消息byebye
#define CONVERSATION		'C'				//對話
#define HEADERLEN			(sizeof(hdr))	//頭長度

//資料包頭結構，該結構在win32下為4byte
typedef struct _head
{
	char			type;	//類型		
	unsigned short	len;	//資料包的長度(包括頭的長度)
}hdr, *phdr;

//資料包中的資料結構
typedef struct _data 
{
	char	buf[MAX_NUM_BUF];//數據
}DATABUF, *pDataBuf;


class CClient
{
	public:
		typedef list<CClient*> CLIENTLIST; 
		CClient(const SOCKET sClient,const sockaddr_in &addrClient, int seq, CRITICAL_SECTION& cs_ctlist);
		virtual ~CClient();

	public:
		BOOL        GetClientList(CLIENTLIST& ctlist);
		BOOL		StartRuning(void);					//創建發送和接收資料執行緒
		void		HandleData(const char* pExpr);		//計算運算式
		BOOL		IsConning(void){					//是否連接存在
						return m_bConning;
					} 
		void		DisConning(void){					//斷開與用戶端的連接
						m_bConning = FALSE;
					}
		BOOL		IsExit(void){						//接收和發送執行緒是否已經退出
						return m_bExit;
					}

	public:
		static DWORD __stdcall	 RecvDataThread(void* pParam);		//接收用戶端資料
		static DWORD __stdcall	 SendDataThread(void* pParam);		//向用戶端發送資料
		int         seq;                                //客戶端編號

	private:
		CClient();
	private:
		SOCKET		m_socket;			//通訊端
		sockaddr_in	m_addr;				//地址
		DATABUF		m_data;				//數據
		HANDLE		m_hEvent;			//事件物件
		HANDLE		m_hThreadSend;		//發送資料執行緒控制碼
		HANDLE		m_hThreadRecv;		//接收資料執行緒控制碼
		CRITICAL_SECTION m_cs;			//臨界區對象
		BOOL		m_bConning;			//用戶端連接狀態
		BOOL		m_bExit;			//執行緒退出
		CLIENTLIST  m_clientlist;       //管理客戶端連線的鏈表(List)
		CRITICAL_SECTION m_cs_clientlist;	//保護客戶端連線的鏈表的臨界區(Critical section)物件
};
#endif
