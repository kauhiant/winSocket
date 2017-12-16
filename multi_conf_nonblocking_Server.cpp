// multi_conf_nonblocking_Server.cpp  伺服器程式(主程式)
/* 伺服器程式共有三個:
   multi_conf_nonblocking_Server.cpp
   multi_conf_nonblocking_Server(Client_Function).cpp
   multi_conf_nonblocking_Server(Client_Class).h

   配合的客戶端程式為: multi_conf_nonblocking_Client.cpp
*/

#include <iostream>
using namespace std;
#include <winsock2.h> //wincosk2.h內含windows.h的引入,而windows.h內又含winbase.h的引入
#include "multi_conf_nonblocking_Server(Client_Class).h"

/*
 * 變數的第一個字母為小寫且該字母表明該變數的類型，如 nErrCode.為整形的錯誤代碼
 * 函數的第一個字母為大寫且表明函數的功能。如void AcceptThread(void).為接受連接的執行緒
 */
#pragma comment(lib, "ws2_32.lib")

typedef list<CClient*> CLIENTLIST;			//鏈表(List)

#define SERVERPORT			5556			//伺服器TCP埠
#define SERVER_SETUP_FAIL	1				//啟動伺服器失敗

#define TIMEFOR_THREAD_EXIT			5000	//主執行緒睡眠時間
#define TIMEFOR_THREAD_HELP			1500	//清理資源執行緒退出時間
#define TIMEFOR_THREAD_SLEEP		500		//等待用戶端請求執行緒睡眠時間

HANDLE	hThreadAccept;						//接受用戶端連接執行緒控制碼
HANDLE	hThreadHelp;						//釋放資源執行緒控制碼
SOCKET	sServer;							//監聽客戶端連線Socket
BOOL	bServerRunning;						//伺服器的目前工作狀態變數
HANDLE	hServerEvent;						//伺服器退出事件控制碼
CLIENTLIST			clientlist;				//管理客戶端連線的鏈表(List)
CRITICAL_SECTION	csClientList;			//保護鏈表的臨界區(Critical section)物件
int     seq;                                //客戶端序別

BOOL	InitSever(void);					//初始化
BOOL	StartService(void);					//啟動服務
void	StopService(void);					//停止服務
BOOL	CreateHelperAndAcceptThread(void);	//創建接收用戶端連接執行緒
void	ExitServer(void);					//伺服器退出

void	InitMember(void);					//初始化全域變數
BOOL	InitSocket(void);					//初始化SOCKET

void	ShowTipMsg(BOOL bFirstInput);		//顯示提示資訊
void	ShowServerStartMsg(BOOL bSuc);		//顯示伺服器已經啟動
void	ShowServerExitMsg(void);			//顯示伺服器正在退出

DWORD	__stdcall	HelperThread(void *pParam);			//釋放資源
DWORD	__stdcall 	AcceptThread(void *pParam);			//接受用戶端連接執行緒
void	ShowConnectNum();								//顯示用戶端的連接數目

int main(int argc, char* argv[])
{
	//初始化伺服器
	if (!InitSever())
	{
		ExitServer();
		return SERVER_SETUP_FAIL;
	}

	//啟動服務
	if (!StartService())
	{
		ShowServerStartMsg(FALSE);
		ExitServer();
		return SERVER_SETUP_FAIL;
	}

	//停止服務
	StopService();

	//伺服器退出
	ExitServer();

	return 0;
}

/**
 * 初始化
 */
BOOL	InitSever(void)
{

	InitMember();//初始化全域變數

	//初始化SOCKET
	if (!InitSocket())
		return FALSE;

	return TRUE;
}


/**
 * 初始化全域變數
 */
void	InitMember(void)
{
	InitializeCriticalSection(&csClientList);				//初始化臨界區
	hServerEvent = CreateEvent(NULL, TRUE, FALSE, NULL);	//手動設置事件，初始化為無資訊號狀態
	hThreadAccept = NULL;									//設置為NULL
	hThreadHelp = NULL;										//設置為NULL
	sServer = INVALID_SOCKET;								//設置為無效的通訊端
	bServerRunning = FALSE;									//伺服器為沒有運行狀態
	clientlist.clear();										//清空鏈表
	seq = 0;                                                //客戶端序號
}

