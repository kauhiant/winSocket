// multi_conf_nonblocking_Client.cpp   客戶端程式

/* 配合三個伺服器程式:
   multi_conf_nonblocking_Server.cpp
   multi_conf_nonblocking_Server(Client_Function).cpp
   multi_conf_nonblocking_Server(Client_Class).h
*/

#include <string.h>
#include <iostream>
#include <winsock2.h>//wincosk2.h內含windows.h的引入,而windows.h內又含winbase.h的引入

#pragma comment(lib, "WS2_32.lib")
using namespace std;

#define CLIENT_SETUP_FAIL			1		//啟動用戶端失敗
#define CLIENT_CREATETHREAD_FAIL	2		//創建執行緒失敗
#define CLIENT_CONNECT_FAIL			3		//網路連接斷開
#define TIMEFOR_THREAD_EXIT			1000	//子執行緒退出時間
#define TIMEFOR_THREAD_SLEEP		500		//執行緒睡眠時間

#define	SERVERIP			"127.0.0.1"		//伺服器IP
#define	SERVERPORT			5556			//伺服器TCP埠
#define	MAX_NUM_BUF			48				//緩衝區的最大長度
#define ADD					'+'				//+
#define SUB					'-'				//- 
#define MUT					'*'				//*
#define DIV					'/'				///
#define EQU					'='				//=

//資料包類型
#define EXPRESSION			'E'				//算數運算式
#define BYEBYE				'B'				//消息byebye
#define CONVERSATION		'C'				//對話
#define HEADERLEN			(sizeof(hdr))	//頭長度

//資料包頭結構該結構在win32xp下為4byte
typedef struct _head
{
	char			type;//類型		
	unsigned short	len;//資料包的長度(包括頭的長度)
}hdr, *phdr;

//資料包中的資料結構
typedef struct _data 
{
	char	buf[MAX_NUM_BUF];
}DATABUF, *pDataBuf;

SOCKET	sClient;							//客戶端Socket
HANDLE	hThreadSend;						//傳送資料執行緒控制碼
HANDLE	hThreadRecv;						//接收資料執行緒控制碼
DATABUF bufSend;							//傳送資料緩衝區
DATABUF bufRecv;							//接收資料緩衝區
CRITICAL_SECTION csSend;					//臨界區物件，確保主執行緒和傳送資料執行緒對bufSend變數的互斥存取
CRITICAL_SECTION csRecv;					//臨界區物件，確保主執行緒和傳送資料執行緒對bufRecv變數的互斥存取
BOOL	bSendData;							//通知傳送資料執行緒傳送資料的布林變數
HANDLE	hEventShowDataResult;				//顯示計算結果的事件
BOOL	bConnecting;						//與伺服器的連接狀態
HANDLE	arrThread[2];						//子執行緒陣列


BOOL	InitClient(void);					//初始化
BOOL	ConnectServer(void);				//連接伺服器
BOOL	CreateSendAndRecvThread(void);		//創建發送和接收資料執行緒
void	InputAndOutput(void);				//使用者輸入資料
void	ExitClient(void);					//退出

void	InitMember(void);					//初始化全域變數
BOOL    InitSockt(void);					//創建SOCKET

DWORD __stdcall	RecvDataThread(void* pParam);		//接收資料執行緒
DWORD __stdcall	SendDataThread(void* pParam);		//發送資料執行緒

BOOL	PackByebye(const char* pExpr);		//將輸入的"Byebye" "byebye"的字串打包
BOOL	PackExpression(const char *pExpr);	//將輸入的算數運算式打包
BOOL	Conversation(const char* pExpr);

void	ShowConnectMsg(BOOL bSuc);			//顯示連接伺服器消息
void	ShowDataResultMsg(void);			//顯示連計算結果
void	ShowTipMsg(BOOL bFirstInput);		//顯示提示資訊


