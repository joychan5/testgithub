/*****************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * $Id: multi-post.c,v 1.6 2008-05-22 21:20:09 danf Exp $
 *
 * This is an example application source code using the multi interface
 * to do a multipart formpost without "blocking".
 */
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <math.h>
#include <curl/curl.h>

int curl2_progress_callback(void* ptr, double TotalToDownload, double NowDownloaded, 
                    double TotalToUpload, double NowUploaded);
int curl1_progress_callback(void* ptr, double TotalToDownload, double NowDownloaded,
                    double TotalToUpload, double NowUploaded);
FILE *fcurl1;
FILE *fcurl2;

int main(int argc, char *argv[])
{
  CURL *curl;
  CURL *curl2;

  CURLM *multi_handle;
  int still_running;

  struct curl_httppost *formpost=NULL;
  struct curl_httppost *lastptr=NULL;
  struct curl_slist *headerlist=NULL;
  static const char buf[] = "Expect:";

  fcurl1 = fopen("/tmp/fcurl1", "a+");
  fcurl2 = fopen("/tmp/fcurl2", "a+");
  //setbuf(fcurl1,NULL);
  //setbuf(fcurl2,NULL);

  /* Fill in the file upload field. This makes libcurl load data from  
     the given file name when curl_easy_perform() is called. */
  curl_formadd(&formpost,
               &lastptr,
               CURLFORM_COPYNAME, "file1",
               CURLFORM_FILE, "/home/joychan/Pictures/p_large_B7bw_6531l016062.jpg",
               CURLFORM_END);



  /* Fill in the filename field */
  curl_formadd(&formpost,
               &lastptr,
               CURLFORM_COPYNAME, "filename",
               CURLFORM_COPYCONTENTS, "postit2.c",
               CURLFORM_END);

  /* Fill in the submit field too, even if this is rarely needed */
  curl_formadd(&formpost,
               &lastptr,
               CURLFORM_COPYNAME, "submit",
               CURLFORM_COPYCONTENTS, "send",
               CURLFORM_END);

  curl = curl_easy_init();
  curl2 = curl_easy_init();
  multi_handle = curl_multi_init();

  /* initalize custom header list (stating that Expect: 100-continue is not
     wanted */
  headerlist = curl_slist_append(headerlist, buf);
  if(curl && multi_handle) {

    /* what URL that receives this POST */
    curl_easy_setopt(curl, CURLOPT_URL,
                     "http://freedomhui.com/test/upload.php");
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);
	curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, curl1_progress_callback);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);
    curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);

    /* what URL that receives this POST */
    curl_easy_setopt(curl2, CURLOPT_URL,
                     "http://freedomhui.com/test/upload.php");
    curl_easy_setopt(curl2, CURLOPT_VERBOSE, 1L);
	curl_easy_setopt(curl2, CURLOPT_NOPROGRESS, 0);
	curl_easy_setopt(curl2, CURLOPT_PROGRESSFUNCTION, curl2_progress_callback);
    curl_easy_setopt(curl2, CURLOPT_HTTPHEADER, headerlist);
    curl_easy_setopt(curl2, CURLOPT_HTTPPOST, formpost);


    curl_multi_add_handle(multi_handle, curl);
    curl_multi_add_handle(multi_handle, curl2);

    while(CURLM_CALL_MULTI_PERFORM ==
          curl_multi_perform(multi_handle, &still_running));

    while(still_running) {
      struct timeval timeout;
      int rc; /* select() return code */

      fd_set fdread;
      fd_set fdwrite;
      fd_set fdexcep;
      int maxfd;

      FD_ZERO(&fdread);
      FD_ZERO(&fdwrite);
      FD_ZERO(&fdexcep);

      /* set a suitable timeout to play around with */
      timeout.tv_sec = 1;
      timeout.tv_usec = 0;

      /* get file descriptors from the transfers */
      curl_multi_fdset(multi_handle, &fdread, &fdwrite, &fdexcep, &maxfd);

      /* In a real-world program you OF COURSE check the return code of the
         function calls, *and* you make sure that maxfd is bigger than -1
         so that the call to select() below makes sense! */

      rc = select(maxfd+1, &fdread, &fdwrite, &fdexcep, &timeout);

      switch(rc) {
      case -1:
        /* select error */
        break;
      case 0:
        printf("timeout!\n");
      default:
        /* timeout or readable/writable sockets */
        printf("perform!\n");
        while(CURLM_CALL_MULTI_PERFORM ==
              curl_multi_perform(multi_handle, &still_running));
        printf("running: %d!\n", still_running);
        break;
      }
    }

    curl_multi_cleanup(multi_handle);

    /* always cleanup */
    curl_easy_cleanup(curl);

    /* then cleanup the formpost chain */
    curl_formfree(formpost);

    /* free slist */
    curl_slist_free_all (headerlist);
  }
  return 0;
}

int curl1_progress_callback(void* ptr, double TotalToDownload, double NowDownloaded, 
                    double TotalToUpload, double NowUploaded)
{
    // how wide you want the progress meter to be
    int totaldotz=40;
    double fractiondownloaded = NowUploaded / TotalToUpload;
    // part of the progressmeter that's already "full"
    int dotz = round(fractiondownloaded * totaldotz);

    // create the "meter"
    int ii=0;
    fprintf(fcurl1, "curl1: %3.0f ",fractiondownloaded*100);
/*
    fprintf(fcurl1,"curl1: %3.0f%% [",fractiondownloaded*100);
    // part  that's full already
    for ( ; ii < dotz;ii++) {
       fprintf(fcurl1,"=");
    }
    // remaining part (spaces)
    for ( ; ii < totaldotz;ii++) {
        fprintf(fcurl1," ");
    }
    // and back to line begin - do not forget the fflush to avoid output buffering problems!
    fprintf(fcurl1,"]\r");
*/
    fflush(fcurl1);
	return 0;
}


int curl2_progress_callback(void* ptr, double TotalToDownload, double NowDownloaded, 
                    double TotalToUpload, double NowUploaded)
{
    // how wide you want the progress meter to be
    int totaldotz=40;
    double fractiondownloaded = NowUploaded / TotalToUpload;
    // part of the progressmeter that's already "full"
    int dotz = round(fractiondownloaded * totaldotz);

    // create the "meter"
    int ii=0;
    fprintf(fcurl2, "curl2: %3.0f ",fractiondownloaded*100);
/*
    fprintf(fcurl2, "curl2: %3.0f%% [",fractiondownloaded*100);
    // part  that's full already
    for ( ; ii < dotz;ii++) {
        fprintf(fcurl2, "=");
    }
    // remaining part (spaces)
    for ( ; ii < totaldotz;ii++) {
        fprintf(fcurl2," ");
    }
    // and back to line begin - do not forget the fflush to avoid output buffering problems!
    fprintf(fcurl2,"]\r");
*/
    fflush(fcurl2);
	return 0;
}