/**
 *  初始化SOCKET
 */
BOOL InitSocket(void)
{
	//返回值
	int reVal;

	//初始化Windows Sockets DLL
	WSADATA  wsData;
	reVal = WSAStartup(MAKEWORD(2,2),&wsData);

	//創建通訊端
	sServer = socket(AF_INET, SOCK_STREAM, 0);
	if(INVALID_SOCKET== sServer)
		return FALSE;

	//設置通訊端非阻塞模式
	unsigned long ul = 1;
	reVal = ioctlsocket(sServer, FIONBIO, (unsigned long*)&ul);
	if (SOCKET_ERROR == reVal)
		return FALSE;

	//綁定通訊端
	sockaddr_in serAddr;
	serAddr.sin_family = AF_INET;
	serAddr.sin_port = htons(SERVERPORT);
	serAddr.sin_addr.S_un.S_addr = INADDR_ANY;
	reVal = bind(sServer, (struct sockaddr*)&serAddr, sizeof(serAddr));
	if(SOCKET_ERROR == reVal )
		return FALSE;

	//監聽
	reVal = listen(sServer, SOMAXCONN);
	if(SOCKET_ERROR == reVal)
		return FALSE;

	return TRUE;
}

/**
 *  啟動服務
 */
BOOL	StartService(void)
{
	BOOL reVal = TRUE;	//返回值

	ShowTipMsg(TRUE);	//提示用戶輸入

	char cInput;		//輸入字元
	do
	{
		cin >> cInput;
		if ('s' == cInput || 'S' == cInput)
		{
			if (CreateHelperAndAcceptThread())	//創建清理資源和接受用戶端請求的執行緒
			{
				ShowServerStartMsg(TRUE);		//創建執行緒成功資訊
			}else{
				reVal = FALSE;
			}
			break;//跳出循環體

		}else{
			ShowTipMsg(TRUE);
		}

	} while(cInput != 's' &&//必須輸入's'或者'S'字元
			cInput != 'S');

	return reVal;
}

/**
 *  停止服務
 */
void	StopService(void)
{
	BOOL reVal = TRUE;	//返回值

	ShowTipMsg(FALSE);	//提示用戶輸入

	char cInput;		//輸入的操作字元
	for (;bServerRunning;)
	{
		cin >> cInput;
		if (cInput == 'E' || cInput == 'e')
		{
			if (IDOK == MessageBox(NULL, "Are you sure?", //等待用戶確認退出的訊息方塊
				"Server", MB_OKCANCEL))
			{
				break;//跳出循環體
			}else{
				Sleep(TIMEFOR_THREAD_EXIT);	//執行緒睡眠
			}
		}else{
			Sleep(TIMEFOR_THREAD_EXIT);		//執行緒睡眠
		}
	}

	bServerRunning = FALSE;		//伺服器退出

	ShowServerExitMsg();		//顯示伺服器退出資訊

	Sleep(TIMEFOR_THREAD_EXIT);	//給其他執行緒時間退出

	WaitForSingleObject(hServerEvent, INFINITE);//等待清理資源執行緒發送的事件

	return;
}

/**
 *  顯示提示資訊
 */
