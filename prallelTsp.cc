#include"prallelTsp.h"
void* PrallelTsp::ThreadCalculate(void* args)
{
    WorkerThreadArgs* ThreadArgs=(WorkerThreadArgs*)args;
    PrallelTsp* TspClass=ThreadArgs->TspClass;
    pthread_t ThreadId=ThreadArgs->ThreadId;
    while(1){
        sem_wait(&(TspClass->WorkerSem[ThreadId]));//当CalculateFitness函数未被调用时，线程在自己的信号量上休眠
        if(TspClass->ThreadExit){
            break;
        }
        int ThreadLoad=INIT_POPULATION_SIZE/THREAD_NUM;//根据线程Id按块划分任务
        int BeginPos=ThreadId*ThreadLoad;
        int EndPos=ThreadId==THREAD_NUM-1 ? (ThreadId+1)*ThreadLoad : INIT_POPULATION_SIZE;
        vector<ChromosomeInfo*>* Chromosome=TspClass->GetChromosome();
        for(unsigned int i=BeginPos;i<EndPos;++i){//进行适应性评价过程
            double FitnessScore=0;
            for(unsigned int j=0;j<Chromosome->at(i)->CityPath->size()-1;++j){
                FitnessScore+=TspClass->GetCityDistance(Chromosome->at(i)->CityPath->at(j),Chromosome->at(i)->CityPath->at(j+1));
            }
            Chromosome->at(i)->Fitness=FitnessScore;
        }
        pthread_barrier_wait(&TspClass->WorkerProcessBarrier);//等待其他线程
    }
    pthread_exit(NULL);
}
void* PrallelTsp::ThreadCalculate1(void* args)
{
    WorkerThreadArgs* ThreadArgs=(WorkerThreadArgs*)args;
    PrallelTsp* TspClass=ThreadArgs->TspClass;
    pthread_t ThreadId=ThreadArgs->ThreadId;
    while(1){
        sem_wait(&TspClass->WorkerSem[ThreadId]);
        int Task=0;
        TspClass->NextLine=0;
        if(TspClass->ThreadExit){
            break;
        }
        vector<ChromosomeInfo*>* Chromosome=TspClass->GetChromosome();
        while (1){//动态任务划分，每个线程一次取一行进行适应性评价过程
            pthread_mutex_lock(&TspClass->WorkerProcessMutex);
            Task=TspClass->NextLine++;
            pthread_mutex_unlock(&TspClass->WorkerProcessMutex);
            if(Task>=INIT_POPULATION_SIZE){
                break;
            }
            double FitnessScore=0;
            for(unsigned int j=0;j<Chromosome->at(Task)->CityPath->size()-1;++j){
                FitnessScore+=TspClass->GetCityDistance(Chromosome->at(Task)->CityPath->at(j),Chromosome->at(Task)->CityPath->at(j+1));
            }
            Chromosome->at(Task)->Fitness=FitnessScore;
        }
        pthread_barrier_wait(&TspClass->WorkerProcessBarrier);
    }
    pthread_exit(NULL);
}
void* PrallelTsp::ThreadCrossOver(void* args)
{
    WorkerThreadArgs* ThreadArgs=(WorkerThreadArgs*)args;
    PrallelTsp* TspClass=ThreadArgs->TspClass;
    pthread_t ThreadId=ThreadArgs->ThreadId;
    while(1){
        sem_wait(&TspClass->WorkerSem[ThreadId+THREAD_NUM]);
        if(TspClass->ThreadExit){
            break;
        }
        vector<ChromosomeInfo*>* Chromosome=TspClass->GetChromosome();
        for(uint32_t i=TspClass->CrossOverInfo[ThreadId].Begin;i<TspClass->CrossOverInfo[ThreadId].End;++i){ //生成child1在CutBegin之前的部分
            auto it=TspClass->CrossOverInfo[ThreadId].CutRangeCities->find(Chromosome->at(TspClass->CrossOverInfo[ThreadId].ParentId1)->CityPath->at(i));
            if(it!=TspClass->CrossOverInfo[ThreadId].CutRangeCities->end()){//解决冲突,it返回child1里已有的元素值，注意it指向的是parent2
                //对去重表进行查找来解决冲突
                auto DeduplicationIt=TspClass->CrossOverInfo[ThreadId].DeduplicationMap->find(*it);//去重表迭代器
                auto ParentIt=TspClass->CrossOverInfo[ThreadId].CutRangeCities->find(DeduplicationIt->second);//指向parent2中重复元素的迭代器
                while (ParentIt!=TspClass->CrossOverInfo[ThreadId].CutRangeCities->end()){ //循环判断去重是否成功
                    DeduplicationIt=TspClass->CrossOverInfo[ThreadId].DeduplicationMap->find(*ParentIt);
                    ParentIt=TspClass->CrossOverInfo[ThreadId].CutRangeCities->find(DeduplicationIt->second);
                }
                TspClass->CrossOverInfo[ThreadId].CityPath->push_back(DeduplicationIt->second);//去重成功
            }
            else{//不存在冲突
                TspClass->CrossOverInfo[ThreadId].CityPath->push_back(Chromosome->at(TspClass->CrossOverInfo[ThreadId].ParentId1)->CityPath->at(i));
            }
        }
        pthread_barrier_wait(&TspClass->CrossOverBarrier);
    }
    pthread_exit(NULL);
}
ChromosomeInfo* PrallelTsp::MakeChild(uint32_t ParentId1,uint32_t ParentId2,uint32_t CutBegin,uint32_t CutEnd,const unordered_map<uint32_t,uint32_t>& DeduplicationMap)
{
    ChromosomeInfo* Child=new ChromosomeInfo(); 
    vector<ChromosomeInfo*>* Chromosome=GetChromosome();
    auto FindBeginIt=Chromosome->at(ParentId2)->CityPath->begin();
    std::unordered_set<uint32_t> CutRangeCities;//使用hash set存储交换区信息，提高查找效率
    for(auto it=FindBeginIt+CutBegin;it!=FindBeginIt+CutEnd;++it){
        CutRangeCities.insert(*it);
    }
    //主线程更新全局数据结构
    CrossOverInfo[0].Begin=0;
    CrossOverInfo[1].Begin=CutEnd;
    CrossOverInfo[0].End=CutBegin;
    CrossOverInfo[1].End=Chromosome->at(ParentId1)->CityPath->size();
    CrossOverInfo[0].ParentId1=CrossOverInfo[1].ParentId1=ParentId1;
    CrossOverInfo[0].CutRangeCities=CrossOverInfo[1].CutRangeCities=&CutRangeCities;
    CrossOverInfo[0].DeduplicationMap=CrossOverInfo[1].DeduplicationMap=&DeduplicationMap;
    CrossOverInfo[0].CityPath=new vector<uint32_t>();//子线程结果暂时存储在这里
    CrossOverInfo[1].CityPath=new vector<uint32_t>();
    for(int i=THREAD_NUM;i<THREAD_NUM+2;++i){
        sem_post(&WorkerSem[i]);//唤醒子线程
    }
    pthread_barrier_wait(&CrossOverBarrier);//同步操作
    for(uint32_t i=0;i<CutBegin;++i){//以下3个for循环为主线程合并计算结果，从而得到最终的子代序列
        Child->CityPath->push_back(CrossOverInfo[0].CityPath->at(i));
    }
    for(uint32_t i=CutBegin;i<CutEnd;++i){
        Child->CityPath->push_back(Chromosome->at(ParentId2)->CityPath->at(i));
    }
    for(uint32_t i=CutEnd;i<Chromosome->at(ParentId2)->CityPath->size();++i){
        Child->CityPath->push_back(CrossOverInfo[1].CityPath->at(i-CutEnd));
    }
    delete CrossOverInfo[0].CityPath;
    delete CrossOverInfo[1].CityPath;//释放空间
    return Child;
}
int PrallelTsp::InitThreads()
{
    if(pthread_barrier_init(&WorkerProcessBarrier,NULL,THREAD_NUM+1)<0){
        perror("Init thread barrier error!");
        return ERROR;
    }
    if(pthread_barrier_init(&CrossOverBarrier,NULL,2+1)<0){
        perror("Init thread barrier error!");
        return ERROR;
    }
    for(int i=0;i<THREAD_NUM;++i){ //初始化主从并行模型线程
        WorkerThreadArgs* ThreadArgs=new WorkerThreadArgs();
        pthread_t pid;
        ThreadArgs->TspClass=this;
        ThreadArgs->ThreadId=i;
        if(pthread_create(&pid,NULL,ThreadCalculate1,ThreadArgs)<0){
            perror("Create thread error!");
            return ERROR;
        }
        if(sem_init(&WorkerSem[i],0,0)<0){
            perror("Init sem error!");
            return ERROR;
        }
    } 
    for(int i=THREAD_NUM;i<THREAD_NUM+2;++i){//初始化CrossOver线程
        WorkerThreadArgs* ThreadArgs=new WorkerThreadArgs();
        pthread_t pid;
        ThreadArgs->TspClass=this;
        ThreadArgs->ThreadId=i-THREAD_NUM;//这两个线程的id分别为0，1
        if(pthread_create(&pid,NULL,ThreadCrossOver,ThreadArgs)<0){
            perror("Create thread error!");
            return ERROR;
        }
        if(sem_init(&WorkerSem[i],0,0)<0){//这两个线程等待的信号量在WorkerSem的第Thread_num和Thread_num+1处
            perror("Init sem error!");
            return ERROR;
        }
    }  
    IsInit=true;
    ThreadExit=false;
    WorkerProcessMutex=PTHREAD_MUTEX_INITIALIZER;
    NextLine=0;
    return SUCCESS;
}
void PrallelTsp::CalculateFitness()
{
    for(int i=0;i<THREAD_NUM;++i){
        sem_post(&WorkerSem[i]);
    }
    pthread_barrier_wait(&WorkerProcessBarrier);
}
PrallelTsp::~PrallelTsp()
{
    ThreadExit=true;
    for(int i=0;i<THREAD_NUM+2;++i){
        sem_post(&WorkerSem[i]);
    }
    pthread_barrier_destroy(&WorkerProcessBarrier);
    pthread_barrier_destroy(&CrossOverBarrier);
    for(int i=0;i<THREAD_NUM;++i){
        sem_destroy(&WorkerSem[i]);
    }
}