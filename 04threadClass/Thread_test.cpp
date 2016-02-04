#include "Thread.h"
#include <boost/bind.hpp>
#include <unistd.h>
#include <iostream>

using namespace std;

class Foo
{
public:
    Foo(int count) : count_(count)
    {
    }
    void MemberFunc()
    {
        while (count_--) {
            cout << "this is a test ..." << endl;
            sleep(1);
        }
    }
    int count_;
};

void ThreadFunc()
{
    cout << "ThreadFunc ..." << endl;
}

void ThreadFunc2(int count)
{
    cout << "ThreadFunc ..." << count << endl;
}

int main(void)
{
    Thread t1(ThreadFunc);
    Thread t2(boost::bind(ThreadFunc2, 3));
    t1.Start();
    t1.Join();
    t2.Start();
    t2.Join();

    Foo foo(5);
    Thread t3(boost::bind(&Foo::MemberFunc, &foo));
    t3.Start();
    t3.Join();

    return 0;
}
