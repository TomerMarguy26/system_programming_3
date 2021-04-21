/*
Author: Tomer Marguy...........id_number: 205705874
Project name: "Hw3"
*/

#include "Hw3IncludesAndDefines.h"
#include "Hw3Functions.h"

/* Global variables */

struct rooms rooms_arr[MAX_NUM_ROOMS];
struct resident resident_arr[MAX_NUM_RESIDENTS];
int day_counter = 1;

/* A variable used as a method of accessing the same mutex from different threads. */
static HANDLE write2file_mutex_handle = NULL;

/*
Description: This is the main function. argv[1] will be the <full_path>, the path with all the input details.
			 The final (Log) file will be printed there as well.
Parameters:  int argc - the number of strings pointed to by argv.
			 char *argv[] - an array of strings representing the individual arguments provided on the command line.
Returns:	 exit program with 0 if everything went ok.
*/

int main(int argc, char *argv[]) {
	/* variables initiations */
	int names_index = 0, rooms_index = 0, path_length = (2*PATH_LENGTH) + strlen(argv[1]),
		num_of_threads = 0, num_of_semaphores = 0;
	char *names_path = NULL, *rooms_path = NULL, *log_path = NULL,
		 name_with_budget[MAX_LINE] = "", room_line[MAX_LINE] = "";
	HANDLE p_thread_handles[MAX_NUM_RESIDENTS];
	DWORD p_thread_ids[MAX_NUM_RESIDENTS];
	DWORD wait_code;
	BOOL ret_val;
	BOOL an_error_occured = FALSE; //a bool variable used in goto sections

	/* Memory Allocation */
	rooms_path = (char*)malloc(path_length);
	if (NULL == rooms_path) {
		an_error_occured = TRUE;
		printf("Memory allocation has failed!");
		goto free_mallocs;
	}

	names_path = (char*)malloc(path_length);
	if (NULL == names_path) {
		an_error_occured = TRUE;
		printf("Memory allocation has failed!");
		goto free_mallocs;
	}

	log_path = (char*)malloc(path_length); 
	if (NULL == log_path) {
		an_error_occured = TRUE;
		printf("Memory allocation has failed!");
		goto free_mallocs;
	}

	/* Copy path from argv[1] */
	strcpy(rooms_path, argv[1]);
	strcpy(names_path, argv[1]);
	strcpy(log_path,   argv[1]);
	strcat(rooms_path, "/rooms.txt"); //creates the path to the rooms.txt
	strcat(names_path, "/names.txt"); //creates the path to the names.txt
	strcat(log_path,   "/roomLog.txt"); //creates the path to the roomLog.txt

	/* Open files */
	FILE *fp_rooms = NULL; //fp_rooms is the pointer to the rooms.txt file
	if (NULL == (fp_rooms = fopen(rooms_path, "r"))) {
		an_error_occured = TRUE;
		printf("Unable to open file!");
		goto close_files;
	}

	FILE *fp_names = NULL; //fp_names is the pointer to the names.txt file
	if (NULL == (fp_names = fopen(names_path, "r"))) {
		an_error_occured = TRUE;
		printf("Unable to open file!");
		goto close_files;
	}

	/* Fill rooms_array */
	while (fgets(room_line, sizeof(room_line), fp_rooms) != NULL) { //while loop over all names.txt
		/* Seperate details from line */
		char* room_name = strtok(room_line, " ");
		char* price = strtok(NULL, " ");
		char* limit = strtok(NULL, " ");
		limit[strlen(limit)] = '\0';
		/* Update */
		strcpy(rooms_arr[rooms_index].name, room_name);
		rooms_arr[rooms_index].price = atoi(price);
		rooms_arr[rooms_index].limit = atoi(limit);
		rooms_arr[rooms_index].room_limit_sempahore = CreateSemaphore(
			NULL,					/* Default security attributes */
			rooms_arr[rooms_index].limit,		/* Initial Count - all slots are empty */
			rooms_arr[rooms_index].limit,		/* Maximum Count */
			NULL);		/* named */
		num_of_semaphores++;

		if (rooms_arr[rooms_index].room_limit_sempahore == NULL) {
			an_error_occured = TRUE;
			printf("An error has occured while creating the semaphore, Last Error = 0x%x\n!", GetLastError());
			goto close_semaphores;
		}
		rooms_index++; //in order to know the amount of rooms in practice
	}

	/* Fill resident_array */
	while (fgets(name_with_budget, sizeof(name_with_budget), fp_names) != NULL) { //while loop over all names.txt
		/* Seperate details from line */
		char* res_name = strtok(name_with_budget, " ");
		char* budget = strtok(NULL, " ");
		budget[strlen(budget)] = '\0';
		/* Update */
		strcpy(resident_arr[names_index].name, res_name);
		resident_arr[names_index].budget = atoi(budget);
		resident_arr[names_index].log_path = log_path;
		resident_arr[names_index].id = names_index;

		for (int i = 0; i < rooms_index; i++)
		{
			if (resident_arr[names_index].budget % rooms_arr[i].price == 0) {
				strcpy(resident_arr[names_index].suit_room, rooms_arr[i].name);
				resident_arr[names_index].stay_days = (resident_arr[names_index].budget / rooms_arr[i].price);
			}
		}
		names_index++; //in order to know the amount of residents in practice
	}
	fclose(fp_names);

	/* Create the mutex that will be used to synchronize access writing into the file */
	write2file_mutex_handle = CreateMutex(
		NULL,	/* default security attributes */
		FALSE,	/* initially not owned */
		NULL);	/* unnamed mutex */
	if (NULL == write2file_mutex_handle)
	{
		an_error_occured = TRUE;
		printf("An error has occured while creating the mutex, Last Error = 0x%x\n!", GetLastError());
		goto close_mutex;
	}

	/* Create Threads */
	for (int i = 0; i < names_index; i++) {
		p_thread_handles[i] = CreateThreadSimple(check_resident, &resident_arr[i], &p_thread_ids[0]); 
		num_of_threads++;
	}

	/* Check Threads */
	for (int i = 0; i < names_index; i++) {
		if (NULL == p_thread_handles[i])
		{
			an_error_occured = TRUE;
			printf("Error when creating thread: %d\n", GetLastError());
			goto close_threads;	
		}
	}

	/* Check if need to increase the day 
	Wait for all threads to finish and check */
	while (1) {
		wait_code = WaitForMultipleObjects(names_index, p_thread_handles, TRUE, 500);
		/* not all the residents checked out */
		if (WAIT_TIMEOUT == wait_code) {
			day_counter++;
		}

		if (WAIT_FAILED == wait_code) {
			printf("Error when waiting\n");
			goto close_threads;
		}

		/* all therads have finished! */
		if (WAIT_OBJECT_0 == wait_code)
			break;
	}

close_threads:
	for (int i = 0; i < num_of_threads; i++)
	{
		ret_val = CloseHandle(p_thread_handles[i]);
		if (FALSE == ret_val)
		{
			printf("Error when closing thread: %d\n", GetLastError());
		}
	}

close_mutex:
	ret_val = CloseHandle(write2file_mutex_handle);
	if (FALSE == ret_val)
	{
		printf("Error when closing mutex handle: %d\n", GetLastError());
	}

close_semaphores:
	for (int i = 0; i < num_of_semaphores; i++) {
		ret_val = CloseHandle(rooms_arr[i].room_limit_sempahore);
		if (FALSE == ret_val)
		{
			printf("Error when closing semaphore handle: %d\n", GetLastError());
		}

	}
close_files:
	fclose(fp_rooms);

free_mallocs:
	free(names_path);
	free(rooms_path);
	free(log_path);

	if (an_error_occured)
		return ERROR_CODE;
	else {
		printf("%d", day_counter);
		return SUCCESS_CODE;
	}
}

