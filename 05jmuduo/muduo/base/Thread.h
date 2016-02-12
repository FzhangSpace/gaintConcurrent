#ifndef MUDUO_BASE_THREAD_H
#define MUDUO_BASE_THREAD_H

#include <muduo/base/Atomic.h>
#include <muduo/base/Types.h>

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <pthread.h>

namespace muduo
{
class Thread : boost::noncopyable
{
public:
    typedef boost:funciton<void()> ThreadFunc;
    explicit Thread(const ThreadFunc&, const string& name = string());

#ifdef __GXX_EXPERIMENTAL_CXX0X__
    explicit Thread(ThreadFunc&&, const string& name = string());
#endif
    ~Thread();

    void start();
    int join(); // return pthread_join()

    bool started() const { return started_; }
    // pthread_t pthreadId() const { return pthreadId_; }
    pid_t tid() const { return *tid; }
    const string &name() const { return name_; }

    static int numCreated() { return numCreated_.get(); }

private:
    void setDefaultName();
    bool        started_;
    bool        joined_;
    pthread_t   pthreadId;
    boost::shared_ptr<pid_t> tid_;
    ThreadFunc  func_;
    string      name_;

    static AtomicInt32 numCreated_;
};

}
#endif // MUDUO_BASE_THREAD_H
