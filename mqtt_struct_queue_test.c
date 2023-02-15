#include "mqtt_struct_queue.c"

void main() {
    Queue *queue = initQueue();
    
    char *s1 = "string1";
    char *s2 = "string2";
    
    char *s3 = "stringa";
    char *s4 = "stringb";
    
    printf("eq b %d\n", getElementCount(queue));
    enqueue(queue, s1, s2);
    printf("eq a %d\n", getElementCount(queue));

    printf("dq b %d\n", getElementCount(queue));
    dequeue(queue, &s3, &s4);
    printf("dq a %d\n", getElementCount(queue));

    printf("eq b %d\n", getElementCount(queue));
    enqueue(queue, s1, s2);
    printf("eq a %d\n", getElementCount(queue));

    printf("dq b %d\n", getElementCount(queue));
    dequeue(queue, &s3, &s4);
    printf("dq a %d\n", getElementCount(queue));

}