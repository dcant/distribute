// the lock server implementation

#include "lock_server.h"

lock_info::lock_info()
{
	islocked = false;
	waitnum = 0;
	pthread_cond_init(&cond, NULL);
}

lock_info::~lock_info()
{
	pthread_cond_destroy(&cond);
}

lock_server::lock_server()
{
	pthread_mutex_init(&mut, NULL);
}

lock_server::~lock_server()
{
	map<int, lock_info*>::iterator it;
	pthread_mutex_lock(&mut);	//make sure that there is no one hold it.
	pthread_mutex_destroy(&mut);
	for(it = locks.begin(); it != locks.end(); it++)
	{
		pthread_cond_destroy(&it->second->cond);	//destroy every condition variable
		free(it->second);
}	}


int lock_server::acquire(int lid)
{
	int ret = 0;
	
	pthread_mutex_lock(&mut);
	map<int, lock_info*>::iterator it = locks.find(lid);
	if(it == locks.end())
	{
		lock_info *newlock = new lock_info();
		locks.insert(pair<int, lock_info*>(lid, newlock));
	}
	
	locks[lid]->waitnum++;
	
	while(locks[lid]->islocked == true)
	{
		pthread_cond_wait(&locks[lid]->cond, &mut);
	}
	
	locks[lid]->waitnum--;
	locks[lid]->islocked = true;
	
	pthread_mutex_unlock(&mut);
	return ret;
}

int lock_server::release(int lid)
{
	int ret = 0;

	pthread_mutex_lock(&mut);
	map<int, lock_info*>::iterator it = locks.find(lid);
	if(it != locks.end())
	{
		if(locks[lid]->islocked)
		{
			locks[lid]->islocked = false;
			if(locks[lid]->waitnum > 0)
				pthread_cond_signal(&locks[lid]->cond);
		}
	}

	pthread_mutex_unlock(&mut);
	return ret;
}
