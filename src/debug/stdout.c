
#include <config.h>
#include <debug_manager.h>
#include <stdio.h>
#include <string.h>


/**
 * "标准输出调试通道"的输出函数
 * @strData: 要打印的字符串调试信息
 * @return: 打印的长度
 */
static int StdoutDebugPrint(char *strData)
{
	/* 直接把输出信息用printf打印出来 */
	printf("%s", strData);
	return strlen(strData);	
}

static T_DebugOpr g_tStdoutDbgOpr = {
	.name       = "stdout",
	.isCanUse   = 1,                 /* 1表示将使用它来输出调试信息 */
	.DebugPrint = StdoutDebugPrint,  /* 打印函数 */
};

/**
 * 注册标准输出调试方法, 供给上层注册调试选项使用
 * @return: 0 成功, -1 失败
 */
int StdoutInit(void)
{
	return RegisterDebugOpr(&g_tStdoutDbgOpr);
}

