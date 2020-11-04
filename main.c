#include "buffer.h"
/*
int main(){
    printf("node:%d\n",sizeof(bPlusTreeNode));
    int choice;
    int index;
    FILE* rd;
    DataType* d;
    DataType k;
    keyType KT;
    keyType kt;
    DataType ans;
    off_t Offset;
    rd=fopen("Flightdata.txt","r");
    initialization();
    printf("initialize B+ tree from file(flightdata).\n");
    while(fscanf(rd,"%s %s %s %s %s %d %d",k.flightNumber,k.flightTime,k.start,k.end,k.flyNum,&k.maxNum,&k.leaveNum)!=-1){
        off_t Offset=InsertTuple(&k);
        strcpy(KT.flightNumber,k.flightNumber);
        strcpy(KT.flightTime,k.flightTime);
        Insert(KT,Offset);
        printf("flightNumber:%s\n",k.flightNumber);
    }
    fclose(rd);
    workEnd();
    return 0;
}

*/

int main(){
    //初始化B+树
    int choice;
    int index;
    FILE* rd;
    DataType* d;
    DataType tmp;
    keyType kt;
    DataType ans;
    off_t Offset;
    clock_t start,end;
    workStart();
    scanf("%s",kt.flightNumber);
    scanf("%s",kt.flightTime);
    start=clock();
    d=ReadTuple(kt,&ans);
    if(d==NULL){//数据不存在
        printf("data not exists");
    }
    else{ 
        printf("%s,",d->flightNumber);
        printf("%s,",d->flightTime);
        printf("%s,",d->start);
        printf("%s,",d->end);
        printf("%s,",d->flyNum);
        printf("%d,%d,",d->maxNum,d->leaveNum);
    }
    end=clock();
    printf("%f",(double)(end-start)/CLOCKS_PER_SEC);
    workEnd();
    return 0;
}
