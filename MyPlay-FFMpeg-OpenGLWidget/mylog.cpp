#include "mylog.h"

int MaxLogFileSize  = (MByte * 5);  //日志文件大小
int MaxLogFileIndex = (5);          //日志文件数量

const int LogLineSize = (KByte);
const int MaxLineSize = (KByte * 10);

const int LogBuffSize  = (MByte);
const int LogBuffOver = (LogBuffSize - LogLineSize);
const int LogBuffWarn  = (LogBuffSize - (MaxLineSize * 2));

//--------------------------------------------------------------------

CMyLog::CMyLog()
{
	m_pLogBuff    = NULL;
	m_pLogBuffBak = NULL;
	m_iLogBuff    = 0;
	m_iLogBuffBak = 0;

	memset(m_szLogLevel, 0, sizeof(m_szLogLevel));
	m_enLogLevel = ALL;  //初始日志全开

	m_pFile = NULL;
	memset(m_szDir,      0, sizeof(m_szDir));
	memset(m_szFileExt,  0, sizeof(m_szFileExt));
	memset(m_szFileName, 0, sizeof(m_szFileName));
	memset(m_szExePath,  0, sizeof(m_szExePath));

	m_iFileIndex = 0;
	m_iFileLength = 0;

	m_bExitFlag = false;

	init();
}

CMyLog::~CMyLog()
{
	//结束线程
	if (m_thread.joinable())
	{
		m_bExitFlag = true;

		m_Mutex.lock();  //---加锁
		m_Condition.notify_one();
		m_Mutex.unlock();  //---解锁

		m_thread.join();
	}

	//关闭文件
	if (m_pFile)
	{
		fclose(m_pFile);
		m_pFile = NULL;
	}

	//释放缓存区
	if (NULL != m_pLogBuff)
	{
		delete[] m_pLogBuff;
		m_pLogBuff = NULL;
	}

	if (NULL != m_pLogBuffBak)
	{
		delete[] m_pLogBuffBak;
		m_pLogBuffBak = NULL;
	}

	m_iLogBuff = 0;
	m_iLogBuffBak = 0;
}

int CMyLog::parseCfg()
{


	return false;
}

//初始化日志系统
void CMyLog::init()
{
	printf("CMyLog::init()---> szLogFile = %s \n", m_szExePath);

	setlocale(LC_ALL, "");

	if (m_szExePath[0] == '\0')
	{
    #if defined _WINDOWS
		//char szFilePath[MAX_PATH];
		GetModuleFileNameA(NULL, m_szExePath, sizeof(m_szExePath)-1);
		//CMyLog::instance()->init(szFilePath);
    #else
		//char szFilePath[128] = { 0 };
		int ret = readlink("/proc/self/exe", m_szExePath, sizeof(m_szExePath) - 1);
		if (ret >= 0)
		{
			m_szExePath[ret] = '\0';
		}
    #endif
	}

	//----------------------------------------------

	//分解路径-查找分隔符
	char *pch = strrchr(m_szExePath, '\\');
	if (NULL == pch)
	{
		pch = strrchr(m_szExePath, '/');
		if (NULL == pch)
		{
			printf("CMyLog::init()---> strrchr(szLogFile, '/') = NULL; \n");
			return;
		}
	}

	//文件名
	strncpy(m_szFileName, pch + 1, sizeof(m_szFileName) - 1);
	if (m_szFileName[0] == '\0')
	{
		printf("CMyLog::init()---> m_szFileName = NULL; \n");
		return;
	}

	//文件目录
	pch[1] = '\0';
	strncpy(m_szDir, m_szExePath, sizeof(m_szDir) - 1);

	//扩展名
	pch = strrchr(m_szFileName, '.');
	if (pch)
	{
		pch[0] = '\0';
		strncpy(m_szFileExt, pch + 1, sizeof(m_szFileExt) - 1);
	}

    //-------------------------------------------------

	//尝试读配置文件
	int iRet = parseCfg();

	//检查目录存在
	//int _waccess( const wchar_t *path, int mode );
	iRet = _access(m_szDir, F_OK);
	if (0 != iRet)
	{
		printf("CMyLog::init()---> _access(m_szDir, F_OK) = %d; \n", iRet);
		return;
	}

	//打开日志文件
	iRet = openLogFile();
	if ((int)false == iRet)
	{
		printf("CMyLog::init()---> openLogFile() fail \n");
		return;
	}

	//初始化日志缓存区
	if (NULL == m_pLogBuff)
	{
		m_pLogBuff = new char[LogBuffSize];
		if (NULL == m_pLogBuff)
		{
			printf("CMyLog::init()---> m_pLogBuff = new char[%d] fail \n", LogBuffSize);
			return;
		}

		m_iLogBuff = 0;
	}

	if (NULL == m_pLogBuffBak)
	{
		m_pLogBuffBak = new char[LogBuffSize];
		if (NULL == m_pLogBuffBak)
		{
			printf("CMyLog::init()---> m_pLogBuffBak = new char[%d] fail \n", LogBuffSize);
			return;
		}

		m_iLogBuffBak = 0;
	}

	//--------------------------------------------------

	//创建写日志线程
	//std::thread *pT = &std::thread(&CMyLog::thread_writeLogFile, this);
	m_thread = std::thread(&CMyLog::thread_writeLogFile, this);
	printf("CMyLog::init() Succ \n");
	return;
}

