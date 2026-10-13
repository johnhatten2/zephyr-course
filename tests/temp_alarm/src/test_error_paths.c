/*
 * temp_alarm - Error-path unit tests (homework skeleton)
 *
 * Task 2: fill in the stubs according to TEST_SPEC.md. Each stub calls
 * ztest_test_skip(), so the binary builds and runs cleanly until the test is
 * implemented.
 *
 * Run:
 *   west twister -T tests/temp_alarm -p native_sim
 */

#include <errno.h>
#include <zephyr/fff.h>
#include <zephyr/ztest.h>

#include "sensor_fake.h"
#include "temp_alarm.h"

#define DEFAULT_THRESHOLD 30

/* Same before() hook as in test_basic.c: a clean fake and an initialised module. */
static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	sensor_fake_reset();
	sensor_fake_init();

	zassume_ok(temp_alarm_init(&fake_sensor_dev, DEFAULT_THRESHOLD),
		   "precondition: temp_alarm_init must succeed");

	// All tests starts with at least 1 good measurement
	sensor_fake_set_temperature(5);
	temp_alarm_check();
}

/*
 * ============================================================================
 * Test Suite: temp_alarm_sensor_errors
 *
 * Failures from the sensor driver, injected through the fakes.
 * ============================================================================
 */
ZTEST_SUITE(temp_alarm_sensor_errors, NULL, NULL, before, NULL, NULL);

ZTEST(temp_alarm_sensor_errors, test_fetch_error_propagates)
{
	struct temp_alarm_status status;

	sensor_fake_set_temperature(6);
	fake_sensor_sample_fetch_fake.return_val = -EIO;

	zassert_equal(temp_alarm_check(), -EIO, "check must propagate fetch error");
	zassert_ok(temp_alarm_get_status(&status), "get_status must propagate fetch error");
	zassert_equal(status.last_error, -EIO, "last_error must be set to fetch error");
	zassert_equal(status.last_temp, 5, "last_temp must be 5 after fetch error, got %d", status.last_temp);
}

ZTEST(temp_alarm_sensor_errors, test_channel_get_error_propagates)
{
	struct temp_alarm_status status;

	sensor_fake_set_temperature(6);
	fake_sensor_channel_get_fake.return_val = -EIO;

	zassert_equal(temp_alarm_check(), -EIO, "check must propagate channel_get error");
	zassert_ok(temp_alarm_get_status(&status), "get_status must propagate channel_get error");
	zassert_equal(status.last_error, -EIO, "last_error must be set to channel_get error");
	zassert_equal(status.last_temp, 5, "last_temp must be 5 after channel_get error, got %d", status.last_temp);
}

ZTEST(temp_alarm_sensor_errors, test_fail_then_recover)
{
	struct temp_alarm_status status;

	sensor_fake_set_temperature(6);
	fake_sensor_sample_fetch_fake.return_val = -EIO;

	zassert_equal(temp_alarm_check(), -EIO, "check must propagate fetch error");
	zassert_ok(temp_alarm_get_status(&status), "get_status must propagate fetch error");
	zassert_equal(status.last_error, -EIO, "last_error must be set to fetch error");
	zassert_equal(status.last_temp, 5, "last_temp must be 5 after fetch error, got %d", status.last_temp);

	// Recover: clear the error and check again
	sensor_fake_set_temperature(20);
	fake_sensor_sample_fetch_fake.return_val = 0;

	zassert_ok(temp_alarm_check(), "check must succeed after recovery");
	zassert_ok(temp_alarm_get_status(&status), "get_status must succeed after recovery");
	zassert_equal(status.last_error, -EIO, "last_error must be 0 after recovery, got %d", status.last_error);
	zassert_false(status.is_alarming, "is_alarming must be false after recovery");
	zassert_equal(status.last_temp, 20, "last_temp must be 20 after recovery, got %d", status.last_temp);
}

/*
 * ============================================================================
 * Test Suite: temp_alarm_invalid_input
 *
 * Arguments the public API must reject.
 * ============================================================================
 */
ZTEST_SUITE(temp_alarm_invalid_input, NULL, NULL, before, NULL, NULL);

ZTEST(temp_alarm_invalid_input, test_get_status_null)
{
	zassert_equal(temp_alarm_get_status(NULL), -EINVAL, "get_status must reject NULL");
}

ZTEST(temp_alarm_invalid_input, test_init_without_device)
{
	zassert_equal(temp_alarm_init(NULL, DEFAULT_THRESHOLD), -ENODEV, "init must reject NULL device");
}

ZTEST(temp_alarm_invalid_input, test_init_threshold_out_of_range)
{
	zassert_equal(temp_alarm_init(&fake_sensor_dev, TEMP_ALARM_THRESHOLD_MIN - 1), -EINVAL,
		      "init must reject threshold below min");
	zassert_equal(temp_alarm_init(&fake_sensor_dev, TEMP_ALARM_THRESHOLD_MAX + 1), -EINVAL,
		      "init must reject threshold above max");
}

ZTEST(temp_alarm_invalid_input, test_set_threshold_out_of_range)
{
	zassert_equal(temp_alarm_set_threshold(TEMP_ALARM_THRESHOLD_MIN - 1), -EINVAL,
		      "set_threshold must reject threshold below min");
	zassert_equal(temp_alarm_set_threshold(TEMP_ALARM_THRESHOLD_MAX + 1), -EINVAL,
		      "set_threshold must reject threshold above max");
}
