#include <check.h>
#include <stdlib.h>

#include "../data_base_lib.h"

START_TEST(update_1) {
    FILE* fd = fopen("./tests/build/data", "r+b");
    ENTITY x;
    memset(&x, 0, sizeof(ENTITY));

    strcpy(x.d.name, "name1");
    x.d.level = 1;
    strcpy(x.d.author, "us1");
    strcpy(x.d.data, "u1");

    int err = update(fd, &x, sizeof(struct data));
    ck_assert_int_eq(err, 0);

    int offset = 0;
    ENTITY* upde = select_name(fd, x.d.name, sizeof(struct data), &offset);
    if (upde != NULL) {
        ck_assert_int_eq(upde->d.level, x.d.level);
        ck_assert_str_eq(upde->d.data, x.d.data);
        ck_assert_str_eq(upde->d.author, x.d.author);
        free(upde);
    }

    fclose(fd);
}
END_TEST

START_TEST(update_2) {
    FILE* fd = fopen("./tests/build/data", "r+b");
    ENTITY x;
    memset(&x, 0, sizeof(ENTITY));

    strcpy(x.d.name, "name2");
    x.d.level = 2;
    strcpy(x.d.author, "us2");
    strcpy(x.d.data, "u2");

    int err = update(fd, &x, sizeof(struct data));
    ck_assert_int_eq(err, 0);

    int offset = 0;
    ENTITY* upde = select_name(fd, x.d.name, sizeof(struct data), &offset);
    if (upde != NULL) {
        ck_assert_int_eq(upde->d.level, x.d.level);
        ck_assert_str_eq(upde->d.data, x.d.data);
        ck_assert_str_eq(upde->d.author, x.d.author);
        free(upde);
    }

    fclose(fd);
}
END_TEST

START_TEST(update_3) {
    FILE* fd = fopen("./tests/build/data", "r+b");
    ENTITY x;
    memset(&x, 0, sizeof(ENTITY));

    strcpy(x.d.name, "noexist");
    x.d.level = 4;
    strcpy(x.d.data, "u3");

    int err = update(fd, &x, sizeof(struct data));
    ck_assert_int_eq(err, -1);

    fclose(fd);
}
END_TEST

// Добавление тестов в Suite
Suite* insert_suite(void) {
    Suite* s = suite_create("update");
    TCase* tc = tcase_create("update_tcase");

    tcase_add_test(tc, update_1);
    tcase_add_test(tc, update_2);
    tcase_add_test(tc, update_3);
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
