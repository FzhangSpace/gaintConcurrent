#include <muduo/base/Condition.h>

#include <errno.h>

bool muduo::Condition::waitForSeconds(int seconds)
{
    struct timespec abstime;

    clock_gettime(CLOCK_REALTIME, &abstime);
    abstime.tv_sec += seconds;
    MutexLock::UnassignGuard ug(mutex_);

    return ETIMEDOUT == pthread_cond_timedwait(&pcond_, mutex_.getPthreadMutex(), &abstime);
}
