#include"tsp.h"
#include<pthread.h>
#include<semaphore.h>
#define THREAD_NUM 4 //线程
class PrallelTsp;
typedef struct tagWorkerThreadArgs
{
    int ThreadId;
    PrallelTsp* TspClass;
}WorkerThreadArgs; //线程参数
typedef struct tagCrossOverThreadInfo
{
    uint32_t Begin;
    uint32_t End;
    uint32_t ParentId1;
    std::unordered_set<uint32_t>* CutRangeCities;
    const unordered_map<uint32_t,uint32_t>* DeduplicationMap;
    vector<uint32_t> *CityPath;
}CrossOverThreadInfo;//CrossOver并行要用到的数据结构
class PrallelTsp:public Tsp
{
    public:
        ~PrallelTsp();
        int InitThreads();//线程初始化
        void CalculateFitness();//主从并行模型中的计算适应度函数
        //这两个函数都是对适应性评价过程进行优化，其优化效率相当，可任选其一作为改过程并行函数
        static void* ThreadCalculate(void* args);//按块划分，必须声明成static否则会被编译器隐式补上this指针做参数无法通过类型检查
        static void* ThreadCalculate1(void* args);//动态划分，类的this指针由args来传入
        //对Cross过程进行并行化
        static void* ThreadCrossOver(void* args);
        ChromosomeInfo* MakeChild(uint32_t ParentId1,uint32_t ParentId2,uint32_t CutBegin,uint32_t CutEnd,const unordered_map<uint32_t,uint32_t>& DeduplicationMap);
    private:
        pthread_barrier_t WorkerProcessBarrier,CrossOverBarrier;//前一个是主从并行模型要用到的屏障，后一个是CrossOver过程用的
        sem_t WorkerSem[THREAD_NUM+2];//用来唤醒线程的信号量
        pthread_mutex_t WorkerProcessMutex;//互斥锁
        CrossOverThreadInfo CrossOverInfo[2];//CrossOver并行使用
        int NextLine;
        bool ThreadExit;
};

