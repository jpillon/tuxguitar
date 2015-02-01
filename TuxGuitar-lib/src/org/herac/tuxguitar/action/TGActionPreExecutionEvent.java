package org.herac.tuxguitar.action;

import org.herac.tuxguitar.event.TGEvent;

public class TGActionPreExecutionEvent extends TGEvent {
	
	public static final String EVENT_TYPE = "action-pre-execution";
	
	public static final String PROPERTY_ACTION_ID = "actionId";
	public static final String PROPERTY_ACTION_CONTEXT = "actionContext";
	
	public TGActionPreExecutionEvent(String actionId, TGActionContext actionContext) {
		super(EVENT_TYPE);
		
		this.setProperty(PROPERTY_ACTION_ID, actionId);
		this.setProperty(PROPERTY_ACTION_CONTEXT, actionContext);
	}
}
