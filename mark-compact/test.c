//
// Created by jiangxin on 2020/3/15.
//

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include "./gc.h"

#define MAX_ROOTS 100

typedef struct dept {
    class_descriptor *class;//对象对应的类型
    byte marked;//是否可达
    object *forwarding;//目标位置
    int id;
} dept;

typedef struct emp {
    class_descriptor *class;//对象对应的类型
    byte marked;//是否可达
    object *forwarding;//目标位置
    int id;
    dept *dept;
} emp;


class_descriptor emp_object_class = {
        "emp_object",
        sizeof(struct emp),
        1,
        (int[]) {
                offsetof(struct emp, dept)
        }
};

class_descriptor dept_object_class = {
        "dept_object",
        sizeof(struct dept), /* size of string obj, not string */
        0, /* fields */
        NULL
};

int main(int argc, char *argv[]) {

    gc_init((emp_object_class.size + dept_object_class.size) * 3);

    for (int i = 0; i < 4; ++i) {
        printf("loop %d\n" ,i);
        emp *_emp1 = (emp *) gc_alloc(&emp_object_class);
        gc_add_root(_emp1);
        dept *_dept1 = (dept *) gc_alloc(&dept_object_class);
        _emp1->dept = _dept1;

        if (i == 2) {
            printf("即将内存溢出\n");
        }
        emp *_emp2 = (emp *) gc_alloc(&emp_object_class);
        dept *_dept2 = (dept *) gc_alloc(&dept_object_class);
        _emp2->dept = _dept2;

//        gc();
    }
}