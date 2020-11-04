#include "buffer.h"

keyType clearKey={"\0","\0"};  //空键
extern HeaderPage* head;
extern bPlusTreePtr root;
extern FILE* fp;
extern FILE* dt;

/*
 * setWriteBit - 将节点的isWrite位置为true
 */
void setWriteBit(bPlusTreePtr ptr) {
    ptr->isWrite = true;
}

/*
 * initializeNewNode - 创建一个空的B+树节点，同时会将这个新创建的节点填入缓存池
 */ 
bPlusTreePtr initializeNewNode(){  //创建一个空的新节点
    int i;
    bPlusTreePtr newNode=(bPlusTreePtr)malloc(sizeof(bPlusTreeNode));
    if(newNode==NULL){
        printf("error:create new bPlusTreeNode fail!!\n");
        exit(-1);
    }
    for(int i=0;i<maxChrNum;i++){
        newNode->Children[i]=-1;
        newNode->Key[i]=clearKey;
    }
    newNode->Children[maxChrNum]=-1;
    newNode->Parent=-1;
    newNode->isLeaf=false;
    setWriteBit(newNode);
    newNode->me=addIntoBuffer(newNode,-1);
    return newNode;
}

/*
 * initialization - 初始化B+树，主要工作在于创建头部记录B+树的信息
 */
off_t initialization(){  //第一次建立索引，创建头节点并写入磁盘
    head=(HeaderPage*)malloc(sizeof(HeaderPage));
    head->BlockSize=sizeof(bPlusTreeNode); //直接先定好每一个节点和记录的size
    head->TupleSize=sizeof(DataType); 
    head->TupleNum=0;
    head->NodeNum=0;
    head->FreeBlockNum=0;
    head->rootPos=-1;  //还未建树，不存在root，每次将head写回到磁盘的时候都会更新这一项防止漏写
    head->fl=NULL;

    dt=fopen(DataPath,"wb+");

    //顺便也创建一下root,记得把root加入缓冲池，因为root会变，最后来做会导致最初的root没有写出到磁盘的情况
    root=initializeNewNode();
    root->isLeaf=true;
    //将这个头节点写入磁盘
    fp=fopen(TreeDataPath,"wb+");

    off_t successWrite=fwrite(head,sizeof(HeaderPage),1,fp);
    //fclose(fp);
    return successWrite;
}

/*
 * workStart - 每次程序启动，使用B+树之前，调用本函数读出B+树的头部和根节点
 */
void workStart(){  //每一次启动时从磁盘读入头节点和根节点
    if(head==NULL){  //head!=null表明执行过了initialization
        head=(HeaderPage*)malloc(sizeof(HeaderPage));
        root=(bPlusTreePtr)malloc(sizeof(bPlusTreeNode));
        dt=fopen(DataPath,"rb+");
        fp=fopen(TreeDataPath,"rb+");
        rewind(fp);
        fread(head,sizeof(HeaderPage),1,fp);
        if(ferror(fp))
            perror("error:");
        fseek(fp,head->rootPos,SEEK_SET);
        fread(root,sizeof(bPlusTreeNode),1,fp);
        
        addIntoBuffer(root,root->me);
    }
    //初始化缓冲池
    BfPool.BufferLen=0;
    BfPool.BufferHead=NULL;
    BfPool.BufferTail=NULL;
}

/*
 * GetTreeNode - 根据文件偏移量，首先遍历缓存池查看节点是否已经读入内存
 * 如果是，则直接返回节点的内存地址
 * 如果不是，则从文件中将节点读入缓存池
 */
bPlusTreePtr GetTreeNode(off_t blkOffset){
    if(blkOffset==-1)   return NULL;
    //遍历缓冲池，查看对应节点是否在内存中,如果在直接返回指针，如果不在就到磁盘中去取
    Buffer* iterator=BfPool.BufferHead;
    while(iterator!=NULL){
        if(blkOffset==iterator->BlockOffset){
            return iterator->ptr;
        }
        iterator=iterator->next;
    }
    return ReadIntoBuffer(blkOffset);
}


/*
 * workEnd - 本次B+树使用结束，将缓存池中的B+树节点以及头部写回文件
 */
