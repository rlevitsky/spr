#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <curl/curl.h>

// Change your token here
#define AUTH "Authorization: Bearer xoxp-XXXXXXXXXX-XXXXXXXXXXXXX-XXXXXXXXXXXXX-XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#define URL "https://slack.com/api/users.profile.set"
#define CONTYPE "Content-type: application/json; charset=utf-8"
#define ONBATTERY "{\"profile\": {\"status_text\": \"On Battery\", \"status_emoji\": \":battery:\", \"status_expiration\": 0}}"
#define LOWBATTERY "{\"profile\": {\"status_text\": \"Low Battery\", \"status_emoji\": \":low_battery:\", \"status_expiration\": 0}}"
#define CHARGING "{\"profile\": {\"status_text\": \"Charging\", \"status_emoji\": \":electric_plug:\", \"status_expiration\": 0}}"
#define CLEAN "{\"profile\": {\"status_text\": \"\", \"status_emoji\": \"\", \"status_expiration\": 0}}"
#define UNKNOWN "{\"profile\": {\"status_text\": \"Power status Unkown\", \"status_emoji\": \":grey_question:\", \"status_expiration\": 0}}"

typedef struct mybuff {
  void* bp;
  uint  size;
} mybuff;

void mybuffappend(mybuff* buf, void* vp, uint size)
{
  if (!buf->size) buf->size = 0;
  buf->bp = realloc(buf->bp, buf->size + size);
  memcpy( buf->bp + buf->size, vp, size );
  buf->size += size;
}

uint write_cb(char *in, uint size, uint nmemb, mybuff *out)
{
  uint r;
  r = size * nmemb;
  mybuffappend(out, in, r);
  return r;
}

int do_curl (int status)
{
  int ecode = 1;
  CURL *curl;
  CURLcode res;
  struct curl_slist *list = NULL;
  mybuff docbuf = {0,0};

  res = curl_global_init(CURL_GLOBAL_ALL);
  if(res)
    return (int)res;
  curl = curl_easy_init();
  if(curl) {
    curl_easy_setopt(curl, CURLOPT_URL, URL);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &docbuf);
    list = curl_slist_append(list, CONTYPE);
    list = curl_slist_append(list, AUTH);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);

    switch (status) {
    case 0:
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, CLEAN);
      break;
    case 1:
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, ONBATTERY);
      break;
    case 2:
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, LOWBATTERY);
      break;
    case 3:
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, CHARGING);
      break;
    case 4:
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, UNKNOWN);
      break;
    default:
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, CLEAN);
      break;
    }
    
    res = curl_easy_perform(curl);
    if (res) {
      ecode = res;
      fprintf(stderr, "Got error from libcurl: %d\n", res);
      long response_code;
      res = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
      if ((CURLE_OK == res) && response_code)
        fprintf(stderr, "We received response code: %ld\n", response_code);
    }
    curl_easy_cleanup(curl);
  }
  char *a = strstr(docbuf.bp, "\"ok\":true");
  if ( a == NULL ) {
    printf("Ok not found\n%s\n", (char *)docbuf.bp);
    return 1;
  }
  printf("Ok received\n");
  return 0;
}

int main (int argc, char **argv)
{
  char *statuspath = "/sys/class/power_supply/BAT0/status", *percentpath = "/sys/class/power_supply/BAT0/capacity";
  char *tmpfile = "/tmp/spr.tmp";
  FILE *fp;
  size_t ret;
  unsigned char buf[16], log[32], previous_log[32] = "";
  int percent, slack_status, saved_slack_status, log_length;

  sleep(1);

  setvbuf(stdout, NULL, _IOLBF, 0);

  while (1) {
    fp = fopen(percentpath, "r");
    if (!fp) {
      perror("fopen");
      return EXIT_FAILURE;
    }
    ret = fread(buf, sizeof(*buf), ARRAY_SIZE(buf), fp);
    memset(buf + ret - 1, 0, 1); 
    fclose(fp);
    sscanf(buf, "%d", &percent);
  
    fp = fopen(statuspath, "r");
    if (!fp) {
      perror("fopen");
      return EXIT_FAILURE;
    }
    ret = fread(buf, sizeof(*buf), ARRAY_SIZE(buf), fp);
    memset(buf + ret - 1, 0, 1); 
    fclose(fp);

    if (strstr(buf, "Discharging")) {
      if (percent >= 30) slack_status = 1;
      else slack_status=2;
    }
    else if (strstr(buf, "Charging")) slack_status = 3;
    else if (strstr(buf, "Unknown")) slack_status = 4;
    else if (strstr(buf, "Full") || strstr(buf, "Not charging")) slack_status = 0;
    else slack_status = 4;
  
    log_length = snprintf(log, 32, "%s %d", buf, percent);
    if ( memcmp(log, previous_log, log_length) ) {
      printf("%s\n", log);
      memcpy(previous_log, log, log_length);
    }

    fp = fopen(tmpfile, "r+");
    if (fp == NULL) {
      fp = fopen(tmpfile, "w");
      if (fp == NULL) {
        fprintf(stderr, "Unable to create temporary file\n");
        exit(1);
      }
      else {
        saved_slack_status = slack_status;
        printf("Updating slack status via http\n");
        if (do_curl(slack_status)) {
          fprintf(stderr, "http call error, exiting...\n");
          sleep(5);
          exit(1);
        }
        printf("Writing new status to file\n");
        fwrite(&slack_status, sizeof(int), 1, fp);
      }
    }
    else {
      ret = fread(&saved_slack_status, sizeof(int), 1, fp);
    }
    if (slack_status != saved_slack_status) {
      printf("Updating slack status via http\n");
      if (do_curl(slack_status)) {
        fprintf(stderr, "http call error, exiting...\n");
        sleep(5);
        exit(1);
      }
      printf("Updating slack status file with %d\n", slack_status);
      rewind(fp);
      fwrite(&slack_status, sizeof(int), 1, fp);
    }
    fclose(fp);
    sleep(15);
  }
  return slack_status;
}
