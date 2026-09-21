#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/task_wdt/task_wdt.h>
#include <stdbool.h>

LOG_MODULE_REGISTER(homework, LOG_LEVEL_DBG);

#define STACK_SIZE               1024
#define PRODUCER_SLEEP_TIME      50
#define CONSUMER_SLEEP_TIME      100
#define WATCHDOG_TIMEOUT_MS      2000
#define CONSUMER_SHUTDOWN_MS     4000
#define NB_SAMPLES               25
#define SENSOR_QUEUE_DEPTH       10

K_SEM_DEFINE(sensor_producer_gate, 1, 1);
K_MSGQ_DEFINE(sensor_msgq, sizeof(int), SENSOR_QUEUE_DEPTH, 4);

/* Statistics */
static volatile int total_events;
static volatile int total_processed;

static void watchdog_cb(int channel_id, void *user_data)
{
    ARG_UNUSED(channel_id);
    ARG_UNUSED(user_data);

    LOG_ERR("[WATCHDOG] watchdog fired; triggering kernel oops");
    k_oops();
}

static void consumer(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    int ret = task_wdt_init(NULL);
    if (ret < 0) {
        LOG_ERR("task_wdt init failed: %d", ret);
        return;
    }

    int task_wdt_channel_id = task_wdt_add(WATCHDOG_TIMEOUT_MS, watchdog_cb, NULL);
    if (task_wdt_channel_id < 0) {
        LOG_ERR("task_wdt add failed: %d", task_wdt_channel_id);
        return;
    }

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

        ret = task_wdt_feed(task_wdt_channel_id);
        if (ret < 0) {
            LOG_ERR("task_wdt feed failed: %d", ret);
        }

        LOG_INF("[CONSUMER] watchdog fed");
        k_msleep(CONSUMER_SLEEP_TIME);
    }

    ret = task_wdt_delete(task_wdt_channel_id);
    if (ret < 0) {
        LOG_ERR("task_wdt delete failed: %d", ret);
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
        k_sem_take(&sensor_producer_gate, K_FOREVER);

        total_events++;
        int event_id = total_events;

        LOG_INF("[SENSOR] event %d", event_id);

        int ret = k_msgq_put(&sensor_msgq, &event_id, K_FOREVER);
        if (ret < 0) {
            LOG_ERR("[SENSOR] k_msgq_put failed: %d", ret);
        }

        k_sem_give(&sensor_producer_gate);

        k_msleep(PRODUCER_SLEEP_TIME);
    }

    LOG_INF("[SENSOR] Ended successfully");
}
K_THREAD_DEFINE(sensor_sim_thread,  STACK_SIZE, sensor_sim, NULL, NULL, NULL, 5, 0, 0);

static void sensor_pending_watcher(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    enum sensor_monitor_state_level {
        STATE_NORMAL_LEVEL = 0,
        STATE_WARNING_LEVEL = 7,
        STATE_CRITICAL_LEVEL = 9,
    };

    enum sensor_monitor_state {
        STATE_NORMAL,
        STATE_WARNING,
        STATE_CRITICAL,
    };

    int pending = 0;
    enum sensor_monitor_state prev_state = STATE_NORMAL, state = STATE_NORMAL;

    while (true) {
        pending = k_msgq_num_used_get(&sensor_msgq);

        prev_state = state;

        switch (state) {
        case STATE_NORMAL:
            if (pending >= STATE_WARNING_LEVEL) {
                state = STATE_WARNING;
                LOG_INF("[WATCHER] state WARNING (%d pending)", pending);
            }
            if (pending >= STATE_CRITICAL_LEVEL) {
                state = STATE_CRITICAL;
                LOG_INF("[WATCHER] state CRITICAL (%d pending)", pending);
                k_sem_take(&sensor_producer_gate, K_FOREVER);
            }
            break;

        case STATE_WARNING:
            if (pending >= STATE_CRITICAL_LEVEL) {
                state = STATE_CRITICAL;
                LOG_INF("[WATCHER] state CRITICAL (%d pending)", pending);
                k_sem_take(&sensor_producer_gate, K_FOREVER);
            }

            // Fallthrough

        case STATE_CRITICAL:
            if (pending == STATE_NORMAL_LEVEL) {
                state = STATE_NORMAL;
                LOG_INF("[WATCHER] state back to NORMAL (queue drained)");
                k_msleep(200);
                k_sem_give(&sensor_producer_gate);
            }
        }

        k_msleep(10);
    }
}
K_THREAD_DEFINE(sensor_pending_thread,  STACK_SIZE, sensor_pending_watcher, NULL, NULL, NULL, 5, 0, 0);
