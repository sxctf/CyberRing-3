#include <check.h>
#include <stdlib.h>

#include "../data_base_lib.h"

START_TEST(select_level_1) {
    FILE* fd = fopen("./tests/build/data", "rb");

    /* select */
    int offset = 0;
    int lvl = 4;
    ENTITY* out = select_level(fd, lvl, sizeof(struct data), &offset);
    ck_assert_ptr_nonnull(out);
    if (fd) fclose(fd);

    /* check */
    if (out) {
        ck_assert_int_eq(out->d.level, lvl);
        free(out);
    }
}
END_TEST

START_TEST(select_level_2) {
    FILE* fd = fopen("./tests/build/data", "rb");

    /* select */
    int offset = 0;
    int lvl = 3;
    ENTITY* out = select_level(fd, lvl, sizeof(struct data), &offset);
    ck_assert_ptr_nonnull(out);
    if (fd) fclose(fd);

    /* check */
    if (out) {
        ck_assert_int_eq(out->d.level, lvl);
        free(out);
    }
}
END_TEST

START_TEST(select_level_3) {
    FILE* fd = fopen("./tests/build/data", "rb");

    int offset = 0;
    int lvl = -100;
    ENTITY* out = select_level(fd, lvl, sizeof(struct data), &offset);
    ck_assert_ptr_null(out);

    if (fd) fclose(fd);
    if (out) free(out);
}
END_TEST

// Добавление тестов в Suite
Suite* insert_suite(void) {
    Suite* s = suite_create("select");
    TCase* tc = tcase_create("select_level");

    tcase_add_test(tc, select_level_1);
    tcase_add_test(tc, select_level_2);
    tcase_add_test(tc, select_level_3);
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