//打开日志文件
int CMyLog::openLogFile()
{
	char szFilePath[256] = { 0 };

	//打开日志文件
	for (int m_iFileIndex = 0; m_iFileIndex < MaxLogFileIndex; ++m_iFileIndex)
	{
		//拼接文件名
		int iRet = snprintf(szFilePath, sizeof(szFilePath) - 1, "%s%s_%d.log", m_szDir, m_szFileName, m_iFileIndex);

		//打开文件
		m_pFile = fopen(szFilePath, "ab");
		if (NULL == m_pFile)
		{
			printf("CMyFile::openLogFile()---> fopen(ab) fail: %s \n", szFilePath);
			//perror(szFilePath);
			continue;
		}

		//检查文件长度
		m_iFileLength = fseek(m_pFile, 0, SEEK_END);
		m_iFileLength = ftell(m_pFile);
		if (m_iFileLength > (MaxLogFileSize - MaxLineSize))
		{
			printf("CMyFile::openLogFile()---> Fail: m_iFileLength = %d; %s\n", m_iFileLength, szFilePath);
			fclose(m_pFile);
			m_pFile = NULL;
			continue;
		}

		//return true;

		//-----------------------------------------------------------------

		//拼装文件头字符串
		char szLogHead[1024] = { 0 };
		int iCount = 0;
		iCount += sprintf(szLogHead, "\r\n===================== Build Date: %s %s =====================\r\n", __DATE__, __TIME__);
		iCount += sprintf(szLogHead + iCount, "LogFile: %s \r\n", szFilePath);

		//写入日志文件
		iCount = (int)fwrite(szLogHead, sizeof(char), iCount, m_pFile);
		fflush(m_pFile);

		//文件长度
		m_iFileLength += iCount;
		//iCount = ftell(m_pFile);
		printf("CMyFile::openLogFile()---> Succ: m_iFileLength = %d; %s\n", m_iFileLength, szFilePath);
		return true;
	}

	return false;
}

//创建日志文件
int CMyLog::createLogFile()
{
	//关闭旧文件
	fclose(m_pFile);
	m_pFile = NULL;

	//文件名
	char szFilePath[256] = { 0 };

	//打开日志文件
	for (int iIndex = 0; iIndex < MaxLogFileIndex; ++iIndex)
	{
		//文件序号
		if (++m_iFileIndex >= MaxLogFileIndex)
		{
			m_iFileIndex = (m_iFileIndex % MaxLogFileIndex);
		}

		//拼接文件名
		int iRet = snprintf(szFilePath, sizeof(szFilePath) - 1, "%s/%s_%d.log", m_szDir, m_szFileName, m_iFileIndex);

		//创建文件
		m_pFile = fopen(szFilePath, "wb");
		if (NULL == m_pFile)
		{
			printf("CMyFile::createLogFile()---> fopen(wb) fail: %s \n", szFilePath);
			continue;
		}

		//文件长度
		m_iFileLength = 0;

		//-----------------------------------------------------------------

		//拼装文件头字符串
		char szLogHead[1024] = { 0 };
		int iCount = 0;
		iCount += sprintf(szLogHead, "\r\n\r\n===================== Build Date: %s %s =====================\r\n", __DATE__, __TIME__);
		iCount += sprintf(szLogHead + iCount, "LogFile: %s \r\n", szFilePath);

		//写入日志文件
		iCount = (int)fwrite(m_pLogBuff, sizeof(char), iCount, m_pFile);
		fflush(m_pFile);

		//文件长度
		m_iFileLength += iCount;

		printf("CMyFile::createLogFile()---> Succ: m_iFileLength = %d; %s\n", m_iFileLength, szFilePath);

		return true;
	}

	return false;
}

