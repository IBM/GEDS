#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define MAX_SIZE 100 * 1024 * 1024
#define STEP_SIZE 10

void get_timestamp(char *timestamp) {
    time_t now;
    struct tm *tm_info;

    time(&now);
    tm_info = localtime(&now);

    strftime(timestamp, 20, "%Y-%m-%d %H:%M:%S", tm_info);
}

void generate_data(char *data, size_t data_size) {
    char timestamp[20];
    get_timestamp(timestamp);

    size_t text_size = data_size - strlen(timestamp) - 1;
    char *text = malloc(text_size + 1);

    if (text == NULL) {
        exit(EXIT_FAILURE);
    }

    memset(text, 'A', text_size);
    text[text_size] = '\0';
    snprintf(data, data_size + 1, "%s,%s", timestamp, text);
    free(text);
}

int main() {
    FILE *file;
    char *data;
    size_t size, written;
    clock_t start, end;

    // WASI calls happening here
    file = fopen("output.txt", "w");    // Actually writing to GEDS, not just a local file.
    if (file == NULL) {
        return 1;
    }

    for (size = 1024; size <= MAX_SIZE; size *= STEP_SIZE) {
        data = malloc(size + 1);

        if (data == NULL) {
            fclose(file);
            return 1;
        }

        generate_data(data, size);

        start = clock();      // need to import WASI clocks
        written = fwrite(data, 1, size, file);
        end = clock();

        if (written != size) {
            free(data);
            fclose(file);
            return 1;
        }

        double time_taken = ((double)(end - start)) / CLOCKS_PER_SEC;
        printf("Written %d bytes in %.3f seconds.\n", size, time_taken);

        free(data);
    }

    fclose(file);

    return 0;
}
