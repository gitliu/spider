/**
  @file

  @brief
  Windows NT Service class library.

*/
#include <windows.h>
#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include "nt_service.h"


static NTService *pService;

/* ------------------------------------------------------------------------

 -------------------------------------------------------------------------- */
NTService::NTService()
{

    bOsNT	     = FALSE;
	szInternName = "spider2";
	szDisplayName = "spider2";
	szAccountName = NULL;
	szPassword = NULL;

    // 服务变量
    ServiceName      = NULL;
    hExitEvent	     = 0;
    bPause	     = FALSE;
    bRunning	     = FALSE;
    hThreadHandle    = 0;
    fpServiceThread  = NULL;

    // 超时变量
    nStartTimeOut    = 15000;
    nStopTimeOut     = 86400000;
    nPauseTimeOut    = 5000;
    nResumeTimeOut   = 5000;

    // 安装变量
    dwDesiredAccess  = SERVICE_ALL_ACCESS;
    dwServiceType    = SERVICE_WIN32_OWN_PROCESS;
    dwStartType      = SERVICE_AUTO_START;
    dwErrorControl   = SERVICE_ERROR_NORMAL;
    szLoadOrderGroup = NULL;
    lpdwTagID	     = NULL;
    szDependencies   = NULL;

    my_argc	     = 0;
    my_argv	     = NULL;
    hShutdownEvent   = 0;
    nError	     = 0;
    dwState	     = 0;
}

/* ------------------------------------------------------------------------

 -------------------------------------------------------------------------- */
NTService::~NTService()
{
	if (ServiceName != NULL) 
		delete[] ServiceName;
}
/* ------------------------------------------------------------------------

 -------------------------------------------------------------------------- */

BOOL NTService::GetOS()
{
	bOsNT = FALSE;
	memset(&osVer, 0, sizeof(OSVERSIONINFO));
	osVer.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	if (GetVersionEx(&osVer))
	{
		if (osVer.dwPlatformId == VER_PLATFORM_WIN32_NT)
			bOsNT = TRUE;
	}
	return bOsNT;
}


/**
  注册服务的主要服务线程管理器。

  @param ServiceThread  主程序入口函数指针当服务已启动
*/


long NTService::Init(void *ServiceThread)
{

	pService = this;

	fpServiceThread = (THREAD_FC)ServiceThread;
	ServiceName = new char[lstrlen(szInternName)+1];
	lstrcpy(ServiceName,szInternName);

	SERVICE_TABLE_ENTRY stb[] =
	{
		{ (char *)szInternName,(LPSERVICE_MAIN_FUNCTION) ServiceMain} ,
		{ NULL, NULL }
	};

	return StartServiceCtrlDispatcher(stb); //register with the Service Manager
}


/**
  安装服务、服务管理

  nError values:
  - 0  success
  - 1  Can't open the Service manager
  - 2  Failed to create service.
*/


BOOL NTService::Install()
{
	BOOL ret_val = FALSE;
	SC_HANDLE newService, schManager, schService;
	SERVICE_DESCRIPTION sd;

	if (!SeekStatus(1))
		return FALSE;

	char szFilePath[_MAX_PATH];
	GetModuleFileName(NULL, szFilePath, sizeof(szFilePath));

	// open a connection to the SCM
	if (!(schManager = OpenSCManager(0, 0, SC_MANAGER_CREATE_SERVICE)))
		printf("Failed to install the service %d.(Couldn't open the SCM %d)\n", GetLastError());
	else 				// Install the new service
	{
		if (!(newService=
			CreateService(schManager,
				szInternName,
				szDisplayName,
				dwDesiredAccess,//default: SERVICE_ALL_ACCESS
				dwServiceType,	//default: SERVICE_WIN32_OWN_PROCESS
							//default: SERVICE_AUTOSTART
				SERVICE_AUTO_START,
				dwErrorControl,	//default: SERVICE_ERROR_NORMAL
				szFilePath,	//exec full path
				szLoadOrderGroup, //default: NULL
				lpdwTagID,	//default: NULL
				szDependencies,	//default: NULL
				szAccountName,	//default: NULL
				szPassword)))	//default: NULL
			printf("Failed to install the service. %d(Couldn't create service)\n", GetLastError());
		else
		{
			printf("Service successfully installed.\n");
			if (!(schService = OpenService(schManager, szInternName, SERVICE_CHANGE_CONFIG)))
			{
				printf("Failed to OpenService the service %d\n", GetLastError());
			}
			else
			{
				sd.lpDescription = "流媒体服务，用于连接相机转发RTSP流。";
				if (!ChangeServiceConfig2(schService, SERVICE_CONFIG_DESCRIPTION, &sd))
				{
					printf("ChanageServiceConfig2 failed\n");
				}
				else
					printf("Service desription updated successfully.\n");
				CloseServiceHandle(schService);

			}
			CloseServiceHandle(newService);
			ret_val=TRUE;				// Everything went ok
		}
		CloseServiceHandle(schManager);
	}
	
	return ret_val;
}


