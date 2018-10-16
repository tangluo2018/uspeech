/*
 *  audio_queue.c
 *
 *  Create on: 2017.02.25
 *     Author: minhua huang
 */

#include <string.h>
#include "audio_queue.h"

int audio_queue_init(audio_queue *queue)
{
    if(queue == NULL){
    	return -1;
    }
    memset(queue, 0, sizeof(audio_queue));
    pthread_mutex_init(&queue->mutex, NULL);
    pthread_cond_init(&queue->cond, NULL);
    return 0;
}

int audio_queue_put(audio_queue *queue, void *data, unsigned int size)
{
    audio_buffer *buf;
    
    buf = (audio_buffer *)malloc(sizeof(audio_buffer));
    if(buf){
      memset(buf, 0, sizeof(audio_buffer));
    }else {
      return -1;
    }
    buf->data = malloc(size);
    if(buf->data){
      memset(buf->data, 0, size);
    }else{
      return -1;
    }
    memcpy(buf->data, data, size);
    buf->size = size;
    buf->next = NULL;

    pthread_mutex_lock(&queue->mutex);
    if(!queue->last_buf){
       queue->first_buf = buf;
    }else{
       queue->last_buf->next = buf;
    }
    queue->last_buf = buf;
    queue->count++;
    queue->size += buf->size;
    pthread_cond_signal(&queue->cond);
    pthread_mutex_unlock(&queue->mutex);

    return 0;
}

int audio_queue_get(audio_queue *queue, audio_buffer *buf)
{
    audio_buffer *buffer;
    int ret;
    
    if(buf == NULL){
      return -1;
    }

    pthread_mutex_lock(&queue->mutex);
    for(;;){
        buffer = queue->first_buf;
        if(buffer){
           queue->first_buf = buffer->next;
           if(!queue->first_buf){
              queue->last_buf = NULL;
           }
           *buf = *buffer;
           queue->count--;
           queue->size -= buffer->size;
           free(buffer);
           ret = 0;
           break;
        }else {
           pthread_cond_wait(&queue->cond, &queue->mutex);
        }
   }
   pthread_mutex_unlock(&queue->mutex);
   return ret;
}

void audio_queue_flush(audio_queue *queue)
{
   audio_buffer *buf, *buf1;
   pthread_mutex_lock(&queue->mutex);
   for(buf = queue->first_buf; buf != NULL; buf = buf1){
      buf1 = buf->next;
      audio_data_free(buf);
      free(buf);
   }
   queue->first_buf = NULL;
   queue->last_buf = NULL;
   queue->count = 0;
   queue->size = 0;
   pthread_mutex_unlock(&queue->mutex);
}

void audio_data_free(audio_buffer *buffer)
{
   if(buffer != NULL && buffer->data != NULL){
      free(buffer->data);
      buffer->data = NULL;
   }
}

void audio_queue_free(audio_queue *queue)
{
   if(queue != NULL){
     audio_queue_flush(queue);
     free(queue);
   }
}

unsigned int audio_queue_size(audio_queue *queue)
{
	unsigned int size = 0;
	pthread_mutex_lock(&queue->mutex);
	size = queue->size;
	pthread_mutex_unlock(&queue->mutex);
	return size;
}

unsigned int audio_queue_count(audio_queue *queue)
{
	unsigned int count = 0;
	pthread_mutex_lock(&queue->mutex);
	count = queue->count;
	pthread_mutex_unlock(&queue->mutex);
	return count;

}
