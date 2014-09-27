#include "msgjob.h"
#include "iostream"
using namespace std;

int main(int argc, char **argv)
{
	pj_init();
	pj_log_set_level(5);

	msgjob *mj = new msgjob;
	mj->Execute();
	
	pj_thread_sleep(5000);
	msgjob::StopAll();
	cout<<"hello word"<<endl;
	return 0;
}