/**
  删除服务

  nError values:
  - 0  success
  - 1  Can't open the Service manager
  - 2  Failed to locate service
  - 3  Failed to delete service.
*/


BOOL NTService::Remove()
{
	BOOL ret_value=FALSE;
	SC_HANDLE service, scm;

	if (!SeekStatus(0))
		return FALSE;

	nError=0;

	// open a connection to the SCM
	if (!(scm = OpenSCManager(0, 0, SC_MANAGER_CREATE_SERVICE)))
	{
		printf("Failed to remove the service (Couldn't open the SCM)\n");
	}
	else
	{
		if ((service = OpenService(scm,szInternName, DELETE)))
		{
			if (!DeleteService(service))
				printf("Failed to remove the service\n");
			else
			{
				printf("Service successfully removed.\n");
				ret_value=TRUE;				// everything went ok
			}
			CloseServiceHandle(service);
		}
		else
			printf("Failed to remove the service (Couldn't open the service)\n");
			CloseServiceHandle(scm);
	}
	return ret_value;
}

/**
  this function should be called before the app. exits to stop
  the service
*/
void NTService::Stop(void)
{
	SetStatus(SERVICE_STOP_PENDING,NO_ERROR, 0, 1, 60000);
	StopService();
	SetStatus(SERVICE_STOPPED, NO_ERROR, 0, 1, 1000);
}

/**
  This is the function that is called from the
  service manager to start the service.
*/


void NTService::ServiceMain(DWORD argc, LPTSTR *argv)
{

	// registration function 注册函数
	if (!(pService->hServiceStatusHandle =
		RegisterServiceCtrlHandler(pService->ServiceName,
					(LPHANDLER_FUNCTION)
					NTService::ServiceCtrlHandler)))
		goto error;

	// notify SCM of progress 设置scm状态标志
	if (!pService->SetStatus(SERVICE_START_PENDING,NO_ERROR, 0, 1, 8000))
		goto error;

	// create the exit event 创建退出事件
	if (!(pService->hExitEvent = CreateEvent(0, TRUE, FALSE,0)))
		goto error;

	if (!pService->SetStatus(SERVICE_START_PENDING,NO_ERROR, 0, 3,
				pService->nStartTimeOut))
		goto error;

	// save start arguments 保存启动参数
	pService->my_argc=argc;
	pService->my_argv=argv;

	// start the service
	if (!pService->StartService())
		goto error;

	// wait for exit event
	WaitForSingleObject (pService->hExitEvent, INFINITE);

	// wait for thread to exit
	if (WaitForSingleObject (pService->hThreadHandle, INFINITE) == WAIT_TIMEOUT)
		CloseHandle(pService->hThreadHandle);

	pService->Exit(0);
	return;

error:
	pService->Exit(GetLastError());
	return;
}



void NTService::SetRunning()
{
	if (pService)
		pService->SetStatus(SERVICE_RUNNING, NO_ERROR, 0, 0, 0);
}

void NTService::SetSlowStarting(unsigned long timeout)
{
	if (pService)
		pService->SetStatus(SERVICE_START_PENDING,NO_ERROR, 0, 0, timeout);
}


/* ------------------------------------------------------------------------
   StartService() - starts the application thread 
					启动应用程序线程
 -------------------------------------------------------------------------- */

BOOL NTService::StartService()
{
	// Start the real service's thread (application)  开始真正的服务线程
	if (!(hThreadHandle = (HANDLE) _beginthread((THREAD_FC)fpServiceThread,0,
							(void *) this)))
		return FALSE;
	bRunning = TRUE;
	return TRUE;
}
/* ------------------------------------------------------------------------
		停止服务
 -------------------------------------------------------------------------- */
