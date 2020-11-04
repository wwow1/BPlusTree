## README

### 使用说明

对于使用者而言，提供三个函数接口如下：

```
bPlusTreePtr Find(keyType value,int* ansIndex); //查找
bool Delete(keyType value); //删除
void Insert(keyType value,off_t date);  //插入
```

其中keyType为主键类型，由航班号和飞行时间组成。

对于类型的进一步了解请自行查看bPlusTree.h。

数据信息保存在文件Data.txt中，B+树结构信息保存在TreeData.txt中。

用于初始化B+树的所有记录信息需要存放在input_data中，供程序自行读取并初始化。



### 代码说明

为了支持B+树在磁盘上的持久化存储，B+树内部的指针，都使用文件偏移量的形式描述，即父节点指向子节点的文件偏移量。

并且由于主键的特殊性，程序中没有考虑主键相同的情况。

每一次使用B+树需要调用workStart()初始化B+树，使用结束时需要调用workEnd()将内存中的B+树节点写回文件。