#include <pthread.h>
#include <time.h>
#include "errors.h"

typedef struct alarm_tag { //The "alarm" structure now contains the time_t for each alarm, so that they can be sorted. Storing the requested number of seconds would not be enough, since the "alarm thread" cannot tell how long it has been on the list.
	struct alarm_tag* link;
	int seconds;
	time_t time; //seconds from EPOCH
	char message[64];
} alarm_t;

pthread_mutex_t alarm_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t alarm_cond = PTHREAD_COND_INITIALIZER; //20,22: Part 1 shows the declarations for alarm_cond.c. There are two additions to this section, compared to alarm_mutex.c: a condition variable called alarm_cond...
alarm_t* alarm_list = NULL;
time_t current_alarm = 0; //20,22: ...and the current_alarm variable, which allows main to determine the expiration time of the alarm on which alarm_thread is currently waiting. The current_alarm variable is an optimization — main does not need to awaken alarm_thread unless it is either idle, or waiting for an alarm later than the one main has just inserted.

//Part 2 shows the new function alarm_insert. This function is nearly the same as the list insertion code from alarm_mutex.c, except that it signals the condition variable alarm_cond when necessary. I made alarm_insert a separate function because now it needs to be called from two places — once by main to insert a new alarm, and now also by alarm_thread to reinsert an alarm that has been "preempted" by a new earlier alarm.
void alarm_insert(alarm_t* alarm) { // Insert alarm entry on list, in order.
	int status;
	alarm_t **last, *next;
	//LOCKING PROTOCOL: This routine requires that the caller have locked the alarm_mutex! //9-14 I have recommended that mutex locking protocols be documented, and here is an example: The alarm_insert function points out explicitly that it must be called with the alarm_mutex locked.
	last = &alarm_list;
	next = *last;
	while (next != NULL) {
		if (next->time >= alarm->time) {
			alarm->link = next;
			*last = alarm;
			break;
		}
		last = &next->link;
		next = next->link;
	}

	if (next == NULL) { //If we reached the end of the list, insert the new alarm there.("next" is NULL, and "last" points to the link field of the last item, or to the list header.)
		*last = alarm;
		alarm->link = NULL;
	}
#ifdef DEBUG
	printf("[list: ");
	for(next = alarm_list; next != NULL; next = next->link)
		printf("%d(%d)[\"%s\"] ", next->time, next->time - time(NULL), next->message);
	printf("]\n");
#endif

	//Wake the alarm thread if it is not busy(that is, if current_alarm is 0, signifying that it's waiting for work), or if the new alarm comes before the one on which the alarm thread is waiting.
	if (current_alarm == 0 || alarm->time < current_alarm) { //48-53: If current_alarm(the time of the next alarm expiration) is 0. then the alarm_thread is not aware of any outstanding alarm requests, and is waiting for new work. If current_alarm has a time greater than the expiration time of the new alarm, then alarm_thread is not planning to look for new work soon enough to handle the new alarm. In either case, signal the alarm_cond condition variable so that alarm_thread will wake up and process the new alarm.
		current_alarm = alarm->time;
		status = pthread_cond_signal(&alarm_cond);
		if (status != 0)
			err_abort(status, "Signal cond");
	} //53
}

