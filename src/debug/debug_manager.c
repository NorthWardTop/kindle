#include <config.h>
#include <debug_manager.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

//调试选项头指针
static PT_DebugOpr g_ptDebugOprHead;
//调试级别高于或等于g_iDbgLevelLimit的调试信息才会打印出来
static int g_iDbgLevelLimit = 8;


/**
 * 注册调试对象: 讲调试选项加入到调试选项列表
 * @ptDebugOpr: 调试对象指针
 * @return: 0
 */
int RegisterDebugOpr(PT_DebugOpr ptDebugOpr)
{
	PT_DebugOpr ptTmp;

	if (!g_ptDebugOprHead) {
		g_ptDebugOprHead   = ptDebugOpr;
		ptDebugOpr->ptNext = NULL;
	} else {
		ptTmp = g_ptDebugOprHead;
		while (ptTmp->ptNext)
			ptTmp = ptTmp->ptNext;

		ptTmp->ptNext	  = ptDebugOpr;
		ptDebugOpr->ptNext = NULL;
	}

	return 0;
}

/**
 * 显示所有调试选项: 打印调试选项链表
 */
void ShowDebugOpr(void)
{
	int i = 0;
	PT_DebugOpr ptTmp = g_ptDebugOprHead;

	while (ptTmp) {
		DBG_PRINTF("%02d %s\n", i++, ptTmp->name);
		ptTmp = ptTmp->ptNext;
	}
}

/**
 * 获取调试选项: 根据名字取出指定的"调试模块"地址返回
 * @return: 获取成功返回非NULL, 获取失败返回NULL
 */
PT_DebugOpr GetDebugOpr(char *pcName)
{
	PT_DebugOpr ptTmp = g_ptDebugOprHead;
	
	while (ptTmp) {
		if (strcmp(ptTmp->name, pcName) == 0)
			return ptTmp;
		ptTmp = ptTmp->ptNext;
	}
	return NULL;
}


/**
 * 设置打印限制级: 级别>=g_iDbgLevelLimit会输出
 * @strBuf: 类似与"dbglevel=3"
 * @return: 0
 */
int SetDbgLevel(char *strBuf)
{
	g_iDbgLevelLimit = strBuf[9] - '0';
	return 0;
}

/**
 * 打开或关闭调试选项
 * @strBuf: 类似于以下字符串
 *                     stdout=0			   : 关闭stdout打印
 *                     stdout=1			   : 打开stdout打印
 *                     netprint=0		   : 关闭netprint打印
 *                     netprint=1		   : 打开netprint打印
 * @return: 成功返回0, 失败返回-1
 */
int SetDbgChanel(char *strBuf)
{
	char *pStrTmp;
	char strName[100];
	PT_DebugOpr ptTmp;

	pStrTmp = strchr(strBuf, '=');
	if (!pStrTmp)
	{
		return -1;
	}
	else
	{
		/* 先截取名字, 然后根据名字找到调试模块 */
		strncpy(strName, strBuf, pStrTmp-strBuf);
		strName[pStrTmp-strBuf] = '\0';
		ptTmp = GetDebugOpr(strName);
		if (!ptTmp)
			return -1;

		if (pStrTmp[1] == '0')
			ptTmp->isCanUse = 0;
		else
			ptTmp->isCanUse = 1;
		return 0;
	}
	
}


/**
 * 向上提供的打印接口函数: 程序里用DBG_PRINTF来打印, 它就是DebugPrint
 *            在config.h里有这样的宏定义: #define DBG_PRINTF DebugPrint
 * @pcFormat: 同printf, 格式化字符串
 * @...: 格式化字符串参数
 * @return: 0 - 成功
 * 			-1 - 失败
 */
int DebugPrint(const char *pcFormat, ...)
{
	char strTmpBuf[1024];
	char *pcTmp;
	va_list tArg;
	int iNum;
	PT_DebugOpr ptTmp = g_ptDebugOprHead;
	int dbglevel = DEFAULT_DBGLEVEL;

	/* 可变参数的处理, 抄自glibc的printf函数 */
	va_start (tArg, pcFormat);
	iNum = vsprintf (strTmpBuf, pcFormat, tArg);
	va_end (tArg);
	strTmpBuf[iNum] = '\0'; //格式化转换后的字符串


	pcTmp = strTmpBuf;
	
	/* 根据打印级别决定是否打印 */
    /*SetDbgLevel =6 */
	if ((strTmpBuf[0] == '<') && (strTmpBuf[2] == '>')) {
		dbglevel = strTmpBuf[1] - '0';
		if (dbglevel >= 0 && dbglevel <= 9)
			pcTmp = strTmpBuf + 3; //在打印级别内就去掉 "<3>"
		else
			dbglevel = DEFAULT_DBGLEVEL; //不在打印级别内就默认值
	}
	//低于打印级别直接返回, 不用打印
	if (dbglevel > g_iDbgLevelLimit)
		return -1;

	/* 调用链表中所有isCanUse为1的结构体的DebugPrint函数 
	 * 用来输出调试信息
	 */
	while (ptTmp) {
		if (ptTmp->isCanUse)
			ptTmp->DebugPrint(pcTmp);
		ptTmp = ptTmp->ptNext;
	}

	return 0;
}


/**
 * 初始化所有需要初始化的打印选项通道, 比如网络打印的绑定端口
 * @return: 0
 */
int InitDebugChanel(void)
{
	PT_DebugOpr ptTmp = g_ptDebugOprHead;
	while (ptTmp) {
		//开关打开并且需要初始化的打印通道
		if (ptTmp->isCanUse && ptTmp->DebugInit)
			ptTmp->DebugInit();
		ptTmp = ptTmp->ptNext;
	}

	return 0;
}


/**
 * 注册所有打印选项
 * @return: 0
 */
int DebugInit(void)
{
	int iError;
	//加入两个打印选项
	iError = StdoutInit();
	iError |= NetPrintInit();
	return iError;
}

