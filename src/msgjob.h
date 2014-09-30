#include "job.h"
#include <map>
#ifndef MSG_JOB_H
#define MSG_JOB_H

#define BUFLEN 1500
using namespace std;
class conference;
class msgjob : public RegularJob
{
public:
	msgjob():m_clearnum(0){}
	~msgjob(){}
	bool monitorudp(int port);
	void checkconfs();

	virtual void Exec() ;
	virtual void Stop() ;
private:
	pj_sock_t m_sock ;
	char m_buf[BUFLEN];
	map<pj_uint64_t , conference *> m_confs;
	int m_clearnum;

};


#endif
