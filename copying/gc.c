//
// Created by jiangxin on 2020/3/15.
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "gc.h"

object *_roots[MAX_ROOTS];

int _rp;
void *heap;//堆指针
void *from;//from指针
void *to;//to指针
int next_forwarding_offset;//复制目标区域的free pointer
object *_roots[MAX_ROOTS];
int next_free_offset;//下一个空闲内存地址（相对位置）
int heap_size;//堆容量
int heap_half_size;//堆容量

/**
 * 复制回收
 */
void copying();

/**
 * 将对象复制到to
 * @param obj
 * @return 复制后的对象指针
 */
object *copy(object *obj);

/**
 * 更新引用
 * 将复制后的对象引用从from区更新到to区
 */
void adjust_ref();

/**
 * 交换from/to
 */
void swap(void **src,void **dst);

/**
 * 处理初始值
 * @param size 初始堆大小
 * @return 一半堆的大小
 */
int resolve_heap_size(int size);

void swap(void **src, void **dst) {
    object *temp = *src;
    *src = *dst;
    *dst = temp;
}

int resolve_heap_size(int size) {
    if (size > MAX_HEAP_SIZE) {
        size = MAX_HEAP_SIZE;
    }
    return size / 2 * 2;
}

void gc_init(int size) {
    heap_size = resolve_heap_size(size);
    heap_half_size = heap_size / 2;
    heap = (void *) malloc(heap_size);
    from = heap;
    to = (void *) (heap_half_size + from);
    _rp = 0;
}


object *gc_alloc(class_descriptor *class) {

    //检查是否可以分配
    if (next_free_offset + class->size > heap_half_size) {
        printf("Allocation Failed. execute gc...\n");
        gc();
        if (next_free_offset + class->size > heap_half_size) {
            printf("Allocation Failed! OutOfMemory...\n");
            abort();
        }
    }

    int old_offset = next_free_offset;

    //分配后，free移动至下一个可分配位置
    next_free_offset = next_free_offset + class->size;

    //分配
    object *new_obj = (object *) (old_offset + heap);

    //初始化
    new_obj->class = class;
    new_obj->forwarded = FALSE;
    new_obj->forwarding = NULL;

    for (int i = 0; i < new_obj->class->num_fields; ++i) {
        //*(data **)是一个dereference操作，拿到field的pointer
        //(void *)o是强转为void* pointer，void*进行加法运算的时候就不会按类型增加地址
        *(object **) ((void *) new_obj + new_obj->class->field_offsets[i]) = NULL;
    }

    return new_obj;
}

object *copy(object *obj) {

    if (!obj) { return NULL; }

    //由于一个对象可能会被多个对象引用，所以此处判断，避免重复复制
    if (!obj->forwarded) {

        //计算复制后的指针
        object *forwarding = (object *) (next_forwarding_offset + to);

        //赋值
        memcpy(forwarding, obj, obj->class->size);

        obj->forwarded = TRUE;

        //将复制后的指针，写入原对象的forwarding pointer，为最后更新引用做准备
        obj->forwarding = forwarding;

        //复制后，移动to区forwarding偏移
        next_forwarding_offset += obj->class->size;

        //递归复制引用对象，递归是深度优先
        for (int i = 0; i < obj->class->num_fields; i++) {
            copy(*(object **) ((void *) obj + obj->class->field_offsets[i]));
        }
        return forwarding;
    }

    return obj->forwarding;
}

void copying() {
    next_forwarding_offset = 0;
    //遍历GC ROOTS
    for (int i = 0; i < _rp; ++i) {
        object *forwarded = copy(_roots[i]);

        //先将GC ROOTS引用的对象更新到to空间的新对象
        _roots[i] = forwarded;
    }

    //更新引用
    adjust_ref();

    //清空from，并交换from/to
    swap(&from,&to);
}


void adjust_ref() {
    int p = 0;
    //遍历to，即复制的目标空间
    while (p < next_forwarding_offset) {
        object *obj = (object *) (p + to);
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

void gc() {
    printf("gc...\n");
    copying();
}


