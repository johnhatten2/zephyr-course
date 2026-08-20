#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(l1_task1);

#define STACK_SIZE 1024

void t_low_fn(void *arg1, void *arg2, void *arg3)
{
	LOG_INF("[T_LOW] Start - Priority: %d", k_thread_priority_get(k_current_get()));

	while (1) {
		LOG_INF("[T_LOW] Running");
		k_msleep(300);
	}
}

void t_med_fn(void *arg1, void *arg2, void *arg3)
{
	LOG_INF("[T_MED] Start - Priority: %d", k_thread_priority_get(k_current_get()));

	while (1) {
		LOG_INF("[T_MED] Running");
		k_msleep(200);
	}
}

void t_high_fn(void *arg1, void *arg2, void *arg3)
{
	LOG_INF("[T_HIGH] Start - Priority: %d", k_thread_priority_get(k_current_get()));

	while (1) {
		LOG_INF("[T_HIGH] Running");
		k_msleep(100);
	}
}

void t_coop_fn(void *arg1, void *arg2, void *arg3)
{
	LOG_INF("[T_COOP] Start - Priority: %d", k_thread_priority_get(k_current_get()));

	for (int i = 0; i < 5; i++) {
		LOG_INF("[T_COOP] Iteration %d", i + 1);
		/* Busy work */
		k_busy_wait(500000);
        k_yield();
	}

	LOG_INF("[T_COOP] Yielding");
	k_yield();
	LOG_INF("[T_COOP] Exiting");
}

void t_coop2_fn(void *arg1, void *arg2, void *arg3)
{
	LOG_INF("[T_COOP2] Start - Priority: %d", k_thread_priority_get(k_current_get()));

	for (int i = 0; i < 5; i++) {
		LOG_INF("[T_COOP2] Iteration %d", i + 1);
		/* Busy work */
		k_busy_wait(500000);
        k_yield();
	}

	LOG_INF("[T_COOP2] Yielding");
	k_yield();
	LOG_INF("[T_COOP2] Exiting");
}

K_THREAD_DEFINE(thread_1, STACK_SIZE, t_low_fn, NULL, NULL, NULL, 7, 0, 0);
K_THREAD_DEFINE(thread_2, STACK_SIZE, t_med_fn, NULL, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(thread_3, STACK_SIZE, t_high_fn, NULL, NULL, NULL, 3, 0, 0);
K_THREAD_DEFINE(thread_4, STACK_SIZE, t_coop_fn, NULL, NULL, NULL, -1, 0, 0);
K_THREAD_DEFINE(thread_5, STACK_SIZE, t_coop2_fn, NULL, NULL, NULL, -2, 0, 0);

int main(void)
{
	return 0;
}
