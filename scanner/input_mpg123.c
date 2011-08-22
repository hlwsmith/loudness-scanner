/* See LICENSE file for copyright and license details. */
#define _POSIX_C_SOURCE 1
#include <mpg123.h>
#include <stdio.h>

#include "ebur128.h"
#include "input.h"

struct input_handle {
  mpg123_handle* mh;
  long mh_rate;
  int mh_channels, mh_encoding;
  float* buffer;
};

size_t mpg123_get_channels(struct input_handle* ih) {
  return (size_t) ih->mh_channels;
}

size_t mpg123_get_samplerate(struct input_handle* ih) {
  return (size_t) ih->mh_rate;
}

float* mpg123_get_buffer(struct input_handle* ih) {
  return ih->buffer;
}

size_t mpg123_get_buffer_size(struct input_handle* ih) {
  return (size_t) ih->mh_rate * (size_t) ih->mh_channels;
}

struct input_handle* mpg123_handle_init() {
  struct input_handle* ret;
  ret = malloc(sizeof(struct input_handle));
  return ret;
}

void mpg123_handle_destroy(struct input_handle** ih) {
  free(*ih);
  *ih = NULL;
}

int mpg123_open_file(struct input_handle* ih, FILE* file) {
  int result;
  ih->mh = mpg123_new(NULL, &result);
  if (!ih->mh) {
    fprintf(stderr, "Could not create mpg123 handler!\n");
    goto close_file;
  }
  result = mpg123_open_fd(ih->mh, fileno(file));
  if (result != MPG123_OK) {
    fprintf(stderr, "Could not open input file!\n");
    goto close_file;
  }
  result = mpg123_getformat(ih->mh, &ih->mh_rate, &ih->mh_channels, &ih->mh_encoding);
  if (result != MPG123_OK) {
    fprintf(stderr, "mpg123_getformat failed!\n");
    goto close_file;
  }
  result = mpg123_format_none(ih->mh);
  if (result != MPG123_OK) {
    fprintf(stderr, "mpg123_format_none failed!\n");
    goto close_file;
  }
  result = mpg123_format(ih->mh, ih->mh_rate, ih->mh_channels, MPG123_ENC_FLOAT_32);
  if (result != MPG123_OK) {
    fprintf(stderr, "mpg123_format failed!\n");
    goto close_file;
  }
  result = mpg123_close(ih->mh);
  result = mpg123_open_fd(ih->mh, fileno(file));
  if (result != MPG123_OK) {
    fprintf(stderr, "Could not open input file!\n");
    goto close_file;
  }
  result = mpg123_getformat(ih->mh, &ih->mh_rate, &ih->mh_channels, &ih->mh_encoding);
  if (result != MPG123_OK) {
    fprintf(stderr, "mpg123_getformat failed!\n");
    goto close_file;
  }
  return 0;

close_file:
  mpg123_close(ih->mh);
  mpg123_delete(ih->mh);
  ih->mh = NULL;
  return 1;
}

int mpg123_set_channel_map(struct input_handle* ih, ebur128_state* st) {
  (void) ih;
  (void) st;
  return 1;
}

int mpg123_allocate_buffer(struct input_handle* ih) {
  ih->buffer = (float*) malloc((size_t) ih->mh_rate *
                               (size_t) ih->mh_channels *
                               sizeof(float));
  if (ih->buffer) {
    return 0;
  } else {
    return 1;
  }
}

size_t mpg123_read_frames(struct input_handle* ih) {
  size_t nr_frames_read;
  int result = mpg123_read(ih->mh, (unsigned char*) ih->buffer,
                           (size_t) ih->mh_rate *
                           (size_t) ih->mh_channels * sizeof(float),
                           &nr_frames_read);
  if (result != MPG123_OK && result != MPG123_DONE) {
    if (result == MPG123_ERR && mpg123_errcode(ih->mh) == MPG123_RESYNC_FAIL) {
      fprintf(stderr, "%s\n", mpg123_strerror(ih->mh));
      fprintf(stderr, "Maybe your file has an APEv2 tag?\n");
      return 0;
    } else {
      fprintf(stderr, "Internal MPG123 error!\n");
      return 0;
    }
  }
  nr_frames_read /= (size_t) ih->mh_channels * sizeof(float);
  return nr_frames_read;
}

int mpg123_check_ok(struct input_handle* ih, size_t nr_frames_read_all) {
  (void) ih;
  (void) nr_frames_read_all;
  return 0;
}

void mpg123_free_buffer(struct input_handle* ih) {
  free(ih->buffer);
  ih->buffer = NULL;
}

void mpg123_close_file(struct input_handle* ih, FILE* file) {
  mpg123_close(ih->mh);
  mpg123_delete(ih->mh);
  fclose(file);
  ih->mh = NULL;
}

int mpg123_init_library() {
  int result = mpg123_init();
  if (result != MPG123_OK) {
    return 1;
  }
  return 0;
}

void mpg123_exit_library() {
  mpg123_exit();
}

R128EXPORT struct input_ops ip_ops = {
  mpg123_get_channels,
  mpg123_get_samplerate,
  mpg123_get_buffer,
  mpg123_get_buffer_size,
  mpg123_handle_init,
  mpg123_handle_destroy,
  mpg123_open_file,
  mpg123_set_channel_map,
  mpg123_allocate_buffer,
  mpg123_read_frames,
  mpg123_check_ok,
  mpg123_free_buffer,
  mpg123_close_file,
  mpg123_init_library,
  mpg123_exit_library
};

R128EXPORT const char* ip_exts[] = {"mp3", "mp2", NULL};
