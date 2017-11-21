#include <pthread.h>
#include <time.h>
#include "errors.h"

typedef struct alarm_tag { //The "alarm" structure now contains the time_t (time since the Epoch, in seconds) for each alarm, so that they can be sorted. Storing the requested number of seconds would not be enough, since the "alarm thread" cannot tell how long it has been on the list.
	struct alarm_tag* link;
	int seconds;
	time_t time; //seconds from EPOCH
	char message[64];
} alarm_t;

pthread_mutex_t alarm_mutex = PTHREAD_MUTEX_INITIALIZER;
alarm_t* alarm_list = NULL;

void* alarm_thread(void* arg) { //The alarm thread's start routine.
	printf("\nalarm_thread: 111\n");
	alarm_t* alarm;
	int sleep_time;
	time_t now;
	int status;

	while (1) { //Loop forever, processing commands. The alarm thread will be disintegrated when the process exits.
		printf("\nalarm_thread: 222\n");
		status = pthread_mutex_lock(&alarm_mutex);
		if (status != 0)
			err_abort(status, "Lock mutex");
		alarm = alarm_list;

		if (alarm == NULL) { //If the alarm list is empty, wait for one second. This allows the main thread to run, and read another command. If the list is not empty, remove the first item. Compute the number of seconds to wait — if the result is less than 0 (the time has passed), then set the sleep_time to 0.
			printf("\nalarm_thread: 333\n");
			sleep_time = /*ORIG 1*/ 3;
			printf("\nalarm_thread: 444\n");
		} else {
			printf("\nalarm_thread: 555\n");
			alarm_list = alarm->link;
			now = time(NULL);
			if (alarm->time <= now)
				sleep_time = 0;
			else
				sleep_time = alarm->time - now;
			#ifdef DEBUG
				printf("[waiting: %d(%d)\"%s\"]\n", alarm->time, sleep_time, alarm->message);
			#endif
		}

		status = pthread_mutex_unlock (&alarm_mutex); //Unlock the mutex before waiting, so that the main thread can lock it to insert a new alarm request. If the sleep_time is 0, then call sched_yield, giving the main thread a chance to run if it has been readied by user input, without delaying the message if there's no input.
		printf("\nalarm_thread: 666\n");
		if (status != 0)
			err_abort (status, "Unlock mutex");
		if (sleep_time > 0) {
			printf("\nalarm_thread: 777\n");
			sleep(sleep_time);
			printf("\nalarm_thread: 888\n");
		} else {
			printf("\nalarm_thread: 999\n");
			sched_yield();
			printf("\nalarm_thread: 100\n");
		}

		if (alarm != NULL) { //If a timer expired, print the message and free the structure.
			printf ("(%d) %s\n", alarm->seconds, alarm->message);
			free (alarm);
		}
	}
}

int main(int argc, char *argv[]) {
	int status;
	char line[128];
	alarm_t *alarm, **last, *next;
	pthread_t thread;

	printf("\nmain: 111\n");
	status = pthread_create(&thread, NULL, alarm_thread, NULL);
	printf("\nmain: 222\n");
	if (status != 0)
		err_abort (status, "Create alarm thread");
	while (1) {
		printf("Alarm> ");
		if (fgets (line, sizeof (line), stdin) == NULL)
			exit (0);
		if (strlen (line) <= 1)
			continue;
		printf("\nmain: 333\n");
		alarm = (alarm_t*)malloc(sizeof(alarm_t));
		printf("\nmain: 444\n");
		if (alarm == NULL)
			("Allocate alarm");

		if (sscanf(line, "%d %64[^\n]", &alarm->seconds, alarm->message) < 2) { //Parse input line into seconds (%d) and a message (%64[^\n]), consisting of up to 64 characters separated from the seconds by whitespace.
			fprintf(stderr, "Bad command\n");
			free(alarm);
		} else {
			printf("\nmain: 555\n");
			status = pthread_mutex_lock(&alarm_mutex);
			if (status != 0)
				err_abort(status, "Lock mutex");
			alarm->time = time(NULL) + alarm->seconds;

			last = &alarm_list; // Insert the new alarm into the list of alarms, sorted by expiration time.
			next = *last;
			while (next != NULL) {
				printf("\nmain: 666\n");
				if (next->time >= alarm->time) {
					alarm->link = next;
					*last = alarm;
					break;
					}
				last = &next->link;
				next = next->link;
				}

			if (next == NULL) { //If we reached the end of the list, insert the new alarm there, ("next" is NULL, and "last" points to the link field of the last item, or to the list header).
				*last = alarm;
				alarm->link = NULL;
				}
			#ifdef DEBUG
				printf ("[list: ");
				for (next = alarm_list; next != NULL; next = next->link)
					printf ("%d(%d)[\"%s\"] ", next->time, next->time - time (NULL), next->message);
				printf ("]\n");
			#endif
			status = pthread_mutex_unlock (&alarm_mutex);
			if (status != 0)
				err_abort (status, "Unlock mutex");
			}
		}
	}
