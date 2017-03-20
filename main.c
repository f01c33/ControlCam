#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

// #include <dirent.h>                  // diretórios

// #include "./deps/b64/b64.h"             // fazer encode do base64
#include "./deps/fat_array/fat_array.h" // array dinâmica
#include "./deps/flag/flag.h"           // lidar com argv e argc
#include "./deps/jsmn/jsmn.h"           // json parser lib

#include <curl/curl.h> // para comunicação web no geral

#define VERSION "0.0.2"

#include "external.h"
#include "fstr_manipulation.h"
#include "json_functions.h"
#include "base_types.h"
#include "base_functions.h"

struct command {
  fstr name;
  int arg;
  float factor;
};
// process the argument, subtitute macros, execute commands
// maybe use a recursive function to do it:
// maybe only the base parameter can have an argument
// or maybe one can use named arguments (fkeyval) as input parameters
//
// void execute_cmds(cam_cfg* cfg, fkeyval cam, ffstr arguments, int arg_idx,
// fstr current_cmd){
// //substitute macro on current_cmd as needed
// //call FUNC on the command
// return
//}
//
void run_cmd_tree(cam_cfg *cfg, fkeyval table, struct command cmd) {
  if (cmd.name == NULL) {
    return;
  }
  request tmp = (request){.name = cmd.name};
  request command =
      cfg->requests[fat_bsearch(request, cfg->requests, CMP(request), tmp)];

  run_cmd_tree(cfg, table, (struct command){.name = command.prev_cmd,
                                            .arg = -2,
                                            .factor = 0.0}); // to the left

  // ffstr headers = cfg->headers;
  ffstr headers = fat_new(fstr, fat_len(fstr, cfg->headers));
  for (int i = 0; headers != NULL && i < fat_len(fstr, headers); i++) {
    headers[i] = substitute_macros(fat_dup(str, cfg->headers[i]), table);
  }

  // duplicates the strings so that it does not change the underlying data
  command.base = substitute_macros(fat_dup(str, command.base), table);
  for (int i = 0; command.headers && i < fat_len(str, command.headers); i++) {
    command.headers[i] =
        substitute_macros(fat_dup(str, command.headers[i]), table);
  }
  printf("args[0]=%s\n", command.args[0]);
  if (cmd.arg > fat_len(str, command.args) ||
      cmd.arg < -1) { // the standard argument is the first one
    command.base = fstr_replace(command.base, "{{ARG}}", command.args[0]);
  } else {
    command.base = fstr_replace(command.base, "{{ARG}}", command.args[cmd.arg]);
  }
  printf("base:%s\n", command.base);
  fflush(stdout);
  // /*
  printf("base_hdrs: \n");
  for (int i = 0; headers != NULL && i < fat_len(fstr, headers); i++) {
    printf("bhdr[%i]:%s\n", i, headers[i]);
  }
  printf("name: %s, base: %s, headers:\n", command.name, command.base);
  for (int i = 0; command.headers != NULL && i < fat_len(fstr, command.headers);
       i++) {
    printf("hdr[%i]:%s", i, command.headers[i]);
    // call correct function here
  }
  // */
  run_cmd_tree(cfg, table, (struct command){.name = command.next_cmd,
                                            .arg = -2,
                                            .factor = 0.0}); // to the right

  fat_free(str, command.base);
  for (int i = 0; command.headers && i < fat_len(str, command.headers); i++) {
    fat_free(str, command.headers[i]);
  }
  for (int i = 0; headers != NULL && i < fat_len(fstr, headers); i++) {
    fat_free(str, headers[i]);
  }
  fat_free(fstr, headers);
  return;
}

int main(int argc, char *argv[]) {
  // ignoring program name, as it's not usefull in this case'
  fstr argjoin = ffstr_join(argv + 1, argc - 1, ' ');
  puts(argjoin);

  ffstr arguments = fstr_explode(argjoin, " -");
  // printf("len:%zu,alloc:%zu\n", fat_len(fstr, arguments),
  //        fat_alloc(fstr, arguments));
  // for (int i = 0; i < fat_len(fstr, arguments); i++) {
  //   printf("%i:%s\n", i, arguments[i]);
  //   fflush(stdout);
  // }
  // for(int i = 0; i < fat_len(str,arguments);i++){
  // arguments[i] = fstr_replace(arguments[i]," ","");
  // puts(arguments[i]);
  // }
  fstr camera = arguments[0];

  int ret = 0;
  // fkeyval cam = read_camera_args("cam1");
  fkeyval cam = read_camera_args(camera);

  if (cam == NULL) {
    fprintf(stderr, "camera of name %s not found\n", camera);
    goto error_cam_args;
  }

  keyval tmp = {.key = "type", .val = NULL};

  // for (int i = 0; i < fat_len(keyval, cam); i++) {
  //   printf("%s\n", cam[i].key);
  // }

  int type_i = fat_bsearch(keyval, cam, CMP(keyval), tmp);

  cam_cfg *cfg = read_cam_config(cam[type_i].val);
  if (cfg == NULL) {
    fprintf(stderr,
            "Something went while parsing the camera config, aborting\n");
    goto error_cam_cfg;
  }
  // print_cam_cfg(cfg);
  fat_sort(request, cfg->requests, CMP(request)); // so we can find the
  // request with a binary search

  // run commands here
  // for(int i = 1; i < fat_len(str,arguments);i++){

  run_cmd_tree(cfg, cam,
               (struct command){.name = "up", .arg = -2, .factor = 0.0});
  // }
  // print_cam_cfg(cfg);

  if (0) {
  error_cam_cfg:
    ret = -1;
  }

  cam_cfg_free(cfg);

  for (int i = 0; i < (int)fat_len(keyval, cam); i++) {
    // printf("%s : %s\n", cam[i].key, cam[i].val);
    fat_free(str, cam[i].key);
    fat_free(str, cam[i].val);
  }
  fat_free(keyval, cam);
  if (0) {
  error_cam_args:
    ret = -2;
  }

  fat_free(fstr, arguments);
  return ret;
}
