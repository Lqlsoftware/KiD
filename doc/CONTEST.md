### 赛题
Tair是阿里云自研的云原生内存数据库，接口兼容开源Redis/Memcache。在阿里巴巴集团内和阿里云上提供了缓存服务和高性能存储服务，追求极致的性能和稳定性。全新英特尔® 傲腾™ 数据中心级持久内存重新定义了传统的架构在内存密集型工作模式，具有突破性的性能水平，同时具有持久化能力，Aliyun弹性计算服务首次（全球首家）在神龙裸金属服务器上引入傲腾持久内存，撘配阿里云官方提供的Linux操作系统镜像Aliyun Linux，深度优化完善支持，为客户提供安全、稳定、高性能的体验。本题结合Tair基于神龙非易失性内存增强型裸金属实例和Aliyun Linux操作系统，探索新介质和新软件系统上极致的持久化和性能。参赛者在充分认知Aep硬件特质特性的情况下设计最优的数据结构。
### 赛题描述
热点是阿里巴巴双十一洪峰流量下最重要的一个问题，双十一淘宝首页上的一件热门商品的访问TPS在千万级别。这样的热点Key，在分布式的场景下也只会路由到其中的一台服务器上。解决这个问题的方法有Tair目前提供的热点散列机制，同时在单节点能力上需要做更多的增强。  
本题设计一个基于傲腾持久化内存(Aep)的KeyValue单机引擎，支持Set和Get的数据接口，同时对于热点访问具有良好的性能。  
### 评测逻辑
评测程序会调用参赛选手的接口，启动16个线程进行写入和读取数据，最终统计读写完指定数目所用的时间，按照使用时间的从低到高排名。  
评测分为2个阶段：  
1）程序正确性验证，验证数据读写的正确性（复赛会增加持久化能力的验证），这部分的耗时不计入运行时间的统计  
2）初赛性能评测  
引擎使用的内存和持久化内存限制在 4G Dram和 74G Aep。  
每个线程分别写入约48M个Key大小为16Bytes，Value大小为80Bytes的KV对象，接着以95：5的读写比例访问调用48M次。其中95%的读访问具有热点的特征，大部分的读访问集中在少量的Key上面。  
3）复赛性能评测  
引擎使用的内存和持久化内存限制在16G Dram和 128G Aep，复赛要求数据具有持久化和可恢复(Crash-Recover)的能力，确保在接口调用后，即使掉电的时候能保证数据的完整性和正确恢复。  
运行流程和初赛一致，差别是Value从80bytes定长变为80Bytes~1024Bytes之间的随机值，同时在程序正确性验证加入Crash-Recover的测试。

### 赛题资料
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

> 74GB = 74 * 1024 * 1024 * 1024 Byte

> 48M = 48 * 1024 * 1024

##### 纯写入有重复的Key吗？

> 存在低于0.2% 的重复key。

##### 读写混合是重复的Key吗？

> 全部是更新操作，均是重复的key。

##### 如何输出Log

> 引擎接口里有log_file 文件指针，允许输出log的测试中该指针不为空，可直接写入到此文件中，大小限制在5MB以内。
将每5MB清空一次log文件。

##### Log文件显示Correctness Failed 或 Performance Judge Failed是什么意思。

> 正确性或性能评测时失败。具体失败原因有很多，需要自行排查。

##### 评测失败是否反馈具体原因？例如OOM，Core等。

> 初赛不反馈程序意外退出的原因，仅反馈评测失败。

> 除此以外，如果程序超出时间限制被杀死，会在log底部注明： Time Limit Error.


##### 内存的上限是多少？

> 评测程序为选手预留了320MB 左右的用户空间，不计算在4GB DRAM内。

##### AEP的上限是多少？

> 74GB

##### libpmem需要自己包含在工程吗，还是测试环境已经有
> 已有

##### Linux 与编译
> - Linux 4.19.91-19.1.al7.x86_64 x86_64
> - gcc version 4.8.5

##### 评测时间

> 性能评测时限15分钟，一次写入测试，10次读写测试，取读写测试最慢速度作为读写成绩。

##### 初赛一定要依赖libpmem或PMDK吗？
> 没有必要，可以不依赖库，直接作为内存使用即可。
