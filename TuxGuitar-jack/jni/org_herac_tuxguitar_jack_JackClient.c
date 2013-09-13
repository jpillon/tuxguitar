#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include <jack/jack.h>
#include <jack/midiport.h>
#include "org_herac_tuxguitar_jack_JackClient.h"

#define EVENT_BUFFER_SIZE 512

typedef struct {
	int event_size;
	jack_port_t *event_port;
	jack_midi_data_t *event_data;
} jack_jni_event_t;

typedef struct {
	int event_count;
	int event_port_count;
	jack_jni_event_t event_queue[ EVENT_BUFFER_SIZE ];
	jack_port_t **event_ports;
} jack_jni_synth_t;

typedef struct {
	pthread_mutex_t lock;
	jack_client_t *client;
	jack_jni_synth_t *midi;
} jack_jni_handle_t;

int  JackProcessCallbackImpl(jack_nframes_t nframes, void *ptr);
void JackShutdownCallbackImpl(void *ptr);

JNIEXPORT jlong JNICALL Java_org_herac_tuxguitar_jack_JackClient_malloc(JNIEnv* env, jobject obj)
{
	jlong ptr = 0;
	
	jack_jni_handle_t *handle = (jack_jni_handle_t *) malloc( sizeof(jack_jni_handle_t) );
	handle->client = NULL;
	handle->midi = NULL;
	
	pthread_mutex_init( &handle->lock , NULL );
	
	memcpy(&ptr, &handle, sizeof( handle ));
	
	return ptr;
}

JNIEXPORT void JNICALL Java_org_herac_tuxguitar_jack_JackClient_free(JNIEnv* env, jobject obj, jlong ptr)
{
	jack_jni_handle_t *handle = NULL;
	memcpy(&handle, &ptr, sizeof(handle));
	if(handle != NULL){
		
		pthread_mutex_destroy( &handle->lock );
		
		free( handle );
	}
}

JNIEXPORT void JNICALL Java_org_herac_tuxguitar_jack_JackClient_open(JNIEnv* env, jobject obj, jlong ptr)
{
	jack_jni_handle_t *handle = NULL;
	memcpy(&handle, &ptr, sizeof(handle));
	if(handle != NULL){
		
		if( pthread_mutex_lock( &handle->lock ) == 0 ){
			
			if(handle->client == NULL)
			{
				handle->client = jack_client_open ("TuxGuitar", JackNoStartServer , NULL );
				if( handle->client != NULL ){
					jack_on_shutdown(handle->client, JackShutdownCallbackImpl, handle);
					jack_set_process_callback (handle->client, JackProcessCallbackImpl , handle);
					jack_activate (handle->client);
				}
				
				handle->midi = (jack_jni_synth_t *) malloc( sizeof(jack_jni_synth_t) );
				if( handle->midi != NULL ) {
					int index = 0;
					for(index = 0 ; index < EVENT_BUFFER_SIZE ; index ++){
						handle->midi->event_queue[ index ].event_size = 0;
						handle->midi->event_queue[ index ].event_port = NULL;
						handle->midi->event_queue[ index ].event_data = NULL;
					}
					handle->midi->event_count = 0;
					handle->midi->event_ports = NULL;
					handle->midi->event_port_count = 0;
				}
			}
			
			pthread_mutex_unlock( &handle->lock );
		}
	}
}

JNIEXPORT void JNICALL Java_org_herac_tuxguitar_jack_JackClient_close(JNIEnv* env, jobject obj, jlong ptr)
{
	jack_jni_handle_t *handle = NULL;
	memcpy(&handle, &ptr, sizeof(handle));
	if(handle != NULL){
		if( pthread_mutex_lock( &handle->lock ) == 0 ){
			
			if( handle->client != NULL)
			{
				jack_deactivate (handle->client);
				jack_client_close (handle->client);
				handle->client = NULL;
			}
			if( handle->midi != NULL )
			{
				if( handle->midi->event_ports != NULL )
				{
					free( handle->midi->event_ports );
					handle->midi->event_ports = NULL;
					handle->midi->event_port_count = 0;
				}
				free( handle->midi );
				handle->midi = NULL;
			}
			
			pthread_mutex_unlock( &handle->lock );
		}
	}
}

