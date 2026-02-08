#include <CoreFoundation/CoreFoundation.h>
#define CONFIG_H

#include "yyjson.h"
#include <pwd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#define CONFIG_MAX_FINGERS 16

typedef struct {
	bool natural_swipe;
	bool wrap_around;
	bool haptic;
	bool skip_empty;
	int fingers;
	int swipe_tolerance;
	float distance_pct; // distance
	float velocity_pct; // velocity
	float settle_factor;
	float min_step;
	float min_travel;
	float min_step_fast;
	float min_travel_fast;
	float palm_disp;
	CFTimeInterval palm_age;
	float palm_velocity;
	const char* swipe_left;
	const char* swipe_right;
} Config;

static int clamp_int_with_warning(const char* key, int value, int min, int max, int fallback)
{
	if (value < min || value > max) {
		fprintf(stderr, "Warning: Invalid '%s'=%d. Using %d (allowed range %d..%d).\n",
			key, value, fallback, min, max);
		return fallback;
	}
	return value;
}

static float clamp_float_with_warning(const char* key, float value, float min, float max, float fallback)
{
	if (value < min || value > max) {
		fprintf(stderr, "Warning: Invalid '%s'=%.6f. Using %.6f (allowed range %.6f..%.6f).\n",
			key, value, fallback, min, max);
		return fallback;
	}
	return value;
}

static bool try_get_json_number(yyjson_val* root, const char* key, float* out)
{
	yyjson_val* item = yyjson_obj_get(root, key);
	if (!item || !yyjson_is_num(item))
		return false;
	*out = (float)yyjson_get_real(item);
	return true;
}

static bool try_get_json_int(yyjson_val* root, const char* key, int* out)
{
	yyjson_val* item = yyjson_obj_get(root, key);
	if (!item || !yyjson_is_int(item))
		return false;
	*out = (int)yyjson_get_int(item);
	return true;
}

static Config default_config()
{
	Config config;
	config.natural_swipe = false;
	config.wrap_around = true;
	config.haptic = false;
	config.skip_empty = true;
	config.fingers = 3;
	config.swipe_tolerance = 0;
	config.distance_pct = 0.08f; // ≥8 % travel triggers
	config.velocity_pct = 0.30f; // ≥0.30 × w pts / s triggers
	config.settle_factor = 0.15f; // ≤15 % of flick speed -> flick ended
	config.min_step = 0.005f;
	config.min_travel = 0.015f;
	config.min_step_fast = 0.0f;
	config.min_travel_fast = 0.003f;
	config.palm_disp = 0.025; // 2.5% pad from origin
	config.palm_age = 0.06; // 60ms before judgment
	config.palm_velocity = 0.1; // 10% of pad dimension per second
	config.swipe_left = "prev";
	config.swipe_right = "next";
	return config;
}

static int read_file_to_buffer(const char* path, char** out, size_t* size)
{
	FILE* file = fopen(path, "rb");
	if (!file)
		return 0;

	struct stat st;
	if (stat(path, &st) != 0) {
		fclose(file);
		return 0;
	}
	*size = st.st_size;

	*out = (char*)malloc(*size + 1);
	if (!*out) {
		fclose(file);
		return 0;
	}

	fread(*out, 1, *size, file);
	(*out)[*size] = '\0';
	fclose(file);
	return 1;
}

