#include "msgjob.h"
#include "iostream"
#include <unistd.h> 
#include <fcntl.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h> 
using namespace std;

#define THIS_FILE "main.cpp"
extern pj_caching_pool g_cp;
void looprun();
FILE *logfd = NULL;
void logwrite(int level, const char *data, int len)
{
	if(logfd)
	{
		if(level <= pj_log_get_level())
		{
			fwrite(data,len,1,logfd);
			fflush(logfd);
		}
	}
}


int main(int argc, char **argv)
{

	if(argc != 4)
	{
		cout<<"参数错误"<<endl;
		cout<<"参数：监控的端口 日志文件 是否后台运行"<<endl;
		return -1;
	}
	pj_init();
	logfd = fopen(argv[2], "a+");
	pj_log_set_decor(pj_log_get_decor()|PJ_LOG_HAS_YEAR|PJ_LOG_HAS_MONTH|PJ_LOG_HAS_DAY_OF_MON);
	pj_log_set_log_func(logwrite);
	pj_log_set_level(3);

	
	int port = atoi(argv[1]);
	msgjob *mj = new msgjob;
	mj->monitorudp(port);
	mj->Execute();
	int isht= atoi(argv[3]);
	if(isht)
		daemon(1,0);
	PJ_LOG(5,(THIS_FILE, "参数:%d %s %d",port, argv[2], isht));
	looprun();
	cout<<"stop looprun"<<endl;
	msgjob::StopAll();
	if(logfd)
		fclose(logfd);
	return 0;
}
#define FIFO_NAME "/tmp/fifo_conference"
void looprun()
{
	char buf[PIPE_BUF+1];
	
	if(access(FIFO_NAME, F_OK) == -1)
	{
		int ret = mkfifo(FIFO_NAME, 0777);
		if(ret != 0)
		{
			PJ_LOG(3,(THIS_FILE, THIS_FILE, "创建fifo失败"));
			exit(-1);
		}
	}

	int fifo_fd = open(FIFO_NAME, O_RDWR);
	
	if(fifo_fd != -1)
	{
		while(1)
		{
			memset(buf, 0, PIPE_BUF);
			int	ret = read(fifo_fd, buf, PIPE_BUF);
			if(ret > 0)
			{
				buf[ret-1]='\0';
				pj_str_t cmd = pj_str(buf);
				pj_strtrim(&cmd);


				PJ_LOG(3,(THIS_FILE, "fifo read ret = %d  %s",ret, buf));
				if(pj_stricmp2(&cmd, "exit") == 0)
				{
					break;
				}
				else if(pj_stricmp2(&cmd, "uplog") == 0)
				{
					pj_log_set_level(pj_log_get_level()+1);
				}
				else if(pj_stricmp2(&cmd, "downlog") == 0)
				{
					pj_log_set_level(pj_log_get_level()-1);
				}
			}
			pj_thread_sleep(1000);
		}
		close(fifo_fd);
	}
	else
	{
		PJ_LOG(3,(THIS_FILE, THIS_FILE, "打开fifo 失败"));
		exit(-1);
	}	
}

