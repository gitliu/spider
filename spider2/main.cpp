//
//	@功能：rtsp流转发(代理)
//	@作者：小刘
//	@版本：1.0
//	@日期：2013.10.21
//	@版权所有，翻版必究。
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
	main函数和线程入口函数
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
	处理基本的处理服务，如安装和卸载

	@param argv				指针参数列表
	@param servicename		服务内部名字
	@param displayname		服务显示名字
	@param file_path		程序的路径
	
	@retval	
		0	处理选项
	@retval
		1	不能处理选项
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
