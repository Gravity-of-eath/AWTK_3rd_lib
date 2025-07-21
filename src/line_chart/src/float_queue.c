#include "float_queue.h"
#include "tkc/mem.h"
/**
 * 初始化队列
 * @param queue 队列指针
 * @param capacity 队列容量
 * @return 0成功，-1失败
 */
int32_t float_queue_init(FloatQueue *queue, int32_t capacity)
{
    queue->buffer = (float_t *)TKMEM_CALLOC(capacity, sizeof(float_t));
    if (!queue->buffer)
        return -1;

    queue->capacity = capacity;
    queue->count = 0;
    queue->head = 0;
    queue->tail = 0;
    return 0;
}

/**
 * 释放队列资源
 * @param queue 队列指针
 */
void float_queue_free(FloatQueue *queue)
{
    if (queue->buffer)
    {
        free(queue->buffer);
        queue->buffer = NULL;
    }
    queue->capacity = 0;
    queue->count = 0;
}

/**
 * 向队列添加N个数据
 * @param queue 队列指针
 * @param data 数据数组
 * @param n 要添加的数据个数
 */
void float_queue_push_n(FloatQueue *queue, float_t *data, int32_t n)
{
    if (n == 0 || !queue || !data)
        return;

    // 如果要添加的数据超过容量，只保留最后capacity个数据
    if (n >= queue->capacity)
    {
        int32_t offset = n - queue->capacity;
        memcpy(queue->buffer, data + offset, queue->capacity * sizeof(float_t));
        queue->head = 0;
        queue->tail = 0;
        queue->count = queue->capacity;
        return;
    }

    // 计算需要丢弃的老数据数量
    int32_t overflow = (queue->count + n > queue->capacity) ? (queue->count + n - queue->capacity) : 0;

    // 丢弃overflow个老数据
    if (overflow > 0)
    {
        queue->head = (queue->head + overflow) % queue->capacity;
        queue->count -= overflow;
    }

    // 添加新数据
    for (int32_t i = 0; i < n; i++)
    {
        queue->buffer[queue->tail] = data[i];
        queue->tail = (queue->tail + 1) % queue->capacity;
    }

    queue->count += n;
}

/**
 * 获取队列当前数据量
 * @param queue 队列指针
 * @return 当前数据个数
 */
int32_t float_queue_size(const FloatQueue *queue)
{
    return queue->count;
}

/**
 * 检查队列是否为空
 * @param queue 队列指针
 * @return 1为空，0非空
 */
int32_t float_queue_empty(const FloatQueue *queue)
{
    return queue->count == 0;
}

/**
 * 获取队列中第index个元素（0表示最早的元素）
 * @param queue 队列指针
 * @param index 索引
 * @return 元素值
 */
float_t float_queue_at(const FloatQueue *queue, int32_t index)
{
    if (index >= queue->count)
        return 0.0f;
    return queue->buffer[(queue->head + index) % queue->capacity];
}

/**
 * 重新初始化队列容量
 * @param queue 队列指针
 * @param new_capacity 新的容量大小
 * @return 0成功，-1失败
 */
int32_t float_queue_reinit(FloatQueue *queue, int32_t new_capacity)
{
    if (!queue || new_capacity == 0)
        return -1;

    // 如果新容量等于当前容量，无需任何操作
    if (new_capacity == queue->capacity)
        return 0;

    // 分配新缓冲区
    float_t *new_buffer = (float_t *)TKMEM_CALLOC(new_capacity, sizeof(float_t));
    if (!new_buffer)
        return -1;

    // 计算需要保留的数据量
    int32_t data_to_keep = (queue->count < new_capacity) ? queue->count : new_capacity;

    // 将数据从旧缓冲区复制到新缓冲区
    if (data_to_keep > 0)
    {
        if (new_capacity > queue->capacity)
        {
            // 扩容情况：保留所有原数据
            for (int32_t i = 0; i < data_to_keep; i++)
            {
                new_buffer[i] = float_queue_at(queue, i);
            }
        }
        else
        {
            // 缩容情况：只保留最后new_capacity个数据
            int32_t start_index = queue->count - new_capacity;
            for (int32_t i = 0; i < data_to_keep; i++)
            {
                new_buffer[i] = float_queue_at(queue, start_index + i);
            }
        }
    }

    // 释放旧缓冲区并更新队列属性
    free(queue->buffer);
    queue->buffer = new_buffer;
    queue->capacity = new_capacity;
    queue->count = data_to_keep;
    queue->head = 0;
    queue->tail = (data_to_keep == new_capacity) ? 0 : data_to_keep;

    return 0;
}