JNIEXPORT jlong JNICALL Java_org_herac_tuxguitar_jack_JackClient_openPort(JNIEnv* env, jobject obj, jlong ptr, jstring jack_port_name)
{
	jlong jack_port_id = 0;
	
	jack_jni_handle_t *handle = NULL;
	memcpy(&handle, &ptr, sizeof(handle));
	if(handle != NULL){
		if( pthread_mutex_lock( &handle->lock ) == 0 ){
			
			if( handle->client != NULL )
			{				
				char port_name[50];
				const char *port_name_value = (*env)->GetStringUTFChars(env, jack_port_name, 0);
				sprintf( port_name , "%s", port_name_value );
				(*env)->ReleaseStringUTFChars(env, jack_port_name, port_name_value);
				
				jack_port_t *jack_port = jack_port_register(handle->client, port_name, JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);
				if( jack_port != NULL ){
					jack_port_t **event_ports_aux = handle->midi->event_ports;
					int event_port_index = 0;
					int event_port_count = handle->midi->event_port_count;
					
					handle->midi->event_ports = (jack_port_t **) malloc( (event_port_count + 1) * sizeof(jack_port_t *) );
					handle->midi->event_port_count = 0;
					for( event_port_index = 0 ; event_port_index < event_port_count ; event_port_index ++ ){
						handle->midi->event_ports[handle->midi->event_port_count ++] = event_ports_aux[event_port_index];
					}
					handle->midi->event_ports[handle->midi->event_port_count ++] = jack_port;
					
					free( event_ports_aux );
					event_ports_aux = NULL;
					
					memcpy(&jack_port_id, &jack_port, sizeof( jack_port ));
				}
			}
			pthread_mutex_unlock( &handle->lock );
		}
	}
	
	return jack_port_id;
}

JNIEXPORT void JNICALL Java_org_herac_tuxguitar_jack_JackClient_closePort(JNIEnv* env, jobject obj, jlong ptr, jlong jack_port_id)
{
	jack_jni_handle_t *handle = NULL;
	memcpy(&handle, &ptr, sizeof(handle));
	if(handle != NULL){
		if( pthread_mutex_lock( &handle->lock ) == 0 ){
			
			if(handle->client != NULL)
			{
				jack_port_t *jack_port = NULL;
				memcpy(&jack_port, &jack_port_id, sizeof(jack_port));
				if( jack_port != NULL )
				{
					int event_index = 0;
					for(event_index = 0 ; event_index < EVENT_BUFFER_SIZE ; event_index ++){
						if( handle->midi->event_queue[ event_index ].event_port == jack_port ) {
							free ( handle->midi->event_queue[ event_index ].event_data );
							
							handle->midi->event_queue[ event_index ].event_data = NULL;
							handle->midi->event_queue[ event_index ].event_port = NULL;
							handle->midi->event_queue[ event_index ].event_size = 0;
						}
					}
					
					if( handle->midi->event_port_count > 0 )
					{
						jack_port_t **event_ports_aux = handle->midi->event_ports;
						int event_port_index = 0;
						int event_port_count = handle->midi->event_port_count;
						
						handle->midi->event_ports = (jack_port_t **) malloc( (event_port_count - 1) * sizeof(jack_port_t *) );
						handle->midi->event_port_count = 0;
						for(event_port_index = 0 ; event_port_index < event_port_count ; event_port_index ++){
							if( event_ports_aux[event_port_index] != jack_port ) {
								handle->midi->event_ports[handle->midi->event_port_count ++] = event_ports_aux[event_port_index];
							}
						}
						free( event_ports_aux );
						event_ports_aux = NULL;
					}

					jack_port_unregister(handle->client, jack_port );
				}
			}
			pthread_mutex_unlock( &handle->lock );
		}
	}
}

JNIEXPORT jlong JNICALL Java_org_herac_tuxguitar_jack_JackClient_getTransportUID(JNIEnv* env, jobject obj, jlong ptr)
{
	jlong result = 0;
	
	jack_jni_handle_t *handle = NULL;
	memcpy(&handle, &ptr, sizeof(handle));
	if(handle != NULL){
		if( pthread_mutex_lock( &handle->lock ) == 0 ){
			
			if(handle->client != NULL)
			{
				jack_position_t pos;
				jack_transport_query( handle->client , &pos );
				
				result = pos.unique_1;
			}
			pthread_mutex_unlock( &handle->lock );
		}
	}
	return result;
}

JNIEXPORT jlong JNICALL Java_org_herac_tuxguitar_jack_JackClient_getTransportFrame(JNIEnv* env, jobject obj, jlong ptr)
{
	jlong result = 0;
	
	jack_jni_handle_t *handle = NULL;
	memcpy(&handle, &ptr, sizeof(handle));
	if(handle != NULL){
		if( pthread_mutex_lock( &handle->lock ) == 0 ){
			
			if(handle->client != NULL)
			{
				jack_position_t pos;
				jack_transport_query( handle->client , &pos );
				
				result = pos.frame;
			}
			pthread_mutex_unlock( &handle->lock );
		}
	}
	return result;
}

