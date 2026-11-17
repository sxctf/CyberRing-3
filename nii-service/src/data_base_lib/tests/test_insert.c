#include <check.h>
#include <stdlib.h>

#include "../data_base_lib.h"

START_TEST(insert_1) {
    FILE* fd = fopen("./tests/build/data", "w+b");
    ENTITY in;
    memset(&in, 0, sizeof(ENTITY));

    strcpy(in.d.name, "name1");
    in.d.level = 4;
    strcpy(in.d.author, "usr1");
    strcpy(in.d.data, "d1");

    int error = insert(fd, &in, sizeof(struct data));
    ck_assert_int_eq(error, 0);

    if (fd) fclose(fd);
}
END_TEST

START_TEST(insert_2) {
    FILE* fd = fopen("./tests/build/data", "r+b");
    ENTITY in;
    memset(&in, 0, sizeof(ENTITY));

    strcpy(in.d.name, "name2");
    in.d.level = 3;
    strcpy(in.d.author, "usr2");
    strcpy(in.d.data, "d2");

    int error = insert(fd, &in, sizeof(struct data));

    ck_assert_int_eq(error, 0);

    if (fd) fclose(fd);
}
END_TEST

START_TEST(insert_3) {
    FILE* fd = fopen("./tests/build/data", "r+b");
    ENTITY in;
    memset(&in, 0, sizeof(ENTITY));

    strcpy(in.d.name, "name2");
    in.d.level = 4;
    strcpy(in.d.author, "usr3");
    strcpy(in.d.data, "d3");

    int error = insert(fd, &in, sizeof(struct data));

    ck_assert_int_eq(error, -1);

    if (fd) fclose(fd);
}
END_TEST

// Добавление тестов в Suite
Suite* insert_suite(void) {
    Suite* s = suite_create("insert");
    TCase* tc = tcase_create("insert_tcase");

    tcase_add_test(tc, insert_1);
    tcase_add_test(tc, insert_2);
    tcase_add_test(tc, insert_3);
    suite_add_tcase(s, tc);

    return s;
}

int main(void) {
    Suite* s = insert_suite();
    SRunner* sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);

    int failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return failed;
}
