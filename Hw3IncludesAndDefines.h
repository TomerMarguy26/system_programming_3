#pragma once
#define _CRT_SECURE_NO_WARNINGS 
#define _CRT_NONSTDC_NO_DEPRECATE

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <windows.h>
#include <stdbool.h>

#define ERROR_CODE ((int)(-1))
#define SUCCESS_CODE ((int)(0))
#define PATH_LENGTH 256 
#define ROOMS_LEN 20
#define RESIDENT_LEN 20
#define MAX_NUM_ROOMS 5
#define MAX_NUM_RESIDENTS 15
#define MAX_LINE 50

/* Structs */

struct rooms
{
	char name[ROOMS_LEN + 1];
	int price;
	int limit;
	HANDLE room_limit_sempahore;
};

struct resident
{
	char name[RESIDENT_LEN + 1];
	char suit_room[ROOMS_LEN + 1];
	int budget;
	int id;
	int stay_days;
	char *log_path;
};
