#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <ctime>

#include <curl/curl.h>

#include "download.h"

#define CRUMB_INTERVAL 3600
#define COOKIE_INTERVAL 3600

using namespace std;

download::download() {
	download::crumb_time = 0;
	download::cookie_time = 0;
}

struct data {
  char trace_ascii; /* 1 or 0 */
};

size_t getBody(char *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

size_t getCookieHeader(char *contents, size_t size, size_t nmemb, void *userp) {
	if(size * nmemb > 12 && strncmp(contents, "set-cookie: ", 12) == 0) {
		std::string s;
    	s.append((char*)contents + 12, size * nmemb);
		std::string delimiter = ";"; 
		std::string ckie = s.substr(0, s.find(delimiter)); 
    	((std::string*)userp)->append(ckie);
	} 
    return size * nmemb;
}

void dump(const char *text,
          FILE *stream, unsigned char *ptr, size_t size,
          char nohex)
{
  size_t i;
  size_t c;
 
  unsigned int width = 0x10;
 
  if(nohex)
    /* without the hex output, we can fit more on screen */
    width = 0x40;
 
  fprintf(stream, "%s, %10.10lu bytes (0x%8.8lx)\n",
          text, (unsigned long)size, (unsigned long)size);
 
  for(i = 0; i<size; i += width) {
 
    fprintf(stream, "%4.4lx: ", (unsigned long)i);
 
    if(!nohex) {
      /* hex not disabled, show it */
      for(c = 0; c < width; c++)
        if(i + c < size)
          fprintf(stream, "%02x ", ptr[i + c]);
        else
          fputs("   ", stream);
    }
 
    for(c = 0; (c < width) && (i + c < size); c++) {
      /* check for 0D0A; if found, skip past and start a new line of output */
      if(nohex && (i + c + 1 < size) && ptr[i + c] == 0x0D &&
         ptr[i + c + 1] == 0x0A) {
        i += (c + 2 - width);
        break;
      }
      fprintf(stream, "%c",
              (ptr[i + c] >= 0x20) && (ptr[i + c]<0x80)?ptr[i + c]:'.');
      /* check again for 0D0A, to avoid an extra \n if it's at width */
      if(nohex && (i + c + 2 < size) && ptr[i + c + 1] == 0x0D &&
         ptr[i + c + 2] == 0x0A) {
        i += (c + 3 - width);
        break;
      }
    }
    fputc('\n', stream); /* newline */
  }
  fflush(stream);
}

int trace(CURL *handle, curl_infotype type,
             char *data, size_t size,
             void *userp)
{
  struct data *config = (struct data *)userp;
  const char *text;
 
  switch(type) {
  case CURLINFO_TEXT:
    fprintf(stderr, "== Info: %s", data);
    return 0;
  case CURLINFO_HEADER_OUT:
    text = "=> Send header";
    break;
  case CURLINFO_DATA_OUT:
    text = "=> Send data";
    break;
  case CURLINFO_SSL_DATA_OUT:
    text = "=> Send SSL data";
    break;
  case CURLINFO_HEADER_IN:
    text = "<= Recv header";
    break;
  case CURLINFO_DATA_IN:
    text = "<= Recv data";
    break;
  case CURLINFO_SSL_DATA_IN:
    text = "<= Recv SSL data";
    break;
  default: /* in case a new one is introduced to shock us */
    return 0;
  }

  dump(text, stderr, (unsigned char *)data, size, config->trace_ascii);
 
  return 0;
}

std::string download::request_bin_data(std::string url) {
	return rq(url, false);
}

std::string download::request(std::string url) {
	url = url + "&crumb=" + get_crumb();
	return rq(url, false);
}

std::string download::rq(std::string url, bool returnCookies) {

    CURL* curl;
    CURLcode res;

	std::string userAgent = "User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_10_1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/39.0.2171.95 Safari/537.36";

	curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
			
	struct curl_slist* headers = NULL;
	headers = curl_slist_append(headers, userAgent.c_str()); 

	std::string body, cookies;
    if (curl) {
		if(!returnCookies && !cookie.empty()) {
			std::string ck = "Cookie: ";
    		ck.append(download::cookie);
			headers = curl_slist_append(headers, ck.c_str()); 
		}

		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, getBody);
 		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body);
		curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, getCookieHeader);
   		curl_easy_setopt(curl, CURLOPT_HEADERDATA, &cookies);
		curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");

	   	struct data config;
  		config.trace_ascii = 1; /* enable ascii tracing */ 

		curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, trace);
	    //curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
	    curl_easy_setopt(curl, CURLOPT_DEBUGDATA, &config);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
		}

        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();

	return returnCookies ? cookies : body;
}

void download::fetchCookie() {
	ulong current_time = time(NULL);
	if(current_time <= cookie_time) {
		return;
	}

	log.log("Fetch cookie");

	std::string c = rq("https://fc.yahoo.com", true);

	download::cookie = std::string(c);
	download::cookie_time = COOKIE_INTERVAL + current_time;
}

std::string download::get_crumb() {
	time_t current_time = time(NULL);
	if(current_time <= crumb_time) {
		return crumb;
	}

	log.log("Fetch crumb");

	fetchCookie();
	download::crumb = rq("https://query1.finance.yahoo.com/v1/test/getcrumb", false);
	crumb_time = CRUMB_INTERVAL + current_time;
	return download::crumb;
}
