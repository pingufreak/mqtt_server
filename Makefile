mqtt_server mqtt_server.c:
	gcc mqtt_server.c -o mqtt_server -lpthread
mqtt_client_sub mqtt_client_sub.c:
	gcc mqtt_client_sub.c -o mqtt_client_sub
mqtt_client_pub mqtt_client_pub.c:
	gcc mqtt_client_pub.c -o mqtt_client_pub
mqtt_semaphore_test.c:
	gcc mqtt_semaphore_test.c -o mqtt_semaphore_test
mqtt_struct_queue.c:
	gcc mqtt_struct_queue.c -o mqtt_struct_queue