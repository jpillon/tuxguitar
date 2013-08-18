package org.herac.tuxguitar.jack.sequencer;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

import org.herac.tuxguitar.jack.provider.JackClientProvider;
import org.herac.tuxguitar.player.base.MidiPlayerException;
import org.herac.tuxguitar.player.base.MidiSequencer;
import org.herac.tuxguitar.player.base.MidiSequencerProvider;

public class JackSequencerProvider implements MidiSequencerProvider{
	
	private List jackSequencerProviders;
	private JackClientProvider jackClientProvider;
	
	public JackSequencerProvider(JackClientProvider jackClientProvider){
		this.jackClientProvider = jackClientProvider;
	}
	
	public List listSequencers() throws MidiPlayerException {
		if( this.jackSequencerProviders == null ){
			this.jackSequencerProviders = new ArrayList();
			this.jackSequencerProviders.add(new JackSequencer(this.jackClientProvider.getJackClient()));
		}
		return this.jackSequencerProviders;
	}
	
	public void closeAll() throws MidiPlayerException {
		Iterator it = listSequencers().iterator();
		while(it.hasNext()){
			MidiSequencer sequencer = (MidiSequencer)it.next();
			sequencer.close();
		}
	}
	
}
