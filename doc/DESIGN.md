# “炸酱面”队比赛攻略

## 1. 团队介绍
#### 组队背景与目标
炸酱面队的3位成员是在参加另外一个比赛的过程中认识的，由于大赛题目难度较高，完成它的工作过程是一个系统工程，需要投入很多时间进行****赛题分析、MVP（Minimum Viable Product）版本开发、评测程序运行特征分析、基础组件代码开发、多种设计思路的探索实验、最终版本的局部优化****等，作为已经工作多年的个人，难以保证投入充足的时间完成上述工作，因此通过组队弥补了个人参赛投入时间精力不足的短板，在完成赛题的过程中，每个人都能发挥自己所长，进行设计思路的充分碰撞。比如我们最终版本的代码，索引、数据存储、宕机恢复、存储复用、关键逻辑优化等不同模块的设计思路和代码贡献就是都来自不同的队友。

我们的组队目标很明确，通过队员间的密切配合，保季军争冠军。

#### 合作方式
主要介绍复赛阶段的团队合作方式：
1. 复赛开始第一周，由一个队员开发MVP版本，用于赛题特征和评测过程的分析，**炸酱面队正是在复赛阶段第一个提交有效成绩的队伍**
2. 复赛的早期，团队基于MVP版本对评测程序的行为进行分析，获得诸如去重后的KV数量、实际数据量、读取热点等信息
3. 复赛的中段，团队队员基于MVP版本各自进行优化改进，探索最优的数据结构设计方案，由于可以“偷取”评测次数，实际上每天每个人跑测试的次数是无限的
4. 复赛结束前一周，团队收敛代码到最优的数据结构版本，大家开始共同维护同一个代码仓库，开始进行“最后一秒”的代码优化
5. 复赛结束后，团队通过共享文档，共同编辑Markdown文档和PPT，完成答辩材料准备

## 2. 赛题与解析
#### 赛题回顾
赛题要求在挂载傲腾持久化内存的单节点上设计实现高性能Key-Value缓存引擎，并提供持久化与容灾恢复。 
赛题提供的评测平台参数如下： 采用Intel(R) Xeon(R) Platinum 8269CY处理器，具有26物理核心以及52线程，支持向量化指令。存储方面，有8GB的DRAM以及64GB的持久化内存。
赛题测评分为三个阶段。 
* 正确性评测： 使用16个线程进行并发写入，读取验证正确性。 
* 持久化评测： 模拟掉电情形，对写入数据的一致性进行评测，包括更新与物理落盘。 
* 性能评测：
    * 第一阶段，使用16个线程进行24M次并发写入。
    * 第二阶段，使用16个线程进行24M次混合读写。

评测得分为两部分的总耗时。 三个评测阶段分别涵盖了引擎的三个特性：正确性、一致性与高效性。

#### 持久性内存和PMDK特性
Intel Optane持久性内存（pmem）提供大容量的空间与持久性，通过内存接口直连内存控制器，具有更低的读写延迟及更高的访问带宽。为了实现更好的pmem使用性能，Intel还提供了PMDK开发库，封装了一系列适配pmem的CPU指令，而本次比赛的复赛赛题也要求我们使用PMDK提供的方法对pmem进行操作。进过团队的分析和实验，针对本次比赛，我们总结一些pmem和PMDK的特性：
* **顺序写性能优于随机写**：这一特点决定了我们的架构设计，在能够进行顺序写的时候，需要尽量保证顺序写。
* **随机读取延迟略低于DRAM**：减少对pmem的随机读取次数，需要考虑写入和读取时的一次IO操作设计，以及考虑热点记录的cache。
* **原子性与顺序性保证**：pmem仅保证对齐的8byte写入原子性，单不保证多个8byte之间的顺序性，即使是虚拟地址连续的多个8byte顺序写入，也不能保证在掉电场景下的原子性，这要求我们在设计持久化与恢复逻辑时注意它与传统HDD和SSD的差别。
* **基于Linux虚拟地址的读写**：尽管pmem以DirectAccess方式挂载文件系统，我们仍然要注意PMDK提供的对pmem的读写方式是对Linux虚拟地址的操作，这要求我们在进行软件设计时需要考虑虚拟地址映射机制的影响。