//线程函数---写日志文件
void CMyLog::thread_writeLogFile()
{
	if (NULL == m_pLogBuff || NULL == m_pLogBuffBak)
	{
		printf("CMyLog::thread_writeLogFile()---> Parameter check fail \n");
		return;
	}

	std::unique_lock <std::mutex> lock(m_Mutex);  //唯一锁

	m_bExitFlag = false;

	while (!m_bExitFlag)
	{
		//std::cv_status::timeout;
		m_Condition.wait_for(lock, std::chrono::seconds(2));  //等待2秒
		if (m_iLogBuff <= 0)
		{
			continue;
		}

		//交换缓存区
		char *pch = m_pLogBuff;
		m_pLogBuff = m_pLogBuffBak;
		m_pLogBuffBak = pch;

		m_iLogBuffBak = m_iLogBuff;
		m_iLogBuff = 0;

		lock.unlock();  //---解锁

        //创建新文件
		if (NULL == m_pFile)
		{
			int iRet = createLogFile();
		}

		//写入日志文件
		if (m_pFile)
		{
			//m_iLogBuffBak /= sizeof(wchar_t);
			//int iCount = (int)fwrite(m_pLogBuffBak, sizeof(wchar_t), m_iLogBuffBak, m_pFile);
			int iCount = (int)fwrite(m_pLogBuffBak, sizeof(char), m_iLogBuffBak, m_pFile);
			if (iCount == m_iLogBuffBak)
			{
				fflush(m_pFile);

				//计算文件长度
				m_iFileLength += iCount;
				if (m_iFileLength >= MaxLogFileSize)
				{
					//创建新文件
					int iRet = createLogFile();
				}
			}
			else
			{
				//创建新文件
				int iRet = createLogFile();
			}
		}

		lock.lock();  //---加锁
	}

	return;
}

/*
#include <sys/timeb.h>
struct timeb tb;
ftime(&tb);
printf(".%03d", tb.millitm);
*/

