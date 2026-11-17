#include <check.h>
#include <stdlib.h>

#include "../data_base_lib.h"

START_TEST(select_name_1) {
    FILE* fd = fopen("./tests/build/data", "rb");

    /* sample */
    ENTITY sample;
    memset(&sample, 0, sizeof(ENTITY));

    strcpy(sample.d.name, "name1");
    sample.d.level = 4;
    strcpy(sample.d.author, "usr1");
    strcpy(sample.d.data, "d1");

    /* select */
    int offset = 0, diff = 0;
    ENTITY* out = select_name(fd, "name1", sizeof(struct data), &offset);
    ck_assert_ptr_nonnull(out);
    if (fd) fclose(fd);

    /* check */
    if (out) {
        diff = memcmp(out->d.name, sample.d.name, MAX_NAME);
        ck_assert_str_eq(out->d.name, sample.d.name);
        ck_assert_int_eq(diff, 0);

        ck_assert_int_eq(out->d.level, 4);

        diff = memcmp(out->d.author, sample.d.author, MAX_NAME);
        ck_assert_str_eq(out->d.author, sample.d.author);
        ck_assert_int_eq(diff, 0);

        diff = memcmp(out->d.data, sample.d.data, MAX_DATA);
        ck_assert_str_eq(out->d.name, sample.d.name);
        ck_assert_int_eq(diff, 0);

        free(out);
    }
}
END_TEST

START_TEST(select_name_2) {
    FILE* fd = fopen("./tests/build/data", "rb");

    /* sample */
    ENTITY sample;
    memset(&sample, 0, sizeof(ENTITY));

    strcpy(sample.d.name, "name2");
    sample.d.level = 4;
    strcpy(sample.d.author, "usr2");
    strcpy(sample.d.data, "d2");

    /* select */
    int offset = 0, diff = 0;
    ENTITY* out = select_name(fd, "name2", sizeof(struct data), &offset);
    ck_assert_ptr_nonnull(out);
    if (fd) fclose(fd);

    /* check */
    if (out) {
        diff = memcmp(out->d.name, sample.d.name, MAX_NAME);
        ck_assert_str_eq(out->d.name, sample.d.name);
        ck_assert_int_eq(diff, 0);

        ck_assert_int_eq(out->d.level, 3);

        diff = memcmp(out->d.author, sample.d.author, MAX_NAME);
        ck_assert_str_eq(out->d.author, sample.d.author);
        ck_assert_int_eq(diff, 0);

        diff = memcmp(out->d.data, sample.d.data, MAX_DATA);
        ck_assert_str_eq(out->d.name, sample.d.name);
        ck_assert_int_eq(diff, 0);

        free(out);
    }
}
END_TEST

START_TEST(select_name_3) {
    FILE* fd = fopen("./tests/build/data", "rb");

    /* sample */
    ENTITY sample;
    memset(&sample, 0, sizeof(ENTITY));

    strcpy(sample.d.name, "name3");
    sample.d.level = 4;
    strcpy(sample.d.author, "usr3");
    strcpy(sample.d.data, "dddd");

    /* select */
    int offset = 0;
    ENTITY* out = select_name(fd, "name3", sizeof(struct data), &offset);
    ck_assert_ptr_null(out);

    if (fd) fclose(fd);
}
END_TEST

// Добавление тестов в Suite
Suite* insert_suite(void) {
    Suite* s = suite_create("select");
    TCase* tc = tcase_create("select_name");

    tcase_add_test(tc, select_name_1);
    tcase_add_test(tc, select_name_2);
    tcase_add_test(tc, select_name_3);
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
