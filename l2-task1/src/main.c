#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <stdlib.h>

LOG_MODULE_REGISTER(l2_task1);

#define STACK_SIZE 1024

#define MAX_COUNT 100000

static volatile int counter = 0;
K_SEM_DEFINE(sem, 0, 2);
K_MUTEX_DEFINE(counter_mutex);

void might_yield(void)
{
	int rand_val = rand() % 100;
	if (rand_val == 0) {
		k_yield();
	}
}

void non_atomic_counter_increment(void)
{
    // Simulate the non-atomic increment operation with potential yielding
    int temp = counter;
    might_yield();
    temp++;
    might_yield();
    counter = temp;
}

void task_handler(void *arg1, void *arg2, void *arg3)
{
	for (int i = 0; i < MAX_COUNT; i++) {

        k_mutex_lock(&counter_mutex, K_FOREVER);
        non_atomic_counter_increment();
        k_mutex_unlock(&counter_mutex);

        for (int j = 0; j < 100; j++) {
            // More ods it yields elsewhere in the code
            might_yield();
        }
	}
	k_sem_give(&sem);
}

K_THREAD_DEFINE(task_1, STACK_SIZE, task_handler, NULL, NULL, NULL, -1, 0, 0);
K_THREAD_DEFINE(task_2, STACK_SIZE, task_handler, NULL, NULL, NULL, -1, 0, 0);

int main(void)
{
	k_sem_take(&sem, K_FOREVER);
	k_sem_take(&sem, K_FOREVER);

	LOG_INF("Total count: %d. Lost: %d", counter, 2 * MAX_COUNT - counter);
	
	return 0;
}
