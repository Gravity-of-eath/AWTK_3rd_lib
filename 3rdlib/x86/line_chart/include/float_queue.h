#include <stdint.h>
#include <stdlib.h>
#include <math.h>

typedef struct {
    float_t* buffer;      // 数据存储区
    int32_t capacity;    // 队列总容量
    int32_t count;       // 当前数据量
    int32_t head;        // 队列头部索引
    int32_t tail;        // 队列尾部索引
} FloatQueue;

int32_t float_queue_init(FloatQueue* queue, int32_t capacity);

void float_queue_free(FloatQueue* queue);

void float_queue_push_n(FloatQueue* queue, float_t* data, int32_t n);

int32_t float_queue_size(const FloatQueue* queue);

int32_t float_queue_empty(const FloatQueue* queue);

float_t float_queue_at(const FloatQueue* queue, int32_t index) ;

int32_t float_queue_reinit(FloatQueue* queue, int32_t new_capacity);