void NTService::StopService()
{
	bRunning=FALSE;

	// Set the event for application 设置应用程序的事件
	if (hShutdownEvent)
		SetEvent(hShutdownEvent);

	// Set the event for ServiceMain 设置ServiceMain的事件
	SetEvent(hExitEvent);
}
/* ------------------------------------------------------------------------
		挂起指定的线程
 -------------------------------------------------------------------------- */
void NTService::PauseService()
{
	bPause = TRUE;
	SuspendThread(hThreadHandle);
}
/* ------------------------------------------------------------------------
		消耗线程挂起的时间计数，直到时间计数为0，则激活线程
 -------------------------------------------------------------------------- */
void NTService::ResumeService()
{
	bPause=FALSE;
	ResumeThread(hThreadHandle);
}
/* ------------------------------------------------------------------------

 -------------------------------------------------------------------------- */
BOOL NTService::SetStatus (DWORD dwCurrentState,DWORD dwWin32ExitCode,
			   DWORD dwServiceSpecificExitCode, DWORD dwCheckPoint,
			   DWORD dwWaitHint)
{
	BOOL bRet;
	SERVICE_STATUS serviceStatus;

	dwState=dwCurrentState;

	serviceStatus.dwServiceType	= SERVICE_WIN32_OWN_PROCESS;
	serviceStatus.dwCurrentState = dwCurrentState;

	if (dwCurrentState == SERVICE_START_PENDING)
		serviceStatus.dwControlsAccepted = 0;	//don't accept control events 不接收控制事件
	else
		serviceStatus.dwControlsAccepted =    (SERVICE_ACCEPT_STOP |
						SERVICE_ACCEPT_PAUSE_CONTINUE |
						SERVICE_ACCEPT_SHUTDOWN);

	// if a specific exit code is defined,set up the win32 exit code properly 如果定义一个特定的退出代码，则建立win32正常退出代码
	if (dwServiceSpecificExitCode == 0)
		serviceStatus.dwWin32ExitCode = dwWin32ExitCode;
	else
		serviceStatus.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;

	serviceStatus.dwServiceSpecificExitCode = dwServiceSpecificExitCode;

	serviceStatus.dwCheckPoint = dwCheckPoint;
	serviceStatus.dwWaitHint   = dwWaitHint;

	// Pass the status to the Service Manager
	if (!(bRet=SetServiceStatus(hServiceStatusHandle, &serviceStatus)))
		StopService();

	return bRet;
}
/* ------------------------------------------------------------------------

 -------------------------------------------------------------------------- */
void NTService::ServiceCtrlHandler(DWORD ctrlCode)
{
	DWORD  dwState;

	if (!pService)
		return;

	dwState=pService->dwState;  // get current state

	switch(ctrlCode) 
	{
	case SERVICE_CONTROL_SHUTDOWN:
	case SERVICE_CONTROL_STOP:
		dwState = SERVICE_STOP_PENDING;
		pService->SetStatus(SERVICE_STOP_PENDING,NO_ERROR, 0, 1,
				pService->nStopTimeOut);
		pService->StopService();
		Sleep(500);
		pService->Stop();
		break;

	default:
		pService->SetStatus(dwState, NO_ERROR,0, 0, 0);
		break;
	}
	//pService->SetStatus(dwState, NO_ERROR,0, 0, 0);
}

/* ------------------------------------------------------------------------

 -------------------------------------------------------------------------- */

void NTService::Exit(DWORD error)
{
	if (hExitEvent)
		CloseHandle(hExitEvent);

	// Send a message to the scm to tell that we stop  // 给scm发送停止消息
	if (hServiceStatusHandle)
		SetStatus(SERVICE_STOPPED, error, 0, 0, 0);

	// If the thread has started kill it ???
	// if (hThreadHandle) CloseHandle(hThreadHandle);

}

/* ------------------------------------------------------------------------

 -------------------------------------------------------------------------- */