int main(int argc, char* argv[])
{	
	//初始化
	if (!InitClient())
	{	
		ExitClient();
		return CLIENT_SETUP_FAIL;
	}	
	
	//連接伺服器
	if (ConnectServer())
	{
		ShowConnectMsg(TRUE);	
	}else{
		ShowConnectMsg(FALSE);		
		ExitClient();
		return CLIENT_SETUP_FAIL;		
	}
	
	//創建發送和接收資料執行緒
	if (!CreateSendAndRecvThread())
	{
		ExitClient();
		return CLIENT_CREATETHREAD_FAIL;
	}
	
	//使用者輸入資料和顯示結果
	InputAndOutput();
	
	//退出
	ExitClient();
	
	return 0;
}


/**
 *	初始化
 */
BOOL	InitClient(void)
{
	//初始化全域變數
	InitMember();

	//創建SOCKET
	if (!InitSockt())
	{
		return FALSE;
	}

	return TRUE;	
}


/**
 * 初始化全域變數
 */
void	InitMember(void)
{
	//初始化臨界區
	InitializeCriticalSection(&csSend);
	InitializeCriticalSection(&csRecv);

	sClient = INVALID_SOCKET;	//通訊端
	hThreadRecv = NULL;			//接收資料執行緒控制碼
	hThreadSend = NULL;			//發送資料執行緒控制碼
	bConnecting = FALSE;		//為連接狀態
	bSendData = FALSE;			//不發送資料狀態	

	//初始化數據緩衝區
	memset(bufSend.buf, 0, MAX_NUM_BUF);
	memset(bufRecv.buf, 0, MAX_NUM_BUF);
	memset(arrThread, 0, 2);
	
	//手動設置事件，初始化為無信號狀態
	hEventShowDataResult = (HANDLE)CreateEvent(NULL, TRUE, FALSE, NULL);
}	
	

/**
 * 創建非阻塞通訊端
 */
BOOL    InitSockt(void)
{
	int			reVal;	//返回值
	WSADATA		wsData;	//WSADATA變數	
	reVal = WSAStartup(MAKEWORD(2,2),&wsData);//初始化Windows Sockets Dll
	
	//創建通訊端		
	sClient = socket(AF_INET, SOCK_STREAM, 0);
	if(INVALID_SOCKET == sClient)
		return FALSE;

	
	//設置通訊端非阻塞模式
	unsigned long ul = 1;
	reVal = ioctlsocket(sClient, FIONBIO, (unsigned long*)&ul);
	if (reVal == SOCKET_ERROR)
		return FALSE;

	return TRUE;
}	
	

/**
 * 連接伺服器
 */
BOOL	ConnectServer(void)
{
	int reVal;			//返回值
	sockaddr_in serAddr;//伺服器地址
	
	serAddr.sin_family = AF_INET;
	serAddr.sin_port = htons(SERVERPORT);
	serAddr.sin_addr.S_un.S_addr = inet_addr(SERVERIP);

	cout << "連線中...\n";
	for (;;)
	{
		//連接伺服器
		reVal = connect(sClient, (struct sockaddr*)&serAddr, sizeof(serAddr));
		
		//處理連接錯誤
		if(SOCKET_ERROR == reVal)
		{
			int nErrCode = WSAGetLastError();
			if( WSAEWOULDBLOCK == nErrCode ||//連接還沒有完成
					 WSAEINVAL == nErrCode || 
				   WSAEALREADY == nErrCode )
			{
				continue;
			}else if (WSAEISCONN == nErrCode)//連接已經完成
			{
				break;
			}else //其它原因，連接失敗 
			{
				return FALSE;
			}
		}	
		
		if ( reVal == 0 )//連接成功
			break;		
	}

	bConnecting = TRUE;
	return TRUE;
}

/**
 * 顯示連接伺服器失敗資訊
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
 * 創建發送和接收資料執行緒
 */
