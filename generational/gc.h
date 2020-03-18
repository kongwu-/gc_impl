//
// Created by jiangxin on 2020/3/15.
//

#ifndef GC_IMPL_GC_H
#define GC_IMPL_GC_H

#endif //GC_IMPL_GC_H

/**
 * 1字节的byte类型，用来做标识位
 */
typedef unsigned char byte;

/**
 * 类描述
 */
typedef struct class_descriptor {
    char *name;//类名称
    int size;//类大小，即对应sizeof(struct)
    int num_fields;//属性数量
    int *field_offsets;//类中的属性偏移，即所有属性在struct中的偏移量（字节）
} class_descriptor;

/**
 * 基本对象类型
 * 所有对象都继承于Object
 * C中没有继承的概念，不过可以通过定义相同属性来实现，所有“继承”Object的struct，都需要将class/marked属性定义在开头
 */
typedef struct _object object;
struct _object {
    class_descriptor *class;//对象对应的类型
    byte forwarded;//已拷贝标识
    byte marked;//reachable标识
    byte remembered;//已经记录在rs中的标识
    object *forwarding;//目标位置
    int age;//对象年龄
};

/**
 * free-list的单元节点
 * 为了实现简单，在此算法中不考虑“碎片化”的问题
 * 将单元的大小设置的足够大，并且不允许分配超过此单元大小的内存
 * 每个单元只存放一个Object
 */
typedef struct _node node;
struct _node {
    node *next;
    byte used;//是否使用
    int size;
    object *data;//单元中的数据
};

#define MAX_ROOTS 100

#define MAX_AGE 3//晋升年龄，达到该值就晋升

#define MAX_HEAP_SIZE 1024*1024*10//10MB

#define NEW_RATIO 2//新生代和老年代配比，2代表新生代:老年代=2:8

#define SURVIVOR_RATIO 8 //新生代中Eden和from/to区配比，8代表Eden:From:To=8:1:1

#define NODE_SIZE 128//free-list单元大小(B)


const static byte TRUE = 1;
const static byte FALSE = 0;
/**
 * GC ROOTS
 */
extern object *_roots[MAX_ROOTS];

/**
 * remembered set
 */
extern object *_rs[MAX_ROOTS];

/**
 * GC ROOT 的当前下标，即记录到了第几个元素
 */
extern int _rp;
extern int _rsp;

/**
 * 堆总大小
 */
extern int heap_size;
/*===============================GC interface=================================*/

/**
 * 初始化GC
 * @param size
 * @param new_ratio 新老年代的比例
 */
extern void gc_init(int size);

/**
 * 执行GC
 */
extern void gc();

/**
 * GC结束，彻底清理堆
 */
extern void gc_done();

/**
 * 在GC堆上分配指定类型的内存
 * 使用顺序分配(sequential allocation)内存的方式
 * @param class 需要分配的类型
 * @return 分配的对象指针
 */
extern object *gc_alloc(class_descriptor *class);

/**
 * 修改引用
 * @param obj 原对象
 * @param ptr 原对象的属性指针
 * @param new_obj 新对象指针
 */
void gc_update_ptr(object *obj,object **field_ref, object *new_obj);

/**
 * DUMP GC状态
 * @return
 */
extern char *gc_get_state();

/**
 * 获取GC ROOTS数量
 * @return ROOTS数量
 */
extern int gc_num_roots();


/**
 * 暂存GC ROOTS下标
 */
#define gc_save_rp          int __rp = _rp;

/**
 * 将对象添加到GC ROOTS
 */
#define gc_add_root(p)    _roots[_rp++] = (object *)(p);

/**
 * 将对象添加到remembered set
 */
#define gc_add_rs(p)    _rs[_rsp++] = (object *)(p);

/**
 * 恢复GC ROOTS下标
 */
#define gc_restore_roots    _rp = __rp;

extern void swap(void **src, void **dst) ;