void workEnd(){  //本次索引使用结束，进行收尾工作(写回缓冲池和头节点)
    Buffer* tmp=BfPool.BufferHead;
    while(tmp!=NULL){
        if(tmp->ptr==root) //最后结束时要更新root节点的偏移量到head块中，下次才能正确读出
            head->rootPos=tmp->BlockOffset;
        //if(tmp->ptr->isWrite) //只有被修改的才需要被写回磁盘
            //printf("flightNum=%s\n",tmp->ptr->Key[0]);
            WriteBackToDisk(tmp);
        Buffer* del=tmp;
        tmp=tmp->next;
        BfPool.BufferLen--;
        free(del);
    }
    //head要最后写回
    fseek(fp,0,SEEK_SET);//由于文件指针一直在被使用，不知道当前指向的偏移量是多少，所以要重新指向初始位
    fwrite(head,sizeof(HeaderPage),1,fp);
    free(head);
    fclose(fp);
    fclose(dt);
}


/* 
 * InsertTuple - 将一条记录写入文件中
 */
off_t InsertTuple(DataType* dPtr){  //将一条记录写入磁盘
    off_t Offset=head->TupleSize*head->TupleNum;
    head->TupleNum++;
    fseek(dt,Offset,SEEK_SET);
    fwrite(dPtr,sizeof(DataType),1,dt);
    return Offset;
}


/*
 * cmpKey - 比较主键大小
 */
int cmpKey(keyType a,keyType b){  //比较搜索码
    int cmp=strcmp(a.flightNumber,b.flightNumber);
    if(cmp==0){ 
        return strcmp(a.flightTime,b.flightTime);
    }else{
        return cmp;
    }
}

/*
 * writeNode - 将指针信息以及主键信息写入某个B+树节点
 */
void writeNode(bPlusTreePtr node,keyType value,off_t ptr,int KeyIndex,int PtrIndex){ //写键和Children
    if(KeyIndex!=-1)
        node->Key[KeyIndex]=value;
    if(PtrIndex!=-1)
        node->Children[PtrIndex]=ptr;
    setWriteBit(node);  //设置脏位
}

/*
 * getChildPtr - 得到指定节点的子节点指针
 */
bPlusTreePtr getChildPtr(bPlusTreePtr parentPtr,int index){
    return GetTreeNode(parentPtr->Children[index]);
}   

/*
 * getChildKey - 得到指定节点的子节点的主键
 */
keyType getChildKey(bPlusTreePtr parentPtr,int index){
    return parentPtr->Key[index];
}

/*
 * writeParent - 修改某个节点的父节点指针(文件偏移量)
 */
void writeParent(bPlusTreePtr child,off_t parent){
    child->Parent=parent;
    setWriteBit(child);  //设置脏位
}

/*
 * getParent - 得到某个节点的父节点指针（文件偏移量）
 */
bPlusTreePtr getParent(bPlusTreePtr child){
    return GetTreeNode(child->Parent);
}

/*
 * getChildIndex - 从parentPtr节点中，找到满足以下条件的键的下标：
 * mode->true 找到第一个大于value的键或是空键
 * mode->false 找到等于value的键
 */
int getChildIndex(bPlusTreePtr parentPtr,keyType value,bool mode){  
    int i=0;                                                     
    while(1){                                                    
        keyType tmpKey=getChildKey(parentPtr,i);
        if(strcmp(tmpKey.flightNumber,"\0")==0){  //从这个结点之后都是空值,没必要再搜索
            if(!mode)  
                i=-1;
            break;
        }
        int cmpAns=cmpKey(tmpKey,value);
        if(cmpAns>0){ 
            if(!mode) //对于mode->false模式下，这种情况意味着不存在与value相等的键
                i=-1;
            break;
        }
        else if(cmpAns==0){
            if(mode) //对mode->true模式，相等时需要再次自增i
                i++;
            break;
        }
        else{
            i++;
            if(i==maxChrNum-1){  //对于mode->true,前面的所有key都小于value，则应该使用节点中最后一个指针
                if(!mode)  //对于mode->false,找不到与value相等的键
                    i=-1;
                break;
            }
        }
    }
    return i;
}

