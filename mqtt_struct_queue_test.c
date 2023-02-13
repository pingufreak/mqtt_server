#include "mqtt_struct_queue.c"

void main() {
    Queue *queue = initQueue();
    
    char *s1 = "string1";
    char *s2 = "string2";
    
    char *s3 = "stringa";
    char *s4 = "stringb";
    
    enqueue(queue, s1, s2);

printf("%p %p\n", s1, s3);
    dequeue(queue, &s3, &s4);
printf("%p %p\n", s1, s3);

    printf("%s %s\n", s1, s2);

    printf("%s %s\n", s3, s4);
}