#### 赛题核心考点分析
* **数据重复写入**：经过对评测程序的分析，去重后写入的KV数量为229,037,390，而总写入次数为402,653,184，有近一半的数据为重复写入，这要求我们在设计存储数据结构时需要考虑对同一个Key的覆盖写入，以及在宕机恢复时能够恢复出每个Key的最新值。
* **空间回收**：性能阶段总体写入的数据量为75G，而评测的硬件限制内存大小为8G，pmem大小为64G，这要求我们在处理数据覆盖写入后，要能够回收被覆盖写入的空间，而Value大小为80-1024Byte的可变长度，空间回收逻辑需要能够高效率进行这种变长空间的回收和碎片处理。
* **持久化宕机恢复**：持久化逻辑的设计需要考虑上述pmem的原子性和顺序性特性，我们需要能够正确处理部分KV没有被完整持久化的情况，而另外一方面经过精心设计多次8byte的原子写，虽然可以保证数据完整性，但是还需要考虑这里的性能代价，我们需要在数据完整性和持久化性进行综合考虑。
* **读写并发**：出题方已经确认不会出现对同一个Key的多写并发，但是对同一个Key的读写并发是有可能出现的，如何保证数据在被覆盖写的过程中并发执行的读取操作能够读到完整的旧值或完整的新值，**以及这个过程与空间回收逻辑的冲突，也是本次赛题的重要难点，我们也自信我们的设计在这一点上的正确性**。

#### 评测运行特征分析
* **Pintool Hook**：评测程序在正确性阶段使用Intel的Pintool工具对PMDK的方法进行了hook，而在性能阶段则没有hook，这给了我们机会在程序中能够判断当前处于何种阶段，而这一点的判断在后期帮助我们以比较tricky的方式实现了pmem地址空间在性能阶段进行的初始化（后文会提到）。
* **线程模型**：可在性能评测阶段，每次写入测试、读取测试的都会启动一组新的线程进行测试，这给了我们机会，能够比较精确的判断当前的读写测试进度，使得一方面我们可以选择提前退出评测，另一方面也能够自行记录读写阶段的耗时。
* **确定KV个数**：通过多次对KV去重个数的统计，我们统计出去重后的KV数量为229,037,390，这使得我们可以对Hash索引的膨胀率有比较精确的预估。
* **初始化阶段不计时**：复赛的初始化阶段不计时，因此我们可以做更复杂的恢复和初始化动作，这也意味着我们不需要投入精力为程序的数据结构在Recovery逻辑上进行性能上的优化设计，而pmem虚拟地址空间的初始化操作也可以在这个阶段完成。
* **可无限评测的糖**：性能评测阶段程序异常退出时，评测程序并不记录对评测次数的占用，因此我们在上述精确判断评测阶段的基础上，在最后一次性能读取测试结束之前退出，既不占用评测次数，也能够得到日志分析测试结果。
* **读取热点**：通过对每个KV使用少量Bit的记录和统计分布，我们发现被读取在10次以上的KV仅仅集中在约5万行的热点记录上，但是遗憾的是我们并没有能在热点cache的设计上得到性能提升（后文会提到），最终提交的是不带cache的版本，这也是后续可能继续研究的技术点。


