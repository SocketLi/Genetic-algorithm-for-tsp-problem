#include"tsp.h"
#include<wchar.h>
#include<cmath>
#include<set>
#include<assert.h>
bool IsVecHasRepeat(const vector<uint32_t>* vec) {
	std::set<uint32_t> help;
	for (uint32_t i = 0; i < vec->size()-1; ++i) {
		if (help.find(vec->at(i)) == help.end()) {
			help.insert(vec->at(i));
		}
		else {
			return false;
		}
	}
	return true;
}
Tsp::Tsp()
{
    Chromosome=new vector<ChromosomeInfo*>();
    IsInit=false;
}
Tsp::~Tsp()
{
    for(unsigned int i=0;i<TspCities.size();++i){
        delete TspCities[i];
    }
    FreeChromosome();
}
void Tsp::FreeChromosome()
{
    for(unsigned int i=0;i<Chromosome->size();++i){
        delete Chromosome->at(i);
    }
    delete Chromosome;
}
vector<ChromosomeInfo*>* Tsp::GetChromosome()
{
    return Chromosome;
}
int Tsp::Init(const char* FileName)//文件里数据第一项经度，第二项北纬，分隔符一个空格
{
    fstream DataFile;
    DataFile.open(FileName,std::ios::in);
    if(!DataFile.is_open()){
        cerr<<"Open data file error"<<endl;
        return ERROR;
    }
    string FileLine;
    uint32_t CityId=0;
    vector<string> SplitResults;
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> Conv;//提供对中文城市名称的支持
    while(getline(DataFile,FileLine)){//文件内容编码为utf8
        if(FileLine.empty()){
            continue;
        }
        SplitResults.clear();
        wstring CityData=Conv.from_bytes(FileLine); //wstring存储需要utf16编码的数据，这里需要做转换
        wstring Delimiter=L" ";
        wstring::size_type DelimiterPos;
        while((DelimiterPos=CityData.find_first_of(Delimiter))!=CityData.npos){//根据空格进行字符串切分，获取城市名称与经纬度
            string SubData=Conv.to_bytes(CityData.substr(0,DelimiterPos));
            CityData=CityData.substr(DelimiterPos+1,CityData.length()-DelimiterPos-1);
            SplitResults.push_back(SubData);
        }
        SplitResults.push_back(Conv.to_bytes(CityData));//将utf16编码的wstring转化为utf8编码的string进行存储
        if(SplitResults.size()!=DATA_NUMS){ //检查一下文件的格式对不对，即是不是读了3个信息
            cerr<<"Data file format error!"<<endl;
            return ERROR;
        }//构造cityInfo
        CityInfo *pCityInfo=new CityInfo(CityId,SplitResults[0],std::stod(SplitResults[2].c_str(),NULL),std::stod(SplitResults[1].c_str(),NULL));
        TspCities.push_back(pCityInfo);
        CityId++;
    }
    GenInitPopulation();//初始化种群
    if (InitThreads()!=SUCCESS){
        cerr<<"Init threads error"<<endl;
        return ERROR;
    }
    return SUCCESS;
}
int Tsp::InitThreads()
{
    IsInit=true;
    return SUCCESS;
}
void Tsp::GenInitPopulation()
{//使用抽牌法生成随机序列，方法是建立一个包含1到CityNum-1编号的数组（0为开始与结束城市）
//每次从数组中随机抽一个数，放到NewChromosome->CityPath中，然后把原数组的该数删除
    uint32_t CityNum=TspCities.size();
    vector<uint32_t> CityIdSet;
    srand(time(NULL));
    for(uint32_t i=0;i<INIT_POPULATION_SIZE;++i){
        uint32_t RemainNum=CityNum-1;
        ChromosomeInfo* NewChromosome=new ChromosomeInfo();
        for(uint32_t j=1;j<CityNum;++j){
            CityIdSet.push_back(j);
        } 
        NewChromosome->CityPath->push_back(GetStartCityId());
        for(uint32_t j=1;j<CityNum;++j){
            uint32_t CityNo=rand()%RemainNum;//数组下标
            NewChromosome->CityPath->push_back(CityIdSet[CityNo]);
            CityIdSet.erase(CityIdSet.begin()+CityNo);
            RemainNum--;
        }
        NewChromosome->CityPath->push_back(GetStartCityId());
        //assert(IsVecHasRepeat(NewChromosome->CityPath));校验生成的合法性，测试使用
        Chromosome->push_back(NewChromosome);
    }
}
int Tsp::GetStartCityId()
{
    return START_CITY;
}
double Tsp::GetCityDistance(uint32_t City1,uint32_t City2)//通过经纬度计算城市间距离，参数为城市id,这种算法只适用于北纬
{   
    double LatA=(90-TspCities[City1]->Latitude)*M_PI/180,LatB=(90-TspCities[City2]->Latitude)*M_PI/180;
    double LotA=TspCities[City1]->Longitude*M_PI/180,LotB=TspCities[City2]->Longitude*M_PI/180;
    return R*acos((sin(LatA)*sin(LatB)*cos(LotA-LotB)+cos(LatA)*cos(LatB)));
}
void Tsp::CalculateFitness()
{
    for(unsigned int i=0;i<Chromosome->size();++i){
        double FitnessScore=0;
        for(unsigned int j=0;j<Chromosome->at(i)->CityPath->size()-1;++j){
            FitnessScore+=GetCityDistance(Chromosome->at(i)->CityPath->at(j),Chromosome->at(i)->CityPath->at(j+1));
        }
        Chromosome->at(i)->Fitness=FitnessScore;
    }
}
void Tsp::Mutation(ChromosomeInfo* Child)
{
    if((rand()%INT32_MAX)<INT32_MAX*MUTATION_PROBABILITY){
        uint32_t PathLength=Child->CityPath->size();
        uint32_t SwapLeft,SwapRight; //首先生成随机交换的两个城市的id
        do//不能改变第一个和最后一个城市
        {
            do
            {
                SwapLeft=rand()%PathLength;
            } while (!SwapLeft || SwapLeft==PathLength-1);
            do
            {
                SwapRight=rand()%PathLength;
            } while (!SwapRight || SwapRight==PathLength-1);
        } while (SwapLeft==SwapRight);//id不能一样
        std::swap(Child->CityPath->at(SwapLeft),Child->CityPath->at(SwapRight));
    }
}
double Tsp::GetTspResult()
{
    if(!IsInit){
        cerr<<"Tsp class not init!"<<endl;
        return -1;
    }
    for(uint32_t i=0;i<GENERATION_NUM;++i){
        CalculateFitness();//计算适应度
        vector<ChromosomeInfo*>* NextGeneration=new vector<ChromosomeInfo*>();
        for(uint32_t j=0;j<PARTENT_NUM;++j){
            uint32_t ParentId1=SelectParent(),ParentId2=SelectParent();//选择
            CrossOver(ParentId1,ParentId2,NextGeneration);//交叉
            //变异,最新生成的两个子代在NextGeneration的后两个
            Mutation(NextGeneration->at(NextGeneration->size()-1));
            Mutation(NextGeneration->back());
        }
        FreeChromosome();//释放空间
        Chromosome=NextGeneration;
    }
    CalculateFitness();
    std::sort(Chromosome->begin(),Chromosome->end(),[=](const ChromosomeInfo* lhs,const ChromosomeInfo* rhs){
        return lhs->Fitness < rhs->Fitness;
    });//排序
    return Chromosome->at(0)->Fitness;
}
/*
   /*cout<<"最短路径为:";
    for(uint32_t i=0;i<Chromosome->at(0)->CityPath->size();++i){
        uint32_t CityId=Chromosome->at(0)->CityPath->at(i);
        cout<<TspCities[CityId]->CityName;
        if(i!=Chromosome->at(0)->CityPath->size()-1){
            cout<<"->";
        }
    }
cout<<endl;*/
ChromosomeInfo* Tsp::MakeChild(uint32_t ParentId1,uint32_t ParentId2,uint32_t CutBegin,uint32_t CutEnd,const unordered_map<uint32_t,uint32_t>& DeduplicationMap)
{//ParentId1是CutBegin和CutEnd之间部分将要被ParentId2的CutBegin和CutEnd之间的部分替换的那个parent
    ChromosomeInfo* Child=new ChromosomeInfo(); 
    auto FindBeginIt=Chromosome->at(ParentId2)->CityPath->begin();
    std::unordered_set<uint32_t> CutRangeCities;//使用hash set存储交换区信息，提高查找效率
    for(auto it=FindBeginIt+CutBegin;it!=FindBeginIt+CutEnd;++it){
        CutRangeCities.insert(*it);
    }
    for(uint32_t i=0;i<CutBegin;++i){ //生成child1在CutBegin之前的部分
        auto it=CutRangeCities.find(Chromosome->at(ParentId1)->CityPath->at(i));
        if(it!=CutRangeCities.end()){//解决冲突,it返回child1里已有的元素值，注意it指向的是parent2
            //对去重表进行查找来解决冲突
            auto DeduplicationIt=DeduplicationMap.find(*it);//去重表迭代器
            auto ParentIt=CutRangeCities.find(DeduplicationIt->second);//指向parent2中重复元素的迭代器
            while (ParentIt!=CutRangeCities.end()){ //循环判断去重是否成功
                DeduplicationIt=DeduplicationMap.find(*ParentIt);
                ParentIt=CutRangeCities.find(DeduplicationIt->second);
            }
            Child->CityPath->push_back(DeduplicationIt->second);//去重成功
        }
        else{//不存在冲突
            Child->CityPath->push_back(Chromosome->at(ParentId1)->CityPath->at(i));
        }
    }
    for(uint32_t i=CutBegin;i<CutEnd;++i){//cutbegin和cutend之间的部分交叉
        Child->CityPath->push_back(Chromosome->at(ParentId2)->CityPath->at(i));
    }
    for(uint32_t i=CutEnd;i<Chromosome->at(ParentId1)->CityPath->size();++i){ //生成child1在CutEnd之后的部分，生成方法与上面同
        auto it=CutRangeCities.find(Chromosome->at(ParentId1)->CityPath->at(i));
        if(it!=CutRangeCities.end()){
            auto DeduplicationIt=DeduplicationMap.find(*it);
            auto ParentIt=CutRangeCities.find(DeduplicationIt->second);
            while (ParentIt!=CutRangeCities.end()){ 
                DeduplicationIt=DeduplicationMap.find(*ParentIt);
                ParentIt=CutRangeCities.find(DeduplicationIt->second);
            }
            Child->CityPath->push_back(DeduplicationIt->second);
        }
        else{//不存在冲突
            Child->CityPath->push_back(Chromosome->at(ParentId1)->CityPath->at(i));
        }
    }
    return Child;
}
void Tsp::CrossOver(uint32_t ParentId1,uint32_t ParentId2,vector<ChromosomeInfo*>* NextGeneration)
{
    uint32_t PathLength=Chromosome->at(ParentId1)->CityPath->size();
    uint32_t CutBegin,CutEnd; //Begin和end左开右闭，与stl相同
    do
    {
        do//不要包含第一个开始城市
        {
            CutBegin=rand()%PathLength;
        } while (!CutBegin);

        do
        {
            CutEnd=rand()%PathLength;
        } while (!CutEnd);
    } while (CutBegin==CutEnd);//切割起点和终点不能相同
    if(CutEnd<CutBegin){
        std::swap(CutBegin,CutEnd);
    }
    unordered_map<uint32_t,uint32_t> Child1DeduplicationMap,Child2DeduplicationMap;
    //child1要查的去重表是parent2里CutBegin和CutEnd之间数据为key,parent1里CutBegin和CutEnd之间数据为value
    //child2的表则反过来
    for(uint32_t i=CutBegin;i<CutEnd;++i){
        Child1DeduplicationMap[Chromosome->at(ParentId2)->CityPath->at(i)]=Chromosome->at(ParentId1)->CityPath->at(i);
        Child2DeduplicationMap[Chromosome->at(ParentId1)->CityPath->at(i)]=Chromosome->at(ParentId2)->CityPath->at(i);
    }
    ChromosomeInfo* child1=MakeChild(ParentId1,ParentId2,CutBegin,CutEnd,Child1DeduplicationMap);
    ChromosomeInfo* child2=MakeChild(ParentId2,ParentId1,CutBegin,CutEnd,Child2DeduplicationMap);
    NextGeneration->push_back(child1);
    NextGeneration->push_back(child2);
    //assert(IsVecHasRepeat(child1->CityPath));
    //assert(IsVecHasRepeat(child2->CityPath));
}
uint32_t Tsp::SelectParent()
{
    //在种群中随机挑选两个个体
    return TournamentSelection(rand()%INIT_POPULATION_SIZE,rand()%INIT_POPULATION_SIZE);
}
uint32_t Tsp::TournamentSelection(uint32_t ChromosomeId1,uint32_t ChromosomeId2)
{
    //Fitness是哈密顿回路的长度，越小越好
    return Chromosome->at(ChromosomeId1)->Fitness < Chromosome->at(ChromosomeId2)->Fitness ? ChromosomeId1 : ChromosomeId2;
}
