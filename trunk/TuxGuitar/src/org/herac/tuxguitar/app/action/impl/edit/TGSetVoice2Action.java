package org.herac.tuxguitar.app.action.impl.edit;

import org.herac.tuxguitar.action.TGActionContext;
import org.herac.tuxguitar.app.view.component.tab.TablatureEditor;
import org.herac.tuxguitar.editor.action.TGActionBase;
import org.herac.tuxguitar.util.TGContext;

public class TGSetVoice2Action extends TGActionBase{
	
	public static final String NAME = "action.edit.voice-2";
	
	public TGSetVoice2Action(TGContext context) {
		super(context, NAME);
	}
	
	protected void processAction(TGActionContext context){
		TablatureEditor.getInstance(getContext()).getTablature().getCaret().setVoice(1);
	}
}
