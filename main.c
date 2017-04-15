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

#define VERSION "0.0.4" // git

// ordem correta
// #include "fstr_manipulation.h"
// #include "json_functions.h"
// #include "base_types.h"
// #include "base_functions.h"
// #include "external.h"

#include "fstr_manipulation.h"
#include "json_functions.h"
#include "base_types.h"
#include "base_functions.h"
#include "external.h"

struct command {
  fstr name;
  fstr function;
  int arg;
  double factor;
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
void debug(fstr base, ffstr base_headers, ffstr headers) {
  printf("base:\"%s\"\n", base);
  if (base_headers) { //!= NULL
    printf("b_headers:[");
    for (int i = 0; i < fat_len(fstr, base_headers); i++) {
      printf("\"%s\",", base_headers[i]);
    }
    printf("]\n");
  }
  if (headers) { //!= NULL
    printf("headers:[");
    for (int i = 0; i < fat_len(fstr, headers); i++) {
      printf("\"%s\",", headers[i]);
    }
    printf("]\n");
  }
  return;
}

void run_cmd_tree(cam_cfg *cfg, fkeyval table, struct command cmd) {
  if (cmd.name == NULL) {
    return;
  }

  request tmp = (request){.name = cmd.name};
  int index_command = fat_bsearch(request, cfg->requests, CMP(request), tmp);
  // printf("\t[%s,%i]\n",cmd.name,index_command);
  if (index_command == -1) {
    return;
  }
  request command = cfg->requests[index_command];

  run_cmd_tree(cfg, table,
               (struct command){.name = command.prev_cmd,
                                .arg = -2,
                                .factor = 0.0,
                                .function = cmd.function}); // to the left

  ffstr headers = fat_new(fstr, fat_len(fstr, cfg->headers));
  fat_setlen(fstr, headers, fat_len(fstr, cfg->headers));
  for (int i = 0; headers != NULL && i < fat_len(fstr, headers); i++) {
    headers[i] = substitute_macros(fat_dup(str, cfg->headers[i]), table);
  }

  // duplicates the strings so that it does not change the underlying data
  command.base = substitute_macros(fat_dup(str, command.base), table);
  for (int i = 0; command.headers && i < fat_len(str, command.headers); i++) {
    command.headers[i] =
        substitute_macros(fat_dup(str, command.headers[i]), table);
  }
  // printf("args[0]=%s\n", command.args[0]);
  if (cmd.arg > fat_len(str, command.args) ||
      cmd.arg < -1) { // the standard argument is the first one
    command.base = fstr_replace(command.base, "{{ARG}}", command.args[0]);
  } else {
    command.base = fstr_replace(command.base, "{{ARG}}", command.args[cmd.arg]);
  }

  /*
   printf("base:%s\n", command.base);
   printf("base_hdrs: \n");
   fflush(stdout);
   for (int i = 0; headers != NULL && i < fat_len(fstr, headers); i++) {
     printf("bhdr[%i]:%s\n", i, headers[i]);
   }
   printf("name: %s, base: %s, headers:\n", command.name, command.base);
   for (int i = 0; command.headers != NULL && i < fat_len(fstr,
   command.headers);
        i++) {
     printf("hdr[%i]:%s", i, command.headers[i]);
   }
   // */

  //  decide que função chamar, comparando seu nome
  if (cmd.function != NULL) { // não da pra fazer switch em strcmp, talvez o
                              // hash
    // printf("Running function: %s\n", cmd.function);
    if (0 == strcmp("debug", cmd.function)) {
      debug(command.base, headers, command.headers);
    } else if (0 == strcmp("curl", cmd.function)) {
      curl_rqst(command.base, headers, command.headers);
    }
  }
  // printf("sleep! %lf %lf\n",cmd.factor,command.fact_to_next);
  n_sleep(cmd.factor * command.fact_to_next);
  run_cmd_tree(cfg, table,
               (struct command){.name = command.next_cmd,
                                .arg = -2,
                                .factor = 0.0,
                                .function = cmd.function}); // to the right

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

struct command parse_user_command(char *input) {
  struct command out = {.name = NULL, .arg = -2, .factor = 0.0};
  ffstr tmp = fstr_explode(input, ",");
  char *breaker;
  for (int i = 0; i < fat_len(fstr, tmp); i++) {
    if ((breaker = strstr(tmp[i], "n:")), breaker != NULL) {
      // breaker = strstr(tmp[i],"n:")+1;
      out.name = breaker + 2;
      breaker = strchr(breaker, ']');
      if (breaker)
        *breaker = '\0';
    } else if ((breaker = strstr(tmp[i], "a:")), breaker != NULL) {
      breaker = strchr(tmp[i], ']');
      if (breaker)
        *breaker = '\0';
      breaker = strstr(tmp[i], "a:") + 2;
      out.arg = atoi(breaker);
    } else if ((breaker = strstr(tmp[i], "f:")), breaker != NULL) {
      breaker = strchr(tmp[i], ']');
      if (breaker)
        *breaker = '\0';
      breaker = strstr(tmp[i], "f:") + 2;
      out.factor = strtod(breaker, NULL);
    }
  }
  // return (struct command){};
  return out;
}

int main(int argc, char *argv[]) {
  // ignoring program name, as it's not usefull in this case'
  // fstr argjoin = ffstr_join(argv + 1, argc - 1, ' ');
  // puts(argjoin);

  // ffstr arguments = fstr_explode(argjoin, " -cmd=");

  char argfake[60];
  strcpy(argfake, "cam1 -cmd=[n:up,a:0,f:1] -cmd=[n:up]");
  ffstr arguments = fstr_explode(argfake, " -cmd=");
  // printf("len:%zu,alloc:%zu\n", fat_len(fstr, arguments),
  //        fat_alloc(fstr, arguments));
  struct command user_commands[fat_len(fstr, arguments) - 1];
  for (int i = 1; i < fat_len(fstr, arguments); i++) {
    // printf("%i:%s\n", i, arguments[i]);
    user_commands[i - 1] = parse_user_command(arguments[i]);
    // puts("");
  }
  // fflush(stdout);
  // for(int i = 0; i < fat_len(str,arguments);i++){
  // // arguments[i] = fstr_replace(arguments[i]," ","");
  //   puts(arguments[i]);
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
  int type_i = fat_bsearch(keyval, cam, CMP(keyval), tmp);

  tmp = (keyval){.key = "function", .val = NULL};
  char *cam_function = cam[fat_bsearch(keyval, cam, CMP(keyval), tmp)].val;

  // if camera config has valid cache
  uint16_t curr_cksum = cksum_file(camera);
  // use cache
  cam_cfg *cfg = NULL;
  if (valid_cache(curr_cksum, cam[type_i].val)) {
    cfg = load_cam_cache(cam[type_i].val);
  } else { // file has been updated
    cfg = read_cam_config(cam[type_i].val);
    if (cfg == NULL) {
      fprintf(stderr,
              "Something went while parsing the camera config, aborting\n");
      goto error_cam_cfg;
    }
    // print_cam_cfg(cfg);
    fat_sort(request, cfg->requests, CMP(request)); // so we can find the
    // request with a binary search
    save_cam_cache(curr_cksum, cam[type_i].val, cfg);
    // printf("len:%zu",fat_len(fstr,arguments));
  }

  // run commands here
  for (int i = 0; i < fat_len(fstr, arguments) - 1; i++) {
    user_commands[i].function = cam_function;
    // printf("n:\'%s\',a:\'%i\',f:\'%lf\',func:\'%s\'\n",
    // user_commands[i].name,user_commands[i].arg,
    // user_commands[i].factor,user_commands[i].function);

    run_cmd_tree(cfg, cam, user_commands[i]);
    // run_cmd_tree(cfg, cam, (struct command){.name = "up",
    //                                         .arg = -2,
    //                                         .factor = 1.0,
    //                                         .function = "debug"});
  }
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