## 3. 总体设计
#### 总体架构
![](https://codimd.s3.shivering-isles.com/demo/uploads/upload_8d82986c70148a151bd061ce0a517bea.png)
如上图所示，我们的设计包含4个模块，他们的设计功能描述如下：
* **NvmEngine**：实现评测程序的接口要求，提供写入和查询接口，组合索引结构HashMap和数据存储结构HeapTable，实现初始化、数据恢复和一些性能统计功能。
* **HashMap**：Key-Value索引结构，提供Key到Value Postion的写入、更新和查询能力，支持无锁的并发写入和查询能力，但不支持相同Key的并发写入。
* **HeapTable**：提供通过逻辑Position的Key-Value对的数据存储和查询能力，提供启动恢复能力，配合HashMap实现启动后索引的重新装载；提供数据覆盖写的空间回收管理能力；对pmem存储空间进行规划，实现并发无锁的数据写入能力。
* **Utils**：基础库模块，提供写日志、原子操作、计时器、Hash函数、常用的宏等基础代码的封装，提升研发效率。
* **测试代码**：我们对于HashMap、HeapTable实现了单元测试代码，并基于评测代码的特征开发了模拟评测代码，避免低级错误提交评测浪费时间。

#### 文件存储布局
我们将pmem的64GB存储空间平均划分为16个partition，每个线程对应一个partition大小为4GB，因此可以使用线程局部位置偏移来管理在partition内部的顺序写。

#### GC设计
Key-Value被覆盖写入后，旧的空间需要能够回收后给其他Key-Value的写入，由于Value为80-1024Byte的变长大小，因此最初我们使用长度为945的数组来针对每种Value长度维护一个FreeList来存储被回收的Key-Value空间，当partition上连续空间不足时，则根据要申请的空间长度去查询上述FreeList来获取空闲空间，当指定大小的空间没有空闲时，则遍历查找更大空间的FreeList。在这个设计基础上的优化包括了对945这个数组长度的压缩和线程局部化优化，请见后文。

#### 读写并发设计
组委会认为需要考虑对同一个Key的读写并发，因此我们需要考虑两种情况：
1. Key-Value正在被更新的过程中，有其他线程来读取，这个处理比较简单，我们只需要保证数据完成后再把新的Position原子的更新到HashMap中，读取操作要么拿到旧的position要么拿到新的position。这里我们利用的是Intel x86_64架构在cacheline对齐情况下的8Byte读写的原子特性。
2. Key-Value的旧的postition已经被读线程获取到，但是在通过position读取Value之前，这个Position已经被空间回收逻辑提供给其他Key-Value写入，这里最有效的工程化方案是引用计数或Hazard Pointer/Hazard Version方案，但是考虑到这两种方案稍高的性能成本，我们采用Double Check方案，在根据Position读取Value后，对HashMap中存储的position进行乐观检查，如果HashMap中的position和version已经发生变化，则完整重试一遍读取操作，乐观检查的风险是Version溢出，不过这一点风险极低，可以忽略。

#### Recovery设计
由于同一个Key可能在不同线程被覆盖写入，在恢复的时候，我们不能简单的通过比较物理位置的大小来觉得Value的新旧，因此我们为Key-Value维护了一个Version字段，每次覆盖写入则更新Version，为了避免在更新Key-Value时对旧的Version的读取，我们将Version冗余到了HashMap中，因此更新的时候只需要将HashMap中存储的Version加1后作为新Value的version即可。

#### 完整性设计
考虑到pmem的8Byte原子性和顺序性特性，我们设计了checksum的机制来保证Key-Value写入的完整性，即将Key-Value算出CRC Checksum后通过一次pmem_persist操作写入到pmem然后再返回写入成功，这个过程中如果发生掉电，即使Key-Value写不完整，我们也能够在恢复阶段通过校验checksum的方式来检查出来。


## 4. 关键逻辑设计

### 4.1 关键模块设计

#### Hash Line 设计

由于内存空间大小的限制，Hashmap只有约**5%**的额外存储空间，哈希冲突成为亟待解决的问题。在Hashmap的设计中，采用线性探查的方式处理哈希冲突的情况，为了尽可能降低线性探查带来的性能损失，我们设计了Hash Line即哈希桶，其映射与查找过程如下：

- 对Key进行哈希，将结果作为索引定位到某一个Hash Line

  <img src="https://raw.githubusercontent.com/Lqlsoftware/public-imgrepo/master/image-20201130145647563.png" alt="image-20201130145647563" style="zoom:50%;" />

- 在桶内（Hash Line）进行Key的顺序比较查找

- 如果未找到则进入下一个Hash Line查找，直到找到对应索引项或找到空闲项为止。

  <img src="https://raw.githubusercontent.com/Lqlsoftware/public-imgrepo/master/image-20201130145742548.png" alt="image-20201130145742548" style="zoom:50%;" />

Hash Line的数据结构如下：

```c
struct HashLine {
  Key 			keys[5];
  uint64_t 	vals[5];
  uint8_t 	count;
  uint8_t 	padding[7];
}
```

| 属性      | 类型          | 解释                 |
| --------- | ------------- | -------------------- |
| `keys`    | `Key[5]`      | 桶内索引项的所有键   |
| `vals`    | `uint64_t[5]` | 桶内索引项的所有值   |
| `count`   | `uint8_t`     | 桶内已有索引项的个数 |
| `padding` | `uint8_t[7]`  | 无意义，用于实现对齐 |

HashLine设计上按cache line大小进行了对齐，防止出现伪共享问题；在桶内容量的选择上采用了使得Padding最小的原则，即最节省空间，利用整数线性规划求解能够满足条件的最优解，得到当桶内容纳5个索引项时，Padding最小，同时按2倍的Cache Line对齐。
$$
\begin{array}{l}
\begin{array}{rl}
minimize & 64C−(16+8)X−1\\
subject\ to & 64C−(16+8)X \ge 1 \\
& 255\ge X \ge 0 \\
& X \text{ is an integer}\\
& C \ge 0 \\
& C \text{ is an integer}
\end{array}
\\=> X=5, C=2
\end{array}
$$


#### Index Key-Value 设计

实现多线程下PMEM读写的一致性需要较大的开销，包括对PMEM额外的读写操作以及额外的内存空间。针对本赛题的硬件特性，我们将PMEM的空间按照线程数量进行了分区，屏蔽掉多线程下PMEM潜在的并发读写问题。

在Hashmap中Key映射的堆表索引数据结构如下：

```c
struct Pos {
	union {
		struct {
			uint32_t 	pos_in_part;
			uint16_t 	mem_len;
			uint8_t 	part;
			uint8_t 	version;
		};
		uint64_t v;
	};
};
```

索引总共8Byte，满足CPU以及持久化内存的原子写大小（8Byte），其采用Union的方式放入uint64_t的类型中，具体包含的属性如下：

| 属性          | 类型       | 解释                                  |
| ------------- | ---------- | ------------------------------------- |
| `part`        | `uint32_t` | 线程分区号                            |
| `pos_in_part` | `uint16_t` | 线程分区内的偏移量                    |
| `mem_len`     | `uint8_t`  | 在堆表中可用空间的长度                |
| `version`     | `uint8_t`  | Key-Value的版本号，值越大代表版本越新 |

为了进一步减少对PMEM的随机读次数，我们将`mem_len`与`version`使用Union集成在索引中，在进行旧空间的回收时，访问内存中的Hashmap得到索引即可取得Buffer长度以及Version信息。



#### KV Item设计

基于对PMEM性能的分析，其性能与带宽在顺序读写大于等于256Byte时能取得较好的效果，KV Item的数据结构应尽可能的适配其读写特点，尽量使得对一个Key-Value的键值对（**KV Item**）的Get以及Set过程顺序读写PMEM，并降低并发读写带来的一致性问题，同时需要支持崩溃恢复功能，即可以通过PMEM中的内容构建内存中完整的映射关系。

KV Item的数据结构如下：

```c
struct KVItem {
	uint16_t 	checksum;
	uint16_t 	mem_len;
	uint16_t 	value_len;
	uint8_t 	version;
	uint8_t 	padding;
	char 			key[16];
	char 			value[0];
}
```

| 属性        | 类型       | 解释                                  |
| ----------- | ---------- | ------------------------------------- |
| `checksum`  | `uint16_t` | KV Item的校验和                       |
| `mem_len`   | `uint16_t` | 可用空间的长度                        |
| `value_len` | `uint16_t` | Value的长度                           |
| `version`   | `uint8_t`  | Key-Value的版本号，值越大代表版本越新 |
| `padding`   | `uint8_t`  | 无意义，用于实现对齐                  |
| `key`       | `char[16]` | 存储的Key                             |
| `value`     | `char[0]`  | 存储的Value（起始地址）               |

<img src="https://raw.githubusercontent.com/Lqlsoftware/public-imgrepo/master/image-20201130155329155.png" alt="image-20201130155329155" style="zoom:50%;" />

在KV Item的数据结构中，将其他信息（Header）KEY、VALUE顺序存储，减少Set过程中对PMEM写入的次数，增加每次写的大小。

同时Header中保留了整体的校验和、value的Buffer长度、value长度以及version信息，为断电恢复以及PMEM空间的复用提供支持。


#### Allocator与空间复用

我们基于Free list实现了支持可变长度Value以及内存的失效-复用机制的PMEM空间分配器。

由于对PMEM按照线程数量进行了划分，PMEM空间分配器将是线程间独立的，即为每个分区块配置了一个空间分配器，同时无需考虑现线程间的并发竞争问题。在每个分区块内，以32Byte为粒度按块进行PMEM空间的申请与分配，对于一块分配的PMEM空间来说，其可能包含4到最大33个块（Header 24Byte, Value 80~1024Byte）。

在描述索引结构的章节中介绍过，索引包含了复用PMEM空间需要的所有信息（位置、偏移量、长度），因此使用索引作为一块分配的PMEM空间的描述符。

Allocator的数据结构如下：

```c
struct Freelist {
	PageArena arena;
	Node 			*free_list_head;
	Node 			*lists[34];
	uint64_t 	padding;
};
```

| 属性             | 类型        | 解释                         |
| ---------------- | ----------- | ---------------------------- |
| `arena`          | `PageArena` | 页内存空间池                 |
| `free_list_head` | `Node*`     | 可用Node节点组成的链表       |
| `lists`          | `Node*[34]` | 可用PMEM空间描述符组成的链表 |
| `padding`        | `uint64_t`  | 无意义，用于实现对齐         |

<img src="https://raw.githubusercontent.com/Lqlsoftware/public-imgrepo/master/image-20201130162420880.png" alt="image-20201130162420880" style="zoom:50%;" />

Allocator采用Free list实现，分别以链表的形式记录4～34个Block大小的可复用空间描述符，在对空间进行回收时，采用全局回收的返回回收至线程内部。

链表中的描述符节点采用Page Arena页内存空间池（内存按页申请，按需分配）进行分配，并在初始化时为Free-list预先分配了足够的节点。当Freelist中不为空时，直接使用Free-list中内存空间存储描述符；当Freelist用尽时，利用Page Arena按需分配节点空间。

Allocator使用了一种优先顺序分配的策略，在线程内总写入（分配）量小于2G时，优先顺序申请新的PMEM空间；当总写入量大于2G时，我们认为Allocator已经收集了足够多的可复用PMEM空间，此时优先复用回收的空间。



### 4.2 工程优化

 #### Switch Case硬编码

在Hashmap的查找过程中，映射至某个Hash Line后，将在Hash Line内部逐一进行Key的比对，由于只有5个元素且存在不满的情况，利用for循环实现每个循环将存在一个分支，用于比较当前下标与Hash Line内元素个数。

```c
bool Get(const Slice &key, OccupyHandle &oh, bool &is_fullfilled) {
  uint8_t total = count;
  for (int idx = 0;idx < total;idx++) {
		if (keys[0] == key) {
			if (vs[0] == UINT64_MAX) {
				is_fullfilled = false;
				return false;
			}
			oh.hline = this;
			oh.index = 0;
			oh.is_exist = true;
			return true;
    }
  }
  is_fullfilled = (count >= KV_PER_LINE);
  return false;
}
```

我们利用switch对4分支以上采用跳表实现的特性，实现了对循环的展开，减少了分支的数量。

```c
bool Get(const Slice &key, OccupyHandle &oh, bool &is_fullfilled) {
	switch (count) {
    case 5:
      if (keys[5] == key) {
        if (vs[5] == UINT64_MAX) {
          is_fullfilled = false;
          return false;
        }
        oh.hline = this;
        oh.index = 5;
        oh.is_exist = true;
        return true;
      }
    case 4:
      if (keys[4] == key) {
        if (vs[4] == UINT64_MAX) {
          is_fullfilled = false;
          return false;
        }
        oh.hline = this;
        oh.index = 4;
        oh.is_exist = true;
        return true;
      }
    ...
    default:
      break;
  }
  is_fullfilled = (count >= KV_PER_LINE);
  return false;
}
```



#### xxHash

xxHash是一种具有超越内存读写带宽性能的哈希函数，相比CRC、MD5，SHA256等等函数具有较低且稳定的碰撞率，通过了 [SMHasher](https://code.google.com/p/smhasher/wiki/SMHasher) 的哈希测试套件的测试。

其在i7 9700K平台的性能测试结果如下：

| Hash Name             | Width | Bandwidth (GB/s) | Small Data Velocity | Quality | Comment                                                      |
| --------------------- | ----- | ---------------- | ------------------- | ------- | ------------------------------------------------------------ |
| **XXH3** (SSE2)       | 64    | 31.5 GB/s        | 133.1               | 10      |                                                              |
| **XXH128** (SSE2)     | 128   | 29.6 GB/s        | 118.1               | 10      |                                                              |
| *RAM sequential read* | N/A   | 28.0 GB/s        | N/A                 | N/A     | *for reference*                                              |
| City64                | 64    | 22.0 GB/s        | 76.6                | 10      |                                                              |
| T1ha2                 | 64    | 22.0 GB/s        | 99.0                | 9       | Slightly worse [collisions](https://github.com/Cyan4973/xxHash/wiki/Collision-ratio-comparison#collision-study) |
| City128               | 128   | 21.7 GB/s        | 57.7                | 10      |                                                              |
| **XXH64**             | 64    | 19.4 GB/s        | 71.0                | 10      |                                                              |
| SpookyHash            | 64    | 19.3 GB/s        | 53.2                | 10      |                                                              |
| Mum                   | 64    | 18.0 GB/s        | 67.0                | 9       | Slightly worse [collisions](https://github.com/Cyan4973/xxHash/wiki/Collision-ratio-comparison#collision-study) |
| **XXH32**             | 32    | 9.7 GB/s         | 71.9                | 10      |                                                              |
| City32                | 32    | 9.1 GB/s         | 66.0                | 10      |                                                              |
| Murmur3               | 32    | 3.9 GB/s         | 56.1                | 10      |                                                              |
| SipHash               | 64    | 3.0 GB/s         | 43.2                | 10      |                                                              |
| FNV64                 | 64    | 1.2 GB/s         | 62.7                | 5       | Poor avalanche properties                                    |
| Blake2                | 256   | 1.1 GB/s         | 5.1                 | 10      | Cryptographic                                                |
| SHA1                  | 160   | 0.8 GB/s         | 5.6                 | 10      | Cryptographic but broken                                     |
| MD5                   | 128   | 0.6 GB/s         | 7.8                 | 10      | Cryptographic but broken                                     |

xxHash3在处理大规模数据时具有显著的性能优势，同时可以通过设置强制对齐以及修改预取距离来提高xxHash3的性能。利用xxHash代替CRC64计算Key-Value数据对的校验和，并修改删除了计算代码中小于104 Byte、大于1048 Byte规模的计算分支，最终将总体写入的校验和计算时间缩短了2秒。



#### Key Compare

Key的定义为16Byte长度的字符串，对Key的比较是Hashmap访问的热点函数，每一次Get访问都可能存在大量Key的比较计算。使用`memcmp`函数将对两个Key逐字节进行比较，并返回比较结果（大于/小于/等于）。 

```c
char key[16];
```

由于Hashmap中线性探查的特殊性，只需要比较的布尔型结果即可，我们针对定长的Key设计了一种比较方案：

```c
struct Key {
	uint64_t k[2];

	bool operator== (const Key &key) const {
		if (k[0] == key.k[0] && k[1] == key.k[1]) {
			return true;
		} else {
			return false;
		}
	}
}
```

利用`uint64_t`代替逐字节比较，同时由于短路特性，比较返回的速度将更快。



#### ThreadLocal伪共享

作为Heaptable中控制线程分配PMEM以及空间回收策略的数据结构，ThreadLocal内部包含了线程当前顺序分配PMEM的位置，PMEM顺序空间分配的大小，其内容将在每次线程执行Set操作时修改。

```c
struct ThreadLocalInfo {
	uint64_t gc_append_len;
	uint64_t gc_append_len_sum;
	uint64_t gc_elastic_room;
	uint64_t data_index;
	uint32_t version;
};
ThreadLocalInfo tlis_[16];
```

由于Intel处理器每个核心拥有独立的Cache，多线程下ThreadLocalInfo的不同部分将被载入不同核心的Cache中，由于ThreadLocalInfo未按照Cache line对齐，可能若干个核心共享相同的Cache line，由于Cache的一致性协议的存在，当线程进行写入时会造成伪共享的问题，多线程下的性能大大降低。

```c
struct ThreadLocalInfo {
	uint64_t gc_append_len;
	uint64_t gc_append_len_sum;
	uint64_t gc_elastic_room;
	uint64_t data_index;
	uint32_t version;
	uint8_t  padding[28];
};
ThreadLocalInfo tlis_[16];
```

在修改ThreadLocalInfo的数据结构使得每个线程拥有的部分按Cache line对齐后，性能大幅上升，性能测试阶段的写入时间约减少10s。



#### Hashmap膨胀率

在**Hash Line 设计**章节详细讲述了Hash line的设计，以5个元素为一个Hash line（Bucket），按照Cache line对数据结构进行了对齐。同时，我们对性能测试阶段写入的Hashmap的Key的个数进行了统计，去除重复元素后（更新操作）共计`229037390`个Key，按照5个元素为一个Hash line，总共`45807478`个Hash line，共计`5.46G`。考虑到内存容量的限制`8G`以及其他数据结构在内存中需要预留的空间，设计上为Hashmap预留了**5%**的额外空间，容量为`240489295`个Key，共计`5.73G`，膨胀率为**95.24%**。

实际测试中，我们对5%、10%、15%三种内存分配策略进行了测试，结果表明：5%的额外空间具有最佳的性能结果。

虽然较高的膨胀率导致对HashMap的查找成为了引擎性能的瓶颈，但是由于内存具有较高的随机读性能，相比其他利用PMEM构建索引组织表的方案更加合理，线性探查优化后的性能远高于对PMEM直接顺序读写，成为了性能最佳的方案。



#### ATOMIC ADD

针对Hashmap以及Heaptable中对原子加法操作的需求，我们设计实现了`ATOMIC_ADD`，利用CAS指令完成了原子加法，同时提供原子加法指令不包括的功能，例如结果的范围控制等等。

```c
bool atomic_add_(uint64_t *p, const uint64_t delta, uint64_t limit, uint64_t &prev) {
  limit -= delta;
  uint64_t oldv = *p;
  while (true) {
    if (oldv > limit) {
      return false;
    }
    uint64_t cmpv = oldv;
    if (cmpv == (oldv = ATOMIC_VCAS(p, oldv, oldv + delta))) {
      prev = oldv;
      return true;
    }
    PAUSE();
  }
}
```

当CAS失败时，直接通过CAS原子指令的特性把对应指针指向的最新值取出，避免在原子操作外额外取一次最新值的cache miss代价。



#### gettid/gettno调用

在整体的架构设计中，针对线程数量对PMEM进行了划分。16个线程在进行对PMEM的读写操作时，需要知道自己全局唯一且不变的递增顺序号。

gettid使用NR_gettid的系统调用实现，获取全局唯一且不变的线程ID。然而NR_gettid的结果不保证有序，对ID进行取模可能将两个线程映射至同一个PMEM区块，产生并发问题。

```c
int64_t gettid() {
	static __thread int64_t tid = -1;
	if (UNLIKELY(-1 == tid)) {
		tid = static_cast<int64_t>(syscall(__NR_gettid));
	}
	return tid;
}
```

gettno则通过原子操作对全局静态变量进行递增，返回递增前的标号作为线程ID。gettno在保证线程ID全局唯一不变的同时，还保证了ID之间的顺序分布。

```c
int64_t gettno() {
  static __thread int64_t tno = -1;
  static int64_t tno_seq = 0;
	if (UNLIKELY(-1 == tno)) {
		tno = __sync_fetch_and_add(&tno_seq, 1);
	}
	return tno;
}
```

同时，在首次获取ID后，利用线程局部变量存储，并使用UNLIKELY编译指示来指导后面的分支预测，其后访问时只需读取线程内部变量即可返回，保证ID不变的同时避免系统调研提高了性能。



#### AEP初始化

将PMEM作为持久化设备时，需要通过文件系统对其进行访问，Ext4-DAX文件系统支持通过`mmap()`将PMEM文件映射至内存来绕过文件系统的页缓存，以直接访问的方式提高访问性能。

<img src="https://raw.githubusercontent.com/Lqlsoftware/public-imgrepo/master/image-20201203145553312.png" alt="image-20201203145553312" style="zoom:50%;" />

PMEM文件映射至内存后，可以通过映射的虚拟地址以操作PMEM空间。对于未访问过的内存空间，对应的物理空间未被分配，由于页表与TLB机制的存在，对每个页面的首次访问会触发page fault中断，分配物理内存并映射至虚拟内存页，这需要额外的时间来完成；同时，多线程下并发的page fault中断会带来内核锁冲突，导致显著的性能下降。

所以我们希望在初始化阶段提前将页面映射完成，即预热。预热正常不应该在构造函数中完成，我们根据比赛场景进行了变通，使用16个线程调用pmem_memset对整个PMEM进行初始化，同时更新页表与TLB。

正确性与持久化测试阶段使用Pin tools覆盖了pmdk的方法，增加了若干日志打印，调用pmem_memset进行初始化会导致日志过多而评测失败。我们通过判断pmem_drain的函数指针值来确定pmdk的方法有没有被pin tool覆盖，从而决定是否进行PMEM的预热操作。



#### Allocator初始化

Allocator采用了Free-list的设计，复用Free-list中的链表节点，使用Page Arena页内存空间池进行内存分配（内存按页申请，按需分配），这种分配方式大大提高了内存申请的性能。

然而由于Free-list初始化时为空，Allocator从Free-list中尝试获取链表节点失败，会默认从Page Arena中申请内存，直到Free-list能够满足Allocator的需要，这带来了巨大的性能损失。

为了解决这个问题，在初始化Free-list时预先分配了足够的节点，Allocator能够保持从Free-list获取链表节点而无需从Page Arena中申请内存，提高了每次空间回收时的性能。



#### ThreadOnce

在引擎开发的过程中，日志的反馈结果尤为重要，然而由于访问次数过多（24M，16线程），日志打印时需要灵活控制数量以及日志打印的位置。同时，我们希望线程在第一次调用Get、Set方法时进行额外的操作以及计时操作，方便我们对线程进行性能检测以及性能调优。

我们设计了宏`ThreadOnce`

```c
#define THREAD_ONCE(f) \
  do { \
    static __thread bool flag = false; \
    if (!flag) { \
      L(INFO, "THREAD_ONCE"); \
      f; \
      flag = true; \
    } \
  } while(0)
```

通过线程内部变量记录是否是第一次调用此宏，以较小的性能代价（几乎不影响整体性能）实现了对应的功能，为后续引擎优化提供了支持。



#### Key的预取

在Hashmap中，主要的访问集中在Get、Occupy和Replace三个函数上，由于Hashmap的填充率较高，线性探查成为了访问的热点。其中对Key之间的比较占了绝大多数时间，虽然对Key的比较进行了优化，然而在Hash Line中顺序比较Key带来的Cache miss仍然影响着整体的查找性能：

```c
bool Hashmap::Get(const Slice &key, OccupyHandle &oh) {
  uint32_t hash = tavern::CRC32Hash(key);
  for (uint32_t i = 0; i < HASH_LINE_CNT; ++i) {
    uint32_t pos = (hash + i) % HASH_LINE_CNT;
    bool is_fullfilled = false;
    if (hlines_[hash].Get(key, oh, is_fullfilled)) {
      return true;
    }
    if (!is_fullfilled) {
      return false;
    }
  }
  return false;
}
```

由于Key的访问规律已知，我们可以提前将Hash Line中要比较的Key通过预取的方式提前提到L0 Cache：

```c
bool Hashmap::Get(const Slice &key, OccupyHandle &oh) {
  uint32_t hash = tavern::CRC32Hash(key) % HASH_LINE_CNT;
  for (uint32_t i = 0; i < HASH_LINE_CNT; ++i) {
    bool is_fullfilled = false;
    if (hlines_[hash].Get(key, oh, is_fullfilled)) {
      return true;
    }
    if (!is_fullfilled) {
      return false;
    }
    hash++;
    hash = UNLIKELY(hash == HASH_LINE_CNT) ? 0 : hash;
    _mm_prefetch(hlines_[hash].keys, _MM_HINT_T0);
  }
  return false;
}
```

同时利用分支代替了取模的计算，减少了不必要的计算时间。

## 5 未尽之优化

### 5.1 热点数据Cache

我们尝试了许多种针对热点数据的优化，设计了3种不同结构的热点数据Cache。

- LRU Cache
- 直接替换Cache
- 线程内独立Cache

根据测试时的统计信息，三种Cache的命中数量占全部Get请求数量的99.99%，均达到了99.99%的命中率，然而使用热点Cache带来的效果接近不用Cache的性能表现，猜想是由于维护Cache结构的损耗与cache命中带来的性能提升相抵。

这不代表在热点数据Cache上没有优化空间，内存与PMEM的读性能差异依旧存在，应尽可能降低维护Cache结构带来的性能损失，同时提高Cache的缓存性能，应该存在较大的优化空间。



### 5.2 Hashmap优化

- 基于热点数据进行Hashmap的分裂

  将热点数据的映射关系从总的Hashmap中分裂出来，构成小的热点Hashmap，目的是为了使得热点访问尽可能进行更少的线性探查操作，减少热点访问的线性查找时间。

- Hot-ring环形结构

  Tair内提出的Hot-ring结构在本赛题中也存在一定的优化空间，利用环形链表使得热点数据在桶内最先被访问，能够减少热点访问在桶内的比较次数，然而考虑装填因子较高，可能需要数个桶上进行线性探查，在这种设计下无法发挥Hot-ring的结构优势，需要针对性的进行数据结构的适配。

### 5.3 Allocator优化

在PMEM空间分配器上，可以考虑更多策略上的改变，例如：

- 针对PMEM剩余空间的大小进行不同策略的分配（弹性分配等等）
- 对回收的PMEM空间进行空间合并，降低分配节点的碎片化
- 减少内存分配时链表查询的时间
