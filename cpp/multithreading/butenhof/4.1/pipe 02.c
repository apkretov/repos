#include <pthread.h>	//The program shows the pieces of a simple pipeline program. Each thread in the pipeline increments its input value by 1 and passes it to the next thread. The main program reads a series of "command lines" from stdin. A command line is either a number, which is fed into the beginning of the pipeline, or the character "=," which causes the program to read the next result from the end of the pipeline and print it to stdout.
#include "errors.h"

typedef struct stage_tag {	//9						//9-17 Each stage of a pipeline is represented by a variable of type stage_t.			//Internal structure describing a "stage" in the pipeline. One for each thread, plus a "result stage" where the final thread can stash the value.
	pthread_mutex_t mutex;	//Protect data			//9-17 stage_t contains a mutex to synchronize access to the stage (???).
	pthread_cond_t avail;	//Data available		//9-17 The avail condition variable is used to signal a stage that data is ready for it to process,
	pthread_cond_t ready;	//Ready for data		//9-17 and each stage signals its own ready condition variable when it is ready for new data.
	int data_ready;			//Data present
	long data;					//Data to process		//9-17 The data member is the data passed from the previous stage,
	pthread_t thread;			//Thread for stage	//9-17 thread is the thread operating this stage,
	struct stage_tag* next; //Next stage			//9-17 and next is a pointer to the following stage.
} stage_t;						//17

typedef struct pipe_tag {	//23							//23-29 The pipe_t structure describes a pipeline. It provides pointers to the first and last stage of a pipeline.		//External structure representing the entire pipeline.
	pthread_mutex_t mutex;	//Mutex to protect pipe
	stage_t* head;				//First stage				//23-29 The first stage, head, represents the first thread in the pipeline.
	stage_t* tail;				//Final stage				//23-29 The last stage, tail, is a special stage_t that has no thread—it is a place to store the final result of the pipeline.
	int stages;					//Number of stages
	int active;					//Active data elements
} pipe_t;						//29

int pipe_send(stage_t* stage, long data) { //Part 2 shows pipe_send, a utility function used to start data along a pipeline, and also called by each stage to pass data to the next stage.		//Internal function to send a "message" to the specified pipe stage. Threads use this to pass along the modified data item.
	int status;

	status = pthread_mutex_lock(&stage->mutex);
	if (status != 0)
		return status;
	while (stage->data_ready) { //17-23 It begins by waiting on the specified pipeline stage's ready condition variable until it can accept new data.			//If there's data in the pipe stage, wait for it to be consumed.
		status = pthread_cond_wait(&stage->ready, &stage->mutex);
		if (status != 0) {
			pthread_mutex_unlock(&stage->mutex);
			return status;
		}
	} //23
	stage->data = data; //28-30 Store the new data value, and then tell the stage that data is available.				//Send the new data
	stage->data_ready = 1;
	status = pthread_cond_signal(&stage->avail); //30
	if (status != 0) {
		pthread_mutex_unlock(&stage->mutex);
		return status;
	}
	status = pthread_mutex_unlock(&stage->mutex);

	return status;
}

//Part 3 shows pipe_stage, the start function for each thread in the pipeline. The thread's argument is a pointer to its stage_t structure.
//The thread start routine for pipe stage threads. Each will wait for a data item passed from the caller or the previous stage, modify the data and pass it along to the next (or final) stage.
void* pipe_stage(void* arg) {
	stage_t* stage = (stage_t*)arg;
	stage_t* next_stage = stage->next;
	int status;

	status = pthread_mutex_lock(&stage->mutex); if (status != 0) err_abort(status, "Lock pipe stage");
	while (1) { //16-27 The thread loops forever, processing data. Because the mutex is locked outside the loop, the thread appears to have the pipeline stage's mutex locked all the time. However, it spends most of its time waiting for new data, on the avail condition variable. Remember that a thread automatically unlocks the mutex associated with a condition variable, while waiting on that condition variable. In reality, therefore, the thread spends most of its time with mutex unlocked.
		while (stage->data_ready != 1) {
			status = pthread_cond_wait(&stage->avail, &stage->mutex); if (status != 0)	err_abort(status, "Wait for previous stage"); //A condition variable wait always returns with the mutex locked. @ 1. 3.3 Condition variables.
		}
		pipe_send(next_stage, stage->data + 1); //22-26 When given data, the thread increases its own data value by one, and passes the result to the next stage. The thread then records that the stage no longer has data by clearing the data_ready flag, and signals the ready condition variable to wake any thread that might be waiting for this pipeline stage.
		stage->data_ready = 0;
		status = pthread_cond_signal(&stage->ready);	if (status != 0) err_abort(status, "Wake next stage"); //26
	} //27
} //Notice that the routine never unlocks the stage->mutex. The call to pthread_cond_wait implicitly unlocks the mutex while the thread is waiting, allowing other threads to make progress. Because the loop never terminates, this function has no need to unlock the mutex explicitly.
  //Waiting on a condition variable atomically releases the associated mutex and waits until another thread signals the condition variable. The mutex must always be locked when you wait on a condition variable and, when a thread wakes up from a condition variable wait, it always resumes with the mutex locked. @ 3.3 Condition variables

