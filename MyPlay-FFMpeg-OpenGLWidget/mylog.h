#ifndef FOUNDATION_MYLOG_H
#define FOUNDATION_MYLOG_H

#include <condition_variable>
#include <mutex>
#include <thread>
#include <string>
#include <stdlib.h>

#include <time.h>
#include <stdarg.h>     //for 可变参数
#include <sys/timeb.h>  //ftimb(); 到毫秒

#if defined _WINDOWS
    #include <windows.h>
    #include <io.h>
    #define snprintf   _snprintf
    #define vsnprintf  _vsnprintf

    #define snwprintf   _snwprintf
    #define vsnwprintf  _vsnwprintf

    //#define access _access
    const int F_OK = 0;
    const int W_OK = 2;
    const int R_OK = 4;
    const int RW_OK = 6;

#else
    #include <unistd.h>  //for F_OK:0; X_OK:1; W_OK:2; R_OK:4
#endif

const int KByte = (1024);                //KByte
const int MByte = (1024 * 1024);         //MByte
const int GByte = (1024 * 1024 * 1024);  //GByte


//定义日志宏
#define LOG CMyLog::instance()->writeLog

//日志级别
typedef enum LOG_LEVEL_
{
    ALL = 0
    , Timer = 0x1
    , Debug = 0x2
    , Info  = 0x4
    , Warn  = 0x8
    , Error = 0x10
}LOG_LEVEL;


//日志类
class CMyLog
{
public:
	static CMyLog* instance()
	{
		static CMyLog obj;
		return &obj;
	}

	void init();
	void writeLog(LOG_LEVEL enLevel, const char *lpszFormat, ...);
	void writeLog(LOG_LEVEL enLevel, const wchar_t *lpwszFormat, ...);
	
	//void setLogFileSize(int iSize);
    void setLogLevel(LOG_LEVEL enLevel);

private:
	CMyLog();
	virtual ~CMyLog();

	int parseCfg();
	int openLogFile();
	int createLogFile();  //创建日志文件
	void thread_writeLogFile();

private:
	std::condition_variable m_Condition;
	std::thread m_thread;
	std::mutex m_Mutex;  
	//std::unique_lock <std::mutex> lock(m_Mutex);  //唯一锁
	//std::unique_lock<std::mutex> m_UniqueLock;

	char *m_pLogBuff;
	char *m_pLogBuffBak;
	int  m_iLogBuff;
	int  m_iLogBuffBak;

	char m_szLogLevel[16];    //日志级别名称
	LOG_LEVEL m_enLogLevel;

	va_list m_args;

	FILE *m_pFile;
	char m_szDir[256];       //目录
	char m_szFileExt[32];    //扩展名
	char m_szFileName[128];  //文件名
	char m_szExePath[128];   //当前程序的路径名

	int  m_iFileIndex;       //日志文件序号
	int  m_iFileLength;      //日志文件字节数

	bool m_bExitFlag;        //结束标志
};


#endif
