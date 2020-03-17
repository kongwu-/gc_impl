//
// Created by jiangxin on 2020/3/15.
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "gc.h"

object *_roots[MAX_ROOTS];

int _rp;//GC ROOTS index
void *heap;//堆指针
object *_roots[MAX_ROOTS];//GC ROOTS
int next_free_offset;//下一个空闲内存地址（相对位置）
int heap_size;//堆容量

/**
 * 计算并更新forwarding pointer
 */
void set_forwarding();

/**
 * 调整存活对象的引用指针
 */
void adjust_ref();

/**
 * 移动存活对象至forwarding pointer
 */
void move_obj();

/**
 * 整理
 */
void compact();

/**
 * 标记
 */
void mark(object *obj);

int resolve_heap_size(int size);

/**
 * 处理初始值
 * @param size 初始堆大小
 * @return 一半堆的大小
 */
int resolve_heap_size(int size) {
    if (size > MAX_HEAP_SIZE) {
        size = MAX_HEAP_SIZE;
    }
    return size;
}

void gc_init(int size) {
    heap_size = resolve_heap_size(size);
    heap = (void *) malloc(heap_size);
    _rp = 0;
}


object *gc_alloc(class_descriptor *class) {

    //检查是否可以分配
    if (next_free_offset + class->size > heap_size) {
        printf("Allocation Failed. execute gc...\n");
        gc();
        if (next_free_offset + class->size > heap_size) {
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
    new_obj->marked = FALSE;
    new_obj->forwarding = NULL;

    for (int i = 0; i < new_obj->class->num_fields; ++i) {
        //*(data **)是一个dereference操作，拿到field的pointer
        //(void *)o是强转为void* pointer，void*进行加法运算的时候就不会按类型增加地址
        *(object **) ((void *) new_obj + new_obj->class->field_offsets[i]) = NULL;
    }

    return new_obj;
}

void set_forwarding() {
    int p = 0;
    int forwarding_offset = 0;

    //遍历堆的已使用部分，这里不用遍历全堆
    //因为是顺序分配法，所以只需要遍历到已分配的终点即可
    while (p < next_free_offset) {
        object *obj = (object *) (p + heap);

        //为可达的对象设置forwarding
        if (obj->marked) {

            obj->forwarding = (object *) (forwarding_offset + heap);

            forwarding_offset = forwarding_offset + obj->class->size;
        }
        p = p + obj->class->size;
    }
}

void adjust_ref() {
    int p = 0;

    //先将roots的引用更新
    for (int i = 0; i < _rp; ++i) {
        object *r_obj = _roots[i];
        _roots[i] = r_obj->forwarding;
    }

    //再遍历堆，更新存活对象的引用
    while (p < next_free_offset) {
        object *obj = (object *) (p + heap);

        if (obj->marked) {
            //更新引用为forwarding
            for (int i = 0; i < obj->class->num_fields; i++) {
                object **field = (object **) ((void *) obj + obj->class->field_offsets[i]);
                if ((*field) && (*field)->forwarding) {
                    *field = (*field)->forwarding;
                }
            }

        }
        p = p + obj->class->size;
    }
}

void move_obj() {
    int p = 0;
    int new_next_free_offset = 0;
    while (p < next_free_offset) {
        object *obj = (object *) (p + heap);

        if (obj->marked) {
            //移动对象至forwarding
            obj->marked = FALSE;
            memcpy(obj->forwarding, obj, obj->class->size);
            new_next_free_offset = new_next_free_offset + obj->class->size;
        }
        p = p + obj->class->size;
    }

    //清空移动后的间隙
    memset((void *)(new_next_free_offset+heap),0,next_free_offset-new_next_free_offset);

    //移动完成后，更新free pointer为新的边界指针
    next_free_offset = new_next_free_offset;

}

void compact() {
    set_forwarding();
    adjust_ref();
    move_obj();
}

void mark(object *obj) {
    if (!obj || obj->marked) { return; }

    obj->marked = TRUE;
    printf("marking...\n");
    //递归标记对象的引用
    for (int i = 0; i < obj->class->num_fields; ++i) {
        mark(*((object **) ((void *) obj + obj->class->field_offsets[i])));
    }
}

void gc() {

    for (int i = 0; i < _rp; ++i) {
        mark(_roots[i]);
    }

    compact();
}


