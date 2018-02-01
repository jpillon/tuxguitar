#include <stdio.h>
#include <stdlib.h>
#include <aeffectx.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <pthread.h>
#include "org_herac_tuxguitar_player_impl_midiport_vst_jni_VSTEffectUI.h"
#include "org_herac_tuxguitar_player_impl_midiport_vst_jni_VST.h"

typedef struct {
	Display *dpy;
	JNIEffect* effect;
	jboolean editorOpen;
	jboolean editorProcessRunning;
} JNIEffectUI;

void* JNIEffectUI_openProcess(void* handle);

/*
 * Class:     org_herac_tuxguitar_player_impl_midiport_vst_jni_VSTEffectUI
 * Method:    malloc
 * Signature: (J)J
 */
JNIEXPORT jlong JNICALL Java_org_herac_tuxguitar_player_impl_midiport_vst_jni_VSTEffectUI_malloc(JNIEnv *env, jobject obj, jlong ptr)
{
	jlong jptr = 0;
	
	JNIEffect *effect = NULL;
	memcpy(&effect, &ptr, sizeof(effect));
	if( effect != NULL ){
		
		JNIEffectUI *handle = (JNIEffectUI *) malloc( sizeof(JNIEffectUI) );
		
		handle->dpy = XOpenDisplay(NULL);
		handle->effect = effect;
		handle->editorOpen = JNI_FALSE;
		handle->editorProcessRunning = JNI_FALSE;
		
		memcpy(&jptr, &handle, sizeof( handle ));
	}
	
	return jptr;
}

/*
 * Class:     org_herac_tuxguitar_player_impl_midiport_vst_jni_VSTEffectUI
 * Method:    delete
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_org_herac_tuxguitar_player_impl_midiport_vst_jni_VSTEffectUI_delete(JNIEnv *env, jobject obj, jlong ptr)
{
	JNIEffectUI *handle = NULL;
	memcpy(&handle, &ptr, sizeof(handle));
	if(handle != NULL){
		if( handle->effect != NULL){
			handle->effect = NULL;
		}
		if( handle->dpy != NULL){
			XCloseDisplay(handle->dpy);
			handle->dpy = NULL;
		}
		free ( handle );
	}
}

/*
 * Class:     org_herac_tuxguitar_player_impl_midiport_vst_jni_VSTEffectUI
 * Method:    openEditor
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_org_herac_tuxguitar_player_impl_midiport_vst_jni_VSTEffectUI_openEditor(JNIEnv *env, jobject obj, jlong ptr)
{
	JNIEffectUI *handle = NULL;
	memcpy(&handle, &ptr, sizeof(handle));
	if( handle != NULL && handle->editorOpen != JNI_TRUE ){
		handle->editorOpen = JNI_TRUE;
		
		pthread_t thread;
		if( pthread_create(&thread, NULL, JNIEffectUI_openProcess, handle)) {
			handle->editorOpen = JNI_FALSE;
		}
	}
}

/*
 * Class:     org_herac_tuxguitar_player_impl_midiport_vst_jni_VSTEffectUI
 * Method:    closeEditor
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_org_herac_tuxguitar_player_impl_midiport_vst_jni_VSTEffectUI_closeEditor(JNIEnv *env, jobject obj, jlong ptr)
{
	JNIEffectUI *handle = NULL;
	memcpy(&handle, &ptr, sizeof(handle));
	if( handle != NULL && handle->effect != NULL ) {
		handle->editorOpen = JNI_FALSE;
	}
	
	while(handle->editorProcessRunning == JNI_TRUE) {
		// wait for end...
	}
}

/*
 * Class:     org_herac_tuxguitar_player_impl_midiport_vst_jni_VSTEffectUI
 * Method:    isEditorOpen
 * Signature: (J)Z
 */
JNIEXPORT jboolean JNICALL Java_org_herac_tuxguitar_player_impl_midiport_vst_jni_VSTEffectUI_isEditorOpen(JNIEnv *env, jobject obj, jlong ptr)
{
	JNIEffectUI *handle = NULL;
	memcpy(&handle, &ptr, sizeof(handle));
	if( handle != NULL && handle->effect != NULL ){
		return handle->editorOpen;
	}
	return JNI_FALSE;
}

