//
// Created by jiangxin on 2020/3/15.
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "gc.h"

object *_roots[MAX_ROOTS];

node *next_free;
node *head;
int _rp;

int resolve_heap_size(int size);

node *init_free_list(int free_list_size);

node *find_idle_node();

void mark(object *obj);

void gc();

void sweep();


int resolve_heap_size(int size) {
    if (size > MAX_HEAP_SIZE) {
        size = MAX_HEAP_SIZE;
    }
    if (size < NODE_SIZE) {
        return NODE_SIZE;
    }
    return size / NODE_SIZE * NODE_SIZE;
}

node *init_free_list(int free_list_size) {
    node *head = NULL;
    for (int i = 0; i < free_list_size; ++i) {
        node *_node = (node *) malloc(NODE_SIZE);
        _node->next = head;
        _node->size = NODE_SIZE;
        _node->used = FALSE;
        _node->data = NULL;
        head = _node;
    }

    return head;
}

node *find_idle_node() {
    for (next_free = head; next_free && next_free->used; next_free = next_free->next) {}

    //还找不到就触发回收
    if (!next_free) {
        gc();
    }

    for (next_free = head->next; next_free && next_free->used; next_free = next_free->next) {}

    //再找不到真的没了……
    if (!next_free) {
        printf("Allocation Failed!OutOfMemory...\n");
        abort();
    }
}

void gc_init(int size) {
    int heap_size = resolve_heap_size(size);
    int free_list_size = heap_size / NODE_SIZE;
    head = init_free_list(free_list_size);

    _rp = 0;
    next_free = head;
}




object *gc_alloc(class_descriptor *class) {

    if (!next_free || next_free->used) {
        find_idle_node();
    }

    //赋值当前freePoint
    node *_node = next_free;

    //新分配的对象指针
    //将新对象分配在free-list的节点数据之后，node单元的空间内除了sizeof(node)，剩下的地址空间都用于存储对象
    object *new_obj = (void *) _node + sizeof(node);
    new_obj->class = class;
    new_obj->marked = FALSE;

    _node->used = TRUE;
    _node->data = new_obj;
    _node->size = class->size;

    for (int i = 0; i < new_obj->class->num_fields; ++i) {
        //*(data **)是一个dereference操作，拿到field的pointer
        //(void *)o是强转为void* pointer，void*进行加法运算的时候就不会按类型增加地址
        *(object **) ((void *) new_obj + new_obj->class->field_offsets[i]) = NULL;
    }
    next_free = next_free->next;

    return new_obj;
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

void sweep() {
    for (node *_cur = head; _cur && _cur; _cur = _cur->next) {
        if (!_cur->used)continue;
        object *obj = _cur->data;
        if (obj->marked) {
            obj->marked = FALSE;
        } else {
            //回收对象所属的node
            memset(obj, 0, obj->class->size);

            //通过地址计算出，对象所在的node
            node *_node = (node *) ((void *) obj - sizeof(node));
            _node->used = FALSE;
            _node->data = NULL;
            _node->size = 0;

            //将next_free更新为当前回收的node
            next_free = _node;
            printf("collection ...\n");
        }
    }

}

void gc() {
    for (int i = 0; i < _rp; ++i) {
        mark(_roots[i]);
    }
    sweep();
}

int gc_num_roots() {
    return _rp;
}

