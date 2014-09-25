//////////////////////////////////////////////////////////////////
//
// job.cxx
//
// Abstraction of threads' jobs
//
// Copyright (c) Citron Network Inc. 2002-2003
// Copyright (c) 2006-2010, Jan Willamowius
//
// This work is published under the GNU Public License version 2 (GPLv2)
// see file COPYING for details.
// We also explicitly grant the right to link this code
// with the OpenH323/H323Plus and OpenSSL library.
//
//////////////////////////////////////////////////////////////////

#include <list>
#include "job.h"
#include "singleton.h"
//#include "zlog.h"

// timeout (seconds) for an idle Worker to be deleted

/** This class represents a thread that performs jobs. It has two states:
    idle and busy. When it accepts a new Job, it becomes busy. When the job
    is finished it becomes idle. Each idle Worker is stopped (deleted) after
    the specified timeout, so Workers that are not needed anymore do not use
    system resources. This makes passible to create dynamic sets of Workers.
*/

//extern zlog_category_t *zc;
pj_caching_pool g_cp;
pj_pool_t * name::m_pool = NULL;
void jobinit()
{
	name::init();
}

void name::init()
{
	if(m_pool == NULL)
	{
		pj_caching_pool_init(&g_cp, NULL ,0);
		m_pool = pj_pool_create(&g_cp.factory,"jobpool",1024,1024,NULL);  
		PJ_LOG(5,("JOBLIB","pool == %p",m_pool));
	}
}
class Agent;
class PThread : public name
{
private:
	pj_thread_t *m_threadid;
public:
	PThread();
	virtual ~PThread();
	static int threadstart(void *);
	virtual void Main() = 0;
	void waitexit();
	int GetThreadId() { return (int)pj_getpid();}
};
class Worker : public PThread 
{
public:

	/// create a new Worker thread and start it immediately
	Worker(
		/// pointer to the Agent instance that is controlling this worker
		Agent* agent,
		/// timeout (seconds) for this Worker to be deleted, if idle
		long idleTimeout = DEFAULT_WORKER_IDLE_TIMEOUT
		);
	
	virtual ~Worker();

	/** Tell this Worker to execute a new Job. The function returns
		immediately and the job is executed under control of the Worker thread.
		After the job is finished, its object is deleted.
		
		@return
		true if this Worker is idle and has taken the Job, false otherwise 
		(on failuer the job object is not deleted).
	*/
	bool Exec(Job* job);
		
	/** Stop the Worker thread and any jobs being executed, 
	    wait for Worker thread termination and delete this object.
	*/
	void Destroy();

	// override from class PThread
	virtual void Main();
	
private:
	Worker();
	Worker(const Worker&);
	Worker& operator=(const Worker&);

private:
	/// idle timeout (seconds), after which the Worker is destoyed
	//PTimeInterval m_idleTimeout;
	/// signals that either a new Job is present or the Worker is destroyed
	PSyncPoint m_wakeupSync;
	/// true if the Worker is being destroyed
	volatile bool m_closed;
	/// for atomic job insertion and deletion
	PMutex m_jobMutex;
	/// actual Job being executed, NULL if the Worker is idle
	Job* volatile m_job;
	/// Worker thread identifier
	int m_id;
	/// Agent singleton pointer to avoid unnecessary Instance() calls
	Agent* m_agent;
};
/** Agent singleton manages a set of Worker threads. It creates
    new Workers if required. Idle Workers are deleted automatically
	after configured idle timeout.
*/

class Agent : public Singleton<Agent> 
{
public:
	Agent();
	virtual ~Agent();

	/** Execute the job by the first idle Worker or a new Worker.
		Delete the Job object after it is done.
	*/
	void Exec(Job * job);
		
	/** Remove the Worker from busy and idle lists. 
		Called by the Worker when it deletes itself.
	*/
	void Remove(Worker * worker);

	/** Move the Worker from the busy list to the idle list. 
		Called by the Worker when it finishes each job.
	*/
	void JobDone(
		/// the worker to be marked as idle
		Worker* worker
		);

private:
	Agent(const Agent&);
	Agent& operator=(const Agent&);
			
private:
	/// mutual access to Worker lists
	PMutex m_wlistMutex;
	/// list of idle Worker threads
	std::list<Worker*> m_idleWorkers;
	/// list of Worker threads executing some Jobs
	std::list<Worker*> m_busyWorkers;
	/// flag preventing new workers to be registered during Agent destruction
	volatile bool m_active;
};

PThread::PThread()
{
	pj_status_t rt = pj_thread_create(m_pool,"PThread",threadstart,this,1024*1024,0,&m_threadid);
	if(rt != PJ_SUCCESS)
	{
		PJ_LOG( 3,("PThread","create thread err: %d",rt));
	}
}
PThread::~PThread()
{
//	pthread_join(m_threadid,NULL);
	pj_thread_destroy(m_threadid);
}
int PThread::threadstart(void *arg)
{
	PThread *th = (PThread *)arg;
	th->Main();
	return 0;
}
void PThread::waitexit()
{
	pj_thread_join(m_threadid);
}