BOOL	CreateSendAndRecvThread(void)
{
	//創建接收資料的執行緒
	unsigned long ulThreadId;
	hThreadRecv = CreateThread(NULL, 0, RecvDataThread, NULL, 0, &ulThreadId);
	if (NULL == hThreadRecv)
		return FALSE;
	
	//創建發送資料的執行緒
	hThreadSend = CreateThread(NULL, 0, SendDataThread, NULL, 0, &ulThreadId);
	if (NULL == hThreadSend)
		return FALSE;

	//添加到執行緒陣列
	arrThread[0] = hThreadRecv;
	arrThread[1] = hThreadSend;	
	return TRUE;
}

/**
 * 輸入資料和顯示結果
 */
void	InputAndOutput(void)
{
	char cInput[MAX_NUM_BUF];	//用戶輸入緩衝區	
	BOOL bFirstInput = TRUE;	//第一次只能輸入算數運算式
	
	ShowTipMsg(bFirstInput);		//提示輸入資訊
	for (;bConnecting;)			//連接狀態
	{
		memset(cInput, 0, MAX_NUM_BUF);		
		cin.getline(cInput,MAX_NUM_BUF);
		char *pTemp = cInput;
		if (bFirstInput)				//第一次輸入
		{
			if (!PackExpression(pTemp))	//算數運算式打包
			{
				if(!Conversation(pTemp))
				continue;				//重新輸入
			}
			bFirstInput = FALSE;		//成功輸入第一個算數運算式
			
		}else if (!PackByebye(pTemp))	//“Byebye”“byebye”打包
		{			
			if (!PackExpression(pTemp))	//算數運算式打包
			{
				if(!Conversation(pTemp))
				continue;				//重新輸入
			}
		}
		
	}

	if (!bConnecting)			//與伺服器連接已經斷開
	{
		ShowConnectMsg(FALSE);	//顯示資訊
		Sleep(TIMEFOR_THREAD_EXIT);
	}

	//等待資料發送和接收執行緒退出
	DWORD reVal = WaitForMultipleObjects(2, arrThread, TRUE, INFINITE);
	if (WAIT_ABANDONED_0 == reVal)
	{
		int nErrCode = GetLastError();
	}
}


/**
 * 打包計算運算式的資料
 */
