package org.herac.tuxguitar.io.lilypond;

import org.herac.tuxguitar.app.io.TGSongStreamSettingsHandler;
import org.herac.tuxguitar.document.TGDocumentContextAttributes;
import org.herac.tuxguitar.io.base.TGSongStreamContext;
import org.herac.tuxguitar.song.models.TGSong;
import org.herac.tuxguitar.util.TGContext;

public class LilypondSettingsHandler implements TGSongStreamSettingsHandler {

	private TGContext context;
	
	public LilypondSettingsHandler(TGContext context)  {
		this.context = context;
	}
	
	public String getProviderId() {
		return LilypondSongExporter.PROVIDER_ID;
	}

	public void handleSettings(final TGSongStreamContext context, final Runnable callback) {
		final TGSong song = context.getAttribute(TGDocumentContextAttributes.ATTRIBUTE_SONG);
		final LilypondSettings settings = LilypondSettings.getDefaults();
		
		new LilypondSettingsDialog(this.context, song).open(settings, new Runnable() {
			public void run() {
				if( settings != null ) {
					context.setAttribute(LilypondSettings.class.getName(), settings);
					callback.run();
				}
			}
		});
	}	
}