PMutex::PMutex():m_mutexid(NULL)
{
	PJ_LOG(5,("JOBLIB","pool == %p",m_pool));
	if(m_pool == NULL)
		init();
	pj_mutex_create_simple(m_pool,"PMutex",&m_mutexid);
}
PMutex::~PMutex()
{
	pj_mutex_destroy(m_mutexid);
}
int PMutex::lock()
{
	return pj_mutex_lock(m_mutexid);
}
int PMutex::unlock()
{
	return pj_mutex_unlock(m_mutexid);
}
PSyncPoint::PSyncPoint()
{
	if(m_pool == NULL)
		init();
	pj_event_create(m_pool,"PSyncPoint",PJ_FALSE,PJ_FALSE,&m_condid);	
}
PSyncPoint::~PSyncPoint()
{
	pj_event_destroy(m_condid);
}
int PSyncPoint::Wait(int timelen)
{
	//if(timelen <=0)
	//{
	//	pthread_cond_wait(&m_condid,&m_mutexid);
	//}	
	//else
	//{
	//	struct timespec tspec;
	//	tspec.tv_sec = time(NULL) + timelen;
	//	tspec.tv_nsec = 0;
	//	pthread_cond_timedwait(&m_condid,&m_mutexid,&tspec);
	//}
	return pj_event_wait(m_condid);
}
int PSyncPoint::Signal()
{
	return pj_event_set(m_condid);
	//pthread_cond_broadcast(&m_condid);
}
Worker::Worker(
	/// pointer to the Agent instance that the worker is run under control of
	Agent* agent,
	/// timeout (seconds) for this Worker to be deleted, if idle
	long idleTimeout
	) 
	:
	 m_closed(false), m_job(NULL), m_id(0),
	m_agent(agent)
{
	// resume suspended thread (and run Main)
}

Worker::~Worker()
{
	PWaitAndSignal lock(m_jobMutex);
	if (m_job) {
		PJ_LOG(3, ("Worker","JOB\tDestroying Worker %d with active Job ",m_id));
		delete m_job;
		m_job = NULL;
	}
	PJ_LOG(4, ("Worker","JOB\tWorker %d destroyed",m_id));
}

void Worker::Main()
{
	m_id = GetThreadId();
	PJ_LOG(4, ("Worker", "JOB\tWorker  %d  started",m_id));
	
	while (!m_closed) {
		bool timedout = false;
		// wait for a new job or idle timeout expiration
		if (m_job == NULL) {
			timedout = !m_wakeupSync.Wait(DEFAULT_WORKER_IDLE_TIMEOUT);
			if (timedout) {
				PJ_LOG(4, ("Worker", "JOB\tIdle timeout for Worker %d" , m_id));
			}
		}
		// terminate this worker if closed explicitly or idle timeout expired
		if (m_closed || (timedout && m_job == NULL)) {
			m_closed = true;
			break;
		}
		
		if (m_job) {
			PJ_LOG(4, ("Worker", "JOB\tStarting Job %s at Worker thread %d",m_job->getname(),m_id));

			m_job->Run();

			{
				PWaitAndSignal lock(m_jobMutex);
				delete m_job;
				m_job = NULL;
			}
			
			m_agent->JobDone(this);
		}
	}

	PJ_LOG(4, ("Worker", "JOB \tWorker %d closed",m_id));
	// remove this Worker from the list of workers
	m_agent->Remove(this);
	if (m_job) {

		PJ_LOG(4, ("Worker", "JOB\tActive Job left at closing Worker thread"));
	}
}

bool Worker::Exec(Job * job)
{
	// fast check if there is no job being executed
	if (m_job == 0 && !m_closed) {
		PWaitAndSignal lock(m_jobMutex);
		// check again there is no job being executed
		if (m_job == 0 && !m_closed) {
			m_job = job;
			m_wakeupSync.Signal();
			return true;
		}
	}
	return false;
}

void Worker::Destroy()
{
	// do not delete itself when the thread is stopped
	//SetNoAutoDelete();
	
	m_jobMutex.lock();
	if (m_job)
		m_job->Stop();
	m_jobMutex.unlock();
	m_closed = true;
	m_wakeupSync.Signal();

	waitexit();
	PJ_LOG(4, ("Worker", "JOB\tWaiting for Worker thread %d termination",m_id));
	//WaitForTermination(5 * 1000);	// max. wait 5 sec.
}


Agent::Agent() : Singleton<Agent>("Agent"), m_active(true)
{
}

