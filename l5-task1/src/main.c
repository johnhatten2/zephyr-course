#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <stdbool.h>

LOG_MODULE_REGISTER(homework, LOG_LEVEL_DBG);

#define STACK_SIZE               1024
#define PRODUCER_SLEEP_TIME      50
#define CONSUMER_SLEEP_TIME      100
#define CONSUMER_SHUTDOWN_MS     4000
#define NB_SAMPLES               25
#define SENSOR_QUEUE_DEPTH       10

K_MSGQ_DEFINE(sensor_msgq, sizeof(int), SENSOR_QUEUE_DEPTH, 4);

/* Statistics */
static volatile int total_events;
static volatile int total_processed;

static void consumer(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    int ret;

    LOG_INF("[CONSUMER] sensor task started");

    while (k_uptime_get() < CONSUMER_SHUTDOWN_MS) {
        int event_id = 0;

        while (k_msgq_peek(&sensor_msgq, &event_id) == 0) {
            ret = k_msgq_get(&sensor_msgq, &event_id, K_NO_WAIT);
            if (ret < 0) {
                break;
            }

            total_processed++;
            LOG_INF("[CONSUMER] processing event %d", event_id);
            k_msleep(CONSUMER_SLEEP_TIME);
        }

        k_msleep(CONSUMER_SLEEP_TIME);
    }
}
K_THREAD_DEFINE(consumer_thread, STACK_SIZE, consumer, NULL, NULL, NULL, 5, 0, 0);

static void sensor_sim(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    LOG_INF("[SENSOR] Generating %d samples", NB_SAMPLES);

    for (int i = 0; i < NB_SAMPLES; i++) {
        total_events++;
        int event_id = total_events;

        LOG_INF("[SENSOR] event %d", event_id);

        int ret = k_msgq_put(&sensor_msgq, &event_id, K_FOREVER);
        if (ret < 0) {
            LOG_ERR("[SENSOR] k_msgq_put failed: %d", ret);
        }

        k_msleep(PRODUCER_SLEEP_TIME);
    }

    LOG_INF("[SENSOR] Ended successfully");
}
K_THREAD_DEFINE(sensor_sim_thread,  STACK_SIZE, sensor_sim, NULL, NULL, NULL, 5, 0, 0);
