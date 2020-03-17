//
// Created by jiangxin on 2020/3/15.
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "gc.h"

object *_roots[MAX_ROOTS];

int _rp;
void *heap;//堆指针
void *new;//新生代指针
void *new_eden;//新生代eden指针
void *new_from;//新生代from指针
void *new_to;//新生代to指针
void *old;//老年代

int next_forwarding_offset;//复制目标区域的free pointer
int next_free_offset;//下一个空闲内存地址（相对位置）
int heap_size;//堆容量
int new_size;//新生代容量
int old_size;//老年代容量
int eden_size;//eden区容量
int survivor_size;//survivor区容量
object *_roots[MAX_ROOTS];

node *old_next_free;//老年代下一个空闲单元
node *old_head;//老年代free-list的头节点

void new_copying();//新生代复制

object *new_copy(object *obj);//新生代复制对象

void adjust_ref();//复制后更新引用
void new_swap_space();//新生代from/to交换
int resolve_heap_size(int size);
void minor_gc();//新生代gc
void major_gc();//老年代gc
void promotion(object *obj);//对象晋升

void new_swap_space() {

    //清空from
    memset(from, 0, heap_half_size);

    //交换from/to
    void *temp = from;
    from = to;
    to = temp;

    //将free_offset更新为复制时使用的next_forwarding_offset
    next_free_offset = next_forwarding_offset;
}

int resolve_heap_size(int size) {
    if (size > MAX_HEAP_SIZE) {
        size = MAX_HEAP_SIZE;
    }

    //处理小数问题
    new_size = heap_size/10*NEW_RATIO;

    eden_size = new_size/10*SURVIVOR_RATIO;

    survivor_size = new_size/10;

    new_size = eden_size + survivor_size * 2;

    old_size = (heap_size-new_size)/NODE_SIZE * NODE_SIZE;

    heap_size = new_size + old_size;

    return heap_size;
}

void gc_init(int size) {
    heap_size = resolve_heap_size(size);

    //分配
    heap = malloc(heap_size);


    //初始化各区域指针
    new = heap;

    new_eden = heap;

    new_from = eden_size + new_eden;

    new_to = survivor_size + new_from;

    old = new_size + heap;

    _rp = 0;
}

object *new_malloc(class_descriptor *class){
    //检查是否可以分配
    if (next_free_offset + class->size > eden_size) {
        printf("Allocation Failed. execute gc...\n");
        minor_gc();
        if (next_free_offset + class->size > eden_size) {
            printf("Allocation Failed! OutOfMemory...\n");
            abort();
        }
    }

    int old_offset = next_free_offset;

    //分配后，free移动至下一个可分配位置
    next_free_offset = next_free_offset + class->size;

    //分配
    object *new_obj = (object *) (old_offset + new_eden);

    //初始化
    new_obj->class = class;
    new_obj->copied = FALSE;
    new_obj->marked = FALSE;
    new_obj->age = 0;
    new_obj->forwarding = NULL;

    for (int i = 0; i < new_obj->class->num_fields; ++i) {
        //*(data **)是一个dereference操作，拿到field的pointer
        //(void *)o是强转为void* pointer，void*进行加法运算的时候就不会按类型增加地址
        *(object **) ((void *) new_obj + new_obj->class->field_offsets[i]) = NULL;
    }

    return new_obj;
}

object *gc_alloc(class_descriptor *class) {

    object *new_obj = new_malloc(class);

    return new_obj;
}

/**
 * 将对象复制到to
 * 为了实现简单，此处不考虑to区无法存放的问题
 * @param obj
 * @return 复制后的对象指针
 */
object *new_copy(object *obj) {

    if (!obj) { return NULL; }

    //由于一个对象可能会被多个对象引用，所以此处判断，避免重复复制
    if (!obj->copied) {
        if(obj.age<MAX_AGE){
            //计算复制后的指针
            object *forwarding = (object *) (next_forwarding_offset + to);

            //赋值
            memcpy(forwarding, obj, obj->class->size);

            obj->copied = TRUE;

            //将复制后的指针，写入原对象的forwarding pointer，为最后更新引用做准备
            obj->forwarding = forwarding;

            //复制后，移动to区forwarding偏移
            next_forwarding_offset += obj->class->size;

            //递归复制引用对象，递归是深度优先
            for (int i = 0; i < obj->class->num_fields; i++) {
                new_copy(*(object **) ((void *) obj + obj->class->field_offsets[i]));
            }
        }else{
            //超过年龄就晋升
            promotion(obj);
        }
    }

    return obj->forwarding;
}

void swap(object **src,object **dst){
    object *temp = *src;
    *src = *dst;
    *dst = tmp;
}

void new_copying() {
    next_forwarding_offset = 0;
    //遍历GC ROOTS
    for (int i = 0; i < _rp; ++i) {
        object *root = _roots[i];

        //只处理处于新生代中的root
        if((void *)root < (void *)to){
            object *forwarding = new_copy(_roots[i]);

            //先将GC ROOTS引用的对象更新到to空间的新对象
            _roots[i] = forwarding;
        }
    }

    //找到rs中的跨代引用
    while (i < _rsp) {
        object *old_root = _rs[i];
        byte has_new_obj = FALSE;
        for (int i = 0; i < old_root->class->num_fields; i++) {

            object **new_object_p = (object **) ((void *) old_root + old_root->class->field_offsets[i]));
            object *new_object = *new_object_p;

            if((void *)new_object < (void *)old){
                object *forwarding = new_copy(new_object);

                //将老年代对新生代的引用更新为复制之后的对象
                *new_object_p = forwarding;

                if((void *)forwarding < (void *)old){
                    has_new_obj = TRUE;
                }
            }
        }

        //如果该老年代对象引用的所有新生代对象都已经晋升到老年代，则
        if(!has_new_obj){
            _rs[i]->remembered = FALSE;

            //如果不是最后一个元素，就先交换
            if(i < _rsp-1){
                swap(&_rs[i],&_rs[_rsp]);
            }

            //移除最后一个
            _rs[--_rsp] = NULL;
        }else{
            i++;
        }
    }

    //更新引用
    adjust_ref();

    //清空from，并交换from/to
    new_swap_space();
}

void adjust_ref() {
    int p = 0;
    //遍历to，即复制的目标空间
    while (p < next_forwarding_offset) {
        object *obj = (object *) (p + new_to);
        //将还指向from的引用更新为forwarding pointer，即to中的pointer
        for (int i = 0; i < obj->class->num_fields; i++) {
            object **field = (object **) ((void *) obj + obj->class->field_offsets[i]);
            if ((*field) && (*field)->forwarding) {
                *field = (*field)->forwarding;
            }
        }

        //顺序访问下一个对象
        p = p + obj->class->size;
    }
}

void compact_rs(){
    int _new_rsp = 0;
    object *_new_rs[MAX_ROOTS];
    for (int i = 0; i < _rsp; ++i) {
        object *obj = _rs[i];
        if(obj){
            _new_rs[_new_rsp++] = obj;
        }
    }
}

void gc_update_ptr(object** ptr, void *obj){

}

void write_barrier(object *obj,object **field_ref,object *new_obj){

}

void minor_gc(){
    printf("minor gc...\n");
    new_copying();
}
void gc() {

}