/*
 * Class:     org_herac_tuxguitar_player_impl_midiport_vst_jni_VSTEffectUI
 * Method:    isEditorAvailable
 * Signature: (J)Z
 */
JNIEXPORT jboolean JNICALL Java_org_herac_tuxguitar_player_impl_midiport_vst_jni_VSTEffectUI_isEditorAvailable(JNIEnv *env, jobject obj, jlong ptr)
{
	JNIEffectUI *handle = NULL;
	memcpy(&handle, &ptr, sizeof(handle));
	if( handle != NULL && handle->effect != NULL && handle->effect->effect != NULL ){
		return ((handle->effect->effect->flags & effFlagsHasEditor) != 0);
	}
	return JNI_FALSE;
}

void* JNIEffectUI_openProcess(void* ptr)
{
	JNIEffectUI *handle = NULL;
	memcpy(&handle, &ptr, sizeof(handle));
	if( handle != NULL && handle->effect != NULL && handle->effect->effect != NULL ) {
		handle->editorProcessRunning = JNI_TRUE;
		
		Window win = XCreateSimpleWindow(handle->dpy, DefaultRootWindow(handle->dpy), 0, 0, 300, 300, 0, 0, 0);
		
		// ------------------------------------------------------------ //
		Atom wmDeleteMessage = XInternAtom(handle->dpy, "WM_DELETE_WINDOW", false);
		XSetWMProtocols(handle->dpy, win, &wmDeleteMessage, 1);
		
		Atom windowTypeProp = XInternAtom(handle->dpy, "_NET_WM_WINDOW_TYPE", False);
		Atom windowTypeValue = XInternAtom(handle->dpy, "_NET_WM_WINDOW_TYPE_DIALOG", False);
		XChangeProperty(handle->dpy, win, windowTypeProp, XA_ATOM, 32, PropModeReplace, (unsigned char *)&windowTypeValue, 1);

		Atom stateProp = XInternAtom(handle->dpy, "_NET_WM_STATE", False);
		Atom stateValue = XInternAtom(handle->dpy, "_NET_WM_STATE_ABOVE", False);
		XChangeProperty(handle->dpy, win, stateProp, XA_ATOM, 32, PropModeReplace, (unsigned char *) &stateValue, 1);

		// ------------------------------------------------------------ //
		char effect_name[256];
		handle->effect->effect->dispatcher(handle->effect->effect, effGetEffectName, 0, 0, effect_name, 0);
		strcat(effect_name, " [TuxGuitar]");
		XStoreName(handle->dpy, win, effect_name);

		// ------------------------------------------------------------ //
		ERect* eRect = 0;
		handle->effect->effect->dispatcher (handle->effect->effect, effEditGetRect, 0, 0, &eRect, 0);
		if (eRect) {
			int width = eRect->right - eRect->left;
			int height = eRect->bottom - eRect->top;
			
			XSizeHints hHints;
			hHints.min_width  = width;
			hHints.min_height  = height;
			hHints.max_width = width;
			hHints.max_height = height;
			hHints.flags = USSize | PSize | PMinSize | PMaxSize;
			XSetWMSizeHints(handle->dpy, win, &hHints, (Atom) USSize | PSize | PMinSize | PMaxSize );
			XSetNormalHints(handle->dpy, win, &hHints );
			XResizeWindow(handle->dpy, win, width, height);
		}

		// ------------------------------------------------------------ //
		XMapWindow(handle->dpy, win);
		XFlush(handle->dpy);
		handle->effect->effect->dispatcher (handle->effect->effect, effEditOpen, 0, (VstIntPtr) handle->dpy, (void*) win, 0);
		
		// ------------------------------------------------------------ //
		XEvent event;
		while(handle->editorOpen == JNI_TRUE) {
			if (XPending(handle->dpy)) {
				XNextEvent(handle->dpy, &event);
				if (event.type == ClientMessage) {
					if ((Atom)event.xclient.data.l[0] == wmDeleteMessage) {
						handle->editorOpen = JNI_FALSE;
					}
				}
			}
		}
		
		// ------------------------------------------------------------ //
		handle->effect->effect->dispatcher (handle->effect->effect, effEditClose, 0, 0, NULL, 0);
		
		XDestroyWindow(handle->dpy, win);
		XFlush(handle->dpy);
		
		handle->editorProcessRunning = JNI_FALSE;
	}
	return NULL;
}
