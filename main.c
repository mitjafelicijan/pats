#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>

#include <pulse/pulseaudio.h>

static pa_mainloop *mainloop = NULL;
static pa_context *context = NULL;
static int list_mode = 0;
static int toggle_mode = 0;

static pa_sink_info **sinks = NULL;
static unsigned int sink_count = 0;
static unsigned int current_sink_index = 0;

void server_info_callback(pa_context *c, const pa_server_info *i, void *userdata);
void set_default_sink_callback(pa_context *c, int success, void *userdata);

int get_output_type(const char *input, char *output, size_t output_size) {
	if (!input || !output || output_size == 0) return -1;

	const char *last_dot = strrchr(input, '.');
	if (!last_dot) return -1;
	last_dot++;

	const char *hyphen = strchr(last_dot, '-');
	size_t len = hyphen ? (size_t)(hyphen - last_dot) : strlen(last_dot);

	if (len >= output_size) len = output_size - 1;

	strncpy(output, last_dot, len);
	output[len] = '\0';

	return 0;
}

void sink_callback(pa_context *c, const pa_sink_info *i, int eol, void *userdata) {
	(void)userdata;

	if (eol > 0) {
		if (list_mode) {
			pa_mainloop_quit(mainloop, 0);
		} else if (toggle_mode) {
			pa_operation_unref(pa_context_get_server_info(c, server_info_callback, NULL));
		}
		return;
	}

	if (list_mode) {
		int is_active = 0;
		if (i->state == PA_SINK_RUNNING) {
			is_active = 1;
		}

		char output_type[64];
		if (get_output_type(i->name, output_type, sizeof(output_type)) == 0) {
			printf(" %s%s", output_type, is_active ? "*" : "");
		}

	} else if (toggle_mode) {
		sinks = realloc(sinks, (sink_count + 1) * sizeof(pa_sink_info*));
		sinks[sink_count] = malloc(sizeof(pa_sink_info));

		memcpy(sinks[sink_count], i, sizeof(pa_sink_info));

		if (i->name) {
			size_t name_len = strlen(i->name) + 1;
			char *name_copy = malloc(name_len);
			strcpy(name_copy, i->name);
			sinks[sink_count]->name = name_copy;
		}

		if (i->description) {
			size_t desc_len = strlen(i->description) + 1;
			char *desc_copy = malloc(desc_len);
			strcpy(desc_copy, i->description);
			sinks[sink_count]->description = desc_copy;
		}

		sink_count++;
	}
}

void set_default_sink_callback(pa_context *c, int success, void *userdata) {
	(void)userdata;

	if (success) {
		printf("Successfully switched default sink\n");
	} else {
		int error = pa_context_errno(c);
		fprintf(stderr, "Failed to switch default sink: %s (error code: %d)\n", pa_strerror(error), error);
	}
	pa_mainloop_quit(mainloop, success ? 0 : 1);
}

void server_info_callback(pa_context *c, const pa_server_info *i, void *userdata) {
	(void)userdata;

	if (!i) {
		pa_mainloop_quit(mainloop, 1);
		return;
	}

	// Find current default sink in our list.
	for (unsigned int j = 0; j < sink_count; j++) {
		if (strcmp(sinks[j]->name, i->default_sink_name) == 0) {
			current_sink_index = j;
			break;
		}
	}

	// Switch to next sink.
	unsigned int next_sink_index = (current_sink_index + 1) % sink_count;
	printf("Switching from %s to %s\n", 
			sinks[current_sink_index]->description,
			sinks[next_sink_index]->description);

	// Set the new default sink with callback.
	pa_operation_unref(pa_context_set_default_sink(c, sinks[next_sink_index]->name, set_default_sink_callback, NULL));
}

void state_callback(pa_context *c, void *userdata) {
	(void)userdata;

	switch (pa_context_get_state(c)) {
		case PA_CONTEXT_READY:
			if (list_mode) {
				pa_operation_unref(pa_context_get_sink_info_list(c, sink_callback, NULL));
			} else if (toggle_mode) {
				pa_operation_unref(pa_context_get_sink_info_list(c, sink_callback, NULL));
			}
			break;
		case PA_CONTEXT_FAILED:
		case PA_CONTEXT_TERMINATED:
			pa_mainloop_quit(mainloop, 1);
			break;
		default:
			break;
	}
}

void print_usage(const char *program_name) {
	printf("Usage: %s [OPTIONS]\n", program_name);
	printf("Options:\n");
	printf("  -l, --list     List all available audio sinks\n");
	printf("  -t, --toggle   Toggle between available audio sinks\n");
	printf("  -h, --help     Show this help message\n");
}

int main(int argc, char *argv[]) {
	int opt;
	const char *short_options = "lth";
	struct option long_options[] = {
		{"list", no_argument, 0, 'l'},
		{"toggle", no_argument, 0, 't'},
		{"help", no_argument, 0, 'h'},
		{0, 0, 0, 0}
	};

	while ((opt = getopt_long(argc, argv, short_options, long_options, NULL)) != -1) {
		switch (opt) {
			case 'l':
				list_mode = 1;
				break;
			case 't':
				toggle_mode = 1;
				break;
			case 'h':
				print_usage(argv[0]);
				return 0;
			default:
				print_usage(argv[0]);
				return 1;
		}
	}

	if (!list_mode && !toggle_mode) {
		fprintf(stderr, "Error: Please specify either -l (list) or -t (toggle) option\n");
		print_usage(argv[0]);
		return 1;
	}

	if (list_mode && toggle_mode) {
		fprintf(stderr, "Error: Cannot use both -l and -t options simultaneously\n");
		return 1;
	}

	mainloop = pa_mainloop_new();
	context = pa_context_new(pa_mainloop_get_api(mainloop), "SinkLister");

	if (pa_context_connect(context, NULL, PA_CONTEXT_NOFLAGS, NULL) < 0) {
		fprintf(stderr, "Failed to connect to PulseAudio server: %s\n", pa_strerror(pa_context_errno(context)));
		return 1;
	}

	pa_context_set_state_callback(context, state_callback, NULL);

	int retval;
	pa_mainloop_run(mainloop, &retval);

	// Cleanup for toggle mode
	if (toggle_mode && sinks) {
		for (unsigned int i = 0; i < sink_count; i++) {
			if (sinks[i]->name) {
				free((void*)sinks[i]->name);
			}
			if (sinks[i]->description) {
				free((void*)sinks[i]->description);
			}
			free(sinks[i]);
		}
		free(sinks);
	}

	pa_context_disconnect(context);
	pa_context_unref(context);
	pa_mainloop_free(mainloop);

	return retval;
}
