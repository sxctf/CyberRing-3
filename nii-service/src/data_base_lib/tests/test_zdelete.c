#include <check.h>
#include <stdlib.h>

#include "../data_base_lib.h"

START_TEST(delete_1) {
    FILE* fd = fopen("./tests/build/data", "r+b");
    const char* name = "name1";

    int err = delete(fd, name, sizeof(struct data));
    ck_assert_int_eq(err, 0);

    int offset = 0;
    ck_assert_ptr_null(select_name(fd, name, sizeof(struct data), &offset));

    if (fd) fclose(fd);
}
END_TEST

START_TEST(delete_2) {
    FILE* fd = fopen("./tests/build/data", "r+b");
    const char* name = "name2";

    int err = delete(fd, name, sizeof(struct data));
    ck_assert_int_eq(err, 0);

    int offset = 0;
    ck_assert_ptr_null(select_name(fd, name, sizeof(struct data), &offset));

    if (fd) fclose(fd);
}

END_TEST

START_TEST(delete_3) {
    FILE* fd = fopen("./tests/build/data", "r+b");
    const char* name = "noexist";

    int err = delete(fd, name, sizeof(struct data));
    ck_assert_int_eq(err, -1);

    if (fd) fclose(fd);
}
END_TEST

// Добавление тестов в Suite
Suite* insert_suite(void) {
    Suite* s = suite_create("delete");
    TCase* tc = tcase_create("delete_tcase");

    tcase_add_test(tc, delete_1);
    tcase_add_test(tc, delete_2);
    tcase_add_test(tc, delete_3);
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