void CMyLog::writeLog(LOG_LEVEL enLevel, const char *lpszFormat, ...)
{
    if(enLevel < m_enLogLevel)
    {
        return;
    }

    //取系统时间
	char szDateTime[64] = { 0 };
	struct timeb tb;
	ftime(&tb);

	//转换成本地时间
	struct tm *pstTM = localtime(&tb.time);
	if (NULL == pstTM)
	{
		return;
	}

	//日志行缓存
	char szLogBuff[LogLineSize] = { 0 };
	int iLogBuff = 0;

	//写记录头: [Level][时:分:秒.毫秒]
	switch (enLevel)  
	{
	case Timer:
		iLogBuff = snprintf(szLogBuff, sizeof(szLogBuff),
			"[TIMER] [%d-%02d-%02d %02d:%02d:%02d.%03d] ",
			pstTM->tm_year + 1900,
			pstTM->tm_mon + 1,
			pstTM->tm_mday,
			pstTM->tm_hour,
			pstTM->tm_min,
			pstTM->tm_sec,
			tb.millitm);
		break;

	case Debug:
		iLogBuff = snprintf(szLogBuff, sizeof(szLogBuff),
			"[DEBUG] [%d-%02d-%02d %02d:%02d:%02d.%03d] ",
			pstTM->tm_year + 1900,
			pstTM->tm_mon + 1,
			pstTM->tm_mday,
			pstTM->tm_hour,
			pstTM->tm_min,
			pstTM->tm_sec,
			tb.millitm);
		break;

	case Info:
		iLogBuff = snprintf(szLogBuff, sizeof(szLogBuff),
			"[INFO ] [%d-%02d-%02d %02d:%02d:%02d.%03d] ",
			pstTM->tm_year + 1900,
			pstTM->tm_mon + 1,
			pstTM->tm_mday,
			pstTM->tm_hour,
			pstTM->tm_min,
			pstTM->tm_sec,
			tb.millitm);
		break;

	case Warn:
		iLogBuff = snprintf(szLogBuff, sizeof(szLogBuff),
			"[WARN ] [%d-%02d-%02d %02d:%02d:%02d.%03d] ",
			pstTM->tm_year + 1900,
			pstTM->tm_mon + 1,
			pstTM->tm_mday,
			pstTM->tm_hour,
			pstTM->tm_min,
			pstTM->tm_sec,
			tb.millitm);
		break;

	case Error:
		iLogBuff = snprintf(szLogBuff, sizeof(szLogBuff),
			"[ERROR] [%d-%02d-%02d %02d:%02d:%02d.%03d] ",
			pstTM->tm_year + 1900,
			pstTM->tm_mon + 1,
			pstTM->tm_mday,
			pstTM->tm_hour,
			pstTM->tm_min,
			pstTM->tm_sec,
			tb.millitm);
		break;

	default:
		iLogBuff = snprintf(szLogBuff, sizeof(szLogBuff),
			"[0x%05x] [%d-%02d-%02d %02d:%02d:%02d.%03d] ",
			enLevel,
			pstTM->tm_year + 1900,
			pstTM->tm_mon + 1,
			pstTM->tm_mday,
			pstTM->tm_hour,
			pstTM->tm_min,
			pstTM->tm_sec,
			tb.millitm);
	}

	const int DateTimeMsLength = sizeof("[DEBUG] [2020-01-21 19:02:59.321]");

	//写记录体: 取变参
	va_list args;
	va_start(args, lpszFormat);
	int iRet = vsnprintf(szLogBuff + iLogBuff, (sizeof(szLogBuff) - DateTimeMsLength), lpszFormat, args);
	va_end(args);

	//累加长度
	iLogBuff += iRet;

	//--------------------------------------------------------

	m_Mutex.lock();  //---加锁

	if (m_pLogBuff)
	{
		iRet = iLogBuff + m_iLogBuff;
		if (iRet < LogBuffWarn)
		{
			//正常拷贝
			memcpy(m_pLogBuff + m_iLogBuff, szLogBuff, iLogBuff);
			m_iLogBuff = iRet;
		}
		else if (iRet < LogBuffOver)
		{
			//日志缓存满
			iRet = snprintf(szLogBuff + DateTimeMsLength, (sizeof(szLogBuff) - DateTimeMsLength), "CMyLog::writeLog()---> m_iLogBuff=%d, iLogBuff=%d; LogBuff overfllow ...\r\n", m_iLogBuff, iLogBuff);
			iLogBuff = DateTimeMsLength + iRet;

			//正常拷贝
			memcpy(m_pLogBuff + m_iLogBuff, szLogBuff, iLogBuff);
			m_iLogBuff += iLogBuff;

			//通知写文件线程
			m_Condition.notify_one();
		}
		else
		{
			//通知写文件线程
			m_Condition.notify_one();
		}
	}

	m_Mutex.unlock();  //---解锁
	
}


