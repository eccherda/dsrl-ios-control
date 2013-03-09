//
//  dsrl-stream.c
//  DSRLStream
//
//  Created by David Eccher on 09/03/13.
//
//  Derived from libgphoto2 example-capture.c
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
//  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <arpa/inet.h> 
#include <assert.h> 
#include <errno.h> 
#include <signal.h> 
#include <string.h> 
#include <sys/wait.h> 
#include <netdb.h> 
#include <unistd.h> 

#include <gphoto2/gphoto2-camera.h>



Camera *camera;
GPContext *context;
static pthread_t thread,listenthread;
static pthread_mutex_t control_mutex;
static int stop=0;
int delay = 80000; //12fps

static int sock, length, n;
struct sockaddr_in server, from;

int port=0;




void setsocketoption()
{
  int len, trysize, gotsize,err;
  len = sizeof(int);
  trysize = 1048576+32768;
  do {
     trysize -= 32768;
     setsockopt(sock,SOL_SOCKET,SO_SNDBUF,(char*)&trysize,len);
     err = getsockopt(sock,SOL_SOCKET,SO_SNDBUF,(char*)&gotsize,&len);
      if (err < 0) { printf("Error: getsockopt\n"); break; }

  } while (gotsize < trysize);
  printf("Size set to %d\n",gotsize);
}

int createsocket(char *argv[])
{

   
   struct hostent *hp;

   sock= socket(AF_INET, SOCK_DGRAM, 0);
   if (sock < 0) printf("Error:socket\n");

   server.sin_family = AF_INET;
   port=atoi(argv[2]);
   hp = gethostbyname(argv[1]);
   if (hp==0) printf("Error: Unknown host\n");

   bcopy((char *)hp->h_addr, 
        (char *)&server.sin_addr,
         hp->h_length);
   server.sin_port = htons(port);
   length=sizeof(struct sockaddr_in);



   setsocketoption();
}


static void
 errordumper(GPLogLevel level, const char *domain, const char *str, void *data) {
   fprintf(stderr, "%s\n", str);
}

int camera_set(char* name, void* value)
{
  int res;

  CameraWidget* config_root;
  CameraWidget* widget;
  res = gp_camera_get_config(camera, &config_root, context);
  res = gp_widget_get_child_by_name(config_root, name, &widget);
  res = gp_widget_set_value(widget, value);
  res = gp_camera_set_config(camera, config_root, context);
  gp_widget_unref(config_root);
  return 1;
}


int timeval_subtract(struct timeval *result, struct timeval *t2, struct timeval *t1)
{
    long int diff = (t2->tv_usec + 1000000 * t2->tv_sec) - (t1->tv_usec + 1000000 * t1->tv_sec);
    result->tv_sec = diff / 1000000;
    result->tv_usec = diff % 1000000;

    return (diff<0);
}

void timeval_print(struct timeval *tv)
{
    char buffer[30];
    time_t curtime;

    printf("%ld.%06ld", tv->tv_sec, tv->tv_usec);
    curtime = tv->tv_sec;
    strftime(buffer, 30, "%m-%d-%Y  %T", localtime(&curtime));
    printf(" = %s.%06ld\n", buffer, tv->tv_usec);
}



static int
_lookup_widget(CameraWidget*widget, const char *key, CameraWidget **child) {
  int ret;
  ret = gp_widget_get_child_by_name (widget, key, child);
  if (ret < GP_OK)
    ret = gp_widget_get_child_by_label (widget, key, child);
  return ret;
}

