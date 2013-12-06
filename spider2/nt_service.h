#ifndef NT_SERVICE_INCLUDED
#define NT_SERVICE_INCLUDED

/**
  @file

  @brief
  Windows NT Service class library

  Copyright Abandoned 1998 Irena Pancirov - Irnet Snc
  This file is public domain and comes with NO WARRANTY of any kind
*/

// ��Ӧ�ó����߳�
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
    
	// ��װѡ��
    DWORD   dwDesiredAccess;
    DWORD   dwServiceType;
    DWORD   dwStartType;
    DWORD   dwErrorControl;

    LPSTR   szLoadOrderGroup;
    LPDWORD lpdwTagID;
    LPSTR   szDependencies;
    OSVERSIONINFO osVer;

    // ��ʱ
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

    // ��ʼ��������ڵ�
    long Init(void *ServiceThread);

    // Ӧ�ó���ر��¼�
    void SetShutdownEvent(HANDLE hEvent){ hShutdownEvent=hEvent; }

    // ����װ��ж��
    BOOL Install();
    BOOL SeekStatus(int OperationType);
    BOOL Remove();
    BOOL IsService(LPCSTR ServiceName);
    BOOL got_service_option(char **argv, char *service_option);
    BOOL is_super_user();

    /* 
		SetRunning()���õ�Ӧ�ó���
		��ʼ����ɺ�,�����Խ���
		ֹͣ����
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

    void PauseService();	// ����ָ�����߳�
    void ResumeService();	// �����̹߳����ʱ�������ֱ��ʱ�����Ϊ0���򼤻��߳�
    void StopService();		// ֹͣ����
    BOOL StartService();	// ��������

    static void ServiceMain(DWORD argc, LPTSTR *argv);
    static void ServiceCtrlHandler (DWORD ctrlCode);

    void Exit(DWORD error);
    BOOL SetStatus (DWORD dwCurrentState,DWORD dwWin32ExitCode,
		    DWORD dwServiceSpecificExitCode,
		    DWORD dwCheckPoint,DWORD dwWaitHint);

};
/* ------------------------- the end -------------------------------------- */

#endif /* NT_SERVICE_INCLUDED */
