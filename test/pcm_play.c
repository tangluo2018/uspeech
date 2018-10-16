#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <tinyalsa/asoundlib.h>

static int close = 0;

static void set_volume(void *data, int size, int ratio)
{
     short sample;
     short *snd_data;
     int i;
     
     snd_data = (short *)data;
     for(i=0; i<size/2; i++){
       snd_data[i] = snd_data[i] >> ratio;
    }
}

static void stream_close(int sig)
{
   signal(sig, SIG_IGN);
   close = 1;
}

int main(int argc, char* argv[])
{
   int error;
   struct pcm_config config;
   struct pcm* pcm;
   char * fpath;
   FILE* file;
   char* buffer;
   int size;
   int num_read;
   int num_size;
   long flen, fsta, fend;
   char* gbuffer;
   char* filebuf;
   char* tempbuf;

   if(argc < 2){
      printf("Usage: pcm_play file.pcm\n");
      return 0;
   }
   fpath = argv[1];

   file = fopen(fpath, "rb+");
   if(!file){
      printf("Unable to open %s\n", fpath);
      return -1;
   }

   /* catch ctrl-c to shutdown cleanly */
   signal(SIGINT, stream_close);

   #if 0
   gbuffer = malloc(1024 * 120); /*120K*/
   if(!gbuffer){
     printf("gbuffer malloc failed\n");
     return -1;
   }
   memset(gbuffer, 0, 1024 * 120);
   tempbuf = gbuffer;
   fsta = ftell(file);
   fseek(file, 0, SEEK_END);
   fend = ftell(file);
   flen = fend - fsta;
   fseek(file, 0, SEEK_SET);
   filebuf = malloc(4096);
   if(!filebuf){
     printf("filebuf malloc failed\n");
   }
   printf("fread here now ... \n");
   num_read = 0;
   num_size = 0;
   while(1){
      num_read = fread(filebuf, 4096, 1, file);
      if(num_read > 0 && num_size <= flen){
         printf("file read size %d\n", num_read);
         memcpy(gbuffer +  1024 * 20, filebuf, num_read);
         num_size += num_read;
	 gbuffer += num_read;
      }else{
         break;
      }
   }
   #endif

   config.channels = 2;
   config.rate = 8000;
   config.format = PCM_FORMAT_S16_LE;
   config.start_threshold = 0;
   config.stop_threshold = 0;
   config.silence_threshold = 0;
   config.period_size = 1024;
   config.period_count = 4;

   pcm = pcm_open(1, 0, PCM_OUT, &config);
   if(!pcm || !pcm_is_ready(pcm)){
      printf("Unable to open playback PCM device (%s)\n", pcm_get_error(pcm));
      return -1;
   }

   size = pcm_frames_to_bytes(pcm, pcm_get_buffer_size(pcm));
   buffer = malloc(size);
   if(!buffer){
      printf("buffer malloc failed\n");
      pcm_close(pcm);
      return -1;
   }

   #if 0
   num_read = 0;
   while(1){
     if(num_read > flen){
        printf("All file data is read\n");
	break;
     }
     memcpy(buffer, tempbuf, size);
     set_volume(buffer, size, 5);
     if(pcm_write(pcm, buffer, size)){
        printf("pcm write failed\n");
	break;
     }
     tempbuf += size;
     num_read = size;
   }
   pcm_close(pcm);
   free(tempbuf);
   return 0;
   #endif
  
   #if 1 
   do{
      num_read = fread(buffer, 1, size, file);
      if(num_read > 0){
         set_volume(buffer, size, 5);
         if(pcm_write(pcm, buffer, num_read)){
	     printf("pcm write failed\n");
	     break;
	  }
     }
   }while(!close && num_read > 0);
   #endif


   sleep(1);
  

   pcm_close(pcm);
   free(buffer);
   return 0;
}
