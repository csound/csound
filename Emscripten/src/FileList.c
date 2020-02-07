/*
 * C S O U N D
 *
 * L I C E N S E
 *
 * This software is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 */


#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <emscripten.h>

EMSCRIPTEN_KEEPALIVE 
size_t FileList_getFileCount(const char *directoryPath)
{
	struct dirent *directoryEntry;
	DIR *directory = opendir(directoryPath);
	size_t fileCount = 0;
	if (directory) {

		while ((directoryEntry = readdir(directory)) != NULL) {

			struct stat st;
			lstat(directoryEntry->d_name, &st);

			if (S_ISDIR(st.st_mode)) {

				continue;
			}
			else {
				fileCount++;
			}
		}

		closedir(directory);
	}

	return fileCount;
}


EMSCRIPTEN_KEEPALIVE 
char *FileList_getFileNameString(const char *directoryPath, size_t fileNumber)
{
	if (fileNumber >= FileList_getFileCount(directoryPath)) {

		printf("ERROR: illegal file index");
		return 0;
	}
	else {

		struct dirent *directoryEntry;
		DIR *directory = opendir(directoryPath);
		if (directory) {

			size_t currentIndex = 0;
			while ((directoryEntry = readdir(directory)) != NULL) {

				struct stat st;
				lstat(directoryEntry->d_name, &st);

				if (S_ISDIR(st.st_mode)) {

					continue;
				}
				else {
					if (currentIndex == fileNumber) {

						closedir(directory);
						return directoryEntry->d_name;
					}

					currentIndex++;
				}

			}
		}

	}

	return 0;
}