BOOL	PackExpression(const char *pExpr)
{
	
	char* pTemp = (char*)pExpr;	//算數運算式數字開始的位置

	while (*pTemp == ' ')//while (!*pTemp)				//第一個數字位置
		pTemp++;		
	
	char* pos1 = pTemp;	//第一個數字位置
	char* pos2 = NULL;	//運算子位置
	char* pos3 = NULL;	//第二個數字位置
	int len1 = 0;		//第一個數字長度
	int len2 = 0;		//運算子長度
	int len3 = 0;		//第二個數字長度

	//第一個字元必須是+ - 或者是數字
	if ((*pTemp != '+') && 
		(*pTemp != '-') &&
		((*pTemp < '0') || (*pTemp > '9')))
	{
		return FALSE;
	}

	
	if ((*pTemp++ == '+')&&(*pTemp < '0' || *pTemp > '9'))	//第一個字元是'+'時，第二個必須是數字	
		return FALSE;										//重新輸入
	--pTemp;												//上移指針
	
	
	if ((*pTemp++ == '-')&&(*pTemp < '0' || *pTemp > '9'))	//第一個字元是'-'時,第二個必須是數字	
		return FALSE;										//重新輸入
	--pTemp;												//上移指針
	
	char* pNum = pTemp;						//數字開始的位置					
	if (*pTemp == '+'||*pTemp == '-')		//+ -
		pTemp++;
	
	while (*pTemp >= '0' && *pTemp <= '9')	//數字
		pTemp++;							
	
	len1 = pTemp - pNum;//數字長度						
	
	//可能有空格
	while(*pTemp == ' ')//while(!*pTemp)							
		pTemp++;
	
	//算數運算子
	if ((ADD != *pTemp)&&			
		(SUB != *pTemp)&&
		(MUT != *pTemp)&&
		(DIV != *pTemp))
		return FALSE;
	
	pos2 = pTemp;
	len2 = 1;
	
	//下移指針
	pTemp++;
	//可能有空格
	while(*pTemp == ' ')//while(!*pTemp)
		pTemp++;
	
	//第2個數字位置
	pos3 = pTemp;
	if (*pTemp < '0' || *pTemp > '9')
		return FALSE;//重新輸入			
	
	while (*pTemp >= '0' && *pTemp <= '9')//數字
		pTemp++;
	
	while(*pTemp == ' ')//while(!*pTemp)
		pTemp++;

	if (EQU != *pTemp)	//最後是等於號
		return FALSE;	//重新輸入
	
	len3 = pTemp - pos3;//數字長度

	int nExprlen = len1 + len2 + len3;	//算數表示長度

	//運算式讀入發送資料緩衝區
	EnterCriticalSection(&csSend);		//進入臨界區
	//數據包頭
	phdr pHeader = (phdr)(bufSend.buf);
	pHeader->type = EXPRESSION;			//類型
	pHeader->len = nExprlen + HEADERLEN;//數據包長度
	//拷貝資料
	memcpy(bufSend.buf + HEADERLEN, pos1, len1);
	memcpy(bufSend.buf + HEADERLEN + len1, pos2, len2);
	memcpy(bufSend.buf + HEADERLEN + len1 + len2 , pos3,len3);
	LeaveCriticalSection(&csSend);		//離開臨界區
	pHeader = NULL;

	bSendData = TRUE;					//通知發送資料執行緒發送資料

	return TRUE;
}


/**
 * 打包發送byebye資料
 */
BOOL	PackByebye(const char* pExpr)
{
	BOOL reVal = FALSE;
	
	if(!strcmp("Byebye", pExpr)||!strcmp("byebye", pExpr))		//如果是"Byebye" "byebye"
	{
		EnterCriticalSection(&csSend);							//進入臨界區
		phdr pHeader = (phdr)bufSend.buf;						//強制轉換
		pHeader->type = BYEBYE;									//類型
		pHeader->len = HEADERLEN + strlen("Byebye");			//數據包長度
		memcpy(bufSend.buf + HEADERLEN, pExpr, strlen(pExpr));	//複製資料
		LeaveCriticalSection(&csSend);							//離開臨界區		
		
		pHeader = NULL;											//null
		bSendData = TRUE;										//通知發送資料執行緒
		reVal = TRUE;		
	}
	
	return reVal;
}


/**
 * 打包發送 會話 資料
 */
BOOL	Conversation(const char* pExpr)
{
	BOOL reVal = FALSE;

	if(strlen(pExpr)>0)
	{
		EnterCriticalSection(&csSend);							//進入臨界區
		phdr pHeader = (phdr)bufSend.buf;						//強制轉換
		pHeader->type = CONVERSATION;									//類型
		pHeader->len = HEADERLEN + strlen(pExpr);			//數據包長度
		memcpy(bufSend.buf + HEADERLEN, pExpr, strlen(pExpr));	//複製資料
		LeaveCriticalSection(&csSend);							//離開臨界區		
		
		pHeader = NULL;											//null
		bSendData = TRUE;										//通知發送資料執行緒
		reVal = TRUE;
	}
		return reVal;
}



/**
 * 運算式結果
 */
void	ShowDataResultMsg(void)
{
	EnterCriticalSection(&csRecv);
	cout <<  "\t" << bufRecv.buf <<endl;
	LeaveCriticalSection(&csRecv);
		
}

/**
 * 提示資訊
 */