void CMyLog::writeLog(LOG_LEVEL enLevel, const wchar_t *lpwszFormat, ...)
{
    if(enLevel < m_enLogLevel)
    {
        return;
    }

    //取系统时间
	char szDateTime[64] = { 0 };
	struct timeb tb;
	ftime(&tb);

	//转换成本地时间
	struct tm *pstTM = localtime(&tb.time);
	if (NULL == pstTM)
	{
		return;
	}

	//日志行缓存
	char szLogBuff[LogLineSize] = { 0 };
	int iLogBuff = 0;

	//写记录头: [Level][时:分:秒.毫秒]
	switch (enLevel)
	{
	case Timer:
		iLogBuff = snprintf(szLogBuff, sizeof(szLogBuff),
			"[TIMER] [%d-%02d-%02d %02d:%02d:%02d.%03d] ",
			pstTM->tm_year + 1900,
			pstTM->tm_mon + 1,
			pstTM->tm_mday,
			pstTM->tm_hour,
			pstTM->tm_min,
			pstTM->tm_sec,
			tb.millitm);
		break;

	case Debug:
		iLogBuff = snprintf(szLogBuff, sizeof(szLogBuff),
			"[DEBUG] [%d-%02d-%02d %02d:%02d:%02d.%03d] ",
			pstTM->tm_year + 1900,
			pstTM->tm_mon + 1,
			pstTM->tm_mday,
			pstTM->tm_hour,
			pstTM->tm_min,
			pstTM->tm_sec,
			tb.millitm);
		break;

	case Info:
		iLogBuff = snprintf(szLogBuff, sizeof(szLogBuff),
			"[INFO ] [%d-%02d-%02d %02d:%02d:%02d.%03d] ",
			pstTM->tm_year + 1900,
			pstTM->tm_mon + 1,
			pstTM->tm_mday,
			pstTM->tm_hour,
			pstTM->tm_min,
			pstTM->tm_sec,
			tb.millitm);
		break;

	case Warn:
		iLogBuff = snprintf(szLogBuff, sizeof(szLogBuff),
			"[WARN ] [%d-%02d-%02d %02d:%02d:%02d.%03d] ",
			pstTM->tm_year + 1900,
			pstTM->tm_mon + 1,
			pstTM->tm_mday,
			pstTM->tm_hour,
			pstTM->tm_min,
			pstTM->tm_sec,
			tb.millitm);
		break;

	case Error:
		iLogBuff = snprintf(szLogBuff, sizeof(szLogBuff),
			"[ERROR] [%d-%02d-%02d %02d:%02d:%02d.%03d] ",
			pstTM->tm_year + 1900,
			pstTM->tm_mon + 1,
			pstTM->tm_mday,
			pstTM->tm_hour,
			pstTM->tm_min,
			pstTM->tm_sec,
			tb.millitm);
		break;

	default:
		iLogBuff = snprintf(szLogBuff, sizeof(szLogBuff),
			"[0x%05x] [%d-%02d-%02d %02d:%02d:%02d.%03d] ",
			enLevel,
			pstTM->tm_year + 1900,
			pstTM->tm_mon + 1,
			pstTM->tm_mday,
			pstTM->tm_hour,
			pstTM->tm_min,
			pstTM->tm_sec,
			tb.millitm);
	}

	const int DateTimeMsLength = sizeof("[DEBUG] [2020-01-21 19:02:59.321]");

	//-----------------------------------------------------------------------------

	//wchar_t日志行缓存
	wchar_t wszLogBuff[LogLineSize] = { 0 };

	//写记录体: 取变参
	va_list args;
	va_start(args, lpwszFormat);
	int iRet = vsnwprintf(wszLogBuff, ((sizeof(wszLogBuff) / sizeof(wchar_t)) - 1), lpwszFormat, args);
	va_end(args);

	if (iRet > 0)
	{
		//置终止符
		wszLogBuff[iRet] = L'\0';

		//wchar_t转成chare
		//setlocale(LC_ALL, "");
		iRet = (int)wcstombs((szLogBuff + iLogBuff), wszLogBuff, (sizeof(szLogBuff)-iLogBuff));
		//setlocale(LC_ALL, "C");

		//累加长度
		iLogBuff += iRet;
	}

	//--------------------------------------------------------

	m_Mutex.lock();  //---加锁

	if (m_pLogBuff)
	{
		iRet = iLogBuff + m_iLogBuff;
		if (iRet < LogBuffWarn)
		{
			//正常拷贝
			memcpy(m_pLogBuff + m_iLogBuff, szLogBuff, iLogBuff);
			m_iLogBuff = iRet;
		}
		else if (iRet < LogBuffOver)
		{
			//日志缓存满
			iRet = snprintf(szLogBuff + DateTimeMsLength, (sizeof(szLogBuff) - DateTimeMsLength), "CMyLog::writeLog()---> LogBuff overfllow ...\r\n");
			iLogBuff = DateTimeMsLength + iRet;

			//正常拷贝
			memcpy(m_pLogBuff + m_iLogBuff, szLogBuff, iLogBuff);
			m_iLogBuff += iLogBuff;

			//通知写文件线程
			m_Condition.notify_one();
		}
		else
		{
			//通知写文件线程
			m_Condition.notify_one();
		}
	}

	m_Mutex.unlock();  //---解锁

}

void CMyLog::setLogLevel(LOG_LEVEL enLevel)
{
    m_enLogLevel = enLevel;
}

