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
// reaaaally got to refactor this function, it should recieve a complete (worked
// up) url, and a header f_array
// int curl_rqst(const char *auth, const char *url, const char *base,
//               const char *arg, const char *filename) {
int curl_rqst(fstr url, ffstr base_headers, ffstr headers){
  CURL *curl;
  CURLcode res;

  curl = curl_easy_init();
  struct curl_slist *list = NULL;
  // FILE *log = fopen(filename, "a");
  // if (log == NULL) {
    // fprintf(stderr, "Unable to open file %s, printing to stdout\n", filename);
    // log = stdout;
  // }
  // char curl_url[1 << 7];
  // char curl_header[1 << 6];

  if (curl) {
    // strcpy(&curl_url[0], "http://");
    // strcat(&curl_url[0], url);
    // strcat(&curl_url[0], base);
    // strcat(&curl_url[0], arg);

    // strcpy(&curl_header[0], "Authorization: Basic ");
    // strcat(&curl_header[0], auth);

    // curl_easy_setopt(curl, CURLOPT_URL, curl_url);
    curl_easy_setopt(curl,CURLOPT_URL,url);
    // printf("%s,%s\n",curl_url,filename);
    /* example.com is redirected, so we tell libcurl to follow redirection */
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    // curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)log);
    for(int i = 0; i < fat_len(fstr,base_headers); i++){
      list = curl_slist_append(list, base_headers[i]);
    }
    for(int i = 0; i < fat_len(fstr,headers);i++){
      list = curl_slist_append(list, base_headers[i]);
    }
    /* Perform the request, res will get the return code */

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);

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
  static double integer;
  static struct timespec wt[2] = {{0, 0}, {0, 0}};
  // printf("%f\n",sec);
  wt[0].tv_nsec = modf(sec, &integer) * 1000000000L;
  wt[0].tv_sec = integer;
  nanosleep(wt, NULL);
}