void	ShowTipMsg(BOOL bFirstInput)
{
	if (bFirstInput)//首次顯示
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
 * 用戶端退出
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
 * 發送資料執行緒
 */
DWORD __stdcall	SendDataThread(void* pParam)			
{	
	while(bConnecting)						//連接狀態
	{
		if (bSendData)						//發送資料
		{
			EnterCriticalSection(&csSend);	//進入臨界區
			for (;;)
			{
				int nBuflen = ((phdr)(bufSend.buf))->len;		
				int val = send(sClient, bufSend.buf, nBuflen,0);
				
				//處理返回錯誤
				if (SOCKET_ERROR == val)
				{
					int nErrCode = WSAGetLastError();
					if(WSAEWOULDBLOCK == nErrCode)		//發送緩衝區不可用
					{
						continue;						//繼續迴圈
					}else {
						LeaveCriticalSection(&csSend);	//離開臨界區
						bConnecting = FALSE;			//斷開狀態
						SetEvent(hEventShowDataResult);	//通知主執行緒，防止在無限期的等待返回結果。
						return 0;
					}				
				}				
			
				bSendData = FALSE;			//發送狀態
				break;						//跳出for
			}			
			LeaveCriticalSection(&csSend);	//離開臨界區
		}
			
		Sleep(TIMEFOR_THREAD_SLEEP);		//執行緒睡眠
	}
	
	return 0;
}


/**
 * 接收資料執行緒
 */
DWORD __stdcall	RecvDataThread(void* pParam)				
{
	int		reVal;				//返回值
	char	temp[MAX_NUM_BUF];	//區域變數
	memset(temp, 0, MAX_NUM_BUF);

	while(bConnecting)			//連接狀態		
	{	
		reVal = recv(sClient, temp, MAX_NUM_BUF, 0);//接收資料
		
		if (SOCKET_ERROR == reVal)
		{
			int nErrCode = WSAGetLastError();
			if (WSAEWOULDBLOCK == nErrCode)			//接受資料緩衝區不可用
			{
				Sleep(TIMEFOR_THREAD_SLEEP);		//執行緒睡眠
				continue;							//繼續接收資料
			}else{
				bConnecting = FALSE;
				SetEvent(hEventShowDataResult);		//通知主執行緒，防止在無限期的等待
				return 0;							//執行緒退出				
			}
		}
		
		if ( reVal == 0)							//伺服器關閉了連接
		{
			bConnecting = FALSE;					//執行緒退出
			SetEvent(hEventShowDataResult);			//通知主執行緒，防止在無限期的等待
			return 0;								//執行緒退出 
		}
		
		if (reVal > HEADERLEN && -1 != reVal)		//收到資料
		{
			//對數據解包
 			phdr header = (phdr)(temp);
			if (BYEBYE != ((phdr)header)->type)	    
			{
					EnterCriticalSection(&csRecv);		
					memset(bufRecv.buf, 0, MAX_NUM_BUF);	
					memcpy(bufRecv.buf, temp + HEADERLEN, header->len - HEADERLEN);	//將資料結果複製到接收資料緩衝區
					LeaveCriticalSection(&csRecv);
					ShowDataResultMsg();				//顯示資料
			}
			else //如果是“byebye”或者“Byebye”
			{
					EnterCriticalSection(&csRecv);		
					memset(bufRecv.buf, 0, MAX_NUM_BUF);	
					memcpy(bufRecv.buf, temp + HEADERLEN, header->len - HEADERLEN);	//將資料結果複製到接收資料緩衝區
					LeaveCriticalSection(&csRecv);
					ShowDataResultMsg();				//顯示資料

					if (0 == strcmp(bufRecv.buf, "OK"))	//用戶端主動退出
					{
						bConnecting = FALSE;
						Sleep(TIMEFOR_THREAD_EXIT);		//給資料接收和發送執行緒退出時間
						exit(0);
					}			
			}

			memset(temp, 0, MAX_NUM_BUF);
		}
		
		Sleep(TIMEFOR_THREAD_SLEEP);//執行緒睡眠
	}
	return 0;
}