static Config load_config()
{
	Config config = default_config();

	char* buffer = NULL;
	size_t buffer_size = 0;
	const char* paths[] = { "./config.json", NULL };

	char fallback_path[512];
	struct passwd* pw = getpwuid(getuid());
	if (pw) {
		snprintf(fallback_path, sizeof(fallback_path),
			"%s/.config/aerospace-swipe/config.json", pw->pw_dir);
		paths[1] = fallback_path;
	}

	for (int i = 0; i < 2; ++i) {
		if (paths[i] && read_file_to_buffer(paths[i], &buffer, &buffer_size)) {
			printf("Loaded config from: %s\n", paths[i]);
			break;
		}
	}

	if (!buffer) {
		fprintf(stderr, "Using default configuration.\n");
		return config;
	}

	yyjson_doc* doc = yyjson_read(buffer, buffer_size, 0);
	free(buffer);
	if (!doc) {
		fprintf(stderr, "Failed to parse config JSON. Using defaults.\n");
		return config;
	}

	yyjson_val* root = yyjson_doc_get_root(doc);
	yyjson_val* item;
	Config defaults = default_config();

	item = yyjson_obj_get(root, "natural_swipe");
	if (item && yyjson_is_bool(item))
		config.natural_swipe = yyjson_get_bool(item);

	item = yyjson_obj_get(root, "wrap_around");
	if (item && yyjson_is_bool(item))
		config.wrap_around = yyjson_get_bool(item);

	item = yyjson_obj_get(root, "haptic");
	if (item && yyjson_is_bool(item))
		config.haptic = yyjson_get_bool(item);

	item = yyjson_obj_get(root, "skip_empty");
	if (item && yyjson_is_bool(item))
		config.skip_empty = yyjson_get_bool(item);

	int int_value = 0;
	float float_value = 0.0f;

	if (try_get_json_int(root, "fingers", &int_value))
		config.fingers = int_value;
	if (try_get_json_int(root, "swipe_tolerance", &int_value))
		config.swipe_tolerance = int_value;

	if (try_get_json_number(root, "distance_pct", &float_value))
		config.distance_pct = float_value;
	if (try_get_json_number(root, "velocity_pct", &float_value))
		config.velocity_pct = float_value;
	if (try_get_json_number(root, "settle_factor", &float_value))
		config.settle_factor = float_value;
	if (try_get_json_number(root, "min_step", &float_value))
		config.min_step = float_value;
	if (try_get_json_number(root, "min_travel", &float_value))
		config.min_travel = float_value;
	if (try_get_json_number(root, "min_step_fast", &float_value))
		config.min_step_fast = float_value;
	if (try_get_json_number(root, "min_travel_fast", &float_value))
		config.min_travel_fast = float_value;
	if (try_get_json_number(root, "palm_disp", &float_value))
		config.palm_disp = float_value;
	if (try_get_json_number(root, "palm_age", &float_value))
		config.palm_age = float_value;
	if (try_get_json_number(root, "palm_velocity", &float_value))
		config.palm_velocity = float_value;

	config.fingers = clamp_int_with_warning("fingers", config.fingers, 1, CONFIG_MAX_FINGERS, defaults.fingers);
	config.swipe_tolerance = clamp_int_with_warning("swipe_tolerance", config.swipe_tolerance, 0, config.fingers, defaults.swipe_tolerance);
	config.distance_pct = clamp_float_with_warning("distance_pct", config.distance_pct, 0.001f, 1.0f, defaults.distance_pct);
	config.velocity_pct = clamp_float_with_warning("velocity_pct", config.velocity_pct, 0.01f, 5.0f, defaults.velocity_pct);
	config.settle_factor = clamp_float_with_warning("settle_factor", config.settle_factor, 0.01f, 1.0f, defaults.settle_factor);
	config.min_step = clamp_float_with_warning("min_step", config.min_step, 0.0f, 1.0f, defaults.min_step);
	config.min_travel = clamp_float_with_warning("min_travel", config.min_travel, 0.0f, 1.0f, defaults.min_travel);
	config.min_step_fast = clamp_float_with_warning("min_step_fast", config.min_step_fast, 0.0f, 1.0f, defaults.min_step_fast);
	config.min_travel_fast = clamp_float_with_warning("min_travel_fast", config.min_travel_fast, 0.0f, 1.0f, defaults.min_travel_fast);
	config.palm_disp = clamp_float_with_warning("palm_disp", config.palm_disp, 0.0f, 1.0f, defaults.palm_disp);
	config.palm_age = clamp_float_with_warning("palm_age", (float)config.palm_age, 0.0f, 5.0f, (float)defaults.palm_age);
	config.palm_velocity = clamp_float_with_warning("palm_velocity", config.palm_velocity, 0.0f, 10.0f, defaults.palm_velocity);

	config.swipe_left = config.natural_swipe ? "next" : "prev";
	config.swipe_right = config.natural_swipe ? "prev" : "next";

	yyjson_doc_free(doc);
	return config;
}
