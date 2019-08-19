
#include <config.h>
#include <input_manager.h>
#include <string.h>

static PT_InputOpr g_ptInputOprHead;
static T_InputEvent g_tInputEvent;

static pthread_mutex_t g_tMutex  = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  g_tConVar = PTHREAD_COND_INITIALIZER;

/**********************************************************************
 * �������ƣ� RegisterInputOpr
 * ���������� ע��"����ģ��"
 * ��������� ptInputOpr - ����ģ��Ľṹ��ָ��
 * ��������� ��
 * �� �� ֵ�� 0 - �ɹ�, ����ֵ - ʧ��
 * �޸�����        �汾��     �޸���	      �޸�����
 * -----------------------------------------------
 * 2016/01/09	     V2.0	  ����	      �޸�
 ***********************************************************************/
int RegisterInputOpr(PT_InputOpr ptInputOpr)
{
	PT_InputOpr ptTmp;

	if (!g_ptInputOprHead)
	{
		g_ptInputOprHead   = ptInputOpr;
		ptInputOpr->ptNext = NULL;
	}
	else
	{
		ptTmp = g_ptInputOprHead;
		while (ptTmp->ptNext)
		{
			ptTmp = ptTmp->ptNext;
		}
		ptTmp->ptNext	  = ptInputOpr;
		ptInputOpr->ptNext = NULL;
	}

	return 0;
}


/**********************************************************************
 * �������ƣ� ShowInputOpr
 * ���������� ��ʾ��������֧�ֵ�"����ģ��"
 * ��������� ��
 * ��������� ��
 * �� �� ֵ�� ��
 * �޸�����        �汾��     �޸���	      �޸�����
 * -----------------------------------------------
 * 2016/01/09	     V2.0	  ����	      �޸�
 ***********************************************************************/
void ShowInputOpr(void)
{
	int i = 0;
	PT_InputOpr ptTmp = g_ptInputOprHead;

	while (ptTmp)
	{
		printf("%02d %s\n", i++, ptTmp->name);
		ptTmp = ptTmp->ptNext;
	}
}

/**********************************************************************
 * �������ƣ� InputEventThreadFunction
 * ���������� "����ģ��"���̺߳���,ÿ������ģ�鶼��ͨ���������߳�����ȡ��������,
 *            �������ݺ����ỽ�ѵȵ����ݵ������߳�
 * ��������� pVoid - ����ģ���"���������ݺ���"
 * ��������� ��
 * �� �� ֵ�� NULL - �����˳�
 * �޸�����        �汾��     �޸���	      �޸�����
 * -----------------------------------------------
 * 2016/01/09	     V2.0	  ����	      �޸�
 ***********************************************************************/
static void *InputEventThreadFunction(void *pVoid)
{
	T_InputEvent tInputEvent;
	
	/* ���庯��ָ�� */
	int (*GetInputEvent)(PT_InputEvent ptInputEvent);
	GetInputEvent = (int (*)(PT_InputEvent))pVoid;

	while (1)
	{
		if(0 == GetInputEvent(&tInputEvent))
		{
			/* �������߳�, ��tInputEvent��ֵ����һ��ȫ�ֱ��� */
			/* �����ٽ���Դǰ���Ȼ�û����� */
			pthread_mutex_lock(&g_tMutex);
			g_tInputEvent = tInputEvent;

			/*  �������߳� */
			pthread_cond_signal(&g_tConVar);

			/* �ͷŻ����� */
			pthread_mutex_unlock(&g_tMutex);
		}
	}

	return NULL;
}

/**********************************************************************
 * �������ƣ� AllInputDevicesInit
 * ���������� ��������"����ģ��"���豸��صĳ�ʼ������
 *            ���������ڶ�ȡ�������ݵ����߳�
 * ��������� ��
 * ��������� ��
 * �� �� ֵ�� 0 - �ɹ�, ����ֵ - ʧ��
 * �޸�����        �汾��     �޸���	      �޸�����
 * -----------------------------------------------
 * 2016/01/09	     V2.0	  ����	      �޸�
 ***********************************************************************/
int AllInputDevicesInit(void)
{
	PT_InputOpr ptTmp = g_ptInputOprHead;
	int iError = -1;

	while (ptTmp)
	{
		if (0 == ptTmp->DeviceInit())
		{
			/* �������߳� */
			pthread_create(&ptTmp->tTreadID, NULL, InputEventThreadFunction, ptTmp->GetInputEvent);			
			iError = 0;
		}
		ptTmp = ptTmp->ptNext;
	}
	return iError;
}

/**********************************************************************
 * �������ƣ� GetInputEvent
 * ���������� �����������,����ʹ�õ�ǰ�߳�����,
 *            ��������ģ������̶߳������ݺ���������
 * ��������� ��
 * ��������� ptInputEvent - �ں��õ�����������
 * �� �� ֵ�� 0 - �ɹ�
 * �޸�����        �汾��     �޸���	      �޸�����
 * -----------------------------------------------
 * 2016/01/09	     V2.0	  ����	      �޸�
 ***********************************************************************/
int GetInputEvent(PT_InputEvent ptInputEvent)
{
	/* ���� */
	pthread_mutex_lock(&g_tMutex);
	pthread_cond_wait(&g_tConVar, &g_tMutex);	

	/* �����Ѻ�,�������� */
	*ptInputEvent = g_tInputEvent;
	pthread_mutex_unlock(&g_tMutex);
	return 0;	
}

/**********************************************************************
 * �������ƣ� InputInit
 * ���������� ���ø�������ģ��ĳ�ʼ������,����ע���������ģ��
 * ��������� ��
 * ��������� ��
 * �� �� ֵ�� 0 - �ɹ�, ����ֵ - ʧ��
 * �޸�����        �汾��     �޸���	      �޸�����
 * -----------------------------------------------
 * 2016/01/09	     V2.0	  ����	      �޸�
 ***********************************************************************/
int InputInit(void)
{
	int iError = 0;
	// iError = StdinInit();
	iError |= TouchScreenInit();
	return iError;
}