int pipe_create(pipe_t* pipe, int stages) { //Part 4 shows pipe_create, the function that creates a pipeline. It can create a pipeline of any number of stages, linking them together in a list.			//External interface to create a pipeline. All the data is initialized and the threads created. They'll wait for data.
	int pipe_index;
	stage_t **link = &pipe->head, *new_stage, *stage;
	int status;
	status = pthread_mutex_init(&pipe->mutex, NULL); if (status != 0)	err_abort(status, "Init pipe mutex");
	pipe->stages = stages;
	pipe->active = 0;

	for (pipe_index = 0; pipe_index <= stages; pipe_index++) { 	//18-34 For each stage, it allocates a new stage_t structure and initializes the members. Notice that one additional "stage" is allocated and initialized to hold the final result of the pipeline.
		new_stage = (stage_t*)malloc(sizeof(stage_t)); if (new_stage == NULL) errno_abort("Allocate stage");
		status = pthread_mutex_init(&new_stage->mutex, NULL);	if (status != 0) err_abort(status, "Init stage mutex");
		status = pthread_cond_init(&new_stage->avail, NULL); if (status != 0) err_abort(status, "Init avail condition");
		status = pthread_cond_init(&new_stage->ready, NULL); if (status != 0) err_abort(status, "Init ready condition");
		new_stage->data_ready = 0;
		*link = new_stage;
		link = &new_stage->next;
	} //34

	*link = (stage_t*)NULL; //Terminate list  //36-37 The link member of the final stage is set to NULL to terminate the list, and the pipeline's tail is set to point at the final stage. The tail pointer allows pipe_result to easily find the final product of the pipeline, which is stored into the final stage.
	pipe->tail = new_stage; //Record the tail //37

	for (stage = pipe->head; stage->next != NULL; stage = stage->next) { //52-59 After all the stage data is initialized, pipe_create creates a thread for each stage. The extra "final stage" does not get a thread—the termination condition of the for loop is that the current stage's next link is not NULL, which means that it will not process the final stage.			//Create the threads for the pipe stages only after all the data is initialized(including all links). Note that the last stage doesn't get a thread, it's just a receptacle for the final pipeline value. At this point, proper cleanup on an error would take up more space than worthwhile in a "simple example," so instead of cancelling and detaching all the threads already created, plus the synchronization object and memory cleanup done for earlier errors, it will simply abort.
		status = pthread_create(&stage->thread, NULL, pipe_stage, (void*)stage); if (status != 0)	err_abort(status, "Create pipe stage");
	} //59
	return 0;
}

//Part 5 shows pipe_start and pipe_result. The pipe_start function pushes an item of data into the beginning of the pipeline and then returns immediately without waiting for a result. The pipe_result function allows the caller to wait for the final result, whenever the result might be needed.
//External interface to start a pipeline by passing data to the first stage. The routine returns while the pipeline processes in parallel. Call the pipe_result return to collect the final stage values(note that the pipe will stall when each stage fills, until the result is collected).
int pipe_start(pipe_t* pipe, long value) {
	int status;

	status = pthread_mutex_lock(&pipe->mutex); if (status != 0)	err_abort(status, "Lock pipe mutex");
	pipe->active++;
	status = pthread_mutex_unlock(&pipe->mutex);	if (status != 0) err_abort(status, "Unlock pipe mutex"); //	//19-22 The pipe_start function sends data to the first stage of the pipeline. The function increments a count of "active" items in the pipeline, which allows pipe_ result to detect that there are no more active items to collect, and to return immediately instead of blocking. You would not always want a pipeline to behave this way — it makes sense for this example because a single thread alternately "feeds" and "reads" the pipeline, and the application would hang forever if the user inadvertently reads one more item than had been fed.

	pipe_send(pipe->head, value);
	return 0;
} //22

//Collect the result of the pipeline. Wait for a result if the pipeline hasn't produced one.
int pipe_result(pipe_t* pipe, long *result) { //23-47 The pipe_result function first checks whether there is an active item in the pipeline. If not, it returns with a status of 0, after unlocking the pipeline mutex.
	stage_t* tail = pipe->tail;
	long value;
	int empty = 0;
	int status;

	status = pthread_mutex_lock(&pipe->mutex); if (status != 0)	err_abort(status, "Lock pipe mutex");
	if (pipe->active <= 0)
		empty = 1;
	else
		pipe->active--;
	status = pthread_mutex_unlock(&pipe->mutex);	if (status != 0) err_abort(status, "Unlock pipe mutex");

	if (empty)
		return 0; //47

	pthread_mutex_lock(&tail->mutex); //48-55 If there is another item in the pipeline, pipe_result locks the tail(final) stage, and waits for it to receive data. It copies the data and then resets the stage so it can receive the next item of data. Remember that the final stage does not have a thread, and cannot reset itself.
	while (!tail->data_ready)
		pthread_cond_wait(&tail->avail, &tail->mutex);
	*result = tail->data;
	tail->data_ready = 0;
	pthread_cond_signal(&tail->ready);
	pthread_mutex_unlock(&tail->mutex); //55

	return 1;
}

//Part 6 shows the main program that drives the pipeline. It creates a pipeline, and then loops reading lines from stdin. If the line is a single "=" character, it pulls a result from the pipeline and prints it. Otherwise, it converts the line to an integer value, which it feeds into the pipeline.
int main(int argc, char* argv[]) { //The main program to "drive" the pipeline...
	pipe_t sttMyPipe;
	long value, result;
	int status;
	char line[128];

	pipe_create(&sttMyPipe, 10);
	printf("Enter integer values, or \"=\" for next result\n");
	while (1) {
		printf("Data> ");
		if (fgets(line, sizeof(line), stdin) == NULL) exit(0);
		if (strlen(line) <= 1) continue;
		if (strlen(line) <= 2 && line[0] == '=') {
			if (pipe_result(&sttMyPipe, &result))
				printf("Result is %ld\n", result);
			else
				printf("Pipe is empty\n");
		} else {
			if (sscanf(line, "%ld", &value) < 1)
				fprintf(stderr, "Enter an integer value\n");
			else
				pipe_start(&sttMyPipe, value);
		}
	}
}
