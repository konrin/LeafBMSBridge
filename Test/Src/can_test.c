#include "unity.h"
#include "can.h"

void setUp(void) {
    // set stuff up here
}

void tearDown(void) {
    // clean stuff up here
}

void test_function_should_doBlahAndBlah(void) {
    Can1DBMessage_t message;

    uint8_t data[8] = {0x01, 0x60, 0xC3, 0x6A, 0x00, 0x00, 0x00, 0xF7};
    CanDecode1DBMessage(&message, data);

    TEST_ASSERT_EQUAL_FLOAT(390.5, message.voltage);
    TEST_ASSERT_EQUAL_FLOAT(5.5, message.current);
}

void test_function_should_doAlsoDoBlah(void) {
    //more test stuff
}

// not needed when using generate_test_runner.rb
int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_function_should_doBlahAndBlah);
    RUN_TEST(test_function_should_doAlsoDoBlah);
    return UNITY_END();
}
