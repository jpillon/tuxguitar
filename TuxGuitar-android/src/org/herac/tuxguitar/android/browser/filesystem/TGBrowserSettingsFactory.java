package org.herac.tuxguitar.android.browser.filesystem;

import org.herac.tuxguitar.android.browser.model.TGBrowserException;
import org.herac.tuxguitar.android.browser.model.TGBrowserFactorySettingsHandler;

public interface TGBrowserSettingsFactory {
	
	void createSettings(TGBrowserFactorySettingsHandler handler) throws TGBrowserException;
}
