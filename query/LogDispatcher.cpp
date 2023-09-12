#include <fstream>
#include <cstring> 
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
//#include <pthread.h>
#include "LogDispatcher.h"

using namespace std;

/////////////////////////init/////////////////
LogDispatcher::LogDispatcher()
{
	m_nServerHandle = 0;
	m_fileCnt =0;
	for (int itor = 0; itor < MAX_FILE_CNT; itor++)
	{
		m_logStores[itor] = NULL;
	}
}
LogDispatcher::~LogDispatcher()
{
	DisConnect();
	m_nServerHandle = 0;
	m_fileCnt =0;
}

int LogDispatcher::Connect(char* dirPath)
{
    DIR *d;
    struct dirent *file;
	if(dirPath == NULL || strlen(dirPath) <=3)
	{
		dirPath = DIR_PATH_DEFAULT;
	}
    if(!(d = opendir(dirPath)))
    {
		SyslogError("error dir path. path:%s.\n", dirPath);
        return -1;//open failed
    }
	int loadNum =0;
	int totalFilesNum =0;
	m_fileCnt =0;
    while((file = readdir(d)) != NULL)
    {
		if(strncmp(file->d_name, ".", 1) == 0 || strlen(file->d_name) < 3) continue;
		SyslogDebug("%s %s\n", dirPath, file->d_name);
		
		LogStoreApi* logStore = new LogStoreApi();
		loadNum = logStore->Connect(dirPath, file->d_name);
		if(loadNum > 0)
		{
			m_nServerHandle = 1;
			m_logStores[m_fileCnt++] = logStore;
			SyslogDebug("%d --load patterns success,load num:%d, path:%s/%s.\n", m_fileCnt-1, loadNum, dirPath, file->d_name);
		}
		else
		{
			SyslogError("path:%s/%s load failed, skipped already!\n", dirPath, file->d_name);
		}
		totalFilesNum++;
    }
    closedir(d);
	if(m_nServerHandle == 0)
	{
		SyslogError("error load logStore. path:%s.\n", dirPath);
		return 0;
	}
	printf("load logStore success,load num:%d/%d, path:%s.\n", m_fileCnt, totalFilesNum, dirPath);
	//CalRunningTime();
	return m_fileCnt;
}

int LogDispatcher::TraveDir(char* dirPath)
{
    DIR *d;
    struct dirent *file;
	if(dirPath == NULL || strlen(dirPath) <=3)
	{
		dirPath = DIR_PATH_DEFAULT;
	}
    if(!(d = opendir(dirPath)))
    {
		SyslogError("error dir path. path:%s.\n", dirPath);
        return -1;//open failed
    }
	int loadNum = 0;
    while((file = readdir(d)) != NULL)
    {
		if(strncmp(file->d_name, ".", 1) == 0 || strlen(file->d_name) < 3) continue;
		m_filequeue.push(file->d_name);
    }
    closedir(d);
	
	return 1;
}

int LogDispatcher::IsConnect()
{
	return m_nServerHandle;
}

void LogDispatcher::DisConnect()
{
	for (int itor = 0; itor < m_fileCnt; itor++)
	{
		if(m_logStores[itor] != NULL)
		{
			int status = m_logStores[itor]->DisConnect();
			if(status == 0)
			{
				//SyslogDebug("release patterns successful. %d\n", itor);
			}
			else
			{
				SyslogError("release patterns failed.\n");
			}
			m_logStores[itor] = NULL;
		}
	}
}