void	ShowTipMsg(BOOL bFirstInput)
{
	if (bFirstInput)//第一次
	{
		cout << endl;
		cout << endl;
		cout << "**********************" << endl;
		cout << "*                    *" << endl;
		cout << "* s(S): Start server *" << endl;
		cout << "*                    *" << endl;
		cout << "**********************" << endl;
		cout << "Please input:" << endl;

	}else{//退出伺服器
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
 *  釋放資源
 */
void  ExitServer(void)
{
	DeleteCriticalSection(&csClientList);	//釋放臨界區對象
	CloseHandle(hServerEvent);				//釋放事件物件控制碼
	closesocket(sServer);					//關閉SOCKET
	WSACleanup();							//卸載Windows Sockets DLL
}

/**
 * 創建釋放資源執行緒和接收用戶端請求執行緒
 */
BOOL  CreateHelperAndAcceptThread(void)
{

	bServerRunning = TRUE;//設置伺服器為運行狀態

	//創建釋放資源執行緒
	unsigned long ulThreadId;
	hThreadHelp = CreateThread(NULL, 0, HelperThread, NULL, 0, &ulThreadId);
	if( NULL == hThreadHelp)
	{
		bServerRunning = FALSE;
		return FALSE;
	}else{
		CloseHandle(hThreadHelp);
	}

	//創建接收用戶端請求執行緒
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
 * 顯示啟動伺服器成功與失敗消息
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
 * 顯示退出伺服器消息
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
 * 接受用戶端連接
 */
DWORD __stdcall AcceptThread(void* pParam)
{
	SOCKET  sAccept;							//接受用戶端連接的通訊端
	sockaddr_in addrClient;						//用戶端SOCKET地址

	for (;bServerRunning;)						//伺服器的狀態
	{
		memset(&addrClient, 0, sizeof(sockaddr_in));					//初始化
		int			lenClient = sizeof(sockaddr_in);					//地址長度
		sAccept = accept(sServer, (sockaddr*)&addrClient, &lenClient);	//接受客戶請求

		if(INVALID_SOCKET == sAccept )
		{
			int nErrCode = WSAGetLastError();
			if(nErrCode == WSAEWOULDBLOCK)	//無法立即完成一個非阻擋性通訊端操作
			{
				Sleep(TIMEFOR_THREAD_SLEEP);
				continue;//繼續等待
			}else {
				return 0;//執行緒退出
			}

		}
		else//接受用戶端的請求
		{
/*[mark]HEAD
	呼叫建構子進行客戶端節點的初始化。
	將此節點加入鏈表clientlist。
	為鏈表clientlist內的每個客戶端節點取得鏈表clientlist (藉由客戶端節點的GetClientList()函式)
流程:
    1.依照收到到的 sAccept 呼叫建構子進行 CClient 節點的初始化，節點名為 pClient
    2.進入 clientlist 的臨界區
    3.將 pClient 加入 clientlist
    4.對 clientlist 內的每個 CClient 節點做 GetCLientList(clientlist) 將 CClient 節點內的 m_clientlist 指向此 clientlist
    5.離開 clientlist 的臨界區
    6.讓此 CClient 節點啟動 StartRunning()
*/
			CClient *pClient = new CClient(sAccept,addrClient, seq++, csClientList);	//[1]創建用戶端對象

			EnterCriticalSection(&csClientList);			//[2]進入在臨界區

			clientlist.push_back(pClient);					//[3]加入鏈表

			CLIENTLIST::iterator iter = clientlist.begin(); //[4]
			for (iter; iter != clientlist.end();)
			{
				CClient *pClient_tmp = (CClient*)*iter;     //[問題4]
				BOOL r = pClient_tmp->GetClientList(clientlist);
					iter++;						            //指針下移
			}

			LeaveCriticalSection(&csClientList);			//[5]離開臨界區

			pClient->StartRuning();							//[6]為接受的用戶端建立接收資料和發送資料執行緒

/*[mark]END
問題:
        1.CClient 的 m_clientlist 並不是指標而是一般變數，應該會而額外佔很多空間
        2.如果 CClient 的 m_clientlist 是指標的話，就不用每次要改變全域 clientlist 都重新呼叫所有節點 GetClientList() ，省時間
        3.GetClientList() 只會回傳 true ，所以 bool r 一定是 true
        4.因為 clientlist 是 list<CClient*> ，他的節點是指標型態，而 iter 是指向 CClient* 的指標 *iter 就是 CClient* 節點，為什麼還要轉型
*/
		}
	}

	return 0;//執行緒退出
}

/**
 * 清理資源
 */
DWORD __stdcall HelperThread(void* pParam)
{
	for (;bServerRunning;)//伺服器正在運行
	{
/*[mark]HEAD
    清理已經斷開連接之客戶的記憶體空間。
    如果有客戶斷開連線，則必須重新為鏈表clientlist內的每個客戶端節點取得鏈表clientlist (藉由客戶端節點的GetClientList()函式)
流程:
    1.宣告 Erase bool變數 ，紀錄是否有客戶斷開連線
    2.進入 clientlist 的臨界區
    3.尋找已經斷開連線的用戶端，把他從 clientlist 中移除，並釋放他的記憶體空間，再將Erase更改為true
    4.如果有客戶端斷線，把所有的客戶端都呼叫 GetClientList()
    5.離開 clientlist 的臨界區
    6.等待處理時間，避免阻塞
*/
		BOOL Erase = FALSE; //[1]

		EnterCriticalSection(&csClientList);//[2]進入臨界區

		//[3]清理已經斷開的連接用戶端記憶體空間
		CLIENTLIST::iterator iter = clientlist.begin();
		for (iter; iter != clientlist.end();)
		{
			CClient *pClient = (CClient*)*iter;
			if (pClient->IsExit())			//用戶端執行緒已經退出
			{
				clientlist.erase(iter++);	//刪除節點，[問題3]
				delete pClient;				//釋放記憶體
				pClient = NULL;             //[問題2]
				Erase = TRUE;               //[問題1]
			}else{
				iter++;						//指針下移
			}
		}

		if(Erase)   //[4] [問題1]
		{
			CLIENTLIST::iterator iter = clientlist.begin();
			for (iter; iter != clientlist.end();)
			{
				CClient *pClient_tmp = (CClient*)*iter;
				BOOL r = pClient_tmp->GetClientList(clientlist);
					iter++;					//指針下移
			}
		}

		LeaveCriticalSection(&csClientList);    //[5]離開臨界區

		Sleep(TIMEFOR_THREAD_HELP);             //[6]
/*[mark]END
問題:
    1.Erase變數應該不需要，因為可以把[流程4]的動作放到[流程3]的 if 裡面
    2.[流程3]的 pClient = NULL 應該不需要，因為後面都沒用到 pClient
    3.[流程3]的 clientlist.erase(iter++) 應該也能寫成 iter = clientlist.erase(iter)
*/
	}


	//伺服器停止工作
	if (!bServerRunning)
	{
		//斷開每個連接,執行緒退出
		EnterCriticalSection(&csClientList);
		CLIENTLIST::iterator iter = clientlist.begin();
		for (iter; iter != clientlist.end();)
		{
			CClient *pClient = (CClient*)*iter;
			//如果用戶端的連接還存在，則斷開連接，執行緒退出
			if (pClient->IsConning())
			{
				pClient->DisConning();
			}
			++iter;
		}
		//離開臨界區
		LeaveCriticalSection(&csClientList);

		//給連接用戶端執行緒時間，使其自動退出
		Sleep(TIMEFOR_THREAD_SLEEP);

		//進入臨界區
		EnterCriticalSection(&csClientList);

		//確保為每個用戶端分配的記憶體空間都回收。
		//如果不加入while這層迴圈，可能存在這樣的情況。當pClient->IsExit()時，該執行緒還沒有退出。
		//那麼就需要從鏈表的開始部分重新判斷。
		while ( 0 != clientlist.size())
		{
			iter = clientlist.begin();
			for (iter; iter != clientlist.end();)
			{
				CClient *pClient = (CClient*)*iter;
				if (pClient->IsExit())			//用戶端執行緒已經退出
				{
					clientlist.erase(iter++);	//刪除節點
					delete pClient;				//釋放記憶體空間
					pClient = NULL;
				}else{
					iter++;						//指針下移
				}
			}
			//給連接用戶端執行緒時間，使其自動退出
			Sleep(TIMEFOR_THREAD_SLEEP);
		}
		LeaveCriticalSection(&csClientList);//離開臨界區

	}

	clientlist.clear();		//清空鏈表

	SetEvent(hServerEvent);	//通知主執行緒退出

	return 0;
}