int
canon_enable_capture (Camera *camera, int onoff, GPContext *context) {
  CameraWidget    *widget = NULL, *child = NULL;
  CameraWidgetType  type;
  int     ret;

  ret = gp_camera_get_config (camera, &widget, context);
  if (ret < GP_OK) {
    fprintf (stderr, "camera_get_config failed: %d\n", ret);
    return ret;
  }
  ret = _lookup_widget (widget, "capture", &child);
  if (ret < GP_OK) {
    /*fprintf (stderr, "lookup widget failed: %d\n", ret);*/
    goto out;
  }

  ret = gp_widget_get_type (child, &type);
  if (ret < GP_OK) {
    fprintf (stderr, "widget get type failed: %d\n", ret);
    goto out;
  }
  switch (type) {
        case GP_WIDGET_TOGGLE:
    break;
  default:
    fprintf (stderr, "widget has bad type %d\n", type);
    ret = GP_ERROR_BAD_PARAMETERS;
    goto out;
  }
  /* Now set the toggle to the wanted value */
  ret = gp_widget_set_value (child, &onoff);
  if (ret < GP_OK) {
    fprintf (stderr, "toggling Canon capture to %d failed with %d\n", onoff, ret);
    goto out;
  }
  /* OK */
  ret = gp_camera_set_config (camera, widget, context);
  if (ret < GP_OK) {
    fprintf (stderr, "camera_set_config failed: %d\n", ret);
    return ret;
  }
out:
  gp_widget_free (widget);
  return ret;
}

GPContext* sample_create_context() {
  GPContext *context;

  /* This is the mandatory part */
  context = gp_context_new();


  return context;
}



void setcamera(){
  GPContext *context = sample_create_context();

 
  gp_camera_new(&camera);

  /* When I set GP_LOG_DEBUG instead of GP_LOG_ERROR above, I noticed that the
   * init function seems to traverse the entire filesystem on the camera.  This
   * is partly why it takes so long.
   * (Marcus: the ptp2 driver does this by default currently.)
   */
  printf("Camera init.  Takes about 10 seconds.\n");
  int retval = gp_camera_init(camera, context);
  if (retval != GP_OK) {
    printf("  Retval: %d\n", retval);
    exit (1);
  }
  canon_enable_capture(camera, TRUE, context);

}


void savetofile(char * buf,unsigned long int  size,int num)
{

  char filename[256];
  int iSize=size;
  snprintf(filename, 256, "capture_%d.jpg",num);
  printf("-Capturing to file %s size=%d\n", filename,iSize);
  FILE * fd;
  fd=fopen(filename,"w+b");
  int bytes=fwrite(buf, 1, size,fd);

  printf("|---Write %d bytes\n", bytes);

  fclose(fd);
}

void stream(char * buf,unsigned long int  size)
{
 


  struct timeval tvBegin, tvEnd, tvDiff;
  gettimeofday(&tvBegin, NULL);

printf("before send\n\r");
  n = sendto(sock,buf,size,
                  0,(struct sockaddr *)&server,length);
       if (n  < 0) printf("sendto");

printf("end send\n\r");
 gettimeofday(&tvEnd, NULL);
  // diff
  timeval_subtract(&tvDiff, &tvEnd, &tvBegin);
   printf("%ld.%06ld\n", tvDiff.tv_sec, tvDiff.tv_usec);
 


}



void* capture(void* arg)
{
  int res;
  int i = 0;
  int count=0;
  CameraFile* file;
  printf("Capture\r\n");
  
  
  while(!stop)
  {
    unsigned long int xsize;
    const char* xdata;

    


    res = gp_file_new(&file);
    if(res < 0){
      printf("Error gp_file_new\n\r");
      return;
    }
    res = gp_camera_capture_preview(camera, file, context);
    if(res < 0){
      printf("Error gp_camera_capture_preview\n\r");
      break;
    };

    res = gp_file_get_data_and_size(file, &xdata, &xsize);
    if(xsize == 0)
    {
      if(i++ > 3)
      {
        printf("Restarted too many times; giving up\n");
        break ;
      }
      int value = 0;
      printf("Read 0 bytes from camera; restarting it\n");
      camera_set("capture", &value);
      sleep(3);
      value = 1;
      camera_set("capture", &value);
    }
    else
      i = 0;
    if(res < 0){
      printf("Error Camera file and size\n\r");
      break;
    };
    
    

    //savetofile((char *)xdata,xsize,count++);
    stream((char *)xdata,xsize);

    

  

    res = gp_file_unref(file);
    


    usleep(delay);
  }


  return NULL;
}