JNIEXPORT jlong JNICALL Java_org_herac_tuxguitar_jack_JackClient_getTransportFrameRate(JNIEnv* env, jobject obj, jlong ptr)
{
	jlong result = 0;
	
	jack_jni_handle_t *handle = NULL;
	memcpy(&handle, &ptr, sizeof(handle));
	if(handle != NULL){
		if( pthread_mutex_lock( &handle->lock ) == 0 ){
			
			if(handle->client != NULL)
			{
				jack_position_t pos;
				jack_transport_query( handle->client , &pos );
				
				result = pos.frame_rate;
			}
			pthread_mutex_unlock( &handle->lock );
		}
	}
	return result;
}

JNIEXPORT void JNICALL Java_org_herac_tuxguitar_jack_JackClient_setTransportFrame(JNIEnv* env, jobject obj, jlong ptr, jlong frame)
{
	jack_jni_handle_t *handle = NULL;
	memcpy(&handle, &ptr, sizeof(handle));
	if(handle != NULL){
		if( pthread_mutex_lock( &handle->lock ) == 0 ){
			
			if(handle->client != NULL)
			{
				jack_transport_locate( handle->client, (jack_nframes_t) frame );
			}
			
			pthread_mutex_unlock( &handle->lock );
		}
	}
}

JNIEXPORT void JNICALL Java_org_herac_tuxguitar_jack_JackClient_setTransportStart(JNIEnv* env, jobject obj, jlong ptr)
{
	jack_jni_handle_t *handle = NULL;
	memcpy(&handle, &ptr, sizeof(handle));
	if(handle != NULL){
		if( pthread_mutex_lock( &handle->lock ) == 0 ){
			
			if(handle->client != NULL)
			{
				jack_position_t pos;
				jack_transport_state_t state = jack_transport_query( handle->client , &pos );
				if( state == JackTransportStopped ) {
					jack_transport_start( handle->client );
				}
			}
			pthread_mutex_unlock( &handle->lock );
		}
	}
}

JNIEXPORT void JNICALL Java_org_herac_tuxguitar_jack_JackClient_setTransportStop(JNIEnv* env, jobject obj, jlong ptr)
{
	jack_jni_handle_t *handle = NULL;
	memcpy(&handle, &ptr, sizeof(handle));
	if(handle != NULL){
		if( pthread_mutex_lock( &handle->lock ) == 0 ){
			
			if(handle->client != NULL)
			{
				jack_position_t pos;
				jack_transport_state_t state = jack_transport_query( handle->client , &pos );
				if( state != JackTransportStopped ) {
					jack_transport_stop( handle->client );
				}
			}
			pthread_mutex_unlock( &handle->lock );
		}
	}
}

JNIEXPORT jboolean JNICALL Java_org_herac_tuxguitar_jack_JackClient_isTransportRunning(JNIEnv* env, jobject obj, jlong ptr)
{
	jboolean result = JNI_FALSE;
	
	jack_jni_handle_t *handle = NULL;
	memcpy(&handle, &ptr, sizeof(handle));
	if(handle != NULL){
		if( pthread_mutex_lock( &handle->lock ) == 0 ){
			
			if(handle->client != NULL)
			{
				jack_position_t pos;
				jack_transport_state_t state = jack_transport_query( handle->client , &pos );
				if( state != JackTransportStopped ) {
					result = JNI_TRUE;
				}
			}
			pthread_mutex_unlock( &handle->lock );
		}
	}
	return result;
}

JNIEXPORT jboolean JNICALL Java_org_herac_tuxguitar_jack_JackClient_isOpen(JNIEnv* env, jobject obj, jlong ptr)
{
	jboolean result = JNI_FALSE;
	
	jack_jni_handle_t *handle = NULL;
	memcpy(&handle, &ptr, sizeof(handle));
	if(handle != NULL){
		if( pthread_mutex_lock( &handle->lock ) == 0 ){
			
			if( handle->client != NULL ){
				result = JNI_TRUE;
			}
			
			pthread_mutex_unlock( &handle->lock );
		}
	}
	return result;
}

JNIEXPORT jboolean JNICALL Java_org_herac_tuxguitar_jack_JackClient_isPortOpen(JNIEnv * env, jobject obj, jlong ptr, jlong jack_port_id)
{
	jboolean result = JNI_FALSE;
	
	jack_jni_handle_t *handle = NULL;
	memcpy(&handle, &ptr, sizeof(handle));
	if(handle != NULL){
		if( pthread_mutex_lock( &handle->lock ) == 0 ){
			
			if( handle->client != NULL && handle->midi != NULL ) {
				jack_port_t *jack_port = NULL;
				memcpy(&jack_port, &jack_port_id, sizeof(jack_port));
				
				if( jack_port != NULL && handle->midi->event_ports != NULL ){
					int index = 0;
					int count = handle->midi->event_port_count;
					
					for( index = 0 ; index < count ; index ++ ){
						if( handle->midi->event_ports[index] == jack_port ) {
							result = JNI_TRUE;
						}
					}
				}
			}
			
			pthread_mutex_unlock( &handle->lock );
		}
	}
	return result;
}