Agent::~Agent()
{
	PJ_LOG(4, ("Agent", "JOB\tDestroying active Workers for the Agent"));

	std::list<Worker*> workers;
	int numIdleWorkers = -1;
	int numBusyWorkers = -1;
	
	{
		// move all workers to the local list
		PWaitAndSignal lock(m_wlistMutex);
		m_active = false;
		numIdleWorkers = m_idleWorkers.size();
		numBusyWorkers = m_busyWorkers.size();
		while (!m_busyWorkers.empty()) {
			workers.push_front(m_busyWorkers.front());
			m_busyWorkers.pop_front();
		}
		while (!m_idleWorkers.empty()) {
			workers.push_front(m_idleWorkers.front());
			m_idleWorkers.pop_front();
		}
	}

	PJ_LOG(4, ("Agent", "JOB\tWorker threads to cleanup: %d total - %d busy,%d idle",  (numBusyWorkers+numIdleWorkers) 
		,numBusyWorkers ,numIdleWorkers));

	std::list<Worker*>::iterator iter = workers.begin();
	while (iter != workers.end()) {
		Worker * w = *iter;
		workers.erase(iter++);
		w->Destroy();
#ifndef hasWorkerDeleteBug
		// TODO: find a proper fix for deleting workers on Windows!
		delete w;	// don't delete on Windows, issue with PTLib 2.10.1+
#endif
	}
	
	PJ_LOG(4, ("Agent", "JOB\tAgent and its Workers destroyed"));
}

void Agent::Exec(Job * job)
{
	Worker* worker = NULL;
	int numIdleWorkers = -1;
	int numBusyWorkers = -1;
	// pop the first idle worker and move it to the busy list	
	if (job) {
		PWaitAndSignal lock(m_wlistMutex);
		// delete the job if the Agent is being destroyed
		if (!m_active) {
			PJ_LOG(4, ("Agent", "JOB\tAgent did not accept Job ") );
			delete job;
			job = NULL;
			return;
		}
		if (!m_idleWorkers.empty()) {
			worker = m_idleWorkers.front();
			m_idleWorkers.pop_front();
			m_busyWorkers.push_front(worker);
			numIdleWorkers = m_idleWorkers.size();
			numBusyWorkers = m_busyWorkers.size();
		}
	} else
		return;
	
	bool destroyWorker = false;
		
	// if no idle worker has been found, create a new one 
	// and put it on the list of busy workers
	if (worker == NULL) {
		worker = new Worker(this);
		PWaitAndSignal lock(m_wlistMutex);
		if (m_active)
			m_busyWorkers.push_front(worker);
		else
			destroyWorker = true;
		numIdleWorkers = m_idleWorkers.size();
		numBusyWorkers = m_busyWorkers.size();
	}
	
	// execute the job by the worker
	if (!(m_active && worker->Exec(job))) {
		// should not ever happen, but...
		delete job;
		job = NULL;
		PWaitAndSignal lock(m_wlistMutex);
		m_busyWorkers.remove(worker);
		if (m_active)
			m_idleWorkers.push_front(worker);
		else
			destroyWorker = true;
		numIdleWorkers = m_idleWorkers.size();
		numBusyWorkers = m_busyWorkers.size();
	}


	if (destroyWorker) {
		PJ_LOG(4, ("Agent", "JOB\tAgent did not accept Job ") );
		worker->Destroy();
	}
}

void Agent::Remove(Worker* worker)
{
	int numIdleWorkers;
	int numBusyWorkers;
	{
		PWaitAndSignal lock(m_wlistMutex);
		// check both lists for the worker
		m_idleWorkers.remove(worker);
		m_busyWorkers.remove(worker);
		numIdleWorkers = m_idleWorkers.size();
		numBusyWorkers = m_busyWorkers.size();
	}
}

void Agent::JobDone(
	/// the worker to be marked as idle
	Worker* worker
	)
{
	int numIdleWorkers;
	int numBusyWorkers;
	{
		PWaitAndSignal lock(m_wlistMutex);
		m_busyWorkers.remove(worker);
		if (m_active)
			m_idleWorkers.push_front(worker);
		numIdleWorkers = m_idleWorkers.size();
		numBusyWorkers = m_busyWorkers.size();
	}
}


Task::~Task()
{
}

Job::~Job()
{
}

void Job::Execute()
{
	Agent::Instance()->Exec(this);
}

void Job::Stop()
{
}

void Job::StopAll()
{
	delete Agent::Instance();
}


void Jobs::Run()
{
	while (m_current) {
		m_current->Exec();
		m_current = m_current->DoNext();
	}
}


RegularJob::RegularJob() : m_stop(false)
{
}

void RegularJob::OnStart()
{
}

void RegularJob::OnStop()
{
}

void RegularJob::Run()
{
	OnStart();
	
	while (!m_stop)
		Exec();
		
	// lock to allow a member function that is calling Stop
	// return before OnStop is called and the object is deleted
	PWaitAndSignal lock(m_deletionPreventer);
	OnStop();
}

void RegularJob::Stop()
{
	// signal stop flag and wake up job thread, if it is in the waiting state
	m_stop = true;
	m_sync.Signal();
}