static void
capture_to_memory(Camera *camera, GPContext *context, const char **ptr, unsigned long int *size) {
  int retval;
  CameraFile *file;
  CameraFilePath camera_file_path;

  printf("Capturing.\n");

  

  retval = gp_camera_capture(camera, GP_CAPTURE_IMAGE, &camera_file_path, context);
  printf("  Retval: %d\n", retval);

  printf("Pathname on the camera: %s/%s\n", camera_file_path.folder, camera_file_path.name);

  retval = gp_file_new(&file);
  printf("  Retval: %d\n", retval);
  retval = gp_camera_file_get(camera, camera_file_path.folder, camera_file_path.name,
         GP_FILE_TYPE_NORMAL, file, context);
  printf("  Retval: %d\n", retval);

  gp_file_get_data_and_size (file, ptr, size);
  savetofile((char *)(*ptr),*size,0);
  //stream((char *)(*ptr),*size);

  printf("Deleting.\n");
  retval = gp_camera_file_delete(camera, camera_file_path.folder, camera_file_path.name,
      context);
  printf("  Retval: %d\n", retval);

  gp_file_free(file);
}




void* getcommand(void* arg){

  int ret;
  char inbuff[1024*1024];
  printf("GetCommand\n\r"); 
  int insock, length, fromlen, n;
  struct sockaddr_in server;
  struct sockaddr_in from;



   insock=socket(AF_INET, SOCK_DGRAM, 0);
   if (insock < 0) printf("Error: Opening socket");
   length = sizeof(server);
   bzero(&server,length);
   server.sin_family=AF_INET;
   server.sin_addr.s_addr=INADDR_ANY;
   server.sin_port=htons(port);
   if (bind(insock,(struct sockaddr *)&server,length)<0) 
       printf("Error binding");
   fromlen = sizeof(struct sockaddr_in);
   while (!stop) {
    n = recvfrom(insock,&inbuff,1024,0,(struct sockaddr *)&from,&fromlen);
    if (n < 0) printf("recvfrom\n");
    printf("Received shot command\n\r");
    delay=10000000;
    usleep(100000);
    int value = 0;
    //printf("camera_set(0)\n\r");
    //camera_set("capture", &value);
    

    printf("scatto\n\r");
    
    unsigned long int imagesize;
    const char * buffer;
    capture_to_memory(camera, context, &buffer,&imagesize);
    printf("imagesize=%lu\n\r",imagesize);

      
    //gp_camera_capture(camera, GP_CAPTURE_IMAGE, &camera_file_path, context);
    sleep(1);
    value = 1;
    printf("camera_set(1)\n\r");
    camera_set("capture", &value);
    delay=30000;

       

   }



}

char * getstring(void) {
    char * line = malloc(100), * linep = line;
    size_t lenmax = 100, len = lenmax;
    int c;

    if(line == NULL)
        return NULL;

    for(;;) {
        c = fgetc(stdin);
        if(c == EOF)
            break;

        if(--len == 0) {
            len = lenmax;
            char * linen = realloc(linep, lenmax *= 2);

            if(linen == NULL) {
                free(linep);
                return NULL;
            }
            line = linen + (line - linep);
            linep = linen;
        }

        if((*line++ = c) == '\n')
            break;
    }
    *line = '\0';
    return linep;
}

int main (int argc, char *argv[]) {
  printf("Stream camera\n\r");
  if (argc != 3) { printf("Usage: server port\n");
                    exit(1);
   }
  createsocket(argv);
  setcamera();
  // starting thread
  if(pthread_create(&thread, 0, capture, NULL) != 0)
  {
    printf("could not start worker thread\n");
    exit(0);
  }

   if(pthread_create(&listenthread, 0, getcommand, NULL) != 0)
  {
    printf("could not start worker thread\n");
    exit(0);
  }

  printf("Press a key to stop capture\n");
  char * read=getstring();
  stop =1;
  printf("Exit\n\r");
  sleep(2);
  int value = 0;
              
   camera_set("capture", &value);

  pthread_detach(thread);

  gp_camera_exit(camera, context);
  gp_camera_unref(camera);
  gp_context_unref(context);

  printf("Exit\n\r");


 return 0;
}