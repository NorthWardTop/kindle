#include <config.h>
#include <debug_manager.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <pthread.h>

#define SERVER_PORT 5678
#define PRINT_BUF_SIZE   (16*1024)

static int g_iSocketServer;
static struct sockaddr_in g_tSocketServerAddr;
static struct sockaddr_in g_tSocketClientAddr;
static int g_iHaveConnected = 0;
static char *g_pcNetPrintBuf;
static int g_iReadPos  = 0;
static int g_iWritePos = 0;

static pthread_t g_tSendTreadID;
static pthread_t g_tRecvTreadID;

static pthread_mutex_t g_tNetDbgSendMutex  = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  g_tNetDbgSendConVar = PTHREAD_COND_INITIALIZER;

/* 要通过网络打印的信息存在环型缓冲区里,
 * 当有客户端连接上本程序时, 发送线程才从环型缓冲区中取出数据发送出去
 */

/**
 * 判断环形队列是否满
 * @return: 0 未满, 1 满
 */
static int isFull(void)
{
	return (((g_iWritePos + 1) % PRINT_BUF_SIZE) == g_iReadPos);
}

/**
 * 判断环形队列是否空
 * @return: 0 非空, 1 空
 */
static int isEmpty(void)
{
	return (g_iWritePos == g_iReadPos);
}


/**
 * 往环形队列放入数据, 每次一个字符
 * @cVal: 字符数据
 * @return: 0 成功, -1 失败
 */
static int PutData(char cVal)
{
	if (isFull())
		return -1;
	else {
		g_pcNetPrintBuf[g_iWritePos] = cVal;
		g_iWritePos = (g_iWritePos + 1) % PRINT_BUF_SIZE;
		return 0;
	}	
}


/**
 * 从环形队列获取一个字符数据
 * @pcVal: 字符数据指针
 * @return: 0 成功, -1 失败
 */
static int GetData(char *pcVal)
{
	if (isEmpty())
		return -1;
	else {
		*pcVal = g_pcNetPrintBuf[g_iReadPos];
		g_iReadPos = (g_iReadPos + 1) % PRINT_BUF_SIZE;
		return 0;
	}
}

/**
 * 网络调试发送线程
 * 		当调用DbgPrint时候将唤醒本线程发送数据
 * @pVoid: 线程参数(不使用)
 * @return: 线程退出
 */
static void *NetDbgSendTreadFunction(void *pVoid)
{
	char strTmpBuf[512];
	char cVal;
	int i;
	int iAddrLen;
	int iSendLen;
	
	while (1) {
		/* 平时休眠, 其他线程调用DBG_PRINTF函数时, 将会调用到g_tNetDbgOpr.DebugPrint, 它会唤醒本线程 */
		pthread_mutex_lock(&g_tNetDbgSendMutex);
		pthread_cond_wait(&g_tNetDbgSendConVar, &g_tNetDbgSendMutex);	
		pthread_mutex_unlock(&g_tNetDbgSendMutex);

		while (g_iHaveConnected && !isEmpty()) {
			i = 0;

			/* 把环形缓冲区的数据取出来, 最多取512字节 */
			while ((i < 512) && (0 == GetData(&cVal))) {
				strTmpBuf[i] = cVal;
				i++;
			}
			
			/* 执行到这里, 表示被唤醒 */
			/* 用sendto函数发送打印信息给客户端 */
			iAddrLen = sizeof(struct sockaddr);
			iSendLen = sendto(g_iSocketServer, strTmpBuf, i, 0,
			                      (const struct sockaddr *)&g_tSocketClientAddr, iAddrLen);
		}
	}
	return NULL;
}


/**
 * 网络调试接受线程, 接受调试客户端发来的信息
 * @pVoid: 线程参数(不使用)
 * @return: 线程退出
 */
