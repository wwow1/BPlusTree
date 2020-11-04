#ifndef bPlusTree_H //防止多次引入
#define bPlusTree_H

#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<stdbool.h>
#include <sys/types.h>
#include<time.h>
//#include <unistd.h>

#define n 119  //n=120->4096byte，决定每个B+树节点中主键数组和偏移量数组的大小，尽量使单个节点大小达到4KB
#define maxChrNum  n //最大子女数,根据键和指针的大小以及块的大小决定，保证每个节点都存在同一个块内
#define minChrNum  (n-1)/2+1 //最小子女数
#define maxKeyNum  n-1  //最大键数
#define minKeyNum  (n-2)/2+1  //最小键数
#define TreeDataPath "TreeData.txt"
#define DataPath    "Data.txt"


typedef struct BPlusTreeNode*  bPlusTreePtr;
typedef struct KeyType{
    char flightNumber[10]; //航班号
    char flightTime[20];    //飞行时间
}keyType;

typedef struct _data_{
    char start[20]; //始发站
    char end[20];   //终点站
    char flightNumber[10]; //航班号
    char flyNum[10];  //飞行号
    char flightTime[50];    //飞行时间
    int maxNum;    //乘员定额
    int leaveNum;  //余票量
}DataType;

typedef struct BPlusTreeNode{  //B+树节点类型
    bool isLeaf;    //是否叶节点
    keyType Key[maxKeyNum+1];    //子节点的主键数组，实际上一般只有maxKeyNum个，最后一个只在转移时临时使用
    off_t Children[maxChrNum+1];   //子节点的文件偏移量数组
    off_t Parent;    //父节点的文件偏移量
    off_t me;      //自己的文件偏移量
    bool isWrite;  //脏位
}bPlusTreeNode;

typedef struct Head{   //头部块
    int NodeNum;  //B+树总结点数
    int TupleNum;  //记录的数量，用于分配偏移量
    int BlockSize; //每个节点的大小
    int TupleSize; //每个记录的大小

    off_t rootPos;  //根节点的位置

    int FreeBlockNum;  //空闲块的数量
    struct _FreeBlockList_*  fl; //指向空闲块的链表
}HeaderPage;


bPlusTreePtr Find(keyType value,int* ansIndex); //查找
bool Delete(keyType value); //删除
void Insert(keyType value,off_t date);  //插入
void Find_and_print(char* flightNumber,char* flightTime);

void setWriteBit(bPlusTreePtr ptr);
bPlusTreePtr initializeNewNode();
extern bPlusTreePtr GetTreeNode(off_t blkOffset);
int cmpKey(keyType a,keyType b);
void writeNode(bPlusTreePtr node,keyType value,off_t ptr,int KeyIndex,int PtrIndex);
bPlusTreePtr getChildPtr(bPlusTreePtr parentPtr,int index);
keyType getChildKey(bPlusTreePtr parentPtr,int index);
void writeParent(bPlusTreePtr child,off_t parent);
bPlusTreePtr getParent(bPlusTreePtr child);
int getChildIndex(bPlusTreePtr parentPtr,keyType value,bool mode);
int getChildNum(bPlusTreePtr node);
bool checkLeaf(bPlusTreePtr node);
void insertInLeaf(bPlusTreePtr leaf,keyType value,off_t data);
void insertInParent(bPlusTreePtr oldNode,keyType value,bPlusTreePtr newNode);
int ptrIndexInParent(bPlusTreePtr parentPtr,off_t target);
void deleteKey(bPlusTreePtr node,int index);
void deletePtr(bPlusTreePtr node,int index);
void deleteEntry(bPlusTreePtr node,int keyIndex,int ptrIndex);
off_t InsertTuple(DataType* dPtr);
DataType* ReadTuple(keyType k,DataType* ans);
off_t initialization();
void workStart();
void workEnd();

#endif