/*
 * getChildNum - 返回子节点个数
 */
int getChildNum(bPlusTreePtr node){  
    int i;
    int num=0;
    for(i=0;i<maxChrNum;i++){
        if(node->Children[i]!=-1) //改！
            num++;
    }
    return num;
}

/*
 * checkLeaf - 判断该节点是否叶节点
 */
bool checkLeaf(bPlusTreePtr node){ 
    return node->isLeaf;
}


/*
 * Find - 给定主键，返回对应的叶节点
 */
bPlusTreePtr Find(keyType value,int* ansIndex){  //ansIndex返回叶节点中第一个>=value的值的下标
    bPlusTreePtr tmpPtr=root;
    while(!checkLeaf(tmpPtr)){ //非叶节点                                              //结构指针我还是希望以return的方式返回，尽量少使用二重指针
        int index=getChildIndex(tmpPtr,value,true);     //找到包含对应搜索码的子节点
        tmpPtr=getChildPtr(tmpPtr,index);
    }
    //tmpPtr是叶节点
    *ansIndex=getChildIndex(tmpPtr,value,false);  //找到叶节点中value的下标
    return tmpPtr;
}


/*
 * Insert - 给定新节点的主键和数据偏移量，将它们插入B+树
 */
void Insert(keyType value,off_t data){ 
    int valueIndex;
    bPlusTreePtr tmpPtr;
    tmpPtr=Find(value,&valueIndex);//找到应该插入的叶节点  tip:如果根结点为叶节点，且内容为空？

    int keyNum=getChildNum(tmpPtr);

    if(tmpPtr->Children[maxChrNum-1]==-1){  
        keyNum++;
    }
    if(keyNum<maxChrNum){  //如果叶节点的key未满(最后一个指针在一开始是不存在的，这种情况要特判)
        insertInLeaf(tmpPtr,value,data);  //会将指向下一个兄弟节点的指针偏移到空闲位
        writeNode(tmpPtr,clearKey,tmpPtr->Children[maxChrNum],maxChrNum-1,maxChrNum-1);  //将指向下一个兄弟节点的指针移到正确位置
        writeNode(tmpPtr,clearKey,-1,maxKeyNum,maxChrNum);
    }
    else{  //叶节点已满，需要分裂
        int mid=minKeyNum;
        bPlusTreePtr newNode=initializeNewNode();
        newNode->isLeaf=true;
        insertInLeaf(tmpPtr,value,data); //其中存在一个临时存储的Key和data，与上面的insertInleaf意义不一样
        for(int i=mid;i<maxChrNum;i++){
            off_t tmpOffset=tmpPtr->Children[i];  
            writeNode(newNode,getChildKey(tmpPtr,i),tmpOffset,i-mid,i-mid);
            writeNode(tmpPtr,clearKey,-1,i,i);
        }
        writeNode(newNode,clearKey,tmpPtr->Children[maxChrNum],-1,maxChrNum-1); //将指向下一个兄弟节点的指针移到正确位
        writeNode(tmpPtr,clearKey,-1,-1,maxChrNum);  //清空中间存储单元
        writeNode(tmpPtr,clearKey,newNode->me,-1,maxChrNum-1); //tmpPtr指向新的兄弟newNode
        writeParent(newNode,tmpPtr->Parent);
        keyType minKey=getChildKey(newNode,0);

        insertInParent(tmpPtr,minKey,newNode);
    }
}

/*
 * insertInLeaf - 将新节点的主键与数据偏移量插入叶节点（只被Insert函数调用）
 */
