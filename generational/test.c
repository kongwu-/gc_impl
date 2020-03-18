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
    byte forwarded;//已拷贝标识
    byte marked;//reachable标识
    byte remembered;//已经记录在rs中的标识
    object *forwarding;//目标位置
    int age;//对象年龄
    int id;
} dept;

typedef struct emp {
    class_descriptor *class;//对象对应的类型
    byte forwarded;//已拷贝标识
    byte marked;//reachable标识
    byte remembered;//已经记录在rs中的标识
    object *forwarding;//目标位置
    int age;//对象年龄
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

/**
 * 测试新生代GC
 */
void test_minor_gc(){
    gc_init(2000);//分配后，实际可用1936

    emp *_emp1 = (emp *) gc_alloc(&emp_object_class);
    gc_add_root(_emp1);
    dept *_dept1 = (dept *) gc_alloc(&dept_object_class);

    //增加此引用会导致survivor容量不足，复制失败
    gc_update_ptr(_emp1,&_emp1->dept,_dept1);

    for (int i = 0; i < 6; ++i) {
        emp *temp_emp = (emp *) gc_alloc(&emp_object_class);
    }

    printf("即将新生代GC\n");

    emp *temp_emp = (emp *) gc_alloc(&emp_object_class);

    gc_get_state();

}

/**
 * 测试晋升
 */
void test_promotion(){
    gc_init(2000);//分配后，实际可用1936

    emp *_emp1 = (emp *) gc_alloc(&emp_object_class);
    gc_add_root(_emp1);

    //触发3次新生代GC，然后emp1会晋升至老年代
    for (int i = 0; i < 31; ++i) {
        emp *temp_emp = (emp *) gc_alloc(&emp_object_class);
    }

    printf("emp1晋升\n");
    emp *temp_emp = (emp *) gc_alloc(&emp_object_class);

    gc_get_state();

}

/**
 * 测试晋升引发的老年代GC
 */
void test_promotion_old_gc(){
    gc_init(2000);//分配后，实际可用1936

    for (int j = 0; j < 12; j++) {
        emp *_emp1 = (emp *) gc_alloc(&emp_object_class);
        _emp1->id = 666;
        gc_add_root(_emp1);


        //分配新对象，触发新生代GC
        for (int i = 0; i < 31; i++) {
            emp *temp_emp = (emp *) gc_alloc(&emp_object_class);
        }

    }

    printf("老年代GC\n");

    emp *_emp1 = (emp *) gc_alloc(&emp_object_class);

    gc_add_root(_emp1);

    swap((void **)&_roots[0], (void **)(&_roots[--_rp]));


    //分配新对象，触发新生代GC
    for (int i = 0; i < 32; i++) {
        emp *temp_emp = (emp *) gc_alloc(&emp_object_class);
    }
    gc_get_state();


    printf("FULL GC...\n");
    _rp = 1;
    gc();
    gc_get_state();
}

/**
 * 测试跨代引用的回收
 */
void test_cross_gen_ref_gc(){
    gc_init(2000);//分配后，实际可用1936

    emp *_emp1 = (emp *) gc_alloc(&emp_object_class);
    gc_add_root(_emp1);

    //触发3次新生代GC，然后emp1会晋升至老年代
    for (int i = 0; i < 31; ++i) {
        emp *temp_emp = (emp *) gc_alloc(&emp_object_class);
    }

    printf("emp1晋升\n");

    emp *temp_emp = (emp *) gc_alloc(&emp_object_class);

    emp *dept1 = (dept *) gc_alloc(&dept_object_class);

    gc_update_ptr(_roots[0],&_emp1->dept,dept1);

    //此处有问题，新生代对象dept1丢失了
    gc();

    gc_get_state();
}

/**
 * 测试因晋升导致的跨代引用的回收
 */
void test_promotion_cross_gen_ref_gc(){

}

int main(int argc, char *argv[]) {
    test_cross_gen_ref_gc();
}