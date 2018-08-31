// missing definitions from time.h  in c99, but not in c11 /*
#ifndef _POSIX_C_SOURCE
  #define _POSIX_C_SOURCE 199309L
    #if __STDC_VERSION__ == 199901L
    struct timespec {
      time_t tv_sec;
      long tv_nsec;
    };

    #endif
  int nanosleep(const struct timespec *req, struct timespec *rem);
#endif
// */

// modified from libcurl example
int curl_rqst(fstr url, ffstr base_headers, ffstr headers){
  CURL *curl;
  CURLcode res;

  curl = curl_easy_init();
  struct curl_slist *list = NULL;

  if (curl) {

    curl_easy_setopt(curl,CURLOPT_URL,url);
    // printf("%s,%s\n",curl_url,filename);

    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    for(int i = 0; base_headers && i < fat_len(fstr,base_headers); i++){
      list = curl_slist_append(list, base_headers[i]);
    }
    for(int i = 0; headers && i < fat_len(fstr,headers);i++){
      list = curl_slist_append(list, headers[i]);
    }
    /* Perform the request, res will get the return code */

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);

    // printf("url:%s",url);
    res = curl_easy_perform(curl);
    /* Check for errors */
    if (res != CURLE_OK) {
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));
      // return -1;
    }
    /* always cleanup */
    curl_easy_cleanup(curl);
    curl_slist_free_all(list);
  }
  // fclose(log);
  return 0;
}
// divides sleep automatically, whole part goes to seconds, fractional part goes
// to nanoseconds, as tv_nsec can only hold a value so big
void n_sleep(double sec) {
  // printf("sleep:%F",sec);
  static double integer;
  static struct timespec wt[2] = {{0, 0}, {0, 0}};
  // printf("%f\n",sec);
  wt[0].tv_nsec = modf(sec, &integer) * 1000000000L;
  wt[0].tv_sec = integer;
  nanosleep(wt, NULL);
}


// from http://www.linuxmisc.com/9-unix-programmer/e9699aae73de0158.htm
// static struct termios originalTermParam; 
// static void set_keypress() { 
//   struct termios currentTermParam; 
//   tcgetattr( 0, &originalTermParam ); 
//   memcpy( &currentTermParam, &originalTermParam, sizeof( struct termios ) ); 
//   /* 
//    * Disable canonical mode, and set buffer size to 1 byte 
//    */ 
//   currentTermParam.c_lflag       &= ~ICANON; 
//   currentTermParam.c_lflag       &= ~ECHO; 
//   currentTermParam.c_cc[ VTIME ]  = 255; 
//   currentTermParam.c_cc[VMIN]     = 1; 
//   tcsetattr( 0, TCSANOW, &currentTermParam ); 
//   return; 
// }

// static void reset_keypress() { 
//   tcsetattr( 0, TCSANOW, &originalTermParam ); 
//   return; 
// }
//curl 'http://192.168.1.100/decoder_control.cgi?command=0&user=utfpr&pwd=utfpr'