void insertInLeaf(bPlusTreePtr leaf,keyType value,off_t data){  //这里data使用的是off_t !!! 
    int i;
    bPlusTreePtr tmpPtr=NULL;           //保存最后一个孩子指针
    writeNode(leaf,clearKey,leaf->Children[maxChrNum-1],-1,maxChrNum);
    if(cmpKey(value,getChildKey(leaf,0))==0 || cmpKey(getChildKey(leaf,0),clearKey)==0){  //插到最前面 tips：这时候是不是应该修改父节点中的数据？
        for(i=maxChrNum-1;i>0;i--){    //会先占用指向下一个节点的指针(即最后一个指针)，在函数调用前已经将它赋给新节点，所以不会出错
            writeNode(leaf,getChildKey(leaf,i-1),leaf->Children[i-1],i,i);
        }
        writeNode(leaf,value,data,0,0);
    }
    else{
        int index=getChildIndex(leaf,value,true);  //找到可插入位置
        for(i=maxChrNum-1;i>index;i--){   //如果这个位置不是空位，则后续结点向后挪
           writeNode(leaf,getChildKey(leaf,i-1),leaf->Children[i-1],i,i);
        }
        writeNode(leaf,value,data,index,index);
    }
}

/* 
 * ptrIndexInParent - 给定一个B+树节点和一个偏移量，返回该偏移量在B+树节点中的文件偏移量（指针）数组中的下标
 */
int ptrIndexInParent(bPlusTreePtr parentPtr,off_t target){  //对于非叶节点而言，插入位置的寻找方法跟叶节点不同
    int index;
    for(int i=0;i<maxChrNum;i++){
        if(parentPtr->Children[i]==target){  //插入oldNode指针之后
            index=i;
            break;
        }
    }
    return index;
}

/*
 * insertInParent - 插入新节点时，由于某个节点中的子节点过多，需要发生分裂，由此需要改变上一级父节点的信息
 * （只被Insert函数调用）
 */
void insertInParent(bPlusTreePtr oldNode,keyType value,bPlusTreePtr newNode){
    if(oldNode->Parent==-1){  //如果oldNode是根节点,则创建新的根节点
        root=initializeNewNode();
        writeNode(root,value,oldNode->me,0,0);
        writeNode(root,clearKey,newNode->me,1,1);
        writeParent(oldNode,root->me);
        writeParent(newNode,root->me);
        return;
    }
    bPlusTreePtr p=getParent(oldNode);  
    if(getChildNum(p)<maxChrNum){ //父节点还有空位
        int index=ptrIndexInParent(p,oldNode->me);

        for(int i=maxKeyNum;i>index+1;i--){   //如果这个位置不是空位，则后续结点向后挪
           writeNode(p,getChildKey(p,i-1),p->Children[i-1],i,i); 
        }
        writeNode(p,getChildKey(p,index),-1,index+1,-1);  //在index位处只后移Key
        writeNode(p,value,newNode->me,index,index+1);  //key放在index位，ptr放在index+1位
    }
    else{  //父节点无空位，分裂父节点
        bPlusTreePtr newBrother=initializeNewNode();
        int mid=minKeyNum;
        int i;
        int index=ptrIndexInParent(p,oldNode->me); //确定插入位
        if(index!=maxChrNum-1){
            writeNode(p,clearKey,p->Children[maxChrNum-1],-1,maxChrNum); //临时存储
            for(i=maxKeyNum;i>index+1;i--){   //如果这个位置不是空位，则后续结点向后挪
               writeNode(p,getChildKey(p,i-1),p->Children[i-1],i,i);
            }
            writeNode(p,getChildKey(p,index),-1,index+1,-1);  //同上
        }
        writeNode(p,value,newNode->me,index,index+1); 

        keyType minKey=getChildKey(p,mid);
        writeNode(p,clearKey,-1,mid,-1);//newBrother下属的最小的key作为不出现在newBrother中，而是出现在newBrother的父节点中 
        for(i=mid+1;i<=maxKeyNum;i++){    //分裂
            bPlusTreePtr ptr=getChildPtr(p,i);
            writeNode(newBrother,getChildKey(p,i),p->Children[i],i-mid-1,i-mid-1);  
            writeNode(p,clearKey,-1,i,i);
            writeParent(ptr,newBrother->me);  //移动的节点要改变它的父指针
        }
        bPlusTreePtr ptr=getChildPtr(p,maxChrNum);
        writeParent(ptr,newBrother->me);
        writeNode(newBrother,clearKey,p->Children[maxChrNum],-1,maxChrNum-mid-1);
        writeNode(p,clearKey,-1,-1,maxChrNum);
        writeParent(newBrother,p->Parent);
        insertInParent(p,minKey,newBrother);
        //父节点中在p和brother之间的key的取法不一样
    }
}

