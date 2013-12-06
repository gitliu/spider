#ifndef NT_SERVICE_INCLUDED
#define NT_SERVICE_INCLUDED

/**
  @file

  @brief
  Windows NT Service class library

  Copyright Abandoned 1998 Irena Pancirov - Irnet Snc
  This file is public domain and comes with NO WARRANTY of any kind
*/

// 主应用程序线程
typedef void (*THREAD_FC)(void *);

class NTService
{
public:
    NTService();
   ~NTService();

    BOOL    bOsNT;	      ///< true if OS is NT, false for Win95
	
	LPCSTR szInternName;
	LPCSTR szDisplayName;
	LPCSTR szAccountName;
	LPCSTR szPassword;
    
	// 安装选项
    DWORD   dwDesiredAccess;
    DWORD   dwServiceType;
    DWORD   dwStartType;
    DWORD   dwErrorControl;

    LPSTR   szLoadOrderGroup;
    LPDWORD lpdwTagID;
    LPSTR   szDependencies;
    OSVERSIONINFO osVer;

    // 超时
    int     nStartTimeOut;
    int     nStopTimeOut;
    int     nPauseTimeOut;
    int     nResumeTimeOut;

    //
    DWORD   my_argc;
    LPTSTR *my_argv;
    HANDLE  hShutdownEvent;
    int     nError;
    DWORD   dwState;

    BOOL GetOS();	      // returns TRUE if WinNT
    BOOL IsNT() { return bOsNT;}

    // 初始化服务入口点
    long Init(void *ServiceThread);

    // 应用程序关闭事件
    void SetShutdownEvent(HANDLE hEvent){ hShutdownEvent=hEvent; }

    // 服务安装与卸载
    BOOL Install();
    BOOL SeekStatus(int OperationType);
    BOOL Remove();
    BOOL IsService(LPCSTR ServiceName);
    BOOL got_service_option(char **argv, char *service_option);
    BOOL is_super_user();

    /* 
		SetRunning()调用的应用程序
		初始化完成后,它可以接受
		停止请求
    */
    void SetRunning(void);

    /**
      Sets a timeout after which SCM will abort service startup if SetRunning()
      was not called or the timeout was not extended with another call to
      SetSlowStarting(). Should be called when static initialization completes,
      and the variable initialization part begins

      @arg timeout  the timeout to pass to the SCM (in milliseconds)
    */
    void SetSlowStarting(unsigned long timeout);

    /*
      Stop() is to be called by the application to stop 
      the service
    */
    void Stop(void); 

protected:
    LPSTR		   ServiceName;
    HANDLE		   hExitEvent;
    SERVICE_STATUS_HANDLE  hServiceStatusHandle;
    BOOL		   bPause;
    BOOL		   bRunning;
    HANDLE		   hThreadHandle;
    THREAD_FC	   fpServiceThread;

    void PauseService();	// 挂起指定的线程
    void ResumeService();	// 消耗线程挂起的时间计数，直到时间计数为0，则激活线程
    void StopService();		// 停止服务
    BOOL StartService();	// 启动服务

    static void ServiceMain(DWORD argc, LPTSTR *argv);
    static void ServiceCtrlHandler (DWORD ctrlCode);

    void Exit(DWORD error);
    BOOL SetStatus (DWORD dwCurrentState,DWORD dwWin32ExitCode,
		    DWORD dwServiceSpecificExitCode,
		    DWORD dwCheckPoint,DWORD dwWaitHint);

};
/* ------------------------- the end -------------------------------------- */

#endif /* NT_SERVICE_INCLUDED */