/* Threads Function */

static HANDLE CreateThreadSimple(LPTHREAD_START_ROUTINE p_start_routine,
	LPVOID p_thread_parameters,
	LPDWORD p_thread_id)
{
	HANDLE thread_handle;

	if (NULL == p_start_routine)
	{
		printf("Error when creating a thread");
		printf("Received null pointer");
	}

	if (NULL == p_thread_id)
	{
		printf("Error when creating a thread");
		printf("Received null pointer");
	}

	thread_handle = CreateThread(
		NULL,            /*  default security attributes */
		0,               /*  use default stack size */
		p_start_routine, /*  thread function */
		p_thread_parameters, /*  argument to thread function */
		0,               /*  use default creation flags */
		p_thread_id);    /*  returns the thread identifier */

	if (NULL == thread_handle)
	{
		printf("Couldn't create thread\n");
	}

	return thread_handle;
}

/* Function That Uses Global Variables */

/*
Description: This is the function that will be called for any thread to check in or out the resident if it
		     is possible.
Parameters:  char *path - the initial given in the argument path.
Returns:	 exit program with 0 if everything went ok, with -1 else.
*/

DWORD WINAPI check_resident(LPVOID lpParam) {
	DWORD wait_code;
	DWORD wait_res;
	BOOL ret_val, going_out = FALSE;
	BOOL release_res;
	struct resident *p_params;
	int right_room = -1, enterance_day = -1;

	/* Check if lpParam is NULL */
	if (NULL == lpParam)
	{
		return ERROR_CODE;
	}

	p_params = (struct resident *)lpParam; //casting

	/* Find the semaphore of the right room */
	for (int i = 0; i < MAX_NUM_ROOMS; i++) {
		if (!strcmp(p_params->suit_room, rooms_arr[i].name)) //we found the right room
			right_room = i;
	}

	/* Wait until there is space in the room using semaphore */
	wait_res = WaitForSingleObject(rooms_arr[right_room].room_limit_sempahore, INFINITE);
	if (wait_res != WAIT_OBJECT_0) {
		return ERROR_CODE;
	}

	/* Room is available, the resident can go inside! */
	enterance_day = day_counter;
	/* write check in to file */
	wait_code = WaitForSingleObject(write2file_mutex_handle, INFINITE); //waiting for the file
	if (WAIT_OBJECT_0 != wait_code)
	{
		printf("Error when waiting for mutex\n");
		return ERROR_CODE;
	}

	/* File Critical Section */
	FILE  *fp_log = fopen(p_params->log_path, "a");
	char check_in_line[ROOMS_LEN + RESIDENT_LEN + 106] = "";
	char check_out_line[ROOMS_LEN + RESIDENT_LEN + 106] = "";

	if (NULL == fp_log)
	{
		printf("Failed to open file. \n");
		return ERROR_CODE; //in case of failure the function will return -1
	}

	sprintf(check_in_line, "%s %s IN %d\n", p_params->suit_room, p_params->name, day_counter);
	fprintf(fp_log, check_in_line);
	fclose(fp_log);

	/* Release mutex */
	ret_val = ReleaseMutex(write2file_mutex_handle);
	if (FALSE == ret_val)
	{
		printf("Error when releasing\n");
		return ERROR_CODE;
	}

	/* waiting here until needs to check out */
	while (1) {
		if (day_counter - enterance_day == p_params->stay_days )//check if he finished his days
			break;
	}

		/* The resident is going out, need to release semaphore, print to file */ 
		/* write check out to file */
		wait_code = WaitForSingleObject(write2file_mutex_handle, INFINITE); //waiting for the file!
		if (WAIT_OBJECT_0 != wait_code)
		{
			printf("Error when waiting for mutex\n");
			return ERROR_CODE;
		}

		/* File Critical Section */
		FILE  *fp_log2 = fopen(p_params->log_path, "a");
		if (NULL == fp_log2)
		{
			printf("Failed to open file. \n");
			return ERROR_CODE; //in case of failure the function will return -1
		}

		sprintf(check_out_line, "%s %s OUT %d\n", p_params->suit_room, p_params->name, day_counter);
		fprintf(fp_log2, check_out_line);

		fclose(fp_log2);

		/* Release mutex */
		ret_val = ReleaseMutex(write2file_mutex_handle);
		if (FALSE == ret_val)
		{
			printf("Error when releasing\n");
			return ERROR_CODE;
		}

		release_res = ReleaseSemaphore(
			rooms_arr[right_room].room_limit_sempahore,
			1, 		/* Signal that exactly one cell was emptied */
			NULL);
		if (release_res == FALSE) {
			return ERROR_CODE;
		}
}