/*
 * Delete - 给定主键，将对应数据从B+树删除
 */
bool Delete(keyType value){
    int ansIndex=0;
    bPlusTreePtr leaf=Find(value,&ansIndex);
    if(ansIndex==-1){ //不存在要删除的key
        printf("the tuple with key{%s,%s} is not exist!\n",value.flightNumber,value.flightTime);
        return false;
    }
    deleteEntry(leaf,ansIndex,ansIndex);
    printf("delete key{%s,%s} success!\n",value.flightNumber,value.flightTime);
    return true;
}

/*
 * deleteKey - 将某个B+树节点的主键数组中，下标为index的主键删除
 */
void deleteKey(bPlusTreePtr node,int index){
    for(int i=index;i<maxKeyNum;i++){
        writeNode(node,getChildKey(node,i+1),-1,i,-1);
    }
    writeNode(node,clearKey,-1,maxKeyNum,-1);
}

/*
 * deleteKey - 将某个B+树节点的文件偏移量数组中，下标为index的文件偏移量删除
 */
void deletePtr(bPlusTreePtr node,int index){
    int upbound=maxChrNum;
    if(node->isLeaf==true){  //叶节点最后一个指针不应被迁移
        upbound=maxChrNum-2;
    }
    for(int i=index;i<upbound;i++){
        writeNode(node,clearKey,node->Children[i+1],-1,i);
    }
    if(node->isLeaf==true){
        writeNode(node,clearKey,-1,-1,upbound);
    }
    writeNode(node,clearKey,-1,-1,maxChrNum);
}


/*
 * deleteEntry - delete操作的实际执行者，给定主键，将其数据信息从B+树中删除
 * （只会被delete调用）
 */
