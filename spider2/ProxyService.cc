//	proxyserver include 
#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"

// ProxyService include
#include "ProxyService.h"

// Ws2_32 lib
#pragma comment(lib, "Ws2_32.lib")

//	proxyserver lib
#ifdef _DEBUG
#pragma comment(lib, "../Debug/BasicUsageEnvironment.lib")
#pragma comment(lib, "../Debug/liveMedia.lib")
#pragma comment(lib, "../Debug/groupsock.lib")
#pragma comment(lib, "../Debug/UsageEnvironment.lib")
#else
#pragma comment(lib, "../Release/BasicUsageEnvironment.lib")
#pragma comment(lib, "../Release/liveMedia.lib")
#pragma comment(lib, "../Release/groupsock.lib")
#pragma comment(lib, "../Release/UsageEnvironment.lib")
#endif // _DEBUG


// proxy parameters:
char const *progName;
UsageEnvironment *env;
UserAuthenticationDatabase * authDB;

int verbosityLevel = 0;
Boolean streamRTSPOverTCP = False;
portNumBits tunnelOverHTTPPortNum = 0;
char *username = NULL;
char *password = NULL;
Boolean proxyREGISTERRequests = False;

int rtspNum = 0;
char *rtspBuffer[256];



/**
	create rtsp service
*/
static RTSPServer *createRTSPServer(Port port)
{
	if (proxyREGISTERRequests)
	{
//		return RTSPServerWithREGISTERProxying::createNew(*env, port, authDB, 65, 
//			streamRTSPOverTCP, verbosityLevel);
	}
	else
	{
		return RTSPServer::createNew(*env, port, authDB);
	}
}

/**
	proxy service main
*/
unsigned WINAPI proxy(void *p)
{
	OutPacketBuffer::maxSize = 100000; 

	//  建立环境
	TaskScheduler * scheduler = BasicTaskScheduler::createNew();
	env = BasicUsageEnvironment::createNew(*scheduler);

	rtspNum = 1;
	rtspBuffer[0] = "rtsp://192.168.1.125:8557/PSIA/Streaming/channels/2?videoCodecType=H.264";
	// 确保参数rtsp://
	for (int i = 0; i < rtspNum; ++i)
	{
		if (strncmp(rtspBuffer[i], "rtsp://", 7) != 0)
		{
		}
	}

	// 做一些无多余的无效检查
	if (streamRTSPOverTCP)
	{
		if (tunnelOverHTTPPortNum > 0)
		{
		}
		else
		{
			tunnelOverHTTPPortNum = (portNumBits)(~0);  // 通过tcp stream, 不用http
		}
	}

	authDB = NULL;
#ifdef ACCESS_CONTROL
	// 实现服务器端访问控制服务器,执行以下操作:
	authDB = new UserAuthenticationDatabase;
	authDB->addUserRecord("username1", "password1");
#endif

	// 创建rtsp服务器,默认端口554,备用端口8554
	RTSPServer *rtspServer;
	portNumBits rtspServerPortNum = 554;
	rtspServer = createRTSPServer(rtspServerPortNum);
	if (NULL == rtspServer)
	{
		rtspServerPortNum = 8554;
		rtspServer = createRTSPServer(rtspServerPortNum);
	}
	if (NULL == rtspServer)
	{
		return 0;
	}

//	ss_obj_service.SetSlowStarting(15000);
	// 创建代理服务器
	for (int i = 0; i < rtspNum; ++i)
	{
		char const *proxiedStreamURL = rtspBuffer[i];
		char streamName[30];
		sprintf(streamName, "zhst/CamStream/output-%d", i);
		ServerMediaSession * sms 
			= ProxyServerMediaSession::createNew(*env, rtspServer,
					proxiedStreamURL, streamName,
					username, password, tunnelOverHTTPPortNum, verbosityLevel);
		rtspServer->addServerMediaSession(sms);

		char *proxyStreamURL = rtspServer->rtspURL(sms);
		delete[] proxyStreamURL;
	}

	// 创建HTTP服务器 RTSP-over-HTTP
	if (rtspServer->setUpTunnelingOverHTTP(80) || rtspServer->setUpTunnelingOverHTTP(8000)
			|| rtspServer->setUpTunnelingOverHTTP(8080))
	{

	}

	// 消息循环
	env->taskScheduler().doEventLoop();

	return 0;
}