//Part 3 shows the alarm_thread function, the start function for the "alarm server" thread. The general structure of alarm_thread is very much like the alarm_thread in alarm_mutex.c. The differences are due to the addition of the condition variable.
void* alarm_thread(void* arg) { /*printf("\nalarm_thread 1 \t alarm_thread begins. \n");*/ // The alarm thread's start routine.
	alarm_t* alarm;
	struct timespec cond_time;
	time_t now;
	int status, expired;

	status = pthread_mutex_lock(&alarm_mutex); if (status != 0) err_abort(status, "Lock mutex"); /*printf("\nalarm_thread 1 \t Before while. \n");*/ //Loop forever, processing commands. The alarm thread will be disintegrated when the process exits. Lock the mutex at the start — it will be unlocked during condition waits, so the main thread can insert alarms.
	while (1) { /*printf("\nalarm_thread 2 \t After while. \n");*/
		//If the alarm list is empty, wait until an alarm is added. Setting current_alarm to 0 informs the insert routine that the thread is not busy.
		current_alarm = 0; printf("\nalarm_thread 1 \t Before while (alarm_list == NULL). \n"); //26 //26-31 If the alarm_list is empty, alarm_mutex.c could do nothing but sleep anyway, so that main would be able to process a new command. The result was that it could not see a new alarm request for at least a full second. Now, alarm_thread instead waits on the alarm_cond condition variable, with no timeout. It will "sleep" until you enter a new alarm command, and then main will be able to awaken it immediately. Setting current_alarm to 0 tells main that alarm_thread is idle. Remember that pthread_cond_wait unlocks the mutex before waiting, and relocks the mutex before returning to the caller.
		while (alarm_list == NULL) { printf("alarm_thread 2 \t Before pthread_cond_wait. \n");
			status = pthread_cond_wait(&alarm_cond, &alarm_mutex); printf("\nalarm_thread 3 \t After pthread_cond_wait. \n"); if (status != 0) err_abort(status, "Wait on cond");
		} printf("alarm_thread 4 \t After while (alarm_list == NULL). \n"); //31
		alarm = alarm_list;
		alarm_list = alarm->link;
		now = time(NULL);
		expired = 0; //35 The new variable expired is initialized to 0; it will be set to 1 later if the timed condition wait expires. This makes it a little easier to decide whether to print the current alarm's message at the bottom of the loop.
		if (alarm->time > now) { //36-42 If the alarm we've just removed from the list hasn't already expired, then we need to wait for it. Because we're using a timed condition wait, which requires a POSIX.1b struct timespec, rather than the simple integer time required by sleep, we convert the expiration time. This is easy, because a struct timespec has two members—tv_sec is the number of seconds since the Epoch, which is exactly what we already have from the time function, and tv_nsec is an additional count of nanoseconds. We will just set tv_nsec to 0, since we have no need of the greater resolution.
#ifdef DEBUG
			printf("[waiting: %d(%d)\"%s\"]\n", alarm->time, alarm->time - time(NULL), alarm->message);
#endif
			cond_time.tv_sec = alarm->time;
			cond_time.tv_nsec = 0; //42
			current_alarm = alarm->time; //43 Record the expiration time in the current_alarm variable so that main can determine whether to signal alarm_cond when a new alarm is added.
			while (current_alarm == alarm->time) { printf("alarm_thread 5 \t Before pthread_cond_timedwait. \n\n"); //44-53 Wait until either the current alarm has expired, or main requests that alarm_ thread look for a new, earlier alarm. Notice that the predicate test is split here, for convenience. The expression in the while statement is only half the predicate, detecting that main has changed current_alarm by inserting an earlier timer. When the timed wait returns ETIMEDOUT, indicating that the current alarm has expired, we exit the while loop with a break statement at line 49.
				status = pthread_cond_timedwait(&alarm_cond, &alarm_mutex, &cond_time); printf("alarm_thread 6 \t After pthread_cond_timedwait. \n");
				if (status == ETIMEDOUT) { printf("alarm_thread 7 \t status == ETIMEDOUT. \n");
					expired = 1;
					break;
				}
				if (status != 0) err_abort(status, "Cond timedwait");
			} //53
			if (!expired) //54-55 If the while loop exited when the current alarm had not expired, main must have asked alarm_thread to process an earlier alarm. Make sure the current alarm isn't lost by reinserting it onto the list.
				alarm_insert(alarm); //55
		} else
			expired = 1; //57 If we remove from alarm_list an alarm that has already expired, just set the expired variable to 1 to ensure that the message is printed.
		if (expired) {
			printf("(%d) %s\n", alarm->seconds, alarm->message);
			free(alarm);
		}
	}
}

//Part 4 shows the final section of alarm_cond.c, the main program. It is nearly identical to the main function from alarm_mutex.c.
int main(int argc, char* argv[]) {
	int status;
	char line[128];
	alarm_t* alarm;
	pthread_t thread; /*printf("main 1 \t\t Before pthread_create. \n");*/

	status = pthread_create(&thread, NULL, alarm_thread, NULL); /*printf("main 2 \t\t After pthread_create. \n");*/	if (status != 0) err_abort(status, "Create alarm thread");

	while (1) {
		printf("Alarm> ");
		if (fgets(line, sizeof(line), stdin) == NULL) exit(0);
		if (strlen(line) <= 1) continue;
		alarm =(alarm_t*)malloc(sizeof(alarm_t)); if (alarm == NULL) errno_abort("Allocate alarm");
		if (sscanf(line, "%d %64[^\n]", &alarm->seconds, alarm->message) < 2) { fprintf(stderr, "Bad command\n"); free(alarm); //Parse input line into seconds(%d) and a message(%64[^\n]), consisting of up to 64 characters separated from the seconds by whitespace.
		} else { printf("main 3 \t\t Before pthread_mutex_lock. \n");
			status = pthread_mutex_lock(&alarm_mutex); printf("main 4 \t\t After pthread_mutex_lock. \n"); if (status != 0) err_abort(status, "Lock mutex");
			alarm->time = time(NULL) + alarm->seconds;

			//Insert the new alarm into the list of alarms, sorted by expiration time.
			alarm_insert(alarm); printf("main 5 \t\t Before pthread_mutex_unlock. \n"); //38 Because the condition variable signal operation is built into the new alarm_ insert function, we call alarm_insert rather than inserting a new alarm directly.
			status = pthread_mutex_unlock(&alarm_mutex); printf("main 6 \t\t After pthread_mutex_unlock. \n"); if (status != 0) err_abort(status, "Unlock mutex");
		}
	}
}