void deleteEntry(bPlusTreePtr node,int keyIndex,int ptrIndex){
    int i;                              //keyIndex和ptrIndex可能不一样（当node是非叶节点时）
    addIntoFreeBlockList(node,ptrIndex);
    deleteKey(node,keyIndex);
    deletePtr(node,ptrIndex);

    if(node->Parent==-1 && node->Children[1]==-1){ //node为根节点，且只剩1个子节点时
        if(node->isLeaf)    return;
        bPlusTreePtr p=getChildPtr(node,0);
        addIntoFreeBlockList(node,-1);
        writeParent(p,-1);
        root=p;
    }
    else if(node->Children[minChrNum-1]==-1){  //N中的子节点过少
        bPlusTreePtr p=getParent(node);   //父节点
        bPlusTreePtr brother;
        keyType midKey;
        ptrIndex=ptrIndexInParent(p,node->me);
        if(ptrIndex==0){           //找到兄弟节点
            brother=getChildPtr(p,1);
            midKey=getChildKey(p,0);
        }else{
            brother=getChildPtr(p,ptrIndex-1);
            midKey=getChildKey(p,ptrIndex-1);
        }

        int nChildNum=getChildNum(node);
        int bChildNum=getChildNum(brother);
        int ptrSum=nChildNum+bChildNum;
        if(node->isLeaf==true && node->Children[maxChrNum-1]!=-1 && brother->Children[maxChrNum-1]!=-1)
            ptrSum--;  //叶节点中的“指向下一个节点的指针”在此不应重复计算（对合并不影响）

        if(ptrSum<=maxChrNum){  //两个节点可合并
            if(ptrIndex==0){  //让brother指向前面的节点，node指向后面的节点（swap）,ptrIndex指向前者
                bPlusTreePtr tmp=node;
                node=brother;
                brother=tmp;

                int swap=nChildNum;
                nChildNum=bChildNum;
                bChildNum=swap;
            }
            else{
                ptrIndex--;
            }

            if(checkLeaf(node)){  //对叶节点
                for(int i=0;i<nChildNum;i++){  //合并节点
                    writeNode(brother,getChildKey(node,i),node->Children[i],i+bChildNum-1,i+bChildNum-1); //key数等于指针数减去最后一个指针（如果最后一个指针存在）
                }
                writeNode(brother,clearKey,node->Children[maxChrNum-1],-1,maxChrNum-1);
            }
            else{  //非叶节点
                writeNode(brother,midKey,-1,bChildNum-1,-1); 
                for(int i=0;i<nChildNum;i++){  //合并节点
                    writeNode(brother,getChildKey(node,i),node->Children[i],i+bChildNum,i+bChildNum);
                }
            }
            return deleteEntry(p,ptrIndex,ptrIndex+1);
        }
        else{  //两个节点不可合并，借一个索引项
            if(ptrIndex!=0){     //brother在前，将brother最后的key-Ptr插入node头部
                if(checkLeaf(node)){  //叶节点
                    if(bChildNum==maxChrNum){
                        bChildNum--;
                    }
                    keyType lastKey=getChildKey(brother,bChildNum-1);
                    off_t lastPtr=brother->Children[bChildNum-1];
                    writeNode(brother,clearKey,-1,bChildNum-1,bChildNum-1);

                    for(i=nChildNum;i>0;i--){  //node节点不可能满，所以不需要保护最后一个指针
                        writeNode(node,getChildKey(node,i-1),node->Children[i-1],i,i);
                    }
                    writeNode(node,lastKey,lastPtr,0,0);

                    writeNode(p,lastKey,-1,ptrIndex-1,-1); //修改父节点中的key
                }
                else{
                    keyType lastKey=getChildKey(brother,bChildNum-2);
                    off_t lastPtr=brother->Children[bChildNum-1];
        
                    writeNode(brother,clearKey,-1,bChildNum-2,bChildNum-1); //清空brother的对应key和指针
                    writeNode(node,clearKey,node->Children[nChildNum-1],-1,nChildNum);  
                    for(i=nChildNum-1;i>0;i--){ 
                        writeNode(node,getChildKey(node,i-1),node->Children[i-1],i,i);
                    }
                    writeNode(node,p->Key[ptrIndex-1],lastPtr,0,0);  //父节点的key摘下来

                    writeNode(p,lastKey,-1,ptrIndex-1,-1); //修改父节点的key
                }
            }
            else{  //node在前    将brother最前的key-Ptr添加到node最后
                keyType lastKey=getChildKey(brother,0);  //公共操作
                off_t lastPtr=brother->Children[0];
                deleteKey(brother,0);
                deletePtr(brother,0);
                if(checkLeaf(node)){  //叶节点
                    writeNode(node,lastKey,lastPtr,nChildNum-1,nChildNum-1);
                    writeNode(p,getChildKey(brother,0),-1,0,-1); //修改父节点中的key
                }
                else{   //非叶节点
                    writeNode(node,p->Key[0],lastPtr,nChildNum-1,nChildNum);
                    writeNode(p,lastKey,-1,0,-1); //修改父节点的key
                }
            }
        }
    }
}

/*
 * PrintTree - 打印整颗B+树
 */
void PrintTree(int depth,off_t start){
    bPlusTreePtr tmp=GetTreeNode(start);
    if(tmp!=NULL && tmp->Children[0]!=-1){
        for(int i=0;i<depth;i++){
            printf("  ");
        }
        int num=getChildNum(tmp);
        for(int i=0;i<num;i++){
            keyType k=getChildKey(tmp,i);
            if(tmp->Children[i]!=-1){
                printf("*");
            }
            if(cmpKey(clearKey,k)!=0){
                printf("{%s,%s}",k.flightNumber,k.flightTime);
            }
        }
        printf("\n");
        for(int i=0;i<num;i++){
            if(tmp->isLeaf==true)
                continue;
            PrintTree(depth+1,tmp->Children[i]);
        }
    }
}

/*
 * ReadTuple - 从文件中读出含有对应主键的记录
 */
DataType* ReadTuple(keyType k,DataType* ans){
    int index;
    bPlusTreePtr tmp=Find(k,&index);
    off_t Offset=tmp->Children[index];
    if(cmpKey(tmp->Key[index],k)!=0) //未找到指定数据
        return NULL;
    fseek(dt,Offset,SEEK_SET);
    fread(ans,sizeof(DataType),1,dt);
    if(ferror(dt))
        perror("error");
    return ans;
}