BOOL NTService::SeekStatus(int OperationType)
{
	BOOL ret_value=FALSE;
	SC_HANDLE service, scm;

	// open a connection to the SCM
	if (!(scm = OpenSCManager(0, 0, SC_MANAGER_CREATE_SERVICE)))
	{
		DWORD ret_error=GetLastError();
		if (ret_error == ERROR_ACCESS_DENIED)
		{
			printf("Install/Remove of the Service Denied!\n");
			if (!is_super_user())
				printf("That operation should be made by an user with Administrator privileges!\n");
		}
		else
			printf("There is a problem for to open the Service Control Manager!\n");
	}
	else
	{
		if (OperationType == 1)
		{
			/* an install operation */
			if ((service = OpenService(scm,szInternName, SERVICE_ALL_ACCESS )))
			{
				LPQUERY_SERVICE_CONFIG ConfigBuf;
				DWORD dwSize;

				ConfigBuf = (LPQUERY_SERVICE_CONFIG) LocalAlloc(LPTR, 4096);
				printf("The service already exists!\n");
				if (QueryServiceConfig(service,ConfigBuf,4096,&dwSize))
					printf("The current server installed: %s\n",
						ConfigBuf->lpBinaryPathName);
				LocalFree(ConfigBuf);
				CloseServiceHandle(service);
			}
			else
				ret_value=TRUE;
		}
		else
		{
			/* a remove operation */
			if (!(service = OpenService(scm,szInternName, SERVICE_ALL_ACCESS )))
				printf("The service doesn't exist!\n");
			else
			{
				SERVICE_STATUS ss;

				memset(&ss, 0, sizeof(ss));
				if (QueryServiceStatus(service,&ss))
				{
					DWORD dwState = ss.dwCurrentState;
					if (dwState == SERVICE_RUNNING)
						printf("Failed to remove the service because the service is running\nStop the service and try again\n");
					else if (dwState == SERVICE_STOP_PENDING)
						printf("\
							Failed to remove the service because the service is in stop pending state!\n\
							Wait 30 seconds and try again.\n\
							If this condition persist, reboot the machine and try again\n");
					else
						ret_value= TRUE;
				}
				CloseServiceHandle(service);
			}
		}
		CloseServiceHandle(scm);
	}

	return ret_value;
}
/* ------------------------------------------------------------------------
 -------------------------------------------------------------------------- */
BOOL NTService::IsService(LPCSTR ServiceName)
{
	BOOL ret_value=FALSE;
	SC_HANDLE service, scm;
  
	if ((scm= OpenSCManager(0, 0,SC_MANAGER_ENUMERATE_SERVICE)))
	{
		if ((service = OpenService(scm,ServiceName, SERVICE_QUERY_STATUS)))
		{
			ret_value=TRUE;
			CloseServiceHandle(service);
		}
		CloseServiceHandle(scm);
	}
	return ret_value;
}
/* ------------------------------------------------------------------------
		比较argv[1]与service_option，相同返回TRUE,失败返回FALSE.
 -------------------------------------------------------------------------- */
BOOL NTService::got_service_option(char **argv, char *service_option)
{
	char *option;
	for (option= argv[1]; *option; option++)
		if (!strcmp(option, service_option))
			return TRUE;
	return FALSE;
}
/* ------------------------------------------------------------------------
 -------------------------------------------------------------------------- */
BOOL NTService::is_super_user()
{
	HANDLE hAccessToken;
	UCHAR InfoBuffer[1024];
	PTOKEN_GROUPS ptgGroups=(PTOKEN_GROUPS)InfoBuffer;
	DWORD dwInfoBufferSize;
	PSID psidAdministrators;
	SID_IDENTIFIER_AUTHORITY siaNtAuthority = SECURITY_NT_AUTHORITY;
	UINT x;
	BOOL ret_value=FALSE;
 
	if (!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE,&hAccessToken ))
	{
		if (GetLastError() != ERROR_NO_TOKEN)
			return FALSE;
 
		if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hAccessToken))
			return FALSE;
	}
 
	ret_value= GetTokenInformation(hAccessToken,TokenGroups,InfoBuffer,
                                 1024, &dwInfoBufferSize);

	CloseHandle(hAccessToken);
 
	if (!ret_value )
		return FALSE;
 
	if (!AllocateAndInitializeSid(&siaNtAuthority, 2,
					SECURITY_BUILTIN_DOMAIN_RID,
					DOMAIN_ALIAS_RID_ADMINS,
					0, 0, 0, 0, 0, 0,
					&psidAdministrators))
		return FALSE;

	ret_value = FALSE;
 
	for (x=0;x<ptgGroups->GroupCount;x++)
	{
		if ( EqualSid(psidAdministrators, ptgGroups->Groups[x].Sid) )
		{
			ret_value = TRUE;
			break;
		}
 
	}
	FreeSid(psidAdministrators);
	return ret_value;
}
