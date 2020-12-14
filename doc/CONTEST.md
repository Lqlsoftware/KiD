### 赛题
Tair是阿里云自研的云原生内存数据库，接口兼容开源Redis/Memcache。在阿里巴巴集团内和阿里云上提供了缓存服务和高性能存储服务，追求极致的性能和稳定性。全新英特尔® 傲腾™ 数据中心级持久内存重新定义了传统的架构在内存密集型工作模式，具有突破性的性能水平，同时具有持久化能力，Aliyun弹性计算服务首次（全球首家）在神龙裸金属服务器上引入傲腾持久内存，撘配阿里云官方提供的Linux操作系统镜像Aliyun Linux，深度优化完善支持，为客户提供安全、稳定、高性能的体验。本题结合Tair基于神龙非易失性内存增强型裸金属实例和Aliyun Linux操作系统，探索新介质和新软件系统上极致的持久化和性能。参赛者在充分认知Aep硬件特质特性的情况下设计最优的数据结构。
### 赛题描述
热点是阿里巴巴双十一洪峰流量下最重要的一个问题，双十一淘宝首页上的一件热门商品的访问TPS在千万级别。这样的热点Key，在分布式的场景下也只会路由到其中的一台服务器上。解决这个问题的方法有Tair目前提供的热点散列机制，同时在单节点能力上需要做更多的增强。  
本题设计一个基于傲腾持久化内存(Aep)的KeyValue单机引擎，支持Set和Get的数据接口，同时对于热点访问具有良好的性能。  
### 初赛评测逻辑
评测程序会调用参赛选手的接口，启动16个线程进行写入和读取数据，最终统计读写完指定数目所用的时间，按照使用时间的从低到高排名。  
评测分为2个阶段：  
1）程序正确性验证，验证数据读写的正确性（复赛会增加持久化能力的验证），这部分的耗时不计入运行时间的统计  
2）初赛性能评测  
引擎使用的内存和持久化内存限制在 4G Dram和 74G Aep。  
每个线程分别写入约48M个Key大小为16Bytes，Value大小为80Bytes的KV对象，接着以95：5的读写比例访问调用48M次。其中95%的读访问具有热点的特征，大部分的读访问集中在少量的Key上面。  
### 复赛评测逻辑
复赛要求实现一个可持久化的高性能数据库，引擎使用的内存和持久化内存限制在8G Dram和 64G Aep，复赛要求数据具有持久化和可恢复(Crash-Recover)的能力，确保在接口调用后，即使掉电的时候依然能保证数据的完整性和正确恢复。

复赛评测分为三个阶段

1）正确性评测

   1. 验证数据读写的正确性，提供运行日志。这部分的耗时不计入运行时间的统计。
   2. 本阶段会开启16个线程并发写入一定量Key大小为16Bytes，Value大小范围为80-1024Bytes的KV对象，并验证读取和更新后的正确性。

