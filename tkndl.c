#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <sys/stat.h>

size_t write_callback(void *ptr, size_t size, size_t nmemb, void *stream) {
    return fwrite(ptr, size, nmemb, (FILE *)stream);
}

int progress_callback(void *clientp, curl_off_t dltotal, curl_off_t dlnow,
                      curl_off_t ultotal, curl_off_t ulnow) {
    if (dltotal > 0) {
        int percent = (int)((double)dlnow / dltotal * 100);
        printf("\rDownloading: %d%%", percent);
        fflush(stdout);
    }
    return 0;
}

void print_usage() {
    printf("Usage: tkndl <URL> [-o output_file] [-r resume]\n");
    printf("  URL          : http:// or https://\n");
    printf("  -o output    : Output file name\n");
    printf("  -r           : Resume interrupted download\n");
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage();
        return 1;
    }

    char *url = argv[1];
    char *output_file = NULL;
    int resume = 0;

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            output_file = argv[++i];
        } else if (strcmp(argv[i], "-r") == 0) {
            resume = 1;
        }
    }

    if (!output_file) {
        // 从 URL 提取文件名
        char *last_slash = strrchr(url, '/');
        if (last_slash && *(last_slash + 1) != '\0') {
            output_file = last_slash + 1;
        } else {
            output_file = "download";
        }
    }

    curl_global_init(CURL_GLOBAL_ALL);
    CURL *curl = curl_easy_init();
    if (!curl) {
        printf("Error: curl init failed\n");
        return 1;
    }

    FILE *out = fopen(output_file, resume ? "ab" : "wb");
    if (!out) {
        perror("fopen");
        curl_easy_cleanup(curl);
        return 1;
    }

    if (resume) {
        fseek(out, 0, SEEK_END);
        long pos = ftell(out);
        if (pos > 0) {
            curl_easy_setopt(curl, CURLOPT_RESUME_FROM_LARGE, (curl_off_t)pos);
            printf("Resuming from %ld bytes\n", pos);
        }
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, out);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    CURLcode res = curl_easy_perform(curl);
    printf("\n");

    fclose(out);
    curl_easy_cleanup(curl);
    curl_global_cleanup();

    if (res == CURLE_OK) {
        printf("Download complete: %s\n", output_file);
    } else {
        printf("Download failed: %s\n", curl_easy_strerror(res));
        return 1;
    }
    return 0;
}
