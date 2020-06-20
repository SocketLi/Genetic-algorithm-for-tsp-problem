#include"prallelTsp.h"
#include<sys/time.h>
int main()
{
    struct timeval start;
    struct timeval end;
    if(gettimeofday(&start,NULL)<0){
        cerr<<"get time err"<<endl;
        return 1;
    }
    Tsp* myTsp=new PrallelTsp();
    myTsp->Init("data1.txt");
    double Length=myTsp->GetTspResult();
    cout<<"最短路径的长度为:"<<Length<<"KM"<<endl;
    delete myTsp;
    if(gettimeofday(&end,NULL)<0){
        cerr<<"get time err"<<endl;
        return 1;
    }
    cout<<"并行算法的时间为:"<<(end.tv_sec-start.tv_sec)*1000+double(end.tv_usec-start.tv_usec)/1000<<"ms"<<endl;
    if(gettimeofday(&start,NULL)<0){
        cerr<<"get time err"<<endl;
        return 1;
    }
    /*Tsp* myTsp1=new Tsp();
    myTsp1->Init("data1.txt");
    Length=myTsp1->GetTspResult();
    cout<<"最短路径的长度为:"<<Length<<"KM"<<endl;
    delete myTsp;
    if(gettimeofday(&end,NULL)<0){
        cerr<<"get time err"<<endl;
        return 1;
    }
    cout<<"串行算法的时间为:"<<(end.tv_sec-start.tv_sec)*1000+double(end.tv_usec-start.tv_usec)/1000<<"ms"<<endl;*/
    return 0;
}