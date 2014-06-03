#include <stdio.h>
#include <stdlib.h>
#include <map>
#include <pthread.h>

using namespace std;
class lock_info {
public:
	bool islocked;
	int waitnum;
	pthread_cond_t cond;

	lock_info();
	~lock_info();
};

class lock_server {

 protected:
  map<int, lock_info*> locks;
  pthread_mutex_t mut;

 public:
  lock_server();
  ~lock_server();

  int acquire(int);
  int release(int);
};
