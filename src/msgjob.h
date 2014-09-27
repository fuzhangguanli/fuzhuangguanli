#include "job.h"

#ifndef MSG_JOB_H
#define MSG_JOB_H

class msgjob : public RegularJob
{
public:
	msgjob(){}
	~msgjob(){}

	virtual void Exec() ;

};


#endif
