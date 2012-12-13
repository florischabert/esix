#include "test.h"

#include <esix.h>

void esix_send(void *data, int len)
{
}

uint64_t esix_gettime(void)
{
	return 0;
}

int esix_sem_init(esix_sem_t *sem)
{
	return 0;
}
void esix_sem_destroy(esix_sem_t *sem)
{
}

void esix_sem_wait(esix_sem_t *sem, uint64_t abstime)
{
}

void esix_sem_signal(esix_sem_t *sem)
{
}

test_ret test_misc(void)
{
	return test_passed;
}