JNIEXPORT void JNICALL Java_org_herac_tuxguitar_jack_JackClient_addEventToQueue(JNIEnv* env, jobject obj, jlong ptr, jlong jack_port_id, jbyteArray jdata)
{
	
	jack_jni_handle_t *handle = NULL;
	memcpy(&handle, &ptr, sizeof(handle));
	if(handle != NULL){
		if( pthread_mutex_lock( &handle->lock ) == 0 ){
			
			if(handle->client != NULL && handle->midi != NULL )
			{
				jack_port_t *jack_port = NULL;
				memcpy(&jack_port, &jack_port_id, sizeof(jack_port));
				
				if( handle->midi->event_count < EVENT_BUFFER_SIZE ) {
					
					int count = (*env)->GetArrayLength( env,  jdata  );
					if( count > 0 ){
						jbyte* jdataArray = (*env)->GetByteArrayElements( env , jdata, 0);
						if( jdataArray != NULL ) {
							if( handle->midi->event_queue[ handle->midi->event_count ].event_data != NULL ) {
								free ( handle->midi->event_queue[ handle->midi->event_count ].event_data );
								handle->midi->event_queue[ handle->midi->event_count ].event_data = NULL;
							}
							handle->midi->event_queue[ handle->midi->event_count ].event_port = jack_port;
							handle->midi->event_queue[ handle->midi->event_count ].event_size = count;
							handle->midi->event_queue[ handle->midi->event_count ].event_data = (jack_midi_data_t *)malloc( sizeof( jack_midi_data_t ) * count );
							if( handle->midi->event_queue[ handle->midi->event_count ].event_data != NULL ) {
								int index = 0;
								for( index = 0 ; index < count ; index ++ ){
									handle->midi->event_queue[ handle->midi->event_count ].event_data[ index ] = (jack_midi_data_t)jdataArray[ index ];
								}
								handle->midi->event_count ++;
								(*env)->ReleaseByteArrayElements( env , jdata, jdataArray, 0);
							}
						}
					}
					
				}
			}
			pthread_mutex_unlock( &handle->lock );
		}
	}
}

int JackProcessCallbackImpl(jack_nframes_t nframes, void *ptr){
	jack_jni_handle_t *handle = NULL;
	memcpy(&handle, &ptr, sizeof(handle));
	if(handle != NULL){
		
		if( pthread_mutex_trylock( &handle->lock ) == 0 ){
			
			if(handle->client != NULL && handle->midi != NULL )
			{
				if( handle->midi->event_ports != NULL ){
					int index = 0;
					int count = handle->midi->event_port_count;
					
					for( index = 0 ; index < count ; index ++ ){
						void *buffer = jack_port_get_buffer(handle->midi->event_ports[index], jack_get_buffer_size(handle->client) );
						if( buffer != NULL ){
							jack_midi_clear_buffer( buffer );
						}
					}
				}
				
				if( handle->midi->event_count > 0 ) {
					int index = 0;
					int count = handle->midi->event_count;
					for( index = 0 ; index < count ; index ++ ){
						void *buffer = jack_port_get_buffer(handle->midi->event_queue[index].event_port, jack_get_buffer_size(handle->client) );
						
						if( buffer != NULL ){
							jack_midi_data_t *data = jack_midi_event_reserve ( buffer , 0, handle->midi->event_queue[index].event_size);
							if( data != NULL ){
								int data_index = 0;
								for( data_index = 0 ; data_index < handle->midi->event_queue[index].event_size ; data_index ++ ){
									data[ data_index ] = handle->midi->event_queue[index].event_data[ data_index ];
								}
							}
						}
						
						handle->midi->event_count --;
						handle->midi->event_queue[index].event_size = 0;
					}
				}
			}
			
			pthread_mutex_unlock( &handle->lock );
		}
	}
	return 0;
}

void JackShutdownCallbackImpl(void *ptr)
{
	jack_jni_handle_t *handle = NULL;
	memcpy(&handle, &ptr, sizeof(handle));
	if(handle != NULL){
		if( pthread_mutex_lock( &handle->lock ) == 0 ) {
			
			handle->client = NULL;
			
			if( handle->midi != NULL ) {
				if( handle->midi->event_ports != NULL ) {
					free( handle->midi->event_ports );
					handle->midi->event_ports = NULL;
					handle->midi->event_port_count = 0;
				}
				free( handle->midi );
				handle->midi = NULL;
			}
			
			pthread_mutex_unlock( &handle->lock );
		}
	}
}
