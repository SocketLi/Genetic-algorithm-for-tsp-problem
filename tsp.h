#ifndef TSP_H
#define TSP_H
#include<iostream>
#include<string>
#include<algorithm>
#include<vector>
#include<unordered_map>
#include<unordered_set>
#include<stdint.h>
#include<stdlib.h>
#include<stdio.h>
#include<fstream>
#include<locale>
#include <codecvt>
using std::string;
using std::wstring;
using std::vector;
using std::cout;
using std::endl;
using std::cerr;
using std::unordered_map;
#define SUCCESS 0
#define ERROR 1
#define INIT_POPULATION_SIZE 100 //种群数量
#define PARTENT_NUM INIT_POPULATION_SIZE/2 //每次选择的父母数量,每个父母生两个孩子
#define GENERATION_NUM 100 //代数
#define MUTATION_PROBABILITY 0.0005 //变异概率
#define START_CITY 0
#define DATA_NUMS 3
#define R 6371.004 //地球半径
using std::fstream;
using std::find;
typedef struct tagCityInfo
{
    uint32_t CityId;
    string CityName;
    double Latitude,Longitude; //纬度和经度
    //构造函数
    tagCityInfo(uint32_t pCityId,const string& pCityName,double pLatitude,double pLongtitude):CityId(pCityId),CityName(pCityName),Latitude(pLatitude),Longitude(pLongtitude){}
}CityInfo;
typedef struct tagChromosomeInfo//染色体信息
{
    vector<uint32_t>* CityPath;//城市路径
    double Fitness;//染色体适应度评分
    tagChromosomeInfo()
    {
        CityPath=new vector<uint32_t>();
        Fitness=0;
    }
    ~tagChromosomeInfo()
    {
        delete CityPath;
    }
}ChromosomeInfo;
class Tsp
{
    public:
        Tsp();
        ~Tsp();
        int Init(const char* FileName);//初始化函数，从文件中读取数据并按读取顺序为城市编号
        double GetTspResult();//计算函数
    private:
        void GenInitPopulation();//初始化种群
        int GetStartCityId();//获取开始城市信息
        void FreeChromosome();//释放空间函数
        virtual void CalculateFitness();//对染色体进行适应性评价的函数，设为virtual方便并行类覆写
        virtual int InitThreads();//线程初始化函数，在串行算法类中直接return
        uint32_t SelectParent();//选择函数
        uint32_t TournamentSelection(uint32_t ChromosomeId1,uint32_t ChromosomeId2);//竞赛选择算法实现，染色体的id是其在Chromosome数组中的编号
        void CrossOver(uint32_t ParentId1,uint32_t ParentId2,vector<ChromosomeInfo*>* NextGeneration);//交叉算法
        void Mutation(ChromosomeInfo* Child);//变异算法
        //父本母本交叉具体生成子代的方法
        virtual ChromosomeInfo* MakeChild(uint32_t ParentId1,uint32_t ParentId2,uint32_t CutBegin,uint32_t CutEnd,const unordered_map<uint32_t,uint32_t>& DeduplicationMap);
        vector<CityInfo*> TspCities;//存储城市信息的数组
        vector<ChromosomeInfo*>* Chromosome;//存储染色体信息的数组
    protected:
        vector<ChromosomeInfo*>* GetChromosome();//获取指向存储染色体信息的数组的指针
        double GetCityDistance(uint32_t City1,uint32_t City2);//通过两个城市的经纬度计算间距
        bool IsInit;//init函数是否被调用过
};
#endif