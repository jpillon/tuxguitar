package swtimpl.event;

import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseListener;
import org.eclipse.swt.events.MouseMoveListener;
import org.herac.tuxguitar.ui.event.UIMouseDragListenerManager;
import org.herac.tuxguitar.ui.event.UIMouseEvent;
import org.herac.tuxguitar.ui.resource.UIPosition;

import swtimpl.SWTComponent;

public class SWTMouseDragListenerManager extends UIMouseDragListenerManager implements MouseListener, MouseMoveListener {
	
	private SWTComponent<?> control;
	private UIPosition startPosition;
	
	public SWTMouseDragListenerManager(SWTComponent<?> control) {
		this.control = control;
	}
	
	public void mouseMove(MouseEvent e) {
		if( this.startPosition != null ) {
			this.onMouseDrag(new UIMouseEvent(this.control, new UIPosition(e.x - this.startPosition.getX(), e.y - this.startPosition.getY()), e.button));
		}
	}
	
	public void mouseDown(MouseEvent e) {
		this.startPosition = new UIPosition(e.x, e.y);
	}

	public void mouseUp(MouseEvent e) {
		this.startPosition = null;
	}
	
	public void mouseDoubleClick(MouseEvent e) {
		// nothing to do
	}
}