int LogDispatcher::CalRunningTime()
{
	RunningStatus runt;
	Statistics glb_stat;
	for (int itor = 0; itor < m_fileCnt; itor++)
	{
		RunningStatus t = m_logStores[itor] ->RunStatus;
		Statistics ss = m_logStores[itor]->Statistic;
		runt.LogMetaTime += t.LogMetaTime;
		m_runt.LoadDeComLogTime += t.LoadDeComLogTime;
		m_runt.SearchTotalTime += t.SearchTotalTime;
		m_runt.SearchPatternTime += t.SearchPatternTime;
		m_runt.SearchOutlierTime += t.SearchOutlierTime;
		m_runt.MaterializFulTime += t.MaterializFulTime;
		m_runt.MaterializAlgTime += t.MaterializAlgTime;
		runt.SearchTotalEntriesNum += t.SearchTotalEntriesNum;
		//SysCount("%s:%d \n", m_logStores[itor]->FileName.c_str(), t.SearchTotalEntriesNum);
		//runt.SearchOutliersNum += t.SearchOutliersNum;
		glb_stat.total_capsule_cnt += ss.total_capsule_cnt;
		glb_stat.total_decom_capsule_cnt += ss.total_decom_capsule_cnt;
		glb_stat.total_decom_capsule_time += ss.total_decom_capsule_time;
		glb_stat.total_queried_cap_cnt += ss.total_queried_cap_cnt;
		glb_stat.valid_cap_filter_cnt += ss.valid_cap_filter_cnt;
		glb_stat.hit_at_mainpat_cnt += ss.hit_at_mainpat_cnt;
		glb_stat.hit_at_subpat_cnt += ss.hit_at_subpat_cnt;
	}
	SysTotCount("\nLogMetaTime: %lf s\n", runt.LogMetaTime);
	SysInfo("LoadDeComLogTime: %lf s\n", m_runt.LoadDeComLogTime);
	SysTotCount("SearchTotalTime: %lf s\n", m_runt.SearchTotalTime);
	SysTotCount("SearchPatternTime: %lf s\n", m_runt.SearchPatternTime);
	SysTotCount("SearchOutlierTime: %lf s\n", m_runt.SearchOutlierTime);
	SysTotCount("MaterializFulTime: %lf s\n", m_runt.MaterializFulTime);
	SysInfo("MaterializAlgTime: %lf s\n", m_runt.MaterializAlgTime);
	
	SysInfo("SearchOutliersNum: %d\n", runt.SearchOutliersNum);
	SysTotCount("Thulr_CmdlSearchTotalEntriesNum: %d\n", runt.SearchTotalEntriesNum);
	SysTotCount("tot_decom_cap: %d\n", glb_stat.total_decom_capsule_cnt);
	SysTotCount("tot_check_cap: %d\n", glb_stat.total_queried_cap_cnt);
	SysTotCount("tot_valid_cap: %d\n", glb_stat.valid_cap_filter_cnt);
	SysTotCount("tot_cap: %d\n", glb_stat.total_capsule_cnt);
	SysTotCount("tot_decom_time: %lf\n", glb_stat.total_decom_capsule_time);
	SysTotCount("P2P time:%lf s\n", runt.LogMetaTime + m_runt.SearchTotalTime + m_runt.MaterializFulTime);
}	

int LogDispatcher::ResetRunningTime()
{
	m_runt.LoadDeComLogTime = 0;
	m_runt.SearchTotalTime = 0;
	//m_runt.SearchPatternTime = 0;
	//m_runt.SearchOutlierTime = 0;
	m_runt.MaterializFulTime = 0;
	m_runt.MaterializAlgTime = 0;
	//runt.SearchOutliersNum = 0;
}

//////////////////////SearchByWildcard///////////////////////////////

int LogDispatcher::SearchByWildcard(char *args[MAX_CMD_ARG_COUNT], int argCount)
{
	if(MAX_THREAD_PARALLEL == 1)
	{
		SearchByWildcard_Seq(args, argCount);
	}
	else
	{
		SearchByWildcard_Thread(args, argCount);
	}
	//rest time statistic
	ResetRunningTime();
}

int LogDispatcher::SearchByWildcard_Seq(char *args[MAX_CMD_ARG_COUNT], int argCount)
{
	int totalMatNum = MAX_MATERIAL_SIZE;
	int matnum =0;
	//printf("$$$$$$$\n");
	for (int itor = 0; itor < m_fileCnt; itor++)
	{
		
		LogStoreApi* logStore = m_logStores[itor];
		matnum = logStore->SearchByWildcard_Token(args, argCount, totalMatNum);
		totalMatNum -= matnum;
		
		//printf("tt: %d %d\n", totalMatNum, matnum);
	}
	CalRunningTime();
}



