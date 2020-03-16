//
// Created by jiangxin on 2020/3/15.
//

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include "gc.h"

#define MAX_ROOTS 100

typedef struct dept{
    class_descriptor *class;//对象对应的类型
    byte marked;//标记对象是否可达（reachable）
    int id;
} dept;

typedef struct emp{
    class_descriptor *class;//对象对应的类型
    byte marked;//标记对象是否可达（reachable）
    int id;
    dept *dept;
} emp;



class_descriptor emp_object_class = {
        "emp_object",
        sizeof (struct emp),
        1,
        (int []) {
                offsetof(struct emp, dept)
        }
};

class_descriptor dept_object_class = {
        "dept_object",
        sizeof (struct dept), /* size of string obj, not string */
        0, /* fields */
        NULL
};
typedef struct _nested_obj{
    int id;
    struct _nested_obj *nes_obj;
} nested_obj;

/**
 * 我需要在这个子方法里，更新nested_ob中的nes_obj引用
 * 同时我还需要获取nested_obj中的nes_obj对象
 * 参数该怎么传，引用又该怎么修改呢
 * @param ptr
 */
void update_ptr1(nested_obj **ptr){
    printf("..\n");
    nested_obj *obj = malloc(sizeof(struct _nested_obj));
    *ptr = obj;
}

int main(int argc, char *argv[]){
    gc_init(256*3);

    emp *_emp1 = (emp*)gc_alloc(&emp_object_class);
    gc_add_root(_emp1);
    dept *_dept1 = (dept*)gc_alloc(&dept_object_class);
    printf("pre update\n");
    gc_update_ptr(&_emp1->dept,_dept1);


    printf("pre update2\n");
    dept *_dept2 = (dept*)gc_alloc(&dept_object_class);
    gc_update_ptr(&_emp1->dept,_dept2);

    emp *_emp2 = (emp*)gc_alloc(&emp_object_class);
    gc_add_root(_emp2);

    printf("pre remove\n");
    gc_remove_root(_emp1);
}