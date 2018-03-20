#include <arm_neon.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#define SIZE 1000000
#define FILE_NAME "input.txt"

float32_t mul(float *source, float *weight);
void create_input(int size);

static long diff_in_us(struct timespec t1, struct timespec t2)
{
    struct timespec diff;
    if (t2.tv_nsec - t1.tv_nsec < 0) {
        diff.tv_sec = t2.tv_sec - t1.tv_sec - 1;
        diff.tv_nsec = t2.tv_nsec - t1.tv_nsec + 1000000000;
    } else {
        diff.tv_sec = t2.tv_sec - t1.tv_sec;
        diff.tv_nsec = t2.tv_nsec - t1.tv_nsec;
    }
    return (diff.tv_sec * 1000000.0 + diff.tv_nsec / 1000.0);
}

int main()
{
    struct timespec start, end;
    float source[SIZE], weight[SIZE];
    FILE *fptr;
    create_input(SIZE);
    fptr = fopen(FILE_NAME, "r");
    for (int i = 0; i < SIZE; ++i) {
        fscanf(fptr, "%f %f\n", &source[i], &weight[i]);
    }
    clock_gettime(CLOCK_REALTIME, &start);
    printf("output:  %lf\n", mul(source, weight));
    clock_gettime(CLOCK_REALTIME, &end);
    printf("spend:  %ld us\n", diff_in_us(start, end));
    fclose(fptr);
    return 0;
}

float32_t mul(float *source, float *weights)
{
#ifndef SIMPLE
    float32x4_t in1_128, in2_128, sum1, sum2, prod;
    float32_t result[4];
#endif
    float32_t output = 0.0;
#ifdef FLUSH4
    float32x4_t in3_128, in4_128;
#endif
#ifdef NON_FLUSH
    prod = vmovq_n_f32(0.0f);
#endif
    for (int i = 0; i < SIZE;) {
#ifdef SIMPLE
        output += source[i] * weights[i];
        ++i;
#else
        in1_128 = vld1q_f32(&source[i]);
        in2_128 = vld1q_f32(&weights[i]);
        i += 4;
#endif
#ifdef FLUSH4
        in3_128 = vld1q_f32(&source[i]);
        in4_128 = vld1q_f32(&weights[i]);
        i += 4;
        prod = vaddq_f32(vmulq_f32(in1_128, in2_128), vmulq_f32(in3_128, in4_128));
        sum1 = vaddq_f32(prod, vrev64q_f32(prod));
        sum2 =
            vaddq_f32(sum1, vcombine_f32(vget_high_f32(sum1), vget_low_f32(sum1)));
        vst1q_f32((float32_t *)result, sum2);
        output += result[0];
#endif
#ifdef FLUSH
        prod = vmulq_f32(in1_128, in2_128);
        sum1 = vaddq_f32(prod, vrev64q_f32(prod));
        sum2 =
            vaddq_f32(sum1, vcombine_f32(vget_high_f32(sum1), vget_low_f32(sum1)));
        vst1q_f32((float32_t *)result, sum2);
        output += result[0];
#endif
#ifdef NON_FLUSH
        prod = vmlaq_f32(prod, in1_128, in2_128);
#endif
    }
#ifdef NON_FLUSH
    sum1 = vaddq_f32(prod, vrev64q_f32(prod));
    sum2 = vaddq_f32(sum1, vcombine_f32(vget_high_f32(sum1), vget_low_f32(sum1)));
    vst1q_f32((float32_t *)result, sum2);
    output = result[0];
#endif

    return output;
}

void create_input(int size)
{
    FILE *fptr;
    if (access(FILE_NAME, F_OK) != -1) {
        return;
    }

    fptr = fopen(FILE_NAME, "w");
    srand(time(NULL));
    float32_t source, weight;
    for (int i = 0; i < size; ++i) {
        // random between [1, 5]
        source = (float32_t)rand() / (float32_t)(RAND_MAX / 4) + 1;
        weight = (float32_t)rand() / (float32_t)(RAND_MAX / 4) + 1;
        fprintf(fptr, "%f %f\n", source, weight);
    }
    fclose(fptr);
}