2）持久化评测

   1. 评测程序会使用工具记录写操作与持久化操作，并在随机时刻模拟掉电情形。选手需保证已写入的数据在恢复后不受影响。
   2. 本环节~~不提供日志~~，提供评测程序中给出的部分日志。因此[评测程序](https://code.aliyun.com/qinwu.qw/judge-persistence)会在比赛开始前放出。
   3. 为评测的公平性考虑，评测程序的随机KV生成部分代码不给出。
   4. 在模拟的持久化内存设备上运行评测程序的结果未知，因此本地运行结果仅供参考，一切以评测机结果为准。
   5. 本环节时长~~90s~~160s，如果超时，希望选手优化自己的回复部分实现。

3）性能评测

   1. 本阶段首先会开启16个线程并发调用24M次Set操作，写入Key大小为16Bytes，Value大小范围为80-1024Bytes的KV对象，并选择性读取验证；接着会进行10次读写混合测试，取最慢一次的结果作为成绩，每次会开启16个线程以75%：25%的读写比例调用24M次。其中75%的读访问具有热点的特征，大部分的读访问集中在少量的Key上面。最后的分数为纯写入操作的耗时与最慢一次读写混合操作耗时的和。
   2. 本阶段提供运行日志，并会覆盖正确性评测的日志。
   3. 数据安排如下
      1. 本阶段保证任意时刻数据的value部分长度和不超过50G。
      2. 纯写入的24M次操作中
         1. 大约55%的操作Value长度在80-128Bytes之间；
         2. 大约25%的操作Value长度在129-256Bytes之间；
         3. 大约15%的操作Value长度在257-512Bytes之间；
         4. 大约5%的操作Value长度在513-1024Bytes之间；
         5. 总体数据写入量大约在75G左右。
      3. 读写混合的24M操作中，所有Set操作的Value长度均不超过128Bytes。



### 赛题资料
[PMEM编程指北](docs/PMEM编程指北.md)

[Intel 傲腾持久化内存介绍](https://software.intel.com/content/www/us/en/develop/videos/overview-of-the-new-intel-optane-dc-memory.html)

[持久化内存编程系列](https://software.intel.com/content/www/us/en/develop/videos/the-nvm-programming-model-persistent-memory-programming-series.html)

[如何模拟持久化内存设备(必看)](https://software.intel.com/en-us/articles/how-to-emulate-persistent-memory-on-an-intel-architecture-server)

[基于持久化内存的HelloWorld例子(libpmem)](https://software.intel.com/content/www/us/en/develop/articles/code-sample-create-a-c-persistent-memory-hello-world-program-using-libpmem.html)

[基于持久化内存的HelloWorld例子(mmap)](https://code.aliyun.com/db_contest_2nd/tair-contest/blob/master/docs/appdirect-tips.md)

### 赛题规则

##### 1.   语言限定

C/C++

##### 2.   程序目标

请仔细阅读tair_contest/include/db.hpp代码了解接口定义，通过实现子类NvmEngine（在tair_contest/nvm_engine目录下），参赛者通过编写代码，完善NvmEngine来实现自己的kv存储引擎；

##### 3.   参赛方法说明

1）在天池平台完成报名流程；  
2）在code.aliyun.com注册一个账号，Clone比赛的框架代码, 实现NvmEngine类中相关函数的实现；  
3)  将db_contest_2nd添加为Reporter, 用于评测程序拉取选手代码。  
4）在天池提交成绩的入口，提交自己fork的仓库git地址。  
5）等待评测结果。  

注：首次提交代码时，首先在天池页面点击“提交结果”->“修改地址”，在弹出的窗口中“git路径”一栏不需要填写“git@code.aliyun.com:”，只需填写自己仓库名即可，即：“USERNAME/tair-contest.git”
##### 4.   排名规则
在正确性验证通过的情况下，对性能评测阶段整体计时，根据总用时从低到高进行排名（用时越短排名越靠前）。

##### 5.   作弊说明

如果发现有作弊行为，比如通过hack评测程序，绕过了必须的评测逻辑等行为，则程序无效，且取消参赛资格。

### FAQ

##### 提交example代码有分吗？

> 没有，甚至连正确性验证都过不了，example只是一个example。甚至还有bug。

##### 赛题中的数据计算单位是?

> 1024

> 64GB = 64 * 1024 * 1024 * 1024 Byte

> 24M = 24 * 1024 * 1024

##### 能在本地进行持久化验证测试吗？

> 本地测试与评测机不一致通常是由于本地无法保证及时将数据刷回磁盘，如果需要在本地进行测试，需要自己在结束前手动调用pmem_msync。

##### 持久化验证测试的要求是？

> 我们将Set操作分为Insert和Update两类

> 对于Insert
   - 已经返回的key，我们要求断电后一定可以读到数据。
   - 尚未返回的key，我们要求数据写入做到原子性，在恢复后要么返回NotFound，要么返回正确的value。

> 对于Update
   - 已经返回的key，我们要求断电后一定可以读到新数据。
   - 尚未返回的key，我们要求更新做到原子性，在恢复后要么返回旧的value，要么返回新的value。

##### pmem_memcpy_persist等函数是持久化的吗？

> 请参考[pmem](https://pmem.io/pmdk/manpages/linux/master/libpmem/libpmem.7.html)文档。


> 持久性测试中未对类似函数hook是因为设置了PMEM_NO_MOVNT与export PMEM_NO_GENERIC_MEMCPY两个环境变量，此时pmem_memcpy_persist等同于连续调用memcpy, pmem_flush, pmem_drain。



##### 为什么我使用了pmem_persist等函数还是没有通过持久性测试？

> pmem_persist仅提供了对应部分数据的持久化，并不能保证全局的数据结构依旧完整。程序员需要以正确的顺序分步进行持久化以保证在任何时候断电都不会影响旧数据的安全和新数据的一致。

##### 纯写入阶段有重复的Key吗？

> 存在一定量的更新操作，并且更新前后Value大小**不相同**。

##### 我感觉我的AEP不够用，是否是数据排布出了问题？

> 纯写入阶段总写入量约为75G，除去Key部分约有20G的数据会被更新覆盖。选手需要对这部分AEP进行回收。

##### 读写混合是重复的Key吗？

> 全部是更新操作，均是重复的key。

##### Performance Judge Failed会有哪些原因？

> 性能测试在纯写阶段有万分之一左右的抽样，读写混合阶段中也有一定比例的测试，测试不通过会返回Assertion Failed。

> 性能测试有900的时间上限，超过会返回Time Limit Error。

> 除此之外，OOM，segmentation fault等会导致程序异常退出的均会返回Performance Judge Failed。

##### 如何输出Log

> 引擎接口里有log_file 文件指针，允许输出log的测试中该指针不为空，可直接写入到此文件中，大小限制在5MB以内。
将每5MB清空一次log文件。

##### Log文件显示Correctness Failed 或 Performance Judge Failed是什么意思。

> 正确性或性能评测时失败。具体失败原因有很多，需要自行排查。

##### 评测失败是否反馈具体原因？例如OOM，Core等。

> 初赛不反馈程序意外退出的原因，仅反馈评测失败。

> 除此以外，如果程序超出时间限制被杀死，会在log底部注明： Time Limit Error.


##### 内存的上限是多少？

> 评测程序为选手预留了320MB 左右的用户空间，不计算在8GB DRAM内。

##### AEP的上限是多少？

> 64GB

##### libpmem需要自己包含在工程吗，还是测试环境已经有
> 已有

##### Linux 与编译
> - Linux 4.19.91-19.1.al7.x86_64 x86_64
> - gcc version 4.8.5

##### 评测时间

> 性能评测时限15分钟，一次写入测试，10次读写测试，取读写测试最慢速度作为读写成绩。

##### 复赛一定要依赖libpmem或PMDK吗？
> 是的。