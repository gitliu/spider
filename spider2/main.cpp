//
//	@���ܣ�rtsp��ת��(����)
//	@���ߣ�С��
//	@�汾��1.0
//	@���ڣ�2013.10.21
//	@��Ȩ���У�����ؾ���
//


// system include
#include <Windows.h>
#include <process.h>
#include <stdio.h>
#include <stdlib.h>

// service include
#include "../spider2/nt_service.h"

// PorxyService include
#include "ProxyService.h"

// NTService object
static NTService ss_obj_service;

static bool default_service_handling(char **argv);
int spider_service(void *p);

int main(int argc, char** argv) 
{
	if (2 == argc)
	{
		if (!default_service_handling(argv))
		{
			return 0;
		}
		if (ss_obj_service.IsService(argv[1]))
		{
			ss_obj_service.Init(spider_service);
			return 0;
		}
	}
	else if (1 == argc && ss_obj_service.IsService("spider2"))
	{
		ss_obj_service.Init(spider_service);
		return 0;
	}
	//	ss_obj_service.Init(spider_service);
	spider_service(NULL);
	return 0;
}


/**
	main�������߳���ں���
*/
int spider_service(void *p)
{
	HANDLE hThread;
	unsigned threadID;

	// Create the proxy thread
	hThread = (HANDLE)_beginthreadex(NULL, 0, &proxy, NULL, 0, &threadID);

	//set service status SERVICE_RUNNING
	ss_obj_service.SetRunning();
	
	// Wait until proxy thread terminates.
	WaitForSingleObject(hThread, INFINITE);
	
	// Destroy the thread object.
	CloseHandle(hThread);

	return 0;
}

/**
	��������Ĵ�������簲װ��ж��

	@param argv				ָ������б�
	@param servicename		�����ڲ�����
	@param displayname		������ʾ����
	@param file_path		�����·��
	
	@retval	
		0	����ѡ��
	@retval
		1	���ܴ���ѡ��
*/
static bool default_service_handling(char **argv)
{
	if (ss_obj_service.got_service_option(argv, "install"))
	{
		ss_obj_service.Install();
		return FALSE;
	}
	if (ss_obj_service.got_service_option(argv, "remove"))
	{
		ss_obj_service.Remove();
		return FALSE;
	}
	return TRUE;
}