static void *NetDbgRecvTreadFunction(void *pVoid)
{
	socklen_t iAddrLen;
	int iRecvLen;
	char ucRecvBuf[1000];
	struct sockaddr_in tSocketClientAddr;

	while (1) {
		/* 读取客户端发来的数据 */
		iAddrLen = sizeof(struct sockaddr);
		DBG_PRINTF("in NetDbgRecvTreadFunction\n");
		iRecvLen = recvfrom(g_iSocketServer, ucRecvBuf, 999, 0, (struct sockaddr *)&tSocketClientAddr, &iAddrLen);
		
		if (iRecvLen > 0) {
			ucRecvBuf[iRecvLen] = '\0';
			DBG_PRINTF("netprint.c get msg: %s\n", ucRecvBuf);
			
			/* 解析数据:
			 * setclient            : 设置接收打印信息的客户端
			 * dbglevel=0,1,2...    : 修改打印级别
			 * stdout=0             : 关闭stdout打印
			 * stdout=1             : 打开stdout打印
			 * netprint=0           : 关闭netprint打印
			 * netprint=1           : 打开netprint打印
			 */
			if (strcmp(ucRecvBuf, "setclient")  == 0) {
				/* 发送来"setclient"的进程用于接收调试信息 */
				g_tSocketClientAddr = tSocketClientAddr;
				g_iHaveConnected = 1;
			} else if (strncmp(ucRecvBuf, "dbglevel=", 9) == 0)
				/* 设置打印级别 */
				SetDbgLevel(ucRecvBuf);
			else
				/* 开/关打印通道 */
				SetDbgChanel(ucRecvBuf);
		}
		
	}
	return NULL;
}

/**
 * "网络输出调试通道"的初始化函数
 * 		1. 设置端口信息
 * 		2. 创建2个子线程, 一个用来接收控制命令, 比如打开/关闭某个打印通道, 设置打印级别
 * @return: 0 成功, -1 失败
 */
static int NetDbgInit(void)
{
	/* socket初始化 */
	int iRet;
	
	g_iSocketServer = socket(AF_INET, SOCK_DGRAM, 0);
	if (-1 == g_iSocketServer) {
		printf("socket error!\n");
		return -1;
	}

	g_tSocketServerAddr.sin_family      = AF_INET;
	g_tSocketServerAddr.sin_port        = htons(SERVER_PORT);  /* host to net, short */
 	g_tSocketServerAddr.sin_addr.s_addr = INADDR_ANY;
	memset(g_tSocketServerAddr.sin_zero, 0, 8);
	
	iRet = bind(g_iSocketServer, (const struct sockaddr *)&g_tSocketServerAddr, sizeof(struct sockaddr));
	if (-1 == iRet) {
		printf("bind error!\n");
		return -1;
	}
	g_pcNetPrintBuf = malloc(PRINT_BUF_SIZE);
	if (NULL == g_pcNetPrintBuf) {
		close(g_iSocketServer);
		return -1;
	}

	/* 创建netprint发送线程: 它用来发送打印信息给客户端 */
	pthread_create(&g_tSendTreadID, NULL, NetDbgSendTreadFunction, NULL);			
	
	/* 创建netprint接收线否: 用来接收控制信息,比如修改打印级别,打开/关闭打印 */
	pthread_create(&g_tRecvTreadID, NULL, NetDbgRecvTreadFunction, NULL);			

	return 0;	
}


/**
 * 网络调试退出清理
 * @return: 0
 */
static int NetDbgExit(void)
{
	/* 关闭socket,... */
	close(g_iSocketServer);
	free(g_pcNetPrintBuf);
	return 0;
}

/**
 * 网络调试打印, 将打印信息放入环形缓冲区然后唤醒发送线程
 * @sreData: 要打印的字符串信息
 * @return: 放入的字节数
 */
static int NetDbgPrint(char *strData)
{
	/* 把数据放入环形缓冲区 */
	int i;

	for (i = 0; i < strlen(strData); i++) {
		if (0 != PutData(strData[i]))
			break;
	}
	
	/* 如果已经有客户端连接了, 就把数据通过网络发送给客户端 */
	/* 唤醒netprint的发送线程 */
	pthread_mutex_lock(&g_tNetDbgSendMutex);
	pthread_cond_signal(&g_tNetDbgSendConVar);
	pthread_mutex_unlock(&g_tNetDbgSendMutex);

	return i;
	
}

/**
 * 网络调试选项对象
 */
static T_DebugOpr g_tNetDbgOpr = {
	.name       = "netprint",
	.isCanUse   = 1,
	.DebugInit  = NetDbgInit,
	.DebugExit  = NetDbgExit,
	.DebugPrint = NetDbgPrint,
};


/**
 * 网络调试选项注册, 留给上层添加网络调试选项使用
 * @return: 0 成功, -1 失败
 */
int NetPrintInit(void)
{
	return RegisterDebugOpr(&g_tNetDbgOpr);
}

