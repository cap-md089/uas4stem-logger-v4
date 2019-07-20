#include "./commands.h"
#include <cstring>

void command_function_set(
	int argc,
	char* argv[],
	char** pcurrent_target_label
) {
	if (argc < 3) {
		return;
	}

	if (strncmp(argv[1], "target", 6) == 0) {
		delete[] (*pcurrent_target_label);
		size_t total_size = 0;
		for (int i = 2; i < argc; i++) {
			total_size += strlen(argv[i]) + 1;
		}
		total_size--;

		*pcurrent_target_label = new char[total_size];
		size_t current_pointer = 0;
		for (int i = 2; i < argc; i++) {
			for (size_t j = 0; j < strlen(argv[i]); j++) {
				(*pcurrent_target_label)[current_pointer++] = argv[i][j];
			}
			(*pcurrent_target_label)[current_pointer++] = (i == argc - 1) ? '\0' : ' ';
		}
	}

	if (strncmp(argv[1], "batt", 4) == 0) {
		
	}
}
